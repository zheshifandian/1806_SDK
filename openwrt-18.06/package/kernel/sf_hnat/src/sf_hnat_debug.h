#ifndef _SF_HNAT_DEBUG_H_
#define _SF_HNAT_DEBUG_H_

#include "sf_hnat.h"

#include <linux/circ_buf.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/workqueue.h>
#include <linux/types.h>
#include <linux/list.h>
#include <linux/netdevice.h>
#include <linux/fs.h>
#include <linux/jiffies.h>
#include <linux/timer.h>
#include <linux/kthread.h>
#include <asm/atomic.h>
#include "sf_hnat.h"
#include "sf_hnat_common.h"
#ifdef CONFIG_DEBUG_FS
#include <linux/debugfs.h>
#endif

#ifdef CONFIG_SFAX8_HNAT_TEST_TOOL
#include "sf_test_thread_poll.h"
#endif


u64 Divided_64(__u64 a, __u64 b) ;
int sf_hnat_debug_open(struct inode *inode, struct file *file);
ssize_t sf_hnat_debug_read(struct file *file, char __user *buf, size_t count, loff_t *ppos);
ssize_t sf_hnat_debug_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos);
long sf_hnat_debug_ioctl(struct file *file, unsigned int a, unsigned long b);

#ifdef CONFIG_SFAX8_HNAT_TEST_TOOL
void sf_test_tool_deinit(struct platform_device *pdev);
void sf_test_tool_init(struct platform_device *pdev, struct device *driver_dev, void * driver_priv);
void sf_test_tool_set_rx(struct platform_device *pdev, unsigned char type, unsigned int pkt_length);
#endif

#endif //_SF_HNAT_DEBUG_H_
