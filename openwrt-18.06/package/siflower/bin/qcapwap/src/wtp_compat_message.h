#ifndef _WTP_COMPAT_MESSAGE_H_
#define _WTP_COMPAT_MESSAGE_H_

#include "wtp.h"

#define LOCATION "In the wall"
#define WTP_NAME "86AP"

#define PHY_NO_STANDARD 0
#define PHY_STANDARD_A 2
#define PHY_STANDARD_B 1
#define PHY_STANDARD_G 4
#define PHY_STANDARD_N 8
#define PHY_ALL_STANDARD 15

int cwmsg_assemble_wtp_ipv4_addr(struct cw_ctrlmsg *msg);
int cwmsg_assemble_radio_info(struct cw_ctrlmsg *msg);
int cwmsg_assemble_frame_tunnel_mode(struct cw_ctrlmsg *msg);
int cwmsg_send_configure_request(struct capwap_ap *ap);

#endif // _WTP_COMPAT_MESSAGE_H_
