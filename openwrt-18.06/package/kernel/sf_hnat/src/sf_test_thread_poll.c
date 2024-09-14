#include "sf_test_thread_poll.h"
#include "sf_pcap_parse.h"
#include "udp_pkt.h"
#include "tcp_pkt.h"
#include "err_pkt.h"
#include <linux/workqueue.h>
#include <linux/mutex.h>
#include <net/tcp.h>
#include <net/udp.h>
#include <asm/checksum.h>
#include "sf_hnat_debug.h"

#define MAX_THREAD_NUM 100

#define ADJUST_PERIOD 40

//default pppoe hdr
//extern char sfax8_gmac_test_xmit(struct sf_test_unit * ptest_unit);
static int sf_gmac_test_tx_thread(void *data);

#define SF_TEST_LIST_ADD 0
#define SF_TEST_LIST_DEL 1

void print_tx_data(u64 total_pkt_num, u32 tick, u32 pkt_len, u32 pre_fix, u16 key){
	u32 band_width = Divided_64(total_pkt_num * pkt_len * 8, 1024);
	band_width = band_width *  CONFIG_HZ / tick ;
	if ( tick == CONFIG_HZ )
	  printk("key[%d] %ds - %ds  band_width %uKbps process pkt %llu  data %llu\n",
				 key,  pre_fix, pre_fix + tick/CONFIG_HZ, band_width,total_pkt_num,total_pkt_num * pkt_len);
	else
	  printk("key[%d] %ds - %ds  band_width %uKbps process pkt %llu  data %llu\n",
				 key, pre_fix, pre_fix + (tick + CONFIG_HZ/2)/CONFIG_HZ , band_width,total_pkt_num,total_pkt_num * pkt_len);

}


void sf_test_tool_tx_timer_handler(unsigned long data){
	struct sf_test_unit *ptest_unit = (struct sf_test_unit *)data;
	struct timer_list* pop_timer = &ptest_unit->tx_timer;
	u64 now_jiffies = 0;
	u32 pkt_send = 0;
	u32 next_timeout = 0;

	ptest_unit->tx_last_timer_jiffies += CONFIG_HZ;
	next_timeout  = ptest_unit->tx_last_timer_jiffies + CONFIG_HZ;
	// printk("last_timer jiffies %lu jiffies %lu next_time %lu\n", ptest_unit->tx_last_timer_jiffies, jiffies, next_timeout);
	// cal second data
	pkt_send = ptest_unit->tx_pkt_count - ptest_unit->tx_last_pkt_count;
	ptest_unit->tx_last_pkt_count += pkt_send;

	print_tx_data(pkt_send , CONFIG_HZ, ptest_unit->tx_pkt_len , ptest_unit->tx_timer_index++, ptest_unit->key);

	now_jiffies = get_jiffies_64();
	if(now_jiffies > ptest_unit->tx_target_jiffies || ptest_unit->end_tx_jiffies != 0){
		//cal totol data ;
		if(ptest_unit->end_tx_jiffies == 0)
			mdelay(jiffies_to_msecs(ADJUST_PERIOD));

		print_tx_data(ptest_unit->tx_pkt_count ,ptest_unit->end_tx_jiffies - ptest_unit->start_tx_jiffies, ptest_unit->tx_pkt_len , 0, ptest_unit->key);
		del_timer(pop_timer);
		ptest_unit->tx_timer_release = 1;
	}
	else{
		mod_timer(pop_timer, next_timeout);
		// del_timer(pop_timer);
		// pop_timer->expires  = next_timeout;
		// add_timer(pop_timer);
	}
	return;
}

int sf_test_tool_init_tx_timer(struct sf_test_unit * ptest_unit){
	struct timer_list* pop_timer = NULL;
	pop_timer = &ptest_unit->tx_timer;
	ptest_unit->tx_last_timer_jiffies = jiffies;
	ptest_unit->tx_last_pkt_count = 0;
	ptest_unit->tx_timer_index = 0;

	init_timer(pop_timer);
	pop_timer->data     = (unsigned long)ptest_unit;
	pop_timer->function = sf_test_tool_tx_timer_handler;
	pop_timer->expires  = ptest_unit->tx_last_timer_jiffies +  CONFIG_HZ;
	add_timer(pop_timer);
	return 0;
}
void sf_test_list_config(struct sf_test_unit* ptest_unit, struct sf_test_tool_priv *ptest_priv, int type){
	mutex_lock(&ptest_priv->list_lock);
	if(type == SF_TEST_LIST_DEL)
		list_del(&ptest_unit->list_unit);
	if(type == SF_TEST_LIST_ADD)
		list_add_tail(&ptest_unit->list_unit, &ptest_priv->pt_pool->test_list_hdr);
	mutex_unlock(&ptest_priv->list_lock);
}

void sf_thread_pool_init(struct sf_test_tool_priv *ptest_priv){
	ptest_priv->pt_pool = (struct sf_thread_pool *)vmalloc(sizeof(struct sf_thread_pool));
	ptest_priv->pt_pool->sf_test_thread_num = 0;
	INIT_LIST_HEAD(&ptest_priv->pt_pool->test_list_hdr);
	ptest_priv->pt_pool->pmanager_wq = create_workqueue("manager_wq");
	mutex_init(&ptest_priv->tx_lock);
	mutex_init(&ptest_priv->list_lock);
}


