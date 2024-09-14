/* The GPL applies to this program.
  In addition, as a special exception, the copyright holders give
  permission to link the code of portions of this program with the
  OpenSSL library under certain conditions as described in each
  individual source file, and distribute linked combinations
  including the two.
  You must obey the GNU General Public License in all respects
  for all of the code used other than OpenSSL.  If you modify
  file(s) with this exception, you may extend this exception to your
  version of the file(s), but you are not obligated to do so.  If you
  do not wish to do so, delete this exception statement from your
  version.  If you delete this exception statement from all source
  files in the program, then also delete it here.
*/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#ifndef NO_SSL
#include <openssl/ssl.h>
#include "mssl.h"
#endif
#include <arpa/inet.h>

#include "gen.h"
#include "http.h"
#include "io.h"
#include "str.h"
#include "mem.h"
#include "tcp.h"
#include "res.h"
#include "utils.h"
#include "error.h"

static volatile int stop = 0;

int quiet = 0;
char machine_readable = 0;

char nagios_mode = 0;

char last_error[ERROR_BUFFER_SIZE];

void version(void)
{
	fprintf(stderr, "HTTPing v" VERSION ", (C) 2003-2010 folkert@vanheusden.com\n");
#ifndef NO_SSL
	fprintf(stderr, "SSL support included\n");
#endif
}

