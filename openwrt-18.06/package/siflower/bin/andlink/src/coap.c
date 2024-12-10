#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef _WIN32
#define strcasecmp _stricmp
#include "getopt.c"
#if !defined(S_ISDIR)
#define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
#endif
#else
#include <unistd.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#endif

#include <coap2/coap.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include "andlink.h"
#include "log.h"
#include "wifi.h"
#define DEFAULT_PORT  "5683"
#define DEFAULT_GATE_WAY "192.168.1.1"
#define NETWORK_CARD_NAME "sfi0"
#define COAP_DEBUG_LEVEL  (7)



#define MAX_USER 128 /* Maximum length of a user name (i.e., PSK
					  * identity) in bytes. */
#define MAX_KEY   64 /* Maximum length of a key (i.e., PSK) in bytes. */

int flags = 0;

static unsigned char _token_data[8];
coap_binary_t the_token = { 0, _token_data };

#define FLAGS_BLOCK 0x01

static coap_optlist_t *optlist = NULL;
/* Request URI.
 * TODO: associate the resources with transaction id and make it expireable */
static coap_uri_t uri;
static coap_string_t proxy = { 0, NULL };
static uint16_t proxy_port = COAP_DEFAULT_PORT;
static unsigned int ping_seconds = 0;

/* reading is done when this flag is set */
static int ready = 0;

static coap_string_t output_file = { 0, NULL };   /* output file name */
static FILE *file = NULL;               /* output file stream */

static coap_string_t payload = { 0, NULL };       /* optional payload to send */

static int reliable = 0;

unsigned char msgtype = COAP_MESSAGE_CON; /* usually, requests are sent confirmable */

int key_defined = 0;
static char *cert_file = NULL; /* Combined certificate and private key in PEM */
static char *ca_file = NULL;   /* CA for cert_file - for cert checking in PEM */
static char *root_ca_file = NULL; /* List of trusted Root CAs in PEM */

typedef unsigned char method_t;
method_t method = COAP_REQUEST_POST;                    /* the method we are using in our requests */

coap_block_t block = { .num = 0, .m = 0, .szx = 6 };
uint16_t last_block1_tid = 0;


unsigned int wait_seconds = 40;                /* default timeout in seconds */
unsigned int wait_ms = 0;
int wait_ms_reset = 0;
int obs_started = 0;
unsigned int obs_seconds = 30;          /* default observe time */
unsigned int obs_ms = 0;                /* timeout for current subscription */
int obs_ms_reset = 0;

#ifndef min
#define min(a,b) ((a) < (b) ? (a) : (b))
#endif

#ifdef __GNUC__
#define UNUSED_PARAM __attribute__ ((unused))
#else /* not a GCC */
#define UNUSED_PARAM
#endif /* GCC */

typedef enum {
	COAP_PING,
	COAP_SEARCH_GW,
	COAP_NET_INFO,
	COAP_MAX,
} coap_request_type;

static int
append_to_output(const uint8_t *data, size_t len) {
	size_t written;

	if (!file) {
		if (!output_file.s || (output_file.length && output_file.s[0] == '-'))
			file = stdout;
		else {
			if (!(file = fopen((char *)output_file.s, "w"))) {
				perror("fopen");
				return -1;
			}
		}
	}

	do {
		written = fwrite(data, 1, len, file);
		len -= written;
		data += written;
	} while ( written && len );
	fflush(file);

	return 0;
}

static void
close_output(void) {
	if (file) {

		/* add a newline before closing if no option '-o' was specified */
		if (!output_file.s)
			fwrite("\n", 1, 1, file);

		fflush(file);
		fclose(file);
	}
}

static coap_pdu_t *
coap_new_request(coap_context_t *ctx,
		coap_session_t *session,
		method_t m,
		coap_optlist_t **options,
		unsigned char *data,
		size_t length) {
	coap_pdu_t *pdu;
	(void)ctx;

	if (!(pdu = coap_new_pdu(session)))
		return NULL;

	pdu->type = msgtype;
	pdu->tid = coap_new_message_id(session);
	pdu->code = m;

	if ( !coap_add_token(pdu, the_token.length, the_token.s)) {
		LOG("cannot add token to request\n");
	}

	if (options)
		coap_add_optlist_pdu(pdu, options);

	if (length) {
		if ((flags & FLAGS_BLOCK) == 0)
			coap_add_data(pdu, length, data);
		else
			coap_add_block(pdu, length, data, block.num, block.szx);
	}

	return pdu;
}

static coap_tid_t
clear_obs(coap_context_t *ctx, coap_session_t *session) {
	coap_pdu_t *pdu;
	coap_optlist_t *option;
	coap_tid_t tid = COAP_INVALID_TID;
	unsigned char buf[2];
	(void)ctx;

	/* create bare PDU w/o any option  */
	pdu = coap_pdu_init(msgtype,
			COAP_REQUEST_GET,
			coap_new_message_id(session),
			coap_session_max_pdu_size(session));

	if (!pdu) {
		return tid;
	}

	if (!coap_add_token(pdu, the_token.length, the_token.s)) {
		LOG("cannot add token\n");
		goto error;
	}

	for (option = optlist; option; option = option->next ) {
		if (option->number == COAP_OPTION_URI_HOST) {
			if (!coap_add_option(pdu, option->number, option->length,
						option->data)) {
				goto error;
			}
			break;
		}
	}

	if (!coap_add_option(pdu,
				COAP_OPTION_OBSERVE,
				coap_encode_var_safe(buf, sizeof(buf), COAP_OBSERVE_CANCEL),
				buf)) {
	//	LOG(COAP_OBSERVE_CANCEL);
		goto error;
	}

	for (option = optlist; option; option = option->next ) {
		switch (option->number) {
			case COAP_OPTION_URI_PORT :
			case COAP_OPTION_URI_PATH :
			case COAP_OPTION_URI_QUERY :
				if (!coap_add_option(pdu, option->number, option->length,
							option->data)) {
					goto error;
				}
				break;
			default:
				;
		}
	}

	if (flags & FLAGS_BLOCK) {
		block.num = 0;
		block.m = 0;
		coap_add_option(pdu,
				COAP_OPTION_BLOCK2,
				coap_encode_var_safe(buf, sizeof(buf), (block.num << 4 | block.m << 3 | block.szx)),
				buf);
	}

	if (coap_get_log_level() < LOG_DEBUG)
		coap_show_pdu(LOG_INFO, pdu);


	tid = coap_send(session, pdu);

	if (tid == COAP_INVALID_TID)
		LOG("clear_obs: error sending new request\n");

	return tid;
error:

	coap_delete_pdu(pdu);
	return tid;
}

static int
resolve_address(const coap_str_const_t *server, struct sockaddr *dst) {

	struct addrinfo *res, *ainfo;
	struct addrinfo hints;
	static char addrstr[256];
	int error, len=-1;

	memset(addrstr, 0, sizeof(addrstr));
	if (server->length)
		memcpy(addrstr, server->s, server->length);
	else
		memcpy(addrstr, "localhost", 9);

	memset ((char *)&hints, 0, sizeof(hints));
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_family = AF_UNSPEC;

	error = getaddrinfo(addrstr, NULL, &hints, &res);

	if (error != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(error));
		return error;
	}

	for (ainfo = res; ainfo != NULL; ainfo = ainfo->ai_next) {
		switch (ainfo->ai_family) {
			case AF_INET6:
			case AF_INET:
				len = ainfo->ai_addrlen;
				memcpy(dst, ainfo->ai_addr, len);
				goto finish;
			default:
				;
		}
	}

