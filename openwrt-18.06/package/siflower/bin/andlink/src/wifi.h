#ifndef __WIFI__H
#define __WIFI__H


#define SOFT_AP_LIMITTED_TIME (15 * 60) //15 mins
/*
* when machines run in SOFT_AP mode, the machine is a ap and 
* it's ssid should be set to CMQLINK-${device_type}-mac and open
* access; when machine runs in QUICK_LINK mode, the machine is
* a station, and will try to connect hidden ssid CMCC-QLINK.
*/
enum wifi_mode {
	SOFT_AP,
	QUICK_LINK,
	STATION,
	SLEEPED,
};

void wifi_control_thread(void *paras);
#endif