static void start_work_tx_thread( struct work_struct *work ){
	struct sf_tx_work *ptx_work = (struct sf_tx_work *)work;
	struct sf_test_unit* ptest_unit = ptx_work->ptest_unit;
	char buf[50] = {0};
	sprintf(buf,"test%d", ptest_unit->key);
	// timer setup must not int tx thead , otherwise timer would not arrive on time
	sf_test_tool_init_tx_timer(ptest_unit);
	ptest_unit->ptx_task = kthread_run(sf_gmac_test_tx_thread, ptest_unit, buf);

	return;
}
// queue by thread exit or deinit and stop
static void stop_work_tx_thread( struct work_struct *work ){
	struct sf_tx_work *ptx_work = (struct  sf_tx_work *)work;
	struct sf_test_unit* ptest_unit = ptx_work->ptest_unit;
	struct sf_frag_pkt_set *pfrag_set = ptest_unit->ptest_priv->frag_set;

	printk("key[%d] =============tx pkt send %lld stop count %d\n ", ptest_unit->key,ptest_unit->tx_pkt_count, ptest_unit->ptest_priv->g_test_tx_pause_count);

	if(ptest_unit->ptx_task != NULL && ptest_unit->tx_thread_end == 0){

	// printk(" stop count2 \n " );
		kthread_stop(ptest_unit->ptx_task);
		msleep(25);
		while(unlikely(ptest_unit->tx_thread_end != 1)){
			msleep(25);
		}
	}
	if(ptest_unit->is_full_test == 1)
		dma_free_coherent(ptest_unit->ptest_priv->hnat_priv->dev, (ptest_unit->tx_pkt_len*1024), ptest_unit->dma_tx_data ,ptest_unit->dma_tx_data_phy);
	else if(pfrag_set->is_frag)
		dma_free_coherent(ptest_unit->ptest_priv->hnat_priv->dev, (ptest_unit->tx_pkt_len * pfrag_set->frag_no * pfrag_set->ip_id_no), ptest_unit->dma_tx_data, ptest_unit->dma_tx_data_phy);
	else
		dma_free_coherent(ptest_unit->ptest_priv->hnat_priv->dev, ptest_unit->tx_pkt_len, ptest_unit->dma_tx_data ,ptest_unit->dma_tx_data_phy);
	// printk("key[%d] start jiffies %lld end jiffies %lld\n", ptest_unit->key,ptest_unit->start_tx_jiffies, ptest_unit->end_tx_jiffies);
	sf_test_list_config(ptest_unit, ptest_unit->ptest_priv, SF_TEST_LIST_DEL);
//	list_del(&ptest_unit->list_unit);
	vfree(ptest_unit->ptx_work);
	vfree(ptest_unit->ptool_para);
	ptest_unit->ptest_priv->g_start_test_tx--;
	ptest_unit->ptest_priv->pt_pool->sf_test_thread_num--;
	while(ptest_unit->tx_timer_release == 0){
		msleep(50);
	}

	vfree(ptest_unit);
	return;
}

void sf_thread_pool_end_test(struct sf_test_tool_priv *ptest_priv, u16 key){
	struct sf_test_unit * p_tmp_unit;
	struct list_head *pos, *n;
	list_for_each_safe(pos, n, &ptest_priv->pt_pool->test_list_hdr) {
		p_tmp_unit = list_entry(pos, struct sf_test_unit, list_unit);
		if(key == p_tmp_unit->key){
		  stop_work_tx_thread((struct work_struct *)p_tmp_unit->ptx_work);
		}
	}
	return ;
}

void sf_thread_pool_deinit(struct sf_test_tool_priv *ptest_priv){

	struct list_head *pos, *n;
	struct sf_test_unit * p_tmp_unit;
	//TODO: stop all test thread first
	//
	flush_workqueue(ptest_priv->pt_pool->pmanager_wq);
	destroy_workqueue(ptest_priv->pt_pool->pmanager_wq);

	list_for_each_safe(pos, n, &ptest_priv->pt_pool->test_list_hdr) {
		p_tmp_unit = list_entry(pos, struct sf_test_unit, list_unit);
		stop_work_tx_thread((struct work_struct *)p_tmp_unit->ptx_work);
	}
	vfree(ptest_priv->pt_pool);
}

void sfax8_insert_vlan_tag(char * pkt , u16 vlan_id){
	struct vlan_ethhdr * v_hdr =  (struct vlan_ethhdr *) pkt;
	v_hdr->h_vlan_proto = htons(ETH_P_8021Q);
	v_hdr->h_vlan_TCI = htons(vlan_id & 0xFFF);
	v_hdr->h_vlan_encapsulated_proto = htons(ETH_P_IP);
	return;
}

