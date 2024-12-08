#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <libubox/uloop.h>
#include <syslog.h>

#define PORT 55555
#define BACKLOG 5
#define MAX_DATA 1024
#define LOG(X,...) syslog(LOG_CRIT,X,##__VA_ARGS__)

extern struct uloop_fd silog_fd;

void silog_cb(struct uloop_fd *u, unsigned int events){
	struct sockaddr_in client_addr;
	char result_buff[1024*100];
	FILE *stream = NULL;
	char buf[1024];
	int new_fd;
	unsigned int sin_size;
	int len;

	sin_size=sizeof(struct sockaddr_in);
	new_fd=accept(u->fd,(struct sockaddr*)&client_addr,&sin_size);
	if(new_fd==-1){
		LOG("receive failed\n");
	} else{
		len = recv(new_fd,buf,MAX_DATA,0);
		if (len < 1){
			LOG("not data receive\n");
			close(new_fd);
			return;
		}
		buf[len] = '\0';
		printf("receive success %s\n", buf);
		memset(result_buff, 0, sizeof(result_buff));
		if ((stream = popen(buf, "r")) == NULL) {
			fprintf(stderr, "%s", strerror(errno));
			close(new_fd);
			return;
		}
		len = fread(result_buff,1, sizeof(result_buff),stream);
		if ( len > 0) {
			LOG("send data len %d\n",len);
			if(len > 0){
				send(new_fd,result_buff,len,0);
			}
		}
		pclose(stream);
		close(new_fd);
	}
}

int silogserver(void *p){
	int sockfd;
	struct sockaddr_in server_addr;

	sockfd=socket(AF_INET,SOCK_STREAM,0);
	if(sockfd==-1){
		LOG("socket failed:%d",errno);
		return -1;
	}
	server_addr.sin_family=AF_INET;
	server_addr.sin_port=htons(PORT);
	server_addr.sin_addr.s_addr=htonl(INADDR_ANY);
	bzero(&(server_addr.sin_zero),8);
	if(bind(sockfd,(struct sockaddr*)&server_addr,sizeof(struct sockaddr))<0){
		LOG("bind error\n");
		return -1;
	}
	listen(sockfd,BACKLOG);

	silog_fd.cb = silog_cb;
	silog_fd.fd = sockfd;
	silog_fd.registered =  false;
	silog_fd.flags = ULOOP_READ;

	return 0;
}
