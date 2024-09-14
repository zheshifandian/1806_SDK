#include "sf_intel7084_qos_api.h"
#include "gsw_sw_init.h"

extern ethsw_api_dev_t *pedev0[GSW_DEV_MAX];
extern gsw_lowlevel_fkts_t flow_fkt_tbl;

// extern void sf_qos_driver_register(sf_switch_qos_driver *driver);
// extern void sf_qos_driver_unregister(void);
int sf_gsw_queue_shaper_assign(u8 shaper,u8 queue,u8 type,u32 speed);

#define SF_GSW_PORT_LIST	0x2f//0-3 for wan/lan, 5 for host

//can add meter cfg for ingress speed limit here
int sf_gsw_qos_port_meter_set(sf_switch_qos_acl_rule *entry, sf_qos_rule_cfg_t type){
	u8 port;
	u32 speed;

	port = entry->pattern.d_port_id;
	speed = entry->action.speed;

//	printk("%s start\n",__FUNCTION__);
//	printk("speed %d kbps\n",speed);
	if(type == SF_QOS_RULE_DEL)
		sf_gsw_queue_shaper_assign(port, (port*7), 0,speed);//shaper 0~3 to port 0~3
	else
		sf_gsw_queue_shaper_assign(port, (port*7), 1,speed);//shaper 0~3 to port 0~3

	if(type == SF_QOS_RULE_DEL)
		return SF_QOS_RET_DEL_ENTRY_OK;

	return SF_QOS_RET_ADD_ENTRY_OK;
}
/*
//enable or disable all 8 meters
static int sf_gsw_port_meter_cfg(u8 enable){
	int i;
	GSW_QoS_meterCfg_t parm;
	GSW_QoS_meterCfg_t *p_parm = &parm;

	for(i = 0;i < 8;i++){
		p_parm->nMeterId = i;
		p_parm->bEnable = enable;
		GSW_QoS_MeterCfgSet(pedev0[0],
				p_parm);
	}

	return 0;
}*/

int sf_gsw_port_meter_enable(void){
	//sf_gsw_port_meter_cfg(1);
	return 0;
}

int sf_gsw_port_meter_disable(void){
	//sf_gsw_port_meter_cfg(0);
	return 0;
}

/* //if we use meter to limit speed ,this is necessary
static int sf_gsw_port_meter_init(void){
	GSW_QoS_WRED_Cfg_t wred;

	sf_gsw_port_meter_enable();

	GSW_QoS_WredCfgGet(pedev0[0], &wred);//set wred to enable meter
	wred.nRed_Min = 0;
	wred.nRed_Max = 0;
	GSW_QoS_WredCfgSet(pedev0[0], &wred);

	return 0;
}*/

int sf_gsw_port_cfg(GSW_QoS_ClassSelect_t sel_mode){
	GSW_QoS_portCfg_t parm;
	GSW_QoS_portCfg_t *p_parm = &parm;
	u8 port;

	p_parm->eClassMode = sel_mode;
	p_parm->nTrafficClass = 0;
	for(port = 0;port < 7;port++){
		p_parm->nPortId = port;
		GSW_QoS_PortCfgSet(pedev0[0],
				p_parm);
	}

	return 0;
}

