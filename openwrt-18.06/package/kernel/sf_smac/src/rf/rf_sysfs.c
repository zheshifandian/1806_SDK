#include <linux/kernel.h>
#include <linux/kmod.h>
#include <linux/debugfs.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#include "rf_pl_ref.h"
#include "rf_sysfs.h"
#include "rf_access.h"

/* some macros taken from iwlwifi */
#define DEBUGFS_ADD_FILE(name, parent, mode) do {               \
    if (!debugfs_create_file(#name, mode, parent, pl_ctx,      \
                &sf_wifi_rf_dbgfs_##name##_ops))                      \
    goto err;                                                   \
} while (0)

#define DEBUGFS_ADD_BOOL(name, parent, ptr) do {                \
    struct dentry *__tmp;                                       \
    __tmp = debugfs_create_bool(#name, S_IWUSR | S_IRUSR,       \
            parent, ptr);                                       \
    if (IS_ERR(__tmp) || !__tmp)                                \
    goto err;                                                   \
} while (0)

#define DEBUGFS_ADD_X64(name, parent, ptr) do {                 \
    struct dentry *__tmp;                                       \
    __tmp = debugfs_create_x64(#name, S_IWUSR | S_IRUSR,        \
            parent, ptr);                                       \
    if (IS_ERR(__tmp) || !__tmp)                                \
    goto err;                                                   \
} while (0)

#define DEBUGFS_ADD_U64(name, parent, ptr, mode) do {           \
    struct dentry *__tmp;                                       \
    __tmp = debugfs_create_u64(#name, mode,                     \
            parent, ptr);                                       \
    if (IS_ERR(__tmp) || !__tmp)                                \
    goto err;                                                   \
} while (0)

#define DEBUGFS_ADD_X32(name, parent, ptr) do {                 \
    struct dentry *__tmp;                                       \
    __tmp = debugfs_create_x32(#name, S_IWUSR | S_IRUSR,        \
            parent, ptr);                                       \
    if (IS_ERR(__tmp) || !__tmp)                                \
    goto err;                                                   \
} while (0)

#define DEBUGFS_ADD_U32(name, parent, ptr, mode) do {           \
    struct dentry *__tmp;                                       \
    __tmp = debugfs_create_u32(#name, mode,                     \
            parent, ptr);                                       \
    if (IS_ERR(__tmp) || !__tmp)                                \
    goto err;                                                   \
} while (0)

/* file operation */
#define DEBUGFS_READ_FUNC(name)                                 \
    static ssize_t sf_wifi_rf_dbgfs_##name##_read(struct file *file,  \
            char __user *user_buf,                              \
            size_t count, loff_t *ppos);

#define DEBUGFS_WRITE_FUNC(name)                                \
    static ssize_t sf_wifi_rf_dbgfs_##name##_write(struct file *file, \
            const char __user *user_buf,                        \
            size_t count, loff_t *ppos);


#define DEBUGFS_READ_FILE_OPS(name)                             \
    DEBUGFS_READ_FUNC(name);                                    \
static const struct file_operations sf_wifi_rf_dbgfs_##name##_ops = { \
    .read   = sf_wifi_rf_dbgfs_##name##_read,                         \
    .open   = simple_open,                                      \
    .llseek = generic_file_llseek,                              \
};

#define DEBUGFS_WRITE_FILE_OPS(name)                            \
    DEBUGFS_WRITE_FUNC(name);                                   \
static const struct file_operations sf_wifi_rf_dbgfs_##name##_ops = { \
    .write  = sf_wifi_rf_dbgfs_##name##_write,                        \
    .open   = simple_open,                                      \
    .llseek = generic_file_llseek,                              \
};


#define DEBUGFS_READ_WRITE_FILE_OPS(name)                       \
    DEBUGFS_READ_FUNC(name);                                    \
    DEBUGFS_WRITE_FUNC(name);                                       \
static const struct file_operations sf_wifi_rf_dbgfs_##name##_ops = { \
    .write  = sf_wifi_rf_dbgfs_##name##_write,                        \
    .read   = sf_wifi_rf_dbgfs_##name##_read,                         \
    .open   = simple_open,                                      \
    .llseek = generic_file_llseek,                              \
};

#define BOOT_HDR "booted : %2x\n"
#define BOOT_HDR_MAX_LEN  (sizeof(BOOT_HDR) + 16)

#define BB_HDR "working bb : %2x\n"
#define BB_HDR_MAX_LEN (sizeof(BB_HDR) + 16)

#define CALI_HDR "calibrating : %2x\n"
#define CALI_HDR_MAX_LEN (sizeof(CALI_HDR) + 16)

#define IRQ_HDR "irq number : %10d\n"
#define IRQ_HDR_MAX_LEN (sizeof(IRQ_HDR) + 16)

#define CALI_WORK_HDR "calibration work time : %lld\n"
#define CALI_WORK_HDR_MAX_LEN (sizeof(CALI_WORK_HDR) + 16)

#define RF_DUMP_SIZE 200
enum {
    RF_CMD_RECORD_STOP = 0,
    RF_CMD_RECORD_START = 1,
    RF_CMD_DUMP_START = 2,
    RF_CMD_DUMP_STOP = 3,
};
extern void sf_wifi_rf_force_calibrate(uint8_t conf);
extern void sf_wifi_rf_reset_hk(void);