void sfax8_insert_pppoe_hdr(char * pkt, u32 ip_pkt_len, u16 pppoe_sid){
	unsigned short * tmp = (unsigned short *)pkt;
	struct pppoe_ses_hdr * p_hdr;
	pkt += 2;
	p_hdr =  (struct pppoe_ses_hdr* ) pkt;
	*tmp = htons(ETH_P_PPP_SES);
	p_hdr->hdr.ver = 1;
	p_hdr->hdr.type = 1;
	p_hdr->hdr.code = 0;
	p_hdr->hdr.sid = htons(pppoe_sid);
	p_hdr->hdr.length = htons(ip_pkt_len + 2);
	p_hdr->proto = htons(PPP_IP);
	return;
}
#define SET_FRAG_FIRST 1
#define SET_FRAG_MID 2
#define SET_FRAG_LAST 3
#define SET_FRAG_MORE_FRAGS 0x2000
void sfax8_set_frag_info(struct iphdr* pip_hdr, int type, int frag_offset){
	switch(type){
		case SET_FRAG_FIRST:
			pip_hdr->frag_off = htons(SET_FRAG_MORE_FRAGS);
			break;
		case SET_FRAG_MID:
			pip_hdr->frag_off = htons(SET_FRAG_MORE_FRAGS) | htons(frag_offset/8);
			break;
		case SET_FRAG_LAST:
			pip_hdr->frag_off = htons(frag_offset/8);
			break;
		default:
			return;
	}
	return;
}
/*start of full entry test*/
int sf_thread_test_send_full(struct sf_test_tool_priv *ptest_priv, struct sf_test_tool_para * ptool_para){
	unsigned int ori_pkt_len = 0;
	unsigned char* p_pkt = 0;
	struct sf_test_unit* ptest_unit = NULL;
	struct sf_test_tool_set* ptest_set = NULL;
	struct sf_tx_work* ptx_work = NULL;
	struct iphdr* pip_hdr = NULL;
	unsigned short* psport = NULL;
	unsigned short* pdport = NULL;
	unsigned char * index_dma_tx_data;
	unsigned char * last_dma_tx_data;
	u8 pip_hdr_offset = 20;
	int i = 0;
	if(ptest_priv->pt_pool->sf_test_thread_num >= MAX_THREAD_NUM){
		printk("thread pool is full\n");
		return 0;
	}
	p_pkt  = udp_lan_wan_pkt;

	ptest_unit= (struct sf_test_unit * )vmalloc(sizeof(struct sf_test_unit));
	if(!ptest_unit){
		goto malloc_error;
	}
	memset(ptest_unit, 0, sizeof(struct sf_test_unit));
	ptx_work= (struct sf_tx_work * )vmalloc(sizeof(struct sf_tx_work));
	if(!ptx_work){
		goto malloc_error;
	}
	memset(ptx_work, 0, sizeof(struct sf_tx_work));

	ptest_unit->key = jiffies & 0xFFFF;
	ptest_unit->is_full_test = 1;

	// printk("key is  %d \n",ptest_unit->key);
	ptx_work->ptest_unit = ptest_unit;
	ptest_unit->ptx_work = ptx_work;
	ptest_unit->pmanager_wq = ptest_priv->pt_pool->pmanager_wq;
	ptest_unit->ptest_priv = ptest_priv;
	ptest_unit->ptool_para = ptool_para;
	ptest_unit->ptest_set = &ptest_priv->ptest_sets[0];
	ptest_set = ptest_unit->ptest_set;
	ptest_unit->tx_thread_end = 0;

//	list_add_tail(&ptest_unit->list_unit, &ptest_priv->pt_pool->test_list_hdr);
	sf_test_list_config(ptest_unit, ptest_priv, SF_TEST_LIST_ADD);
	ptest_priv->pt_pool->sf_test_thread_num++;
	ptest_priv->g_start_test_tx++;
// init test unit

	ori_pkt_len = sizeof(udp_lan_wan_pkt);

	if(ptool_para->mtu != 0)
		ptest_unit->tx_pkt_len = ptool_para->mtu;
	else
		ptest_unit->tx_pkt_len = ori_pkt_len;

	// printk("key[%d] tx_pkt_len malloc %d", ptest_unit->key, ptest_unit->tx_pkt_len);
	ptest_unit->dma_tx_data = dma_alloc_coherent(ptest_priv->hnat_priv->dev,
				(ptest_unit->tx_pkt_len*1024),
				&ptest_unit->dma_tx_data_phy, GFP_KERNEL);
	if (!ptest_unit->dma_tx_data){
		goto malloc_error;
	}

	memset(ptest_unit->dma_tx_data, 0, (ptest_unit->tx_pkt_len*1024));
	last_dma_tx_data = p_pkt;
	index_dma_tx_data = ptest_unit->dma_tx_data;
	memcpy(index_dma_tx_data , last_dma_tx_data , ptest_unit->tx_pkt_len);
	last_dma_tx_data = index_dma_tx_data;

	for(i = 1;i < 1024;i++){
		index_dma_tx_data = (ptest_unit->dma_tx_data + i*ptest_unit->tx_pkt_len);
		memcpy(index_dma_tx_data , last_dma_tx_data , ptest_unit->tx_pkt_len);
		last_dma_tx_data = index_dma_tx_data;

		pip_hdr = (struct iphdr *)(index_dma_tx_data + ETH_HLEN );
		if((i > 2) && (i % 2 == 0) )
			pip_hdr->saddr = htonl(ntohl(pip_hdr->saddr) + 1);
		pip_hdr_offset = pip_hdr->ihl << 2;
		psport =  (unsigned short *)((char* )pip_hdr + pip_hdr_offset);
		pdport =  (unsigned short *)((char* )pip_hdr + pip_hdr_offset + 2);
		*psport = htons(htons(*psport) + 1);
		*pdport = htons(htons(*pdport) + 1);
	}

	INIT_WORK( (struct work_struct *)ptx_work,  start_work_tx_thread);
	queue_work(ptest_unit->pmanager_wq, (struct work_struct *)ptx_work);
	return 0;

malloc_error:

	if(ptool_para)
	  vfree(ptool_para);

	if(ptest_unit)
	  vfree(ptest_unit);

	if(ptx_work)
	  vfree(ptx_work);
	return 0;
}

/*end of full entry test*/

