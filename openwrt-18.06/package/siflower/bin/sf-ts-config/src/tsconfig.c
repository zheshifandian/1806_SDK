#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <regex.h>
#include <unistd.h>
#include <linux/rtnetlink.h>
#include <linux/netlink.h>
#include <linux/genetlink.h>
#include <netinet/in.h>
#include <errno.h>
#include <getopt.h>
#include <syslog.h>
#include "cJSON.h"
#include <uci.h>
#include "tsconfig.h"

//#define DEBUG 1

#define SF_TS_TXT "/etc/sf-ts-cfg"
#define SF_TS_BINARY_PATH "/tmp/etc/config/"
#define SF_TS_BINARY "/tmp/etc/config/sf-ts-cfg.bin"
#define SF_TS_BINARY_TAR "/etc/config/sf-ts-cfg.bin.tar.gz"
#define SF_TS_JSON_FILE "/tmp/updatelist.bin"

#define SF_TS_UPDATE_CMD_UPDATE 1
#define SF_TS_UPDATE_CMD_REPLACE 2
#define SF_TS_UPDATE_CMD_NOACTION 3
//for internal use
#define SF_TS_UPDATE_CMD_DELETE 4


const char *g_json_file = SF_TS_JSON_FILE;

typedef struct rtnl_handle
{
    int         fd;
    struct sockaddr_nl  local;
    struct sockaddr_nl  peer;
    __u32           seq;
    __u32           dump;
	__u16	family_id;
} rtnl_handle;

#define MAX_MSG_SIZE			64*1024

typedef struct msgtemplate {
	struct nlmsghdr n;
	struct genlmsghdr g;
	char data[MAX_MSG_SIZE];
} msgtemplate_t;

#define SOL_NETLINK				270
#define SF_TS_CMD				1
#define IP_INFO					0
#define TYPE_GAME_KEY			htonl(0x7f000100)
#define TYPE_VIDEO_KEY			htonl(0x7f000200)
#define TYPE_SOCIAL_KEY			htonl(0x7f000300)
#define GENLMSG_DATA(glh)       ((void *)(NLMSG_DATA(glh) + GENL_HDRLEN))
#define NLA_DATA(na)            ((void *)((char *)(na) + NLA_HDRLEN))
#define NLA_NEXT(na)			((void *)((char *)(na) + NLA_ALIGN(na->nla_len)))
#define TOCHAR(n)				(n>0x9 ? n+0x37 : n+0x30)

#define ATTR_NEXT(attr) (struct nlattr *)(((char *)attr) + NLA_ALIGN(attr->nla_len))
#define NDA_RTA(r)			((struct rtattr*)(((char*)(r)) + NLMSG_ALIGN(sizeof(struct ndmsg))))
#define LOG(X,...) syslog(LOG_CRIT,X,##__VA_ARGS__)

int rtnl_open(struct rtnl_handle *rth, unsigned subscriptions, int type)
{
    int addr_len;

    memset(rth, 0, sizeof(rtnl_handle));

    rth->fd = socket(PF_NETLINK, SOCK_RAW, type);
    if (rth->fd < 0) {
        perror("Cannot open netlink socket");
        return -1;
    }

    memset(&rth->local, 0, sizeof(rth->local));
    rth->local.nl_family = AF_NETLINK;
    rth->local.nl_groups = subscriptions;

    if (bind(rth->fd, (struct sockaddr*)&rth->local, sizeof(rth->local)) < 0) {
        perror("Cannot bind netlink socket");
        return -1;
    }
    addr_len = sizeof(rth->local);
    if (getsockname(rth->fd, (struct sockaddr*)&rth->local,
                (socklen_t *) &addr_len) < 0) {
        perror("Cannot getsockname");
        return -1;
    }
    if (addr_len != sizeof(rth->local)) {
        fprintf(stderr, "Wrong address length %d\n", addr_len);
        return -1;
    }
    if (rth->local.nl_family != AF_NETLINK) {
        fprintf(stderr, "Wrong address family %d\n", rth->local.nl_family);
        return -1;
    }
    rth->seq = time(NULL);
    return 0;
}

