#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/un.h>
#include <unistd.h>
#include <fcntl.h>

#include "network.h"
#include "capwap_message.h"
#include "CWLog.h"

// an extension to Unix Network Programming's library: a thread-safe version of sock_ntop and a
// couple of functions to manage port number in IPv4/6

/* include sock_ntop */
char *sock_ntop_r(const struct sockaddr_storage *sa, char *str)
{
	char portstr[8] = {0};

	switch (sa->ss_family) {
	case AF_INET: {
		struct sockaddr_in *sin = (struct sockaddr_in *)sa;

		if (inet_ntop(AF_INET, &sin->sin_addr, str, INET_ADDRSTRLEN) == 0)
			return (NULL);
		if (ntohs(sin->sin_port) != 0) {
			snprintf(portstr, sizeof(portstr), ":%d", ntohs(sin->sin_port));
			strcat(str, portstr);
		}
		return (str);
	}
	case AF_INET6: {
		struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)sa;

		str[0] = '[';
		if (inet_ntop(AF_INET6, &sin6->sin6_addr, str + 1, INET6_ADDRSTRLEN) == NULL)
			return (NULL);
		if (ntohs(sin6->sin6_port) != 0) {
			snprintf(portstr, sizeof(portstr), "]:%d", ntohs(sin6->sin6_port));
			strcat(str, portstr);
			return (str);
		}
		return (str + 1);
	}

	default:
		snprintf(str, 128, "sock_ntop: unknown AF_xxx: %d", sa->ss_family);
		return (str);
	}
	return (NULL);
}

int sock_get_port(struct sockaddr_storage *sa)
{
	switch (sa->ss_family) {
	case AF_INET: {
		struct sockaddr_in *sin = (struct sockaddr_in *)sa;
		return ntohs(sin->sin_port);
	}
	case AF_INET6: {
		struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)sa;
		return ntohs(sin6->sin6_port);
	}
	}
	return -1;
}

void *sock_get_addr(struct sockaddr_storage *sa)
{
	switch (sa->ss_family) {
	case AF_INET: {
		struct sockaddr_in *sin = (struct sockaddr_in *)sa;

		return &sin->sin_addr;
	}
	case AF_INET6: {
		struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)sa;

		return &sin6->sin6_addr;
	}
	}
	return NULL;
}

int sock_cpy_addr_port(struct sockaddr_storage *sa1, const struct sockaddr_storage *sa2)
{
	sa1->ss_family = sa2->ss_family;

	switch (sa1->ss_family) {
	case AF_INET:
		(memcpy(&((struct sockaddr_in *)sa1)->sin_addr,
			&((struct sockaddr_in *)sa2)->sin_addr, sizeof(struct in_addr)));
		((struct sockaddr_in *)sa1)->sin_port = ((struct sockaddr_in *)sa2)->sin_port;
		return 0;
	case AF_INET6:
		(memcpy(&((struct sockaddr_in6 *)sa1)->sin6_addr,
			&((struct sockaddr_in6 *)sa2)->sin6_addr, sizeof(struct in6_addr)));

		(((struct sockaddr_in6 *)sa1)->sin6_port = ((struct sockaddr_in6 *)sa2)->sin6_port);
		return 0;
	}
	return (-1);
}

void sock_set_port_cw(struct sockaddr_storage *sa, int port)
{
	switch (sa->ss_family) {
	case AF_INET: {
		struct sockaddr_in *sin = (struct sockaddr_in *)sa;

		sin->sin_port = htons(port);
		return;
	}
	case AF_INET6: {
		struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)sa;

		sin6->sin6_port = htons(port);
		return;
	}
	}

	return;
}

int capwap_init_socket(int port, struct sockaddr_storage *client, int client_len)
{
	struct sockaddr_in listen_addr = {0};
	int sock;
	int yes = 1;
	int err = 0;

	if ((sock = socket(AF_INET, SOCK_DGRAM | SOCK_CLOEXEC, 0)) < 0) {
		CWCritLog("Can't create basic sock(%d)", errno);
		return -errno;
	}

	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
	setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &yes, sizeof(yes));
	listen_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	listen_addr.sin_family = AF_INET;
	listen_addr.sin_port = htons(port);
	if (bind(sock, (struct sockaddr *)&listen_addr, sizeof(listen_addr))) {
		CWCritLog("sock bind error(%d)", errno);
		close(sock);
		return -errno;
	}

	if (client) {
		if ((err = connect(sock, (struct sockaddr *)client, client_len)) < 0) {
			CWCritLog("sock connect error(%d)", errno);
			close(sock);
			return -errno;
		}
	}

	return sock;
}

