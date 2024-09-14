#include <linux/init.h>
#include <linux/module.h>
#include <linux/kthread.h>
#include <asm/types.h>
#include <linux/inet.h>
#include <net/sock.h>
#include <linux/socket.h>
#include <linux/netlink.h>
#include <linux/skbuff.h>
#include <linux/net.h>
#include <net/genetlink.h>
#include <linux/platform_device.h>

#include "sfax8_netlink.h"

#ifdef CONFIG_DEBUG_FS
#include <linux/debugfs.h>


int sfax8_genl_debug_show(struct seq_file *m, void *v)
{
	seq_printf(m,"generic netlink debug interface\n");
	return 0;
}

int sfax8_genl_debug_open(struct inode *inode, struct file *file)
{
	return single_open(file, sfax8_genl_debug_show, NULL);
}

ssize_t sfax8_genl_debug_read(struct file *file, char __user *user_buf, size_t count, loff_t *ppos)
{
	int ret = 0;
	ssize_t read;
	char recvbuf[256] = {0};

	ret = sprintf(recvbuf, "Welcome to sfax8 generic netlink module!\n");
	read = simple_read_from_buffer(user_buf, count, ppos, recvbuf, ret);
	return ret;
}

ssize_t sfax8_genl_debug_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
	return count;
}

static struct file_operations genl_debug_ops = {
	.owner = THIS_MODULE,
	.open  = sfax8_genl_debug_open,
	.read  = sfax8_genl_debug_read,
	.write  = sfax8_genl_debug_write,
	.llseek  = default_llseek,
};
#endif


// global variable for sfax8
struct sfax8_netlink *g_sfnl = NULL;




/*-------------------------------------------------------
 * FUCTION AREA
 * Differnt functions for differnt operation
 *------------------------------------------------------*/
static int sf_work_thread(void *msg)
{
	unsigned long i = 0, index= 0, last_i = 0, interval = 0, ret = 0;
	char arg[4][16] = {'\0'};
	char *data = (char *)msg;
	int len = strlen(data);

	if(!len){
		printk("receive data empty\n");
		return -1;
	}

	for(;i < len;i++){
		if(data[i] == ' '){
			memcpy(arg[index], data + last_i, i - last_i);
			last_i = i + 1;
			index++;
		}
	}
	memcpy(arg[index], data + last_i, len - last_i);

	ret = kstrtoul(arg[0], 0, &interval);
	if(ret == -EINVAL)
	  goto err_parsing;
	else if(ret == -ERANGE)
	  goto err_overflow;

	while (!kthread_should_stop()) {
		printk("In work thread, execute interval %ld ms\n", interval);
		msleep(interval);
	}
	return 0;

err_parsing:
	printk("parsing Error, please check the message!\n");
err_overflow:
	printk("convert overflow, please check the message!\n");
	return ret;
}

static int cmd_attr_create_kthread(struct genl_info *info)
{
	struct task_struct	*work_kthread;
	struct nlattr *na;
	struct sk_buff *rep_skb;
	size_t size;
	void *reply;
	char *msg;
	int ret;

	na = info->attrs[SF_CMD_ATTR_KTHREAD_RUN];
	if (!na)
	  return -EINVAL;

	/* message from user space */
	msg = nla_data(na);
	printk("Kernel : receive message, %s\n", msg);

	work_kthread = kthread_run(sf_work_thread, (void *)msg, "work_kthread");
	if (IS_ERR(work_kthread)){
		printk("creat work thread fail\n");
		return -EINVAL;
	}

	/* make work_kthread pointer value as a replay message */
	size = nla_total_size(sizeof(void *));

	rep_skb = genlmsg_new(size, GFP_KERNEL);
	if (!rep_skb)
	  return -ENOMEM;

	if (!info)
	  return -EINVAL;

	/* prepare mesage head */
	reply = genlmsg_put_reply(rep_skb, info, &(g_sfnl->generic_family.family), 0, SF_CMD_GENERIC);
	if (!reply)
	  goto err;

	ret = nla_put(rep_skb, SF_CMD_ATTR_KTHREAD_RUN, size, (void *)&work_kthread);
	if (ret < 0)
	  goto err;
	printk("Kernel : send replay message, 0x%p\n", work_kthread);

	genlmsg_end(rep_skb, reply);
	return genlmsg_reply(rep_skb, info);

err:
	nlmsg_free(rep_skb);
	return ret;
}

static int cmd_attr_stop_kthread(struct genl_info *info)
{
	struct task_struct	*work_kthread;
	struct nlattr *na;
	int *msg;

	na = info->attrs[SF_CMD_ATTR_KTHREAD_STOP];
	if (!na)
	  return -EINVAL;

	/* message from user space */
	msg = nla_data(na);
	printk("Kernel : receive message, 0x%2x\n", *msg);

	work_kthread = (struct task_struct *)*msg;
	kthread_stop(work_kthread);

	return 0;
}