void rtnl_close(struct rtnl_handle *rth)
{
    close(rth->fd);
}

int genl_send_msg(int fd, u_int16_t nlmsg_type, u_int32_t nlmsg_pid, u_int8_t genl_cmd, u_int8_t genl_version,
		u_int16_t nla_type, void *nla_data, int nla_len){
    struct nlattr *na;
	struct sockaddr_nl nladdr;
	int r, buflen;
	char *buf;
	msgtemplate_t msg;

	msg.n.nlmsg_len = NLMSG_LENGTH(GENL_HDRLEN);
	msg.n.nlmsg_type = nlmsg_type;
	msg.n.nlmsg_flags = NLM_F_REQUEST;
	msg.n.nlmsg_seq = 0;

	msg.n.nlmsg_pid = nlmsg_pid;
	msg.g.cmd = genl_cmd;
	msg.g.version = genl_version;
	na = (struct nlattr *) GENLMSG_DATA(&msg);
	na->nla_type = nla_type;
	na->nla_len = nla_len + 1 + NLA_HDRLEN;
	memcpy(NLA_DATA(na), nla_data, nla_len);
	msg.n.nlmsg_len += NLMSG_ALIGN(na->nla_len);

	buf = (char *) &msg;
	buflen = msg.n.nlmsg_len;
	memset(&nladdr, 0, sizeof(nladdr));
	nladdr.nl_family = AF_NETLINK;

	printf("genl_send_msg++\n");

	while ((r = sendto(fd, buf, buflen, 0, (struct sockaddr *) &nladdr, sizeof(nladdr))) < buflen) {
		printf("genl_send_msg++r=%d\n",r);
		if (r > 0) {
			buf += r;
			buflen -= r;
		}else if(errno != EAGAIN){
			return -1;
		}
	}
	LOG("Send genl message ok!\n");
	return 0;
}

int find_group(struct nlattr *nlattr, char *group){
	struct nlattr *gattr = (struct nlattr *)(((char *)nlattr) + NLA_HDRLEN);
	int nl_len = nlattr->nla_len - NLA_HDRLEN;
	while(1){
		if(strncmp(((char *)gattr)+3*NLA_HDRLEN+4, group, strlen(group))== 0){
			return *((int *)((char *)gattr + 2*NLA_HDRLEN));
		}
		nl_len -= NLA_ALIGN(gattr->nla_len);
		if( nl_len > 0){
			gattr = ATTR_NEXT(gattr);
		}else{
			LOG("find group fail\n");
			return -1;
		}
	}
}