finish:
	freeaddrinfo(res);
	return len;
}

#define HANDLE_BLOCK1(Pdu)                                        \
	((method == COAP_REQUEST_PUT || method == COAP_REQUEST_POST) && \
	 ((flags & FLAGS_BLOCK) == 0) &&                                \
	 ((Pdu)->hdr->code == COAP_RESPONSE_CODE(201) ||                \
	  (Pdu)->hdr->code == COAP_RESPONSE_CODE(204)))

static inline int
check_token(coap_pdu_t *received) {
	return received->token_length == the_token.length &&
		memcmp(received->token, the_token.s, the_token.length) == 0;
}

static int
event_handler(coap_context_t *ctx UNUSED_PARAM,
		coap_event_t event,
		struct coap_session_t *session UNUSED_PARAM) {

	switch(event) {
		case COAP_EVENT_DTLS_CLOSED:
		case COAP_EVENT_TCP_CLOSED:
		case COAP_EVENT_SESSION_CLOSED:
			{
			struct andlink *andlink = (struct andlink *)coap_get_app_data(ctx);
			andlink->coap_quit = 1;
			}
			break;
		default:
			break;
	}
	return 0;
}

static void
message_handler(struct coap_context_t *ctx,
		coap_session_t *session,
		coap_pdu_t *sent,
		coap_pdu_t *received,
		const coap_tid_t id UNUSED_PARAM) {

	coap_pdu_t *pdu = NULL;
	coap_opt_t *block_opt;
	coap_opt_iterator_t opt_iter;
	unsigned char buf[4];
	coap_optlist_t *option;
	size_t len;
	unsigned char *databuf;
	coap_tid_t tid;

#ifndef NDEBUG
	LOG("** process incoming %d.%02d response:\n",
			(received->code >> 5), received->code & 0x1F);
	if (coap_get_log_level() < LOG_DEBUG)
		coap_show_pdu(LOG_INFO, received);
#endif

	/* check if this is a response to our original request */
	if (!check_token(received)) {
		/* drop if this was just some message, or send RST in case of notification */
		if (!sent && (received->type == COAP_MESSAGE_CON ||
					received->type == COAP_MESSAGE_NON))
			coap_send_rst(session, received);
		return;
	}

	if (received->type == COAP_MESSAGE_RST) {
		LOG("got RST\n");
		return;
	}

	/* output the received data, if any */
	if (COAP_RESPONSE_CLASS(received->code) == 2) {

		/* set obs timer if we have successfully subscribed a resource */
		if (!obs_started && coap_check_option(received, COAP_OPTION_OBSERVE, &opt_iter)) {
			coap_log(LOG_DEBUG,
					"observation relationship established, set timeout to %d\n",
					obs_seconds);
			obs_started = 1;
			obs_ms = obs_seconds * 1000;
			obs_ms_reset = 1;
		}

		/* Got some data, check if block option is set. Behavior is undefined if
		 * both, Block1 and Block2 are present. */
		block_opt = coap_check_option(received, COAP_OPTION_BLOCK2, &opt_iter);
		if (block_opt) { /* handle Block2 */
			uint16_t blktype = opt_iter.type;

			/* TODO: check if we are looking at the correct block number */
			if (coap_get_data(received, &len, &databuf))
				append_to_output(databuf, len);

			if (coap_opt_block_num(block_opt) == 0) {
				/* See if observe is set in first response */
				ready = coap_check_option(received,
						COAP_OPTION_OBSERVE, &opt_iter) == NULL;
			}
			if(COAP_OPT_BLOCK_MORE(block_opt)) {
				/* more bit is set */
				LOG("found the M bit, block size is %u, block nr. %u\n",
						COAP_OPT_BLOCK_SZX(block_opt),
						coap_opt_block_num(block_opt));

				/* create pdu with request for next block */
				pdu = coap_new_request(ctx, session, method, NULL, NULL, 0); /* first, create bare PDU w/o any option  */
				if ( pdu ) {
					/* add URI components from optlist */
					for (option = optlist; option; option = option->next ) {
						switch (option->number) {
							case COAP_OPTION_URI_HOST :
							case COAP_OPTION_URI_PORT :
							case COAP_OPTION_URI_PATH :
							case COAP_OPTION_URI_QUERY :
								coap_add_option(pdu, option->number, option->length,
										option->data);
								break;
							default:
								;     /* skip other options */
						}
					}

					/* finally add updated block option from response, clear M bit */
					/* blocknr = (blocknr & 0xfffffff7) + 0x10; */
					LOG("query block %d\n",
							(coap_opt_block_num(block_opt) + 1));
					coap_add_option(pdu,
							blktype,
							coap_encode_var_safe(buf, sizeof(buf),
								((coap_opt_block_num(block_opt) + 1) << 4) |
								COAP_OPT_BLOCK_SZX(block_opt)), buf);

					tid = coap_send(session, pdu);

					if (tid == COAP_INVALID_TID) {
						LOG("message_handler: error sending new request\n");
					} else {
						wait_ms = wait_seconds * 1000;
						wait_ms_reset = 1;
					}

					return;
				}
			}
			return;
		} else { /* no Block2 option */
			block_opt = coap_check_option(received, COAP_OPTION_BLOCK1, &opt_iter);

			if (block_opt) { /* handle Block1 */
				unsigned int szx = COAP_OPT_BLOCK_SZX(block_opt);
				unsigned int num = coap_opt_block_num(block_opt);
				coap_log(LOG_DEBUG,
						"found Block1 option, block size is %u, block nr. %u\n",
						szx, num);
				if (szx != block.szx) {
					unsigned int bytes_sent = ((block.num + 1) << (block.szx + 4));
					if (bytes_sent % (1 << (szx + 4)) == 0) {
						/* Recompute the block number of the previous packet given the new block size */
						num = block.num = (bytes_sent >> (szx + 4)) - 1;
						block.szx = szx;
						coap_log(LOG_DEBUG,
								"new Block1 size is %u, block number %u completed\n",
								(1 << (block.szx + 4)), block.num);
					} else {
						LOG("ignoring request to increase Block1 size, "
								"next block is not aligned on requested block size boundary. "
								"(%u x %u mod %u = %u != 0)\n",
								block.num + 1, (1 << (block.szx + 4)), (1 << (szx + 4)),
								bytes_sent % (1 << (szx + 4)));
					}
				}

				if (payload.length <= (block.num+1) * (1 << (block.szx + 4))) {
					LOG("upload ready\n");
					ready = 1;
					return;
				}
				if (last_block1_tid == received->tid) {
					/*
					 * Duplicate BLOCK1 ACK
					 *
					 * RFCs not clear here, but on a lossy connection, there could
					 * be multiple BLOCK1 ACKs, causing the client to retransmit the
					 * same block multiple times.
					 *
					 * Once a block has been ACKd, there is no need to retransmit it.
					 */
					return;
				}
				last_block1_tid = received->tid;

				/* create pdu with request for next block */
				pdu = coap_new_request(ctx, session, method, NULL, NULL, 0); /* first, create bare PDU w/o any option  */
				if (pdu) {

					/* add URI components from optlist */
					for (option = optlist; option; option = option->next ) {
						switch (option->number) {
							case COAP_OPTION_URI_HOST :
							case COAP_OPTION_URI_PORT :
							case COAP_OPTION_URI_PATH :
							case COAP_OPTION_CONTENT_FORMAT :
							case COAP_OPTION_URI_QUERY :
								coap_add_option(pdu, option->number, option->length,
										option->data);
								break;
							default:
								;     /* skip other options */
						}
					}

					/* finally add updated block option from response, clear M bit */
					/* blocknr = (blocknr & 0xfffffff7) + 0x10; */
					block.num = num + 1;
					block.m = ((block.num+1) * (1 << (block.szx + 4)) < payload.length);

				//	LOG(block.num);
					coap_add_option(pdu,
							COAP_OPTION_BLOCK1,
							coap_encode_var_safe(buf, sizeof(buf),
								(block.num << 4) | (block.m << 3) | block.szx), buf);

					coap_add_block(pdu,
							payload.length,
							payload.s,
							block.num,
							block.szx);
					if (coap_get_log_level() < LOG_DEBUG)
						coap_show_pdu(LOG_INFO, pdu);

					tid = coap_send(session, pdu);

					if (tid == COAP_INVALID_TID) {
						LOG("message_handler: error sending new request\n");
					} else {
						wait_ms = wait_seconds * 1000;
						wait_ms_reset = 1;
					}

					return;
				}
			} else {
				/* There is no block option set, just read the data and we are done. */
				if (coap_get_data(received, &len, &databuf)) {
					append_to_output(databuf, len);
					struct andlink *andlink = (struct andlink*)coap_get_app_data(ctx);
					LOG("%s session %p\n", __func__, session);
					assert(andlink);
					if (andlink->coap_type == COAP_PING
						|| andlink->coap_type == COAP_SEARCH_GW) {
						andlink->gw_detected = true;
						coap_address_copy(&andlink->gw_addr, &session->remote_addr);
					} else if (andlink->coap_type == COAP_NET_INFO) {
						//TODO save recieved message and get net info from message.
					}
				}
			}
		}
	} else {      /* no 2.05 */

		/* check if an error was signaled and output payload if so */
		if (COAP_RESPONSE_CLASS(received->code) >= 4) {
			fprintf(stderr, "%d.%02d",
					(received->code >> 5), received->code & 0x1F);
			if (coap_get_data(received, &len, &databuf)) {
				fprintf(stderr, " ");
				while(len--)
					fprintf(stderr, "%c", *databuf++);
			}
			fprintf(stderr, "\n");
		}

	}

	/* any pdu that has been created in this function must be sent by now */
	assert(pdu == NULL);

	/* our job is done, we can exit at any time */
	ready = coap_check_option(received, COAP_OPTION_OBSERVE, &opt_iter) == NULL;
}