static int cmd_attr_echo_message(struct genl_info *info)
{
	struct nlattr *na;
	struct sk_buff *rep_skb;
	size_t size;
	char *msg;
	void *reply;
	int ret;

	na = info->attrs[SF_CMD_ATTR_ECHO];
	if (!na)
	  return -EINVAL;

	/* message from user space */
	msg = nla_data(na);
	printk("Kernel : receive message, %s\n", msg);

	/* replay message */
	size = nla_total_size(strlen(msg)+1);

	rep_skb = genlmsg_new(size, GFP_KERNEL);
	if (!rep_skb)
	  return -ENOMEM;

	if (!info)
	  return -EINVAL;

	/* prepare mesage head */
	reply = genlmsg_put_reply(rep_skb, info, &(g_sfnl->generic_family.family), 0, SF_CMD_GENERIC);
	if (!reply)
	  goto err;

	ret = nla_put(rep_skb, SF_CMD_ATTR_ECHO, size, msg);
	if (ret < 0)
	  goto err;

	genlmsg_end(rep_skb, reply);
	return genlmsg_reply(rep_skb, info);

err:
	nlmsg_free(rep_skb);
	return ret;
}

static int cmd_attr_wifi(struct genl_info *info)
{
	//Todo: wait wifi provide API
	return 0;
}

static int cmd_attr_app(struct genl_info *info)
{
	//Todo: wait app provide API
	return 0;
}

/*
 * Func  general api for send message to user space
 * @cmd  generic netlink command to classify message
 * @attr generic netlink attribute type
 * @data attribute payload in message
 * @len  attribute payload len
 */
int genl_sendmsg(struct genl_family *family, int attr, void *data, int len){
	struct sk_buff *skb;
	int ret = 0;
	void *msg_head;

	skb = genlmsg_new(MAX_MSG_SIZE, GFP_KERNEL);
	if (!skb)
	  return -ENOMEM;

	msg_head = genlmsg_put(skb, 0, 0, family, 0, SF_CMD_GENERIC);
	if (!msg_head){
		printk("%s : add genlmsg header error!\n", __func__);
		ret = -ENOMEM;
		goto err;
	}

	ret = nla_put(skb, attr, len, data);
	if (ret < 0)
	  goto err;

	genlmsg_end(skb, msg_head);

	ret = genlmsg_multicast(family, skb, 0, 0, GFP_KERNEL);
	return 0;

err:
	nlmsg_free(skb);
	return ret;
}
EXPORT_SYMBOL(genl_sendmsg);


#ifdef CONFIG_SEND_ERR
#define MAX_KERR_MSG_LEN  256
int ker_err_send(char *type, int module, int code, char *text, char *path, int flag)
{
    struct sk_buff *skb = NULL;
    void *msg_head = NULL;
    int size;
    int rc;
    char msg[MAX_KERR_MSG_LEN];
    if(!type || !text || !path) {
        return -ENXIO;
    }
    if((strlen(type) + strlen(text) + strlen(path)) > (MAX_KERR_MSG_LEN - 20)){
        return -E2BIG;
    }
    sprintf(msg,"%s %s %s %d %d %02x", type, text, path, module, code, flag);

    skb = genlmsg_new(MAX_MSG_SIZE, GFP_KERNEL);
    if(!skb){
        return -ENOMEM;
    }

    msg_head = genlmsg_put(skb, 0, 0, &(g_sfnl->generic_family.family), 0, SF_CMD_GENERIC);

    if(!msg_head)
    {
        rc = -ENOMEM;
        goto err_out;
    }

    rc = nla_put_string(skb, SF_CMD_ATTR_ERR, msg);
    if(rc != 0){
        goto err_out;
    }
    genlmsg_end(skb, msg_head);

    rc = genlmsg_multicast(&(g_sfnl->generic_family.family), skb, 0, 0, GFP_KERNEL);
    if(rc != 0&& rc != -ESRCH)
    {
        printk("genlmsg_multicast to user failed, return :%d\n",rc);
        return rc;
    }
    printk("genl_multicast to user success\n");
    return rc;

err_out:
    if(skb)
        nlmsg_free(skb);
    return rc;
}
EXPORT_SYMBOL(ker_err_send);
#endif

/*-------------------------------------------------------
 * INIT AREA
 * Init family and ops
 *------------------------------------------------------*/
int sf_genl_recv(struct sk_buff *skb, struct genl_info *info){
	/* echo message to user space */
	if (info->attrs[SF_CMD_ATTR_ECHO])
	  return cmd_attr_echo_message(info);
	if (info->attrs[SF_CMD_ATTR_KTHREAD_RUN])
	  return cmd_attr_create_kthread(info);
	if (info->attrs[SF_CMD_ATTR_KTHREAD_STOP])
	  return cmd_attr_stop_kthread(info);
	if (info->attrs[SF_CMD_ATTR_WIFI])
	  return cmd_attr_wifi(info);
	if (info->attrs[SF_CMD_ATTR_APP])
	  return cmd_attr_app(info);
	else
	  return -EINVAL;
}

