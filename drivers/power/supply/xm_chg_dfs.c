// SPDX-License-Identifier: GPL-2.0
/**
 * Copyright (c) 2025 Huaqin Technology(Shanghai) Co., Ltd.
 */

#include <linux/string.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/stddef.h>
#include <linux/kthread.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/power_supply.h>
#include <linux/time.h>
#include <linux/err.h>

#include "xm_chg_dfs.h"

#define CHG_DFX_EVT_DECLARE(type, stat, err, cnt_limit) \
{\
	.dfx_type = CHG_DFX_##type, \
	.first_report = true, \
	.is_stat = stat, \
	.is_err = err, \
	.report_cnt = 0, \
	.report_cnt_limit = cnt_limit, \
}

static struct xm_chg_dfx_evt dfx_evt_cond[] = {
	CHG_DFX_EVT_DECLARE(PD_AUTH_FAIL, false, true, 1),
	CHG_DFX_EVT_DECLARE(CP_EN_FAIL, false, true, 3),
	CHG_DFX_EVT_DECLARE(NONE_STANDARD_CHG, false, true, 1),
	CHG_DFX_EVT_DECLARE(CORROSION_DISCHARGE, false, true, 3),
	CHG_DFX_EVT_DECLARE(CP_IBUS_OCP, false, true, 3),
	CHG_DFX_EVT_DECLARE(CP_IBAT_OCP, false, true, 3),
	CHG_DFX_EVT_DECLARE(CP_VBUS_OVP, false, true, 3),
	CHG_DFX_EVT_DECLARE(CP_VBAT_OVP, false, true, 3),
	CHG_DFX_EVT_DECLARE(CHG_BATT_CYCLE, true, false, 65535),
	CHG_DFX_EVT_DECLARE(SOC_NOT_FULL, false, true, 1),
	CHG_DFX_EVT_DECLARE(FG_I2C_ERR, false, true, 3),
	CHG_DFX_EVT_DECLARE(CP_I2C_ERR, false, true, 3),
	CHG_DFX_EVT_DECLARE(BATT_LINKER_ABSENT, false, true, 3),
	CHG_DFX_EVT_DECLARE(CP_TDIE_HOT, false, true, 3),
	CHG_DFX_EVT_DECLARE(VBUS_UVLO, false, true, 3),
	CHG_DFX_EVT_DECLARE(NOT_CHG_IN_LOW_TEMP, false, true, 3),
	CHG_DFX_EVT_DECLARE(NOT_CHG_IN_HIGH_TEMP, false, true, 3),
	CHG_DFX_EVT_DECLARE(BATT_AUTH_FAIL, false, true, 1),
	CHG_DFX_EVT_DECLARE(TBAT_HOT, false, false, 3),
	CHG_DFX_EVT_DECLARE(TBAT_COLD, false, false, 3),
};

void mievent_upload(int miev_code, char *miev_param, int para_cnt)
{
	char buffer[128] = {0};
	char *p1, *p2, *key, *value;
	int intval;
	struct misight_mievent *event = cdev_tevent_alloc(miev_code);

	memcpy(buffer, miev_param, strlen(miev_param));

	pr_info("[HQ_CHG_DFS] miev_code=%d para_cnt=%d\n", miev_code, para_cnt);

	p1 = buffer;
	while (p1 && (*p1 != '\0')) {
		p2 = strsep(&p1, ",");
		while (p2 && (*p2 != '\0')) {
			key = strsep(&p2, ":");
			if (key) {
				pr_info("[CHG_DFS] key:%s\n", key);
			} else {
				pr_err("[CHG_DFS] none key\n");
				key = "None";
			}

			value = strsep(&p2, ":");
			if (value) {
				pr_info("[CHG_DFS] value:%s\n", value);
			} else {
				pr_err("[CHG_DFS] none value\n");
				value = "None";
			}
		}

		if (kstrtoint(value, 10, &intval) != 0) {
			cdev_tevent_add_str(event, key, value);
			pr_info("type=string, %s:%s\n", key, value);
		} else {
			cdev_tevent_add_int(event, key, intval);
			pr_info("type=int, %s:%d\n", key, intval);
		}
	}

	cdev_tevent_write(event);
	cdev_tevent_destroy(event);

	return;
}

