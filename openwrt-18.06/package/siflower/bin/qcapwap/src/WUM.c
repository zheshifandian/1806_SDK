#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/un.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/uio.h>
#include <json-c/json.h>

#include "ac_interface.h"
#include "CWLog.h"

typedef int (*cmd_fn)(void *);

int gLoggingLevel = DEFAULT_LOGGING_LEVEL;
int gEnabledLog = 1;
static struct sockaddr_un server, client;


void usage(const char *name)
{
	printf("Compile time:%s %s\n", __DATE__, __TIME__);
	printf("%s -c command \n", name);
	printf("\nAvailable commands:\n");
	printf("  quit: exit AC.\n");
	printf(" stations: print connected stations in logread\n");
}

struct json_object *json_object_object_get_old(struct json_object *obj, const char *name)
{
	struct json_object *sub;
	return json_object_object_get_ex(obj, name, &sub) ? sub : NULL;
}

static const char *get_string_of_json_key(json_object *json, const char *key)
{
	struct json_object *key_obj = json_object_object_get_old(json, key);
	if (!key_obj)
		return NULL;

	return json_object_get_string(key_obj);
}

static int connect_to_dev(const char *mac)
{
	struct timeval t;
	int sock;
	int yes = 1;

	memset(&server, 0, sizeof(server));
	memset(&client, 0, sizeof(client));
	server.sun_family = AF_LOCAL;
	snprintf(server.sun_path, sizeof(server.sun_path), "/tmp/WTP.%s", mac);
	client.sun_family = AF_LOCAL;
	snprintf(client.sun_path, sizeof(client.sun_path), "/tmp/WUM.%s", mac);

	sock = socket(AF_LOCAL, SOCK_DGRAM, 0);
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
	unlink(client.sun_path);
	if (bind(sock, (struct sockaddr *)&client, sizeof(client)) < 0) {
		close(sock);
		perror("bind");
		return -EBUSY;
	}
	if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
		close(sock);
		perror("connect");
		return -errno;
	}
	t.tv_sec = 5;
	t.tv_usec = 0;
	if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &t, sizeof(t)) < 0) {
		close(sock);
		perror("set timeout");
		return -errno;
	}
	// printf("connect to %s\n", server.sun_path);
	return sock;
}

int send_message(int server, struct capwap_interface_message *msg, void *data, int len)
{
	struct iovec io[2];

	io[0].iov_base = msg;
	io[0].iov_len = sizeof(*msg);
	io[1].iov_base = data;
	io[1].iov_len = len;
	if (writev(server, io, 2) < 0) {
		return -errno;
	}
	return 0;
}

// Send ping cmd to ac interface, to check whether the sock is alive.
// Return 1 if we receive a PONG, return 0 otherwise.
int ping(int sock)
{
	struct capwap_interface_message ping;
	int err;

	ping.cmd = PING_CMD;
	ping.type = 0;
	ping.length = 0;
	if ((err = send_message(sock, &ping, NULL, 0)))
		return 0;
	err = recv(sock, &ping, sizeof(ping), 0);
	if (err < 0 || ping.cmd != PONG_CMD || ping.type)
		return 0;
	return 1;
}

int receive_result(int sock)
{
	struct capwap_interface_message if_msg = {0};
	struct iovec io[2];
	void *buffer;
	int buffer_len = 1024, recv_len = 0;
	int result = 0;
	int len;

	buffer = malloc(buffer_len);
	if (!buffer)
		return ENOMEM;
	io[0].iov_base = &if_msg;
	io[0].iov_len = sizeof(if_msg);
	io[1].iov_base = buffer;
	io[1].iov_len = buffer_len;
	do {
		recv_len = readv(sock, io, 2);
		if (recv_len < 0) {
			if (ping(sock))
				continue;
			perror("Read result");
			return errno;
		} else if (len == 0)
			return EFAULT;

		if (if_msg.type == MSG_TYPE_STRING) {
			printf("%s\n", (char *)buffer);
		} else if (if_msg.type == MSG_TYPE_RESULT) {
			len = if_msg.length < sizeof(result) ? if_msg.length : sizeof(result);
			memcpy(&result, buffer, len);
		}
	} while (if_msg.cmd != MSG_END_CMD);

	return result;
}

static const char *get_server_name(const char *dev, const char *command)
{
	if (!dev)
		return AC_MAIN_MAC;
	if (strcmp(command, "delete_device") == 0 ||
	    strcmp(command, "set_group_name") == 0)
		return AC_MAIN_MAC;
	return dev;
}

static int do_command(int cmd, const char *server_path, void *data, int len)
{
	struct capwap_interface_message msg = {0};
	int server, err;

	msg.cmd = cmd;
	msg.length = len;
	server = connect_to_dev(server_path);
	if (server == -ENOENT || server == -ECONNREFUSED)
		server = connect_to_dev(AC_MAIN_MAC);
	if (server < 0)
		return server;
	if (send_message(server, &msg, data, len) < 0) {
		close(server);
		unlink(client.sun_path);
		return EFAULT;
	}
	err = receive_result(server);
	close(server);
	unlink(client.sun_path);
	return err;
}

int do_json_cmd(char *json_msg)
{
	json_object *msg_obj;
	const char *command;
	const char *dev;

	msg_obj = json_tokener_parse(json_msg);
	if (is_error(msg_obj))
		return EINVAL;

	dev = get_string_of_json_key(msg_obj, "device");
	command = get_string_of_json_key(msg_obj, "command");
	return do_command(JSON_CMD, get_server_name(dev, command), json_msg, strlen(json_msg) + 1);
}

static int do_simple_command(int cmd)
{
	return do_command(cmd, AC_MAIN_MAC, NULL, 0);
}

int ac_quit()
{
	return do_simple_command(QUIT_CMD);
}

int print_stations()
{
	return do_simple_command(PRINT_STATIONS);
}

int main(int argc, char *argv[])
{
	int c = -1, err;
	char *json_msg = NULL;
	char *command = NULL;

	while ((c = getopt(argc, argv, "ha:p:w:c:f:n:s:r:l:t:m:j:")) != -1)
		switch (c) {
		case 'j':
			json_msg = optarg;
			break;
		case 'c':
			command = optarg;
			break;
		case 'h':
			usage(argv[0]);
			return 0;
		default:
			usage(argv[0]);
			abort();
		}

	if (json_msg) {
		err = do_json_cmd(json_msg);
	} else if (command) {
		if (strncmp(command, "quit", 4) == 0) {
			err = ac_quit();
		} else if (strncmp(command, "stations", 8) == 0) {
			err = print_stations();
		} else {
			printf("Command not support\n");
			usage(argv[0]);
			return -1;
		}
	} else {
		usage(argv[0]);
		return -1;
	}
	err = err < 0 ? -err : err;
	printf("result(%d): %s\n", err, strerror(err));
	return err;
}