static struct nla_policy sf_genl_policy[SF_CMD_ATTR_MAX + 1] = {
	[SF_CMD_ATTR_ECHO]			= {.type = NLA_STRING, .len  = MSGLEN},
	[SF_CMD_ATTR_KTHREAD_RUN]	= {.type = NLA_STRING, .len  = MSGLEN},
	[SF_CMD_ATTR_WIFI]			= {.type = NLA_STRING, .len  = MSGLEN},
	[SF_CMD_ATTR_APP]			= {.type = NLA_STRING, .len  = MSGLEN},
    [SF_CMD_ATTR_ERR]           = {.type = NLA_STRING, .len  = MSGLEN},
};

static const struct genl_ops sf_genl_ops[] = {
	{
		.cmd		= SF_CMD_GENERIC,
		.doit		= sf_genl_recv,
		.policy		= sf_genl_policy,
		.flags		= (GENL_CMD_CAP_DO|GENL_CMD_CAP_HASPOL),
		.dumpit		= NULL,
	},
};

int sf_genl_family_init(sf_nlfamily *sf_family, char *family_name, char *group_name)
{
	int ret;

	//sf_family->family.id = GENL_ID_GENERATE;
	sf_family->family.hdrsize = 0;
	sf_family->family.version = SF_GENL_VERSION;
	sf_family->family.maxattr = SF_CMD_ATTR_MAX;
	memcpy(sf_family->family.name, family_name, min_t(size_t, strlen(family_name)+1, GENL_NAMSIZ));
	sf_family->family.ops = sf_genl_ops;
	sf_family->family.n_ops = ARRAY_SIZE(sf_genl_ops);
	printk("register new generic netlink family name %s!\n",sf_family->family.name);

	if (group_name){
		/* muticast message must register a group */
		memcpy(sf_family->group[0].name, group_name, min_t(size_t, strlen(group_name)+1, GENL_NAMSIZ));
		sf_family->family.mcgrps = sf_family->group;
		sf_family->family.n_mcgrps = ARRAY_SIZE(sf_family->group);
	}
	ret= genl_register_family(&(sf_family->family));

	if (ret){
		printk("register generic netlink family error!\n");
		return ret;
	}

	return 0;
}

int sf_genl_family_exit(struct genl_family *family)
{
	int ret;

	ret = genl_unregister_family(family);
	if (ret){
		printk("unregister generic netlink family error!\n");
	}

	return 0;
}


static int sf_genl_probe(struct platform_device *pdev)
{
	struct sfax8_netlink *pgenl_priv;

	pgenl_priv = kzalloc(sizeof(struct sfax8_netlink), GFP_KERNEL);
	if (!pgenl_priv) {
		dev_err(&pdev->dev, "no memory for sfax8_netlink data\n");
		return -ENOMEM;
	}
	memset(pgenl_priv, 0, sizeof(struct sfax8_netlink));

	platform_set_drvdata(pdev, pgenl_priv);
	pgenl_priv->genl_family_init = sf_genl_family_init;
	pgenl_priv->genl_family_exit = sf_genl_family_exit;
#ifdef CONFIG_DEBUG_FS
	pgenl_priv->genl_debug = debugfs_create_file("genl_debug", 0777, NULL, NULL, &genl_debug_ops);
#endif

	g_sfnl = pgenl_priv;
	sf_genl_family_init(&(pgenl_priv->generic_family), "COMMON_NL", "common_nl");
	printk("sf generic netlink module init success,get family id %d!\n",pgenl_priv->generic_family.family.id);
	return 0;
}

static int sf_genl_remove(struct platform_device *pdev)
{
	struct sfax8_netlink *pgenl_priv = platform_get_drvdata(pdev);

#ifdef CONFIG_DEBUG_FS
	debugfs_remove(pgenl_priv->genl_debug);
#endif

	sf_genl_family_exit(&(pgenl_priv->generic_family.family));
	platform_set_drvdata(pdev, NULL);
	kfree(pgenl_priv);
	printk("sf generic netlink module exit success\n");
	return 0;
}

static struct platform_driver sf_genl_driver = {
	.driver = {
		.name = "sf_genl",
		.owner = THIS_MODULE,
	},
	.probe = sf_genl_probe,
	.remove = sf_genl_remove,
};


static int __init sf_genl_module_init(void) {
	return platform_driver_register(&sf_genl_driver);
}
module_init(sf_genl_module_init);

static void __exit sf_genl_module_exit(void) {
	platform_driver_unregister(&sf_genl_driver);
}
module_exit(sf_genl_module_exit);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("qin xia <qin.xia@siflower.com.cn>");
MODULE_DESCRIPTION("Siflower generic netlink driver");
MODULE_VERSION(SF_GENL_VERSION);