static void xm_chg_dfx_handle_event_report(struct xm_chg_dfs *chg_dfs)
{
	char para_str[PARAMETER_LEN_MAX] = {0};
	int len = 0;

	pr_info("[HQ_CHG_DFS] dfx_type = %d\n", chg_dfs->dfx_type);

	switch (chg_dfs->dfx_type) {
	case CHG_DFX_PD_AUTH_FAIL:
		len += scnprintf((para_str+len), (PARAMETER_LEN_MAX-len), "%s:%s", "chgErrInfo", "pdAuthFail");
		len += scnprintf((para_str+len), (PARAMETER_LEN_MAX-len), ",%s:%x", "adapterId", chg_dfs->adapter_svid);
		mievent_upload(DFX_ID_CHG_PD_AUTH_FAIL, para_str, 2);
		break;

	case CHG_DFX_CP_EN_FAIL:
		len += scnprintf((para_str+len), (PARAMETER_LEN_MAX-len), "%s:%s", "chgErrInfo", "CpEnFail");
		mievent_upload(DFX_ID_CHG_CP_EN_FAIL, para_str, 1);
		break;

	case CHG_DFX_NONE_STANDARD_CHG:
		len += scnprintf((para_str+len), (PARAMETER_LEN_MAX-len), "%s:%s", "chgErrInfo", "noneStandartChg");
		mievent_upload(DFX_ID_CHG_NONE_STANDARD_CHG, para_str, 1);
		break;

	case CHG_DFX_CORROSION_DISCHARGE:
		len += scnprintf((para_str+len), (PARAMETER_LEN_MAX-len), "%s:%s", "chgErrInfo", "CorrosionDischarge");
		mievent_upload(DFX_ID_CHG_CORROSION_DISCHARGE, para_str, 1);
		break;

	case CHG_DFX_BATT_LINKER_ABSENT:
		len += scnprintf((para_str+len), (PARAMETER_LEN_MAX-len), "%s:%s", "chgErrInfo", "BattLinkerAbsent");
		mievent_upload(DFX_ID_CHG_BATT_LINKER_ABSENT, para_str, 1);
		break;

	case CHG_DFX_NOT_CHG_IN_LOW_TEMP:
		len += scnprintf((para_str+len), (PARAMETER_LEN_MAX-len), "%s:%s", "chgErrInfo", "NotChgInLowTemp");
		len += scnprintf((para_str+len), (PARAMETER_LEN_MAX-len), ",%s:%d", "tbat", chg_dfs->tbat);
		mievent_upload(DFX_ID_CHG_NOT_CHG_IN_LOW_TEMP, para_str, 2);
		break;

	case CHG_DFX_CP_VBUS_OVP:
		len += scnprintf((para_str+len), (PARAMETER_LEN_MAX-len), "%s:%s", "chgErrInfo", "CpVbusOvp");
		mievent_upload(DFX_ID_CHG_CP_VBUS_OVP, para_str, 1);
		break;

	case CHG_DFX_CP_IBUS_OCP:
		len += scnprintf((para_str+len), (PARAMETER_LEN_MAX-len), "%s:%s", "chgErrInfo", "CpIbusOcp");
		mievent_upload(DFX_ID_CHG_CP_IBUS_OCP, para_str, 1);
		break;

	case CHG_DFX_CP_VBAT_OVP:
		len += scnprintf((para_str+len), (PARAMETER_LEN_MAX-len), "%s:%s", "chgErrInfo", "CpVbatOvp");
		mievent_upload(DFX_ID_CHG_CP_VBAT_OVP, para_str, 1);
		break;

	case CHG_DFX_CP_IBAT_OCP:
		len += scnprintf((para_str+len), (PARAMETER_LEN_MAX-len), "%s:%s", "chgErrInfo", "CpIbatOcp");
		mievent_upload(DFX_ID_CHG_CP_IBAT_OCP, para_str, 1);
		break;

	case CHG_DFX_CHG_BATT_CYCLE:
		len += scnprintf((para_str+len), (PARAMETER_LEN_MAX-len), "%s:%s", "chgStatInfo", "chgBattCycle");
		len += scnprintf((para_str+len), (PARAMETER_LEN_MAX-len), ",%s:%d", "CycleCnt", chg_dfs->cycle_cnt);
		mievent_upload(DFX_ID_CHG_CHG_BATT_CYCLE, para_str, 2);
		break;

	case CHG_DFX_VBUS_UVLO:
		len += scnprintf((para_str+len), (PARAMETER_LEN_MAX-len), "%s:%s", "chgStatInfo", "VbusUvlo");
		len += scnprintf((para_str+len), (PARAMETER_LEN_MAX-len), ",%s:%d", "vbus", chg_dfs->vbus);
		mievent_upload(DFX_ID_CHG_VBUS_UVLO, para_str, 2);
		break;

	case CHG_DFX_SOC_NOT_FULL:
		len += scnprintf((para_str+len), (PARAMETER_LEN_MAX-len), "%s:%s", "chgErrInfo", "SocNotFull");
		len += scnprintf((para_str+len), (PARAMETER_LEN_MAX-len), ",%s:%d", "vbat", chg_dfs->vbat);
		len += scnprintf((para_str+len), (PARAMETER_LEN_MAX-len), ",%s:%d", "soc", chg_dfs->soc);
		len += scnprintf((para_str+len), (PARAMETER_LEN_MAX-len), ",%s:%d", "rsoc", chg_dfs->rsoc);
		mievent_upload(DFX_ID_CHG_SOC_NOT_FULL, para_str, 4);
		break;

	case CHG_DFX_FG_I2C_ERR:
		len += scnprintf((para_str+len), (PARAMETER_LEN_MAX-len), "%s:%s", "chgErrInfo", "fgI2cErr");
		mievent_upload(DFX_ID_CHG_FG_I2C_ERR, para_str, 1);
		break;

	case CHG_DFX_CP_I2C_ERR:
		len += scnprintf((para_str+len), (PARAMETER_LEN_MAX-len), "%s:%s", "chgErrInfo", "CpI2CErr");
		len += scnprintf((para_str+len), (PARAMETER_LEN_MAX-len), ",%s:%d", "cp_absent", chg_dfs->cp_absent);
		mievent_upload(DFX_ID_CHG_CP_I2C_ERR, para_str, 2);
		break;

	case CHG_DFX_CP_TDIE_HOT:
		len += scnprintf((para_str+len), (PARAMETER_LEN_MAX-len), "%s:%s", "chgErrInfo", "CpTdieHot");
		len += scnprintf((para_str+len), (PARAMETER_LEN_MAX-len), ",%s:%d", "tdie", chg_dfs->tdie);
		mievent_upload(DFX_ID_CHG_CP_TDIE_HOT, para_str, 2);
		break;

	case CHG_DFX_NOT_CHG_IN_HIGH_TEMP:
		len += scnprintf((para_str+len), (PARAMETER_LEN_MAX-len), "%s:%s", "chgErrInfo", "NotChgInHighTemp");
		len += scnprintf((para_str+len), (PARAMETER_LEN_MAX-len), ",%s:%d", "tbat", chg_dfs->tbat);
		mievent_upload(DFX_ID_CHG_NOT_CHG_IN_HIGH_TEMP, para_str, 2);
		break;

	case CHG_DFX_BATT_AUTH_FAIL:
		len += scnprintf((para_str+len), (PARAMETER_LEN_MAX-len), "%s:%s", "chgErrInfo", "BattAuthFail");
		mievent_upload(DFX_ID_CHG_BATT_AUTH_FAIL, para_str, 1);
		break;

	case CHG_DFX_TBAT_HOT:
		len += scnprintf((para_str+len), (PARAMETER_LEN_MAX-len), "%s:%s", "chgStatInfo", "TbatHot");
		len += scnprintf((para_str+len), (PARAMETER_LEN_MAX-len), ",%s:%d", "tbat", chg_dfs->tbat);
		len += scnprintf((para_str+len), (PARAMETER_LEN_MAX-len), ",%s:%d", "tbatMax", (chg_dfs->tbat_max / 10));
		len += scnprintf((para_str+len), (PARAMETER_LEN_MAX-len), ",%s:%d", "isCharging", chg_dfs->adapter_plug_in);
		mievent_upload(DFX_ID_CHG_TBAT_HOT, para_str, 4);
		break;

	case CHG_DFX_TBAT_COLD:
		len += scnprintf((para_str+len), (PARAMETER_LEN_MAX-len), "%s:%s", "chgStatInfo", "TbatCold");
		len += scnprintf((para_str+len), (PARAMETER_LEN_MAX-len), ",%s:%d", "tbat", chg_dfs->tbat);
		len += scnprintf((para_str+len), (PARAMETER_LEN_MAX-len), ",%s:%d", "tbatMin", (chg_dfs->tbat_min / 10));
		len += scnprintf((para_str+len), (PARAMETER_LEN_MAX-len), ",%s:%d", "isCharging", chg_dfs->adapter_plug_in);
		mievent_upload(DFX_ID_CHG_TBAT_COLD, para_str, 4);
		break;

	default:
		pr_err("[HQ_CHG_DFS]: unknown type to report\n");
		break;
	}
}