/*start of send special pkt*/
int sf_thread_test_select_pkt(struct sf_test_tool_priv *ptest_priv,struct sf_test_tool_para * ptool_para, u8 select_pkt_index){
	unsigned int ori_pkt_len = 0;
	unsigned char* p_pkt = 0;
	struct sf_test_unit* ptest_unit = NULL;
	struct sf_test_tool_set* ptest_set = NULL;
	struct sf_tx_work* ptx_work = NULL;
	if(ptest_priv->pt_pool->sf_test_thread_num >= MAX_THREAD_NUM){
		printk("thread pool is full\n");
		return 0;
	}

	switch(select_pkt_index){
		case 0:
			printk("send ipv6 pkt\n");
			p_pkt  = ipv6_pkt;
			ptool_para->mtu = sizeof(ipv6_pkt);
			break;
		case 1:
			printk("send short pkt\n");
			p_pkt  = short_pkt;
			ptool_para->mtu = sizeof(short_pkt);
			break;
		case 2:
			printk("send long pkt\n");
			p_pkt  = long_pkt;
			ptool_para->mtu = sizeof(long_pkt);
			break;
		case 3:
			printk("send err pkt\n");
			p_pkt  = err_pkt;
			ptool_para->mtu = sizeof(err_pkt);
			break;
		default:
			break;
	}

	ptest_unit= (struct sf_test_unit * )vmalloc(sizeof(struct sf_test_unit));
	if(!ptest_unit){
		goto malloc_error;
	}
	memset(ptest_unit, 0, sizeof(struct sf_test_unit));
	ptx_work= (struct sf_tx_work * )vmalloc(sizeof(struct sf_tx_work));
	if(!ptx_work){
		goto malloc_error;
	}
	memset(ptx_work, 0, sizeof(struct sf_tx_work));

	ptest_unit->key = jiffies & 0xFFFF;

	// printk("key is  %d \n",ptest_unit->key);
	ptx_work->ptest_unit = ptest_unit;
	ptest_unit->ptx_work = ptx_work;
	ptest_unit->pmanager_wq = ptest_priv->pt_pool->pmanager_wq;
	ptest_unit->ptest_priv = ptest_priv;
	ptest_unit->ptool_para = ptool_para;
	ptest_unit->ptest_set = &ptest_priv->ptest_sets[0];
	ptest_set = ptest_unit->ptest_set;
	ptest_unit->tx_thread_end = 0;

//	list_add_tail(&ptest_unit->list_unit, &ptest_priv->pt_pool->test_list_hdr);
	sf_test_list_config(ptest_unit, ptest_priv, SF_TEST_LIST_ADD);
	ptest_priv->pt_pool->sf_test_thread_num++;
	ptest_priv->g_start_test_tx++;
// init test unit

	ori_pkt_len = sizeof(ipv6_pkt);

	ptest_unit->tx_pkt_len = ptool_para->mtu;

	// printk("key[%d] tx_pkt_len malloc %d", ptest_unit->key, ptest_unit->tx_pkt_len);
	ptest_unit->dma_tx_data = dma_alloc_coherent(ptest_priv->hnat_priv->dev,
				ptest_unit->tx_pkt_len,
				&ptest_unit->dma_tx_data_phy, GFP_KERNEL);
	if (!ptest_unit->dma_tx_data){
		goto malloc_error;
	}

	memset(ptest_unit->dma_tx_data, 0, ptest_unit->tx_pkt_len);
	memcpy(ptest_unit->dma_tx_data, p_pkt, ptest_unit->tx_pkt_len);


	INIT_WORK( (struct work_struct *)ptx_work,  start_work_tx_thread);
	queue_work(ptest_unit->pmanager_wq, (struct work_struct *)ptx_work);
	return 0;

malloc_error:

	if(ptool_para)
	  vfree(ptool_para);

	if(ptest_unit)
	  vfree(ptest_unit);

	if(ptx_work)
	  vfree(ptx_work);
	return 0;

}
/*end of send special pkt*/