static void
usage( const char *program, const char *version) {
	const char *p;
	char buffer[64];

	p = strrchr( program, '/' );
	if ( p )
		program = ++p;

	fprintf( stderr, "%s v%s -- a small CoAP implementation\n"
			"Copyright (C) 2010-2019 Olaf Bergmann <bergmann@tzi.org> and others\n\n"
			"%s\n\n"
			"Usage: %s [-a addr] [-b [num,]size] [-e text] [-f file] [-l loss]\n"
			"\t\t[-m method] [-o file] [-p port] [-r] [-s duration] [-t type]\n"
			"\t\t[-v num] [-A type] [-B seconds] [-K interval] [-N] [-O num,text]\n"
			"\t\t[-P addr[:port]] [-T token] [-U]\n"
			"\t\t[[-k key] [-u user]]\n"
			"\t\t[[-c certfile] [-C cafile] [-R root_cafile]] URI\n\n"
			"\tURI can be an absolute URI or a URI prefixed with scheme and host\n\n"
			"General Options\n"
			"\t-a addr\t\tThe local interface address to use\n"
			"\t-b [num,]size\tBlock size to be used in GET/PUT/POST requests\n"
			"\t       \t\t(value must be a multiple of 16 not larger than 1024)\n"
			"\t       \t\tIf num is present, the request chain will start at\n"
			"\t       \t\tblock num\n"
			"\t-e text\t\tInclude text as payload (use percent-encoding for\n"
			"\t       \t\tnon-ASCII characters)\n"
			"\t-f file\t\tFile to send with PUT/POST (use '-' for STDIN)\n"
			"\t-l list\t\tFail to send some datagrams specified by a comma\n"
			"\t       \t\tseparated list of numbers or number ranges\n"
			"\t       \t\t(for debugging only)\n"
			"\t-l loss%%\tRandomly fail to send datagrams with the specified\n"
			"\t       \t\tprobability - 100%% all datagrams, 0%% no datagrams\n"
			"\t-m method\tRequest method (get|put|post|delete|fetch|patch|ipatch),\n"
			"\t       \t\tdefault is 'get'\n"
			"\t-o file\t\tOutput received data to this file (use '-' for STDOUT)\n"
			"\t-p port\t\tListen on specified port\n"
			"\t-r     \t\tUse reliable protocol (TCP or TLS)\n"
			"\t-s duration\tSubscribe to / Observe resource for given duration\n"
			"\t       \t\tin seconds\n"
			"\t-t type\t\tContent format for given resource for PUT/POST\n"
			"\t-v num \t\tVerbosity level (default 3, maximum is 9). Above 7,\n"
			"\t       \t\tthere is increased verbosity in GnuTLS logging\n"
			"\t-A type\t\tAccepted media type\n"
			"\t-B seconds\tBreak operation after waiting given seconds\n"
			"\t       \t\t(default is %d)\n"
			"\t-K interval\tsend a ping after interval seconds of inactivity\n"
			"\t       \t\t(TCP only)\n"
			"\t-N     \t\tSend NON-confirmable message\n"
			"\t-O num,text\tAdd option num with contents text to request\n"
			"\t-P addr[:port]\tUse proxy (automatically adds Proxy-Uri option to\n"
			"\t       \t\trequest)\n"
			"\t-T token\tInclude specified token\n"
			"\t-U     \t\tNever include Uri-Host or Uri-Port options\n"
			"PSK Options (if supported by underlying (D)TLS library)\n"
			"\t-k key \t\tPre-shared key for the specified user\n"
			"\t-u user\t\tUser identity for pre-shared key mode\n"
			"PKI Options (if supported by underlying (D)TLS library)\n"
			"\t-c certfile\tPEM file containing both CERTIFICATE and PRIVATE KEY\n"
			"\t       \t\tThis argument requires (D)TLS with PKI to be available\n"
			"\t-C cafile\tPEM file containing the CA Certificate that was used to\n"
			"\t       \t\tsign the certfile. This will trigger the validation of\n"
			"\t       \t\tthe server certificate.  If certfile is self-signed (as\n"
			"\t       \t\tdefined by '-c certfile'), then you need to have on the\n"
			"\t       \t\tcommand line the same filename for both the certfile and\n"
			"\t       \t\tcafile (as in '-c certfile -C certfile') to trigger\n"
			"\t       \t\tvalidation\n"
			"\t-R root_cafile\tPEM file containing the set of trusted root CAs that\n"
			"\t       \t\tare to be used to validate the server certificate.\n"
			"\t       \t\tThe '-C cafile' does not have to be in this list and is\n"
			"\t       \t\t'trusted' for the verification.\n"
			"\t       \t\tAlternatively, this can point to a directory containing\n"
			"\t       \t\ta set of CA PEM files\n"
			"\n"
			"Examples:\n"
			"\tcoap-client -m get coap://[::1]/\n"
			"\tcoap-client -m get coap://[::1]/.well-known/core\n"
			"\tcoap-client -m get coap+tcp://[::1]/.well-known/core\n"
			"\tcoap-client -m get coaps://[::1]/.well-known/core\n"
			"\tcoap-client -m get coaps+tcp://[::1]/.well-known/core\n"
			"\tcoap-client -m get -T cafe coap://[::1]/time\n"
			"\techo -n 1000 | coap-client -m put -T cafe coap://[::1]/time -f -\n"
			,program, version, coap_string_tls_version(buffer, sizeof(buffer))
			,program, wait_seconds);
}

