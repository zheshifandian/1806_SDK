/*
 * Copyright (C) 2018-2021 Franklin <franklin.wang@siflower.com.cn>
 * sf_advance_wireless
 * this is for the advance feature for wireless devices, for example:
 * 1, record the wifi device
 * 2, support wifi 2bands use the same ssid and passwd, if one device(stations) support both band, then with this service, they will
 *    connects with our AP smartly(we will provide better connection quanlity to the device)
 * 3, provide management for the USB2BLE devices which will be used in smart audio box application(e.g. AIRouter project)
 *
 *
 * How to connect with us:
 * 1, this service will be acted as a UBUS server and expose interfaces to all client, which are:
 *    1)wifi_ap_connect
 *      when a wifi bss was set up, then it will notify us with the bandm
 *    2)
 *
 *
 */

#include <unistd.h>
#include <signal.h>

#include "libubus.h"
#include "wifi.h"

struct ubus_context *ubus_ctx;
unsigned int debug = 7;
int sta_rec_num = 20;
int probe_rssi_24g_r = -59, probe_rssi_5g_l = -68;
int deauth_rssi_24g_r = -50, deauth_rssi_5g_l = -73;
int check_deauth_time = 10;
int sta_record = 0;

static void server_main(void)
{
	if (wifi_ap_init(ubus_ctx, sta_record)) {
		DEBUG(LOG_ERR, "wifi_ap advance service init failed!\n");
		return;
	}

	uloop_run();

	wifi_ap_deinit_ctx(ubus_ctx);
}

int main(int argc, char **argv)
{
	const char *ubus_socket = NULL;
	int ch;

	while ((ch = getopt(argc, argv, "cs:r:l:R:L:d:n:p:t:")) != -1) {
		switch (ch) {
		case 's':
			ubus_socket = optarg;
			break;
		case 'r':
			probe_rssi_24g_r = atoi(optarg);
			DEBUG(LOG_WARNING, "set probe_rssi_24g_r = %d\n",
							probe_rssi_24g_r);
			break;
		case 'l':
			probe_rssi_5g_l = atoi(optarg);
			DEBUG(LOG_WARNING, "set probe_rssi_5g_l = %d\n",
							probe_rssi_5g_l);
			break;
		case 'R':
			deauth_rssi_24g_r = atoi(optarg);
			DEBUG(LOG_WARNING, "set deauth_rssi_24g_r = %d\n",
							deauth_rssi_24g_r);
			break;
		case 'L':
			deauth_rssi_5g_l = atoi(optarg);
			DEBUG(LOG_WARNING, "set deauth_rssi_5g_l = %d\n",
							deauth_rssi_5g_l);
			break;
		case 'd':
			DEBUG(LOG_DEBUG, "set debug = %d\n", atoi(optarg));
			debug = atoi(optarg);
			break;
		case 'n':
			DEBUG(LOG_DEBUG, "set max station records = %d\n", atoi(optarg));
			sta_rec_num = atoi(optarg);
			break;
		case 'p':
			DEBUG(LOG_DEBUG, "enable sta mac records = %d\n", atoi(optarg));
			sta_record = atoi(optarg);
			break;
		case 't':
			DEBUG(LOG_DEBUG, "set sta deauth per %ds = %d\n", atoi(optarg));
			check_deauth_time = atoi(optarg);
			/* support recording 30s at most */
			if (check_deauth_time > 30)
				check_deauth_time = 30;
			break;
		default:
			break;
		}
	}

	argc -= optind;
	argv += optind;

	uloop_init();
	signal(SIGPIPE, SIG_IGN);

	ubus_ctx = ubus_connect(ubus_socket);
	if (!ubus_ctx) {
		fprintf(stderr, "Failed to connect to ubus\n");
		return -1;
	}

	ubus_add_uloop(ubus_ctx);

	server_main();

	ubus_free(ubus_ctx);
	uloop_done();

	return 0;
}