void sf_thread_pool_start_test(struct sf_test_tool_priv *ptest_priv, u8 test_index, u8 is_wan_lan, u8 is_udp, struct sf_test_tool_para * ptool_para){
	unsigned short use_vlan = 0;
	unsigned int ip_pkt_len = 0;
	unsigned char ip_hdr_offset = 0, vlan_offset = 0;
	unsigned char l4_hdr_len = 0;
	unsigned int l4_len = 0;
	unsigned int cpy_pkt_len = 0, ori_pkt_len = 0;
	unsigned char* p_pkt = 0;
	unsigned char* p_wan_lan_hdr = 0;
	struct sf_test_unit* ptest_unit = NULL;
	struct sf_test_tool_set* ptest_set = NULL;
	struct sf_tx_work* ptx_work = NULL;
	struct sf_cap_file_data *pcap_file = NULL;
	struct iphdr* pip_hdr = NULL;
	struct udphdr * pudp_hdr = NULL;
	struct tcphdr * ptcp_hdr = NULL;
	unsigned short* psport = NULL;
	unsigned short* pdport = NULL;
	struct sf_frag_pkt_set *pfrag_set = ptest_priv->frag_set;
	u8 pip_hdr_offset = 20;
	int i,j;
	unsigned char * index_dma_tx_data;
	unsigned char * last_dma_tx_data;
	int data_len = 0;
	if(ptest_priv->pt_pool->sf_test_thread_num >= MAX_THREAD_NUM){
		printk("thread pool is full\n");
		return;
	}

	ptest_unit= (struct sf_test_unit * )vmalloc(sizeof(struct sf_test_unit));
	if(!ptest_unit){
		printk("alloc test_unit fail  with test index %d\n",test_index);
		goto malloc_error;
	}
	memset(ptest_unit, 0, sizeof(struct sf_test_unit));
	ptx_work= (struct sf_tx_work * )vmalloc(sizeof(struct sf_tx_work));
	if(!ptx_work){
		printk("alloc work fail  with test index %d \n",test_index);
		goto malloc_error;
	}
	memset(ptx_work, 0, sizeof(struct sf_tx_work));

	ptest_unit->key = jiffies & 0xFFFF;

	// printk("key is  %d \n",ptest_unit->key);
	ptx_work->ptest_unit = ptest_unit;
	ptest_unit->ptx_work = ptx_work;
	ptest_unit->pmanager_wq = ptest_priv->pt_pool->pmanager_wq;
	ptest_unit->ptest_priv = ptest_priv;
	ptest_unit->ptool_para = ptool_para;
	ptest_unit->ptest_set = &ptest_priv->ptest_sets[test_index];
	ptest_set = ptest_unit->ptest_set;
	ptest_unit->tx_thread_end = 0;

//	list_add_tail(&ptest_unit->list_unit, &ptest_priv->pt_pool->test_list_hdr);
	sf_test_list_config(ptest_unit, ptest_priv, SF_TEST_LIST_ADD);
	ptest_priv->pt_pool->sf_test_thread_num++;
	ptest_priv->g_start_test_tx++;
// init test unit

	if(ptest_set->valid == 1){
		if(ptest_set->proto == 1)
		  is_udp = 1;
		else
		  is_udp = 0;
	}

	if(is_udp){
		ori_pkt_len = sizeof(udp_lan_wan_pkt);
		l4_hdr_len = sizeof(udp_wan_lan_pkt_hdr);
		p_pkt  = udp_lan_wan_pkt;
		p_wan_lan_hdr = udp_wan_lan_pkt_hdr;
	}
	else{
		ori_pkt_len = sizeof(tcp_lan_wan_pkt);
		l4_hdr_len = sizeof(tcp_wan_lan_pkt_hdr);
		p_pkt  = tcp_lan_wan_pkt;
		p_wan_lan_hdr = tcp_wan_lan_pkt_hdr;
	}

	if((!is_wan_lan && ptest_set->src_vlan != 0)){
		use_vlan = ptest_set->src_vlan;
	}
	if((is_wan_lan && ptest_set->dest_vlan != 0)){
		use_vlan = ptest_set->dest_vlan;
	}
	if(ptest_set->pcap_file != NULL){
		pcap_file = ptest_set->pcap_file;
		ptest_unit->tx_pkt_len = (pcap_file->buf_read_len - pcap_file->cap_offset);
	}
	else {
		if(ptool_para->mtu != 0)
		  ptest_unit->tx_pkt_len = ptool_para->mtu;
		else
		  ptest_unit->tx_pkt_len = ori_pkt_len;

		// printk("key[%d] mtu %d",ptest_unit->key, ptool_para->mtu);
		// printk("key[%d] ori_pkt_len %d", ptest_unit->key,ori_pkt_len);
		// printk("key[%d] tx_pkt_len %d", ptest_unit->key,ptest_unit->tx_pkt_len);

		ip_pkt_len = ptest_unit->tx_pkt_len- ETH_HLEN;

		cpy_pkt_len = ori_pkt_len > ptest_unit->tx_pkt_len ? ptest_unit->tx_pkt_len : ori_pkt_len;
		if(use_vlan != 0){
			ptest_unit->tx_pkt_len += 4;
			ip_hdr_offset += 4;
		}
		if( ptest_set->ppp_sid != 0){
			ptest_unit->tx_pkt_len += 8;
			ip_hdr_offset += 8;
		}
	}

	if(pfrag_set->is_frag){
			ptest_unit->dma_tx_data = dma_alloc_coherent(ptest_priv->hnat_priv->dev,
					(ptest_unit->tx_pkt_len * pfrag_set->frag_no *pfrag_set->ip_id_no),
					&ptest_unit->dma_tx_data_phy, GFP_KERNEL);
			if (!ptest_unit->dma_tx_data){
				printk("dma alloc fail with test index %d thread num %d\n",test_index, ptest_priv->pt_pool->sf_test_thread_num);
				goto malloc_error;
			}
		memset(ptest_unit->dma_tx_data, 0, ptest_unit->tx_pkt_len);
	}else{
		ptest_unit->dma_tx_data = dma_alloc_coherent(ptest_priv->hnat_priv->dev,
				ptest_unit->tx_pkt_len,
				&ptest_unit->dma_tx_data_phy, GFP_KERNEL);
		if (!ptest_unit->dma_tx_data){
			printk("dma alloc fail with test index %d thread num %d\n",test_index, ptest_priv->pt_pool->sf_test_thread_num);
			goto malloc_error;
		}
		memset(ptest_unit->dma_tx_data, 0, ptest_unit->tx_pkt_len);

	}

	pip_hdr = (struct iphdr *)(ptest_unit->dma_tx_data + ETH_HLEN + ip_hdr_offset);
	ptest_unit->ip_hdr_offset = ip_hdr_offset;
	// insert vlan tag here
	// start send parse pkt or build a pkt with para
	// prepare test_data
	if(pcap_file != NULL){
		memcpy(ptest_unit->dma_tx_data, pcap_file->pmalloc_buf + pcap_file->cap_offset, ptest_unit->tx_pkt_len);
		printk("key[%d] pkt size is %d  use pkt file\n ", ptest_unit->key,ptest_unit->tx_pkt_len);
	}
	else{
		memcpy(ptest_unit->dma_tx_data, p_pkt, cpy_pkt_len);

		// printk("key[%d] cpy_pkt_len %d p_pkt%p\n", ptest_unit->key, cpy_pkt_len, p_pkt);
		if(is_wan_lan){
			memcpy(ptest_unit->dma_tx_data, p_wan_lan_hdr, l4_hdr_len);
		}
		if(use_vlan){
			memmove(ptest_unit->dma_tx_data+18, ptest_unit->dma_tx_data + 14, ip_pkt_len);
			sfax8_insert_vlan_tag(ptest_unit->dma_tx_data, use_vlan );
			vlan_offset = 4;
		}
		if(ptest_set->ppp_sid){
			memmove(ptest_unit->dma_tx_data+22 + vlan_offset, ptest_unit->dma_tx_data + 14 + vlan_offset, ip_pkt_len);
			// modify eth proto type and pppoe hdr
			sfax8_insert_pppoe_hdr(ptest_unit->dma_tx_data + 12 + vlan_offset,ip_pkt_len,ptest_unit->ptest_set->ppp_sid);
		}

		pip_hdr_offset = pip_hdr->ihl << 2;
		psport =  (unsigned short *)((char* )pip_hdr + pip_hdr_offset);
		pdport =  (unsigned short *)((char* )pip_hdr + pip_hdr_offset + 2);

// use test unit to set hdr
		if(ptest_unit->ptest_set->valid){
			if(is_wan_lan){
				memcpy(ptest_unit->dma_tx_data, ptest_set->dest_mac, ETH_ALEN);
				memcpy(ptest_unit->dma_tx_data, ptest_set->router_dest_mac, ETH_ALEN);
				pip_hdr->saddr = ptest_set->dip;
				pip_hdr->daddr = ptest_set->router_ip;
				*psport = htons(ptest_set->dport);
				*pdport = htons(ptest_set->router_port);
			}
			else{
				memcpy(ptest_unit->dma_tx_data, ptest_set->src_mac, ETH_ALEN);
				memcpy(ptest_unit->dma_tx_data, ptest_set->router_src_mac, ETH_ALEN);
				pip_hdr->saddr = ptest_set->sip;
				pip_hdr->daddr = ptest_set->dip;
				*psport = htons(ptest_set->sport);
				*pdport = htons(ptest_set->dport);
			}
		}

		pip_hdr->tot_len = htons(ip_pkt_len);
		data_len = 1472;
		pip_hdr->check = 0;
		pip_hdr->check = ip_fast_csum (pip_hdr, pip_hdr->ihl);
		l4_len = ip_pkt_len - pip_hdr_offset;

		// printk("key[%d] pkt ip checksum 0x%x, pip_hdr_offset%d \n ", ptest_unit->key,pip_hdr->check, pip_hdr_offset);
		// printk("key[%d] l4_len %d, ip_pkt_len %d \n ", ptest_unit->key,l4_len, ip_pkt_len);
		if(is_udp){
			pudp_hdr = (struct udphdr *)(pip_hdr + pip_hdr_offset);
			pudp_hdr->check = 0;
			pudp_hdr->check = udp_v4_check(l4_len ,pip_hdr->saddr, pip_hdr->daddr,csum_partial((char *)pudp_hdr, l4_len, 0));;
			printk("key[%d] pkt udp checksum 0x%x \n ", ptest_unit->key,pip_hdr->check);
		} else{
			ptcp_hdr = (struct tcphdr *)((char*)pip_hdr + pip_hdr_offset);
			ptcp_hdr->check = 0;
			ptcp_hdr->check = tcp_v4_check(l4_len, pip_hdr->saddr, pip_hdr->daddr, csum_partial((char *)ptcp_hdr, l4_len, 0));
			printk("key[%d] pkt tcp checksum 0x%x \n ", ptest_unit->key,pip_hdr->check);
		}
		printk("data len = %d\n", data_len );
		if(pfrag_set->is_frag){
			index_dma_tx_data = ptest_unit->dma_tx_data + ptest_unit->tx_pkt_len;
			last_dma_tx_data = ptest_unit->dma_tx_data;
			for(i = 0; i < pfrag_set->ip_id_no; i++){
				if(i == 0){
					pip_hdr = (struct iphdr *)(last_dma_tx_data + ETH_HLEN + ip_hdr_offset);
					sfax8_set_frag_info(pip_hdr, SET_FRAG_FIRST, 0);
					for(j = 1; j < pfrag_set->frag_no; j++){
						memcpy(index_dma_tx_data , last_dma_tx_data , ptest_unit->tx_pkt_len);
						pip_hdr = (struct iphdr *)(index_dma_tx_data + ETH_HLEN + ip_hdr_offset);
						if(j == pfrag_set->frag_no - 1){
							sfax8_set_frag_info(pip_hdr, SET_FRAG_LAST, data_len*j);
						}else
							sfax8_set_frag_info(pip_hdr, SET_FRAG_MID, data_len*j);
						last_dma_tx_data = index_dma_tx_data ;
						index_dma_tx_data += ptest_unit->tx_pkt_len;
					}
				}else{
					for(j = 0; j < pfrag_set->frag_no; j++){
						memcpy(index_dma_tx_data , last_dma_tx_data , ptest_unit->tx_pkt_len);
						pip_hdr = (struct iphdr *)(index_dma_tx_data + ETH_HLEN + ip_hdr_offset);
						if(j == 0){
							pip_hdr->id = htons(ntohs(pip_hdr->id) + 1);
							sfax8_set_frag_info(pip_hdr, SET_FRAG_FIRST, 0);
						}else if(j == pfrag_set->frag_no - 1){
							sfax8_set_frag_info(pip_hdr, SET_FRAG_LAST, data_len*j);
						}else{
							sfax8_set_frag_info(pip_hdr, SET_FRAG_MID, data_len*j);
						}
						last_dma_tx_data = index_dma_tx_data;
						index_dma_tx_data += ptest_unit->tx_pkt_len;
					}
				}
			}

		}
		printk("key[%d] pkt size is %d  use vlan %u is_wan_lan %u  pppoe 0x%x\n ", ptest_unit->key,ptest_unit->tx_pkt_len, use_vlan, is_wan_lan, ptest_set->ppp_sid );
	}

	INIT_WORK( (struct work_struct *)ptx_work,  start_work_tx_thread);
	queue_work(ptest_unit->pmanager_wq, (struct work_struct *)ptx_work);
	return ;

malloc_error:

	if(ptool_para)
	  vfree(ptool_para);

	if(ptest_unit)
	  vfree(ptest_unit);

	if(ptx_work)
	  vfree(ptx_work);

	return ;
}


