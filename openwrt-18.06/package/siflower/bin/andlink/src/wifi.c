#include <sys/time.h>
#include <stdlib.h>
#include "wifi.h"
#include "andlink.h"
#include "log.h"
#include "utils.h"

void wifi_control_thread(void *paras)
{
	struct andlink *andlink = (struct andlink *)paras;
	struct timespec abstime;
	struct timeval now;
	int ret, result;
	LOG("%s entry\n", __func__);
	pthread_mutex_lock(&andlink->lock);
	andlink->wifi_thread_inited = true;
	pthread_mutex_unlock(&andlink->lock);
	if (check_andlink_config() == 0) {
		LOG("%s device has registerted\n");
		andlink->change_wifi_state = true;
		andlink->wifi_mode = STATION;
	} else {
		andlink->change_wifi_state = true;
		andlink->wifi_mode = SOFT_AP;
	}

	while (1) {
		if (andlink->exit)
			break;
		//wait 15min 
		gettimeofday(&now, NULL);
		abstime.tv_nsec = now.tv_usec * 1000;
		abstime.tv_sec = now.tv_sec + SOFT_AP_LIMITTED_TIME;
		pthread_mutex_lock(&andlink->wifi_lock);
		if (andlink->change_wifi_state) {
			andlink->change_wifi_state = false;
			pthread_mutex_unlock(&andlink->wifi_lock);
			ret = 0;
		} else {
			pthread_mutex_unlock(&andlink->wifi_lock);
			pthread_mutex_lock(&andlink->lock);
			ret = pthread_cond_timedwait(&andlink->wifi_cond, &andlink->lock, &abstime);
			pthread_mutex_unlock(&andlink->lock);
		}

		if (ret != -ETIMEDOUT) {
			if (andlink->change_wifi_state)
				andlink->change_wifi_state = false;
			if (andlink->wifi_mode == QUICK_LINK) {
				pthread_mutex_lock(&andlink->wifi_lock);
				LOG("%s wifi will be set to quick link mode\n", __func__);
				//TODO control wifi to quick link mode
				// if quick link failed then change wifi in softap mode. 
				result = system("wifi_control.sh QUICK_LINK");
				if (andlink->change_wifi_state) {
					pthread_mutex_unlock(&andlink->wifi_lock);
					continue;
				}
				pthread_mutex_unlock(&andlink->wifi_lock);
				//if sucess
				if (!result) {
					LOG("Quick link connect success\n");
					if (andlink->wifi_connect_callback)
						andlink->wifi_connect_callback(andlink);
					continue;
				}
				//TODO if failed
				//entry softap mode
			} else if (andlink->wifi_mode == STATION) {
				//TODO
				result = system("wifi_control.sh STATION");
				continue;
			}
			pthread_mutex_lock(&andlink->wifi_lock);
			LOG("%s wifi will be set to soft ap mode\n", __func__);
			//TODO control wifi to softap mode
			result = system("wifi_control.sh SOFT_AP");
			andlink->wifi_mode = SOFT_AP;
			pthread_mutex_unlock(&andlink->wifi_lock);
		} else {
			if (andlink->wifi_mode == SOFT_AP) {
				LOG("%s wifi will be set to sleep mode\n", __func__);
				//TODO
				//maybe shutdown wifi due to being in soft ap mode over 15mins
				pthread_mutex_lock(&andlink->lock);
				andlink->wifi_mode = SLEEPED;
				pthread_mutex_unlock(&andlink->lock);
			}
			/*else QUICK_LINK do nothing*/
		}

	}
	LOG("%s exit\n", __func__);
}
