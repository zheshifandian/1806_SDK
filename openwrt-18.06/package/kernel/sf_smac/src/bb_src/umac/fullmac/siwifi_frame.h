#include "siwifi_defs.h"
#include <linux/netdevice.h>

#define SIWIFI_SKB_LENGTH                   256

#define SIWIFI_BEACON_SKB_LENGTH            SIWIFI_SKB_LENGTH
#define SIWIFI_BEACON_FRAME_LENGTH           225
#define SIWIFI_AUTH_SKB_LENGTH              SIWIFI_SKB_LENGTH
#define SIWIFI_AUTH_FRAME_LENGTH             30
#define SIWIFI_DEAUTH_SKB_LENGTH            SIWIFI_SKB_LENGTH
#define SIWIFI_DEAUTH_FRAME_LENGTH           26
#define SIWIFI_ASSOC_REQ_SKB_LENGTH         SIWIFI_SKB_LENGTH
#define SIWIFI_ASSOC_REQ_FRAME_LENGTH        152
#define SIWIFI_ASSOC_RSP_SKB_LENGTH         SIWIFI_SKB_LENGTH
#define SIWIFI_ASSOC_RSP_FRAME_LENGTH        168
#define SIWIFI_DISASSOC_SKB_LENGTH          SIWIFI_SKB_LENGTH
#define SIWIFI_DISASSOC_FRAME_LENGTH         26
#define SIWIFI_PROBE_RSP_SKB_LENGTH          SIWIFI_SKB_LENGTH
#define SIWIFI_PROBE_RSP_FRAME_LENGTH         223

#define SIWIFI_IE_SUPP_RATES_LEN                8
#define SIWIFI_IE_PWR_CAPABILITY_LEN            2
#define SIWIFI_IE_SUPPORTED_CHANNELS_LEN        2
#define SIWIFI_IE_EXT_CAPABILITY_LEN            8
#define SIWIFI_IE_SUPPORTED_REGULATORY_LEN     13  // assoc req
#define SIWIFI_IE_WMM_LEN                       7  // assoc req
#define SIWIFI_IE_HT_CAPABILITY_LEN            26
#define SIWIFI_IE_VHT_CAPABILITY_LEN           12
#define SIWIFI_IE_DSPS_LEN                      1
#define SIWIFI_IE_COUNTRY_LEN                  12
#define SIWIFI_IE_QBSS_LEN                      5
#define SIWIFI_IE_SUPPORTED_REGULATORY_LEN_2    2  // probe rsp
#define SIWIFI_IE_HT_OPERATION_LEN             22
#define SIWIFI_IE_EXT_CAPABILITY_LEN            8
#define SIWIFI_IE_VHT_OPERATION_LEN             5
#define SIWIFI_IE_VHT_TX_POWER_ENVELOPE_LEN     4
#define SIWIFI_IE_WMM_LEN_2                    24  // probe rsp
#define SIWIFI_IE_EXT_SUPP_RATES_LEN            4
#define SIWIFI_IE_LB_COUNTRY_LEN                6
#define SIWIFI_IE_ERP_INFORMATION_LEN           1
#define SIWIFI_IE_LB_SUPPORTED_REGULATORY_LEN   4

struct sk_buff *siwifi_create_beacon(struct siwifi_vif *siwifi_vif, uint8_t *add_mac);
struct sk_buff *siwifi_create_auth_req(struct siwifi_vif *siwifi_vif, uint8_t *add_mac);
struct sk_buff *siwifi_create_auth_rsp(struct siwifi_vif *siwifi_vif, uint8_t *add_mac);
struct sk_buff *siwifi_create_deauth(struct siwifi_vif *siwifi_vif, uint8_t *add_mac);
struct sk_buff *siwifi_create_assoc_req(struct siwifi_vif *siwifi_vif, uint8_t *add_mac);
struct sk_buff *siwifi_create_assoc_rsp(struct siwifi_vif *siwifi_vif, uint8_t *add_mac);
struct sk_buff *siwifi_create_probe_rsp(struct siwifi_vif *siwifi_vif, uint8_t *add_mac);

int siwifi_frame_send_mgmt(struct siwifi_vif *vif, struct sk_buff *skb);