static int xm_chg_dfx_report_events(struct xm_chg_dfs *chg_dfs)
{
	int evt_index = 0;

	pr_info("[HQ_CHG_DFS] evt_report_bits = 0x%lX\n", chg_dfs->evt_report_bits[0]);

	for_each_set_bit(evt_index, chg_dfs->evt_report_bits, CHG_DFX_MAX_TYPE) {
		chg_dfs->dfx_type = evt_index;
		xm_chg_dfx_handle_event_report(chg_dfs);
		clear_bit(evt_index, chg_dfs->evt_report_bits);
	}

	return 0;
}

static void wake_up_report(struct xm_chg_dfs *chg_dfs)
{
	if (!atomic_cmpxchg(&chg_dfs->run_report, 0, 1)) {
		wake_up_interruptible(&chg_dfs->report_wq);
	}
}

static int xm_chg_dfx_report_thread_fn(void *data)
{
	struct xm_chg_dfs *chg_dfs = data;

	while (!kthread_should_stop()) {
		wait_event_interruptible(chg_dfs->report_wq,
			(atomic_read(&chg_dfs->run_report) || kthread_should_stop()));

		atomic_set(&chg_dfs->run_report, 0);

		xm_chg_dfx_report_events(chg_dfs);
	}

	return 0;
}

/* the charge pumps' power supplies: Southchip SC8551 or LN8000 */
static const char * const cp_psy_names[] = {
	"sc-cp-standalone",
	"ln8000-charger",
};

