/*
* Description
*
* Copyright (C) 2016-2020 Qin.Xia <qin.xia@siflower.com.cn>
*
* Siflower software
*/

#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <uapi/linux/in.h>
#include <net/arp.h>
#include <net/netfilter/nf_conntrack_helper.h>
#include <linux/inetdevice.h>
#include "ts.h"


struct sf_ts_priv *g_ts_priv = NULL;
int ts_log = 0;

extern unsigned int sf_dev_ct_limit;
extern unsigned long sf_oom_drop_level;
extern int sf_add_dev_to_blacklist(u8 *mac);
extern int sf_del_dev_from_blacklist(u8 *mac);
#define	DB_LOG(fmt,...) if (ts_log) printk(fmt,##__VA_ARGS__)


static inline u32 sf_mac_hash(const unsigned char *mac)
{
	/* use 1 byte of OUI and 3 bytes of NIC */
	u32 key = get_unaligned((u32 *)(mac + 2));
	return jhash_1word(key, g_ts_priv->random_seed) & MAC_HASH_NUM;
}

static struct dev_info* check_dev_in_hlist(u8 *mac)
{
	struct dev_info *dev;
	u32 key;

	key = sf_mac_hash(mac);
	hlist_for_each_entry(dev, &g_ts_priv->devlist[key], snode){
		if(ether_addr_equal(mac, dev->mac)){
			return dev;
		}
	}

	return NULL;
}

static int sf_ts_show(struct seq_file *file, void *data)
{
	struct dev_info *dev;
	struct ts_info *rcu_c;
	u64 upload, download;
	u32 start, cpu;
	u16 i;
	u8 *mac;

	seq_printf(file, "mac                upload               download             upspeed    downspeed  maxups     maxdowns   ct_count\n");
	rcu_read_lock();
	for(i = 0; i < MAC_HASH_SIZE; i++){
		hlist_for_each_entry(dev, &g_ts_priv->devlist[i], snode){
			upload = 0;
			download = 0;
			mac = dev->mac;
			rcu_c = rcu_dereference(dev->c);
			for_each_possible_cpu(cpu) {
				struct ts_info *per_c = per_cpu_ptr(rcu_c, cpu);
				do{
					start = u64_stats_fetch_begin(&per_c->lock);
					upload += per_c->up_c;
					download += per_c->down_c;
				}while(u64_stats_fetch_retry(&per_c->lock, start));
			}
			seq_printf(file, "%02X:%02X:%02X:%02X:%02X:%02X  %-20llu %-20llu %-10u %-10u %-10u %-10u %u\n",
					mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], upload, download,
					dev->upload_s, dev->download_s, dev->m_upload_s, dev->m_download_s, dev->ct_count);
		}
	}
	rcu_read_unlock();
	return 0;
}

static int sf_ts_open(struct inode *inode, struct file *file){
	return single_open(file, sf_ts_show, inode->i_private);
}

static ssize_t sf_ts_write(struct file *file, const char __user *input,
		size_t size, loff_t *ofs)
{
	struct ts_info *new_c, *old_c;
	struct dev_info *dev, *newdev;
	int i, ret, cmd, key;
	u8 buf[128] = {0};
	u8 mac[ETH_ALEN] = {0};

	if (size > 128)
		return -ENOMEM;

	if (copy_from_user(buf, input, size))
		return -EFAULT;

	ret = sscanf(buf, "%u %2hhx:%2hhx:%2hhx:%2hhx:%2hhx:%2hhx",
			&cmd, &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);
	if (ret < 7){
		DB_LOG("CMD param error\n");
		goto done;
	}

	switch (cmd){
		case CMD_ADD_STS:
			dev = check_dev_in_hlist(mac);
			if (!dev){
				key = sf_mac_hash(mac);
				newdev = kzalloc(sizeof(struct dev_info), GFP_KERNEL);
				if (!newdev)
					break;

				newdev->c = sf_alloc_percpu_stats(struct ts_info);
				if(!newdev->c){
					kfree(newdev);
					break;
				}
				memcpy(newdev->mac, mac, ETH_ALEN);

				INIT_LIST_HEAD(&newdev->ct_list);
				spin_lock_init(&newdev->ct_lock);
				u64_stats_init(&newdev->total.lock);
				hlist_add_head_rcu(&newdev->snode, &g_ts_priv->devlist[key]);
			}
			break;

		case CMD_DEL_STS:
			dev = check_dev_in_hlist(mac);
			if (dev){
				hlist_del_rcu(&dev->snode);
				synchronize_rcu();
				free_percpu(dev->c);
				kfree(dev);
			}else{
				DB_LOG("CMD_DEL_STS: mac not found\n");
			}
			break;

		case CMD_ADD_BLK:
			sf_add_dev_to_blacklist(mac);
			DB_LOG("add mac:%pM to blacklist\n", mac);
			break;

		case CMD_DEL_BLK:
			sf_del_dev_from_blacklist(mac);
			DB_LOG("del mac:%pM from blacklist\n", mac);
			break;

		case CMD_RESET_TXRX:
			dev = check_dev_in_hlist(mac);
			if (dev){
				new_c = sf_alloc_percpu_stats(struct ts_info);
				if (!new_c)
					break;
				old_c = dev->c;
				rcu_assign_pointer(dev->c, new_c);
				synchronize_rcu();
				free_percpu(old_c);
				/*wait clear each cpu traffic, then clear total traffic*/
				dev->clear = 1;
			}else{
				DB_LOG("Reset txrx: not found\n");
			}
			break;

		case CMD_RESET_ALL_TXRX:
			for (i = 0; i < MAC_HASH_SIZE; i++){
				hlist_for_each_entry(dev, &g_ts_priv->devlist[i], snode){
					new_c = sf_alloc_percpu_stats(struct ts_info);
					if (!new_c)
						continue;
					old_c = dev->c;
					rcu_assign_pointer(dev->c, new_c);
					synchronize_rcu();
					free_percpu(old_c);
					dev->clear = 1;
				}
			}
			break;

		case CMD_LOG_ENABLE:
			if (ts_log)
				ts_log = 0;
			else
				ts_log = 1;
			break;

		default:
			break;
	}
done:
	return size;
}

