#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>

#include "utils.h"
#include "cJSON.h"

#define URLFILE "/etc/url_list"
#define FLAGS O_RDWR| O_CREAT
#define MODE S_IRWXU | S_IXGRP | S_IROTH | S_IXOTH
// if is add == 1 add mac is_add == 0 remove mac
int32_t set_ipset(unsigned char * mac, unsigned char is_add){
	char cmd[256];
	if(is_add)
	  sprintf(cmd,"ipset add urllist_mac %02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	else
	  sprintf(cmd,"ipset del urllist_mac %02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

	LOG( "operate urllist_mac %s", cmd);
	system(cmd);
	return 0;
}
unsigned char* get_mmap_addr(unsigned short expand_len, unsigned int * psize, int* pfd, unsigned int *pmap_total_len){
	int fd;
	struct stat sb;
	unsigned char* addr = NULL;
	unsigned int map_size = 0;

	fd = open(URLFILE, FLAGS, MODE);
	if(fd < 0){
		LOG("failed to open url list\n");
		return NULL;
	}

	if (fstat(fd, &sb) == -1){ /* To obtain file size */
		LOG("get file info error\n");
		close(fd);
		return NULL;
	}

	*psize = sb.st_size;
	*pfd = fd;

	ftruncate(fd, sb.st_size + expand_len);
	// align to 4K page
	if(sb.st_size == 0) map_size = 4096;
	else  map_size =  ((sb.st_size+ expand_len + 4096 - 1) & (~( 4096 - 1)));
	// memory ref to dns_control.pdf
	// jffs2 not support shared memory
	addr = (unsigned char *) mmap(NULL, map_size, PROT_READ|PROT_WRITE, MAP_PRIVATE, fd, 0);
	if (addr == MAP_FAILED){
		LOG("================map file error,  errno %s\n", strerror(errno));
		return NULL;
	}
	*pmap_total_len = map_size;
	return addr;
}

int change_file_size(int size, unsigned int map_addr_len,int fd ){
	ftruncate(fd, map_addr_len + size);
	close(fd);
	return 0;
}


static unsigned short char_to_short(unsigned char* src){
	unsigned short dst = *src | (*(src + 1) << 8 );
	return dst;
}
static void short_to_char(unsigned short src, unsigned char* dst){
	*dst = src & 0x00ff;
	*(dst + 1) = (src & 0xff00) >> 8;
	return ;
}

unsigned char * generate_flat_url_list(cJSON *listitems, unsigned short *plen){
	unsigned short i = 0;
	unsigned short len = 0;
	for (; i < cJSON_GetArraySize(listitems) ; i++) {
		cJSON * listitem = cJSON_GetArrayItem(listitems, i);
		len += strlen(listitem->valuestring) + 2;
	}
	unsigned short flat_url_list_len = len + 2;
	// add node count
	unsigned char * flat_url_list = malloc(flat_url_list_len);
	short_to_char(i, flat_url_list);
	unsigned char * node_count_array = flat_url_list +2;
	unsigned char * url_string = node_count_array + i*2;
	for (i = 0; i < cJSON_GetArraySize(listitems) ; i++) {
		cJSON * listitem = cJSON_GetArrayItem(listitems, i);
		len = strlen(listitem->valuestring);
		short_to_char(len,node_count_array);
		memcpy(url_string, listitem->valuestring, len );
		LOG(" save %s, len %d", listitem->valuestring, len);
		node_count_array += 2;
		url_string+=len;
	}
	*plen = flat_url_list_len ;
	return flat_url_list;
}

unsigned char * generate_flat_mac_node_hdr( unsigned char* mac, unsigned short len, char listtype ){
	unsigned char * flat_mac_node_hdr = malloc(9);
	short_to_char(len,flat_mac_node_hdr);
	memcpy((flat_mac_node_hdr + 2), mac, 6);
	*(flat_mac_node_hdr+8) = listtype;
	return flat_mac_node_hdr;
}

int search_flat_mac_node(unsigned char** ppmap_flat_mac_node_hdr, unsigned char **ppmap_flat_url_list, unsigned int * pmap_flat_url_list_len, unsigned char* map_addr, unsigned char* mac){
	unsigned short map_node_count = char_to_short(map_addr+7);
	LOG("node count is %d", map_node_count);
	unsigned char * flat_url_list = map_addr + 9 + 9 * map_node_count;
	unsigned int len = 0, i = 0;
	for(i = 0; i < map_node_count; i++){
		unsigned char * flat_mac_node_hdr = map_addr + 9 + 9 * i;
		len = char_to_short(flat_mac_node_hdr);
		LOG("mac is %02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
		LOG("map mac is %02x:%02x:%02x:%02x:%02x:%02x", flat_mac_node_hdr[2], flat_mac_node_hdr[3], flat_mac_node_hdr[4], flat_mac_node_hdr[5], flat_mac_node_hdr[6], flat_mac_node_hdr[7]);
		if(memcmp(flat_mac_node_hdr + 2, mac, 6 ) == 0){
			*ppmap_flat_mac_node_hdr  = flat_mac_node_hdr;
			*ppmap_flat_url_list = flat_url_list;
			*pmap_flat_url_list_len = len;
			return 0;
		}
		flat_url_list += len;
	}
	return -1;
}
// return size of change
int change_flat_mac_node(unsigned char* flat_mac_node_hdr, unsigned char * flat_url_list,unsigned short flat_url_list_len, unsigned char* map_addr, unsigned int  map_addr_len,unsigned char* mac){
	unsigned char * pmap_flat_url_list = NULL;
	unsigned char * pmap_flat_mac_node_hdr= NULL;
	unsigned int  map_flat_url_list_len = 0;
	unsigned int move_len = 0;
	unsigned short node_count = char_to_short(map_addr+7);
	unsigned char * plast_mac_node= map_addr + 9 + 9 * (node_count -1);
	if(search_flat_mac_node(&pmap_flat_mac_node_hdr, &pmap_flat_url_list, &map_flat_url_list_len, map_addr, mac) < 0){
		return -11111;
	}
	if(plast_mac_node != pmap_flat_mac_node_hdr)
	  memmove(pmap_flat_mac_node_hdr, pmap_flat_mac_node_hdr+ 9,(plast_mac_node - pmap_flat_mac_node_hdr) );

	memcpy(plast_mac_node,flat_mac_node_hdr,9);


	move_len = map_addr + map_addr_len - (pmap_flat_url_list + map_flat_url_list_len);
	memmove(pmap_flat_url_list, pmap_flat_url_list + map_flat_url_list_len, move_len);
	memcpy(map_addr + map_addr_len - map_flat_url_list_len,flat_url_list,flat_url_list_len);

	return flat_url_list_len - map_flat_url_list_len;
}

// return is the file size  delete
int delete_flat_mac_node(unsigned char* map_addr, unsigned char* mac, unsigned int map_addr_len){

	if(map_addr_len < 18)
	  return 0;
	unsigned short map_node_count = char_to_short(map_addr+7);
	unsigned char * pmap_flat_url_list = NULL;
	unsigned char * pmap_flat_mac_node_hdr= NULL;
	unsigned int  map_flat_url_list_len = 0;
	unsigned int move_len = 0;
	if(search_flat_mac_node(&pmap_flat_mac_node_hdr, &pmap_flat_url_list, &map_flat_url_list_len, map_addr, mac) == 0){

		move_len = map_addr + map_addr_len - (pmap_flat_url_list + map_flat_url_list_len);
		memmove(pmap_flat_url_list, pmap_flat_url_list + map_flat_url_list_len, move_len);
		// move from mac node
		move_len = map_addr+ map_addr_len - (pmap_flat_mac_node_hdr + 9) -  map_flat_url_list_len;
		memmove(pmap_flat_mac_node_hdr, pmap_flat_mac_node_hdr + 9, move_len);
		map_node_count--;
		short_to_char(map_node_count, map_addr + 7);

		// turn to negative
		return   ~(map_flat_url_list_len + 9) + 1;
	}
	else return 0;

}
// return size change
int add_flat_mac_node(unsigned char* flat_mac_node_hdr, unsigned char * flat_url_list,unsigned short flat_url_list_len, unsigned char* map_addr, unsigned int map_addr_len){
	// first inser flat_mac_node_hdr()
	unsigned short map_node_count = char_to_short(map_addr+7);
	unsigned char* map_flat_url_list = map_addr + 9 + 9* map_node_count;
	unsigned int  move_len = map_addr+ map_addr_len - (map_flat_url_list);
	memmove(map_flat_url_list + 9, map_flat_url_list, move_len);
	unsigned char* map_new_mac_node_hdr = map_addr + 9 + 9* map_node_count;
	memcpy(map_new_mac_node_hdr, flat_mac_node_hdr, 9);
	memcpy(map_addr + map_addr_len + 9, flat_url_list, flat_url_list_len);
	map_node_count++;
	short_to_char(map_node_count, map_addr + 7);

		// turn to negative
	return 9+flat_url_list_len;
}

int add_first_flat_mac_node(unsigned char* flat_mac_node_hdr, unsigned char * flat_url_list,unsigned short flat_url_list_len, unsigned char* map_addr, unsigned int map_addr_len){
	// first inser flat_mac_node_hdr()
	unsigned short map_node_count = 0;
	unsigned char* map_new_mac_node_hdr = map_addr + 9;
	memcpy(map_new_mac_node_hdr, flat_mac_node_hdr, 9);
	map_node_count++;
	memcpy(map_addr + 9 + 9*map_node_count, flat_url_list, flat_url_list_len);
	short_to_char(map_node_count, map_addr + 7);

	if(map_addr_len == 9) return 9+flat_url_list_len;
	return 9+9+flat_url_list_len;
}

int set_operate(unsigned char optype, unsigned char* mac, unsigned char* map_addr){
	if(mac) memcpy(map_addr, mac, 6);
	map_addr[6] = optype;
	return 0;
}
int parse_data(cJSON* root){
	unsigned char mac[6] = {0};
	unsigned char * map_addr = NULL, *flat_url_list = NULL, *flat_mac_node_hdr = NULL;
	unsigned char optype = 0;
	unsigned char listtype = 0;
	int size_change = 0, fd = 0;
	unsigned int map_addr_len;
	unsigned int map_total_len;
	unsigned short  flat_url_list_len;
	cJSON * p_jmac = cJSON_GetObjectItem(root, "mac");
	cJSON * p_jfunc= cJSON_GetObjectItem(root, "func");
	cJSON * p_juci_func= cJSON_GetObjectItem(root, "uci_func");
	cJSON *listitems = cJSON_GetObjectItem(root,"list");

	int func = atoi(p_jfunc->valuestring);
	int uci_func = atoi(p_juci_func->valuestring);
	LOG(" func is %d uci func is %d",func, uci_func);
	sscanf(p_jmac->valuestring, "%02hhx_%02hhx_%02hhx_%02hhx_%02hhx_%02hhx", &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);
	// func means 0 diable 1 enable white 2 enable black 3 set white with empty list 3 set black with empty list

	// set listtype
	if(func == 1) listtype = 0;
	else if(func == 2) listtype = 1;

	// optype: 0 add / 1 change/ 2 delete/  3 init

	// delete
	if(func == 0 || func == 3 || func == 4){
		if(uci_func == 0 || uci_func == 3 || uci_func == 4)  return -1;
		else {
			optype = 2;
			LOG("delete mac");
			map_addr = get_mmap_addr(0, &map_addr_len,&fd, &map_total_len);
			size_change = delete_flat_mac_node(map_addr,mac, map_addr_len);
			if(size_change ==  0){
				munmap(map_addr, map_total_len);
				close(fd);
				LOG("do nothing");
				return -1;
			}
			set_ipset(mac, 0);
		}
	}

	if(func == 1 || func == 2 ){
		if(uci_func == 1 || uci_func == 2){
			optype = 1;
			LOG("change mac");
			flat_url_list = generate_flat_url_list(listitems, &flat_url_list_len);
			map_addr = get_mmap_addr(flat_url_list_len, &map_addr_len, &fd, &map_total_len);
			flat_mac_node_hdr = generate_flat_mac_node_hdr(mac, flat_url_list_len, listtype);
			size_change = change_flat_mac_node(flat_mac_node_hdr, flat_url_list, flat_url_list_len,  map_addr,  map_addr_len, mac);
			free(flat_url_list);
			free(flat_mac_node_hdr);
			// special error value for  not find the operate mac
			if(size_change ==  -11111){
				munmap(map_addr, map_total_len);
				ftruncate(fd, map_addr_len);
				close(fd);
				LOG("do nothing");
				return -1;
			}
		}
		else{
			optype = 0;
			LOG("add mac");
			flat_url_list = generate_flat_url_list(listitems, &flat_url_list_len);
			LOG("generate url_list success len %d", flat_url_list_len);
			map_addr = get_mmap_addr(flat_url_list_len + 9, &map_addr_len, &fd, &map_total_len);
			LOG("get map_len %d total %d", map_addr_len, map_total_len);
			flat_mac_node_hdr = generate_flat_mac_node_hdr(mac, flat_url_list_len, listtype);
			LOG("generate mac node success");
			if(map_addr_len < 18) size_change = add_first_flat_mac_node(flat_mac_node_hdr, flat_url_list, flat_url_list_len,  map_addr,  map_addr_len);
			else size_change = add_flat_mac_node(flat_mac_node_hdr, flat_url_list, flat_url_list_len,  map_addr,  map_addr_len);
			LOG("size change %d", size_change);
			free(flat_url_list);
			free(flat_mac_node_hdr);
			set_ipset(mac, 1);
		}
	}

	// set operate
	set_operate(optype,mac, map_addr);
	write(fd, map_addr, map_addr_len + size_change);
	LOG("write file size change is %d", size_change);
	munmap(map_addr, map_total_len);
	change_file_size(size_change, map_addr_len, fd);
	return 0;
}

int send_signal(){
	// get dhcp pid
	///var/run/dnsmasq/dnsmasq.pid
	char buf[7] ={'\0'};
	FILE *file = fopen("/var/run/dnsmasq/dnsmasq.pid", "r");
	if(fread(buf, 1, 7, file) == 0){
		fclose(file);
		return -1;
	}

	fclose(file);
	unsigned int dns_pid = atoi(buf);

	LOG("==============send signal to dnsmasq pid %d", dns_pid);
	kill(dns_pid, SIGUSR2);
	return 0;
}

int32_t do_urllist_change(char *data, char **callback) {
	int ret = -1;
	LOG( "urllist data %s, len = %d", data, strlen(data));
	cJSON *root=cJSON_Parse(data);
	ret = parse_data(root);
	cJSON_Delete(root);
	if(ret >= 0) send_signal();
	return ret;
}
// 0 means set up ipset 1 means dump node
int32_t traverse_urllist(unsigned char * map_addr, unsigned char operation){
	unsigned short node_count = char_to_short(map_addr+7);
	int i = 0, len = 0;
	char mac[6] = {0};
	char url[51] = {'\0'};
	char listtype = 0;
	unsigned char * flat_url_list = map_addr + 9 + 9*node_count;
	unsigned char *flat_mac_node_hdr = NULL;
	for(i = 0; i < node_count; i++){
		flat_mac_node_hdr = map_addr + 9 + 9 * i;
		len = char_to_short(flat_mac_node_hdr);
		memcpy(mac, flat_mac_node_hdr + 2, 6 );
		if(operation == 0){
			set_ipset((unsigned char *)mac, 1);
		}
		else if(operation == 1){
			listtype = *(flat_mac_node_hdr + 8);
			LOG( "urllist mac is %s", mac);
			LOG( "list type is  is %d", listtype);
			unsigned short count = char_to_short(flat_url_list);
			unsigned short i = 0, len = 0;
			unsigned char * flat_url_string = flat_url_list + 2 + 2*count;
			for(i = 0; i < count; i++){
				// should include in len
				len = char_to_short(flat_url_list + 2 + 2*i);
				if(len < 50){
					memcpy(url, flat_url_string, len );
					url[len] = '\0';
				}
				else{
					memcpy(url, flat_url_string, 50);
					url[50] = '\0';
				}
				LOG( "url is %s", url);
				flat_url_string += len;
			}

		}
		else{
			return -1;
		}
		flat_url_list += len;
	}
	return 0;
}


int32_t urllist_init() {
	// init
	int fd = -1;
	unsigned int map_addr_len = 0, map_total_len = 0;
	unsigned char* map_addr = get_mmap_addr(0, &map_addr_len,&fd, &map_total_len);
	if(map_addr_len > 18){
		traverse_urllist(map_addr, 0);
		traverse_urllist(map_addr, 1);
		set_operate(3,NULL, map_addr);
		LOG( "do init file len %d optype %d", map_addr_len, map_addr[6]);
		write(fd, map_addr, map_addr_len);
		LOG("write file");
		close(fd);
		munmap(map_addr, map_total_len);
		send_signal();
	}
	else munmap(map_addr, map_addr_len);

	return 0;
}
