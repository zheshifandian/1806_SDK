/*
 * =====================================================================================
 *
 *       Filename:  bind.h
 *
 *    Description:  when user want to control this router, first
                    he/she must make a binding process, this file is describe how we do the binding in details:
                    Callers: syncservice receive a socket message:BIND xxxxx              xxxxx is the user's object id
                    Actions:
                    1,create router table
                    2,create router state table , bind to the router table
                    3,create wireless table, bind to the router table
                    4,create device info table , bind to the router table
                    5,create router sub table, bind to the router table
                    6,bind the users table to the router table
                    7,save the router table's unique table id, free some resources or clean up all resource and tables if failed
 *
 *        Version:  1.0
 *        Created:  07/21/2015 08:58:24 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  franklin(), franklin.wang@siflower.com.cn
 *        Company:  Siflower Communication Tenology Co.,Ltd
 *
 * =====================================================================================
 */


#ifndef __SOCK_SERVER_BIND_H__
#define __SOCK_SERVER_BIND_H__
/*
func:
    bind this router to some users
params:
    char *data, bind message data, generally will be the userobjectid
    char **ret, returned string, when user want to get a result from us, user should release this point after use it!!
return:
 */
void do_bind(char *data, char **ret);

/*
func:unbind the router to some users(managers or binder)
params:
return:
 */
void do_unbind(char *data, char **ret );

int32_t do_unbind_internal(char *router_object,char *result,int32_t notify_binder);

/*
func:do manager operation
params:
return:
 */
void do_manager_op(char *args, char **callback);

void do_test();

int32_t doAddRouterUser(char *args, char **callback);
#endif
