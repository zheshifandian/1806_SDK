#ifndef _AC_MANAGER_H
#define _AC_MANAGER_H

#define WLAN_IFACE_ID	0

int ac_init_wtp_attr(struct capwap_wtp *wtp);

int ac_add_wlan(struct capwap_wtp *wtp);
int ac_set_dev_attr(struct capwap_wtp *wtp, bool changed);
int ac_reset_device(struct capwap_wtp *wtp);
int ac_do_wtp_command(struct capwap_wtp *wtp, const char *command);
int ac_update_wifi(struct capwap_wtp *wtp, int i);

void ac_modify_mac_list(struct capwap_wtp *wtp);
int capwap_wtp_send_mac_list(struct capwap_wtp *wtp);
int capwap_wtp_send_ap_roam_event(struct capwap_wtp *wtp, struct mac_event *event, int num);
void capwap_wtp_remove_from_mac_list(struct capwap_wtp *wtp);

#endif // _AC_MANAGER_H