int get_genl_group(struct rtnl_handle *rth, char *family, char *group){
	msgtemplate_t genlmsg;
	int ret;
	struct nlattr *nlattr;
	int rc_len;
	int nl_len;

	ret = genl_send_msg(rth->fd, GENL_ID_CTRL, 0, CTRL_CMD_GETFAMILY, 1, CTRL_ATTR_FAMILY_NAME,
		   	family, strlen(family)+1);
	if(ret){
		LOG("Send genl fail\n");
		goto fail;
	}
	rc_len = recv(rth->fd, &genlmsg, sizeof(genlmsg), 0);

	if(rc_len < 0){
		LOG("Receive error!\n");
		goto fail;
	}

	if(genlmsg.n.nlmsg_type == NLMSG_ERROR || !NLMSG_OK((&genlmsg.n), rc_len)){
		LOG("Genlmsg type is %d, rc_len is %d, msg len is %d\n", genlmsg.n.nlmsg_type, rc_len, genlmsg.n.nlmsg_len);
		goto fail;
	}

	if(genlmsg.n.nlmsg_type == GENL_ID_CTRL){
		LOG("nl type ok\n");
		if(genlmsg.g.cmd == CTRL_CMD_NEWFAMILY){
			nlattr = (struct nlattr *)GENLMSG_DATA(&genlmsg);
			nl_len = genlmsg.n.nlmsg_len - NLMSG_HDRLEN - NLA_HDRLEN;
			if(nlattr->nla_type == CTRL_ATTR_FAMILY_NAME){
				if(strncmp(((char *)nlattr)+NLA_HDRLEN, family, strlen(family)) == 0){
					nlattr = ATTR_NEXT(nlattr);
					rth->family_id = *((__u16 *)(NLA_DATA(nlattr)));
					LOG("return family is %d\n", rth->family_id);
				    while(1){
						nl_len -= NLA_ALIGN(nlattr->nla_len);
						if( nl_len <=0){
							LOG("Not find attr\n");
							goto fail;
						}
						nlattr = ATTR_NEXT(nlattr);
						if( nlattr->nla_type == CTRL_ATTR_MCAST_GROUPS){
							return find_group(nlattr, group);
						}else{
							continue;
						}
					}
				}
			}
		}
	}
fail:
	return -1;

}

static int sf_ts_genl(struct rtnl_handle *rth)
{

	int group;
	__u16 id;

	if(rtnl_open(rth, 0, NETLINK_GENERIC) < 0)
	{
		perror("Can't initialize rtnetlink socket");
		return -1;
	}
	group = get_genl_group(rth, "SF_TS_NL", "sf-ts");
	LOG("group number is %d\n", group);
	if (group < 0){
		goto err1;
	}

	rtnl_close(rth);
	id = rth->family_id;
	if (rtnl_open(rth, 1 << (group-1), NETLINK_GENERIC) < 0){
		perror("Can't initialize generic netlink\n");
		goto err1;
	}
	rth->family_id = id;
	return 0;
err1:
	rtnl_close(rth);

	return -1;
}

void uciSetValue(struct uci_context *ctx,struct uci_ptr *ptr,char *key,char *retBuffer)
{
	ptr->o      = NULL;
	ptr->option = key;
	ptr->value  = retBuffer;
	uci_set(ctx, ptr);
}

int32_t setTsValueToUci(char *province, char *updateAt, int action, int count)
{
	struct uci_context *ctx = uci_alloc_context();
	struct uci_package *p = NULL;
	int ret = -1;
	char buf[32];
	char action_buf[16];
	uci_set_confdir(ctx, "/etc/config");
	if(uci_load(ctx, "tsconfig", &p) == UCI_OK)
	{
		//LOG("setTsValueToUci-------------------%s %s----1\n",province,updateAt);
		struct uci_section *sec = uci_lookup_section(ctx, p, "tsrecord");
		if(sec != NULL){
		//	LOG("setTsValueToUci-------------------%s %s----2\n",province,updateAt);
			struct uci_ptr ptr = { .p = p, .s = sec};
			uciSetValue(ctx,&ptr,"province",province);
			uciSetValue(ctx,&ptr,"updateAt",updateAt);

			sprintf(buf,"%d",count);
			uciSetValue(ctx,&ptr,"count",buf);

			sprintf(action_buf,"%d",action);
			uciSetValue(ctx,&ptr,"action",action_buf);
			uci_save(ctx,p);
			uci_commit(ctx,&p,false);
			ret = 0;
		}

		uci_unload(ctx,p);
	}
	uci_free_context(ctx);
	return ret;
}


