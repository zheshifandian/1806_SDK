#ifndef _STARTCORE_H_
#define _STARTCORE_H_

#include <linux/init.h>
#include <linux/kernel.h>

//kmalloc/kfree
#include <linux/slab.h>

struct task_info {
    struct list_head list;
    unsigned long entry_addr;
    int gp_addr;
    int task_id;
    int state;
    int filesize;
    void *platdata;
};

enum {
    SF_TASK_READY,
    SF_TASK_RUNNING,
    SF_TASK_STOPPED,
};

#ifdef CONFIG_SF16A18_LMAC_USE_M_SFDSP
//use m_SFDSP to run lmac
struct m_SFDSP_syscon {
    unsigned long *exceptionbase_12_19;
    unsigned long *exceptionbase_20_27;
    unsigned long *exceptionbase_28;
    unsigned long *aresetn;
    unsigned long *por_resetn;
};
#endif

extern int plat_load_task(struct task_info *node);
extern int plat_start_task(struct task_info *node);
extern int plat_stop_task(struct task_info *node);
extern int plat_remove_task(struct task_info *node);

#endif /* _STARTCORE_H_ */