static const struct file_operations sf_ts_ops = {
	.owner   = THIS_MODULE,
	.open    = sf_ts_open,
	.read    = seq_read,
	.write   = sf_ts_write,
	.llseek  = seq_lseek,
	.release = single_release,
};

static int update_dev_ct_info(struct dev_info *dev, struct sk_buff *skb)
{
	struct conn_info *entry, *next;
	struct nf_conn *ct;
	enum ip_conntrack_info ctinfo;
	unsigned long curr_time = jiffies;
	bool found = false;

	ct = nf_ct_get(skb, &ctinfo);
	if (unlikely(!ct))
		return NF_ACCEPT;

	if (unlikely(nfct_help(ct)))
		goto drop_check;

	spin_lock(&dev->ct_lock);
	list_for_each_entry_safe(entry, next, &dev->ct_list, list) {
		if (entry->ct == ct){
			found = true;
			entry->update_time = curr_time;
		}else {
			if ((curr_time - entry->update_time) > (10*HZ)) {
				list_del(&entry->list);
				kfree(entry);
				dev->ct_count--;
			}
		}
	}
	spin_unlock(&dev->ct_lock);

	if (dev->ct_count < sf_dev_ct_limit && !found) {
		entry = kzalloc(sizeof(*entry), GFP_ATOMIC);
		if (!entry) {
			printk("%s: no memory for kzalloc\n", __func__);
			return NF_ACCEPT;
		}

		entry->ct = ct;
		entry->update_time = curr_time;
		spin_lock(&dev->ct_lock);
		list_add_tail(&entry->list, &dev->ct_list);
		spin_unlock(&dev->ct_lock);
		dev->ct_count++;
		// make ct fast recycle while hash drop
		if (test_bit(SF_HASH_DROP, &sf_oom_drop_level))
			ct->timeout = nfct_time_stamp + 10*HZ;
		return NF_ACCEPT;
	}

drop_check:
	if (test_bit(SF_HASH_DROP, &sf_oom_drop_level) && !found) {
		// force recycle ct
		set_bit(IPS_DYING_BIT, &ct->status);
		return NF_DROP;
	}

	return NF_ACCEPT;
}

static unsigned int sf_nf_upload_hookfn(void *priv, struct sk_buff *skb,
		const struct nf_hook_state *st)
{
	struct dev_info *dev;
	struct ts_info *per_c;
	u32 key;
	u8 *mac;
	int ret = NF_ACCEPT;

	if (strncmp(st->in->name, "br-", 3) == 0) {
		mac = eth_hdr(skb)->h_source;
		key = sf_mac_hash(mac);
		rcu_read_lock();
		hlist_for_each_entry_rcu(dev, &g_ts_priv->devlist[key], snode){
			if (ether_addr_equal(mac, dev->mac))
			{
				if ((ret = update_dev_ct_info(dev, skb)) == NF_DROP)
					break;
				per_c = this_cpu_ptr(rcu_dereference(dev->c));
				u64_stats_update_begin(&per_c->lock);
				per_c->up_c += skb->len;
				u64_stats_update_end(&per_c->lock);

				/*set device alive*/
				if (!dev->alive)
					dev->alive = 1;
				break;
			}
		}
		rcu_read_unlock();
	}else if ((strncmp(st->in->name, "eth", 3) == 0)
			|| (strncmp(st->in->name, "ppp", 3) == 0)){
		skb->mark |= DMARK;
	}

	return ret;
}