typedef struct {
	unsigned char code;
	const char *media_type;
} content_type_t;

static void
cmdline_content_type(char *arg, uint16_t key) {
	static content_type_t content_types[] = {
		{  0, "plain" },
		{  0, "text/plain" },
		{ 40, "link" },
		{ 40, "link-format" },
		{ 40, "application/link-format" },
		{ 41, "xml" },
		{ 41, "application/xml" },
		{ 42, "binary" },
		{ 42, "octet-stream" },
		{ 42, "application/octet-stream" },
		{ 47, "exi" },
		{ 47, "application/exi" },
		{ 50, "json" },
		{ 50, "application/json" },
		{ 60, "cbor" },
		{ 60, "application/cbor" },
		{ 255, NULL }
	};
	coap_optlist_t *node;
	unsigned char i;
	uint16_t value;
	uint8_t buf[2];

	if (isdigit(*arg)) {
		value = atoi(arg);
	} else {
		for (i=0;
				content_types[i].media_type &&
				strncmp(arg, content_types[i].media_type, strlen(arg)) != 0 ;
				++i)
			;

		if (content_types[i].media_type) {
			value = content_types[i].code;
		} else {
			LOG("W: unknown content-format '%s'\n",arg);
			return;
		}
	}

	node = coap_new_optlist(key, coap_encode_var_safe(buf, sizeof(buf), value), buf);
	if (node) {
		coap_insert_optlist(&optlist, node);
	}
}

static uint16_t
get_default_port(const coap_uri_t *u) {
	return coap_uri_scheme_is_secure(u) ? COAPS_DEFAULT_PORT : COAP_DEFAULT_PORT;
}

/**
 * Sets global URI options according to the URI passed as @p arg.
 * This function returns 0 on success or -1 on error.
 *
 * @param arg             The URI string.
 * @param create_uri_opts Flags that indicate whether Uri-Host and
 *                        Uri-Port should be suppressed.
 * @return 0 on success, -1 otherwise
 */
static int
cmdline_uri(char *arg, int create_uri_opts) {
	unsigned char portbuf[2];
#define BUFSIZE 40
	unsigned char _buf[BUFSIZE];
	unsigned char *buf = _buf;
	size_t buflen;
	int res;

	if (proxy.length) {   /* create Proxy-Uri from argument */
		size_t len = strlen(arg);
		while (len > 270) {
			coap_insert_optlist(&optlist,
					coap_new_optlist(COAP_OPTION_PROXY_URI,
						270,
						(unsigned char *)arg));

			len -= 270;
			arg += 270;
		}

		coap_insert_optlist(&optlist,
				coap_new_optlist(COAP_OPTION_PROXY_URI,
					len,
					(unsigned char *)arg));

	} else {      /* split arg into Uri-* options */
		if (coap_split_uri((unsigned char *)arg, strlen(arg), &uri) < 0) {
			LOG("invalid CoAP URI\n");
			return -1;
		}

		if (uri.scheme==COAP_URI_SCHEME_COAPS && !reliable && !coap_dtls_is_supported()) {
			coap_log(LOG_EMERG,
					"coaps URI scheme not supported in this version of libcoap\n");
			return -1;
		}

		if ((uri.scheme==COAP_URI_SCHEME_COAPS_TCP || (uri.scheme==COAP_URI_SCHEME_COAPS && reliable)) && !coap_tls_is_supported()) {
			coap_log(LOG_EMERG,
					"coaps+tcp URI scheme not supported in this version of libcoap\n");
			return -1;
		}

		if (uri.port != get_default_port(&uri) && create_uri_opts) {
			coap_insert_optlist(&optlist,
					coap_new_optlist(COAP_OPTION_URI_PORT,
						coap_encode_var_safe(portbuf, sizeof(portbuf),
							(uri.port & 0xffff)),
						portbuf));
		}

		if (uri.path.length) {
			buflen = BUFSIZE;
			res = coap_split_path(uri.path.s, uri.path.length, buf, &buflen);

			while (res--) {
				coap_insert_optlist(&optlist,
						coap_new_optlist(COAP_OPTION_URI_PATH,
							coap_opt_length(buf),
							coap_opt_value(buf)));

				buf += coap_opt_size(buf);
			}
		}

		if (uri.query.length) {
			buflen = BUFSIZE;
			buf = _buf;
			res = coap_split_query(uri.query.s, uri.query.length, buf, &buflen);

			while (res--) {
				coap_insert_optlist(&optlist,
						coap_new_optlist(COAP_OPTION_URI_QUERY,
							coap_opt_length(buf),
							coap_opt_value(buf)));

				buf += coap_opt_size(buf);
			}
		}
	}

	return 0;
}

static int
cmdline_blocksize(char *arg) {
	uint16_t size;

again:
	size = 0;
	while(*arg && *arg != ',')
		size = size * 10 + (*arg++ - '0');

	if (*arg == ',') {
		arg++;
		block.num = size;
		goto again;
	}

	if (size)
		block.szx = (coap_fls(size >> 4) - 1) & 0x07;

	flags |= FLAGS_BLOCK;
	return 1;
}

/* Called after processing the options from the commandline to set
 * Block1 or Block2 depending on method. */
static void
set_blocksize(void) {
	static unsigned char buf[4];        /* hack: temporarily take encoded bytes */
	uint16_t opt;
	unsigned int opt_length;

	if (method != COAP_REQUEST_DELETE) {
		opt = method == COAP_REQUEST_GET ? COAP_OPTION_BLOCK2 : COAP_OPTION_BLOCK1;

		block.m = (opt == COAP_OPTION_BLOCK1) &&
			((1u << (block.szx + 4)) < payload.length);

		opt_length = coap_encode_var_safe(buf, sizeof(buf),
				(block.num << 4 | block.m << 3 | block.szx));

		coap_insert_optlist(&optlist, coap_new_optlist(opt, opt_length, buf));
	}
}

static void
cmdline_subscribe(char *arg) {
	obs_seconds = atoi(arg);
	coap_insert_optlist(&optlist, coap_new_optlist(COAP_OPTION_OBSERVE,
				COAP_OBSERVE_ESTABLISH, NULL));
}

