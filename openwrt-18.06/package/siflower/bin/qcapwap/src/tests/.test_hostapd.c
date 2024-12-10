#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "wpa_ctrl.h"

#define HOSTAPD_GLOABLE_INTERFACE "/var/run/hostapd/hostapd"
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

struct conf_convert {
	const char *option;
	char *(*to_file)(void *);
	void (*to_conf)(void *, const char *);
};

struct hostapd_config {
	int channel;
	int wpa;
	int logger_syslog;
	int logger_syslog_level;
	int logger_stdout;
	int logger_stdout_level;
	char *ssid;
	char *country_code;
	char *ctrl_interface;
	char *interface;
	char *wpa_passphrase;
	char *extra_data;
};

static char *attr_itoa(void *value)
{
	char *buf = malloc(32);
	if (!buf)
		return NULL;
	memset(buf, 0, 32);
	snprintf(buf, 32, "%d", *(int *)value);

	return buf;
}

static int attr_atoi(const char *value)
{
	if (!value)
		return 0;
	return atoi(value);
}

static char *attr_strdup(const char *value)
{
	if (!value)
		return NULL;
	return strdup(value);
}

#define BUILD_INT_CONVERT(type)                                \
	static char *type##_itoa(void *data)                   \
	{                                                      \
		struct hostapd_config *conf = data;            \
		return attr_itoa(&conf->type);                 \
	}                                                      \
	static void type##_atoi(void *data, const char *value) \
	{                                                      \
		struct hostapd_config *conf = data;            \
		conf->type = attr_atoi(value);                 \
	}

BUILD_INT_CONVERT(channel)
BUILD_INT_CONVERT(wpa)
BUILD_INT_CONVERT(logger_syslog)
BUILD_INT_CONVERT(logger_syslog_level)
BUILD_INT_CONVERT(logger_stdout)
BUILD_INT_CONVERT(logger_stdout_level)

#define BUILD_STRING_CONVERT(type)                                \
	static char *type##_to_file(void *data)                   \
	{                                                         \
		struct hostapd_config *conf = data;               \
		return attr_strdup(conf->type);                   \
	}                                                         \
	static void type##_to_conf(void *data, const char *value) \
	{                                                         \
		struct hostapd_config *conf = data;               \
		conf->type = attr_strdup(value);                  \
	}
BUILD_STRING_CONVERT(ssid)
BUILD_STRING_CONVERT(country_code)
BUILD_STRING_CONVERT(ctrl_interface)
BUILD_STRING_CONVERT(interface)
BUILD_STRING_CONVERT(wpa_passphrase)

#define INT_CONVERT(type)                                                        \
	{                                                                        \
		.option = #type, .to_file = type##_itoa, .to_conf = type##_atoi, \
	}
#define STRING_CONVERT(type)                                                           \
	{                                                                              \
		.option = #type, .to_file = type##_to_file, .to_conf = type##_to_conf, \
	}

static const struct conf_convert config_convert[] = {
    INT_CONVERT(channel),
    INT_CONVERT(wpa),
    INT_CONVERT(logger_syslog),
    INT_CONVERT(logger_syslog_level),
    INT_CONVERT(logger_stdout),
    INT_CONVERT(logger_stdout_level),
    STRING_CONVERT(ssid),
    STRING_CONVERT(country_code),
    STRING_CONVERT(ctrl_interface),
    STRING_CONVERT(interface),
    STRING_CONVERT(wpa_passphrase),
};

void hostapd_config_free(struct hostapd_config *conf)
{
	if (!conf)
		return;

	free(conf);
}

static struct conf_convert *find_convert(const char *option)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(config_convert); i++) {
		if (!strcmp(option, config_convert[i].option))
			return &config_convert[i];
	}
	return NULL;
}

static int hostapd_config_fill(struct hostapd_config *conf, char *buf, char *pos, int line)
{
	struct conf_convert *conv;

	conv = find_convert(buf);
	if (!conv)
		return 1;
	conv->to_conf(conf, pos);

	return 0;
}