#ifdef CW_DEBUGGING
static void hexdump(void *buff, int len)
{
	int i;
	unsigned char *data = buff;

	if (!len)
		return;

	for (i = 0; i < len; i++) {
		printf("0x%02hhx ", data[i]);
	}
	printf("\n");
}
#else
static void hexdump(void *buff, int len)
{
	return 0;
}
#endif

int capwap_send_message(int sock, struct iovec *io, size_t iovlen, struct sockaddr_storage *addr, int addr_len)
{
	struct msghdr msg;

	bzero(&msg, sizeof(struct msghdr));
	if (addr) {
		msg.msg_name = addr;
		msg.msg_namelen = addr_len;
	}
	msg.msg_iov = io;
	msg.msg_iovlen = iovlen;

	while (sendmsg(sock, &msg, 0) < 0) {
		if (errno == EINTR)
			continue;
		CWCritLog("Send ctrlmsg fail(%d): %s", errno, strerror(errno));
		return -errno;
	}
	return 0;
}

static int _capwap_send_ctrl_message(int sock, struct cw_ctrlmsg *ctrlmsg, struct sockaddr_storage *addr, int addr_len)
{
	struct iovec io[2];
	int err;

	if (!ctrlmsg)
		return -EINVAL;

	err = cwmsg_ctrlmsg_serialize(ctrlmsg);
	if (err) {
		CWCritLog("control message serialize failed(%d)", err);
		return err;
	}
	io[0].iov_base = ctrlmsg->raw_hdr;
	io[0].iov_len = ctrlmsg->raw_hdr_len;
	hexdump(io[0].iov_base, io[0].iov_len);
	io[1].iov_base = cwmsg_ctrlmsg_get_buffer(ctrlmsg);
	io[1].iov_len = cwmsg_ctrlmsg_get_msg_len(ctrlmsg);
	hexdump(io[1].iov_base, io[1].iov_len > 32 ? 32 : io[1].iov_len);

	return capwap_send_message(sock, io, 2, addr, addr_len);
}

int capwap_send_ctrl_message(int sock, struct cw_ctrlmsg *msg)
{
	return _capwap_send_ctrl_message(sock, msg, NULL, 0);
}

int capwap_send_ctrl_message_unconnected(int sock, struct cw_ctrlmsg *msg, struct sockaddr_storage *addr, int addr_len)
{
	return _capwap_send_ctrl_message(sock, msg, addr, addr_len);
}

int capwap_send_response(int sock, struct cw_ctrlmsg *msg)
{
	int err;

	err = capwap_send_ctrl_message(sock, msg);
	cwmsg_ctrlmsg_destroy(msg);
	return err;
}

ssize_t capwap_recv_message(int sock, void *buff, int len, struct sockaddr *addr, socklen_t *addr_len)
{
	ssize_t recv_len;

	while ((recv_len = recvfrom(sock, buff, len, 0, addr, addr_len)) < 0) {
		if (errno == EINTR)
			continue;
		CWCritLog("Receive ctrlmsg fail(%d): %s", recv_len, strerror(errno));
		return recv_len;
	}
	return recv_len;
}

ssize_t capwap_recv_ctrl_message(int sock, void *buff, int len)
{
	ssize_t recv_len;

	while ((recv_len = recv(sock, buff, len, 0)) < 0) {
		if (errno == EINTR)
			continue;
		CWCritLog("Receive ctrlmsg fail(%d): %s", recv_len, strerror(errno));
		return recv_len;
	}
	return recv_len;
}

int capwap_init_interface_sock(char *path)
{
	struct sockaddr_un addr = {0};
	int sock;

	addr.sun_family = AF_LOCAL;
	strncpy(addr.sun_path, path, sizeof(addr.sun_path) - 1);

	sock = socket(AF_LOCAL, SOCK_DGRAM | SOCK_CLOEXEC, 0);
	if (sock < 0) {
		CWLog("Create %s AF_LOCAL socket failed with %d", path, errno);
		return -errno;
	}

	unlink(path);
	if (bind(sock, (struct sockaddr *)&addr, sizeof(addr))) {
		CWLog("Bind %s AF_LOCAL socket failed with %d", path, errno);
		close(sock);
		return -errno;
	}

	return sock;
}
