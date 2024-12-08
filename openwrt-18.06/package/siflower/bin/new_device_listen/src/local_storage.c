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
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include "utils.h"
#include <uci.h>

static char *typestr = NULL;
static const char *cur_section_ref = NULL;
static struct uci_type_list *type_list = NULL;

struct uci_type_list {
    unsigned int idx;
    const char *name;
    struct uci_type_list *next;
};

static void
uci_reset_typelist(void)
{
    struct uci_type_list *type;
    while (type_list != NULL) {
            type = type_list;
            type_list = type_list->next;
            free(type);
    }
    if (typestr) {
        free(typestr);
        typestr = NULL;
    }
    cur_section_ref = NULL;
}


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

int32_t getValueFromSiServer(const char *section,const char *key,char *retBuffer)
{
    struct uci_context *ctx = uci_alloc_context();
    struct uci_package *p = NULL;
    int ret = -1;
    uci_set_confdir(ctx, "/etc/config");
    if(uci_load(ctx, "siserver", &p) == UCI_OK)
    {
        struct uci_section *router = uci_lookup_section(ctx, p, section);
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

int32_t getRouterValueUci(const char *key,char *retBuffer)
{
    return getValueFromSiServer("cloudrouter",key,retBuffer);
}

int32_t getServerStateValueUci(const char *key,char *retBuffer)
{
    return getValueFromSiServer("serverstate",key,retBuffer);
}

int32_t getServerConnectedState()
{
    char retBuffer[16] = "";
    int32_t ret = getServerStateValueUci("connected",retBuffer);
    if(ret == 0) ret = atoi(retBuffer);
    return ret;
}

int32_t setRouterValueUci(const char *key,char *value)
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
            struct uci_ptr ptr = { .p = p, .s = router};
            ptr.o      = NULL;
            ptr.option = key;
            ptr.value  = value;
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


int32_t getRoutersValueFromSection(struct uci_context *ctx,struct uci_section *router,const char *key,char *retBuffer)
{
    int ret = -1;
    //lookup values
    if(router != NULL){
        const char *value = uci_lookup_option_string(ctx, router, key);
        if(value != NULL){
            ret = 0;
            sprintf(retBuffer,"%s",value);
        }
    }
    return ret;
}


int32_t saveRouterManager(const char *managerObjectId)
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
            struct uci_ptr ptr = { .p = p, .s = router};
            //update num
            char managernum[4] = "";
            if((getRoutersValueFromSection(ctx,router,"managernum",managernum) == 0) && (atoi(managernum) >= 0))
            {
                sprintf(managernum,"%d",atoi(managernum) + 1);
            }else{
                sprintf(managernum,"%d",1);
            }
            uciSetValue(ctx,&ptr,"managernum",managernum);
            //add manager object
            ptr.o      = NULL;
            ptr.option = "manager";
            ptr.value  = managerObjectId;
            uci_add_list(ctx,&ptr);
            //save
            uci_save(ctx,p);
            uci_commit(ctx,&p,false);
            ret = 0;
        }

        uci_unload(ctx,p);
    }
    uci_free_context(ctx);
    return ret;
}

int32_t delRouterManager(const char *managerObjectId)
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
            struct uci_ptr ptr = { .p = p, .s = router};
            struct uci_option *option = uci_lookup_option(ctx,router,"manager");
            if(option != NULL && option->type == UCI_TYPE_LIST){
                char managernum[4] = "";
                if((getRoutersValueFromSection(ctx,router,"managernum",managernum) == 0) && (atoi(managernum) >= 1))
                {
                    int32_t count = atoi(managernum);
                    int32_t found = 0;
                    if(count > 0){
                        int counter = 0;
                        struct uci_element *e;
                        uci_foreach_element(&option->v.list, e) {
                            if(!strcmp(e->name,managerObjectId)){
                                found = 1;
                                break;
                            }
                            counter++;
                            if(counter == count) break;
                        }
                    }
                    if(found == 1){
                        ptr.o      = NULL;
                        ptr.option = "manager";
                        ptr.value  = managerObjectId;
                        uci_del_list(ctx,&ptr);
                        ret = 0;
                        //reduce num
                        struct uci_ptr ptr1 = { .p = p, .s = router};
                        sprintf(managernum,"%d",atoi(managernum) - 1);
                        uciSetValue(ctx,&ptr1,"managernum",managernum);
                        //save
                        uci_save(ctx,p);
                        uci_commit(ctx,&p,false);
                    }
                }
            }
        }
        uci_unload(ctx,p);
    }
    uci_free_context(ctx);
    return ret;
}

