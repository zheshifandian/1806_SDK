/*
 * =====================================================================================
 *
 *       Filename:  local_storage.c
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  2015年08月19日 11时23分13秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  robert , robert.chang@siflower.com.cn
 *        Company:  Siflower
 *
 * =====================================================================================
 */

#include "local_storage.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cloud_common.h"
#include <uci.h>
#include "mtd.h"
#include "sf_factory_read.h"
int32_t uciDeleteOption(struct uci_context *ctx,struct uci_package *p,struct uci_section *section,const char *option)
{
    struct uci_ptr ptr = { .p = p, .s = section};
    ptr.o      = NULL;
    ptr.option = option;
    ptr.value  = NULL;
    return uci_delete(ctx,&ptr);
}

void uciSetValue(struct uci_context *ctx,struct uci_ptr *ptr,char *key,char *retBuffer)
{
    ptr->o      = NULL;
    ptr->option = key;
    ptr->value  = retBuffer;
    uci_set(ctx, ptr);
}

int32_t setRouterValueToUci(char *sectionName,const char *key,char *retBuffer)
{
    struct uci_context *ctx = uci_alloc_context();
    struct uci_package *p = NULL;
    int ret = -1;
    uci_set_confdir(ctx, "/etc/config");

    if(uci_load(ctx, "siserver", &p) == UCI_OK)
    {
        struct uci_section *router = uci_lookup_section(ctx, p, sectionName);
        if(!router){
            //create section force if not exist
            struct uci_ptr ptr;
            memset(&ptr, 0, sizeof(struct uci_ptr));
            ptr.value = "cloud";
            ptr.package = "siserver";
            ptr.section = sectionName;
            ptr.option = NULL;
            uci_set(ctx, &ptr);
            uci_save(ctx,p);
            router = uci_lookup_section(ctx, p, sectionName);
        }
        //lookup values
        if(router != NULL){
            struct uci_ptr ptr = { .p = p, .s = router};
            ptr.o      = NULL;
            ptr.option = key;
            ptr.value  = retBuffer;
            uci_set(ctx, &ptr);
            uci_save(ctx,p);
            uci_commit(ctx,&p,false);
            ret = 0;
        }

        uci_unload(ctx,p);
    }
    uci_free_context(ctx);
    return ret;
}

int32_t getRouterValueFromUci(const char *key,char *retBuffer)
{
    struct uci_context *ctx = uci_alloc_context();
    struct uci_package *p = NULL;
    int ret = -1;
    uci_set_confdir(ctx, "/etc/config");
    if(uci_load(ctx, "siserver", &p) == UCI_OK)
    {
        struct uci_section *router = uci_lookup_section(ctx, p, "cloudrouter");
        //lookup values
        if(router != NULL){
            const char *value = uci_lookup_option_string(ctx, router, key);
            if(value != NULL){
                ret = 0;
                sprintf(retBuffer,"%s",value);
            }
        }

        uci_unload(ctx,p);
    }
    uci_free_context(ctx);
    return ret;
}

int32_t createUciRouterSection(char *sectionType,char *sectionName)
{
    struct uci_context *ctx = uci_alloc_context();
    struct uci_package *p = NULL;
    int ret = -1;
    uci_set_confdir(ctx, "/etc/config");
    if(uci_load(ctx, "siserver", &p) == UCI_OK)
    {
        struct uci_ptr ptr;
        memset(&ptr, 0, sizeof(struct uci_ptr));
        ptr.value = sectionType;
        ptr.package = "siserver";
        ptr.section = sectionName;
        ptr.option = NULL;
        uci_set(ctx, &ptr);
        uci_save(ctx,p);
        uci_commit(ctx,&p,false);
        ret = 0;
        uci_unload(ctx,p);
    }
    uci_free_context(ctx);
    return ret;
}