GSW_QoS_Scheduler_t	sch_global_algo = GSW_QOS_SCHEDULER_STRICT;
int sf_gsw_qos_acl_cfg(sf_switch_qos_acl_rule *entry, sf_qos_rule_cfg_t type){
	int i;
	u32 tc;
	u8 pcp,dscp;
	GSW_QoS_PCP_ClassCfg_t pcp_cfg;
	GSW_QoS_DSCP_ClassCfg_t dscp_cfg;
	GSW_QoS_schedulerCfg_t sch_cfg;
	_sf_switch_qos_pattern_bits *p_patt_e = &entry->pattern.pattern_e.patt_e_bits;
	//_sf_switch_qos_action_bits *p_act_e = &entry->action.action_e.act_e_bits;
	sf_qos_sch_t *p_sch = &entry->action.sch;

	//if(p_sch->queue == 0)
	//	return SF_QOS_RET_ADD_ENTRY_OK;

	/*classify*/
	if(type == SF_QOS_RULE_DEL)
		tc = 0; //delete, set to default queue 0
	else
		tc = p_sch->queue;

	if(p_patt_e->pcp_enable){
		pcp = entry->pattern.pcp;
		pcp_cfg.nPCP = pcp;
		pcp_cfg.nTrafficClass[pcp] = tc;
		printk("pcp = 0x%x, tc =%d\n", pcp, tc);
		GSW_QoS_PCP_ClassSet(pedev0[0],
				&pcp_cfg);
	}else if(p_patt_e->dscp_enable){
		dscp = entry->pattern.dscp;
		dscp_cfg.nDSCP = dscp;
		dscp_cfg.nTrafficClass[dscp] = tc;
		printk("dscp = 0x%x, tc =%d\n", dscp, tc);
		GSW_QoS_DSCP_ClassSet(pedev0[0],
				&dscp_cfg);
	}

	if(type == SF_QOS_RULE_DEL)
		return SF_QOS_RET_DEL_ENTRY_OK;

	/*sch*/
	for(i = 0;i < 4;i++){
		sch_cfg.nQueueId = i*7 + tc;
		sch_cfg.eType = sch_global_algo;
		sch_cfg.nWeight = p_sch->weight * 1000;
		GSW_QoS_SchedulerCfgSet(pedev0[0],
				&sch_cfg);
	}
/*	for(i = 0;i < 32;i++){
		sch_cfg.nQueueId = i;
		GSW_QoS_SchedulerCfgGet(pedev0[0],
				&sch_cfg);
		printk("queue %d, type %d, weight %d\n", i, sch_cfg.eType, sch_cfg.nWeight);
	}*/
	return SF_QOS_RET_ADD_ENTRY_OK;
}


int sf_gsw_pcp_enable(void){
	printk("pcp enable\n");
	sf_gsw_port_cfg(GSW_QOS_CLASS_SELECT_PCP);
	return 0;
}

int sf_gsw_portcfg_disable(void){
	sf_gsw_port_cfg(GSW_QOS_CLASS_SELECT_NO);
	return 0;
}

int sf_gsw_dscp_enable(void){
	printk("dscp enable\n");
	sf_gsw_port_cfg(GSW_QOS_CLASS_SELECT_DSCP);
	return 0;
}

//now we use shaper for egress port speed limit
//if we need ingress port speed limit, we can use meter
sf_switch_qos_common_list gsw_qos_port_shaper = {
	.pattern_e.patt_e = SF_QOS_PATTERN_D_PORT_ID_ENABLE_BIT,
	.action_e.act_e = SF_QOS_ACTION_SPEED_ENABLE_BIT,
	.next = NULL,
	.head_rule = NULL,
	.max_entry_num = 8,
	.status = SF_QOS_LIST_EMPTY,
	.enable = sf_gsw_port_meter_enable,
	.disable = sf_gsw_port_meter_disable,
	.set_rule = sf_gsw_qos_port_meter_set,
};

sf_switch_qos_common_list gsw_qos_pcp = {
	.pattern_e.patt_e = SF_QOS_PATTERN_PCP_ENABLE_BIT,
	.action_e.act_e = SF_QOS_ACTION_SCH_ENABLE_BIT,
	.next = &gsw_qos_port_shaper,
	.head_rule = NULL,
	.max_entry_num = 8,
	.status = SF_QOS_LIST_EMPTY,
	.enable = sf_gsw_pcp_enable,
	.disable = sf_gsw_portcfg_disable,
	.set_rule = sf_gsw_qos_acl_cfg,
};

sf_switch_qos_common_list gsw_qos_dscp = {
	.pattern_e.patt_e = SF_QOS_PATTERN_DSCP_ENABLE_BIT,
	.action_e.act_e = SF_QOS_ACTION_SCH_ENABLE_BIT,
	.next = &gsw_qos_pcp,
	.head_rule = NULL,
	.max_entry_num = 8,
	.status = SF_QOS_LIST_EMPTY,
	.enable = sf_gsw_dscp_enable,
	.disable = sf_gsw_portcfg_disable,
	.set_rule = sf_gsw_qos_acl_cfg,
};

//print some information about the switch
int sf_gsw_help(void){
	printk(">>>switch type : intel7084\n");
	printk("qos description:\n");
	printk("tmp support dscp and pcp classsify\n");
	printk("can't enable both at the same time\n");
	printk("set dscp/pcp value to queue (0~6) and set WFQ/SP weight/priority\n");
	printk("\n");
	printk("set port egress speed limit by shaper\n");
	printk(">>>intel7084 switch qos end\n");
	return 0;
}


