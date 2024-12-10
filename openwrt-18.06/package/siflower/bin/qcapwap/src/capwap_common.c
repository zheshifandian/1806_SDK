#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <errno.h>

#include "common.h"
#include "CWLog.h"

static int hex2num(char c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	return -1;
}


int hex2byte(const char *hex)
{
	int a, b;
	a = hex2num(*hex++);
	if (a < 0)
		return -1;
	b = hex2num(*hex++);
	if (b < 0)
		return -1;
	return (a << 4) | b;
}


static const char * hwaddr_parse(const char *txt, uint8_t *addr)
{
	size_t i;

	for (i = 0; i < ETH_ALEN; i++) {
		int a;

		a = hex2byte(txt);
		if (a < 0)
			return NULL;
		txt += 2;
		addr[i] = a;
		if (i < ETH_ALEN - 1 && *txt++ != ':')
			return NULL;
	}
	return txt;
}


/**
 * hwaddr_aton - Convert ASCII string to MAC address (colon-delimited format)
 * @txt: MAC address as a string (e.g., "00:11:22:33:44:55")
 * @addr: Buffer for the MAC address (ETH_ALEN = 6 bytes)
 * Returns: 0 on success, -1 on failure (e.g., string not a MAC address)
 */
int hwaddr_aton(const char *txt, uint8_t *addr)
{
	return hwaddr_parse(txt, addr) ? 0 : -1;
}

static void strip_ret(char *s)
{
	while (*s != '\0') {
		if (*s == '\n') {
			*s = '\0';
			return;
		}
		s++;
	}
}

int cat_simple_file(const char *fname, char *buff, int len)
{
	FILE *f = NULL;
	int err = 0;

	if (!fname)
		return -EINVAL;
	f = fopen(fname, "r");
	if (!f)
		return -errno;
	if (fgets(buff, len, f) == NULL)
		err = -EINVAL;
	fclose(f);
	strip_ret(buff);
	return err;
}

char *execute_command(const char *fmt, ...)
{
	FILE *fstream = NULL;
	char *command = NULL;
	char *buff;
	int size = 0;
	va_list ap;

	if (!fmt)
		return NULL;
	/* Determine required size */
	va_start(ap, fmt);
	size = vsnprintf(command, size, fmt, ap);
	va_end(ap);

	if (size < 0)
		return NULL;

	size++; /* For '\0' */
	command = malloc(size);
	if (command == NULL)
		return NULL;

	va_start(ap, fmt);
	size = vsnprintf(command, size, fmt, ap);
	if (size < 0) {
		free(command);
		return NULL;
	}
	va_end(ap);

	buff = calloc(1, 128);
	if (!buff) {
		free(command);
		return NULL;
	}
	fstream = popen(command, "r");
	if (!fstream) {
		free(command);
		free(buff);
		return NULL;
	}

	fgets(buff, sizeof(buff), fstream);
	strip_ret(buff);
	pclose(fstream);
	free(command);

	return buff;
}

static int get_file_string(const char *file_name, const char *fallback_command, char *buff, int len)
{
	char *s;
	int i, j;
	int err;

	if ((!file_name) || (!buff) || (len == 0))
		return -EINVAL;

	err = cat_simple_file(file_name, buff, len);
	if (err == 0 || !fallback_command)
		return err;

	s = execute_command(fallback_command);
	if (!s)
		return err;
	for (i = 0, j = 0; i < strlen(s); i++) {
		if (s[i] >= '!' && s[i] <= 'z' && j < len)
			buff[j++] = s[i];
	}
	buff[j] = '\0';
	free(s);

	return 0;
}

int get_hardware(char *buff, int len)
{
	int err;

	if (!buff)
		return -EINVAL;
	// 1.check hw version falg
	err = get_file_string("/sys/devices/factory-read/hw_ver_flag",
			    "hexdump -c -n2 -s 27 /dev/mtdblock2", buff, len);
	if (err)
		return err;

	if (strncmp(buff, "hv", 2))
		return -ENODATA;

	// 2.get the version
	return get_file_string("/sys/devices/factory-read/hw_ver",
			    "hexdump -c -n32 -s 29 /dev/mtdblock2", buff, len);
}

int get_version(char *buff, int len)
{
	if (!buff)
		return -EINVAL;

	return cat_simple_file("/etc/openwrt_version", buff, len);
}

int get_model(char *buff, int len)
{
	int err = 0;

	if (!buff)
		return -EINVAL;
	err = get_file_string("/sys/devices/factory-read/model_ver_flag",
			    "hexdump -c -n2 -s 63 /dev/mtdblock2", buff, len);
	if (err)
		return err;

	// model_ver_flag != "mv" means there is no model_ver in the flash.
	if (strncmp(buff, "mv", 2))
		return -ENODATA;

	return get_file_string("/sys/devices/factory-read/model_ver",
			    "hexdump -c -n32 -s 65 /dev/mtdblock2", buff, len);
}

in_addr_t get_ipv4_addr(const char *if_name)
{
	struct ifreq ifr = {0};
	int sock;

	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0)
		return 0;

	strcpy(ifr.ifr_name, if_name);
	if (ioctl(sock, SIOCGIFADDR, &ifr) < 0) {
		close(sock);
		return -errno;
	}

	close(sock);
	return ((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr.s_addr;
}

int get_boardcast_addr(const char *if_name, struct sockaddr *addr)
{
	struct ifreq ifr = {0};
	int sock;

	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0)
		return -errno;
	strcpy(ifr.ifr_name, if_name);
	if (ioctl(sock, SIOCGIFBRDADDR, &ifr) < 0) {
		close(sock);
		return -errno;
	}
	memcpy(addr, &ifr.ifr_broadaddr, sizeof(struct sockaddr_in));
	close(sock);
	return 0;
}

int get_mac_addr(const char* interface, uint8_t *macAddr)
{
	struct ifreq ifr;
	int sock;

	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0) {
		CWLog("Error Creating Socket for ioctl");
		return -1;
	}

	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, interface, IFNAMSIZ);

	if (ioctl(sock, SIOCGIFHWADDR, &ifr) < 0) {
		close(sock);
		return -errno;
	}

	memcpy(macAddr, ifr.ifr_hwaddr.sa_data, ETH_ALEN);
	close(sock);

	return 0;
}

void save_status(const char *file_name, const char *status)
{
	FILE *file = fopen(file_name, "w");
	fprintf(file, "%s\n", status);
	fclose(file);
}