static ssize_t sf_wifi_rf_dbgfs_stats_read(struct file *file,
        char __user *user_buf,
        size_t count,
        loff_t *ppos)
{
    struct rf_pl_context *pl_ctx = file->private_data;
    char *buf;
    int res;
    ssize_t read;
    uint8_t tmp;
    //IRQ uint32_t
    //band registered:
    size_t bufsz = (BOOT_HDR_MAX_LEN + BB_HDR_MAX_LEN + CALI_HDR_MAX_LEN + IRQ_HDR_MAX_LEN + CALI_WORK_HDR_MAX_LEN);

    /*everything is read out in one go*/
    if(*ppos)
        return 0;
    if(!pl_ctx)
        return 0;

    bufsz = min_t(size_t, bufsz, count);
    buf = kzalloc(bufsz, GFP_ATOMIC);
    if(buf == NULL)
        return 0;

    bufsz--;
    tmp = ((pl_ctx->hb_working ? 1 : 0) << 1) | (pl_ctx->lb_working ? 1 : 0);

    res = scnprintf(buf, bufsz, BOOT_HDR  \
            BB_HDR  \
            CALI_HDR    \
            IRQ_HDR \
            CALI_WORK_HDR   \
            "\n",   \
            pl_ctx->booted, \
            tmp,    \
            pl_ctx->calibrating,    \
            pl_ctx->irq_num,    \
            pl_ctx->cali_work_num);

    read = simple_read_from_buffer(user_buf, count, ppos, buf, res);
    kfree(buf);

    return read;
}
DEBUGFS_READ_FILE_OPS(stats);

static ssize_t sf_wifi_rf_dbgfs_irqnum_read(struct file *file,
        char __user *user_buf,
        size_t count,
        loff_t *ppos)
{
    struct rf_pl_context *pl_ctx = file->private_data;
    char *buf = NULL;
    int res, i;
    ssize_t read;
    size_t bufsz = 1024;
    uint32_t hk_irq_num = 0;
    uint32_t cmd_err_irq_num = 0;

    /*everything is read out in one go*/
    if(*ppos)
        return 0;
    if(!pl_ctx)
        return 0;

    buf = kzalloc(bufsz, GFP_ATOMIC);
    if(buf == NULL)
        return 0;

    res = 0;
    for(i = 0; i < 16; i++){
        res += scnprintf(&buf[res], min_t(size_t, bufsz - 1, count - res),
                "hk irq[%d] number : %d\n", i, pl_ctx->hk_irq_source_num[i]);
        hk_irq_num += pl_ctx->hk_irq_source_num[i];
    }

    for (i = 0; i < 16; i++) {
        res += scnprintf(&buf[res], min_t(size_t, bufsz - 1, count - res),
                "cmd err irq[%d] number : %d\n", i, pl_ctx->cmd_err_irq_source_num[i]);
        cmd_err_irq_num += pl_ctx->cmd_err_irq_source_num[i];
    }

    res += scnprintf(&buf[res], min_t(size_t, bufsz - 1, count - res),
            "total hk irq : %d, total cmd err irq : %d, total irq : %d\n", hk_irq_num, cmd_err_irq_num, pl_ctx->irq_num);

    read = simple_read_from_buffer(user_buf, count, ppos, buf, res);
    kfree(buf);

    return read;
}
DEBUGFS_READ_FILE_OPS(irqnum);

static ssize_t sf_wifi_rf_dbgfs_recalibrate_read(struct file *file,
        char __user *user_buf,
        size_t count,
        loff_t *ppos)
{
    struct rf_pl_context *pl_ctx = file->private_data;
    char *buf;
    int res;
    ssize_t read;
    //IRQ uint32_t
    //band registered:
    size_t bufsz = 32;
    /*everything is read out in one go*/
    if(*ppos)
        return 0;
    if(!pl_ctx)
        return 0;

    bufsz = min_t(size_t, bufsz, count);
    buf = kmalloc(bufsz, GFP_ATOMIC);
    if(buf == NULL)
        return 0;

    bufsz--;

    res = scnprintf(buf, bufsz, "%d\n", pl_ctx->dbg_recalibrate);

    read = simple_read_from_buffer(user_buf, count, ppos, buf, res);
    kfree(buf);

    return read;
}

static ssize_t sf_wifi_rf_dbgfs_recalibrate_write(struct file *file,
                                        const char __user *user_buf,
                                        size_t count, loff_t *ppos)
{
    struct rf_pl_context *pl_ctx = file->private_data;

    char buf[32];
    unsigned int val;
    size_t len = min_t(size_t, count, sizeof(buf) - 1);
    bool formatx = false;

    if (copy_from_user(buf, user_buf, len))
        return -EFAULT;
    if(!pl_ctx)
        return -EFAULT;

    buf[len] = '\0';

    if(buf[0] == '0' && buf[1] == 'x'){
        formatx = true;
    }
    if (sscanf(buf, formatx ? "0x%x" : "%d", &val)){
        pl_ctx->dbg_recalibrate = val;
    }else{
        printk("can not sscanf the buf!\n");
        return -EFAULT;
    }

    sf_wifi_rf_force_calibrate(pl_ctx->dbg_recalibrate);

    return count;
}
DEBUGFS_READ_WRITE_FILE_OPS(recalibrate);