static int get_ip_info_from_text(u_int32_t *buf){
	int ret = 0;
	FILE *fg;
	char fbuf[128];
	u_int8_t ip[4];
	u_int32_t l = 0, key;
	int ipcnt = 0;

	fg = fopen(SF_TS_TXT, "r");
	while(fgets(fbuf, 128, fg) != NULL){
		if(strncmp(fbuf,"game",4) == 0){
			key = TYPE_GAME_KEY;
		}else if(strncmp(fbuf,"video",5) == 0){
			key = TYPE_VIDEO_KEY;
		}else if(strncmp(fbuf,"social",6) == 0){
			key = TYPE_SOCIAL_KEY;
		}else{
			printf("config param error\n");
			fclose(fg);
			return -1;
		}
		buf[l] = key;
		while(fgets(fbuf, 128, fg) != NULL){
			++l;
			if(strncmp(fbuf,"end",3) != 0){
				ret = sscanf(fbuf,"%hhu.%hhu.%hhu.%hhu",&ip[0], &ip[1], &ip[2], &ip[3]);
				ipcnt++;
				if(ret < 4){
					printf("ip format error\n");
					return -1;
				}
				buf[l] = *((u_int32_t *)ip);
			}else{
				break;
			}
		}
		buf[l] = key;
		++l;
		printf("++++++++l=%d\n",l);
	}
	printf("ip cnt1=%d\n",ipcnt);

	fclose(fg);
	return l;

}

static u_int32_t get_ts_type(uint8_t type)
{
	u_int32_t ret = 0;
	switch(type)
	{
		case IP_TAG_TYPE_GAME:
			ret = TYPE_GAME_KEY;
			break;
		case IP_TAG_TYPE_VIDEO:
			ret = TYPE_VIDEO_KEY;
			break;
		case IP_TAG_TYPE_SOCIAL:
			ret = TYPE_SOCIAL_KEY;
			break;
		default:
			printf("unknown type=%d\n",type);
			break;
	}

	return ret;
}

//check if extract
void sync_ts_tar_file(bool extract)
{
	char command[128];
	if(extract){
		sprintf(command,"mkdir -p %s",SF_TS_BINARY_PATH);
		system(command);
		sprintf(command,"tar -xzf %s -C /",SF_TS_BINARY_TAR);
	}else{
		sprintf(command,"tar -zcf %s %s",SF_TS_BINARY_TAR,SF_TS_BINARY);
	}
	system(command);
//	printf("%s\n",command);
}

#define MAX_IP_CNT (MAX_MSG_SIZE/4 - 100)

static int get_ip_info_from_binary(u_int32_t **inbuf)
{
	FILE *infile;
	uint8_t ip[4];
	int ret = -1;
	int file_size;
	u_int32_t size;
	u_int32_t l1 = 0;
	u_int32_t l2 = 0;
	u_int32_t l3 = 0;
	u_int32_t *buf_game = NULL;
	u_int32_t *buf_social = NULL;
	u_int32_t *buf_video = NULL;
	u_int32_t *buf = NULL;
	*inbuf = NULL;
	//decompression from tar.gz file
	sync_ts_tar_file(true);
	infile = fopen(SF_TS_BINARY, "rb");
	//not workaround for compat with sf-ts config
	if(infile){
		struct pctl_ip_tag tag;
		int ipcnt = 0;
		fseek(infile, 0, SEEK_END );
		file_size = ftell(infile);
		fseek( infile, 0, SEEK_SET );
		size = (file_size / sizeof(tag))*sizeof(u_int32_t) + 256;
		buf = (u_int32_t *)malloc(size);
		if(!buf){
			ret = -1;
			goto out;
		}
		buf_game = malloc(size);
		buf_social = malloc(size);
		buf_video = malloc(size);
		if(!buf_game || !buf_social || !buf_video) goto out;
		buf_game[l1++] = TYPE_GAME_KEY;
		buf_video[l2++] = TYPE_VIDEO_KEY;
		buf_social[l3++] = TYPE_SOCIAL_KEY;
		while(fread((void *)&tag,1,sizeof(tag),infile) == sizeof(tag)){
			memcpy(ip,&tag.ip,sizeof(ip));
			if(tag.type == IP_TAG_TYPE_GAME){
				buf_game[l1++] = *((u_int32_t *)(&ip[0]));
			}else if(tag.type == IP_TAG_TYPE_VIDEO){
				buf_video[l2++] = *((u_int32_t *)(&ip[0]));
			}else if(tag.type == IP_TAG_TYPE_SOCIAL){
				buf_social[l3++] = *((u_int32_t *)(&ip[0]));
			}else {
			}
			ipcnt++;
			if(ipcnt > MAX_IP_CNT){
				printf("ip cnt reach the max limit\n");
				break;
			}
		}
		printf("ip cnt=%d l1=%d l2=%d l3=%d\n",ipcnt,l1,l2,l3);
		ret = (l1 + 1) + (l2 + 1) + (l3 + 1);
		if(ret > size){
			ret = -1;
		}else{
			ret = 0;
			buf_game[l1] = TYPE_GAME_KEY;
			buf_video[l2] = TYPE_VIDEO_KEY;
			buf_social[l3] = TYPE_SOCIAL_KEY;
			memcpy(buf,buf_game,(l1 + 1)*sizeof(u_int32_t));
			ret += (l1 + 1);
			memcpy(buf + ret,buf_video,(l2 + 1)*sizeof(u_int32_t));
			ret += (l2 + 1);
			memcpy(buf + ret,buf_social,(l3 + 1)*sizeof(u_int32_t));
			ret += (l3 + 1);
			buf[ret] = 0;
			*inbuf = buf;
		}
out:
		if(buf_game) free(buf_game);
		if(buf_video) free(buf_video);
		if(buf_social) free(buf_social);
	}
	if(infile) fclose(infile);
	if(!(*inbuf) && buf) free(buf);
	return ret;
}

