/*
 *
 * FocalTech ftxxxx TouchScreen driver.
 *
 * Copyright (c) 2012-2020, Focaltech Ltd. All rights reserved.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

/*****************************************************************************
*
* File Name: focaltech_ex_mode.c
*
* Author: Focaltech Driver Team
*
* Created: 2016-08-31
*
* Abstract:
*
* Reference:
*
*****************************************************************************/

/*****************************************************************************
* 1.Included header files
*****************************************************************************/
#include "focaltech_core.h"
#include <linux/hq_notifier.h>

/* N17 code for HQ-291711 by liunianliang at 2023/06/03 start */
/* N17 code for HQ-291711 by liunianliang at 2023/06/03 end */

/*****************************************************************************
* 2.Private constant and macro definitions using #define
*****************************************************************************/

/*****************************************************************************
* 3.Private enumerations, structures and unions using typedef
*****************************************************************************/
enum _ex_mode {
    MODE_GLOVE = 0,
    MODE_COVER,
    MODE_CHARGER,
    /* N17 code for HQ-291087 by liunianliang at 2023/5/29 start */
    MODE_EARPHONE,
    MODE_EDGE,
    /* N17 code for HQ-291087 by liunianliang at 2023/5/29 end */
};

/*****************************************************************************
* 4.Static variables
*****************************************************************************/

/*****************************************************************************
* 5.Global variable or extern global variabls/functions
*****************************************************************************/

/*****************************************************************************
* 6.Static function prototypes
*******************************************************************************/
/* N17 code for HQ-291087 by liunianliang at 2023/5/29 start */
static int fts_ex_mode_switch(enum _ex_mode mode, u8 value)
{
    int ret = 0;
    u8 m_val = 0;

    if (value)
        m_val = value;//0x01;
    else
        m_val = 0x00;

    switch (mode) {
    case MODE_GLOVE:
        ret = fts_write_reg(FTS_REG_GLOVE_MODE_EN, m_val);
        if (ret < 0) {
            FTS_ERROR("MODE_GLOVE switch to %d fail", m_val);
        }
        break;
    case MODE_COVER:
        ret = fts_write_reg(FTS_REG_COVER_MODE_EN, m_val);
        if (ret < 0) {
            FTS_ERROR("MODE_COVER switch to %d fail", m_val);
        }
        break;
    case MODE_CHARGER:
        ret = fts_write_reg(FTS_REG_CHARGER_MODE_EN, m_val);
        if (ret < 0) {
            FTS_ERROR("MODE_CHARGER switch to %d fail", m_val);
        }
        break;
    case MODE_EARPHONE:
        ret = fts_write_reg(FTS_REG_EARPHONE_MODE_EN, m_val);
        if (ret < 0) {
            FTS_ERROR("MODE_EARPHONE switch to %d fail", m_val);
        }
        break;
    case MODE_EDGE:
        ret = fts_write_reg(FTS_REG_EDGE_MODE_EN, m_val);
        if (ret < 0) {
            FTS_ERROR("MODE_EDGE switch to %d fail", m_val);
        }
        break;
    default:
        FTS_ERROR("mode(%d) unsupport", mode);
        ret = -EINVAL;
        break;
    }

    return ret;
}
/* N17 code for HQ-291087 by liunianliang at 2023/5/29 end */

static ssize_t fts_glove_mode_show(
    struct device *dev, struct device_attribute *attr, char *buf)
{
    int count = 0;
    u8 val = 0;
    struct fts_ts_data *ts_data = dev_get_drvdata(dev);
    struct input_dev *input_dev = ts_data->input_dev;

    mutex_lock(&input_dev->mutex);
    fts_read_reg(FTS_REG_GLOVE_MODE_EN, &val);
    count = snprintf(buf + count, PAGE_SIZE, "Glove Mode:%s\n",
                     ts_data->glove_mode ? "On" : "Off");
    count += snprintf(buf + count, PAGE_SIZE, "Glove Reg(0xC0):%d\n", val);
    mutex_unlock(&input_dev->mutex);

    return count;
}

static ssize_t fts_glove_mode_store(
    struct device *dev,
    struct device_attribute *attr, const char *buf, size_t count)
{
    int ret = 0;
    struct fts_ts_data *ts_data = dev_get_drvdata(dev);

    if (FTS_SYSFS_ECHO_ON(buf)) {
        if (!ts_data->glove_mode) {
            FTS_DEBUG("enter glove mode");
            ret = fts_ex_mode_switch(MODE_GLOVE, ENABLE);
            if (ret >= 0) {
                ts_data->glove_mode = ENABLE;
            }
        }
    } else if (FTS_SYSFS_ECHO_OFF(buf)) {
        if (ts_data->glove_mode) {
            FTS_DEBUG("exit glove mode");
            ret = fts_ex_mode_switch(MODE_GLOVE, DISABLE);
            if (ret >= 0) {
                ts_data->glove_mode = DISABLE;
            }
        }
    }