// almost 27000 tx pause with 0 10 945 send
#define DELAY_UNIT 60
#define DELAY_FOR_TX_PAUSE 200
#define DELAY_FOR_TX_COM  10

static unsigned int pkt_send_per_period(u32 pkt_num, u32 delay_us, struct sf_test_unit * ptest_unit, u32 start_jiffies, u32 max_us,struct mutex *ptx_lock){
	u32 i = 0;
	struct sf_test_tool_priv * ptest_priv = ptest_unit->ptest_priv;
	// u32 wait_count = 0;
	u32 delay_us_adjust = 0;
	u32 delay_us_total = delay_us * DELAY_UNIT;
	int delay_us_new = 0;
	unsigned long target_jiffies = start_jiffies + ADJUST_PERIOD;
	//printk("here %s %d\n",__FUNCTION__ ,__LINE__);
	unsigned short rest_space = 1;
//TODO: for tx complete use
	ptest_priv->rest_space = rest_space;

	while((i < pkt_num) &&  time_before(jiffies , target_jiffies)){
		//printk("here %s %d i = %d pkt_num = %d\n",__FUNCTION__ ,__LINE__, i , pkt_num);
		//printk("here %s %d tx_pause = %d\n",__FUNCTION__ ,__LINE__, ptest_unit->ptest_priv->is_tx_pause);
		mutex_lock(ptx_lock);
		while(atomic_read(&ptest_unit->ptest_priv->is_tx_pause) == 1){
			udelay(DELAY_FOR_TX_PAUSE);
			// delay_us_adjust+= 30;
			delay_us_adjust++;
			// continue;
			// wait_count++;

			//TODO: still got tx stopped because not do tx complete
			if(delay_us_adjust > DELAY_FOR_TX_COM){
				ptest_priv->driver_tx_complete(ptest_priv->driver_priv, rest_space, &ptest_priv->is_tx_pause);
			}
		}
		ptest_priv->driver_xmit(ptest_priv->driver_priv,
					(ptest_unit->dma_tx_data_phy),
					ptest_unit->tx_pkt_len,
					rest_space,
					&ptest_priv->is_tx_pause,
					&ptest_priv->g_test_tx_pause_count
					);

		mutex_unlock(ptx_lock);
		if(pkt_num < DELAY_UNIT){
			usleep_range(delay_us/2, delay_us);
		}
		else{
			if((i+1) % DELAY_UNIT == 0 ){
				delay_us_new = delay_us_total;
				if(delay_us_adjust){
					delay_us_adjust *= DELAY_FOR_TX_PAUSE;
					delay_us_new -= delay_us_adjust;
					delay_us_adjust = 0;
				}
				// for  release cpu
				if(delay_us_new > 0){
					if(delay_us_new > 200){
						delay_us_new  = delay_us_new < max_us ? delay_us_new : max_us;
						usleep_range(delay_us_new/2, delay_us_new);
					}
					else
					  udelay(delay_us_new);
				}
			}
		}
		i++;
	}
	//printk("here %s %d\n",__FUNCTION__ ,__LINE__);
	ptest_unit->tx_pkt_count+=i;
	return i;
}

