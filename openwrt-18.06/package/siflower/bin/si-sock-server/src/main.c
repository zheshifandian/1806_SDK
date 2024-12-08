/*
 * =====================================================================================
 *
 *       Filename:  main.c
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  07/20/2015 10:38:43 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  franklin , franklin.wang@siflower.com.cn
 *        Company:  Siflower Communication Tenology Co.,Ltd
TODO:
1)SYNC STATUS CMD/INIT CMD
2)clean up all the resource if we accept stop cmd
3)when we died , restart ourself
4)safe read the client's socket content empty:

content-length : total_size \r\n
CMD args -data .....

getlen(data, &len);
while(readsize < len)
read(socket, buffer, size);

 *
 * =====================================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <unistd.h>
#include <getopt.h>
#include <curl/curl.h>
#include <uci.h>
#include <regex.h>
#include <signal.h>
#include <linux/rtnetlink.h>
#include <openssl/rsa.h>
#include <openssl/aes.h>
#include <openssl/pem.h>

#include "utils.h"
#include "bind.h"
#include "changes.h"
#include "status_sync.h"
#include "http.h"
#include "local_http.h"
#include "local_storage.h"
#include "thread_ssl.h"
#include "queue.h"
#include "ssst_request.h"
#include "mtd.h"
#include "token.h"
#include "cJSON.h"
#include "sf_factory_read.h"
#include "ker_health.h"
#include "iwevent.h"
#include "publish.h"
#include "urllist.h"

#define MSG_BUFFER 16384
#define MAX_CLIENT  10
#define BUFFER_SIZE 1024
#define SIGN_LEN    128

#define BIND_MSG_STRING                 "BIND"
#define UNBIND_MSG_STRING               "UBND"
#define UCI_CHANGE_MSG_STRING           "UCIM"
#define INIT_MSG_STRING                 "INIT"
#define MANAGER_OP_MSG_STRING           "MGOP"
#define SYSTEM_EVENT_MSG_STRING         "RSCH"
#define SYNC_STATUS_STRING              "SYNC"
#define SUB_EVENT_STRING                "SUBE"
#define USER_INFO_STRING                "INFO"
//#ifdef P2P
//#define CP2P_STRING						"CP2P"
//#define DP2P_STRING						"DP2P"
//#endif
#define TOKEN_STRING					"TOKN"
#ifdef URLLIST
#define URLLIST_STRING    "URLL"
#endif
#define ADD_ROUTER_USER_MSG_STRING                 "ADDU"
/*BIND_CMD, when application's want to bind our router, we will receive this message,
  the message content will be "BIND router object"*/
#define BIND_CMD 0
/*UCI_CHANGE_CMD, when uci config has been changed, the message content will be like :
  UCIM{["wireless"] = {["mt7620"] = {["bw"] = "0",["ht_extcha"] = "",},},}*/
#define UCI_CHANGE_CMD 1
/*INIT_CMD,XXX TODO*/
#define INIT_CMD 2
/*MANAGER_OP_CMD */
#define MANAGER_OP_CMD 3
/*SYNC_STATUS_CMD sync the router's status(speed of downloading,uploading, cpuloading and so on)*/
#define SYNC_STATUS_CMD 4
/*UNBIND_CMD */
#define UNBIND_CMD  5
/*HANDLE SYSTEM EVENT*/
#define SYSTEM_EVENT_CMD 6
/*define user subscribe event*/
#define SUB_EVENT_CMD 7
/*define save user info cmd*/
#define USER_INFO_CMD 8
/*get token*/
#define TOKEN_CMD	11
#ifdef URLLIST
#define URL_CMD	12
#endif
#define ADD_ROUTER_USER_CMD	13

//seconds, should use a config to load this value
//config-> args->main
#define LOOP_STATUS_SYNC_INTERVAL  10
#define KER_HEALTH 1
#ifdef USE_CUSTOM_SOCKET
#define WIFI_CUSTOM_LISTEN_PORT 7892
#endif

extern char slversion[64];
char ssversion[16];
char tmp_pkt[BUFFER_SIZE];
int  tmp_pkt_len;
char tmp_pkt_offset;

int routerDataSync();

static int32_t g_binded = false;
pthread_mutex_t g_queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t g_queue_cond = PTHREAD_COND_INITIALIZER;
my_msg_list *g_msg = NULL;