static int translateTxtToBin()
{
	int ret = 0;
	FILE *fg = NULL;
	FILE *outfile = NULL;
	char fbuf[128];
	u_int8_t ip[4];
	u_int32_t l = 0, key;
	u_int8_t type;
	struct pctl_ip_tag tag;

	outfile = fopen(SF_TS_BINARY, "wb");
	if(!outfile) goto out;
	fg = fopen(SF_TS_TXT, "r");
	if(!fg) goto out;
	while(fgets(fbuf, 128, fg) != NULL){
		if(strncmp(fbuf,"game",4) == 0){
			key = TYPE_GAME_KEY;
			type = IP_TAG_TYPE_GAME;
		}else if(strncmp(fbuf,"video",5) == 0){
			key = TYPE_VIDEO_KEY;
			type = IP_TAG_TYPE_VIDEO;
		}else if(strncmp(fbuf,"social",6) == 0){
			key = TYPE_SOCIAL_KEY;
			type = IP_TAG_TYPE_SOCIAL;
		}else{
			printf("config param error\n");
			fclose(fg);
			return -1;
		}
		while(fgets(fbuf, 128, fg) != NULL){
			++l;
			if(strncmp(fbuf,"end",3) != 0){
				ret = sscanf(fbuf,"%hhu.%hhu.%hhu.%hhu",&ip[0], &ip[1], &ip[2], &ip[3]);
				if(ret < 4){
					printf("ip format error\n");
					return -1;
				}
#if 1
				memset(&tag,0,sizeof(tag));
				tag.type = type;
				tag.flags = IP_TAG_FLAG_VALID;
				memcpy(&tag.ip[0],ip,4);
				fwrite(&tag,1,sizeof(tag),outfile);
#endif
			}else{
				break;
			}
		}
		++l;
	}
	sync_ts_tar_file(false);
out:
	if(fg) fclose(fg);
	if(outfile) fclose(outfile);
	return l;
}