static unsigned int pkt_send_per_period_full(u32 pkt_num, u32 delay_us, struct sf_test_unit * ptest_unit, u32 start_jiffies, u32 max_us,struct mutex *ptx_lock){
	u32 i = 0;
	// u32 wait_count = 0;
	u32 delay_us_adjust = 0;
	u32 delay_us_total = delay_us * DELAY_UNIT;
	int delay_us_new = 0;
	unsigned long target_jiffies = start_jiffies + ADJUST_PERIOD;
	struct sf_test_tool_priv *ptest_priv = ptest_unit->ptest_priv;
	struct sf_frag_pkt_set *pfrag_set = ptest_priv->frag_set;
	unsigned short rest_space = 1;

	//printk("here %s %d\n",__FUNCTION__ ,__LINE__);
	if(pfrag_set->is_continus)
		rest_space = ptest_unit->max_pkt_index + 1;


//TODO: for tx complete use
	ptest_priv->rest_space = rest_space;

	while((i < pkt_num) &&  time_before(jiffies , target_jiffies)){
		mutex_lock(ptx_lock);
		while(atomic_read(&ptest_priv->is_tx_pause) == 1){
			udelay(DELAY_FOR_TX_PAUSE);
			// delay_us_adjust+= 30;
			delay_us_adjust++;
			// continue;
			// wait_count++;
			//TODO: still got tx stopped because not do tx complete
			if(delay_us_adjust > DELAY_FOR_TX_COM){
				ptest_priv->driver_tx_complete(ptest_priv->driver_priv, rest_space, &ptest_priv->is_tx_pause);
			}
		}
		if(pfrag_set->is_continus){
			for(;ptest_unit->pkt_index <= ptest_unit->max_pkt_index;ptest_unit->pkt_index++){
				ptest_priv->driver_xmit(ptest_priv->driver_priv,
							(ptest_unit->dma_tx_data_phy + ptest_unit->tx_pkt_len * ptest_unit->pkt_index),
							ptest_unit->tx_pkt_len,
							rest_space,
							&ptest_priv->is_tx_pause,
							&ptest_priv->g_test_tx_pause_count
							);
			}
		}//in multi thread test, avoid out of order
		else
		  ptest_priv->driver_xmit(ptest_priv->driver_priv,
					  (ptest_unit->dma_tx_data_phy + ptest_unit->tx_pkt_len * ptest_unit->pkt_index),
					  ptest_unit->tx_pkt_len,
					  rest_space,
					  &ptest_priv->is_tx_pause,
					  &ptest_priv->g_test_tx_pause_count
					  );

		mutex_unlock(ptx_lock);
		if(pkt_num < DELAY_UNIT){
			usleep_range(delay_us/2, delay_us);
		}
		else{
			if((i+1) % DELAY_UNIT == 0 ){
				delay_us_new = delay_us_total;
				if(delay_us_adjust){
					delay_us_adjust *= DELAY_FOR_TX_PAUSE;
					delay_us_new -= delay_us_adjust;
					delay_us_adjust = 0;
				}
				// for  release cpu
				if(delay_us_new > 0){
					if(delay_us_new > 200){
						delay_us_new  = delay_us_new < max_us ? delay_us_new : max_us;
						usleep_range(delay_us_new/2, delay_us_new);
					}
					else
					  udelay(delay_us_new);
				}
			}
		}

		if(pfrag_set->is_continus){
			i += ptest_unit->pkt_index;
		}
		else{
			ptest_unit->pkt_index++;
			i++;
		}
		if(ptest_unit->pkt_index > ptest_unit->max_pkt_index)
			ptest_unit->pkt_index = 0;
	}
	ptest_unit->tx_pkt_count+=i;
	return i;
}