int find_overlay_mount(char *overlay)
{
	FILE *fp = fopen("/proc/mounts", "r");
	static char line[256];
	int ret = -1;

	if(!fp)
	  return ret;

	while (ret && fgets(line, sizeof(line), fp))
	  if (!strncmp(line, overlay, strlen(overlay)))
		ret = 0;

	fclose(fp);

	return ret;
}

void getlocalVersion(char *version){
	struct uci_context *ctx = uci_alloc_context();
	struct uci_package *p = NULL;
	uci_set_confdir(ctx, "/etc/config");
	if(uci_load(ctx, "siversion", &p) == UCI_OK){
		struct uci_section *cloudcode = uci_lookup_section(ctx, p, "version");
		strcpy(version,uci_lookup_option_string(ctx, cloudcode, "api"));
	}
	uci_unload(ctx,p);
	uci_free_context(ctx);
}

void getserverVersion(char *version){
	struct uci_context *ctx = uci_alloc_context();
	struct uci_package *p = NULL;
	uci_set_confdir(ctx, "/etc/config");
	if(uci_load(ctx, "sicloud", &p) == UCI_OK){
		struct uci_section *cloudcode = uci_lookup_section(ctx, p, "addr");
		strcpy(version,uci_lookup_option_string(ctx, cloudcode, "version"));
	}
	uci_unload(ctx,p);
	uci_free_context(ctx);
}

int getBindId(char *id){
	int ret = -1;
	struct uci_context *ctx = uci_alloc_context();
	struct uci_package *p = NULL;
	uci_set_confdir(ctx, "/etc/config");
	if(uci_load(ctx, "siserver", &p) == UCI_OK){
		struct uci_section *cloudrouter = uci_lookup_section(ctx, p, "cloudrouter");
		if(cloudrouter != NULL){
			const char *value = uci_lookup_option_string(ctx, cloudrouter, "binder");
			if(value != NULL){
				ret = 0;
				strcpy(id,value);
			}
		}
	}
	uci_unload(ctx,p);
	uci_free_context(ctx);
	return ret;
}

int strsub(char *str, char start, char end)
{
	*(str+end) = '\0';
	strcpy(str, str+start);
	return 0;
}

int regex_match(char *str, const char *pattern)
{

	regmatch_t pm[1];
	regex_t preg;
	if(regcomp(&preg, pattern, REG_EXTENDED | REG_NEWLINE) != 0)
	{
		fprintf(stderr, "Cannot regex compile!");
		return -1;
	}

	regexec(&preg, str, 1, pm, REG_NOTEOL);

	strsub(str, pm[0].rm_so, pm[0].rm_eo);

	regfree(&preg);
	return 0;
}

void dumpmsg(my_message *msg)
{
#ifdef USE_FILE_DUMP_MSG
	FILE *file = fopen("syscservice_dump_message.txx", "a+b");
	if(!file){
		LOG("[server]can not open the syscservice_dump_message.txx\n");
		return;
	}
	WRITE_MSG_HEAD(msg);
	char tmp[32];
	memset(tmp, 0, 32);
	sprintf(tmp,"\n\n-----------------------\n");
	fwrite(tmp, strlen(tmp), 1, file);
	memset(tmp, 0, 32);
	sprintf(tmp, "msg->cmd = %d\n", msg->cmd);
	fwrite(tmp, strlen(tmp), 1, file);
	memset(tmp, 0, 32);
	if(msg->data){
		fwrite(msg->data, 1, strlen((char *)msg->data), file);
	}
	fclose(file);
#else
	if(!msg){
		LOG("[server]msg is null");
		return;
	}
	LOG( "[server]msg->cmd = %d\n", msg->cmd);
	LOG( "[server]msg->data = %s\n", msg->data ? (char *)msg->data: "NULL");
#endif
}


int32_t InitPeriodEventQueue()
{
	g_periodEventQueue = createMsgQueue();
	my_assert(g_periodEventQueue);
	return 0;
}

int32_t DestroyPeriodEventQueue()
{
	return destoryMsgQueue(g_periodEventQueue);
}

void init_env()
{
	//use uci config file to get the binding state
	g_binded = true;
	g_msg = (my_msg_list *)malloc(sizeof(my_msg_list));
	my_assert(g_msg);
	memset((void *)g_msg, 0, sizeof(my_msg_list));

	InitChangesQueue();
	InitPeriodEventQueue();
}