static ssize_t sf_wifi_rf_dbgfs_dumpstart_record_write(struct file *file,
                const char __user *user_buf,
                size_t count,
                loff_t *ppos)
{
    struct rf_pl_context *pl_ctx = file->private_data;

    char buf[32];
    int val = 0;
    int band = 0;
    int ret = 0;
    uint8_t flag = 0;
    struct rf_cmd_dump_val *dump_val = NULL;

    size_t len = min_t(size_t, count, sizeof(buf) - 1);

    if (copy_from_user(buf, user_buf, len))
        return -EFAULT;
    if(!pl_ctx)
        return -EFAULT;

    buf[len] = '\0';

    if (!sscanf(buf, "%d", &val)){
        printk("can not sscanf the buf!\n");
        return -EFAULT;
    }
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    if (!pl_ctx->booted) {
        printk("have not boot up, do not support dumping!\n");
        return count;
    }

    switch (val) {
    case 0:
        band = 0; //LB
        flag = RF_CMD_RECORD_STOP;
        break;
    case 1:
        band = 0;
        flag = RF_CMD_RECORD_START;
        break;
    case 10:
        band = 1; //HB
        flag = RF_CMD_RECORD_STOP;
        break;
    case 11:
        band = 1;
        flag = RF_CMD_RECORD_START;
        break;
    default:
        printk("error val\n");
        return count;
    }
    mutex_lock(&pl_ctx->bb_mtx);
    if (pl_ctx->clients[band].band_type) {
        //setting
        if (!!flag) {
            pl_ctx->rf_cmd_sets[band].address = (uint32_t)(kzalloc(sizeof(struct rf_cmd_dump_val) * RF_DUMP_SIZE, GFP_KERNEL));
            pl_ctx->rf_cmd_sets[band].size = RF_DUMP_SIZE;
            printk("set addr:%x\n", pl_ctx->rf_cmd_sets[band].address);
        } else {
            //check if started has been called,
            //otherwise, release the mutex and return;
            if (!pl_ctx->rf_cmd_sets[band].flag || !pl_ctx->rf_cmd_sets[band].address) {
                mutex_unlock(&pl_ctx->bb_mtx);
                return count;
            }
        }
        dump_val = (struct rf_cmd_dump_val *)(pl_ctx->rf_cmd_sets[band].address);
        pl_ctx->rf_cmd_sets[band].flag = flag;
        pl_ctx->rf_cmd_sets[band].count = 0;
        ret = pl_ctx->clients[band].cb(pl_ctx->clients[band].cb_data, RF_EVENT_DUMP_CMD, 0, &(pl_ctx->rf_cmd_sets[band]));
        if (!ret && flag == 1)
            dump_val = NULL;
    }
    mutex_unlock(&pl_ctx->bb_mtx);

    if (dump_val)
        kfree(dump_val);

    return count;
}
DEBUGFS_WRITE_FILE_OPS(dumpstart_record);

static ssize_t sf_wifi_rf_dbgfs_dumpcmd_write(struct file *file,
                const char __user *user_buf,
                size_t count,
                loff_t *ppos)
{
    struct rf_pl_context *pl_ctx = file->private_data;

    char buf[32];
    mm_segment_t fs;
    struct file *fp;

    int val = 0;
    int band = 0;
    int ret = 0;
    uint16_t pos = 0;
    uint16_t num = 0;
    uint8_t flag = 0;
    char result[64] = "\0";
    loff_t fp_pos = 0;
    size_t len = min_t(size_t, count, sizeof(buf) - 1);
    struct rf_cmd_dump_val *dump_val = NULL;

    if (copy_from_user(buf, user_buf, len))
        return -EFAULT;
    if(!pl_ctx)
        return -EFAULT;

    buf[len] = '\0';

    if (!sscanf(buf, "%d", &val)){
        printk("can not sscanf the buf!\n");
        return -EFAULT;
    }
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    if (!pl_ctx->booted) {
        printk("have not boot up, do not support dumping!\n");
        return count;
    }
    switch (val) {
    case 0:
        band = 0;
        flag = RF_CMD_DUMP_START;
        fp = filp_open("/rf_lb_cmds_dump.txt", O_RDWR | O_CREAT, 0777);
        break;
    case 1:
        band = 1;
        flag = RF_CMD_DUMP_START;
        fp = filp_open("/rf_hb_cmds_dump.txt", O_RDWR | O_CREAT, 0777);
        break;
    default:
        printk("error val\n");
        return count;
    }
    if (IS_ERR(fp)) {
        printk("can not create the rf_cmds_dump.txt\n");
        return count;
    }

    mutex_lock(&pl_ctx->bb_mtx);
    dump_val = (struct rf_cmd_dump_val *)(pl_ctx->rf_cmd_sets[band].address);
    if (dump_val == NULL) {
        printk("dump_val is null\n");
        filp_close(fp, NULL);
        mutex_unlock(&pl_ctx->bb_mtx);
        return count;
    }
    if (pl_ctx->clients[band].band_type) {
        pl_ctx->rf_cmd_sets[band].flag = flag;
        ret = pl_ctx->clients[band].cb(pl_ctx->clients[band].cb_data, RF_EVENT_DUMP_CMD, 0, &(pl_ctx->rf_cmd_sets[band]));
        if (ret) {
            printk("RF_EVENT_DUMP_CMD fail,ret:%d\n", ret);
            filp_close(fp, NULL);
            mutex_unlock(&pl_ctx->bb_mtx);
            return count;
        }
    }
    printk("~~~~dump the recent 200 RF cmds from %p,count:%d~~~~~~\n", dump_val, pl_ctx->rf_cmd_sets[band].count);
    pos = pl_ctx->rf_cmd_sets[band].count;
    mutex_unlock(&pl_ctx->bb_mtx);

    fs = get_fs();
    set_fs(KERNEL_DS);
    while (num < RF_DUMP_SIZE) {
        if (pos >= RF_DUMP_SIZE)
            pos = 0;
        switch (dump_val[pos].argnum) {
        case 0:
            sprintf(result, "[%d]cmd:0x%x\r\n", num, dump_val[pos].cmdid);
            break;
        case 1:
            sprintf(result, "[%d]cmd:0x%x,arg0:0x%x\r\n", num, dump_val[pos].cmdid, dump_val[pos].arg0);
            break;
        case 2:
            sprintf(result, "[%d]cmd:0x%x,arg0:0x%x,arg1:0x%x\r\n", num, dump_val[pos].cmdid, dump_val[pos].arg0, dump_val[pos].arg1);
            break;
        case 3:
            sprintf(result, "[%d]cmd:0x%x,arg0:0x%x,arg1:0x%x,arg2:0x%x\r\n", num, dump_val[pos].cmdid, dump_val[pos].arg0, dump_val[pos].arg1, dump_val[pos].arg2);
            break;
        case 4:
            sprintf(result, "[%d]cmd:0x%x,arg0:0x%x,arg1:0x%x,arg2:0x%x,arg3:0x%x\r\n", num, dump_val[pos].cmdid,
                            dump_val[count].arg0, dump_val[pos].arg1, dump_val[pos].arg2, dump_val[pos].arg3);
            break;
        default:
            sprintf(result, "argnum is %d,error(>4)\r\n", dump_val[pos].argnum);
            break;
        }
        printk("%s\n", result);
		kernel_write(fp, result, sizeof(result), &fp_pos);
	   	pos++;
        num++;
    }
    set_fs(fs);
    mutex_lock(&pl_ctx->bb_mtx);
    if (pl_ctx->clients[band].band_type) {
        pl_ctx->rf_cmd_sets[band].flag = RF_CMD_DUMP_STOP;
        ret = pl_ctx->clients[band].cb(pl_ctx->clients[band].cb_data, RF_EVENT_DUMP_CMD, 0, &(pl_ctx->rf_cmd_sets[band]));
        if (ret)
            printk("RF_EVENT_DUMP_CMD fail,ret:%d\n", ret);
    }
    mutex_unlock(&pl_ctx->bb_mtx);
    filp_close(fp, NULL);
    return count;
}
DEBUGFS_WRITE_FILE_OPS(dumpcmd);