static int
cmdline_proxy(char *arg) {
	char *proxy_port_str = strrchr((const char *)arg, ':'); /* explicit port ? */
	if (proxy_port_str) {
		char *ipv6_delimiter = strrchr((const char *)arg, ']');
		if (!ipv6_delimiter) {
			if (proxy_port_str == strchr((const char *)arg, ':')) {
				/* host:port format - host not in ipv6 hexadecimal string format */
				*proxy_port_str++ = '\0'; /* split */
				proxy_port = atoi(proxy_port_str);
			}
		} else {
			arg = strchr((const char *)arg, '[');
			if (!arg) return 0;
			arg++;
			*ipv6_delimiter = '\0'; /* split */
			if (ipv6_delimiter + 1 == proxy_port_str++) {
				/* [ipv6 address]:port */
				proxy_port = atoi(proxy_port_str);
			}
		}
	}

	proxy.length = strlen(arg);
	if ( (proxy.s = coap_malloc(proxy.length + 1)) == NULL) {
		proxy.length = 0;
		return 0;
	}

	memcpy(proxy.s, arg, proxy.length+1);
	return 1;
}

static inline void
cmdline_token(char *arg) {
	the_token.length = min(sizeof(_token_data), strlen(arg));
	if (the_token.length > 0) {
		memcpy((char *)the_token.s, arg, the_token.length);
	}
}

static void
cmdline_option(char *arg) {
	unsigned int num = 0;

	while (*arg && *arg != ',') {
		num = num * 10 + (*arg - '0');
		++arg;
	}
	if (*arg == ',')
		++arg;

	coap_insert_optlist(&optlist,
			coap_new_optlist(num, strlen(arg), (unsigned char *)arg));
}

/**
 * Calculates decimal value from hexadecimal ASCII character given in
 * @p c. The caller must ensure that @p c actually represents a valid
 * heaxdecimal character, e.g. with isxdigit(3).
 *
 * @hideinitializer
 */
#define hexchar_to_dec(c) ((c) & 0x40 ? ((c) & 0x0F) + 9 : ((c) & 0x0F))

/**
 * Decodes percent-encoded characters while copying the string @p seg
 * of size @p length to @p buf. The caller of this function must
 * ensure that the percent-encodings are correct (i.e. the character
 * '%' is always followed by two hex digits. and that @p buf provides
 * sufficient space to hold the result. This function is supposed to
 * be called by make_decoded_option() only.
 *
 * @param seg     The segment to decode and copy.
 * @param length  Length of @p seg.
 * @param buf     The result buffer.
 */
static void
decode_segment(const uint8_t *seg, size_t length, unsigned char *buf) {

	while (length--) {

		if (*seg == '%') {
			*buf = (hexchar_to_dec(seg[1]) << 4) + hexchar_to_dec(seg[2]);

			seg += 2; length -= 2;
		} else {
			*buf = *seg;
		}

		++buf; ++seg;
	}
}

/**
 * Runs through the given path (or query) segment and checks if
 * percent-encodings are correct. This function returns @c -1 on error
 * or the length of @p s when decoded.
 */
static int
check_segment(const uint8_t *s, size_t length) {

	size_t n = 0;

	while (length) {
		if (*s == '%') {
			if (length < 2 || !(isxdigit(s[1]) && isxdigit(s[2])))
				return -1;

			s += 2;
			length -= 2;
		}

		++s; ++n; --length;
	}

	return n;
}

static int
cmdline_input(char *text, coap_string_t *buf) {
	int len;
	len = check_segment((unsigned char *)text, strlen(text));

	if (len < 0)
		return 0;

	buf->s = (unsigned char *)coap_malloc(len);
	if (!buf->s)
		return 0;

	buf->length = len;
	decode_segment((unsigned char *)text, strlen(text), buf->s);
	return 1;
}

static int
cmdline_input_from_file(char *filename, coap_string_t *buf) {
	FILE *inputfile = NULL;
	ssize_t len;
	int result = 1;
	struct stat statbuf;

	if (!filename || !buf)
		return 0;

	if (filename[0] == '-' && !filename[1]) { /* read from stdin */
		buf->length = 20000;
		buf->s = (unsigned char *)coap_malloc(buf->length);
		if (!buf->s)
			return 0;

		inputfile = stdin;
	} else {
		/* read from specified input file */
		inputfile = fopen(filename, "r");
		if ( !inputfile ) {
			perror("cmdline_input_from_file: fopen");
			return 0;
		}

		if (fstat(fileno(inputfile), &statbuf) < 0) {
			perror("cmdline_input_from_file: stat");
			fclose(inputfile);
			return 0;
		}

		buf->length = statbuf.st_size;
		buf->s = (unsigned char *)coap_malloc(buf->length);
		if (!buf->s) {
			fclose(inputfile);
			return 0;
		}
	}

	len = fread(buf->s, 1, buf->length, inputfile);

	if (len < 0 || ((size_t)len < buf->length)) {
		if (ferror(inputfile) != 0) {
			perror("cmdline_input_from_file: fread");
			coap_free(buf->s);
			buf->length = 0;
			buf->s = NULL;
			result = 0;
		} else {
			buf->length = len;
		}
	}

	if (inputfile != stdin)
		fclose(inputfile);

	return result;
}

static method_t
cmdline_method(char *arg) {
	static const char *methods[] =
	{ 0, "get", "post", "put", "delete", "fetch", "patch", "ipatch", 0};
	unsigned char i;

	for (i=1; methods[i] && strcasecmp(arg,methods[i]) != 0 ; ++i)
		;

	return i;     /* note that we do not prevent illegal methods */
}

static ssize_t
cmdline_read_user(char *arg, unsigned char *buf, size_t maxlen) {
	size_t len = strnlen(arg, maxlen);
	if (len) {
		memcpy(buf, arg, len);
	}
	return len;
}

static ssize_t
cmdline_read_key(char *arg, unsigned char *buf, size_t maxlen) {
	size_t len = strnlen(arg, maxlen);
	if (len) {
		memcpy(buf, arg, len);
		return len;
	}
	return -1;
}

static int
verify_cn_callback(const char *cn,
		const uint8_t *asn1_public_cert UNUSED_PARAM,
		size_t asn1_length UNUSED_PARAM,
		coap_session_t *session UNUSED_PARAM,
		unsigned depth,
		int validated UNUSED_PARAM,
		void *arg UNUSED_PARAM
		) {
	LOG("CN '%s' presented by server (%s)\n",
			cn, depth ? "CA" : "Certificate");
	return 1;
}