static int chg_dfs_psy_notifier_call(struct notifier_block *nb,
				unsigned long event, void *data)
{
	struct xm_chg_dfs *chg_dfs = container_of(nb, struct xm_chg_dfs, psy_nb);
	struct power_supply *psy = data;
	union power_supply_propval pval;
	struct mtk_charger *info = NULL;
	struct mtk_battery_manager *bm = NULL;
	int ret = 0;
	int i;

	pr_info("[HQ_CHG_DFS] %s\n", __func__);

	if (event != PSY_EVENT_PROP_CHANGED)
		return NOTIFY_OK;

	if (strcmp(psy->desc->name, "primary_chg") == 0) {
		ret = power_supply_get_property(psy, POWER_SUPPLY_PROP_USB_TYPE, &pval);
		if (ret != 0)
			return NOTIFY_DONE;
		chg_dfs->vbus_type = pval.intval;
	}

	for (i = 0; i < ARRAY_SIZE(cp_psy_names); i++) {
		if (strcmp(psy->desc->name, cp_psy_names[i]) != 0)
			continue;

		pr_info("[HQ_CHG_DFS] cp match success, name: %s\n", cp_psy_names[i]);

		ret = power_supply_get_property(psy, POWER_SUPPLY_PROP_TEMP, &pval);
		if (ret != 0)
			return NOTIFY_DONE;
		chg_dfs->tdie = pval.intval;

		ret = power_supply_get_property(psy, POWER_SUPPLY_PROP_VOLTAGE_NOW, &pval);
		if (ret != 0)
			return NOTIFY_DONE;
		chg_dfs->vbus = pval.intval;

		ret = power_supply_get_property(psy, POWER_SUPPLY_PROP_ONLINE, &pval);
		if (ret != 0)
			return NOTIFY_DONE;
		chg_dfs->cp_absent = pval.intval;

		pr_info("[HQ_CHG_DFS] cp match success, chg_dfs->tdie: %d,chg_dfs->vbus:%d.chg_dfs->cp_absent_flag:%d\n",
			chg_dfs->tdie, chg_dfs->vbus, chg_dfs->cp_absent);
		break;
	}

	if (strcmp(psy->desc->name, "battery") == 0) {
		ret = power_supply_get_property(psy, POWER_SUPPLY_PROP_CYCLE_COUNT, &pval);
		if (ret != 0)
			return NOTIFY_OK;
		chg_dfs->cycle_cnt = pval.intval;

		ret = power_supply_get_property(psy, POWER_SUPPLY_PROP_TEMP, &pval);
		if (ret != 0)
			return NOTIFY_OK;
		chg_dfs->tbat = pval.intval;

		ret = power_supply_get_property(psy, POWER_SUPPLY_PROP_CAPACITY, &pval);
		if (ret != 0)
			return NOTIFY_OK;
		chg_dfs->soc = pval.intval;

		ret = power_supply_get_property(psy, POWER_SUPPLY_PROP_VOLTAGE_NOW, &pval);
		if (ret != 0)
			return NOTIFY_OK;
		chg_dfs->vbat = pval.intval;

		ret = power_supply_get_property(psy, POWER_SUPPLY_PROP_PRESENT, &pval);
		if (ret != 0)
			return NOTIFY_OK;
		chg_dfs->batt_present = pval.intval;

		ret = power_supply_get_property(psy, POWER_SUPPLY_PROP_PRESENT, &pval);
		if (ret != 0)
			return NOTIFY_OK;
		chg_dfs->fg_present = pval.intval;

		bm = power_supply_get_drvdata(psy);
		if (!bm) {
			pr_err("[HQ_CHG_DFS] mtk_battery_manager is not rdy\n");
			return NOTIFY_OK;
		}
		pr_info("[HQ_CHG_DFS] mtk_battery_manager is rdy, soc: %d\n", bm->gm1->soc);
		chg_dfs->rsoc = bm->gm1->soc;

		chg_dfs->tbat_max = max(chg_dfs->tbat, chg_dfs->tbat_max);
		chg_dfs->tbat_min = min(chg_dfs->tbat, chg_dfs->tbat_min);
	}

	if (strcmp(psy->desc->name, "mtk-master-charger") == 0) {
		info = power_supply_get_drvdata(psy);
		if (!info) {
			pr_err("[HQ_CHG_DFS] mtk_charger is not rdy\n");
			return NOTIFY_OK;
		}

		if (info->adapter_dev[PD]) {
			chg_dfs->adapter_svid = info->adapter_dev[PD]->adapter_svid;
			chg_dfs->pd_verify_done = info->adapter_dev[PD]->verify_process;
			chg_dfs->pd_auth_fail = !info->adapter_dev[PD]->verifed;
		}
		chg_dfs->chr_type = info->chr_type;
		chg_dfs->usb_type = info->usb_type;

		pr_info("[HQ_CHG_DFS] mtk_charger is rdy, adapter_svid: %d, pd_verify_done: %d,pd_auth_fail: %d,chg_dfs->chr_type: %d, chg_dfs->usb_type: %d .\n",
			chg_dfs->adapter_svid, chg_dfs->pd_verify_done, chg_dfs->pd_auth_fail,
			chg_dfs->chr_type, chg_dfs->usb_type);
	}

	return NOTIFY_DONE;
}

static int chg_dfs_charger_notifier_call(struct notifier_block *nb,
				unsigned long event, void *data)
{
	return NOTIFY_DONE;
}