static ssize_t sf_wifi_rf_dbgfs_rf_hw_version_read(struct file *file,
        char __user *user_buf,
        size_t count,
        loff_t *ppos)
{
    struct rf_pl_context *priv = file->private_data;
    char *buf = NULL;
    int res,value;
    ssize_t read;
    size_t bufsz = 32;
    /*everything is read out in one go*/
    if(*ppos)
        return 0;
    bufsz = min_t(size_t, bufsz, count);
    buf = kmalloc(bufsz, GFP_ATOMIC);
    if(buf == NULL)
        return 0;
    bufsz--;
    value = priv->rf_hw_version;
    res = scnprintf(buf, bufsz, "0x%x\n", value);
    read = simple_read_from_buffer(user_buf, count, ppos, buf, res);
    kfree(buf);
    return read;
}
DEBUGFS_READ_FILE_OPS(rf_hw_version);

static ssize_t sf_wifi_rf_dbgfs_temperature_read(struct file *file,
        char __user *user_buf,
        size_t count,
        loff_t *ppos)
{
    struct rf_pl_context *pl_ctx = file->private_data;
    char *buf;
    int res,value;
    ssize_t read;
    //IRQ uint32_t
    //band registered:
    size_t bufsz = 32;

    /*everything is read out in one go*/
    if(*ppos)
        return 0;
    if(!pl_ctx)
        return 0;

    bufsz = min_t(size_t, bufsz, count);
    buf = kmalloc(bufsz, GFP_ATOMIC);
    if(buf == NULL)
        return 0;

    bufsz--;

    value = rf_get_temperature(1);

    if (value == NO_BAND_RUN)
        value = rf_get_temperature(0);
    res = scnprintf(buf, bufsz, "%d\n", value);
    read = simple_read_from_buffer(user_buf, count, ppos, buf, res);
    kfree(buf);

    return read;
}
DEBUGFS_READ_FILE_OPS(temperature);

#ifdef CONFIG_SFA28_FULLMASK
//get temperature from temperature node,then mapped it to surface temperature.
//Mapping rules are obtained by testing(see RM#7951).
//It may be related to room temperature, which was 24 degrees Celsius at the time.
static ssize_t sf_wifi_rf_dbgfs_temperature_surface_read(struct file *file,
        char __user *user_buf,
        size_t count,
        loff_t *ppos)
{
    struct rf_pl_context *pl_ctx = file->private_data;
    char *buf;
    int res,value;
    ssize_t read;
    //IRQ uint32_t
    //band registered:
    size_t bufsz = 32;

    /*everything is read out in one go*/
    if(*ppos)
        return 0;
    if(!pl_ctx)
        return 0;

    bufsz = min_t(size_t, bufsz, count);
    buf = kmalloc(bufsz, GFP_ATOMIC);
    if(buf == NULL)
        return 0;

    bufsz--;
#ifdef COOLING_TEMP
    value = pl_ctx->rf_thermal_state.temp + pl_ctx->rf_thermal_state.relative_diff;
#else
    value = rf_get_temperature(1);
#endif
    if (value == NO_BAND_RUN)
	{
        value = rf_get_temperature(0);
	}

	if (24 < value && value < 56)
	{
		value = (55 - value) / 2 + value;
	}

    res = scnprintf(buf, bufsz, "surface temperature:%d\n", value);
    read = simple_read_from_buffer(user_buf, count, ppos, buf, res);
    kfree(buf);

    return read;
}
DEBUGFS_READ_FILE_OPS(temperature_surface);
#endif

