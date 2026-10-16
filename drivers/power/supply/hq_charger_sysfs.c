#include <linux/init.h>		/* For init/exit macros */
#include <linux/module.h>	/* For MODULE_ marcros  */
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/platform_device.h>
#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/wait.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/poll.h>
#include <linux/power_supply.h>
#include <linux/pm_wakeup.h>
#include <linux/time.h>
#include <linux/mutex.h>
#include <linux/kthread.h>
#include <linux/proc_fs.h>
#include <linux/platform_device.h>
#include <linux/seq_file.h>
#include <linux/scatterlist.h>
#include <linux/suspend.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/reboot.h>

#include <asm/setup.h>
#include "mtk_charger.h"
#include "mtk_battery.h"
#include "tcpci_typec.h"
/* N17 code for HQ-292525 by tongjiacheng at 20230506 start */
#include "adapter_class.h"
/* N17 code for HQ-292525 by tongjiacheng at 20230506 end */

static int log_level = 2;

#define sysfs_err(fmt, ...)     \
do {                                    \
    if (log_level > 0)                  \
        printk(KERN_ERR "[hq_charger_sysfs]" fmt, ##__VA_ARGS__);   \
}while (0)

#define sysfs_info(fmt, ...)    \
do {                                    \
    if (log_level > 1)          \
        printk(KERN_ERR "[hq_charger_sysfs]" fmt, ##__VA_ARGS__);    \
}while (0)

#define sysfs_dbg(fmt, ...)    \
do {                                        \
    if (log_level >=2 )          \
        printk(KERN_ERR "[hq_charger_sysfs]" fmt, ##__VA_ARGS__);    \
}while (0)

#define SHUTDOWN_DELAY_VOL 3400

struct sysfs_desc {
	struct mtk_charger *info;
	struct mtk_battery_manager *bm;
	struct charger_device *chg_dev;

	struct tcpc_device *tcpc;

	struct power_supply *chg_psy;
	struct power_supply *bat_psy;
	struct power_supply *usb_psy;

	struct delayed_work shutdown_dwork;

	struct notifier_block tcpc_nb;
/*N17 code for HQ-299258 by tongjiacheng at 2023/7/5 start*/
	bool shutdown_delay;
/*N17 code for HQ-299258 by tongjiacheng at 2023/7/5 end*/

/* N17 code for HQ-308084 by liunianliang at 2023/7/31 start */
/* N17 code for HQ-308084 by liunianliang at 2023/7/31 end */
};

static struct sysfs_desc *g_desc;

static const char *const real_type_name[] = {
	"Unknown", "USB", "USB_CDP", "USB_FLOAT",
	"USB_DCP", "USB_HVDCP", "USB_PD",
};

static const char *const typec_mode_name[] = {
/*N17 code for HQHW-4214 by wangtingting at 2023/06/21 start*/
	"Nothing attached", "Source attached", "Sink attached",
/*N17 code for HQHW-4214 by wangtingting at 2023/06/21 end*/
	"Audio Adapter", "Debug Accessory",
};



static int shutdown_delay;
static void shutdown_delay_handler(struct work_struct *work)
{
	struct sysfs_desc *desc;
	struct battery_data *bs_data;

	char sd_str[32];
	char *envp[] = { sd_str, NULL };

	desc = container_of(to_delayed_work(work),
			    struct sysfs_desc, shutdown_dwork);

	bs_data = &desc->bm->bs_data;

	/* N17 code for HQ-290778 by tongjiacheng at 20230530 start */
	if (bs_data->bat_capacity <= 1) {
        /* N17 code for HQ-298644 by tongjiacheng at 20230608 start */
		if (bs_data->bat_batt_vol <= SHUTDOWN_DELAY_VOL &&
		bs_data->bat_status != POWER_SUPPLY_STATUS_CHARGING)
			shutdown_delay = true;
		else if (bs_data->bat_status == POWER_SUPPLY_STATUS_CHARGING &&
		shutdown_delay)
			shutdown_delay = false;
		else
			shutdown_delay = false;
        /* N17 code for HQ-298644 by tongjiacheng at 20230608 end */
	} else
		shutdown_delay = false;
/*N17 code for HQ-299258 by tongjiacheng at 2023/7/5 start*/
	if (desc->shutdown_delay != shutdown_delay) {
		sysfs_dbg("pre status:%d cur status: %d\n", 
				desc->shutdown_delay, shutdown_delay);

		sprintf(envp[0], "POWER_SUPPLY_SHUTDOWN_DELAY=%d",
			shutdown_delay);
		kobject_uevent_env(&desc->bat_psy->dev.kobj,
				   KOBJ_CHANGE, envp);

		desc->shutdown_delay  = shutdown_delay;
	}
/*N17 code for HQ-299258 by tongjiacheng at 2023/7/5 end */
	/* N17 code for HQ-290778 by tongjiacheng at 20230530 end */
	mod_delayed_work(system_wq,
			 &(desc->shutdown_dwork), msecs_to_jiffies(2000));
}

static ssize_t shutdown_delay_show(struct device *dev,
				   struct device_attribute *attr,
				   char *buf)
{
	return sprintf(buf, "%d\n", shutdown_delay);
}

static struct device_attribute shutdown_delay_attr =
__ATTR(shutdown_delay, 0444, shutdown_delay_show, NULL);

static int quick_chr_type;
static int quick_chr_type_noti(struct notifier_block *nb,
			       unsigned long event, void *v)
{
	int res = 0;
	char chr_type_str[64];
	char *envp[] = { chr_type_str, NULL };
/*N17 code for HQ-290781 by wangtingting at 2023/5/29 start*/
	struct sysfs_desc *desc;

	desc = container_of(nb,
					struct sysfs_desc, tcpc_nb);

	switch (desc->info->chr_type) {
	case POWER_SUPPLY_TYPE_USB:
	case POWER_SUPPLY_TYPE_USB_DCP:
	case POWER_SUPPLY_TYPE_USB_CDP:
		/* N17 code for HQ-308327 by liunianliang at 2023/7/31 */
		if (desc->info->ta_capability == APDO_TA ||
		    desc->info->ta_capability == WO_APDO_TA) {
			if (desc->info->pd_adapter && desc->info->pd_adapter->verifed)
				res = 3;
			else
				res = 2;
		}
		break;
	case POWER_SUPPLY_TYPE_USB_ACA:
		res = 1;
		break;
	default:
		break;
	}

	quick_chr_type = res;

	sprintf(envp[0], "POWER_SUPPLY_QUICK_CHARGE_TYPE=%d", res);
	kobject_uevent_env(&(desc->info->chg_psy->dev.kobj),
			   KOBJ_CHANGE, envp);
/*N17 code for HQ-290781 by wangtingting at 2023/5/29 end*/
	return 0;
}




static ssize_t quick_charge_type_show(struct device *dev,
				      struct device_attribute *attr,
				      char *buf)
{
	/* N17 code for HQ-291012 by tongjiacheng at 20230601 start */
	if (g_desc->info->chr_type == POWER_SUPPLY_TYPE_USB_ACA)
		quick_chr_type = 1;
	/* N17 code for HQ-291012 by tongjiacheng at 20230601 end */

	return sprintf(buf, "%d\n", quick_chr_type);
}

static struct device_attribute quick_charge_type_attr =
__ATTR(quick_charge_type, 0444, quick_charge_type_show, NULL);

static ssize_t real_type_show(struct device *dev,
			      struct device_attribute *attr, char *buf)
{
	int chr_type = CHARGER_UNKNOWN;

/*N17 code for HQHW-4214 by miaozhichao at 2023/06/21 start*/
	switch (g_desc->info->chr_type) {
	case POWER_SUPPLY_TYPE_UNKNOWN:
		chr_type = CHARGER_UNKNOWN;
		break;
/*N17 code for HQHW-4213 by miaozhichao at 2023/06/26 start*/
	case POWER_SUPPLY_TYPE_USB:
		if (g_desc->info->usb_type == POWER_SUPPLY_USB_TYPE_DCP &&
		    g_desc->info->protocol_state == RUN_ON_PD) {
			chr_type = PD_CHARGER;
		} else if (g_desc->info->usb_type == POWER_SUPPLY_USB_TYPE_DCP) {
			chr_type = NONSTANDARD_CHARGER;
		} else if (g_desc->info->protocol_state == RUN_ON_PD) {  /* N17 code for HQ-308327 by liunianliang at 2023/7/31, add APDO */
			chr_type = PD_CHARGER;
		} else
			chr_type = STANDARD_HOST;
		break;
/*N17 code for HQHW-4213 by miaozhichao at 2023/06/26 end*/
/*N17 code for HQHW-4214 by miaozhichao at 2023/06/21 end*/
	case POWER_SUPPLY_TYPE_USB_CDP:
		chr_type = CHARGING_HOST;
		break;
	case POWER_SUPPLY_TYPE_USB_DCP:
		chr_type = STANDARD_CHARGER;
		if (g_desc->info->protocol_state == RUN_ON_PD)
			chr_type = PD_CHARGER;
		break;
	case POWER_SUPPLY_TYPE_USB_ACA:
		chr_type = HVDCP_CHARGER;
		break;
	default:
		chr_type = NONSTANDARD_CHARGER;
	}

	return sprintf(buf, "%s\n", real_type_name[chr_type]);
}

static struct device_attribute real_type_attr =
__ATTR(real_type, 0444, real_type_show, NULL);

#if IS_ENABLED(CONFIG_TCPC_CLASS)
static int typec_cc_orientation_handler(struct tcpc_device *tcpc)
{
	int typec_cc_orientation = 0;

	tcpci_get_cc(tcpc);

	if (typec_get_cc1() == 0 && typec_get_cc2() == 0)
		typec_cc_orientation = 0;
	else if (typec_get_cc2() == 0)
		typec_cc_orientation = 1;
	else if (typec_get_cc1() == 0)
		typec_cc_orientation = 2;

	return typec_cc_orientation;
}

static ssize_t typec_cc_orientation_show(struct device *dev,
					 struct device_attribute *attr,
					 char *buf)
{
	struct sysfs_desc *desc;
	struct power_supply *psy;

	psy = dev_get_drvdata(dev);
	desc = power_supply_get_drvdata(psy);

	return sprintf(buf, "%d\n",
		       typec_cc_orientation_handler(desc->tcpc));
}

static struct device_attribute typec_cc_orientation_attr =
__ATTR(typec_cc_orientation, 0444, typec_cc_orientation_show, NULL);

static ssize_t typec_mode_show(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	struct sysfs_desc *desc;
	struct power_supply *psy;
	struct tcpc_device *tcpc;

	psy = dev_get_drvdata(dev);
	desc = power_supply_get_drvdata(psy);
	tcpc = desc->tcpc;
/* N17 code for HQ-308497 by miaozhichao at 20230722 start */
	if (tcpc->typec_attach_new > ARRAY_SIZE(typec_mode_name))
		return sprintf(buf, "%s\n", "Unknown");
/* N17 code for HQ-308497 by miaozhichao at 20230722 end */
	return sprintf(buf, "%s\n",
		       typec_mode_name[tcpc->typec_attach_new]);
}

static struct device_attribute typec_mode_attr =
__ATTR(typec_mode, 0444, typec_mode_show, NULL);
#endif				/* CONFIG_TCPC_CLASS */

/* N17 code for HQ-293951 by tongjiacheng at 20230504 start */
static ssize_t usb_otg_show(struct device *dev,
			    struct device_attribute *attr, char *buf)
{
	int res = 0;
#if IS_ENABLED(CONFIG_TCPC_CLASS)
	struct tcpc_device *tcpc;

	tcpc = g_desc->tcpc;

	if (tcpc->typec_attach_new == TYPEC_ATTACHED_SRC)
		res = 1;
#endif

	return sprintf(buf, "%d\n", res);
}

static struct device_attribute usb_otg_attr =
__ATTR(usb_otg, 0444, usb_otg_show, NULL);
/* N17 code for HQ-293951 by tongjiacheng at 20230504 end */
/* N17 code for HQ-294995 by miaozhichao at 20230525 start */
static ssize_t shipmode_count_reset_show(struct device *dev,
			    struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", g_desc->info->ship_mode);
}

static ssize_t shipmode_count_reset_store(struct device *dev,
				   struct device_attribute *attr,
				   const char *buf, size_t count)
{
	if (kstrtobool(buf, &g_desc->info->ship_mode)) {
		sysfs_err("parsing number fail\n");
		return -EINVAL;
	}
	return count;
}

static struct device_attribute shipmode_count_reset_attr =
__ATTR(shipmode_count_reset, 0644, shipmode_count_reset_show, shipmode_count_reset_store);

/* N17 code for HQ-294995 by miaozhichao at 20230525 end */

/* N17 code for HQ-292525 by tongjiacheng at 20230506 start */
static ssize_t input_suspend_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	bool enable = false;

	enable = g_desc->info->disable_charger;

	return sprintf(buf, "%d\n", enable);
}

static ssize_t input_suspend_store(struct device *dev,
				   struct device_attribute *attr,
				   const char *buf, size_t count)
{
	struct mtk_charger *info = g_desc->info;
	struct chg_alg_notify notify = {
		.evt = EVT_HARDRESET,
	};
	union power_supply_propval val;
	bool enable = false;
	int i, ret;

	if (kstrtobool(buf, &enable)) {
		sysfs_err("parsing number fail\n");
		return -EINVAL;
	}

	val.intval = enable;
	/* set disable charger value(1: disabel charger thread, 0: enable charger thread) */
	info->disable_charger = enable;

	/* set battery icon */
	ret = power_supply_set_property(info->chg_psy,
					POWER_SUPPLY_PROP_STATUS, &val);
	if (ret) {
		sysfs_err("set charger enable fail(%d)\n", ret);
		return ret;
	}

	/* set power path & wake up charger */
	ret = power_supply_set_property(info->psy1,
					POWER_SUPPLY_PROP_CHARGE_CONTROL_LIMIT,
					&val);
	if (ret) {
		sysfs_err("set power path fail(%d)\n", ret);
		return ret;
	}

	/*
	 * As stock 6.6 does: the fast charging algorithms stop when charging
	 * is suspended, and start over when it resumes.
	 */
	for (i = 0; i < MAX_ALG_NO; i++) {
		if (!info->alg[i])
			continue;
		if (enable)
			chg_alg_notifier_call(info->alg[i], &notify);
		else
			chg_alg_stop_algo(info->alg[i]);
	}

	/* notifier to mtk battery */
	power_supply_changed(info->psy1);

	return count;
}

static struct device_attribute input_suspend_attr =
__ATTR(input_suspend, 0644, input_suspend_show, input_suspend_store);
/* N17 code for HQ-292525 by tongjiacheng at 20230506 end */
/* N17 code for HQ-295595 by tongjiacheng at 20230509 start*/
static ssize_t chip_ok_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	int ret = 0;
	bool chip_ok_status = false;
	union power_supply_propval val;
	struct power_supply *psy = 
			power_supply_get_by_name("batt_verify");

	if (psy) {
		ret = power_supply_get_property(psy,
			POWER_SUPPLY_PROP_AUTHENTIC, &val);
		if (ret < 0)
			chip_ok_status = false;

		if (val.intval)
			chip_ok_status = true;
		else
			chip_ok_status = false;
	}
	else
		chip_ok_status = false;

	return sprintf(buf, "%d\n", chip_ok_status);
}

static struct device_attribute chip_ok_attr =
__ATTR(chip_ok, 0444, chip_ok_show, NULL);
/* N17 code for HQ-295595 by tongjiacheng at 20230509 end*/

/* N17 code for HQ-292265 by tongjiacheng at 20230515 start */
static ssize_t batt_id_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	int ret = 0;
	u8 id = 0;
	struct power_supply *psy;
	union power_supply_propval val;

	psy = power_supply_get_by_name("batt_verify");
	if (psy) {
		ret = power_supply_get_property(psy, POWER_SUPPLY_PROP_TYPE,
					&val);
		if (ret)
			id = 0xff;

		id = val.intval;
	}
	else
		id = 0xff;

	return sprintf(buf, "%d\n", id);
}