void insertUnFinishedInvitMsg(char *tag,char *destUserId,char *phone,char *destUserName,char *destUserSub,char *time)
{
    if(createUciRouterSection("invitmessage",tag) == 0){
        struct uci_context *ctx = uci_alloc_context();
        struct uci_package *p = NULL;
        int ret = -1;
        uci_set_confdir(ctx, "/etc/config");
        if(uci_load(ctx, "siserver", &p) == UCI_OK)
        {
            struct uci_section *router = uci_lookup_section(ctx, p, tag);
            if(router != NULL){
                struct uci_ptr ptr = { .p = p, .s = router};
                uciSetValue(ctx,&ptr,"destUserId",destUserId);
                uciSetValue(ctx,&ptr,"phone",phone);
                uciSetValue(ctx,&ptr,"destUserName",destUserName);
                uciSetValue(ctx,&ptr,"destUserSub",destUserSub);
                uciSetValue(ctx,&ptr,"updatedTime",time);
                //save
                uci_save(ctx,p);
                uci_commit(ctx,&p,false);
            }
            uci_unload(ctx,p);
        }
        uci_free_context(ctx);
    }

}

int32_t checkUnFinishedInvitMsg(char *tag,char *destUserId,char *phone,char *destUserName,char *destUserSub)
{
    struct uci_context *ctx = uci_alloc_context();
    struct uci_package *p = NULL;
    int ret = -1;
    uci_set_confdir(ctx, "/etc/config");
    if(uci_load(ctx, "siserver", &p) == UCI_OK)
    {
        struct uci_section *router = uci_lookup_section(ctx, p, tag);
        if(router != NULL){
            ret = 0;
            //delete this message tag
            struct uci_ptr ptr;
            memset(&ptr, 0, sizeof(struct uci_ptr));
            ptr.value = "invitmessage";
            ptr.package = "siserver";
            ptr.section = tag;
            ptr.option = NULL;
            uci_delete(ctx,&ptr);
            uci_save(ctx,p);
            uci_commit(ctx,&p,false);
        }
        uci_unload(ctx,p);
    }
    uci_free_context(ctx);
    return ret;
}

void writeServerConnectState(int32_t connected)
{
    char *connectedStr = connected ? "1" : "0";
    setRouterValueToUci("serverstate","connected",connectedStr);
}

int32_t getSfHardwareConfig(const char *key,char *retBuffer)
{
    struct uci_context *ctx = uci_alloc_context();
    struct uci_package *p = NULL;
    int ret = -1;
    uci_set_confdir(ctx, "/etc/config");
    if(uci_load(ctx, "siwifi", &p) == UCI_OK)
    {
        struct uci_section *router = uci_lookup_section(ctx, p, "hardware");
        if(router != NULL){
            const char *value = uci_lookup_option_string(ctx, router, key);
            if(value != NULL){
                ret = 0;
                sprintf(retBuffer,"%s",value);
            }
        }
        uci_unload(ctx,p);
    }
    uci_free_context(ctx);
    return ret;
}
#define ETH_ALEN 6
static void inc_sf_mac_addr(char *mac, int inc)
{
	unsigned int mac_calc[ETH_ALEN] = {0};
	int i = 0;
	for(i = 0 ; i < ETH_ALEN ; i++)
	{
		mac_calc[i] = (unsigned int)(mac[i]) & 0xFF;
	}
	mac_calc[ETH_ALEN - 1] += inc;
	mac_calc[ETH_ALEN - 2] += ((mac_calc[ETH_ALEN - 1] & (0x100))  >> 8);
	mac[ETH_ALEN - 1] = mac_calc[ETH_ALEN - 1] & (0xff);

	mac_calc[ETH_ALEN - 3] += ((mac_calc[ETH_ALEN - 2] & (0x100))  >> 8);
	mac[ETH_ALEN - 2] = mac_calc[ETH_ALEN - 2] & (0xff);
	mac[ETH_ALEN - 3] = mac_calc[ETH_ALEN - 3] & (0xff);
	//the first 3 char is reserved
}

void get_router_macaddr(char *mac_in)
{
	if(sf_factory_read_operation(READ_FIRST_MAC_ADDRESS, mac_in, ETH_ALEN)){
		mtd_operation(CMD_READ, mac_in, 6, 0);
	}
	inc_sf_mac_addr(&mac_in[0],2);
}