#define DEFAULT_PKT_SEND_US 12
static int sf_gmac_test_tx_thread(void *data) {
	u64 last_jiffies_64 = 0;// first send time
	u32 used_tick ;// first send time
	u64 remain_tick ;// first send time
	u64 remain_pkt_to_send;
	u32 pps = 0;
	u32 pkt_send_period = 0;
	u32 pkt_send = 0;
	u32 delay_us = 0;
	// almost 10us to send a pkt
	int delay_us_adjust = DEFAULT_PKT_SEND_US;
	u32 send_count_period = 0;
	u64 total_pkt_to_send = 0;
	u32 peroid_ms = jiffies_to_msecs(ADJUST_PERIOD);
	u32 peroid_us = jiffies_to_usecs(ADJUST_PERIOD);
	struct sf_test_unit *ptest_unit = (struct sf_test_unit *)data;
	struct sf_test_tool_para * ptool_para = ptest_unit->ptool_para;
	struct mutex *ptx_lock = &ptest_unit->ptest_priv->tx_lock;
	struct sf_frag_pkt_set *pfrag_set = ptest_unit->ptest_priv->frag_set;

	//cannot cal directly because 32 bit pc overflow be careful
	u64 total_bits_to_send = ptool_para->band_width * ptool_para->time;
	total_bits_to_send = total_bits_to_send * 1024 * 1024;

	total_pkt_to_send = Divided_64(total_bits_to_send  ,(ptest_unit->tx_pkt_len * 8)) + 1;
	// printk("send time %d \n",ptool_para->time);


	pps = Divided_64(total_pkt_to_send ,ptool_para->time);
	//in this case CONFIG_HZ 100  10 pkt miss
	pkt_send_period = Divided_64(total_pkt_to_send * ADJUST_PERIOD  , (CONFIG_HZ * ptool_para->time));

	//100 pkt miss , delay 10 us for first round
	delay_us = Divided_64(ptool_para->time * 1000000 , total_pkt_to_send);

	printk("key[%d] bandwidth %d time %d \n",ptest_unit->key,ptool_para->band_width , ptool_para->time);
	printk("key[%d] total_bits_to_send %lld \n",ptest_unit->key,total_bits_to_send);
	printk("key[%d] total_pkt_to_send %lld \n",ptest_unit->key,total_pkt_to_send);
	// printk("key[%d] pkt_send_period  %d \n",ptest_unit->key,pkt_send_period);
	// printk("key[%d] delay_us %d \n",ptest_unit->key,delay_us);
	ptest_unit->tx_pkt_count = 0;
	remain_tick  = ptool_para->time * CONFIG_HZ;

	// sf_gmac_init_tx_timer(ptest_unit);
	ptest_unit->pkt_index = 0;
	ptest_unit->start_tx_jiffies = get_jiffies_64();
	last_jiffies_64  = ptest_unit->start_tx_jiffies;// first send time
	ptest_unit->tx_target_jiffies  = ptest_unit->start_tx_jiffies  + remain_tick;
	remain_pkt_to_send = total_pkt_to_send;
	// printk("start jiffies %lld target jiffies %lld", ptest_unit->start_tx_jiffies, ptest_unit->tx_target_jiffies);
	// sync timer with send  thread
	ptest_unit->tx_last_timer_jiffies = (u32)ptest_unit->start_tx_jiffies;
	while(last_jiffies_64 <= ptest_unit->tx_target_jiffies && !kthread_should_stop()){
		send_count_period = 0;
		if(delay_us > delay_us_adjust){
			delay_us -= delay_us_adjust;
		}else{
			delay_us = 0;
		}
		// for multi thread test
		if(delay_us > peroid_us){
			delay_us = peroid_us;
		}

		// for multi thread test
		if(pkt_send_period == 0){
			msleep(peroid_ms);
		}
		else{
			pkt_send = pkt_send_period  < remain_pkt_to_send ? pkt_send_period :  remain_pkt_to_send;
/*			if(mutex_lock_interruptible(ptx_lock) != 0){

				printk("key[%d] mutex return because signal\n", ptest_unit->key);
				break;
			}*/
			if(ptest_unit->is_full_test){
				ptest_unit->max_pkt_index = 1023;
				send_count_period = pkt_send_per_period_full(pkt_send , delay_us, ptest_unit,  (u32)last_jiffies_64, peroid_us, ptx_lock);
			}else if(pfrag_set->is_frag){
				ptest_unit->max_pkt_index = pfrag_set->frag_no * pfrag_set->ip_id_no - 1;
				send_count_period = pkt_send_per_period_full(pkt_send , delay_us, ptest_unit,  (u32)last_jiffies_64, peroid_us, ptx_lock);
			}else{
				send_count_period = pkt_send_per_period(pkt_send , delay_us, ptest_unit,  (u32)last_jiffies_64, peroid_us, ptx_lock);
//			mutex_unlock(ptx_lock);
			}
		}
		used_tick = CIRC_CNT(jiffies,(u32)last_jiffies_64,0xffffffff);
		last_jiffies_64 += used_tick;
		remain_tick -= used_tick;
		if(remain_tick <= 0){
			break;
		}

		if(remain_pkt_to_send < (remain_pkt_to_send - send_count_period)){
			break;
		}
		remain_pkt_to_send -= send_count_period;
		if(remain_pkt_to_send == 0){
			break;
		}

// can not / directly when delay may be -
		if(likely(pkt_send == send_count_period) ){
			delay_us_adjust = (used_tick * 1000000 - delay_us * send_count_period * CONFIG_HZ);
			delay_us_adjust = delay_us_adjust /  (send_count_period * CONFIG_HZ);
		}else if(send_count_period != 0){
			delay_us_adjust = (used_tick * 1000000 / send_count_period - delay_us * CONFIG_HZ);
			delay_us_adjust = delay_us_adjust / CONFIG_HZ;
		}else{
			delay_us_adjust = 0x7FFFFFFF;
		}

		pkt_send_period = Divided_64(remain_pkt_to_send * ADJUST_PERIOD , remain_tick);
		delay_us = Divided_64(remain_tick  * 1000000 , (CONFIG_HZ * remain_pkt_to_send));

		// printk("send_count_period  %u used_tick %u remain_tick %llu, remain_pkt_to_send %llu  \n" , send_count_period ,used_tick , remain_tick , remain_pkt_to_send );
		// printk("delay_us_adjust  %d pkt_send_period %u, delay_us %u\n", delay_us_adjust, pkt_send_period, delay_us);
	}

	// printk("key[%d] delay_us %u delay_us_adjust %d \n",ptest_unit->key,delay_us,delay_us_adjust);
	ptest_unit->end_tx_jiffies = get_jiffies_64();
	if(!kthread_should_stop()){
		// printk("key[%d] stop here \n",ptest_unit->key);
		INIT_WORK( (struct work_struct *)ptest_unit->ptx_work, stop_work_tx_thread);
		queue_work(ptest_unit->pmanager_wq, (struct work_struct *)ptest_unit->ptx_work);
	}
	while (!kthread_should_stop()) {
		msleep(50);
	}
	// printk("key[%d] stop here3 \n",ptest_unit->key);
	ptest_unit->tx_thread_end = 1;
	return 0;
}
