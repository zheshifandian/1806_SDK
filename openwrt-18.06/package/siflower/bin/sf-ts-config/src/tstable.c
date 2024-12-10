/*
 * =====================================================================================
 *
 *       Filename:  tstable.c
 *
 *    Description:  for ts tag search
 *
 *        Version:  1.0
 *        Created:  2018年10月30日 19时01分40秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  robert , robert.chang@siflower.com.cn
 *        Company:  Siflower
 *
 * =====================================================================================
 */

#include "strings.h"
#include "tsconfig.h"

#define LOG(X,...) printf(X,##__VA_ARGS__)

static void freetag(void *freethis)
{
	free(freethis);
}

struct ts_table *ta_table_init(void)
{
	struct ts_table *table = (struct ts_table *)(malloc(sizeof(struct ts_table)));
	int rc;
	if(table){
		rc = Curl_hash_init(&(table->iptag_cache), 100, Curl_hash_str,Curl_str_key_compare, freetag);
		if(rc){
			//fail
			LOG("Curl_hash_init fail rc=%d\n",rc);
			free(table);
			table = NULL;
		}
	}
	//init hash table
	return table;
}

void create_hostcache_id(struct pctl_ip_tag *iptag,char *entry)
{
	sprintf(entry,"%d.%d.%d.%d-%s",iptag->ip[0],iptag->ip[1],iptag->ip[2],iptag->ip[3],iptag->url);
}

struct ts_tag *ts_table_search_tag(struct ts_table *table,struct pctl_ip_tag *iptag)
{
	char entry_id[80];
	size_t entry_len;
	struct ts_tag *tag = NULL;
	create_hostcache_id(iptag,entry_id);
	entry_len = strlen(entry_id) + 1;
	tag = Curl_hash_pick(&(table->iptag_cache), entry_id, entry_len);
	return tag;
}

void ts_table_insert_tag(struct ts_table *table,struct pctl_ip_tag *iptag)
{
	struct ts_tag *tag = (struct ts_tag *)(malloc(sizeof(struct ts_tag)));
	char entry_id[80];
	size_t entry_len;
	memcpy(&((*tag).iptag), iptag, sizeof(struct pctl_ip_tag));
	create_hostcache_id(iptag,entry_id);
	entry_len = strlen(entry_id) + 1;
	Curl_hash_add(&(table->iptag_cache), entry_id, entry_len,(void *)tag);
}

void ts_table_insert_unique_tag(struct ts_table *table,struct pctl_ip_tag *iptag)
{
	struct ts_tag *tag = ts_table_search_tag(table,iptag);
	if(tag){
		memcpy(&((*tag).iptag), iptag, sizeof(struct pctl_ip_tag));
	}else{
		ts_table_insert_tag(table,iptag);
	}
}

void ts_table_delete_tag(struct ts_table *table,struct pctl_ip_tag *iptag)
{
	char entry_id[80];
	size_t entry_len;
	create_hostcache_id(iptag,entry_id);
	entry_len = strlen(entry_id) + 1;
	Curl_hash_delete(&(table->iptag_cache), entry_id, entry_len);
}

void ta_table_deinit(struct ts_table *table)
{
	Curl_hash_destroy(&(table->iptag_cache));
}

int sync_table_to_file(struct ts_table *table,const char *filename)
{
	//write table back to file
	FILE *outfile;
	int total = 0;
	struct curl_hash_iterator iter;
	struct curl_hash_element *he;
	struct pctl_ip_tag *iptag;

	outfile = fopen(filename, "wb");
	if(outfile){
		Curl_hash_start_iterate(&(table->iptag_cache), &iter);
		he = Curl_hash_next_element(&iter);
		while(he) {
			iptag = (struct pctl_ip_tag *)(he->ptr);
			fwrite(iptag,1,sizeof(*iptag),outfile);
			he = Curl_hash_next_element(&iter);
			total++;
		}
		fclose(outfile);
	}
	return total;
}