static int chg_dfs_cp_notifier_call(struct notifier_block *nb,
				unsigned long event, void *data)
{
	struct xm_chg_dfs *chg_dfs = container_of(nb, struct xm_chg_dfs, cp_nb);

	pr_info("[HQ_CHG_DFS] %s, event: %lu\n", __func__, event);

	switch (event) {
	case HQ_CP_EVT_VBUS_OVP:
		pr_info("[HQ_CHG_DFS] receive CHG_DFX_VBUS_OVP event\n");
		chg_dfs->cp_vbus_ovp = 1;
		break;
	case HQ_CP_EVT_IBUS_OCP:
		pr_info("[HQ_CHG_DFS] receive CHG_DFX_IBUS_OCP event\n");
		chg_dfs->cp_ibus_ocp = 1;
		break;
	case HQ_CP_EVT_VBAT_OVP:
		pr_info("[HQ_CHG_DFS] receive CHG_DFX_VBAT_OVP event\n");
		chg_dfs->cp_vbat_ovp = 1;
		break;
	case HQ_CP_EVT_IBAT_OCP:
		pr_info("[HQ_CHG_DFS] receive CHG_DFX_IBAT_OCP event\n");
		chg_dfs->cp_ibat_ocp = 1;
		break;
	case HQ_CP_EVT_ENABLE_FAIL:
		pr_info("[HQ_CHG_DFS] receive CP_EVENT_ENABLE_ERR event\n");
		chg_dfs->cp_en_fail = 1;
		break;
	default:
		break;
	}

	return NOTIFY_DONE;
}

static int chg_dfs_fg_notifier_call(struct notifier_block *nb,
				unsigned long event, void *data)
{
	return NOTIFY_DONE;
}

static int chg_dfs_cm_notifier_call(struct notifier_block *nb,
				unsigned long event, void *data)
{
	struct xm_chg_dfs *chg_dfs = container_of(nb, struct xm_chg_dfs, cm_nb);
	int i;

	switch (event) {
	case HQ_CHGMGR_EVT_PLUG_OUT:
		pr_info("[HQ_CHG_DFS] receive adapter plugout event\n");
		chg_dfs->check_interval = DFS_CHECK_10S_INTERVAL;
		chg_dfs->adapter_plug_in = 0;
		break;
	case HQ_CHGMGR_EVT_PLUG_IN:
		pr_info("[HQ_CHG_DFS] receive adapter plugin event\n");
		for (i = 0; i < ARRAY_SIZE(dfx_evt_cond); i++) {
			if (dfx_evt_cond[i].dfx_type != CHG_DFX_CHG_BATT_CYCLE)
				dfx_evt_cond[i].report_cnt = 0;
		}
		chg_dfs->check_interval = DFS_CHECK_5S_INTERVAL;
		chg_dfs->adapter_plug_in = 1;
		break;
	default:
		break;
	}

	return NOTIFY_DONE;
}