static void printFormatData(void)
{
	FILE *outfile;
	char formatstr[256];
	sync_ts_tar_file(true);
	outfile = fopen(SF_TS_BINARY, "rb");
	if(outfile){
		struct pctl_ip_tag tag;
		while(fread((void *)&tag,1,sizeof(tag),outfile) == sizeof(tag)){
			sprintf(formatstr,"type=%d flags=%d ip=%d.%d.%d.%d url=%s desc=%s",tag.type,tag.flags,tag.ip[0],tag.ip[1],tag.ip[2],tag.ip[3],tag.url,tag.descp);
			printf("%s\n",formatstr);
		}
		fclose(outfile);
	}else{
			printf("open ts file fail\n");
	}
}

static void parser_tag_from_json(struct pctl_ip_tag *tag, int type, char *jsontag)
{
	char descption[512];
	char url[256];
	char *str = NULL;
	int len = 0;
	if(!tag) return;
	memset((void*)tag,0,sizeof(struct pctl_ip_tag));
	tag->type = type;
	tag->flags = IP_TAG_FLAG_VALID;
	sscanf(jsontag,"%hhu.%hhu.%hhu.%hhu,%s",&tag->ip[0], &tag->ip[1], &tag->ip[2], &tag->ip[3],&url[0]);
	str = strstr(url,",");
	*str = 0;
	str++;
	sprintf(descption,"%s",str);
	//handle url
	len = strlen(url);
	memcpy(&tag->url[0],&url[0], (len > (sizeof(tag->url) - 1)) ? (sizeof(tag->url) - 1) : len);
	tag->url[sizeof(tag->url) - 1] = 0;
	//handle descption
	len = strlen(descption);
	memcpy(&tag->descp[0],&descption[0], (len > (sizeof(tag->descp) - 1)) ? (sizeof(tag->descp) - 1) : len);
	tag->descp[sizeof(tag->descp) - 1] = 0;
#ifdef DEBUG
	{
		char formatstr[256];
		sprintf(formatstr,"type=%d flags=%d ip=%d.%d.%d.%d url=%s desc=%s",tag->type,tag->flags,tag->ip[0],tag->ip[1],tag->ip[2],tag->ip[3],tag->url,tag->descp);
		printf("%s\n",formatstr);
	}
#endif
}

struct ts_table *init_ts_table_from_file(void)
{
	FILE *outfile;
	struct ts_table *table = ta_table_init();
	sync_ts_tar_file(true);
	outfile = fopen(SF_TS_BINARY, "rb");
	if(outfile){
		struct pctl_ip_tag tag;
		while(fread((void *)&tag,1,sizeof(tag),outfile) == sizeof(tag)){
			ts_table_insert_tag(table,&tag);
		}
		fclose(outfile);
	}else{
		printf("open ts file fail2\n");
	}
	return table;
}

static int handle_json_tags(cJSON *data_obj,const char *name,int type,int action,FILE *fp,struct ts_table *table)
{
	int count = 0;
	int j = 0;
	struct pctl_ip_tag tag;
	cJSON *sub_obj = cJSON_GetObjectItem(data_obj, name);
	if(sub_obj){
		int sub_num = cJSON_GetArraySize(sub_obj);
		for(j = 0; j < sub_num; j++){
			cJSON *subitem = cJSON_GetArrayItem(sub_obj, j);
			parser_tag_from_json(&tag,type,subitem->valuestring);
			if(action == SF_TS_UPDATE_CMD_UPDATE){
				ts_table_insert_unique_tag(table,&tag);
			}else if(action == SF_TS_UPDATE_CMD_REPLACE){
				fwrite(&tag,1,sizeof(tag),fp);
			}else if(action == SF_TS_UPDATE_CMD_DELETE){
				ts_table_delete_tag(table,&tag);
			}
			count++;
		}
	}
	return count;
}