static struct device_attribute batt_id_attr =
__ATTR(batt_id, 0444, batt_id_show, NULL);
/* N17 code for HQ-292265 by tongjiacheng at 20230515 end */
/* N17 code for HQ-290782 by wangtingting at 2023/05/18 start*/
static ssize_t apdo_max_show(struct device *dev,
			    struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", g_desc->info->pd_adapter ?
		       g_desc->info->pd_adapter->apdo_max / 1000000 : 0);
}
static struct device_attribute apdo_max_attr =
__ATTR(apdo_max, 0444, apdo_max_show, NULL);
/* N17 code for HQ-290782 by wangtingting at 2023/05/18 end*/

/* N17 code for HQ-308327 by liunianliang at 2023/7/31 start */
static ssize_t pd_verifed_show(struct device *dev,
			    struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", g_desc->info->pd_adapter ?
		       g_desc->info->pd_adapter->verifed : 0);
}
static struct device_attribute pd_verifed_attr =
__ATTR(pd_verifed, 0444, pd_verifed_show, NULL);
/* N17 code for HQ-308327 by liunianliang at 2023/7/31 end */

/* The battery cell's maker, by the battery ID the authentication chip gives */
static const char *const batt_mode_name[] = { "COSMX", "NVT" };

static ssize_t model_name_show(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	struct power_supply *psy;
	union power_supply_propval val = { 0 };
	unsigned int id;

	psy = power_supply_get_by_name("batt_verify");
	if (!psy)
		return sprintf(buf, "%s\n", "Unknown");

	power_supply_get_property(psy, POWER_SUPPLY_PROP_TYPE, &val);
	power_supply_put(psy);
	id = val.intval & 0xff;
	if (id >= ARRAY_SIZE(batt_mode_name))
		return sprintf(buf, "%s\n", "Unknown");

	return sprintf(buf, "%s\n", batt_mode_name[id]);
}
static struct device_attribute model_name_attr =
__ATTR(model_name, 0444, model_name_show, NULL);

