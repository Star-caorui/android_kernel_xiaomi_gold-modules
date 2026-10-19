// SPDX-License-Identifier: GPL-2.0
/*
 * Huaqin's common kallsyms helper: kallsyms_lookup_name() is not exported
 * to modules, so take its address from a kprobe on it once at load time,
 * and look kernel symbols up with it for other modules.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kprobes.h>

static unsigned long (*g_kallsyms_lookup_name)(const char *name);

unsigned long __nocfi hq_kallsyms_lookup_name(const char *name)
{
	pr_info("%s: %s\n", __func__, name);
	pr_info("%s: by g_kallsyms_lookup_name!\n", __func__);

	if (g_kallsyms_lookup_name)
		return g_kallsyms_lookup_name(name);

	pr_err("%s: g_kallsyms_lookup_name not init\n", __func__);
	return 0;
}
EXPORT_SYMBOL_GPL(hq_kallsyms_lookup_name);

static int hq_get_kallsyms(void)
{
	struct kprobe kp;
	int ret;

	memset(&kp, 0, sizeof(kp));
	kp.symbol_name = "kallsyms_lookup_name";
	ret = register_kprobe(&kp);
	if (ret < 0) {
		pr_err("register kallsyms_lookup_name kporbe failed!, ret = %d\n", ret);
		return ret;
	}

	g_kallsyms_lookup_name = (void *)kp.addr;
	if (!g_kallsyms_lookup_name)
		pr_err("g_kallsyms_lookup_name is null\n");

	unregister_kprobe(&kp);
	return 0;
}

static int __init hq_kallsyms_init(void)
{
	pr_info("%s Entry\n", __func__);

	if (hq_get_kallsyms())
		pr_err("%s: hq_get_kallsyms failed!\n", __func__);

	return 0;
}

static void __exit hq_kallsyms_exit(void)
{
	pr_info("%s Entry\n", __func__);
}

module_init(hq_kallsyms_init);
module_exit(hq_kallsyms_exit);

MODULE_AUTHOR("hq bsp system team");
MODULE_DESCRIPTION("hq common kernel kallsyms driver");
MODULE_LICENSE("GPL v2");
