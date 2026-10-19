// SPDX-License-Identifier: GPL-2.0
/*
 * Huaqin's notifier chains between gold's charging drivers: the charger,
 * the charge pumps, the charger manager and the fuel gauge, as stock's
 * hq_notify module has them.
 */

#include <linux/module.h>
#include <linux/notifier.h>
#include "hq_notify.h"

static BLOCKING_NOTIFIER_HEAD(charger_notifier);
static BLOCKING_NOTIFIER_HEAD(cp_notifier);
static BLOCKING_NOTIFIER_HEAD(chargermanager_notifier);
static ATOMIC_NOTIFIER_HEAD(fg_notifier);

int hq_charger_notifier_register(struct notifier_block *nb)
{
	return blocking_notifier_chain_register(&charger_notifier, nb);
}
EXPORT_SYMBOL_GPL(hq_charger_notifier_register);

int hq_charger_notifier_unregister(struct notifier_block *nb)
{
	return blocking_notifier_chain_unregister(&charger_notifier, nb);
}
EXPORT_SYMBOL_GPL(hq_charger_notifier_unregister);

int hq_charger_notifier_call_chain(unsigned long event, void *data)
{
	return blocking_notifier_call_chain(&charger_notifier, event, data);
}
EXPORT_SYMBOL_GPL(hq_charger_notifier_call_chain);

int hq_cp_notifier_register(struct notifier_block *nb)
{
	return blocking_notifier_chain_register(&cp_notifier, nb);
}
EXPORT_SYMBOL_GPL(hq_cp_notifier_register);

int hq_cp_notifier_unregister(struct notifier_block *nb)
{
	return blocking_notifier_chain_unregister(&cp_notifier, nb);
}
EXPORT_SYMBOL_GPL(hq_cp_notifier_unregister);

int hq_cp_notifier_call_chain(unsigned long event, void *data)
{
	return blocking_notifier_call_chain(&cp_notifier, event, data);
}
EXPORT_SYMBOL_GPL(hq_cp_notifier_call_chain);

int hq_chargermanager_notifier_register(struct notifier_block *nb)
{
	return blocking_notifier_chain_register(&chargermanager_notifier, nb);
}
EXPORT_SYMBOL_GPL(hq_chargermanager_notifier_register);

int hq_chargermanager_notifier_unregister(struct notifier_block *nb)
{
	return blocking_notifier_chain_unregister(&chargermanager_notifier, nb);
}
EXPORT_SYMBOL_GPL(hq_chargermanager_notifier_unregister);

int hq_chargermanager_notifier_call_chain(unsigned long event, void *data)
{
	return blocking_notifier_call_chain(&chargermanager_notifier, event, data);
}
EXPORT_SYMBOL_GPL(hq_chargermanager_notifier_call_chain);

int hq_fg_notifier_register(struct notifier_block *nb)
{
	return atomic_notifier_chain_register(&fg_notifier, nb);
}
EXPORT_SYMBOL_GPL(hq_fg_notifier_register);

int hq_fg_notifier_unregister(struct notifier_block *nb)
{
	return atomic_notifier_chain_unregister(&fg_notifier, nb);
}
EXPORT_SYMBOL_GPL(hq_fg_notifier_unregister);

int hq_fg_notifier_call_chain(unsigned long event, void *data)
{
	return atomic_notifier_call_chain(&fg_notifier, event, data);
}
EXPORT_SYMBOL_GPL(hq_fg_notifier_call_chain);

MODULE_AUTHOR("pengyuzhe@huaqin.com");
MODULE_DESCRIPTION("hq notify common driver");
MODULE_LICENSE("GPL v2");