#ifdef COOLING_TEMP
static ssize_t sf_wifi_rf_dbgfs_cooling_temp_read(struct file *file,
        char __user *user_buf,
        size_t count,
        loff_t *ppos)
{
    struct rf_pl_context *pl_ctx = file->private_data;
    char *buf;
    int res;
    ssize_t read;
    //IRQ uint32_t
    //band registered:
    size_t bufsz = 256;

    /*everything is read out in one go*/
    if(*ppos)
        return 0;
    if(!pl_ctx)
        return 0;

    bufsz = min_t(size_t, bufsz, count);
    buf = kmalloc(bufsz, GFP_ATOMIC);
    if(buf == NULL){
        printk("oom happen!\n");
        return 0;
    }

    bufsz--;

    res = scnprintf(buf, bufsz, "+++flag:%d;lb max_idx:%d;hb max_idx:%d;thermalcnt=%d diff=%d\n",
                        pl_ctx->cooling_temp_set.flag,pl_ctx->cooling_temp_set.max_index[0],
                        pl_ctx->cooling_temp_set.max_index[1],pl_ctx->rf_thermal_state.count,
                        pl_ctx->rf_thermal_state.relative_diff);

    read = simple_read_from_buffer(user_buf, count, ppos, buf, res);
    kfree(buf);

    return read;
}

static ssize_t sf_wifi_rf_dbgfs_cooling_temp_write(struct file *file,
                                        const char __user *user_buf,
                                        size_t count, loff_t *ppos)
{
    struct rf_pl_context *pl_ctx = file->private_data;
    char buf[32];
    int val;
    size_t len = min_t(size_t, count, sizeof(buf) - 1);

    if (copy_from_user(buf, user_buf, len))
        return -EFAULT;

    if(!pl_ctx)
        return -EFAULT;

    buf[len] = '\0';

    if(sscanf(buf, "%d", &val)){
        pl_ctx->cooling_temp_set.flag = val;
        printk("flag %d\n", pl_ctx->cooling_temp_set.flag);
    }else{
        printk("can not sscanf the buf!\n");
        return -EFAULT;
    }

    return count;
}
DEBUGFS_READ_WRITE_FILE_OPS(cooling_temp);

static ssize_t sf_wifi_rf_dbgfs_thermal_on_read(struct file *file,
        char __user *user_buf,
        size_t count,
        loff_t *ppos)
{
    struct rf_pl_context *pl_ctx = file->private_data;
    char *buf;
    int res;
    ssize_t read;
    //IRQ uint32_t
    //band registered:
    size_t bufsz = 32;

    if(*ppos)
        return 0;
    if(!pl_ctx)
        return 0;

    bufsz = min_t(size_t, bufsz, count);
    buf = kmalloc(bufsz, GFP_ATOMIC);
    if(buf == NULL){
        return 0;
    }

    bufsz--;

    res = scnprintf(buf, bufsz, "%d\n", pl_ctx->cooling_temp_set.thermal_on);

    read = simple_read_from_buffer(user_buf, count, ppos, buf, res);
    kfree(buf);

    return read;
}
DEBUGFS_READ_FILE_OPS(thermal_on);
#endif
#ifdef CONFIG_SFA28_FULLMASK
//#ifdef CFG_DUAL_ANTENNA_CALIBRATE
static ssize_t sf_wifi_rf_dbgfs_property_read(struct file *file,
        char __user *user_buf,
        size_t count,
        loff_t *ppos)
{
    struct rf_pl_context *pl_ctx = file->private_data;
    char *buf;
    int ret = 0;
    ssize_t read;
    int property_value=0;
    int i=0;
    size_t bufsz = 4096;

    /*everything is read out in one go*/
    if(*ppos)
        return 0;
    if(!pl_ctx)
        return 0;

    bufsz = min_t(size_t, bufsz, count);
    buf = kmalloc(bufsz, GFP_ATOMIC);
    if(buf == NULL){
        printk("oom happen!\n");
        return 0;
    }
    bufsz--;
    for(i=0;i<53;i++){
        property_value = rf_get_property((uint16_t)i);
        ret += scnprintf(&buf[ret], bufsz, "property_id=%d property_value=%d\n",i,property_value);
    }
    read = simple_read_from_buffer(user_buf, count, ppos, buf, ret);
    kfree(buf);
    return read;
}

static ssize_t sf_wifi_rf_dbgfs_property_write(struct file *file,
                                        const char __user *user_buf,
                                        size_t count, loff_t *ppos)
{
    struct rf_pl_context *pl_ctx = file->private_data;
    char buf[100];
    int property_id;
    int property_value;
    size_t len = min_t(size_t, count, sizeof(buf) - 1);

    if (copy_from_user(buf, user_buf, len))
        return -EFAULT;

    if(!pl_ctx)
        return -EFAULT;

    if(sscanf(buf, "property_id=%d property_value=%d",&property_id, &property_value)){
        rf_set_property(property_id,property_value);
    }else{
        printk("can not sscanf the buf!\n");
        return -EFAULT;
    }
    return count;
}
DEBUGFS_READ_WRITE_FILE_OPS(property);
//#endif
#endif

