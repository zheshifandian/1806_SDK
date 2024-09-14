#ifdef CONFIG_SFA28_FULLMASK
#include <sf19a28.h>
#endif
#include "start_core.h"

#ifdef CONFIG_SF16A18_LMAC_USE_M_SFDSP
int plat_load_task(struct task_info *node)
{
    struct m_SFDSP_syscon *syscon;
    unsigned long offset = 0x400;

    if (!node) {
        pr_err("Invalid node!\n");
        return -EINVAL;
    }
    syscon = kmalloc(sizeof(struct m_SFDSP_syscon), GFP_KERNEL);
    if (!syscon)
        return -ENOMEM;

    syscon->exceptionbase_12_19 = (unsigned long *)(M_SFDSP_0_SI_ExceptionBase_12_19 + node->task_id * offset);
    syscon->exceptionbase_20_27 = (unsigned long *)(M_SFDSP_0_SI_ExceptionBase_20_27 + node->task_id * offset);
    syscon->exceptionbase_28 = (unsigned long *)(M_SFDSP_0_SI_ExceptionBase_28 + node->task_id * offset);
    syscon->aresetn = (unsigned long *)(M_SFDSP_0_ARESETN +
                                        node->task_id * offset);
    syscon->por_resetn = (unsigned long *)(M_SFDSP_0_POR_RESETN + node->task_id * offset);

    node->platdata = (void *)syscon;

    return 0;
}

int plat_start_task(struct task_info *node)
{
    struct m_SFDSP_syscon *syscon;

    if (!node || !node->entry_addr || !node->platdata) {
        pr_err("Invalid node!\n");
        return -EINVAL;
    }

    syscon = (struct m_SFDSP_syscon *)node->platdata;
    printk("task entry_addr=0x%x\n", (int)(node->entry_addr));
    //set reset vector
    writel(CKSEG1ADDR(node->entry_addr) >> 12, syscon->exceptionbase_12_19);
    writel(CKSEG1ADDR(node->entry_addr) >> 20, syscon->exceptionbase_20_27);
    writel(CKSEG1ADDR(node->entry_addr) >> 28, syscon->exceptionbase_28);

    printk("start aresetn %x por_resetn %x \n", readl(syscon->aresetn), readl(syscon->por_resetn));

    //reset & enable m_SFDSP
    writel(0x2, syscon->aresetn);
    writel(1, syscon->por_resetn);

    return 0;
}

int plat_stop_task(struct task_info *node)
{
    struct m_SFDSP_syscon *syscon;

    if (!node || !node->platdata) {
        pr_err("Invalid node!\n");
        return -EINVAL;
    }

    syscon = (struct m_SFDSP_syscon *)node->platdata;

    printk("stop aresetn %x por_resetn %x \n", readl(syscon->aresetn), readl(syscon->por_resetn));
    writel(0, syscon->por_resetn);
    //bus hold
    //should not set areset by single
    //writel(0, syscon->aresetn);
    return 0;
}

int plat_remove_task(struct task_info *node)
{
    kfree(node->platdata);

    return 0;
}
#endif