int hostapd_config_save(struct hostapd_config *conf, const char *fname)
{
	FILE *f;
	int fd;
	char *s;
	char tmp_file[32];
	int err = 0;
	int i;

	strcpy(tmp_file, "/hostapd.conf.XXXXXX");
	fd = mkstemp(tmp_file);
	f = fdopen(fd, "w");
	if (!f) {
		printf("Could not open config file '%s' for write: %s", tmp_file, strerror(errno));
		close(fd);
		return -errno;
	}

	fprintf(f, "%s", conf->extra_data);
	for (i = 0; i < ARRAY_SIZE(config_convert); i++) {
		s = config_convert[i].to_file(conf);
		err = fprintf(f, "%s=%s\n", config_convert[i].option, s);
		free(s);
		if (err < 0) {
			close(fd);
			printf("write config failed: %s", strerror(errno));
			return -errno;
		}
	}

	fclose(f);
	if (rename(tmp_file, fname)) {
		printf("rename failed: %s", strerror(errno));
		return -errno;
	}
	return 0;
}

struct hostapd_config *hostapd_config_read(const char *fname)
{
	struct hostapd_config *conf;
	FILE *f;
	char buf[512], *pos;
	int line = 0;
	int errors = 0;
	size_t i = 0;
	size_t extra_data_len = 4 * 1024;

	conf = malloc(sizeof(*conf));
	if (!conf)
		return NULL;
	conf->extra_data = malloc(extra_data_len);
	if (!conf->extra_data) {
		free(conf);
		return NULL;
	}

	f = fopen(fname, "r");
	if (f == NULL) {
		printf("Could not open configuration file '%s' for reading.", fname);
		return conf;
	}

	while (fgets(buf, sizeof(buf), f)) {
		line++;

		if (buf[0] == '#')
			continue;
		pos = buf;
		while (*pos != '\0') {
			if (*pos == '\n') {
				*pos = '\0';
				break;
			}
			pos++;
		}
		if (buf[0] == '\0')
			continue;

		pos = strchr(buf, '=');
		if (pos == NULL) {
			printf("Line %d: invalid line '%s'", line, buf);
			errors++;
			continue;
		}
		*pos = '\0';
		pos++;

		if (hostapd_config_fill(conf, buf, pos, line)) {
			*(pos - 1) = '=';
			strcat(pos, "\n");
			if (i + strlen(buf) < extra_data_len) {
				memcpy(conf->extra_data + i, buf, strlen(buf) + 1);
				i += strlen(buf);
			} else {
				printf("No extra space for config file");
			}
		}
	}

	fclose(f);

	if (errors) {
		printf("%d errors found in configuration file '%s'", errors, fname);
		hostapd_config_free(conf);
		conf = NULL;
	}

	return conf;
}

int main()
{
	struct wpa_ctrl *ctrl;
	char send[512] = {0};
	char recv[512] = {0};
	size_t recv_len;
	struct stat s = {0};
	pid_t pid;
	struct hostapd_config *conf;

	// pid = fork();
	// if (pid < 0) {
	// 	printf("start hostapd failed: %s", strerror(errno));
	// 	return -1;
	// } else if (pid == 0) { // Child
	// 	if (execlp("hostapd", "hostapd", "-g", HOSTAPD_GLOABLE_INTERFACE, NULL) < 0)
	// 		printf("Start hostapd fail in child process");
	// 	exit(127);
	// }

	// while ((stat(HOSTAPD_GLOABLE_INTERFACE, &s) < 0) ||
	//        !S_ISSOCK(s.st_mode))
	// 	;

	// conf = hostapd_config_read("/hostapd-phy0.conf");
	// ctrl = wpa_ctrl_open(HOSTAPD_GLOABLE_INTERFACE);
	// if (!ctrl) {
	// 	printf("Can't open hostapd interface");
	// 	return -EINVAL;
	// }

	// perror("open");
	// strcpy(send, "ADD bss_config=phy0:/hostapd-phy0.conf");
	// recv_len = 512;
	// wpa_ctrl_request(ctrl, send, 50, recv, &recv_len, NULL);
	// perror("request");
	// printf("%s", recv);
	// wpa_ctrl_close(ctrl);
	// printf("=============\n");
	// sleep(20);
	// printf("modify\n");
	// conf->ssid = "abcdefghijklmn";
	// hostapd_config_save(conf, "hostapd-phy0.conf");
	ctrl = wpa_ctrl_open("/tmp/hostapdd/wlan0");
	perror("open");
	strcpy(send, "UPDATE ");
	recv_len = 512;
	wpa_ctrl_request(ctrl, send, 50, recv, &recv_len, NULL);
	perror("request");
	printf("%s\n", recv);
	wpa_ctrl_close(ctrl);
	return 0;
}

int main2()
{
	struct hostapd_config *conf;
	conf = hostapd_config_read("/hostapd-phy0.conf");
	conf->ssid = "fuck";
	hostapd_config_save(conf, "/hostapd-ppp.conf");
}