void checkServerConnectedState()
{
	//check if subservice exist
	int32_t isSubsericeBooted;
	int32_t serverConnected;
	int8_t i=0;
	//wait for subservice up
	while(1){
		isSubsericeBooted = detect_process("subcloud");
		if(isSubsericeBooted != 0){
			LOG( "[server]--wait subservice boot up--ret=%d\n",isSubsericeBooted);
			i++;
			if(i > 3)
			  return;
			sleep(1);
		}else{
			break;
		}
	}
	serverConnected = getServerConnectedState();
	if(serverConnected == 1){
		LOG( "[server]--remote server is connected now\n");
		on_system_event("{\"action\": 0}",NULL);
	}
}
static void *data_sync_thread(void *args)
{
	int delay = 5;
	/*write hardware info to uci*/
	while(routerDataSync() ==  -1 ){
		LOG( "data sync to server failed!\n");
		sleep(delay);
		if(delay < 300){
			delay += 5;
		}
	}
	return NULL;
}

static void *ker_health_process(void *args)
{
    int ret = 0, mode;
    mode = getKerHealthMode();
    if (mode == 0){
        return NULL;
    }
    else if(mode == 1){
        while(1){
            ret = sys_sta_process(NULL);
            if(ret == -1){
                LOG("ker_health Safety\n");
            }
            ret = dailyHeart();
            if(ret != 0){
                LOG("daily heart beat send fail\n");
            }
            dailyData();
        }
    }
    else{
        LOG("get ker health mode fail!\n");
    }
    LOG("ker health process error ret is%d\n",ret);
    return NULL;
}

static void *socket_server(void *args)
{
	int32_t lsn_fd, apt_fd;
	struct sockaddr_un srv_addr;
	struct sockaddr_un clt_addr;
	socklen_t clt_len;
	int32_t ret;
	int32_t yes = 1;

	//create socket to bind local IP and PORT
	lsn_fd = socket(PF_UNIX, SOCK_STREAM, 0);
	if(lsn_fd < 0)
	{
		LOG("[server]can't create communication socket!\n");
		system("/etc/init.d/syncservice restart");
		return NULL;
	}
	if (setsockopt(lsn_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes))) {
		close(lsn_fd);
		system("/etc/init.d/syncservice restart");
		return NULL;
	}

	//create local IP and PORT
	srv_addr.sun_family = AF_UNIX;
	strncpy(srv_addr.sun_path, UNIX_DOMAIN, sizeof(srv_addr.sun_path) - 1);
	unlink(UNIX_DOMAIN);

	//bind sockfd and sockaddr
	ret = bind(lsn_fd, (struct sockaddr*)&srv_addr, sizeof(srv_addr));
	if(ret == -1)
	{
		LOG("[server]can't bind local sockaddr!\n");
		goto SOCKET_ERR;
	}

	//listen lsn_fd
	ret = listen(lsn_fd, MAX_CLIENT);
	if(ret == -1)
	{
		LOG("[server]can't listen client connect request\n");
		goto SOCKET_ERR;
	}

	char lastCmd[1024] = "";
	clt_len = sizeof(clt_addr);

	while(1)
	{
		apt_fd = accept(lsn_fd, (struct sockaddr*)&clt_addr, &clt_len);
		if(apt_fd < 0)
		{
			LOG("[server]can't listen client connect request\n");
			goto SOCKET_ERR;
		}
		LOG("[server]received a connection\n");
		//read and syslog client info

		void *recv_buf = NULL;
		int32_t rcv_num = 0;
		//4K is big enough
		recv_buf = malloc(MSG_BUFFER);
		my_assert(recv_buf);
		memset(recv_buf, 0, MSG_BUFFER);
		//TODO, how do we know the stream is end??????????
		rcv_num = read(apt_fd, recv_buf, MSG_BUFFER);
		LOG( "[server][%d]bytes from client:%s\n", rcv_num, (char *)recv_buf);
		memset(lastCmd, 0, sizeof(lastCmd));
		if(rcv_num<1024){
			memcpy(lastCmd, recv_buf, rcv_num);
		}else{
			memcpy(lastCmd, recv_buf, 1024);
		}
		//push the message to the message queue
		my_message *msg = (my_message *)malloc(sizeof(my_message));
		my_assert(msg);
		memset((void *)msg, 0, sizeof(my_message));
		msg->data = (char *)recv_buf;
		if(strstr(recv_buf, "need-callback")){
			//need send the result to client, so save the client socket fd
			msg->client = apt_fd;
		}else{
			close(apt_fd);
		}
		char *tmp = NULL;
		if((tmp = strstr(recv_buf,"-data "))){
			msg->args = tmp + 6;
			//user maybe want to joker me!!
			if(!strlen(msg->args))
				msg->args = NULL;
		}
		if(!strncmp(recv_buf, BIND_MSG_STRING, 4)){
			msg->cmd = BIND_CMD;
		}else if(!strncmp(recv_buf, ADD_ROUTER_USER_MSG_STRING, 4)){
			msg->cmd = ADD_ROUTER_USER_CMD;
		}else if(!strncmp(recv_buf, UCI_CHANGE_MSG_STRING, 4)){
			msg->cmd = UCI_CHANGE_CMD;
		}else if(!strncmp(recv_buf, INIT_MSG_STRING, 4)){
			msg->cmd = INIT_CMD;
		}else if(!strncmp(recv_buf, MANAGER_OP_MSG_STRING, 4)){
			msg->cmd = MANAGER_OP_CMD;
		}else if(!strncmp(recv_buf, UNBIND_MSG_STRING, 4)){
			msg->cmd = UNBIND_CMD;
		}else if(!strncmp(recv_buf, SYSTEM_EVENT_MSG_STRING, 4)){
			msg->cmd = SYSTEM_EVENT_CMD;
		}else if(!strncmp(recv_buf, SYNC_STATUS_STRING, 4)){
			msg->cmd = SYNC_STATUS_CMD;
		}else if(!strncmp(recv_buf, SUB_EVENT_STRING, 4)){
			msg->cmd = SUB_EVENT_CMD;
		}else if(!strncmp(recv_buf, USER_INFO_STRING, 4)){
			msg->cmd = USER_INFO_CMD;
#ifdef URLLIST
		}else if(!strncmp(recv_buf, URLLIST_STRING, 4)){
			msg->cmd = URL_CMD;
#endif
		}else if(!strncmp(recv_buf, TOKEN_STRING, 4)){
			msg->cmd = TOKEN_CMD;
		}else{
			free(msg);
			LOG( "[server]unknown message type %s", (char *)recv_buf);
			continue;
		}
		pthread_mutex_lock(&g_queue_mutex);
		push(g_msg,msg);
		pthread_cond_signal(&g_queue_cond);
		pthread_mutex_unlock(&g_queue_mutex);
	}