static ssize_t sf_wifi_rf_dbgfs_rf_switch_trx_write(struct file *file,
                                        const char __user *user_buf,
                                        size_t count, loff_t *ppos)
{
    struct rf_pl_context *pl_ctx = file->private_data;
    char buf[100];
    int property_id;
    int property_value;
    uint8_t femctrl;
    size_t len = min_t(size_t, count, sizeof(buf) - 1);

    if (copy_from_user(buf, user_buf, len))
        return -EFAULT;

    if(!pl_ctx)
        return -EFAULT;
    printk("txpower=  band=   0 to tx 1 to rx\n");
    if(sscanf(buf, "band=%d tx=%d fem=%hhd",&property_id, &property_value,&femctrl)){
        printk("%d %d\n",property_id,property_value);
        if(property_value == 0){
        rf_switch_fem_ctrl(femctrl);
            rf_trx_status_change(10,property_id,property_value);
        }
        else if(property_value == 1)
        {
        rf_switch_fem_ctrl(femctrl);
            rf_trx_status_change(10,property_id,property_value);
        }
    }else{
        printk("can not sscanf the buf!\n");
        return -EFAULT;
    }
    return count;
}
DEBUGFS_WRITE_FILE_OPS(rf_switch_trx);

extern int8_t rf_set_xo_value(uint32_t xo_value);
static ssize_t sf_wifi_rf_dbgfs_setxo_write(struct file *file,
                                        const char __user *user_buf,
                                        size_t count, loff_t *ppos)
{
    struct rf_pl_context *pl_ctx = file->private_data;
    char buf[32];
    int val;
    size_t len = min_t(size_t, count, sizeof(buf) - 1);

    if (copy_from_user(buf, user_buf, len))
        return -EFAULT;

    if(!pl_ctx)
        return -EFAULT;

    buf[len] = '\0';

    if(sscanf(buf, "%d", &val)){
        rf_set_xo_value(val);
    }else{
        printk("can not sscanf the buf!\n");
        return -EFAULT;
    }

    return count;
}
DEBUGFS_WRITE_FILE_OPS(setxo);

static ssize_t sf_wifi_rf_dbgfs_addr_read(struct file *file,
        char __user *user_buf,
        size_t count,
        loff_t *ppos)
{
    struct rf_pl_context *pl_ctx = file->private_data;
    char *buf;
    int res;
    ssize_t read;
    //IRQ uint32_t
    //band registered:
    size_t bufsz = 32;
    /*everything is read out in one go*/
    if(*ppos)
        return 0;

    if(!pl_ctx)
        return 0;

    bufsz = min_t(size_t, bufsz, count);
    buf = kmalloc(bufsz, GFP_ATOMIC);
    if(buf == NULL){
        printk("oom happen!\n");
        return 0;
    }

    bufsz--;

    res = scnprintf(buf, bufsz, "0x%x\n", pl_ctx->dbg_reg_addr);

    read = simple_read_from_buffer(user_buf, count, ppos, buf, res);
    kfree(buf);

    return read;
}

static ssize_t sf_wifi_rf_dbgfs_addr_write(struct file *file,
                                        const char __user *user_buf,
                                        size_t count, loff_t *ppos)
{
    struct rf_pl_context *pl_ctx = file->private_data;

    char buf[32];
    int val;
    size_t len = min_t(size_t, count, sizeof(buf) - 1);

    if (copy_from_user(buf, user_buf, len))
        return -EFAULT;

    if(!pl_ctx)
        return -EFAULT;

    buf[len] = '\0';

    if(sscanf(buf, "0x%x", &val)){
        pl_ctx->dbg_reg_addr = (uint16_t)(val & 0xFFFF);
    }else{
        printk("can not sscanf the buf!\n");
        return -EFAULT;
    }

    return count;
}
DEBUGFS_READ_WRITE_FILE_OPS(addr);

static ssize_t sf_wifi_rf_dbgfs_regr_read(struct file *file,
        char __user *user_buf,
        size_t count,
        loff_t *ppos)
{
    struct rf_pl_context *pl_ctx = file->private_data;
    char *buf;
    int res;
    ssize_t read;
    uint16_t tmp;
    //IRQ uint32_t
    //band registered:
    size_t bufsz = 32;

    /*everything is read out in one go*/
    if(*ppos)
        return 0;
    if(!pl_ctx)
        return 0;

    bufsz = min_t(size_t, bufsz, count);
    buf = kmalloc(bufsz, GFP_ATOMIC);
    if(buf == NULL)
        return 0;

    bufsz--;

    tmp = ml_apb_read(pl_ctx->dbg_reg_addr);
    res = scnprintf(buf, bufsz, "0x%x\n", tmp);

    read = simple_read_from_buffer(user_buf, count, ppos, buf, res);
    kfree(buf);

    return read;
}
DEBUGFS_READ_FILE_OPS(regr);

