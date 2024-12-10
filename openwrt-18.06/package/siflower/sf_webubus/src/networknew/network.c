/*
 * rpcd - UBUS RPC server
 *
 *   Copyright (C) 2013 Jo-Philipp Wich <jow@openwrt.org>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#define _GNU_SOURCE /* crypt() */

#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/statvfs.h>
#include <dirent.h>
#include <arpa/inet.h>
#include <signal.h>
#include <glob.h>
#include <libubox/blobmsg_json.h>
#include <libubox/avl-cmp.h>
#include <libubus.h>
#include <uci.h>

#include <rpcd/plugin.h>

static const struct rpc_daemon_ops *ops;

static struct blob_buf buf;
static struct uci_context *cursor;
enum {
	RPC_D_DATA,
	__RPC_D_MAX
};

static const struct blobmsg_policy rpc_data_policy[__RPC_D_MAX] = {
	[RPC_D_DATA]   = { .name = "data",  .type = BLOBMSG_TYPE_STRING },
};

static int
rpc_web_getport(struct ubus_context *ctx, struct ubus_object *obj,
                   struct ubus_request_data *req, const char *method,
                   struct blob_attr *msg)
{
	int port_wan = 0;
	FILE * p_file = NULL;
	char wan_buf[8];
	char name_buf[8];
	char en[4] = "0";
	struct uci_package *p;
	struct uci_ptr ptr = {
		.package = "network",
		.section = "lan",
		.option  = "port_adaptive"
	};
	p_file = popen("uci get network.@switch_vlan[1].ports | awk -F ' ' '{print $1}' | tr -d '\n'", "r");
	if (p_file) {
		while (fgets(wan_buf, 8, p_file) != NULL) {}
		pclose(p_file);
		port_wan = atoi(wan_buf) + 1;
		if(port_wan < 1 || port_wan > 5){
			port_wan = 0;
		}
	}

	p_file = popen("cat etc/openwrt_release | grep DISTRIB_CODENAME | awk -F '-' '{print $2}'| sed 's/.$//' | tr -d '\n'", "r");
	if (p_file) {
		while (fgets(name_buf, 8, p_file) != NULL) {}
		pclose(p_file);
	}

	if (uci_load(cursor, ptr.package, &p) || !p)
		goto out;

	uci_lookup_ptr(cursor, &ptr, NULL, true);

	if (ptr.o && ptr.o->type == UCI_TYPE_STRING)
		strncpy(en, ptr.o->v.string, sizeof(en));

	uci_unload(cursor, p);

out:
	blob_buf_init(&buf, 0);
	blobmsg_add_u32(&buf, "port", port_wan);
	blobmsg_add_u32(&buf, "enable", atoi(en));
	blobmsg_add_string(&buf, "name", name_buf);
	ubus_send_reply(ctx, req, buf.head);
	return 0;
}

static int
rpc_web_getstatus(struct ubus_context *ctx, struct ubus_object *obj,
                   struct ubus_request_data *req, const char *method,
                   struct blob_attr *msg)
{
	FILE* p_file = NULL;
	char status_buf[8];
	int status = 0;

	p_file = popen("./bin/wanLinkStatus", "r");
	if(p_file){
		while (fgets(status_buf, 8, p_file) != NULL) {};
		pclose(p_file);
		status = atoi(status_buf);
		if(status != 1)
			status = 0;
	}

	blob_buf_init(&buf, 0);
	blobmsg_add_u32(&buf, "status", status);
	ubus_send_reply(ctx, req, buf.head);
	return 0;
}

static int
rpc_web_port_adaptive(struct ubus_context *ctx, struct ubus_object *obj,
                   struct ubus_request_data *req, const char *method,
                   struct blob_attr *msg)
{
	const char *cmd[2] = {"./bin/auto_adapt.sh set ",NULL};
	char setcmd[128];
	struct blob_attr *tb[__RPC_D_MAX];

	blobmsg_parse(rpc_data_policy, __RPC_D_MAX, tb,
	              blob_data(msg), blob_len(msg));

	if (!tb[RPC_D_DATA] || blobmsg_data_len(tb[RPC_D_DATA]) >= 128)
		return UBUS_STATUS_INVALID_ARGUMENT;

	cmd[1] = blobmsg_data(tb[RPC_D_DATA]);
	sprintf(setcmd,"%s%s",cmd[0],cmd[1]);
	system(setcmd);
	return 0;
}

static int
rpc_web_set_boot(struct ubus_context *ctx, struct ubus_object *obj,
                   struct ubus_request_data *req, const char *method,
                   struct blob_attr *msg)
{
	const char *cmd[2] = {"uci -q set network.lan.port_adaptive=",NULL};
	char setcmd[128];
	struct blob_attr *tb[__RPC_D_MAX];

	blobmsg_parse(rpc_data_policy, __RPC_D_MAX, tb,
	              blob_data(msg), blob_len(msg));

	if (!tb[RPC_D_DATA] || blobmsg_data_len(tb[RPC_D_DATA]) >= 128)
		return UBUS_STATUS_INVALID_ARGUMENT;

	cmd[1] = blobmsg_data(tb[RPC_D_DATA]);
	sprintf(setcmd,"%s%s",cmd[0],cmd[1]);
	system(setcmd);
	system("uci commit");
	return 0;
}
static int
rpc_web_start_adaptive(struct ubus_context *ctx, struct ubus_object *obj,
                   struct ubus_request_data *req, const char *method,
                   struct blob_attr *msg)
{
	system("./bin/auto_adapt.sh start");
	return 0;
}

static int
rpc_web_api_init(const struct rpc_daemon_ops *o, struct ubus_context *ctx)
{
	int rv = 0;

	static const struct ubus_method web_network_methods[] = {
		UBUS_METHOD_NOARG("getport",         rpc_web_getport),
		UBUS_METHOD_NOARG("getstatus",         rpc_web_getstatus),
		UBUS_METHOD("setport",               rpc_web_port_adaptive, rpc_data_policy),
		UBUS_METHOD("setboot",               rpc_web_set_boot, rpc_data_policy),
		UBUS_METHOD_NOARG("start",           rpc_web_start_adaptive)
	};

	static struct ubus_object_type web_network_type =
		UBUS_OBJECT_TYPE("siflower-rpc-web-network", web_network_methods);

	static struct ubus_object network_obj = {
		.name = "web.network",
		.type = &web_network_type,
		.methods = web_network_methods,
		.n_methods = ARRAY_SIZE(web_network_methods),
	};

	cursor = uci_alloc_context();

	if (!cursor)
		return UBUS_STATUS_UNKNOWN_ERROR;

	ops = o;

	rv |= ubus_add_object(ctx, &network_obj);

	return rv;
}

struct rpc_plugin rpc_plugin = {
	.init = rpc_web_api_init
};