SOCKET_ERR:
	close(lsn_fd);
	unlink(UNIX_DOMAIN);
	system("/etc/init.d/syncservice restart");
	return NULL;

}

void process_message()
{
	pthread_mutex_lock(&g_queue_mutex);
	if(g_msg->size <= 0){
		LOG( "[server][function:%s]g_msg empty, sleep, waiting my prince", __func__);
		pthread_cond_wait(&g_queue_cond, &g_queue_mutex);
		LOG("[server][function:%s]wake up to process message\n", __func__);
	}else{
		LOG( "[server][function:%s]g_msg is non-empty , g_msg->size===%d==", __func__, g_msg->size);
	}
	my_message *msg = pop(g_msg);
	pthread_mutex_unlock(&g_queue_mutex);
	my_assert(msg);
	dumpmsg(msg);

	char *data = msg->args;
	char **callback = msg->client ? &msg->callback : NULL;

	switch(msg->cmd){
		case BIND_CMD:
			do_bind(data, callback);
			break;
		case ADD_ROUTER_USER_CMD:
			doAddRouterUser(data, callback);
			break;
		case UNBIND_CMD:
			do_unbind(data, callback);
			break;
		case UCI_CHANGE_CMD:
			do_uci_changes(data, callback);
			break;
		case INIT_CMD:
			break;
		case MANAGER_OP_CMD:
			do_manager_op(data,callback);
			break;
		case SYNC_STATUS_CMD:
			do_status_sync(data, callback);
			break;
		case SYSTEM_EVENT_CMD:
			on_system_event(data, callback);
			break;
		case SUB_EVENT_CMD:
			on_sub_event(data,callback);
			break;
		case USER_INFO_CMD:
			do_save_user_info(data,callback);
			break;
		case TOKEN_CMD:
			DoToken(data,callback);
			break;
#ifdef URLLIST
		case URL_CMD:
			do_urllist_change(data,callback);
			break;
#endif
		default:
			LOG("[server]unknown message type : %d\n", msg->cmd);
			break;
	}
	if(msg->data)
	  free(msg->data);

	if(msg->client && msg->callback){
		int32_t tmp_write = 0;
		LOG("[server] write message back to client, %s\n", msg->callback);
		while(tmp_write < strlen(msg->callback))
		  tmp_write += write(msg->client, msg->callback + tmp_write, strlen(msg->callback) - tmp_write);
	}

	if(msg->client)
	  close(msg->client);

	if(msg->callback)
	  free(msg->callback);

	free(msg);
}