static coap_dtls_pki_t *
setup_pki(coap_context_t *ctx) {
	static coap_dtls_pki_t dtls_pki;
	static char client_sni[256];

	/* If general root CAs are defined */
	if (root_ca_file) {
		struct stat stbuf;
		if ((stat(root_ca_file, &stbuf) == 0) && S_ISDIR(stbuf.st_mode)) {
			coap_context_set_pki_root_cas(ctx, NULL, root_ca_file);
		} else {
			coap_context_set_pki_root_cas(ctx, root_ca_file, NULL);
		}
	}

	memset (&dtls_pki, 0, sizeof(dtls_pki));
	dtls_pki.version = COAP_DTLS_PKI_SETUP_VERSION;
	if (ca_file || root_ca_file) {
		/*
		 * Add in additional certificate checking.
		 * This list of enabled can be tuned for the specific
		 * requirements - see 'man coap_encryption'.
		 *
		 * Note: root_ca_file is setup separately using
		 * coap_context_set_pki_root_cas(), but this is used to define what
		 * checking actually takes place.
		 */
		dtls_pki.verify_peer_cert        = 1;
		dtls_pki.require_peer_cert       = 1;
		dtls_pki.allow_self_signed       = 1;
		dtls_pki.allow_expired_certs     = 1;
		dtls_pki.cert_chain_validation   = 1;
		dtls_pki.cert_chain_verify_depth = 2;
		dtls_pki.check_cert_revocation   = 1;
		dtls_pki.allow_no_crl            = 1;
		dtls_pki.allow_expired_crl       = 1;
		dtls_pki.validate_cn_call_back   = verify_cn_callback;
		dtls_pki.cn_call_back_arg        = NULL;
		dtls_pki.validate_sni_call_back  = NULL;
		dtls_pki.sni_call_back_arg       = NULL;
		memset(client_sni, 0, sizeof(client_sni));
		if (uri.host.length)
			memcpy(client_sni, uri.host.s, min(uri.host.length, sizeof(client_sni)));
		else
			memcpy(client_sni, "localhost", 9);
		dtls_pki.client_sni = client_sni;
	}
	dtls_pki.pki_key.key_type = COAP_PKI_KEY_PEM;
	dtls_pki.pki_key.key.pem.public_cert = cert_file;
	dtls_pki.pki_key.key.pem.private_key = cert_file;
	dtls_pki.pki_key.key.pem.ca_file = ca_file;
	return &dtls_pki;
}

#ifdef _WIN32
#define S_ISDIR(x) (((x) & S_IFMT) == S_IFDIR)
#endif

static coap_session_t *
get_session(
		coap_context_t *ctx,
		const char *local_addr,
		const char *local_port,
		coap_proto_t proto,
		coap_address_t *dst,
		const char *identity,
		const uint8_t *key,
		unsigned key_len
		) {
	coap_session_t *session = NULL;
	LOG("%s %d\n", __func__, __LINE__);

	if ( local_addr ) {
		int s;
		struct addrinfo hints;
		struct addrinfo *result = NULL, *rp;

		memset( &hints, 0, sizeof( struct addrinfo ) );
		hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
		hints.ai_socktype = COAP_PROTO_RELIABLE(proto) ? SOCK_STREAM : SOCK_DGRAM; /* Coap uses UDP */
		hints.ai_flags = AI_PASSIVE | AI_NUMERICHOST | AI_NUMERICSERV | AI_ALL;

		s = getaddrinfo( local_addr, local_port, &hints, &result );
		if ( s != 0 ) {
			fprintf( stderr, "getaddrinfo: %s\n", gai_strerror( s ) );
			return NULL;
		}

		/* iterate through results until success */
		for ( rp = result; rp != NULL; rp = rp->ai_next ) {
			coap_address_t bind_addr;
			if ( rp->ai_addrlen <= sizeof( bind_addr.addr ) ) {
				coap_address_init( &bind_addr );
				bind_addr.size = rp->ai_addrlen;
				memcpy( &bind_addr.addr, rp->ai_addr, rp->ai_addrlen );
				if ((root_ca_file || ca_file || cert_file) &&
						(proto == COAP_PROTO_DTLS || proto == COAP_PROTO_TLS)) {
					coap_dtls_pki_t *dtls_pki = setup_pki(ctx);
					session = coap_new_client_session_pki(ctx, &bind_addr, dst, proto, dtls_pki);
				}
				else if ((identity || key) &&
						(proto == COAP_PROTO_DTLS || proto == COAP_PROTO_TLS) ) {
					session = coap_new_client_session_psk( ctx, &bind_addr, dst, proto,
							identity, key, key_len );
				}
				else {
					session = coap_new_client_session( ctx, &bind_addr, dst, proto );
				}
				if ( session )
					break;
			}
		}
		freeaddrinfo( result );
	} else {
		LOG("%s %d\n", __func__, __LINE__);
		if ((root_ca_file || ca_file || cert_file) &&
				(proto == COAP_PROTO_DTLS || proto == COAP_PROTO_TLS)) {
			coap_dtls_pki_t *dtls_pki = setup_pki(ctx);
			session = coap_new_client_session_pki(ctx, NULL, dst, proto, dtls_pki);
		}
		else if ((identity || key) &&
				(proto == COAP_PROTO_DTLS || proto == COAP_PROTO_TLS) )
			session = coap_new_client_session_psk( ctx, NULL, dst, proto,
					identity, key, key_len );
		else {
			session = coap_new_client_session( ctx, NULL, dst, proto );
		}
	}
	return session;
}

static int coap_send_request(struct andlink *andlink, char *uri_str, char *data)
{
	coap_context_t  *ctx = NULL;
	coap_session_t *session = NULL;
	coap_address_t dst;
	static char addr[INET6_ADDRSTRLEN];
	void *addrptr = NULL;
	int result = -1;
	coap_pdu_t  *pdu;
	static coap_str_const_t server;
	uint16_t port = COAP_DEFAULT_PORT;
	char port_str[NI_MAXSERV] = "0";
	int opt, res;
	coap_log_t log_level = LOG_WARNING;
	unsigned char user[MAX_USER + 1], key[MAX_KEY];
	ssize_t user_length = 0, key_length = 0;
	int create_uri_opts = 1;
	coap_startup();
	coap_dtls_set_log_level(log_level);
	coap_set_log_level(log_level);
	bool timeout = false;


	memset(&uri, 0, sizeof(coap_uri_t));
	ready = 0;
	memset(&dst, 0, sizeof(coap_address_t));
	if (cmdline_uri(uri_str, create_uri_opts) < 0) {
		LOG("%s parse uri(%s) failed\n", __func__, uri);
		return result;
	}

	LOG("%s dst %p\n", __func__, &dst);

	if ( (user_length < 0 ) || ( key_length < 0 ) ) {
		LOG("Invalid user name or key specified\n");
		goto finish;
	}
	if (proxy.length) {
		server.length = proxy.length;
		server.s = proxy.s;
		port = proxy_port;
	} else {
		server = uri.host;
		port = uri.port;
	}

	/* resolve destination address where server should be sent */
	res = resolve_address(&server, &dst.addr.sa);

	if (res < 0) {
		LOG("failed to resolve address\n");
		goto finish;
	}

	ctx = coap_new_context( NULL );
	if ( !ctx ) {
		LOG("cannot create context\n");
		goto finish;
	}


	coap_context_set_keepalive(ctx, ping_seconds);

	dst.size = res;
	dst.addr.sin.sin_port = htons(port);

	session = get_session(
			ctx,
			NULL, port_str,
			uri.scheme==COAP_URI_SCHEME_COAP_TCP ? COAP_PROTO_TCP :
			uri.scheme==COAP_URI_SCHEME_COAPS_TCP ? COAP_PROTO_TLS :
			(reliable ?
			 uri.scheme==COAP_URI_SCHEME_COAPS ? COAP_PROTO_TLS : COAP_PROTO_TCP
			 : uri.scheme==COAP_URI_SCHEME_COAPS ? COAP_PROTO_DTLS : COAP_PROTO_UDP),
			&dst,
			user_length > 0 ? (const char *)user : NULL,
			key_length > 0  ? key : NULL, (unsigned)key_length
			);

	if (!session) {
		LOG("cannot create client session\n");
		goto finish;
	}

	LOG("%s session %p\n", __func__, session);
	coap_set_app_data(ctx, andlink);

	/* add Uri-Host if server address differs from uri.host */

	switch (dst.addr.sa.sa_family) {
		case AF_INET:
			addrptr = &dst.addr.sin.sin_addr;
			/* create context for IPv4 */
			break;
		case AF_INET6:
			addrptr = &dst.addr.sin6.sin6_addr;
			break;
		default:
			;
	}

	coap_register_option(ctx, COAP_OPTION_BLOCK2);
	coap_register_response_handler(ctx, message_handler);
	coap_register_event_handler(ctx, event_handler);

	/* construct CoAP message */

	if (!proxy.length && addrptr
			&& (inet_ntop(dst.addr.sa.sa_family, addrptr, addr, sizeof(addr)) != 0)
			&& (strlen(addr) != uri.host.length
				|| memcmp(addr, uri.host.s, uri.host.length) != 0)
	   ) {
		/* add Uri-Host */

		coap_insert_optlist(&optlist,
				coap_new_optlist(COAP_OPTION_URI_HOST,
					uri.host.length,
					uri.host.s));
	}

	/* set block option if requested at commandline */
	if (flags & FLAGS_BLOCK)
		set_blocksize();

	//initial payload
	payload.s = NULL;
	payload.length = 0;

	if (data)
		cmdline_input(data, &payload);

	if (!(pdu = coap_new_request(ctx, session, method, &optlist, payload.s, payload.length))) {
		LOG("%s coap new request failed\n", __func__);
		goto finish;
	}

	LOG("sending CoAP request:\n");
	coap_show_pdu(LOG_INFO, pdu);

	coap_send(session, pdu);

	wait_ms = wait_seconds * 1000;
	LOG("timeout is set to %u seconds\n", wait_seconds);

	while (!andlink->coap_quit && !(ready && coap_can_exit(ctx)))
	{

		result = coap_run_once( ctx, wait_ms == 0 ?
				obs_ms : obs_ms == 0 ?
				min(wait_ms, 1000) : min( wait_ms, obs_ms ) );

		if ( result >= 0 ) {
			if ( wait_ms > 0 && !wait_ms_reset ) {
				if ( (unsigned)result >= wait_ms ) {
					LOG("timeout\n");
					timeout = true;
					break;
				} else {
					wait_ms -= result;
				}
			}
			if ( obs_ms > 0 && !obs_ms_reset ) {
				if ( (unsigned)result >= obs_ms ) {
					LOG("clear observation relationship\n" );
					clear_obs( ctx, session ); /* FIXME: handle error case COAP_TID_INVALID */

					/* make sure that the obs timer does not fire again */
					obs_ms = 0;
					obs_seconds = 0;
				} else {
					obs_ms -= result;
				}
			}
			wait_ms_reset = 0;
			obs_ms_reset = 0;
		}
	}
	if (andlink->coap_quit)
		andlink->coap_quit = 0;

	if (timeout)
		result = -ETIMEDOUT;
	else
		result = 0;

finish:
	coap_delete_optlist(optlist);
	optlist = NULL;
	coap_session_release( session );
	coap_free_context(ctx);
	coap_cleanup();
	close_output();
	return result;
}