int32_t getRouterManagerList(int32_t *num,struct SsstObjectId **objects)
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
            struct uci_option *option = uci_lookup_option(ctx,router,"manager");
            if(option != NULL && option->type == UCI_TYPE_LIST){
                char managernum[4] = "";
                if((getRoutersValueFromSection(ctx,router,"managernum",managernum) == 0) && (atoi(managernum) >= 0))
                {
                    int32_t count = atoi(managernum);
                    *num = count;
                    if(count > 0){
                        *objects = (struct SsstObjectId *)malloc(count * sizeof(struct SsstObjectId));
                        memset((void *)(*objects), 0 , count * sizeof(struct SsstObjectId));
                        int counter = 0;
                        struct uci_element *e;
                        uci_foreach_element(&option->v.list, e) {
                            strcpy((*objects)[counter].objectId, e->name);
                            counter++;
                            if(counter == count) break;
                        }
                    }
                    ret = 0;
                }
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

void insertUnFinishedInvitMsg(char *tag,char *destUserId,char *phone,char *destUserName)
{
    if(createUciRouterSection("invitmessage",tag) == 0){
        struct uci_context *ctx = uci_alloc_context();
        struct uci_package *p = NULL;
        uci_set_confdir(ctx, "/etc/config");
        if(uci_load(ctx, "siserver", &p) == UCI_OK)
        {
            struct uci_section *router = uci_lookup_section(ctx, p, tag);
            if(router != NULL){
                struct uci_ptr ptr = { .p = p, .s = router};
                uciSetValue(ctx,&ptr,"destUserId",destUserId);
                uciSetValue(ctx,&ptr,"phone",phone);
                uciSetValue(ctx,&ptr,"destUserName",destUserName);
                //save
                uci_save(ctx,p);
                uci_commit(ctx,&p,false);
            }
            uci_unload(ctx,p);
        }
        uci_free_context(ctx);
    }

}

int32_t checkUnFinishedInvitMsg(char *tag,char *destUserId,char *phone,char *destUserName,int32_t delete)
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
            if(delete == 1){
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
        }
        uci_unload(ctx,p);
    }
    uci_free_context(ctx);
    return ret;
}

int32_t deleteUnFinishedInvitMsg(char *tag,char *destUserId,char *phone,char *destUserName)
{
    return checkUnFinishedInvitMsg(tag,destUserId,phone,destUserName,1);
}

void deleteTagMsg(char *userid, char *phonenumber){
    LOG("-----enter deleteTagMsg");
    struct uci_context *ctx = uci_alloc_context();
    struct uci_package *p = NULL;
    char *tag[100];
    int i = 0;
    uci_set_confdir(ctx, "/etc/config");
    uci_reset_typelist();
    if(uci_load(ctx, "siserver", &p) == UCI_OK)
    {
        struct uci_element *e;
        uci_foreach_element( &p->sections, e) {
            struct uci_section *s = uci_to_section(e);
            char *destUserId = uci_lookup_option_string(ctx, s, "destUserId");
            char *phone = uci_lookup_option_string(ctx, s, "phone");
            if(destUserId && strcmp(destUserId, userid) == 0){
                tag[i] = (char*)malloc(10*sizeof(char));
                strcpy(tag[i],s->e.name);
                i++;
            }
            if(phone && strcmp(phone, phonenumber) == 0){
                tag[i] = (char*)malloc(10*sizeof(char));
                strcpy(tag[i],s->e.name);
                i++;
            }
        }
    }
    uci_reset_typelist();
    uci_unload(ctx,p);
    uci_free_context(ctx);
    int n;
    for(n =0; n < i; n++){
        LOG("------delete tag[%d] = %s",n,tag[n]);
        deleteUnFinishedInvitMsg(tag[n],NULL,NULL,NULL);
        free(tag[n]);
    }
    LOG("-----leave deleteTagMsg");
}

const char nixio__bin2hex[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

void bin2hex(void *src,int length,char *hex)
{
    char *data = (char *)src;
    int i;
    for (i = 0; i < length; i++) {
        hex[2*i]   = nixio__bin2hex[(data[i] & 0xf0) >> 4];
        hex[2*i+1] = nixio__bin2hex[(data[i] & 0x0f)];
    }
}

void createUniqeTag(char *tag)
{
    int fd;
    unsigned long seed = 0;
    fd = open("/dev/urandom", 0);
    if (fd < 0 || read(fd, &seed, sizeof(seed)) < 0)
    {
        LOG("Could not load seed from /dev/urandom: %s",strerror(errno));
        seed = time(0);
    }
    if (fd >= 0) close(fd);
    bin2hex(&seed,sizeof(seed),tag);
}