static ssize_t sf_wifi_rf_dbgfs_regw_write(struct file *file,
                                        const char __user *user_buf,
                                        size_t count, loff_t *ppos)
{
    struct rf_pl_context *pl_ctx = file->private_data;

    char buf[32];
    int val;
    size_t len = min_t(size_t, count, sizeof(buf) - 1);

    if (copy_from_user(buf, user_buf, len))
        return -EFAULT;
    if(!pl_ctx)
        return -EFAULT;

    buf[len] = '\0';

    if(sscanf(buf, "0x%x", &val)){
        ml_apb_write(pl_ctx->dbg_reg_addr, val & 0xFFFF);
    }else{
        printk("can not sscanf the buf!\n");
        return -EFAULT;
    }

    return count;
}
DEBUGFS_WRITE_FILE_OPS(regw);

static ssize_t sf_wifi_rf_dbgfs_dumpreg_read(struct file *file,
        char __user *user_buf,
        size_t count,
        loff_t *ppos)
{
    struct rf_pl_context *pl_ctx = file->private_data;
    char *buf;
    int res, i;
    ssize_t read;
    size_t bufsz = 5120;
    uint16_t val = 0;

    /*everything is read out in one go*/
    if(*ppos)
        return 0;
    if(!pl_ctx)
        return 0;

    buf = kzalloc(bufsz, GFP_ATOMIC);
    if(buf == NULL)
        return 0;

    res = 0;
    for (i = pl_ctx->dbg_reg_addr; i < pl_ctx->dbg_reg_addr + 0x100; i++) {
        if ((i >= 0x2300) && (i <= 0x23be))
            val = ml_apb_rsbus_read(i);
        else
            val = ml_apb_read(i);
        res += scnprintf(&buf[res], min_t(size_t, bufsz - 1, count - res),
                "0x%04x 0x%04x\n", i, val);
    }

    read = simple_read_from_buffer(user_buf, count, ppos, buf, res);
    kfree(buf);

    return read;
}
DEBUGFS_READ_FILE_OPS(dumpreg);



static ssize_t sf_wifi_rf_dbgfs_cmd_write(struct file *file,
        const char __user *user_buf,
        size_t count,
        loff_t *ppos)
{

    struct rf_pl_context *pl_ctx = file->private_data;

    char *buf;
    char arg[8][16];
    char *token;
    uint16_t cmd_value;
    size_t len;
    size_t bufsz = 128;
    int arg_num = 0;
    int i = 0;
    uint8_t j = 0;
    uint16_t value = 0;
    uint16_t resp_args = 0;
    int arg0, arg1, arg2, arg3, arg4;
    int num;
    if (!pl_ctx)
        return -EFAULT;

    buf = (char *)kmalloc(bufsz, GFP_ATOMIC);
    if (!buf)
        return -EFAULT;

    len = min_t(size_t, count, bufsz - 1);

    if (copy_from_user(buf, user_buf, len)) {
        kfree(buf);
        return -EFAULT;
    }

    buf[len] = '\0';

    printk("sf_wifi_rf_dbgfs_cmd_write, %s\n", buf);

    token = strsep(&buf, " ");
    while (token) {
        strcpy(arg[arg_num], token);
        arg_num ++;
        token = strsep(&buf, " ");
    }

    printk(" totol arg number is %d\n", arg_num);
    for (; i < arg_num; i++) {
        printk(" arg[%d] : %s\n", i, arg[i]);
    }

    num = kstrtoint(arg[0], 10, &arg0);
    num = kstrtoint(arg[1], 10, &arg1);
    num = kstrtoint(arg[2], 10, &arg2);
    num = kstrtoint(arg[3], 10, &arg3);
    num = kstrtoint(arg[4], 10, &arg4);
    cmd_value = (uint16_t)arg0;
    printk("try to send cmd_value : 0x%x 0x%x 0x%x 0x%x 0x%x\n", arg0, arg1, arg2, arg3, arg4);

    if (arg_num == 1) {
        resp_args = ml_apb_send_0_params_cmd(1, ML_CMD_IF_p1_BASE, cmd_value);
    } else if (arg_num == 2) {
        resp_args = ml_apb_send_1_params_cmd(1, ML_CMD_IF_p1_BASE, cmd_value, (uint16_t)arg1);
    } else if (arg_num == 3) {
        resp_args = ml_apb_send_2_params_cmd(1, ML_CMD_IF_p1_BASE, cmd_value, (uint16_t)arg1, (uint16_t)arg2);
    } else if (arg_num == 4) {
        resp_args = ml_apb_send_3_params_cmd(1, ML_CMD_IF_p1_BASE, cmd_value, (uint16_t)arg1, (uint16_t)arg2, (uint16_t)arg3);
    } else if (arg_num == 5) {
        resp_args = ml_apb_send_4_params_cmd(1, ML_CMD_IF_p1_BASE, cmd_value, (uint16_t)arg1, (uint16_t)arg2, (uint16_t)arg3, (uint16_t)arg4);
    } else {
        printk("did not support command with arg number more than 4");
    }
    for (; j < resp_args; j++) {
        value = ml_apb_read(ML_CMD_IF_p1_BASE + 0x000B + j);
        printk("reps_data[%d] = 0x%x\n", j, value);
    }
    kfree(buf);

    return count;
}
DEBUGFS_WRITE_FILE_OPS(cmd);

#ifdef CONFIG_SFA28_FULLMASK

#define TS_UNINITIALIZED    0
#define TS_PARKED           1
#define TS_PREPARED         2
#define TS_TRANSMITTING     3
#define TS_RECEIVING        4

const char RF_TRS_STATUS[5][32] =
{
    "UNINITIALIZED",
    "PARKED",
    "PREPARED",
    "TRANSMITTING",
    "RECEIVING"
};