void handle_evt_cond_check_work(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct xm_chg_dfs *chg_dfs = container_of(dwork, struct xm_chg_dfs, evt_cond_check_work);
	struct power_supply *psy = NULL;
	union power_supply_propval pval = {0};
	int ret = 0;
	int i;

	pr_info("[HQ_CHG_DFS] %s\n", __func__);

	mutex_lock(&chg_dfs->cond_check_lock);

	psy = power_supply_get_by_name("battery");
	if (IS_ERR_OR_NULL(psy)) {
		pr_err("failed to get bat_psy!!\n");
	} else {
		ret = power_supply_get_property(psy, POWER_SUPPLY_PROP_STATUS, &pval);
		if (ret < 0)
			pr_err("failed to get chg status from battery!!\n");
		else
			pr_info("get chg status from battery success!\n");
	}
	chg_dfs->chg_status = pval.intval;

	psy = power_supply_get_by_name("batt_verify");
	if (IS_ERR_OR_NULL(psy)) {
		pr_err("failed to get bat_verify_psy!!\n");
	} else {
		ret = power_supply_get_property(psy, POWER_SUPPLY_PROP_AUTHENTIC, &pval);
		if (ret < 0)
			pr_err("failed to get chg status from bat_verify_psy!!\n");
		else
			pr_info("get chg status from bat_verify_psy success!\n");
	}
	chg_dfs->batt_auth = pval.intval;

	pr_info("[HQ_CHG_DFS] cycle_cnt: %d, tbat: %d [%d %d], batt_auth: %d, soc: %d, rsoc: %d, vbat: %d, chip_chg_status: %d, vbus_type: %d,vbus: %d,tdie: %d\n",
		chg_dfs->cycle_cnt, chg_dfs->tbat, chg_dfs->tbat_min, chg_dfs->tbat_max,
		chg_dfs->batt_auth, chg_dfs->soc, chg_dfs->rsoc, chg_dfs->vbat,
		chg_dfs->chg_status, chg_dfs->vbus_type, chg_dfs->vbus, chg_dfs->tdie);

	for (i = 0; i < ARRAY_SIZE(dfx_evt_cond); i++) {
		switch (dfx_evt_cond[i].dfx_type) {
		case CHG_DFX_PD_AUTH_FAIL:
			if (chg_dfs->adapter_svid == XM_ADAPTER_SVID && chg_dfs->pd_auth_fail &&
			    chg_dfs->pd_verify_done &&
			    dfx_evt_cond[i].report_cnt < dfx_evt_cond[i].report_cnt_limit) {
				set_bit(CHG_DFX_PD_AUTH_FAIL, chg_dfs->evt_report_bits);
				dfx_evt_cond[i].report_cnt++;
				dfx_evt_cond[i].first_report = false;
			}
			break;

		case CHG_DFX_CP_EN_FAIL:
			if (chg_dfs->cp_en_fail == 1 &&
			    dfx_evt_cond[i].report_cnt < dfx_evt_cond[i].report_cnt_limit) {
				set_bit(CHG_DFX_CP_EN_FAIL, chg_dfs->evt_report_bits);
				dfx_evt_cond[i].report_cnt++;
				dfx_evt_cond[i].first_report = false;
				chg_dfs->cp_en_fail = 0;
			}
			break;

		case CHG_DFX_NONE_STANDARD_CHG:
			if (chg_dfs->chr_type == POWER_SUPPLY_TYPE_USB &&
			    chg_dfs->usb_type == POWER_SUPPLY_USB_TYPE_DCP &&
			    dfx_evt_cond[i].report_cnt < dfx_evt_cond[i].report_cnt_limit) {
				set_bit(CHG_DFX_NONE_STANDARD_CHG, chg_dfs->evt_report_bits);
				dfx_evt_cond[i].report_cnt++;
				dfx_evt_cond[i].first_report = false;
			}
			break;

		case CHG_DFX_CP_VBUS_OVP:
			if (chg_dfs->cp_vbus_ovp == 1 &&
			    dfx_evt_cond[i].report_cnt < dfx_evt_cond[i].report_cnt_limit) {
				set_bit(CHG_DFX_CP_VBUS_OVP, chg_dfs->evt_report_bits);
				dfx_evt_cond[i].report_cnt++;
				dfx_evt_cond[i].first_report = false;
				chg_dfs->cp_vbus_ovp = 0;
			}
			break;

		case CHG_DFX_CP_IBUS_OCP:
			if (chg_dfs->cp_ibus_ocp == 1 &&
			    dfx_evt_cond[i].report_cnt < dfx_evt_cond[i].report_cnt_limit) {
				set_bit(CHG_DFX_CP_IBUS_OCP, chg_dfs->evt_report_bits);
				dfx_evt_cond[i].report_cnt++;
				dfx_evt_cond[i].first_report = false;
				chg_dfs->cp_ibus_ocp = 0;
			}
			break;

		case CHG_DFX_CP_VBAT_OVP:
			if (chg_dfs->cp_vbat_ovp == 1 &&
			    dfx_evt_cond[i].report_cnt < dfx_evt_cond[i].report_cnt_limit) {
				set_bit(CHG_DFX_CP_VBAT_OVP, chg_dfs->evt_report_bits);
				dfx_evt_cond[i].report_cnt++;
				dfx_evt_cond[i].first_report = false;
				chg_dfs->cp_vbat_ovp = 0;
			}
			break;

		case CHG_DFX_CP_IBAT_OCP:
			if (chg_dfs->cp_ibat_ocp == 1 &&
			    dfx_evt_cond[i].report_cnt < dfx_evt_cond[i].report_cnt_limit) {
				set_bit(CHG_DFX_CP_IBAT_OCP, chg_dfs->evt_report_bits);
				dfx_evt_cond[i].report_cnt++;
				dfx_evt_cond[i].first_report = false;
				chg_dfs->cp_ibat_ocp = 0;
			}
			break;

		case CHG_DFX_CHG_BATT_CYCLE:
			if (chg_dfs->cycle_cnt != chg_dfs->last_cycle_count) {
				if ((chg_dfs->cycle_cnt % DFX_CYCLE_CNT_STEP == 0 || dfx_evt_cond[i].first_report) &&
				    dfx_evt_cond[i].report_cnt < dfx_evt_cond[i].report_cnt_limit) {
					set_bit(CHG_DFX_CHG_BATT_CYCLE, chg_dfs->evt_report_bits);
					dfx_evt_cond[i].report_cnt++;
					dfx_evt_cond[i].first_report = false;
				}
				chg_dfs->last_cycle_count = chg_dfs->cycle_cnt;
			}
			break;

		case CHG_DFX_SOC_NOT_FULL:
			/* as stock: no POWER_SUPPLY_STATUS_* is 5, so this is never reported */
			if (chg_dfs->chg_status == 5 && chg_dfs->soc != 100 &&
			    dfx_evt_cond[i].report_cnt < dfx_evt_cond[i].report_cnt_limit) {
				set_bit(CHG_DFX_SOC_NOT_FULL, chg_dfs->evt_report_bits);
				dfx_evt_cond[i].report_cnt++;
				dfx_evt_cond[i].first_report = false;
			}
			break;

		case CHG_DFX_FG_I2C_ERR:
			if (chg_dfs->fg_present == 0 &&
			    dfx_evt_cond[i].report_cnt < dfx_evt_cond[i].report_cnt_limit) {
				set_bit(CHG_DFX_FG_I2C_ERR, chg_dfs->evt_report_bits);
				dfx_evt_cond[i].report_cnt++;
				dfx_evt_cond[i].first_report = false;
				chg_dfs->fg_present = 1;
			}
			break;

		case CHG_DFX_CP_I2C_ERR:
			if (chg_dfs->cp_absent == 1 &&
			    dfx_evt_cond[i].report_cnt < dfx_evt_cond[i].report_cnt_limit) {
				set_bit(CHG_DFX_CP_I2C_ERR, chg_dfs->evt_report_bits);
				dfx_evt_cond[i].report_cnt++;
				dfx_evt_cond[i].first_report = false;
				chg_dfs->cp_absent = 0;
			}
			break;

		case CHG_DFX_BATT_LINKER_ABSENT:
			if (chg_dfs->batt_present == 0 &&
			    dfx_evt_cond[i].report_cnt < dfx_evt_cond[i].report_cnt_limit) {
				set_bit(CHG_DFX_BATT_LINKER_ABSENT, chg_dfs->evt_report_bits);
				dfx_evt_cond[i].report_cnt++;
				dfx_evt_cond[i].first_report = false;
				chg_dfs->batt_present = 1;
			}
			break;

		case CHG_DFX_VBUS_UVLO:
			if (chg_dfs->vbus >= 2000 && chg_dfs->vbus <= 3000 &&
			    dfx_evt_cond[i].report_cnt < dfx_evt_cond[i].report_cnt_limit) {
				set_bit(CHG_DFX_VBUS_UVLO, chg_dfs->evt_report_bits);
				dfx_evt_cond[i].report_cnt++;
				dfx_evt_cond[i].first_report = false;
			}
			break;

		case CHG_DFX_CP_TDIE_HOT:
			if (chg_dfs->tdie >= DFX_CP_TDIE_HOT &&
			    dfx_evt_cond[i].report_cnt < dfx_evt_cond[i].report_cnt_limit) {
				set_bit(CHG_DFX_CP_TDIE_HOT, chg_dfs->evt_report_bits);
				dfx_evt_cond[i].report_cnt++;
				dfx_evt_cond[i].first_report = false;
			}
			break;

		case CHG_DFX_NOT_CHG_IN_LOW_TEMP:
			if (chg_dfs->tbat < 0 && chg_dfs->tbat > DFX_BAT_COLD_TEMP &&
			    chg_dfs->adapter_plug_in &&
			    chg_dfs->chg_status == POWER_SUPPLY_STATUS_NOT_CHARGING &&
			    dfx_evt_cond[i].report_cnt < dfx_evt_cond[i].report_cnt_limit) {
				set_bit(CHG_DFX_NOT_CHG_IN_LOW_TEMP, chg_dfs->evt_report_bits);
				dfx_evt_cond[i].report_cnt++;
				dfx_evt_cond[i].first_report = false;
			}
			break;

		case CHG_DFX_NOT_CHG_IN_HIGH_TEMP:
			if (chg_dfs->tbat > 450 && chg_dfs->tbat < 600 &&
			    chg_dfs->adapter_plug_in &&
			    chg_dfs->chg_status == POWER_SUPPLY_STATUS_NOT_CHARGING &&
			    dfx_evt_cond[i].report_cnt < dfx_evt_cond[i].report_cnt_limit) {
				set_bit(CHG_DFX_NOT_CHG_IN_HIGH_TEMP, chg_dfs->evt_report_bits);
				dfx_evt_cond[i].report_cnt++;
				dfx_evt_cond[i].first_report = false;
			}
			break;

		case CHG_DFX_BATT_AUTH_FAIL:
			if (chg_dfs->batt_auth == 0 &&
			    dfx_evt_cond[i].report_cnt < dfx_evt_cond[i].report_cnt_limit) {
				set_bit(CHG_DFX_BATT_AUTH_FAIL, chg_dfs->evt_report_bits);
				dfx_evt_cond[i].report_cnt++;
				dfx_evt_cond[i].first_report = false;
			}
			break;

		case CHG_DFX_TBAT_HOT:
			if (chg_dfs->tbat > DFX_BAT_HOT_TEMP &&
			    dfx_evt_cond[i].report_cnt < dfx_evt_cond[i].report_cnt_limit) {
				set_bit(CHG_DFX_TBAT_HOT, chg_dfs->evt_report_bits);
				dfx_evt_cond[i].report_cnt++;
				dfx_evt_cond[i].first_report = false;
			}
			break;

		case CHG_DFX_TBAT_COLD:
			if (chg_dfs->tbat < DFX_BAT_COLD_TEMP &&
			    dfx_evt_cond[i].report_cnt < dfx_evt_cond[i].report_cnt_limit) {
				set_bit(CHG_DFX_TBAT_COLD, chg_dfs->evt_report_bits);
				dfx_evt_cond[i].report_cnt++;
				dfx_evt_cond[i].first_report = false;
			}
			break;

		default:
			pr_err("[HQ_CHG_DFS]: unknown type to report,dfx_type = %d\n", chg_dfs->dfx_type);
			break;
		}
	}

	if (bitmap_weight(chg_dfs->evt_report_bits, CHG_DFX_MAX_TYPE)) {
		pr_info("[HQ_CHG_DFS] %d dfx event need report\n",
			bitmap_weight(chg_dfs->evt_report_bits, CHG_DFX_MAX_TYPE));
		wake_up_report(chg_dfs);
	}

	mutex_unlock(&chg_dfs->cond_check_lock);

	schedule_delayed_work(&chg_dfs->evt_cond_check_work, msecs_to_jiffies(chg_dfs->check_interval));
}