static int get_boardcast_ip(char *bcast)
{
	int ret = 0;
	int s = socket(AF_INET, SOCK_DGRAM, 0);
	if (s < 0) {
		LOG("Create socket failed!errno=%d\n", errno);
		return -1;
	}
	struct ifreq ifr;
	unsigned long nBroadIP;
	strcpy(ifr.ifr_name, NETWORK_CARD_NAME);
	if (ioctl(s, SIOCGIFBRDADDR, &ifr) < 0) {
		nBroadIP = 0;
		ret = 0;
	} else {
		nBroadIP = *(unsigned long*)&ifr.ifr_broadaddr.sa_data[2];
	}
	LOG("BroadIP: %s\n", inet_ntoa(*(struct in_addr*)&nBroadIP));
	strcpy(bcast, strdup(inet_ntoa(*(struct in_addr*)&nBroadIP)));
	close(s);
	return ret;
}

static int get_netwrok_ip(char *ip)
{
	int ret = 0;
	int s = socket(AF_INET, SOCK_DGRAM, 0);
	if (s < 0) {
		LOG("Create socket failed!errno=%d\n", errno);
		return -1;
	}

	struct ifreq ifr;
	unsigned long nip;
	strcpy(ifr.ifr_name, NETWORK_CARD_NAME);
	if (ioctl(s, SIOCGIFADDR, &ifr) < 0) {
		nip = 0;
		ret = 0;
	} else {
		nip = *(unsigned long*)&ifr.ifr_broadaddr.sa_data[2];
	}
	LOG("BroadIP: %s\n", inet_ntoa(*(struct in_addr*)&nip));
	strcpy(ip, strdup(inet_ntoa(*(struct in_addr*)&nip)));
	close(s);
	return ret;
}



static void coap_notify(coap_context_t *ctx UNUSED_PARAM,
		struct coap_resource_t *resource,
		coap_session_t *session UNUSED_PARAM,
		coap_pdu_t *request,
		coap_binary_t *token UNUSED_PARAM,
		coap_string_t *query UNUSED_PARAM,
		coap_pdu_t *response)
{
	size_t size;
	unsigned char *data;
	int ret, len;
	char buf[32] = { 0 };

	/* coap_get_data() sets size to 0 on error */
	coap_get_data(request, &size, &data);
	LOG("%s recieved data %s\n", __func__, data);
	if (size == 0) {
		LOG("%s coap get data failed\n", __func__);
		return ;
	} else {
		ret = parse_net_data(data, size);
		if (!ret) {
			struct andlink *andlink = (struct andlink*)coap_get_app_data(ctx);
			if (andlink) {
				pthread_mutex_lock(&andlink->wifi_lock);
				andlink->change_wifi_state = true;
				andlink->wifi_mode = STATION;
				pthread_mutex_unlock(&andlink->wifi_lock);
				pthread_cond_signal(&andlink->wifi_cond);
			}
			//coap_add_data(response, 14, (const uint8_t *)"{\"result\" : 0}");
			sprintf(buf, "{\"result\" : 0}");
		} else {
			//coap_add_data(response, 14, (const uint8_t *)"{\"result\" : 1}");
			sprintf(buf, "{\"result\" : 1}");
		}
		len = strlen(buf);
		coap_add_data_blocked_response(resource, session, request, response,
			token,COAP_MEDIATYPE_TEXT_PLAIN, 1, len, buf);
	}
}


