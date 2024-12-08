#include <stdio.h>
#include <stdlib.h>
#include "andlink.h"
#include "log.h"
#include "utils.h"
#include "wifi.h"

extern void coap_process(struct andlink *andlink);

/*
* check_device_registered : 
* check device registered status and wifi connection info,
* if device is not registered or wifi connection is missed, then do distribution network
* return 0 : device is registered
*		 1 : device is not registered
		negative number : some error accurred
* 
*/
int check_device_registered()
{
	return check_andlink_config();
}

static int andlink_init(struct andlink *andlink)
{
	int ret = 0;
	LOG("%s start\n", __func__);
	memset(andlink, 0, sizeof(*andlink));
	pthread_mutex_init(&andlink->lock, NULL);
	pthread_mutex_init(&andlink->wifi_lock, NULL);
	pthread_cond_init(&andlink->cond, NULL);
	pthread_cond_init(&andlink->listener_cond, NULL);
	pthread_cond_init(&andlink->wifi_cond, NULL);

	pthread_create(&andlink->listener, NULL, listener_thread, andlink);
	if (andlink->listener <= 0) {
		LOG("cann't create andlink listener thread\n");
		ret = -1;
		andlink->exit = true;
		goto exit;
	}
	//wait listener initialization completed
	pthread_mutex_lock(&andlink->lock);
	if (!andlink->listener_thread_inited)
		pthread_cond_wait(&andlink->listener_cond, &andlink->lock);
	pthread_mutex_unlock(&andlink->lock);

	pthread_create(&andlink->wifi, NULL, wifi_control_thread, andlink);
	if (andlink->wifi <= 0 ) {
		LOG("can't create andlink wifi control thread\n");
		ret = -1;
		goto exit_listener_thread;
	}
	while (!andlink->wifi_thread_inited)
		usleep(10 * 1000);
	return ret;

exit_listener_thread:
	if (andlink->listener > 0) {
		//exit andlink listener thread
		wakeup_exit_uloop(andlink);
		pthread_join(andlink->listener, NULL);
	}

exit:
	LOG("andlink init failed\n");
	return ret;
}

static void andlink_exit(struct andlink *andlink)
{
	LOG("andlink exit\n");
	pthread_mutex_lock(&andlink->lock);
	andlink->exit = true;
	pthread_mutex_unlock(&andlink->lock);
	if (andlink->listener > 0) {
		wakeup_exit_uloop(andlink);
		pthread_join(andlink->listener, NULL);
	}

	if (andlink->wifi > 0) {
		pthread_cond_signal(&andlink->wifi_cond);
		pthread_join(andlink->wifi, NULL);
	}
	pthread_cond_destroy(&andlink->cond);
	pthread_cond_destroy(&andlink->listener_cond);
	pthread_mutex_destroy(&andlink->lock);
	pthread_mutex_destroy(&andlink->wifi_lock);
}

void main(int argc, char *argv[])
{
	struct andlink andlink;
	int ret = 0;
	ret = andlink_init(&andlink);
	if (ret)
		return;
	coap_process(&andlink);
	andlink_exit(&andlink);
}