static int xm_chg_dfs_probe(struct platform_device *pdev)
{
	struct xm_chg_dfs *chg_dfs;
	int ret = 0;

	chg_dfs = devm_kzalloc(&pdev->dev, sizeof(*chg_dfs), GFP_KERNEL);
	if (!chg_dfs)
		return -ENOMEM;

	platform_set_drvdata(pdev, chg_dfs);

	init_waitqueue_head(&chg_dfs->report_wq);
	chg_dfs->report_thread = kthread_run(xm_chg_dfx_report_thread_fn, chg_dfs, "xm_chg_dfx_report_thread");

	chg_dfs->pd_adapter = get_adapter_by_name("pd_adapter");
	if (!chg_dfs->pd_adapter)
		return -EPROBE_DEFER;

	chg_dfs->adapter_svid = 0;
	chg_dfs->pd_auth_fail = 0;
	chg_dfs->last_cycle_count = -1;
	chg_dfs->check_interval = DFS_CHECK_10S_INTERVAL;
	chg_dfs->tbat_max = INT_MIN;
	chg_dfs->tbat_min = INT_MAX;
	chg_dfs->tdie = 0;
	chg_dfs->cp_en_fail = 0;
	chg_dfs->fg_present = 1;
	chg_dfs->batt_present = 1;
	chg_dfs->pd_verify_done = 0;
	chg_dfs->cp_vac_ovp = 0;
	chg_dfs->cp_ibat_ocp = 0;
	chg_dfs->cp_vbat_ovp = 0;
	chg_dfs->cp_ibus_ocp = 0;
	chg_dfs->cp_vbus_ovp = 0;
	chg_dfs->cp_absent = 0;

	mutex_init(&chg_dfs->cond_check_lock);
	bitmap_zero(chg_dfs->evt_report_bits, CHG_DFX_MAX_TYPE);
	atomic_set(&chg_dfs->run_report, 0);

	INIT_DELAYED_WORK(&chg_dfs->evt_cond_check_work, handle_evt_cond_check_work);
	schedule_delayed_work(&chg_dfs->evt_cond_check_work, msecs_to_jiffies(60000));

	chg_dfs->psy_nb.notifier_call = chg_dfs_psy_notifier_call;
	ret = power_supply_reg_notifier(&chg_dfs->psy_nb);
	if (ret < 0) {
		pr_err("[HQ_CHG_DFS] couldn't register psy notifier ret = %d\n", ret);
		return ret;
	}

	chg_dfs->charger_nb.notifier_call = chg_dfs_charger_notifier_call;
	ret = hq_charger_notifier_register(&chg_dfs->charger_nb);
	if (ret < 0) {
		pr_err("[HQ_CHG_DFS] couldn't register charger notifier, ret = %d\n", ret);
		return ret;
	}

	chg_dfs->cp_nb.notifier_call = chg_dfs_cp_notifier_call;
	ret = hq_cp_notifier_register(&chg_dfs->cp_nb);
	if (ret < 0) {
		pr_err("[HQ_CHG_DFS] couldn't register cp notifier, ret = %d\n", ret);
		return ret;
	}

	chg_dfs->fg_nb.notifier_call = chg_dfs_fg_notifier_call;
	ret = hq_fg_notifier_register(&chg_dfs->fg_nb);
	if (ret < 0) {
		pr_err("[HQ_CHG_DFS] couldn't register fg notifier, ret = %d\n", ret);
		return ret;
	}

	chg_dfs->cm_nb.notifier_call = chg_dfs_cm_notifier_call;
	ret = hq_chargermanager_notifier_register(&chg_dfs->cm_nb);
	if (ret < 0) {
		pr_err("[HQ_CHG_DFS] couldn't register charger manager notifier, ret = %d\n", ret);
		return ret;
	}

	chg_dfs->primary_chg = get_charger_by_name("primary_chg");
	if (!chg_dfs->primary_chg) {
		pr_err("[HQ_CHG_DFS] failed to find primary_chg device\n");
		return -EPROBE_DEFER;
	}

	pr_info("[HQ_CHG_DFS] success...\n");

	return 0;
}

static int xm_chg_dfs_remove(struct platform_device *pdev)
{
	return 0;
}

static const struct of_device_id xm_chg_dfs_of_match[] = {
	{.compatible = "xiaomi,xm-chg-dfs",},
	{},
};
MODULE_DEVICE_TABLE(of, xm_chg_dfs_of_match);

static struct platform_driver xm_chg_dfs_driver = {
	.driver = {
		.name = "xm_chg_dfs",
		.of_match_table = xm_chg_dfs_of_match,
	},
	.probe = xm_chg_dfs_probe,
	.remove = xm_chg_dfs_remove,
};
module_platform_driver(xm_chg_dfs_driver);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Xiaomi Charge DFS Driver");
MODULE_AUTHOR("pengyuzhe@huaqin.com");