/* Xiaomi's MTBF test mode */
static ssize_t mtbf_mode_show(struct device *dev,
			      struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", g_desc->info->is_mtbf);
}

static ssize_t mtbf_mode_store(struct device *dev,
			       struct device_attribute *attr,
			       const char *buf, size_t count)
{
	int val = 0;

	if (kstrtoint(buf, 10, &val)) {
		sysfs_err("get buf error %s\n", buf);
		return -EINVAL;
	}
	g_desc->info->is_mtbf = !!val;
	sysfs_err("set val = %d, is_mtbf = %d\n", val, g_desc->info->is_mtbf);

	return count;
}
static struct device_attribute mtbf_mode_attr =
__ATTR(mtbf_mode, 0644, mtbf_mode_show, mtbf_mode_store);

static struct attribute *usb_psy_attrs[] = {
	&quick_charge_type_attr.attr,
	&real_type_attr.attr,
#if IS_ENABLED(CONFIG_TCPC_CLASS)
	&typec_cc_orientation_attr.attr,
	&typec_mode_attr.attr,
#endif				/* CONFIG_TCPC_CLASS */
/* N17 code for HQ-293951 by tongjiacheng at 20230504 start */
	&usb_otg_attr.attr,
/* N17 code for HQ-293951 by tongjiacheng at 20230504 end */
/* N17 code for HQ-290782 by wangtingting at 2023/05/18 start */
	&apdo_max_attr.attr,
/* N17 code for HQ-290782 by wangtingting at 2023/05/18 end */
/* N17 code for HQ-308327 by liunianliang at 2023/7/31 start */
	&pd_verifed_attr.attr,
/* N17 code for HQ-308327 by liunianliang at 2023/7/31 end */
	&mtbf_mode_attr.attr,
	NULL,
};