int routerDataSync(){
	unsigned char SN[SN_LEN] = "";
	unsigned char mac[6] = "";
	char verify_code = 0;
	char bindid[32] = {'\0'};
	char managerid[ID_BUF_S] = {'\0'};
    char PRODUCT[PRODUCT_LEN] = "";
    char product_code[3] = "";
	char mac_str[18] = "";
	char sn_str[SN_STR_LEN] = {'\0'};
	char sn_str_new[SN_STR_LEN] = {'\0'};
	FILE *mfile, *rkey;
	int i,ret = 0;
	// get sn
	if(sf_factory_read_operation(READ_SN, (char *)SN, sizeof(SN)))
		mtd_operation(CMD_READ, (char *)SN, sizeof(SN), SN_OFFSET);
	LOG("sn is %02x %02x %02x %02x %02x %02x %02x %02x \n",SN[0],SN[1],SN[2],SN[3],SN[4],SN[5],SN[6],SN[7]);
	LOG("===== %02x %02x %02x %02x %02x %02x %02x %02x \n",SN[8],SN[9],SN[10],SN[11],SN[12],SN[13],SN[14],SN[15]);

	for(i = 0; i < SN_LEN; i++){
		sprintf(sn_str + i*2,"%02x",SN[i]);
	}
	LOG("sn str : %s\n",sn_str);
	// check sn
	if(sf_factory_read_operation(READ_SN_FLAG, &verify_code, 1))
		mtd_operation(CMD_READ, &verify_code, 1, SN_VERIFY_OFFSET);
	LOG("verify code : %02x\n",verify_code);
	if(verify_code == SN_VERIFY_VALUE){
		setSfHardwareConfig("sn",sn_str);
		// return 0;
	}
	// get mac
	get_router_macaddr((char *)mac);
	LOG("mac is %02x %02x %02x %02x %02x %02x\n",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
	sprintf(mac_str,"%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
	LOG("mac str = %s\n",mac_str);

	// get bind id
	if(getBindId(bindid) < 0){
		LOG("bind id is null \n");
	}else {
		LOG("bind id is %s\n", bindid);
	}
	// get manager list
	mfile = fopen(MANAGERFILE,"r");
	if (mfile){
		fgets(managerid, ID_BUF_S -1, mfile);
		fclose(mfile);
		LOG("open managerid file success\n");
	}

    //check product
    if(sf_factory_read_operation(READ_PRODUCT_KEY_FLAG, product_code, 3))
       mtd_operation(CMD_READ, product_code,3, PRODUCT_VERIFY_OFFSET);
    LOG("product verify code :%s",product_code);
    if(strcmp(product_code, PRODUCT_VERIFY_VALUE) != 0){
        if((rkey = fopen(ROUTER_KEY_PATH, "r"))){
            fgets(PRODUCT, 33, rkey);
			pclose(rkey);
        }else{
            LOG("open routerkey path fail\n");
        }
    }else{
        //get product key
        if(sf_factory_read_operation(READ_PRODUCT_KEY, PRODUCT, sizeof(PRODUCT)))
            mtd_operation(CMD_READ, PRODUCT, sizeof(PRODUCT), PRODUCT_OFFSET);

        LOG("product str : %s\n",PRODUCT);
    }
	ret = checkRouterDataFromServer(sn_str,mac_str,bindid, managerid,sn_str_new, PRODUCT);

	LOG("[ssst=========LOG======]checkRouterDataFromServer ret=%d", ret);
	DB_LOG("[ssst======DB_LOG=========]checkRouterDataFromServer ret=%d", ret);
	if(ret < 0){
		LOG("check sn from server fail %d", ret);
		return ret;
	}else if(ret == 1){
		char conv[3] = {'\0'};
		LOG("get %s\n",sn_str_new);
		for(i = 0; i < SN_LEN; i++){
			strncpy(conv,sn_str_new + i*2,2);
			SN[i]=strtoul(conv, NULL,16);
		}
		mtd_operation(CMD_WRITE, (char *)SN, sizeof(SN), SN_OFFSET);
		setSfHardwareConfig("sn",sn_str_new);
		verify_code= SN_VERIFY_VALUE;
		mtd_operation(CMD_WRITE, &verify_code, 1, SN_VERIFY_OFFSET);
		TokenInit();
	}else if(ret == 0){
		setSfHardwareConfig("sn",sn_str);
		verify_code= SN_VERIFY_VALUE;
		mtd_operation(CMD_WRITE, &verify_code, 1, SN_VERIFY_OFFSET);
	}
	return 0;
}

char XCLOUD_REMOTE_ADDR[64] = {0};
char XCLOUD_REMOTE_FUNCTION_ADDR[64] = {0};
char XCLOUD_REMOTE_DATA_ADDR[64] = {0};

int32_t main(int32_t argc, char *argv[])
{
	int32_t n = 0;
	int dotest = 0;
	char addr[41] = {'\0'};
	init_log_level();
	while (n >= 0) {

		n = getopt_long(argc, argv, "f:p:t", NULL, NULL);
		if (n < 0)
		  continue;
		switch (n) {
			case 't':
				dotest = 1;
				break;
		}
	}
	if(getServerAddress(addr) < 0){
		LOG("get server address fail");
		exit(0);
	}
	signal(SIGCHLD, SIG_IGN);

	getlocalVersion(slversion);
	getserverVersion(ssversion);
	sprintf(XCLOUD_REMOTE_ADDR,"https://%s/%s", addr,ssversion);
	sprintf(XCLOUD_REMOTE_FUNCTION_ADDR,"https://%s/%s/cloud", addr,ssversion);
	sprintf(XCLOUD_REMOTE_DATA_ADDR,"https://%s/%s/data/", addr,ssversion);
	LOG("function addr is %s\n", XCLOUD_REMOTE_FUNCTION_ADDR);
	LOG("remote addr is %s\n", XCLOUD_REMOTE_ADDR);
	LOG("data addr is %s\n", XCLOUD_REMOTE_DATA_ADDR);
	curl_global_init(CURL_GLOBAL_ALL);
	init_locks();

	while(find_overlay_mount("overlayfs:/tmp/root") == 0){
		LOG( "waiting for fs ok\n");
		sleep(1);
	}

	/*init all the init parameters or some global configs*/
	init_env();
	init_sync_context();
    remote_addr_init(XCLOUD_REMOTE_ADDR);
	TokenInit();
	GetToken();

	pthread_attr_t att;
	pthread_attr_init(&att);
	pthread_attr_setdetachstate(&att, PTHREAD_CREATE_DETACHED);
	
	pthread_t pt_data_sync;
	pthread_create(&pt_data_sync, &att, &data_sync_thread, NULL);
	my_assert(pt_data_sync > 0);
	if(pt_data_sync <= 0){
		LOG( "can not create sync thread !\n");
		// return -1;
	}

	//put it into sync thread
	checkServerConnectedState();
#ifdef KER_HEALTH
    pthread_t ker_health;
    pthread_create(&ker_health, &att, &ker_health_process, NULL);
    my_assert(ker_health > 0);
    if(ker_health <= 0){
        LOG("can not create ker_health!\n");
    }
#endif
	/*start socket server thread which use to watch the following event:
	1, uci configuration changes
	2, global events, like binding, init, reset...
	3, private clound server's command, like reset, shutdown, kick off users
	*/
	pthread_mutex_init(&g_queue_mutex,NULL);
	pthread_cond_init(&g_queue_cond,NULL);
	pthread_t socket_thread;
	pthread_create(&socket_thread, &att, &socket_server, NULL);
	my_assert(socket_thread > 0);
	if(socket_thread <= 0){
		LOG( "can not create socket_thread!\n");
		return -1;
	}

	check_dev_from_boot();
#ifdef URLLIST
	urllist_init();
#endif
	/*enter into the process loop*/
	while(1){
		process_message();
	}
	//never reach here
	//TODO release the resource allocated
	//cleanCookieCache();
	DestroyPeriodEventQueue();
	destroy_sync_context();
	//destroy ssl locks
	kill_locks();
	return 0;
}
