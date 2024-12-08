/*
 * =====================================================================================
 *
 *      Filename:  socket_server.c
 *
 *   Description:  local socket server, listen for autotest
 *       Version:  1.0
 *       Created:  01/09/2018
 *      Compiler:  gcc
 *
 *        Author:  Qin.Xia , qin.xia@siflower.com.cn
 *       Company:  Siflower Communication Tenology Co.,Ltd
 *       TODO:
 *       1)
 *
 *
 * ======================================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/time.h>
#include <fcntl.h>
#include <signal.h>


#define PORT                            1234
#define MAX_CLIENT                      20
#define MSG_BUFFER                      1024
#define TIMEOUT                         20
#define FIFO_NAME                       "/tmp/send_result"

int lsnfd = 0;
int confd[MAX_CLIENT] = {0};

void sighandler(int sig)
{
	int i = 0;

	printf("handle signal!\n");
	if(lsnfd)
	  close(lsnfd);
	for(;i < MAX_CLIENT; i++){
		if(confd[i])
		  close(confd[i]);
	}
}

void *msg_handler(void *fd)
{
	int sockfd = *(int *)fd;
	int fifo_fd;
	int len = 0, timer = 0, flag = 0;
	int i = 0, index = 0, arg_index = 0;
	char str[256] = {0};
	char args[4][16] = {'\0'};
	char sendbuf[MSG_BUFFER] = {0};
	char recvbuf[MSG_BUFFER] = {0};

MSG_POLL:
	//receive message has timeout here, 1K message size is big enough
	len = recv(sockfd, (void *)recvbuf, MSG_BUFFER, 0);
	if (len <= 0){
		printf("[server] receive message error!\n");
		goto ERROR;
	}
	printf("[server] receive message: %s, %d bytes\n",recvbuf, len);

	//diff send_cmd and send_cmd_keep_open, if we use send_cmd_keep_open, we will poll message again.
	if (!strncmp(recvbuf, "send_cmd_keep_open", 18)){
		flag = 1;
		strcpy(recvbuf,recvbuf+19);
	}else
	  flag = 0;

	//handle received message
	if (!strncmp(recvbuf, "active", 6)){
		sprintf(sendbuf, "live\n");
	}
	else if (!strncmp(recvbuf, "get_log", 7)){
		//parse local path and remote path
		for (i = 0; i < len; i++){
			if (recvbuf[i] == ' '){
				memcpy(args[arg_index], recvbuf + index, i - index);
				index = i + 1;
				arg_index++;
			}
		}
		memcpy(args[arg_index], recvbuf + index, i - index);
		sprintf(str, "scp -i /tmp/router/id_rsa %s test@192.168.1.18:~/log/%s", args[1], args[2]);
		sprintf(sendbuf, "get_log success\n");
	}
	else if (!strncmp(recvbuf, "restart", 7)){
		sprintf(str, "killall autotest && /tmp/router/autotest &");
		sprintf(sendbuf, "restart success\n");
	}
	else{
		//create fifo to receive autotest result. If fifo exist, return EEXIST
		if ((mkfifo(FIFO_NAME, O_CREAT|O_EXCL) < 0) && (errno != EEXIST)){
			printf("[server] create fifo error!\n");
			goto ERROR;
		}

		//open fifo for read and set fifo none blcoking.
		fifo_fd = open(FIFO_NAME, O_RDONLY| O_NONBLOCK, 0);
		if (fifo_fd < 0){
			printf("[server] open fifo error!\n");
			goto ERROR;
		}

		//execute test shell
		sprintf(str, "sh /tmp/router/adapter.sh /tmp/router/%s", recvbuf);
		system(str);

		//set timeout for receive result
		while (read(fifo_fd, sendbuf, MSG_BUFFER) <= 0 && timer < TIMEOUT){
			sleep(1);
			timer += 1;
		}

		if (timer == TIMEOUT)
		  sprintf(sendbuf, "receive result timeout\n");

		//send test result to remote server
		if (send(sockfd, sendbuf, strlen(sendbuf), 0) < 0)
		  printf("[server] send fail\n");

		if (!flag){
			close(sockfd);
			return NULL;
		}else{
			memset(sendbuf, 0, MSG_BUFFER);
			memset(recvbuf, 0, MSG_BUFFER);
			goto MSG_POLL;
		}
	}

	system(str);
	send(sockfd, sendbuf, strlen(sendbuf), 0);
	if (!flag){
		close(sockfd);
		return NULL;
	}else{
		memset(sendbuf, 0, MSG_BUFFER);
		memset(recvbuf, 0, MSG_BUFFER);
		goto MSG_POLL;
	}

ERROR:
	close(sockfd);
	return NULL;
}


int main(int argc, char *argv[])
{
	struct sockaddr_in srv_addr, clt_addr;
	struct timeval timeout = {TIMEOUT, 0};
	pthread_t tid;
	pthread_attr_t att;
	socklen_t sock_len;
	int val = 1, i = 0, count = 0, ret = -1;

	signal(SIGINT, sighandler);

	lsnfd = socket(AF_INET, SOCK_STREAM, 0);
	if (lsnfd < 0)
	{
		printf("[server] create socket error!\n");
		return -1;
	}

	if (setsockopt(lsnfd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val))) {
		printf("[server] setsockopt error!\n");
		goto SOCKET_ERR;
	}

	bzero(&srv_addr, sizeof(srv_addr));
	srv_addr.sin_family = AF_INET;
	srv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	srv_addr.sin_port = htons(PORT);

	if (bind(lsnfd, (struct sockaddr*)&srv_addr, sizeof(srv_addr)) < 0)
	{
		printf("[server] bind error!\n");
		goto SOCKET_ERR;
	}

	if (listen(lsnfd, MAX_CLIENT) < 0)
	{
		printf("[server] listen error!\n");
		goto SOCKET_ERR;
	}
	printf("[server] listen OK!\n");

	//set thread detach so when it's down, would free resource by system
	pthread_attr_init(&att);
	pthread_attr_setdetachstate(&att, PTHREAD_CREATE_DETACHED);

	sock_len = sizeof(clt_addr);
	while(1)
	{
		//socket will blocked here waiting for connect requet
		confd[i] = accept(lsnfd, (struct sockaddr *)&clt_addr, &sock_len);
		if (confd[i] < 0)
		{
			printf("[server] accept error!\n");
			continue;
		}
		printf("[server] received a connection, IP:%s port:%d!\n",inet_ntoa(clt_addr.sin_addr), ntohs(clt_addr.sin_port));

		//set timeout for receive data
		setsockopt(confd[i], SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(struct timeval));

		//create thread to handle received message
		ret = pthread_create(&tid, &att, msg_handler, &confd[i]);
		if (ret != 0){
			printf("[server] pthread create error!-%s, count =%d\n", strerror(ret), count);
			break;
		}

		count++;
		i += 1;
		//only allow MAX_CLIENT thread
		i %= MAX_CLIENT;
	}

SOCKET_ERR:
	close(lsnfd);

	return -1;
}