    FTS_DEBUG("glove mode:%d", ts_data->glove_mode);
    return count;
}


static ssize_t fts_cover_mode_show(
    struct device *dev, struct device_attribute *attr, char *buf)
{
    int count = 0;
    u8 val = 0;
    struct fts_ts_data *ts_data = dev_get_drvdata(dev);
    struct input_dev *input_dev = ts_data->input_dev;

    mutex_lock(&input_dev->mutex);
    fts_read_reg(FTS_REG_COVER_MODE_EN, &val);
    count = snprintf(buf + count, PAGE_SIZE, "Cover Mode:%s\n",
                     ts_data->cover_mode ? "On" : "Off");
    count += snprintf(buf + count, PAGE_SIZE, "Cover Reg(0xC1):%d\n", val);
    mutex_unlock(&input_dev->mutex);

    return count;
}

static ssize_t fts_cover_mode_store(
    struct device *dev,
    struct device_attribute *attr, const char *buf, size_t count)
{
    int ret = 0;
    struct fts_ts_data *ts_data = dev_get_drvdata(dev);

    if (FTS_SYSFS_ECHO_ON(buf)) {
        if (!ts_data->cover_mode) {
            FTS_DEBUG("enter cover mode");
            ret = fts_ex_mode_switch(MODE_COVER, ENABLE);
            if (ret >= 0) {
                ts_data->cover_mode = ENABLE;
            }
        }
    } else if (FTS_SYSFS_ECHO_OFF(buf)) {
        if (ts_data->cover_mode) {
            FTS_DEBUG("exit cover mode");
            ret = fts_ex_mode_switch(MODE_COVER, DISABLE);
            if (ret >= 0) {
                ts_data->cover_mode = DISABLE;
            }
        }
    }

    FTS_DEBUG("cover mode:%d", ts_data->cover_mode);
    return count;
}

static ssize_t fts_charger_mode_show(
    struct device *dev, struct device_attribute *attr, char *buf)
{
    int count = 0;
    u8 val = 0;
    struct fts_ts_data *ts_data = dev_get_drvdata(dev);
    struct input_dev *input_dev = ts_data->input_dev;

    mutex_lock(&input_dev->mutex);
    fts_read_reg(FTS_REG_CHARGER_MODE_EN, &val);
    count = snprintf(buf + count, PAGE_SIZE, "Charger Mode:%s\n",
                     ts_data->charger_mode ? "On" : "Off");
    count += snprintf(buf + count, PAGE_SIZE, "Charger Reg(0x8B):%d\n", val);
    mutex_unlock(&input_dev->mutex);

    return count;
}

static ssize_t fts_charger_mode_store(
    struct device *dev,
    struct device_attribute *attr, const char *buf, size_t count)
{
    int ret = 0;
    struct fts_ts_data *ts_data = dev_get_drvdata(dev);

    if (FTS_SYSFS_ECHO_ON(buf)) {
        if (!ts_data->charger_mode) {
            FTS_DEBUG("enter charger mode");
            ret = fts_ex_mode_switch(MODE_CHARGER, ENABLE);
            if (ret >= 0) {
                ts_data->charger_mode = ENABLE;
            }
        }
    } else if (FTS_SYSFS_ECHO_OFF(buf)) {
        if (ts_data->charger_mode) {
            FTS_DEBUG("exit charger mode");
            ret = fts_ex_mode_switch(MODE_CHARGER, DISABLE);
            if (ret >= 0) {
                ts_data->charger_mode = DISABLE;
            }
        }
    }

    FTS_DEBUG("charger mode:%d", ts_data->charger_mode);
    return count;
}

/* N17 code for HQ-291087 by liunianliang at 2023/5/29 start */
static ssize_t fts_earphone_mode_show(
    struct device *dev, struct device_attribute *attr, char *buf)
{
    int count = 0;
    u8 val = 0;
    struct fts_ts_data *ts_data = dev_get_drvdata(dev);
    struct input_dev *input_dev = ts_data->input_dev;

    mutex_lock(&input_dev->mutex);
    fts_read_reg(FTS_REG_EARPHONE_MODE_EN, &val);
    count = snprintf(buf + count, PAGE_SIZE, "Earphone Mode:%s\n",
                     ts_data->earphone_mode ? "On" : "Off");
    count += snprintf(buf + count, PAGE_SIZE, "Earphone Reg(0xC3):%d\n", val);
    mutex_unlock(&input_dev->mutex);

    return count;
}

static ssize_t fts_earphone_mode_store(
    struct device *dev,
    struct device_attribute *attr, const char *buf, size_t count)
{
    int ret = 0;
    struct fts_ts_data *ts_data = dev_get_drvdata(dev);

