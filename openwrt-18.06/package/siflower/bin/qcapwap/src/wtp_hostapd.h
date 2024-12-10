#ifndef _WTP_HOSTAPD_H_
#define _WTP_HOSTAPD_H_

#define HOSTAPD_GLOABLE_INTERFACE "/var/run/hostapd/hostapd"

int capwap_start_hostapd(struct capwap_ap *ap);
int wtp_handle_wlan_request(struct capwap_ap *ap, struct cw_ctrlmsg *msg);
void capwap_close_hostapd(struct capwap_ap *ap);
int wtp_handle_ap_roam(struct capwap_ap *ap, struct cw_ctrlmsg *msg);

#endif // _WTP_HOSTAPD_H_