static struct attribute *bat_psy_attrs[] = {
	&shutdown_delay_attr.attr,
/* N17 code for HQ-292525 by tongjiacheng at 20230506 start */
	&input_suspend_attr.attr,
/* N17 code for HQ-292525 by tongjiacheng at 20230506 end */
/* N17 code for HQ-295595 by tongjiacheng at 20230509 start*/
	&chip_ok_attr.attr,
/* N17 code for HQ-295595 by tongjiacheng at 20230509 end*/
/* N17 code for HQ-292265 by tongjiacheng at 20230515 start */
	&batt_id_attr.attr,
/* N17 code for HQ-292265 by tongjiacheng at 20230515 end */
/* N17 code for HQ-294995 by miaozhichao at 20230525 start */
	&shipmode_count_reset_attr.attr,
/* N17 code for HQ-294995 by miaozhichao at 20230525 end */
	&model_name_attr.attr,
	NULL,
};

static const struct attribute_group usb_psy_group = {
	.attrs = usb_psy_attrs,
};

static const struct attribute_group bat_psy_group = {
	.attrs = bat_psy_attrs,
};

static int sysfs_setup_files(struct sysfs_desc *desc)
{
	int ret;

	if (!desc->usb_psy || !desc->bat_psy || !desc->info) {
		sysfs_err("%s find psy fail\n", __func__);
		ret = -EINVAL;
		goto _out;
	}

	ret = sysfs_create_group(&(desc->bat_psy->dev.kobj),
				 &bat_psy_group);
	if (ret) {
		sysfs_err("%s create battery node fail(%d)\n",
			  __func__, ret);
		goto _out;
	}

	ret = sysfs_create_group(&(desc->usb_psy->dev.kobj),
				 &usb_psy_group);
	if (ret) {
		sysfs_err("%s create usb node fail(%d)\n", __func__, ret);
		sysfs_remove_group(&(desc->bat_psy->dev.kobj), &bat_psy_group);
		goto _out;
	}

	return 0;

      _out:
	return ret;
}

