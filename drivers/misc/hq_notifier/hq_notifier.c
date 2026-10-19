// SPDX-License-Identifier: GPL-2.0
/*
 * Huaqin's notifier for gold's board drivers: tells them when something is
 * plugged into the Type-C port or a headset into the 3.5 mm jack. The
 * touchscreens switch to their charger mode on USB, the switch class reports
 * headsets on h2w.
 */
#include <linux/device.h>
#include <linux/hq_notifier.h>
#include <linux/kprobes.h>
#include <linux/module.h>

#include "tcpm.h"

static BLOCKING_NOTIFIER_HEAD(hq_notifier_list);

struct tcpc_device *tcpc;
struct notifier_block tcpc_nb;
static bool typec_status_registered;
static bool earphone_status_registered;
static int hq_usb_data;
static int hq_earphone_data;

int register_hq_notify(struct notifier_block *nb)
{
	return blocking_notifier_chain_register(&hq_notifier_list, nb);
}
EXPORT_SYMBOL(register_hq_notify);

int unregister_hq_notify(struct notifier_block *nb)
{
	return blocking_notifier_chain_unregister(&hq_notifier_list, nb);
}
EXPORT_SYMBOL(unregister_hq_notify);

int hq_notify_call_chain(unsigned long val, void *v)
{
	return blocking_notifier_call_chain(&hq_notifier_list, val, v);
}
EXPORT_SYMBOL(hq_notify_call_chain);

int tcpc_typec_event(struct notifier_block *nb, unsigned long event,
		     void *data)
{
	struct tcp_notify *noti = data;
	unsigned long hq_event;

	if (event != TCP_NOTIFY_TYPEC_STATE)
		return NOTIFY_OK;

	pr_info("%s: noti->typec_state.old_state is %d, noti->typec_state.new_state is %d!\n",
		__func__, noti->typec_state.old_state, noti->typec_state.new_state);

	switch (noti->typec_state.old_state) {
	case TYPEC_UNATTACHED:
		switch (noti->typec_state.new_state) {
		case TYPEC_ATTACHED_SNK:
		case TYPEC_ATTACHED_DBGACC_SNK:
		case TYPEC_ATTACHED_CUSTOM_SRC:
		case TYPEC_ATTACHED_NORP_SRC:
			pr_info("%s: USB plug in\n", __func__);
			hq_event = HQ_NOTIFY_USB;
			break;
		case TYPEC_ATTACHED_AUDIO:
			pr_info("%s: Type C headset Plug in\n", __func__);
			hq_event = HQ_NOTIFY_TYPEC_EARPHONE;
			break;
		default:
			return NOTIFY_OK;
		}
		hq_usb_data = 1;
		break;
	case TYPEC_ATTACHED_SNK:
	case TYPEC_ATTACHED_DBGACC_SNK:
	case TYPEC_ATTACHED_CUSTOM_SRC:
	case TYPEC_ATTACHED_NORP_SRC:
		if (noti->typec_state.new_state != TYPEC_UNATTACHED)
			return NOTIFY_OK;
		pr_info("%s: USB plug out\n", __func__);
		hq_event = HQ_NOTIFY_USB;
		hq_usb_data = 0;
		break;
	case TYPEC_ATTACHED_AUDIO:
		if (noti->typec_state.new_state != TYPEC_UNATTACHED)
			return NOTIFY_OK;
		pr_info("%s: Type C headset Plug out\n", __func__);
		hq_event = HQ_NOTIFY_TYPEC_EARPHONE;
		hq_usb_data = 0;
		break;
	default:
		return NOTIFY_OK;
	}

	blocking_notifier_call_chain(&hq_notifier_list, hq_event, &hq_usb_data);
	return NOTIFY_OK;
}

/*
 * The accdet driver (sound/soc/codecs/mt6357-accdet.c) has no notifier of its
 * own: catch its send_status_event(cable_type, status) instead.
 */
static int earphone_status_pre_handler(struct kprobe *p, struct pt_regs *regs)
{
	u32 cable_type = regs->regs[0];
	int status = !!(u32)regs->regs[1];

	pr_info("%s: audio hooks accdet-> cable_type:%u, status:%u\n",
		__func__, cable_type, status);

	/* HEADSET_MIC, HEADSET_NO_MIC, LINE_OUT_DEVICE */
	if (cable_type == 1 || cable_type == 2 || cable_type == 4) {
		hq_earphone_data = status;
		blocking_notifier_call_chain(&hq_notifier_list,
					     HQ_NOTIFY_EARPHONE,
					     &hq_earphone_data);
	}

	return 0;
}

static struct kprobe earphone_status = {
	.symbol_name = "send_status_event",
	.pre_handler = earphone_status_pre_handler,
};

static int __init hq_notifier_init(void)
{
	int ret;

	pr_info("%s Entry\n", __func__);

	tcpc = tcpc_dev_get_by_name("type_c_port0");
	if (!tcpc) {
		pr_err("%s: tcpc_dev_get_by_name fail\n", __func__);
		return 0;
	}

	tcpc_nb.notifier_call = tcpc_typec_event;
	ret = register_tcp_dev_notifier(tcpc, &tcpc_nb, TCP_NOTIFY_TYPE_VBUS |
					TCP_NOTIFY_TYPE_USB | TCP_NOTIFY_TYPE_MISC);
	if (ret) {
		pr_err("%s: register_tcp_dev_notifier fail\n", __func__);
		return ret;
	}
	typec_status_registered = true;

	ret = register_kprobe(&earphone_status);
	if (ret < 0) {
		pr_err("%s: audio hooks register_earphone_status failed! ret=%d\n",
		       __func__, ret);
		return ret;
	}
	earphone_status_registered = true;

	return 0;
}

static void __exit hq_notifier_exit(void)
{
	pr_info("%s Entry\n", __func__);

	if (typec_status_registered)
		unregister_tcp_dev_notifier(tcpc, &tcpc_nb, TCP_NOTIFY_TYPE_VBUS |
					    TCP_NOTIFY_TYPE_USB | TCP_NOTIFY_TYPE_MISC);
	if (earphone_status_registered)
		unregister_kprobe(&earphone_status);
}

module_init(hq_notifier_init);
module_exit(hq_notifier_exit);

MODULE_AUTHOR("hq bsp system team");
MODULE_DESCRIPTION("hq common kernel notifier driver");
MODULE_LICENSE("GPL v2");
