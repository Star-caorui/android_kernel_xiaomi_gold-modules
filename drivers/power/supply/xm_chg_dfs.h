/* SPDX-License-Identifier: GPL-2.0 */
/**
 * Copyright (c) 2025 Huaqin Technology(Shanghai) Co., Ltd.
 */

#ifndef __XM_CHG_DFS_H__
#define __XM_CHG_DFS_H__

#include <linux/atomic.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/bitmap.h>
#include <linux/bitops.h>

#include <miev/mievent.h>
#include "mtk_charger.h"
#include "mtk_battery.h"
#include "charger_class.h"
#include "adapter_class.h"
#include "hq_notify.h"

/* Charge DFS configurations */
#define PARAMETER_LEN_MAX                       (128)

#define DFS_CHECK_5S_INTERVAL                   (5000)  // 5s
#define DFS_CHECK_10S_INTERVAL                  (10000) // 10s

/********************************************************/
/*               Charge DFS Events Define               */
/********************************************************/
/* Charge Speed Low */
#define DFX_ID_CHG_PD_AUTH_FAIL                 909001004
#define DFX_ID_CHG_CP_EN_FAIL                   909001005

/* Charge Abnormal Stop */
#define DFX_ID_CHG_NONE_STANDARD_CHG            909002001
#define DFX_ID_CHG_CORROSION_DISCHARGE          909002002
#define DFX_ID_CHG_CP_VBUS_OVP                  909002004
#define DFX_ID_CHG_CP_IBUS_OCP                  909002005
#define DFX_ID_CHG_CP_VBAT_OVP                  909002006
#define DFX_ID_CHG_CP_IBAT_OCP                  909002007

/* Cann't Report 100% SOC */
#define DFX_ID_CHG_CHG_BATT_CYCLE               909003001
#define DFX_ID_CHG_SOC_NOT_FULL                 909003002

/* Cann't Charing */
#define DFX_ID_CHG_FG_I2C_ERR                   909005001
#define DFX_ID_CHG_CP_I2C_ERR                   909005002
#define DFX_ID_CHG_BATT_LINKER_ABSENT           909005003
#define DFX_ID_CHG_CP_TDIE_HOT                  909005004
#define DFX_ID_CHG_VBUS_UVLO                    909005006
#define DFX_ID_CHG_NOT_CHG_IN_LOW_TEMP          909005007
#define DFX_ID_CHG_NOT_CHG_IN_HIGH_TEMP         909005008

/* IC Connumicate Failed */
#define DFX_ID_CHG_BATT_AUTH_FAIL               909007001

/* Temperature Abnormal */
#define DFX_ID_CHG_TBAT_HOT                     909009001
#define DFX_ID_CHG_TBAT_COLD                    909009002

/* Event specific parameters */
#define DFX_BAT_HOT_TEMP       (550)
#define DFX_BAT_COLD_TEMP      (-100)
#define DFX_CP_TDIE_HOT        (90)
#define DFX_CYCLE_CNT_STEP     (100)

/* Xiaomi's PD adapters */
#define XM_ADAPTER_SVID        (0x2717)

enum xm_chg_dfx_type {
	CHG_DFX_NONE,
	CHG_DFX_PD_AUTH_FAIL,
	CHG_DFX_CP_EN_FAIL,
	CHG_DFX_NONE_STANDARD_CHG,
	CHG_DFX_CORROSION_DISCHARGE,
	CHG_DFX_CP_VBUS_OVP,
	CHG_DFX_CP_IBUS_OCP,
	CHG_DFX_CP_VBAT_OVP,
	CHG_DFX_CP_IBAT_OCP,
	CHG_DFX_CHG_BATT_CYCLE,
	CHG_DFX_SOC_NOT_FULL,
	CHG_DFX_FG_I2C_ERR,
	CHG_DFX_CP_I2C_ERR,
	CHG_DFX_BATT_LINKER_ABSENT,
	CHG_DFX_CP_TDIE_HOT,
	CHG_DFX_VBUS_UVLO,
	CHG_DFX_NOT_CHG_IN_LOW_TEMP,
	CHG_DFX_NOT_CHG_IN_HIGH_TEMP,
	CHG_DFX_BATT_AUTH_FAIL,
	CHG_DFX_TBAT_HOT,
	CHG_DFX_TBAT_COLD,

	/* NOTE: add new type here */
	CHG_DFX_MAX_TYPE,
};

struct xm_chg_dfx_evt {
	int dfx_type;
	bool first_report;
	bool is_stat;
	bool is_err;

	int report_cnt;
	int report_cnt_limit;
};

struct xm_chg_dfs {
	struct task_struct *report_thread;
	wait_queue_head_t report_wq;
	atomic_t run_report;
	int dfx_type;
	unsigned long evt_report_bits[BITS_TO_LONGS(CHG_DFX_MAX_TYPE)];

	struct notifier_block psy_nb;
	struct notifier_block charger_nb;
	struct notifier_block fg_nb;
	struct notifier_block cm_nb;
	struct notifier_block cp_nb;

	struct adapter_device *pd_adapter;
	struct charger_device *primary_chg;
	struct delayed_work evt_cond_check_work;
	struct mutex cond_check_lock;

	int check_interval;

	int tdie;
	int cp_en_fail;
	int cp_vac_ovp;
	int cp_ibat_ocp;
	int cp_vbat_ovp;
	int cp_ibus_ocp;
	int cp_vbus_ovp;
	int cp_absent;
	int fg_present;
	int batt_present;
	int vbus;
	int tbat;
	int tbat_max;
	int tbat_min;
	int batt_auth;
	int chg_status;
	int cycle_cnt;
	int last_cycle_count;
	int vbat;
	int soc;
	int rsoc;
	int vbus_type;
	int adapter_svid;
	int pd_auth_fail;
	int pd_verify_done;
	int adapter_plug_in;
	int chr_type;
	int usb_type;
};

#endif /* __XM_CHG_DFS_H__ */