static enum power_supply_property usb_psy_properties[] = {
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_TYPE,
};

static int psy_usb_get_property(struct power_supply *psy,
				enum power_supply_property psp,
				union power_supply_propval *val)
{
	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		/* N17 code for HQHW-4906 by p-gucheng at 20230812 */
		val->intval = charger_dev_get_online (g_desc->info->chg1_dev);
		break;
	case POWER_SUPPLY_PROP_TYPE:
		val->intval = psy->desc->type;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

struct power_supply_desc usb_psy_desc = {
	.name = "usb",
	.properties = usb_psy_properties,
	.num_properties = ARRAY_SIZE(usb_psy_properties),
	.type = POWER_SUPPLY_TYPE_USB,
	.get_property = psy_usb_get_property,
};

static int init_psy_tcpc(struct sysfs_desc *desc)
{
	struct power_supply_config cfg = {
		.drv_data = desc,
	};

	int ret;

	desc->usb_psy = power_supply_register(&(desc->info->pdev->dev),
					      &usb_psy_desc, &cfg);
	if (IS_ERR(desc->usb_psy)) {
		sysfs_err("%s register usb psy fail(%ld)\n", __func__,
			  PTR_ERR(desc->usb_psy));
		return PTR_ERR(desc->usb_psy);
	}

	desc->bat_psy = power_supply_get_by_name("battery");
	if (!desc->bat_psy) {
		sysfs_err("%s get battery psy fail\n", __func__);
		ret = -ENODEV;
		goto unregister_usb;
	}

	desc->tcpc = tcpc_dev_get_by_name("type_c_port0");
	if (!desc->tcpc) {
		sysfs_err("%s get typec device fail\n", __func__);
		ret = -ENODEV;
		goto put_bat;
	}

	/* the battery power supply belongs to the battery manager on 6.6 */
	desc->bm = power_supply_get_drvdata(desc->bat_psy);
	if (!desc->bm) {
		sysfs_err("%s get battery info fail\n", __func__);
		ret = -EINVAL;
		goto put_bat;
	}

	return 0;

put_bat:
	power_supply_put(desc->bat_psy);
unregister_usb:
	power_supply_unregister(desc->usb_psy);
	return ret;
}

static void release_psy_tcpc(struct sysfs_desc *desc)
{
	power_supply_put(desc->bat_psy);
	power_supply_unregister(desc->usb_psy);
}

static int __init hq_charger_sysfs_init(void)
{
	int ret;
	struct sysfs_desc *desc;
	struct power_supply *main_psy;

	sysfs_info("%s\n", __func__);

	desc = kzalloc(sizeof(struct sysfs_desc), GFP_KERNEL);
	if (!desc) {
		sysfs_err("%s alloc desc mem fail\n", __func__);
		ret = -ENOMEM;
		goto alloc_err;
	}

	main_psy = power_supply_get_by_name("mtk-master-charger");
	if (!main_psy) {
		sysfs_err("%s get main charger psy fail\n", __func__);
		ret = -EINVAL;
		goto main_psy_err;
	}

	desc->info = power_supply_get_drvdata(main_psy);
	power_supply_put(main_psy);

	ret = init_psy_tcpc(desc);
	if (ret)
		goto psy_tcpc_err;

	INIT_DELAYED_WORK(&(desc->shutdown_dwork), shutdown_delay_handler);
	schedule_delayed_work(&(desc->shutdown_dwork),
			      msecs_to_jiffies(2000));

	ret = sysfs_setup_files(desc);
	if (ret)
		goto sysfs_setup_err;

	desc->tcpc_nb.notifier_call = quick_chr_type_noti;
	ret = register_tcp_dev_notifier(desc->tcpc, &desc->tcpc_nb,
					TCP_NOTIFY_TYPE_USB |
					TCP_NOTIFY_TYPE_MISC |
					TCP_NOTIFY_TYPE_VBUS);
	if (ret < 0) {
		sysfs_err("%s register tcpc notifier fail(%d)\n", __func__,
			  ret);
		ret = -EINVAL;
		goto tcpc_noti_err;
	}

	g_desc = desc;

	return 0;

      tcpc_noti_err:
	sysfs_remove_group(&(desc->usb_psy->dev.kobj), &usb_psy_group);
	sysfs_remove_group(&(desc->bat_psy->dev.kobj), &bat_psy_group);
      sysfs_setup_err:
	cancel_delayed_work_sync(&(desc->shutdown_dwork));
	release_psy_tcpc(desc);
      psy_tcpc_err:
      main_psy_err:
	kfree(desc);
      alloc_err:
	return ret;
}

static void __exit hq_charger_sysfs_exit(void)
{
	struct sysfs_desc *desc = g_desc;

	if (!desc)
		return;
	unregister_tcp_dev_notifier(desc->tcpc, &desc->tcpc_nb,
				    TCP_NOTIFY_TYPE_USB |
				    TCP_NOTIFY_TYPE_MISC |
				    TCP_NOTIFY_TYPE_VBUS);
	sysfs_remove_group(&(desc->usb_psy->dev.kobj), &usb_psy_group);
	sysfs_remove_group(&(desc->bat_psy->dev.kobj), &bat_psy_group);
	cancel_delayed_work_sync(&(desc->shutdown_dwork));
	g_desc = NULL;
	release_psy_tcpc(desc);
	kfree(desc);
}

late_initcall_sync(hq_charger_sysfs_init);
module_exit(hq_charger_sysfs_exit);

MODULE_LICENSE("GPL");
