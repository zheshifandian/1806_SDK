#ifndef _SF_GSW_QOS_API_H_
#define _SF_GSW_QOS_API_H_

typedef enum{
	SF_GSW_RESRC_NOUSE = 0, //the resource is not used
	SF_GSW_RESRC_USE, //the resource is used
	SF_GSW_RESRC_USEALLP = 5, //the resource is used for all ports(0~3,5 here, total 5 ports)
}sf_gsw_resrc_use_t;

typedef struct {
	sf_gsw_resrc_use_t use_type;

	int tc;//the traffic class of entry use
	int queue[5];//the queue of tc use
	int shaper[5];//the shaper of queue use
}sf_gsw_entry_info_store_t;


int sf_intel7084_qos_register(void);
int sf_intel7084_qos_unregister(void);

#endif