int get_postrouting_dmac(struct sk_buff *skb, char *mac_addr)
{
	struct dst_entry *dst = skb_dst(skb);
	struct rtable *rt = (struct rtable *)dst;
	struct net_device *dev = dst->dev;
	struct neighbour *neigh;
	struct hh_cache *hh;
	unsigned seq;
	unsigned int hh_len, hh_alen;
	u32 nexthop;

	rcu_read_lock_bh();
	nexthop = (__force u32) rt_nexthop(rt, ip_hdr(skb)->daddr);
	neigh = __ipv4_neigh_lookup_noref(dev, nexthop);
	if (unlikely(!neigh))
		neigh = __neigh_create(&arp_tbl, &nexthop, dev, false);
	if (IS_ERR(neigh))
		return -EINVAL;
	hh = &neigh->hh;
	rcu_read_unlock_bh();

	if ((neigh->nud_state & NUD_CONNECTED) && hh->hh_len)
	{
		do {
			seq = read_seqbegin(&hh->hh_lock);
			hh_len = hh->hh_len;
			hh_alen = HH_DATA_ALIGN(hh_len);
			memcpy(mac_addr, (char*)&hh->hh_data[0] + hh_alen - hh_len, ETH_ALEN);
		} while (read_seqretry(&hh->hh_lock, seq));
	} else{
		if (neigh->nud_state & NUD_VALID) {
			memcpy(mac_addr, neigh->ha, ETH_ALEN);
		}
	}
	return 0;
}

static unsigned int sf_nf_download_hookfn(void *priv, struct sk_buff *skb,
		const struct nf_hook_state *st)
{
	struct dev_info *dev;
	struct ts_info *per_c;
	u32 key;
	u8 mac[6] = {0};
	int ret = NF_ACCEPT;

	if (skb->mark & DMARK){
		get_postrouting_dmac(skb, mac);
		key = sf_mac_hash(mac);
		rcu_read_lock();
		hlist_for_each_entry_rcu(dev, &g_ts_priv->devlist[key], snode){
			if(ether_addr_equal(mac, dev->mac))
			{
				if ((ret = update_dev_ct_info(dev, skb)) == NF_DROP)
					break;
				per_c = this_cpu_ptr(rcu_dereference(dev->c));
				u64_stats_update_begin(&per_c->lock);
				per_c->down_c += skb->len;
				u64_stats_update_end(&per_c->lock);

				/*set device alive*/
				if(!dev->alive)
					dev->alive = 1;

				break;
			}
		}
		rcu_read_unlock();
	}

	return ret;
}

static struct nf_hook_ops sf_nf_hook_ops[] __read_mostly = {
	{
		// upload statistic
		.hook           = sf_nf_upload_hookfn,
		.pf             = NFPROTO_IPV4,
		.hooknum        = NF_INET_FORWARD,
		.priority       = NF_UPLOAD_PRI_SF_TS,
	},
	{
		// download statistic
		.hook           = sf_nf_download_hookfn,
		.pf             = NFPROTO_IPV4,
		.hooknum        = NF_INET_POST_ROUTING,
		.priority       = NF_DOWNLOAD_PRI_SF_TS,
	},
};