static coap_context_t *get_context(const char *node, const char *port)
{
  coap_context_t *ctx = NULL;
  int s;
  struct addrinfo hints;
  struct addrinfo *result, *rp;

  ctx = coap_new_context(NULL);
  if (!ctx) {
    return NULL;
  }
  /* Need PSK set up before we set up (D)TLS endpoints */
  //fill_keystore(ctx);

  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
  hints.ai_socktype = SOCK_DGRAM; /* Coap uses UDP */
  hints.ai_flags = AI_PASSIVE | AI_NUMERICHOST;

  s = getaddrinfo(node, port, &hints, &result);
  if ( s != 0 ) {
    LOG("getaddrinfo: %s\n", gai_strerror(s));
    coap_free_context(ctx);
    return NULL;
  }

  /* iterate through results until success */
  for (rp = result; rp != NULL; rp = rp->ai_next) {
    coap_address_t addr, addrs;
    coap_endpoint_t *ep_udp = NULL, *ep_dtls = NULL, *ep_tcp = NULL, *ep_tls = NULL;

    if (rp->ai_addrlen <= sizeof(addr.addr)) {
      coap_address_init(&addr);
      addr.size = rp->ai_addrlen;
      memcpy(&addr.addr, rp->ai_addr, rp->ai_addrlen);
      addrs = addr;
      if (addr.addr.sa.sa_family == AF_INET) {
        uint16_t temp = ntohs(addr.addr.sin.sin_port) + 1;
        addrs.addr.sin.sin_port = htons(temp);
      } else if (addr.addr.sa.sa_family == AF_INET6) {
        uint16_t temp = ntohs(addr.addr.sin6.sin6_port) + 1;
        addrs.addr.sin6.sin6_port = htons(temp);
      } else {
        goto finish;
      }

      ep_udp = coap_new_endpoint(ctx, &addr, COAP_PROTO_UDP);
      if (ep_udp) {
        if (coap_dtls_is_supported() && (key_defined || cert_file)) {
          ep_dtls = coap_new_endpoint(ctx, &addrs, COAP_PROTO_DTLS);
          if (!ep_dtls)
            LOG("cannot create DTLS endpoint\n");
        }
      } else {
        LOG("cannot create UDP endpoint\n");
        continue;
      }
      ep_tcp = coap_new_endpoint(ctx, &addr, COAP_PROTO_TCP);
      if (ep_tcp) {
        if (coap_tls_is_supported() && (key_defined || cert_file)) {
          ep_tls = coap_new_endpoint(ctx, &addrs, COAP_PROTO_TLS);
          if (!ep_tls)
            LOG("cannot create TLS endpoint\n");
        }
      } else {
        LOG("cannot create TCP endpoint\n");
      }
      if (ep_udp)
        goto finish;
    }
  }

  LOG("no context available for interface '%s'\n", node);

finish:
  freeaddrinfo(result);
  return ctx;
}

static int coap_listen(char *addr_str, char *port_str, struct andlink *andlink)
{
	coap_context_t  *ctx;
	coap_tick_t now;
	unsigned wait_ms;
	coap_resource_t *r;
	time_t t_last = 0;
	coap_startup();
	ctx = get_context(addr_str, port_str);
	if (!ctx) {
		LOG("%s get context faild\n", __func__);
		return -1;
	}
	coap_set_app_data(ctx, andlink);
	r = coap_resource_init(coap_make_str_const("qlink/netinfo"), COAP_RESOURCE_FLAGS_NOTIFY_CON);
	coap_register_handler(r, COAP_REQUEST_POST, coap_notify);
	coap_add_attr(r, coap_make_str_const("netinfo"), coap_make_str_const("\"net info\""), 0);
	coap_add_resource(ctx, r);
	wait_ms = COAP_RESOURCE_CHECK_TIME * 1000;
	while ( !andlink->coap_quit ) {
		int result = coap_run_once( ctx, wait_ms );
		if ( result < 0 ) {
			break;
		} else if ( result && (unsigned)result < wait_ms ) {
			/* decrement if there is a result wait time returned */
			wait_ms -= result;
		} else {
			/*
			 * result == 0, or result >= wait_ms
			 * (wait_ms could have decremented to a small value, below
			 * the granularity of the timer in coap_run_once() and hence
			 * result == 0)
			 */
			/*
			time_t t_now = time(NULL);
			if (t_last != t_now) {
				// Happens once per second
				int i;
				t_last = t_now;
				if (time_resource) {
					coap_resource_notify_observers(time_resource, NULL);
				}
				for (i = 0; i < dynamic_count; i++) {
					coap_resource_notify_observers(dynamic_entry[i].resource, NULL);
				}
			}
			*/
			if (result) {
				/* result must have been >= wait_ms, so reset wait_ms */
				wait_ms = COAP_RESOURCE_CHECK_TIME * 1000;
			}
		}
	}

	if (andlink->coap_quit)
		andlink->coap_quit = 0;
	return 0;
}
static int coap_get_net_info(struct andlink *andlink)
{
	coap_pdu_t *pdu = NULL;
	int ret = 0;
	char uri[128] = { 0 };
	char data[256] = { 0 };
	char bcast[40] = { 0 };
	//step 1. send a coap ping request to get gw quickly
	andlink->gw_detected = false;
	sprintf(uri, "coap://192.168.1.1:5683");
	andlink->coap_type = COAP_PING;
	ret = coap_send_request(andlink, uri, NULL);
	if ( ret == -1) {
		LOG("coap send PING request failed\n");
		return ret;
	}

	if (ret == -ETIMEDOUT) {
		LOG("coap send PING request timeout\n");
		if (get_boardcast_ip(&bcast)) {
			LOG("get boardcast ip failed\n");
			return -1;
		}

		memset(uri, 0, sizeof(uri));
		memset(data, 0 , sizeof(data));
		sprintf(uri, "coap://%s:5683/qlink/searchgw", bcast);
		LOG("%s search gw %s\n", __func__, uri);
		sprintf(data, "{\"searchKey\":\"ANDLINK-DEVICE\",\"andlinkVersion\":\"V3\"}");
		andlink->coap_type = COAP_SEARCH_GW;
		ret = coap_send_request(andlink, uri, data);
		if (ret == -1) {
			LOG("coap send SEARCHGW request failed\n");
			return -1;
		}

		if (ret == -ETIMEDOUT) {
			LOG("coap send SEARCHGW request timeout\n");
			andlink->gw_detected = true;
			//return -1;
		}

	}

	//coap ping recieved response or search gw recieved response, we can know gwway ip
	if (andlink->gw_detected) {
		char ip[40] = { 0 };
		memset(uri, 0, sizeof(uri));
		memset(data, 0 , sizeof(data));
		coap_print_addr(&andlink->gw_addr, ip, sizeof(ip));
		andlink->coap_type = COAP_NET_INFO;
		sprintf(uri, "coap://%s/qlink/request", ip);
		LOG("%s qlink reqeust uri %s\n", __func__, uri);
		sprintf(data, "{\"deviceMac\":\"10:16:88:A4:FE:FC\",\"deviceType\":\"500301\"}");
		ret = coap_send_request(andlink, uri, data);
		//now, we have get the net info, we should change wifi connect state
		if ( ret == 0) {
			LOG("send qlink request successfully\n");
			//listen, except to get the net_info
			memset(ip, 0 , sizeof(ip));
			if (get_netwrok_ip(&ip) != 0)
				strcpy(ip, "127.0.0.1");
			LOG("%s listen to (%s:%s)\n", __func__, ip, DEFAULT_PORT);
			coap_listen(ip, DEFAULT_PORT, andlink);
		} else if ( ret == -ETIMEDOUT) {
			LOG("coap send NETINFO request timeout\n");
		} else {
			LOG("coap send NETINFO request failed\n");
		}
	}
	return ret;
}

static void coap_request_notify(void *para)
{
	struct andlink *andlink = (struct andlink *)para;
	if (andlink->coap_request)
		andlink->coap_quit = 1;
	andlink->coap_request = true;
}

void coap_process(struct andlink *andlink)
{
	andlink->wifi_connect_callback = coap_request_notify;
	while (1) {
		if (andlink->exit)
			break;
		if (andlink->coap_request) {
			coap_get_net_info(andlink);
			andlink->coap_request = false;
		} else {
			usleep(50 * 1000);
		}
	}
	LOG("%s exit\n");
	return ;
}