static void update_ts_from_json_file(void)
{
	FILE *fp = NULL;
	int file_size;
	size_t read_count;
	char *buffer = NULL;
	struct ts_table *table = NULL;

	fp = fopen(g_json_file, "rb");
	if(!fp) goto out;
	fseek(fp, 0, SEEK_END );
	file_size = ftell( fp );
	fseek( fp, 0, SEEK_SET );
	buffer = malloc(file_size + 1);
	if(!buffer) goto out;
	printf("++++++++++++++3\n");
	read_count = fread(buffer,1,file_size,fp);
	printf("++++++++++++++4--read=%d\n",read_count);
	if(read_count != file_size) printf("!!!!!!!!!!!!!!get read count=%d filelen=%d\n",read_count,file_size);
	buffer[file_size] = 0;
	if(read_count > 0){
		//parser from json
		cJSON *root_obj = cJSON_Parse(buffer);
		if(root_obj){
			int code = -1;
			cJSON *code_obj = cJSON_GetObjectItem(root_obj, "code");
			cJSON_GetValueInt(code_obj,&code);
			printf("++++++++++++++7--code=%d\n",code);
			if(code == 0){
				cJSON *data_obj = cJSON_GetObjectItem(root_obj, "data");
				printf("++++++++++++++8--data_obj=%p\n",data_obj);
				if(data_obj){
					 int action = -1;
					 int count_update  = 0;
					 int count_delete  = 0;
					 int count_total  = 0;
					 long long  updateAt = 0;
					 long long  province = 0;
					 int ret = 0;
					 FILE *outfile = NULL;
					 char province_str[64];
					 char updateAt_str[256];
					 //parser action
					 cJSON *action_obj = cJSON_GetObjectItem(data_obj, "action");
					 if(!action_obj) { ret = -1; goto end; }
			    	 printf("++++++++++++++9--action=%d\n",action);
					 cJSON_GetValueInt(action_obj,&action);
					 sync_ts_tar_file(true);
					 //check if action is legal
					 if(action == SF_TS_UPDATE_CMD_UPDATE){
						table = init_ts_table_from_file();
					 }else if(action == SF_TS_UPDATE_CMD_REPLACE){
						outfile = fopen(SF_TS_BINARY, "wb");
						if(!outfile) { ret = -3; goto end; }
				     }else{
						LOG("unknown update cmd!!!\n");
						ret = -4;
						goto end;
					 }
					 //parser update
					 cJSON *update_obj = cJSON_GetObjectItem(data_obj, "updateAt");
					 if(!update_obj){ ret = -4; goto end; }
					 cJSON_GetValueInt64(update_obj,&updateAt);
					 printf("++++++++++++++10--update=%lld\n",updateAt);
					 //parser province
					 cJSON *province_obj = cJSON_GetObjectItem(data_obj, "province");
					 if(!province_obj){ ret = -5; goto end; }
					 cJSON_GetValueInt64(province_obj,&province);

					 //static void handle_json_tags(cJSON *data_obj,const char *name,int type,int action,FILE *fp,struct ts_table *table)
					 count_update += handle_json_tags(data_obj,"game",IP_TAG_TYPE_GAME,action,outfile,table);
					 count_update += handle_json_tags(data_obj,"video",IP_TAG_TYPE_VIDEO,action,outfile,table);
					 count_update += handle_json_tags(data_obj,"social",IP_TAG_TYPE_SOCIAL,action,outfile,table);
					 if(action != SF_TS_UPDATE_CMD_REPLACE){
						 //if not replace, we handle delete tags
		    			 count_delete += handle_json_tags(data_obj,"game_delete",IP_TAG_TYPE_GAME,SF_TS_UPDATE_CMD_DELETE,outfile,table);
		 				 count_delete += handle_json_tags(data_obj,"video_delete",IP_TAG_TYPE_VIDEO,SF_TS_UPDATE_CMD_DELETE,outfile,table);
						 count_delete += handle_json_tags(data_obj,"social_delete",IP_TAG_TYPE_SOCIAL,SF_TS_UPDATE_CMD_DELETE,outfile,table);
					 }
					 if(table){
						 count_total = sync_table_to_file(table,SF_TS_BINARY);
					 }else{
						 count_total = count_update;
					 }
					 sprintf(province_str,"%lld",province);
					 sprintf(updateAt_str,"%lld",updateAt);
					 printf("province=%s updateAt=%s update cnt %d delete %d\n",province_str,updateAt_str,count_update,count_delete);
					 setTsValueToUci(province_str,updateAt_str,action,count_total);
end:
					LOG("update ts ret=%d !!!\n",ret);
					if(table) ta_table_deinit(table);
					if(outfile) fclose(outfile);
					sync_ts_tar_file(false);
				}
			}
		}
		if(root_obj) free(root_obj);
	}

out:
	if(buffer) free(buffer);
	if(fp) fclose(fp);
}