//set port 0~3 share all 28 queues, 7 queues for each port
int sf_gsw_queue_init(void){
	int port, tc, queue;
	GSW_QoS_queuePort_t q_port_cfg;

	for(port = 0;port < 4;port++){
		for(tc = 0;tc < 16;tc++){
			queue = port*7 + tc;
			if(tc > 6)
				queue = port*7;
			q_port_cfg.nPortId = port;
			q_port_cfg.nTrafficClassId = tc;
			q_port_cfg.nQueueId = queue;
			GSW_QoS_QueuePortSet(pedev0[0],
					&q_port_cfg);
		}
	}

	//alloc all tc of port 4,5,6 to same queue 29,30,31, avoid unkown error
	for(port = 4;port < 7;port++){
		queue = 25 + port;
		for(tc = 0;tc < 16;tc++){
			q_port_cfg.nPortId = port;
			q_port_cfg.nTrafficClassId = tc;
			q_port_cfg.nQueueId = queue;
			GSW_QoS_QueuePortSet(pedev0[0],
					&q_port_cfg);
		}
	}

/*	for(port = 0;port < 7;port++){
		for(tc =0;tc < 16;tc++){
			q_port_cfg.nPortId = port;
			q_port_cfg.nTrafficClassId = tc;
			GSW_QoS_QueuePortGet(pedev0[0],
					&q_port_cfg);
			printk("port %d tc %d mapto queue %d",
					port,tc,q_port_cfg.nQueueId);
		}
	}*/
	return 0;
}

int sf_gsw_qos_change_algo(sf_qos_algo_t algo){
	int i;
	GSW_QoS_schedulerCfg_t sch_cfg;

	sch_global_algo = algo;
	for(i = 0;i < 32;i++){
		sch_cfg.nQueueId = i;
		sch_cfg.eType = algo;
		if(algo)
			sch_cfg.nWeight = 1;
//		printk("queue = %d,type = %d, weight = %d \n",i, sch_cfg.eType, sch_cfg.nWeight);
		GSW_QoS_SchedulerCfgSet(pedev0[0],
				&sch_cfg);
	}
/*	for(i = 0;i < 32;i++){
		sch_cfg.nQueueId = i;
		GSW_QoS_SchedulerCfgGet(pedev0[0],
				&sch_cfg);
		printk("queue = %d,type = %d, weight = %d \n",i, sch_cfg.eType, sch_cfg.nWeight);
	}
*/
	return 0;
}

int sf_gsw_queue_shaper_assign(u8 shaper,u8 queue,u8 type, u32 speed){
	GSW_QoS_ShaperQueue_t parm;
	GSW_QoS_ShaperCfg_t shaper_cfg;
	int i;

	parm.nRateShaperId = shaper;
	for(i = 0;i < 7;i++){
		parm.nQueueId = queue;
		queue++;
		if(type){
			GSW_QoS_ShaperQueueAssign(pedev0[0],
					&parm);
		}else{
			GSW_QoS_ShaperQueueDeassign(pedev0[0],
					&parm);
		}
	}

	shaper_cfg.nRateShaperId = shaper;
	shaper_cfg.bEnable = 1;
	shaper_cfg.nCbs = 4000;
	shaper_cfg.nRate = speed;
	GSW_QoS_ShaperCfgSet(pedev0[0],
			&shaper_cfg);

/*	for(q = 0;q < 32;q++){
		parm2.nQueueId = q;
		GSW_QoS_ShaperQueueGet(pedev0[0],
				&parm2);
		printk("q %d,assign %d, shaper %d",q,parm2.bAssigned,parm2.nRateShaperId);
	}*/
	return 0;
}

int sf_gsw_qos_deinit(void){
	return 0;
}

int sf_gsw_qos_init(void){
	sf_gsw_queue_init();
	return 0;
}

sf_switch_qos_param gsw_qos_param = {
	.port_list = SF_GSW_PORT_LIST,
	.max_q_for_port = 7,//queue 0~6, queue 0 default
};

sf_switch_qos_driver gsw_qos_driver = {
	.init = sf_gsw_qos_init,
	.deinit = sf_gsw_qos_deinit,
	.change_algo = sf_gsw_qos_change_algo,
	.help = sf_gsw_help,
	.param = &gsw_qos_param,
	.list_head = &gsw_qos_dscp,
};
#if 0
int sf_intel7084_qos_unregister(void){
	sf_qos_driver_unregister();
	return 0;
}

int sf_intel7084_qos_register(void){
	sf_qos_driver_register(&gsw_qos_driver);
	return 0;
}
#endif