    if (FTS_SYSFS_ECHO_ON(buf)) {
        if (!ts_data->earphone_mode) {
            FTS_DEBUG("enter earphone mode");
            ret = fts_ex_mode_switch(MODE_EARPHONE, ENABLE);
            if (ret >= 0) {
                ts_data->earphone_mode = ENABLE;
            }
        }
    } else if (FTS_SYSFS_ECHO_OFF(buf)) {
        if (ts_data->earphone_mode) {
            FTS_DEBUG("exit earphone mode");
            ret = fts_ex_mode_switch(MODE_EARPHONE, DISABLE);
            if (ret >= 0) {
                ts_data->earphone_mode = DISABLE;
            }
        }
    }

    FTS_DEBUG("earphone mode:%d", ts_data->earphone_mode);
    return count;
}

static ssize_t fts_edge_mode_show(
    struct device *dev, struct device_attribute *attr, char *buf)
{
    int count = 0;
    u8 val = 0;
    struct fts_ts_data *ts_data = dev_get_drvdata(dev);
    struct input_dev *input_dev = ts_data->input_dev;

    mutex_lock(&input_dev->mutex);
    fts_read_reg(FTS_REG_EDGE_MODE_EN, &val);
    count = snprintf(buf + count, PAGE_SIZE, "Edge Mode:%s\n",
                     ts_data->edge_mode ? "On" : "Off");
    count += snprintf(buf + count, PAGE_SIZE, "Edge Reg(0x8C):%d\n", val);
    mutex_unlock(&input_dev->mutex);

    return count;
}

static ssize_t fts_edge_mode_store(
    struct device *dev,
    struct device_attribute *attr, const char *buf, size_t count)
{
    int ret = 0;
    struct fts_ts_data *ts_data = dev_get_drvdata(dev);

/* N17 code for HQ-299540 by jiangyue at 2023/6/9 start */
    if (FTS_SYSFS_ECHO_ON(buf)) {
        if (ts_data->edge_mode == 0 || ts_data->edge_mode == 1 || ts_data->edge_mode == 2){
/* N17 code for HQ-299540 by jiangyue at 2023/6/9 end */
            FTS_DEBUG("enter edge mode");
            if (buf[0] == '1') /*USB PORTS RIGHT*/ {
	            ret = fts_ex_mode_switch(MODE_EDGE, 1);
	            if (ret >= 0) {
	                ts_data->edge_mode = 1;
	            }
            }
            else if (buf[0] == '2') /*USB PORTS LEFT*/ {
	            ret = fts_ex_mode_switch(MODE_EDGE, 2);
	            if (ret >= 0) {
	                ts_data->edge_mode = 2;
	            }
            }
        }
    } else if (FTS_SYSFS_ECHO_OFF(buf)) {
        if (ts_data->edge_mode) {
            FTS_DEBUG("exit edge mode");
            ret = fts_ex_mode_switch(MODE_EDGE, DISABLE);
            if (ret >= 0) {
                ts_data->edge_mode = DISABLE;
            }
        }
    }

    FTS_DEBUG("edge mode:%d", ts_data->edge_mode);
    return count;
}
/* N17 code for HQ-291087 by liunianliang at 2023/5/29 end */

/* read and write charger mode
 * read example: cat fts_glove_mode        ---read  glove mode
 * write example:echo 1 > fts_glove_mode   ---write glove mode to 01
 */
static DEVICE_ATTR(fts_glove_mode, S_IRUGO | S_IWUSR,
                   fts_glove_mode_show, fts_glove_mode_store);

static DEVICE_ATTR(fts_cover_mode, S_IRUGO | S_IWUSR,
                   fts_cover_mode_show, fts_cover_mode_store);

static DEVICE_ATTR(fts_charger_mode, S_IRUGO | S_IWUSR,
                   fts_charger_mode_show, fts_charger_mode_store);

/* N17 code for HQ-291087 by liunianliang at 2023/5/29 start */
static DEVICE_ATTR(fts_earphone_mode, S_IRUGO | S_IWUSR,
                   fts_earphone_mode_show, fts_earphone_mode_store);

static DEVICE_ATTR(fts_edge_mode, S_IRUGO | S_IWUSR,
                   fts_edge_mode_show, fts_edge_mode_store);
/* N17 code for HQ-291087 by liunianliang at 2023/5/29 end */

static struct attribute *fts_touch_mode_attrs[] = {
    &dev_attr_fts_glove_mode.attr,
    &dev_attr_fts_cover_mode.attr,
    &dev_attr_fts_charger_mode.attr,
    /* N17 code for HQ-291087 by liunianliang at 2023/5/29 start */
    &dev_attr_fts_earphone_mode.attr,
    &dev_attr_fts_edge_mode.attr,
    /* N17 code for HQ-291087 by liunianliang at 2023/5/29 end */
    NULL,
};