int load_ts_file(void)
{
	u_int32_t *buf = NULL;
	u_int32_t pid;
	int32_t len = 0;
	int ret = 0;
	int real_len = 0;
	int max_size = 0;
	struct rtnl_handle gnrtl;

//	len = get_ip_info_from_text(buf);
	len = get_ip_info_from_binary(&buf);
	printf("get_ip_info buf=%p len=%d\n",buf,len);
	if(len <= 0){
		ret = -1;
		goto out;
	}

	ret = sf_ts_genl(&gnrtl);
	if(ret < 0){
		goto out1;
	}
	pid = getpid();
	printf("%d\n", gnrtl.family_id);
	//NLA_HDRLEN
	max_size = MAX_MSG_SIZE - NLA_HDRLEN - 256;
	real_len = ((len*4) > max_size) ? max_size : (len*4);
	printf("send msg len %d\n",real_len);
	ret = genl_send_msg(gnrtl.fd, gnrtl.family_id, 0, SF_TS_CMD, 1, IP_INFO, (void *)buf, real_len);
	if(ret < 0){
		LOG("netlink send data error %d\n", ret);
		goto out;
	}
out:
	rtnl_close(&gnrtl);
out1:
	if(buf) free(buf);
    return ret;
}

void do_ts_test(int max)
{
	struct ts_table *table = ta_table_init();
	int i,j,m,n;
	struct pctl_ip_tag tag;
	sprintf(tag.url,"%s","www.url.testwwwwwwwwwwwwwwwwwwwwwwwwwwwww.com");

	max &= 0xFF;
	printf("ts test start\n");
	for(i = 0 ; i < max ; i++){
		for(j = 0 ; j < max ; j++){
			for(m = 0 ; m < max ; m++){
				for(n = 0 ; n < max ; n++){
					tag.ip[0] = i & 0xFF;
					tag.ip[1] = j & 0xFF;
					tag.ip[2] = m & 0xFF;
					tag.ip[3] = n & 0xFF;
					ts_table_insert_unique_tag(table,&tag);
				}
			}
		}
	}
	printf("ts test start end\n");

	if(table) ta_table_deinit(table);
}

int main(int argc, char *argv[])
{
    int ret = 0;
	int32_t n = 0;
	int param = 0;
	while (n >= 0) {
		n = getopt_long(argc, argv, "d:hf:iacu:t:", NULL, NULL);
		if (n < 0)
			continue;
		switch (n) {
			case 'd':
				break;
			case 'f':
				LOG("file:%s\n",optarg);
				break;
			case 'u':
				LOG("json file:%s\n",optarg);
				g_json_file = optarg;
				goto update;
			case 'i':
				goto init;
			case 'a':
				goto printdata;
				break;
			case 'c':
				goto convertdata;
				break;
			case 't':
				param = atoi(optarg);
				goto dotest;
				break;
			case 'h':
				goto usage;
		}
	}

usage:
	LOG("not implement!\n");
	return 0;

update:
	update_ts_from_json_file();
	return ret;

init:
	ret = load_ts_file();
	return ret;
printdata:
	printFormatData();
	return 0;

convertdata:
	translateTxtToBin();
	return 0;
dotest:
	do_ts_test(param);
	return 0;
}
