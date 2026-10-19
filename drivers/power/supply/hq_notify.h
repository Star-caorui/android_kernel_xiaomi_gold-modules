/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __HQ_NOTIFY_H__
#define __HQ_NOTIFY_H__

#include <linux/notifier.h>

/* the charge pumps' events */
enum hq_cp_event {
	HQ_CP_EVT_VBUS_OVP = 1,
	HQ_CP_EVT_IBUS_OCP,
	HQ_CP_EVT_VBAT_OVP,
	HQ_CP_EVT_IBAT_OCP,
	HQ_CP_EVT_ENABLE_FAIL,
};

/* the charger manager's events */
enum hq_chargermanager_event {
	HQ_CHGMGR_EVT_PLUG_OUT = 0,
	HQ_CHGMGR_EVT_PLUG_IN,
};

extern int hq_charger_notifier_register(struct notifier_block *nb);
extern int hq_charger_notifier_unregister(struct notifier_block *nb);
extern int hq_charger_notifier_call_chain(unsigned long event, void *data);
extern int hq_cp_notifier_register(struct notifier_block *nb);
extern int hq_cp_notifier_unregister(struct notifier_block *nb);
extern int hq_cp_notifier_call_chain(unsigned long event, void *data);
extern int hq_chargermanager_notifier_register(struct notifier_block *nb);
extern int hq_chargermanager_notifier_unregister(struct notifier_block *nb);
extern int hq_chargermanager_notifier_call_chain(unsigned long event, void *data);
extern int hq_fg_notifier_register(struct notifier_block *nb);
extern int hq_fg_notifier_unregister(struct notifier_block *nb);
extern int hq_fg_notifier_call_chain(unsigned long event, void *data);

#endif /* __HQ_NOTIFY_H__ */
