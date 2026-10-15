// SPDX-License-Identifier: GPL-2.0
/*
 * /sys/class/thermal/thermal_message: what Xiaomi's thermal daemon
 * (mi_thermald) reads and writes. It sets CPU frequency limits through PPM,
 * reports the board temperature, which is passed on to the charger, and keeps
 * the rest of its decisions here for others to read.
 */
#include <linux/arch_topology.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/kernfs.h>
#include <linux/kobject.h>
#include <linux/notifier.h>
#include <linux/sysfs.h>

#include "drivers/base/base.h"
#include "mtk_disp_notify.h"
#include "mtk_ppm_api.h"

/* From gold's charger framework (stock mtk_charger_framework.ko) */
#define THERMAL_BOARD_TEMP 0
extern int charger_notifier_call_cnain(unsigned long event, int val);

int thermal_message_init(void);
void thermal_message_exit(void);

#define BOOST_BUFFER_SIZE 128
#define BOARD_BUFFER_SIZE 128

struct thermal_device {
	struct device *dev;
	const struct class *class;
	struct attribute_group attrs;
};

static struct thermal_device thermal_dev;
static struct notifier_block screen_notifier;
static int screen_state;

static atomic_t temp_state = ATOMIC_INIT(0);
static atomic_t switch_mode = ATOMIC_INIT(-1);
static atomic_t modem_limit = ATOMIC_INIT(0);
static atomic_t balance_mode = ATOMIC_INIT(0);
static atomic_t poor_modem_limit = ATOMIC_INIT(0);
static atomic_t market_download_limit = ATOMIC_INIT(0);
static atomic_t thermal_max_brightness = ATOMIC_INIT(112);
static atomic_t wifi_limit = ATOMIC_INIT(0);
static atomic_t boost_limit = ATOMIC_INIT(0);

static char boost_buf[BOOST_BUFFER_SIZE];
static const char *board_sensor = "VIRTUAL-SENSOR";
static char board_sensor_temp[BOARD_BUFFER_SIZE];

#define THERMAL_INT_ATTR(name, var)					\
static ssize_t name##_show(struct device *dev,				\
			   struct device_attribute *attr, char *buf)	\
{									\
	return sysfs_emit(buf, "%d\n", atomic_read(&var));		\
}									\
static ssize_t name##_store(struct device *dev,				\
			    struct device_attribute *attr,		\
			    const char *buf, size_t len)		\
{									\
	int val;							\
									\
	if (kstrtoint(buf, 10, &val))					\
		return -EINVAL;						\
	atomic_set(&var, val);						\
	return len;							\
}									\
static DEVICE_ATTR_RW(name)

THERMAL_INT_ATTR(temp_state, temp_state);
THERMAL_INT_ATTR(sconfig, switch_mode);
THERMAL_INT_ATTR(modem_limit, modem_limit);
THERMAL_INT_ATTR(balance_mode, balance_mode);
THERMAL_INT_ATTR(poor_modem_limit, poor_modem_limit);
THERMAL_INT_ATTR(market_download_limit, market_download_limit);
THERMAL_INT_ATTR(thermal_max_brightness, thermal_max_brightness);
THERMAL_INT_ATTR(wifi_limit, wifi_limit);
THERMAL_INT_ATTR(boost_limit, boost_limit);

static ssize_t cpu_limits_show(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	return 0;
}

/* "cpu<n> <kHz>": the highest frequency of the cluster cpu<n> is in */
static ssize_t cpu_limits_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t len)
{
	unsigned int cpu, max;
	int cluster;

	if (sscanf(buf, "cpu%u %u", &cpu, &max) != 2 || cpu >= nr_cpu_ids)
		return -EINVAL;

	cluster = topology_cluster_id(cpu);
	if (cluster < 0)
		cluster = 0;

	mt_ppm_sysboost_set_freq_limit(BOOST_BY_XM_THERMAL, cluster, -1, max);
	return len;
}
static DEVICE_ATTR_RW(cpu_limits);

static ssize_t screen_state_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	return sysfs_emit(buf, "%d\n", screen_state);
}
static DEVICE_ATTR_RO(screen_state);

static ssize_t boost_show(struct device *dev,
			  struct device_attribute *attr, char *buf)
{
	return sysfs_emit(buf, "%s", boost_buf);
}

static ssize_t boost_store(struct device *dev,
			   struct device_attribute *attr,
			   const char *buf, size_t len)
{
	strscpy(boost_buf, buf, sizeof(boost_buf));
	return len;
}
static DEVICE_ATTR_RW(boost);