void usage(void)
{
	fprintf(stderr, "\n-g url         url (e.g. -g http://localhost/)\n");
	fprintf(stderr, "-h hostname    hostname (e.g. localhost)\n");
	fprintf(stderr, "-p portnr      portnumber (e.g. 80)\n");
	fprintf(stderr, "-x host:port   hostname+portnumber of proxyserver\n");
	fprintf(stderr, "-c count       how many times to connect\n");
	fprintf(stderr, "-i interval    delay between each connect, can be only smaller than 1 if user is root\n");
	fprintf(stderr, "-t timeout     timeout (default: 30s)\n");
	fprintf(stderr, "-Z             ask any proxies on the way not to cache the requests\n");
	fprintf(stderr, "-Q             use a persistent connection. adds a 'C' to the output if httping had to reconnect\n");
	fprintf(stderr, "-6             use IPv6\n");
	fprintf(stderr, "-s             show statuscodes\n");
	fprintf(stderr, "-S             split time in connect-time and processing time\n");
	fprintf(stderr, "-G             do a GET request instead of HEAD (read the\n");
	fprintf(stderr, "               contents of the page as well)\n");
	fprintf(stderr, "-b             show transfer speed in KB/s (use with -G)\n");
	fprintf(stderr, "-B             like -b but use compression if available\n");
	fprintf(stderr, "-L x           limit the amount of data transferred (for -b)\n");
	fprintf(stderr, "               to 'x' (in bytes)\n");
	fprintf(stderr, "-X             show the number of KB transferred (for -b)\n");
#ifndef NO_SSL
	fprintf(stderr, "-l             connect using SSL\n");
	fprintf(stderr, "-z             show fingerprint (SSL)\n");
#endif
	fprintf(stderr, "-f             flood connect (no delays)\n");
	fprintf(stderr, "-a             audible ping\n");
	fprintf(stderr, "-m             give machine parseable output (see\n");
	fprintf(stderr, "               also -o and -e)\n");
	fprintf(stderr, "-o rc,rc,...   what http results codes indicate 'ok'\n");
	fprintf(stderr, "               coma seperated WITHOUT spaces inbetween\n");
	fprintf(stderr, "               default is 200, use with -e\n");
	fprintf(stderr, "-e str         string to display when http result code\n");
	fprintf(stderr, "               doesn't match\n");
	fprintf(stderr, "-I str         use 'str' for the UserAgent header\n");
	fprintf(stderr, "-R str         use 'str' for the Referer header\n");
	fprintf(stderr, "-r             resolve hostname only once (usefull when\n");
	fprintf(stderr, "               pinging roundrobin DNS: also takes the first\n");
	fprintf(stderr, "               DNS lookup out of the loop so that the first\n");
	fprintf(stderr, "               measurement is also correct)\n");
	fprintf(stderr, "-n warn,crit   Nagios-mode: return 1 when avg. response time\n");
	fprintf(stderr, "               >= warn, 2 if >= crit, otherwhise return 0\n");
	fprintf(stderr, "-N x           Nagios mode 2: return 0 when all fine, 'x'\n");
	fprintf(stderr, "               when anything failes\n");
	fprintf(stderr, "-y ip[:port]   bind to ip-address (and thus interface) [/port]\n");
	fprintf(stderr, "-q             quiet, only returncode\n");
	fprintf(stderr, "-A             Activate Basic authentication\n");
	fprintf(stderr, "-U Username    needed for authentication\n");
	fprintf(stderr, "-P Password    needed for authentication\n");
	fprintf(stderr, "-C cookie=value Add a cookie to the request\n");
	fprintf(stderr, "-V             show the version\n\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "--url			-g\n");
	fprintf(stderr, "--hostname		-h\n");
	fprintf(stderr, "--port			-p\n");
	fprintf(stderr, "--host-port		-x\n");
	fprintf(stderr, "--count		-c\n");
	fprintf(stderr, "--interval		-i\n");
	fprintf(stderr, "--timeout		-t\n");
	fprintf(stderr, "--ipv6		-	6\n");
	fprintf(stderr, "--show-statusodes	-s\n");
	fprintf(stderr, "--split-time		-S\n");
	fprintf(stderr, "--get-request		-G\n");
	fprintf(stderr, "--show-transfer-speed	-b\n");
	fprintf(stderr, "--show-xfer-speed-compressed		-B\n");
	fprintf(stderr, "--data-limit		-L\n");
	fprintf(stderr, "--show-kb		-X\n");
#ifndef NO_SSL
	fprintf(stderr, "--use-ssl		-l\n");
	fprintf(stderr, "--show-fingerprint	-z\n");
#endif
	fprintf(stderr, "--flood		-f\n");
	fprintf(stderr, "--audible-ping		-a\n");
	fprintf(stderr, "--parseable-output	-m\n");
	fprintf(stderr, "--ok-result-codes	-o\n");
	fprintf(stderr, "--result-string	-e\n");
	fprintf(stderr, "--user-agent		-I\n");
	fprintf(stderr, "--referer		-S\n");
	fprintf(stderr, "--resolve-once		-r\n");
	fprintf(stderr, "--nagios-mode-1	-n\n");
	fprintf(stderr, "--nagios-mode-2	-n\n");
	fprintf(stderr, "--bind-to		-y\n");
	fprintf(stderr, "--quiet		-q\n");
	fprintf(stderr, "--basic-auth		-A\n");
	fprintf(stderr, "--username		-U\n");
	fprintf(stderr, "--password		-P\n");
	fprintf(stderr, "--cookie		-C\n");
	fprintf(stderr, "--persistent-connections	-Q\n");
	fprintf(stderr, "--no-cache		-Z\n");
	fprintf(stderr, "--version		-V\n");
	fprintf(stderr, "--help			-h\n");
}

void emit_error()
{
	if (!quiet && !machine_readable && !nagios_mode)
	{
		printf("%s", last_error);
	}

	if (!nagios_mode)
		last_error[0] = 0x00;

	fflush(NULL);
}

void handler(int sig)
{
	fprintf(stderr, "Got signal %d\n", sig);

	stop = 1;
}

/* Base64 encoding start */  
const char *alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

void encode_tryptique(char source[3], char result[4])
/* Encode 3 char in B64, result give 4 Char */
 {
    int tryptique, i;
    tryptique = source[0];
    tryptique *= 256;
    tryptique += source[1];
    tryptique *= 256;
    tryptique += source[2];
    for (i=0; i<4; i++)
    {
 	result[3-i] = alphabet[tryptique%64];
	tryptique /= 64;
    }
} 


int enc_b64(char *source, size_t source_lenght, char *target)
{
	/* Divide string /3 and encode trio */
	while (source_lenght >= 3) {
		encode_tryptique(source, target);
		source_lenght -= 3;
		source += 3;
		target += 4;
	}
	/* Add padding to the rest */
	if (source_lenght > 0) {
		char pad[3];
	 	memset(pad, 0, sizeof(pad));
		memcpy(pad, source, source_lenght);
		encode_tryptique(pad, target);
		target[3] = '=';
		if (source_lenght == 1) target[2] = '=';
		target += 4;
	}
	target[0] = 0;
	return 1;
} 
/* Base64 encoding END */  

int main(int argc, char *argv[])
{
	char *hostname = NULL;
	char *proxy = NULL, *proxyhost = NULL;
	int proxyport = 8080;
	int portnr = 80;
	char *get = NULL, *request;
	int req_len;
	int c;
	int count = -1, curncount = 0;
	double wait = 1.0;
	int audible = 0;
	int ok = 0, err = 0;
	double min = 999999999999999.0, avg = 0.0, max = 0.0;
	int timeout=30;
	char show_statuscodes = 0;
	char use_ssl = 0;
	SSL_CTX *client_ctx = NULL;
	char *ok_str = "200";
	char *err_str = "-1";
	char *useragent = NULL;
	char *referer = NULL;
	char *host;
	char *pwd = NULL;
	char *usr = NULL;
	char *cookie = NULL;
	int port;
	struct sockaddr_in6 addr;
	struct addrinfo ai;
	char resolve_once = 0;
	char auth_mode = 0;
	char have_resolved = 0;
	double nagios_warn=0.0, nagios_crit=0.0;
	int nagios_exit_code = 2;
	double avg_httping_time = -1.0;
	int get_instead_of_head = 0;
	char *buffer;
	int page_size = sysconf(_SC_PAGESIZE);
	char show_Bps = 0, ask_compression = 0;
	int Bps_min = 1 << 30, Bps_max = 0;
	long long int Bps_avg = 0;
	int Bps_limit = -1;
	char show_bytes_xfer = 0, show_fp = 0;
	int fd = -1;
	SSL *ssl_h = NULL;
	BIO *s_bio;
	struct sockaddr_in *bind_to = NULL;
	struct sockaddr_in bind_to_4;
	struct sockaddr_in6 bind_to_6;
	char bind_to_valid = 0;
	char split = 0, use_ipv6 = 0;
	char persistent_connections = 0, persistent_did_reconnect = 0;
	char no_cache = 0;

	static struct option long_options[] =
	{
		{"url",		1, NULL, 'g' },
		{"hostname",	1, NULL, 'h' },
		{"port",	1, NULL, 'p' },
		{"host-port",	1, NULL, 'x' },
		{"count",	1, NULL, 'c' },
		{"persistent-connections",	0, NULL, 'Q' },
		{"interval",	1, NULL, 'i' },
		{"timeout",	1, NULL, 't' },
		{"ipv6",	0, NULL, '6' },
		{"show-statusodes",	0, NULL, 's' },
		{"split-time",	0, NULL, 'S' },
		{"get-request",	0, NULL, 'G' },
		{"show-transfer-speed",	0, NULL, 'b' },
		{"show-xfer-speed-compressed",	0, NULL, 'B' },
		{"data-limit",	1, NULL, 'L' },
		{"show-kb",	0, NULL, 'X' },
		{"no-cache",	0, NULL, 'Z' },
#ifndef NO_SSL
		{"use-ssl",	0, NULL, 'l' },
		{"show-fingerprint",	0, NULL, 'z' },
#endif
		{"flood",	0, NULL, 'f' },
		{"audible-ping",	0, NULL, 'a' },
		{"parseable-output",	0, NULL, 'm' },
		{"ok-result-codes",	1, NULL, 'o' },
		{"result-string",	1, NULL, 'e' },
		{"user-agent",	1, NULL, 'I' },
		{"referer",	1, NULL, 'S' },
		{"resolve-once",0, NULL, 'r' },
		{"nagios-mode-1",	1, NULL, 'n' },
		{"nagios-mode-2",	1, NULL, 'n' },
		{"bind-to",	1, NULL, 'y' },
		{"quiet",	0, NULL, 'q' },
		{"basic-auth",	0, NULL, 'A' },
		{"username",	1, NULL, 'U' },
		{"password",	1, NULL, 'P' },
		{"cookie",	1, NULL, 'C' },
		{"version",	0, NULL, 'V' },
		{"help",	0, NULL, 'h' },
		{NULL,		0, NULL, 0   }
	};

	signal(SIGPIPE, SIG_IGN);

	if (page_size == -1)
		page_size = 4096;

	buffer = (char *)mymalloc(page_size, "receive buffer");

	while((c = getopt_long(argc, argv, "ZQ6Sy:XL:bBg:h:p:c:i:Gx:t:o:e:falqsmV?I:R:rn:N:z:AP:U:C:", long_options, NULL)) != -1)
	{
		switch(c)
		{
			case 'Z':
				no_cache = 1;
				break;

			case '6':
				use_ipv6 = 1;
				break;

			case 'S':
				split = 1;
				break;

			case 'Q':
				persistent_connections = 1;
				break;

			case 'y':
				{
					char *dummy = strchr(optarg, ':');

					bind_to_valid = 1;

					if (dummy)
					{
						bind_to = (struct sockaddr_in *)&bind_to_6;
						memset(&bind_to_6, 0x00, sizeof(bind_to_6));

						if (inet_pton(AF_INET6, optarg, &(bind_to_6.sin6_addr)) != 1)
						{
							error_exit("cannot convert ip address '%s' (for -y)\n", optarg);
						}
					}
					else
					{
						bind_to = (struct sockaddr_in *)&bind_to_4;
						memset(&bind_to_4, 0x00, sizeof(bind_to_4));

						if (inet_pton(AF_INET, optarg, &(bind_to_4.sin_addr)) != 1)
						{
							error_exit("cannot convert ip address '%s' (for -y)\n", optarg);
						}
					}
				}
				break;

			case 'z':
				show_fp = 1;
				break;

			case 'X':
				show_bytes_xfer = 1;
				break;

			case 'L':
				Bps_limit = atoi(optarg);
				break;

			case 'B':
				show_Bps = 1;
				ask_compression = 1;
				break;

			case 'b':
				show_Bps = 1;
				break;

			case 'e':
				err_str = optarg;
				break;

			case 'o':
				ok_str = optarg;
				break;

			case 'x':
				proxy = optarg;
				break;

			case 'g':
				get = optarg;
				break;

			case 'r':
				resolve_once = 1;
				break;

			case 'h':
				hostname = optarg;
				break;

			case 'p':
				portnr = atoi(optarg);
				break;

			case 'c':
				count = atoi(optarg);
				break;

			case 'i':
				wait = atof(optarg);
				if (wait < 1.0 && getuid() != 0)
				{
					fprintf(stderr, "Only root can use intervals smaller than 1\n");
					wait = 1.0;
				}
				break;

			case 't':
				timeout = atoi(optarg);
				break;

			case 'I':
				useragent = optarg;
				break;

			case 'R':
				referer = optarg;
				break;

			case 'a':
				audible = 1;
				break;

			case 'f':
				wait = 0;
				break;

			case 'G':
				get_instead_of_head = 1;
				break;

#ifndef NO_SSL
			case 'l':
				use_ssl = 1;
				break;
#endif

			case 'm':
				machine_readable = 1;
				break;

			case 'q':
				quiet = 1;
				break;

			case 's':
				show_statuscodes = 1;
				break;

			case 'V':
				version();
				return 0;

			case 'n':
				{
					char *dummy = strchr(optarg, ',');
					if (nagios_mode) error_exit("-n and -N are mutual exclusive\n");
					nagios_mode = 1;
					if (!dummy)
						error_exit("-n: missing parameter\n");
					nagios_warn = atof(optarg);
					nagios_crit = atof(dummy + 1);
				} break;
			case 'N':
				if (nagios_mode) error_exit("-n and -N are mutual exclusive\n");
				nagios_mode = 2;
				nagios_exit_code = atoi(optarg);
				break;
			case 'A': 
				auth_mode = 1; 
				break;
			case 'P':
				pwd = optarg;
				break;
			case 'U':
				usr = optarg;
				break;
			case 'C':
				cookie = optarg;
				break;
			case '?':
			default:
				version();
				usage();
				return 1;
		}
	}
	if (optind < argc)
		get = argv[optind];

	last_error[0] = 0x00;

	if (!get_instead_of_head && show_Bps)
		error_exit("-b/-B can only be used when also using -G\n");

	if (get != NULL && hostname == NULL)
	{
		char *slash, *colon;
		char *getcopy = mystrdup(get, "get request");

		if (strncasecmp(getcopy, "http://", 7) == 0)
		{
			getcopy += 7;
		}
		else if (strncasecmp(getcopy, "https://", 8) == 0)
		{
			getcopy += 8;
			use_ssl = 1;
		}

		/*
		   if (strncasecmp(getcopy, http_string, http_string_len) != 0)
		   {
		   fprintf(stderr, "'%s' is a strange URL\n", getcopy);
		   fprintf(stderr, "Expected: %s...\n", http_string);
		   if (strncasecmp(getcopy, "https://", 8) == 0)
		   fprintf(stderr, "Did you forget to add the '-l' switch to the httping commandline?\n");
		   return 2;
		   }
		 */

		slash = strchr(getcopy, '/');
		if (slash)
			*slash = 0x00;

		if (!use_ipv6)
		{
			colon = strchr(getcopy, ':');
			if (colon)
			{
				*colon = 0x00;
				portnr = atoi(colon + 1);
			}
		}

		hostname = getcopy;
	}

	if (hostname == NULL)
	{
		usage();
		error_exit("No hostname/getrequest given\n");
	}

#ifndef NO_SSL
	if (use_ssl && portnr == 80)
		portnr = 443;
#endif

	if (get == NULL)
	{
#ifndef NO_SSL
		if (use_ssl)
		{
			get = mymalloc(8 /* http:// */ + strlen(hostname) + 1 /* colon */ + 5 /* portnr */ + 1 /* / */ + 1 /* 0x00 */, "get");
			sprintf(get, "https://%s:%d/", hostname, portnr);
		}
		else
		{
#endif
			get = mymalloc(7 /* http:// */ + strlen(hostname) + 1 /* colon */ + 5 /* portnr */ + 1 /* / */ + 1 /* 0x00 */, "get");
			sprintf(get, "http://%s:%d/", hostname, portnr);
#ifndef NO_SSL
		}
#endif
	}

	if (proxy)
	{
		char *dummy = strchr(proxy, ':');
		proxyhost = proxy;
		if (dummy)
		{
			*dummy=0x00;
			proxyport = atoi(dummy + 1);
		}

		if (!quiet && !nagios_mode)
			fprintf(stderr, "Using proxyserver: %s:%d\n", proxyhost, proxyport);
	}

#ifndef NO_SSL
	if (use_ssl)
	{
		client_ctx = initialize_ctx();
		if (!client_ctx)
		{
			snprintf(last_error, ERROR_BUFFER_SIZE, "problem creating SSL context\n");
			goto error_exit;
		}
	}
#endif

	request = mymalloc(strlen(get) + 8192, "request");
	if (proxyhost)
		sprintf(request, "%s %s HTTP/1.%c\r\n", get_instead_of_head?"GET":"HEAD", get, persistent_connections?'1':'0');
	else
	{
		char *dummy = get, *slash;
		if (strncasecmp(dummy, "http://", 7) == 0)
			dummy += 7;
		else if (strncasecmp(dummy, "https://", 7) == 0)
			dummy += 8;

		slash = strchr(dummy, '/');
		if (slash)
			sprintf(request, "%s %s HTTP/1.%c\r\n", get_instead_of_head?"GET":"HEAD", slash, persistent_connections?'1':'0');
		else
			sprintf(request, "%s / HTTP/1.%c\r\n", get_instead_of_head?"GET":"HEAD", persistent_connections?'1':'0');
	}
	if (useragent)
		sprintf(&request[strlen(request)], "User-Agent: %s\r\n", useragent);
	else
		sprintf(&request[strlen(request)], "User-Agent: HTTPing v" VERSION "\r\n");
	sprintf(&request[strlen(request)], "Host: %s\r\n", hostname);
	if (referer)
		sprintf(&request[strlen(request)], "Referer: %s\r\n", referer);
	if (ask_compression)
		sprintf(&request[strlen(request)], "Accept-Encoding: gzip,deflate\r\n");

	if (no_cache)
	{
		sprintf(&request[strlen(request)], "Pragma: no-cache\r\n");
		sprintf(&request[strlen(request)], "Cache-Control: no-cache\r\n");
	}

	/* Basic Authentification */
	if (auth_mode) { 
		char auth_string[255];
		char b64_auth_string[255];
		if ( usr == NULL )
			error_exit("Basic Authnetication (-A) can only be used with a username and/or password (-U -P) ");
		sprintf(auth_string,"%s:%s",usr,pwd); 
		enc_b64(auth_string, strlen(auth_string), b64_auth_string);
		sprintf(&request[strlen(request)], "Authorization: Basic %s\r\n", b64_auth_string);
	}

	/* Cookie Insertion */
	if (cookie) { 
		sprintf(&request[strlen(request)], "Cookie: %s;\r\n", cookie);
	}

	if (persistent_connections)
		sprintf(&request[strlen(request)], "Connection: keep-alive\r\n");

	strcat(request, "\r\n");
	req_len = strlen(request);

	if (!quiet && !machine_readable && !nagios_mode)
		printf("PING %s:%d (%s):\n", hostname, portnr, get);

	signal(SIGINT, handler);
	signal(SIGTERM, handler);

	timeout *= 1000;	/* change to ms */

	host = proxyhost?proxyhost:hostname;
	port = proxyhost?proxyport:portnr;

	if (resolve_once)
	{
		memset(&addr, 0x00, sizeof(addr));

		if (resolve_host(host, &addr, &ai, use_ipv6, port) == -1)
		{
			err++;
			emit_error();
			have_resolved = 1;
		}
	}

	if (persistent_connections)
		fd = -1;

	while((curncount < count || count == -1) && stop == 0)
	{
		double ms;
		double dstart, dend, dafter_connect = 0.0;
		char *reply;
		int Bps = 0;
		char is_compressed = 0;
		long long int bytes_transferred = 0;

		dstart = get_ts();

		for(;;)
		{
			char *fp = NULL;
			int rc;
			char *sc = NULL, *scdummy = NULL;
			int persistent_tries = 0;
			int len = 0, overflow = 0, headers_len;

			curncount++;

persistent_loop:
			if ((!resolve_once || (resolve_once == 1 && have_resolved == 0)) && fd == -1)
			{
				memset(&addr, 0x00, sizeof(addr));

				if (resolve_host(host, &addr, &ai, use_ipv6, port) == -1)
				{
					err++;
					emit_error();
					break;
				}

				have_resolved = 1;
			}

			if ((persistent_connections && fd < 0) || (!persistent_connections))
				fd = connect_to(bind_to_valid?bind_to:NULL, &ai, timeout);

			if (fd == -3)	/* ^C pressed */
				break;

			if (fd < 0)
			{
				emit_error();
				fd = -1;
			}

			if (fd >= 0)
			{
				/* set socket to low latency */
				if (set_tcp_low_latency(fd) == -1)
				{
					close(fd);
					fd = -1;
					break;
				}

				/* set fd blocking */
				if (set_fd_blocking(fd) == -1)
				{
					close(fd);
					fd = -1;
					break;
				}

#ifndef NO_SSL
				if (use_ssl && ssl_h == NULL)
				{
					int rc;

					rc = connect_ssl(fd, client_ctx, &ssl_h, &s_bio, timeout);
					if (rc != 0)
					{
						close(fd);
						fd = rc;

						if (persistent_connections)
						{
							if (++persistent_tries < 2)
							{
								close(fd);
								fd = -1;
								persistent_did_reconnect = 1;
								goto persistent_loop;
							}
						}
					}
				}
#endif
			}
			if (split)
				dafter_connect = get_ts();

			if (fd < 0)
			{
				if (fd == -2)
					snprintf(last_error, ERROR_BUFFER_SIZE, "timeout connecting to host\n");

				emit_error();
				err++;

				fd = -1;

				break;
			}

#ifndef NO_SSL
			if (use_ssl)
				rc = WRITE_SSL(ssl_h, request, req_len);
			else
#endif
				rc = mywrite(fd, request, req_len, timeout);
			if (rc != req_len)
			{
				if (persistent_connections)
				{
					if (++persistent_tries < 2)
					{
						close(fd);
						fd = -1;
						persistent_did_reconnect = 1;
						goto persistent_loop;
					}
				}

				if (rc == -1)
					snprintf(last_error, ERROR_BUFFER_SIZE, "error sending request to host\n");
				else if (rc == -2)
					snprintf(last_error, ERROR_BUFFER_SIZE, "timeout sending to host\n");
				else if (rc == -3)
				{/* ^C */}
				else if (rc == 0)
					snprintf(last_error, ERROR_BUFFER_SIZE, "connection prematurely closed by peer\n");

				emit_error();

				close(fd);
				fd = -1;
				err++;

				break;
			}

			rc = get_HTTP_headers(fd, ssl_h, &reply, &overflow, timeout);

			if ((show_statuscodes || machine_readable) && reply != NULL)
			{
				/* statuscode is in first line behind
				 * 'HTTP/1.x'
				 */
				char *dummy = strchr(reply, ' ');

				if (dummy)
				{
					sc = strdup(dummy + 1);

					/* lines are normally terminated with a
					 * CR/LF
					 */
					dummy = strchr(sc, '\r');
					if (dummy)
						*dummy = 0x00;
					dummy = strchr(sc, '\n');
					if (dummy)
						*dummy = 0x00;
				}
			}

			if (ask_compression && reply != NULL)
			{
				char *encoding = strstr(reply, "\nContent-Encoding:");
				if (encoding)
				{
					char *dummy = strchr(encoding + 1, '\n');
					if (dummy) *dummy = 0x00;
					dummy = strchr(sc, '\r');
					if (dummy) *dummy = 0x00;

					if (strstr(encoding, "gzip") == 0 || strstr(encoding, "deflate") == 0)
					{
						is_compressed = 1;
					}
				}
			}

			if (persistent_connections && show_bytes_xfer)
			{
				char *length = strstr(reply, "\nContent-Length:");
				if (!length)
				{
					snprintf(last_error, ERROR_BUFFER_SIZE, "'Content-Length'-header missing!\n");
					emit_error();
					close(fd);
					fd = -1;
					break;
				}
				len = atoi(&length[17]);
			}

			headers_len = (strstr(reply, "\r\n\r\n") - reply) + 4;

			free(reply);

			if (rc < 0)
			{
				if (persistent_connections)
				{
					if (++persistent_tries < 2)
					{
						close(fd);
						fd = -1;
						persistent_did_reconnect = 1;
						goto persistent_loop;
					}
				}

				if (rc == RC_SHORTREAD)
					snprintf(last_error, ERROR_BUFFER_SIZE, "error receiving reply from host\n");
				else if (rc == RC_TIMEOUT)
					snprintf(last_error, ERROR_BUFFER_SIZE, "timeout receiving reply from host\n");

				emit_error();

				close(fd);
				fd = -1;
				err++;

				break;
			}

			ok++;

			if (get_instead_of_head && show_Bps)
			{
				double dl_start = get_ts(), dl_end;
				int cur_limit = Bps_limit;

				if (persistent_connections)
				{
					if (cur_limit == -1 || len < cur_limit)
						cur_limit = len - overflow;
				}

				for(;;)
				{
					int n = cur_limit != -1 ? min(cur_limit - bytes_transferred, page_size) : page_size;
					int rc = read(fd, buffer, n);

					if (rc == -1)
					{
						if (errno != EINTR && errno != EAGAIN)
							error_exit("read failed");
					}
					else if (rc == 0)
						break;

					bytes_transferred += rc;

					if (cur_limit != -1 && bytes_transferred >= cur_limit)
						break;
				}

				dl_end = get_ts();

				Bps = bytes_transferred / max(dl_end - dl_start, 0.000001);
				Bps_min = min(Bps_min, Bps);
				Bps_max = max(Bps_max, Bps);
				Bps_avg += Bps;
			}

			dend = get_ts();

#ifndef NO_SSL
			if (use_ssl && !persistent_connections)
			{
				if (show_fp && ssl_h != NULL)
				{
					fp = get_fingerprint(ssl_h);
				}

				if (close_ssl_connection(ssl_h, fd) == -1)
				{
					snprintf(last_error, ERROR_BUFFER_SIZE, "error shutting down ssl\n");
					emit_error();
				}

				SSL_free(ssl_h);
				ssl_h = NULL;
			}
#endif

			if (!persistent_connections)
			{
				close(fd);
				fd = -1;
			}

			ms = (dend - dstart) * 1000.0;
			avg += ms;
			min = min > ms ? ms : min;
			max = max < ms ? ms : max;

			if (machine_readable)
			{
				if (sc)
				{
					char *dummy = strchr(sc, ' ');

					if (dummy) *dummy = 0x00;

					if (strstr(ok_str, sc))
					{
						printf("%f", ms);
					}
					else
					{
						printf("%s", err_str);
					}

					if (show_statuscodes)
						printf(" %s", sc);
				}
				else
				{
					printf("%s", err_str);
				}
				if(audible)
					putchar('\a');
				printf("\n");
			}
			else if (!quiet && !nagios_mode)
			{
				char current_host[1024];
				char *operation = !persistent_connections ? "connected to" : "pinged host";

				if (getnameinfo((const struct sockaddr *)&addr, sizeof(addr), current_host, sizeof(current_host), NULL, 0, NI_NUMERICHOST) == -1)
					snprintf(current_host, sizeof(current_host), "getnameinfo() failed: %d", errno);

				if (persistent_connections && show_bytes_xfer)
					printf("%s %s:%d (%d/%d bytes), seq=%d ", operation, current_host, portnr, headers_len, len, curncount-1);
				else
					printf("%s %s:%d (%d bytes), seq=%d ", operation, current_host, portnr, headers_len, curncount-1);

				if (split)
					printf("time=%.2f+%.2f=%.2f ms %s", (dafter_connect - dstart) * 1000.0, (dend - dafter_connect) * 1000.0, ms, sc?sc:"");
				else
					printf("time=%.2f ms %s", ms, sc?sc:"");

				if (persistent_did_reconnect)
				{
					printf(" C");
					persistent_did_reconnect = 0;
				}

				if (show_Bps)
				{
					printf(" %dKB/s", Bps / 1024);
					if (show_bytes_xfer)
						printf(" %dKB", (int)(bytes_transferred / 1024));
					if (ask_compression)
					{
						printf(" (");
						if (!is_compressed)
							printf("not ");
						printf("compressed)");
					}
				}

				if (use_ssl && show_fp && fp != NULL)
				{
					printf(" %s", fp);
					free(fp);
				}
				if(audible)
					putchar('\a');
				printf("\n");
			}

			if (show_statuscodes && ok_str != NULL && sc != NULL)
			{
				scdummy = strchr(sc, ' ');
				if (scdummy) *scdummy = 0x00;

				if (strstr(ok_str, sc) == NULL)
				{
					ok--;
					err++;
				}
			}

			free(sc);

			break;
		}

		if (curncount != count && !stop)
			usleep((useconds_t)(wait * 1000000.0));
	}

	avg_httping_time = avg / (double)ok;

	if (!quiet && !machine_readable && !nagios_mode)
	{
		printf("--- %s ping statistics ---\n", get);

		if (count == -1)
			printf("%d connects, %d ok, %3.2f%% failed\n", curncount, ok, (((double)err) / ((double)curncount)) * 100.0);
		else
			printf("%d connects, %d ok, %3.2f%% failed\n", curncount, ok, (((double)err) / ((double)count)) * 100.0);

		if (ok > 0)
		{
			printf("round-trip min/avg/max = %.1f/%.1f/%.1f ms\n", min, avg_httping_time, max);

			if (show_Bps)
				printf("Transfer speed: min/avg/max = %d/%d/%d KB\n", Bps_min / 1024, (int)(Bps_avg / ok) / 1024, Bps_max / 1024);
		}
	}

error_exit:
	if (nagios_mode == 1)
	{
		if (ok == 0)
		{
			printf("CRITICAL - connecting failed: %s", last_error);
			return 2;
		}
		else if (avg_httping_time >= nagios_crit)
		{
			printf("CRITICAL - average httping-time is %.1f\n", avg_httping_time);
			return 2;
		}
		else if (avg_httping_time >= nagios_warn)
		{
			printf("WARNING - average httping-time is %.1f\n", avg_httping_time);
			return 1;
		}

		printf("OK - average httping-time is %.1f (%s)|ping=%f\n", avg_httping_time, last_error, avg_httping_time);
	}
	else if (nagios_mode == 2)
	{
		if (ok && last_error[0] == 0x00)
		{
			printf("OK - all fine, avg httping time is %.1f|ping=%f\n", avg_httping_time, avg_httping_time);
			return 0;
		}

		printf("%s: - failed: %s", nagios_exit_code == 1?"WARNING":(nagios_exit_code == 2?"CRITICAL":"ERROR"), last_error);
		return nagios_exit_code;
	}

	if (ok)
		return 0;
	else
		return 127;
}