static struct attribute_group fts_touch_mode_group = {
    .attrs = fts_touch_mode_attrs,
};

int fts_ex_mode_recovery(struct fts_ts_data *ts_data)
{
    if (ts_data->glove_mode) {
        fts_ex_mode_switch(MODE_GLOVE, ENABLE);
    }

    if (ts_data->cover_mode) {
        fts_ex_mode_switch(MODE_COVER, ENABLE);
    }

    if (ts_data->charger_mode) {
        fts_ex_mode_switch(MODE_CHARGER, ENABLE);
    }

    /* N17 code for HQ-291087 by liunianliang at 2023/5/29 start */
    if (ts_data->earphone_mode) {
        fts_ex_mode_switch(MODE_EARPHONE, ENABLE);
    }

    if (ts_data->edge_mode) {
        fts_ex_mode_switch(MODE_EDGE, ts_data->edge_mode);
    }
    /* N17 code for HQ-291087 by liunianliang at 2023/5/29 end */

    return 0;
}

/*
 * Charger and headset state from Huaqin's hq_notifier, as stock gold's 6.6
 * build has it: the notifier only records the state and queues a work, which
 * sets the panel's charger or headset mode.
 */
static int fts_headset_flag;
static int fts_charger_flag;

void fts_headset_mode_work(struct work_struct *work)
{
    if (fts_headset_flag == 1) {
        fts_write_reg(FTS_REG_EARPHONE_MODE_EN, 1);
        FTS_INFO("fts in headset mode");
    } else if (fts_headset_flag == 0) {
        fts_write_reg(FTS_REG_EARPHONE_MODE_EN, 0);
        FTS_INFO("fts out headset mode");
    }
}

int fts_charger_mode_set(void)
{
    int ret = 0;
    u8 mode = (fts_charger_flag == 1) ? 1 : 0;

    FTS_INFO("fts charger mode:%d", mode);
    ret = fts_write_reg(FTS_REG_CHARGER_MODE_EN, mode);
    if (ret < 0)
        FTS_ERROR("MODE_CHARGER switch to %d fail", mode);
    return ret;
}

void fts_charger_mode_work(struct work_struct *work)
{
    if (fts_charger_mode_set() < 0)
        FTS_ERROR("fts_charger_mode_work err");
}

static int hq_state_notifier_callback(struct notifier_block *nb,
                                      unsigned long event, void *data)
{
    int *status = data;

    if (!status) {
        FTS_ERROR("%s hq_notify_data is NULL!", __func__);
        return 0;
    }

    switch (event) {
    case HQ_NOTIFY_USB:
        fts_charger_flag = *status;
        FTS_INFO("%s fts_charger_flag is %d!", __func__, fts_charger_flag);
        queue_work(fts_data->fts_charger_wq, &fts_data->charger_work);
        break;
    case HQ_NOTIFY_EARPHONE:
    case HQ_NOTIFY_TYPEC_EARPHONE:
        fts_headset_flag = *status;
        FTS_INFO("%s fts_headset_flag is %d!", __func__, fts_headset_flag);
        queue_work(fts_data->fts_headset_wq, &fts_data->headset_work);
        break;
    default:
        break;
    }

    return NOTIFY_OK;
}

int fts_ex_mode_init(struct fts_ts_data *ts_data)
{
    int ret = 0;

    ts_data->glove_mode = DISABLE;
    ts_data->cover_mode = DISABLE;
    ts_data->charger_mode = DISABLE;
    /* N17 code for HQ-291087 by liunianliang at 2023/5/29 start */
    ts_data->earphone_mode = DISABLE;
    ts_data->edge_mode = DISABLE;
    /* N17 code for HQ-291087 by liunianliang at 2023/5/29 end */

    ret = sysfs_create_group(&ts_data->dev->kobj, &fts_touch_mode_group);
    if (ret < 0) {
        FTS_ERROR("create sysfs(ex_mode) fail");
        sysfs_remove_group(&ts_data->dev->kobj, &fts_touch_mode_group);
        return ret;
    } else {
        FTS_DEBUG("create sysfs(ex_mode) successfully");
    }

    ts_data->hq_nb.notifier_call = hq_state_notifier_callback;
    ret = register_hq_notify(&ts_data->hq_nb);
    if (ret < 0) {
        FTS_ERROR("register earphone mode notifier fail");
        return -EINVAL;
    }

    return 0;
}

int fts_ex_mode_exit(struct fts_ts_data *ts_data)
{
    sysfs_remove_group(&ts_data->dev->kobj, &fts_touch_mode_group);
    return 0;
}