static ssize_t board_sensor_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	return sysfs_emit(buf, "%s", board_sensor);
}
static DEVICE_ATTR_RO(board_sensor);

static ssize_t board_sensor_temp_show(struct device *dev,
				      struct device_attribute *attr, char *buf)
{
	return sysfs_emit(buf, "%s", board_sensor_temp);
}

/* In millidegrees; the charger takes tenths of a degree */
static ssize_t board_sensor_temp_store(struct device *dev,
				       struct device_attribute *attr,
				       const char *buf, size_t len)
{
	long val;
	int ret;

	strscpy(board_sensor_temp, buf, sizeof(board_sensor_temp));

	if (kstrtol(strim(board_sensor_temp), 10, &val))
		return len;

	ret = charger_notifier_call_cnain(THERMAL_BOARD_TEMP, val / 100);
	if (ret)
		pr_err("%s: charger notifier: %d\n", __func__, ret);

	return len;
}
static DEVICE_ATTR_RW(board_sensor_temp);

static struct attribute *thermal_message_attrs[] = {
	&dev_attr_temp_state.attr,
	&dev_attr_cpu_limits.attr,
	&dev_attr_sconfig.attr,
	&dev_attr_screen_state.attr,
	&dev_attr_boost.attr,
	&dev_attr_board_sensor.attr,
	&dev_attr_modem_limit.attr,
	&dev_attr_board_sensor_temp.attr,
	&dev_attr_balance_mode.attr,
	&dev_attr_poor_modem_limit.attr,
	&dev_attr_market_download_limit.attr,
	&dev_attr_thermal_max_brightness.attr,
	&dev_attr_wifi_limit.attr,
	&dev_attr_boost_limit.attr,
	NULL,
};

static int screen_state_callback(struct notifier_block *nb,
				 unsigned long val, void *v)
{
	int *blank = v;

	if (val != MTK_DISP_EARLY_EVENT_BLANK && val != MTK_DISP_EVENT_BLANK)
		return NOTIFY_OK;

	switch (*blank) {
	case MTK_DISP_BLANK_UNBLANK:
		screen_state = 1;
		break;
	case MTK_DISP_BLANK_POWERDOWN:
		screen_state = 0;
		break;
	}

	if (thermal_dev.dev)
		sysfs_notify(&thermal_dev.dev->kobj, NULL, "screen_state");

	return NOTIFY_OK;
}

/* The thermal class is not exported; find it the way stock does, by its sysfs directory */
static const struct class *find_thermal_class(void)
{
	struct kernfs_node *class_kn, *thermal_kn;
	const struct class *cls = NULL;

	class_kn = kernfs_find_and_get(kernel_kobj->sd->parent, "class");
	if (!class_kn)
		return NULL;

	thermal_kn = kernfs_find_and_get(class_kn, "thermal");
	if (thermal_kn && thermal_kn->priv)
		cls = to_subsys_private((struct kobject *)thermal_kn->priv)->class;

	kernfs_put(thermal_kn);
	kernfs_put(class_kn);
	return cls;
}

static void create_thermal_message_node(void)
{
	int ret;

	thermal_dev.class = find_thermal_class();
	if (!thermal_dev.class) {
		pr_err("%s: can't find the thermal class\n", __func__);
		return;
	}

	thermal_dev.dev = device_create(thermal_dev.class, NULL, 'H', NULL,
					"thermal_message");
	if (IS_ERR(thermal_dev.dev)) {
		pr_err("%s: can't create the device: %ld\n", __func__,
		       PTR_ERR(thermal_dev.dev));
		thermal_dev.dev = NULL;
		return;
	}

	thermal_dev.attrs.attrs = thermal_message_attrs;
	ret = sysfs_create_group(&thermal_dev.dev->kobj, &thermal_dev.attrs);
	if (ret) {
		pr_err("%s: can't create the attributes: %d\n", __func__, ret);
		device_destroy(thermal_dev.class, 'H');
		thermal_dev.dev = NULL;
	}
}

int thermal_message_init(void)
{
	int ret;

	screen_notifier.notifier_call = screen_state_callback;
	ret = mtk_disp_notifier_register("thermal_message", &screen_notifier);
	if (ret)
		pr_err("%s: can't register the screen notifier: %d\n",
		       __func__, ret);

	create_thermal_message_node();
	return 0;
}

void thermal_message_exit(void)
{
	mtk_disp_notifier_unregister(&screen_notifier);
	if (thermal_dev.dev) {
		sysfs_remove_group(&thermal_dev.dev->kobj, &thermal_dev.attrs);
		device_destroy(thermal_dev.class, 'H');
		thermal_dev.dev = NULL;
	}
}
