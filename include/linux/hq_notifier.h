/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __LINUX_HQ_NOTIFIER_H
#define __LINUX_HQ_NOTIFIER_H

#include <linux/notifier.h>

/* Events of the hq_notifier chain; the data is an int, 1 for in and 0 for out */
enum hq_notify_event {
	HQ_NOTIFY_USB = 0,		/* a USB host or charger on the Type-C port */
	HQ_NOTIFY_EARPHONE,		/* a headset on the 3.5 mm jack */
	HQ_NOTIFY_TYPEC_EARPHONE,	/* an audio accessory on the Type-C port */
};

int register_hq_notify(struct notifier_block *nb);
int unregister_hq_notify(struct notifier_block *nb);
int hq_notify_call_chain(unsigned long val, void *v);

#endif