static ssize_t sf_wifi_rf_dbgfs_trx_status_read(struct file *file,
        char __user *user_buf,
        size_t count,
        loff_t *ppos)
{
    struct rf_pl_context *pl_ctx = file->private_data;
    char *buf;
    int res;
    ssize_t read;
    //IRQ uint32_t
    //band registered:
    size_t bufsz = 128;
    uint16_t trx_state_lb, freq_lb, chbw_lb;
    uint16_t trx_state_hb, freq_hb, chbw_hb;

    /*everything is read out in one go*/
    if(*ppos)
        return 0;
    if(!pl_ctx)
        return 0;

    bufsz = min_t(size_t, bufsz, count);
    buf = kmalloc(bufsz, GFP_ATOMIC);
    if(buf == NULL)
        return 0;

    bufsz--;
    rf_get_trx_status(false, &trx_state_lb, &freq_lb, &chbw_lb);
    rf_get_trx_status(true, &trx_state_hb, &freq_hb, &chbw_hb);
    res = scnprintf(buf, bufsz, "LB:[state=%s freq=%d MHZ chbw=%x]\nHB:[state=%s freq=%d MHZ chbw=%x]\n",
            RF_TRS_STATUS[trx_state_lb], freq_lb/10, chbw_lb,
            RF_TRS_STATUS[trx_state_hb], freq_hb/10, chbw_hb
            );
    read = simple_read_from_buffer(user_buf, count, ppos, buf, res);
    kfree(buf);

    return read;
}
DEBUGFS_READ_FILE_OPS(trx_status);
#endif

int sf_wifi_rf_sysfs_register(struct platform_device *pdev, char *parent)
{
    struct rf_pl_context *pl_ctx;
    struct dentry *dir_drv;
    struct dentry *dir_reg = NULL;
    pl_ctx = (struct rf_pl_context *)platform_get_drvdata(pdev);

    printk("%s, parent :%s\n", __func__, (parent == NULL) ? "NULL": parent);

    if(!parent)
        dir_drv = debugfs_create_dir("sf_wifi_rf", NULL);
    else
        dir_drv = debugfs_create_dir(parent, NULL);
    if(!dir_drv){
        printk("debug fs create directory failed!\n");
        goto err;
    }
    pl_ctx->debugfs = dir_drv;

    DEBUGFS_ADD_FILE(stats,  dir_drv, S_IRUSR);
    DEBUGFS_ADD_FILE(irqnum,  dir_drv, S_IRUSR);
    DEBUGFS_ADD_FILE(recalibrate,  dir_drv, S_IWUSR | S_IRUSR);
    DEBUGFS_ADD_FILE(temperature,  dir_drv, S_IRUSR);
    DEBUGFS_ADD_FILE(rf_hw_version,  dir_drv, S_IRUSR);
     DEBUGFS_ADD_FILE(rf_switch_trx,  dir_drv, S_IWUSR | S_IRUSR);
#ifdef CONFIG_SFA28_FULLMASK
	DEBUGFS_ADD_FILE(temperature_surface,  dir_drv, S_IRUSR);
//#ifdef CFG_DUAL_ANTENNA_CALIBRATE
    DEBUGFS_ADD_FILE(property,  dir_drv, S_IWUSR | S_IRUSR);
//#endif
#endif
#ifdef COOLING_TEMP
    DEBUGFS_ADD_FILE(cooling_temp, dir_drv, S_IWUSR | S_IRUSR);
    DEBUGFS_ADD_FILE(thermal_on, dir_drv, S_IRUSR);
#endif
    DEBUGFS_ADD_FILE(dumpstart_record, dir_drv, S_IWUSR);
    DEBUGFS_ADD_FILE(dumpcmd, dir_drv, S_IWUSR);
    DEBUGFS_ADD_FILE(cmd, dir_drv, S_IWUSR);

    dir_reg = debugfs_create_dir("reg", dir_drv);
    if(!dir_reg){
        printk("reg fs create directory failed!\n");
        goto err;
    }
    DEBUGFS_ADD_FILE(addr,  dir_reg, S_IWUSR | S_IRUSR);
    DEBUGFS_ADD_FILE(regr,  dir_reg, S_IWUSR | S_IRUSR);
    DEBUGFS_ADD_FILE(regw,  dir_reg, S_IWUSR | S_IRUSR);
    DEBUGFS_ADD_FILE(dumpreg, dir_reg, S_IRUSR);
    DEBUGFS_ADD_FILE(setxo, dir_drv, S_IWUSR);

#ifdef CONFIG_SFA28_FULLMASK
    DEBUGFS_ADD_FILE(trx_status,  dir_drv, S_IRUSR);
#endif
    return 0;
err:
    if(dir_reg)
        debugfs_remove_recursive(dir_reg);
    if(dir_drv)
        debugfs_remove_recursive(dir_drv);
    pl_ctx->debugfs = NULL;
    return -1;
}


int sf_wifi_rf_sysfs_unregister(struct platform_device *pdev)
{
    struct rf_pl_context *pl_ctx = (struct rf_pl_context *)platform_get_drvdata(pdev);
    printk("%s\n", __func__);
    if(pl_ctx == NULL){
        printk("invalid platform driver private data!\n");
        return -1;
    }
    if(!pl_ctx->debugfs){
        printk("already removed!\n");
        return -2;
    }
    debugfs_remove_recursive(pl_ctx->debugfs);
    pl_ctx->dbg_reg_addr = 0;

    return 0;
}