static void update_ts_config(unsigned long data)
{
	struct sf_ts_priv *priv = (struct sf_ts_priv *)data;
	struct dev_info *dev;
	struct ts_info *rcu_c;
	u64 download, upload;
	u32 i, start, cpu;
	u32 hnat_count;

	rcu_read_lock();
	for (i = 0; i < MAC_HASH_SIZE; i++){
		hlist_for_each_entry_rcu(dev, &priv->devlist[i], snode){
			if (dev->alive)
			{
				upload = 0;
				download = 0;
				hnat_count = 0;
				rcu_c = rcu_dereference(dev->c);
				for_each_possible_cpu(cpu) {
					struct ts_info *per_c = per_cpu_ptr(rcu_c, cpu);
					do{
						start = u64_stats_fetch_begin(&per_c->lock);
						upload += per_c->up_c;
						download += per_c->down_c;
					}while(u64_stats_fetch_retry(&per_c->lock, start));
				}

#if IS_ENABLED(CONFIG_SFAX8_HNAT_DRIVER)
				if (g_ts_priv->phnat_priv->base) {
					hnat_count = g_ts_priv->phnat_priv->sts_cnt(g_ts_priv->hnat_pdev, dev->mac);
				}
#endif

				/*if set clear, clear total traffic*/
				if (unlikely(dev->clear))
				{
					u64_stats_update_begin(&dev->total.lock);
					dev->total.up_c = 0;
					dev->total.down_c = 0;
					u64_stats_update_end(&dev->total.lock);
					dev->clear = 0;
				}

				/*speed*/
				dev->upload_s = upload - dev->total.up_c;
				dev->download_s = download + (hnat_count * 1500) - dev->total.down_c;

				DB_LOG("update mac:%pM upload:%llu download:%llu hnat_count:%u total.up_c:%llu total.down_c:%llu upload speed %u, download speed %u\n",
						dev->mac, upload, download, hnat_count, dev->total.up_c, dev->total.down_c, dev->upload_s, dev->download_s);
				if (unlikely(dev->upload_s > dev->m_upload_s))
					dev->m_upload_s = dev->upload_s;

				if (unlikely(dev->download_s > dev->m_download_s))
					dev->m_download_s = dev->download_s;

				/*update total traffic*/
				u64_stats_update_begin(&dev->total.lock);
				dev->total.up_c = upload;
				dev->total.down_c = download;
				u64_stats_update_end(&dev->total.lock);

				/*reset device status*/
				dev->alive = 0;
			}
		}
	}

	priv->ts_timer.expires = jiffies + HZ;
	add_timer(&priv->ts_timer);
	rcu_read_unlock();
}

static int __init sf_ts_init(void)
{
	struct sf_ts_priv* ts_priv;
#if IS_ENABLED(CONFIG_SFAX8_HNAT_DRIVER)
	struct device * hnat_dev = NULL;
#endif
	int ret = 0;

	ts_priv = kzalloc(sizeof(struct sf_ts_priv), GFP_KERNEL);
	if (!ts_priv) {
		printk("no memory for ts priv data\n");
		return -ENOMEM;
	}

	g_ts_priv = ts_priv;
	hash_init(ts_priv->devlist);
	get_random_bytes(&ts_priv->random_seed, sizeof(u32));

	ret = nf_register_net_hooks(&init_net, sf_nf_hook_ops, ARRAY_SIZE(sf_nf_hook_ops));
	if (ret)
		goto err_out_free;

#if IS_ENABLED(CONFIG_SFAX8_HNAT_DRIVER)
	hnat_dev = bus_find_device_by_name(&platform_bus_type, NULL, "sf_hnat.0.auto");
	if (hnat_dev){
		ts_priv->hnat_pdev = to_platform_device(hnat_dev);
		ts_priv->phnat_priv = platform_get_drvdata(ts_priv->hnat_pdev);
		printk("############ found hnat device\n");
	}
#endif

	init_timer(&ts_priv->ts_timer);
	ts_priv->ts_timer.function = &update_ts_config;
	ts_priv->ts_timer.data =(unsigned long)ts_priv;
	ts_priv->ts_timer.expires = jiffies + HZ;
	add_timer(&ts_priv->ts_timer);

	ts_priv->file = proc_create("ts", 0644, NULL, &sf_ts_ops);
	if (ts_priv->file == NULL){
		ret = -ENOMEM;
		goto err_out_proc;
	}

	printk("end ts module init\n");
	return 0;

err_out_proc:
	del_timer(&(ts_priv->ts_timer));
	nf_unregister_net_hooks(&init_net, sf_nf_hook_ops, ARRAY_SIZE(sf_nf_hook_ops));
err_out_free:
	kfree(ts_priv);
	return ret;
}

static void __exit sf_ts_exit(void)
{
	struct dev_info* dev;
	struct hlist_node *tmp;
	int i = 0;

	if (g_ts_priv) {
		nf_unregister_net_hooks(&init_net, sf_nf_hook_ops, ARRAY_SIZE(sf_nf_hook_ops));
		remove_proc_entry("ts", NULL);
		del_timer(&(g_ts_priv->ts_timer));

		for (i = 0; i < MAC_HASH_SIZE; i++){
			hlist_for_each_entry_safe(dev, tmp, &g_ts_priv->devlist[i], snode){
				free_percpu(dev->c);
				hlist_del(&dev->snode);
				kfree(dev);
			}
		}
		kfree(g_ts_priv);
	}
}

module_init(sf_ts_init);
module_exit(sf_ts_exit);

MODULE_LICENSE("GPL v2");
MODULE_VERSION("V1.0");
MODULE_AUTHOR("Qin Xia <qin.xia@siflower.com.cn>");
MODULE_DESCRIPTION("siflower tracffic statistics module");
