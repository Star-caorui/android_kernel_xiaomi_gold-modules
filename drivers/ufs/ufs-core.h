/* SPDX-License-Identifier: GPL-2.0 */
/*
 * The kernel's UFS host controller core, built into ufs-mediatek-mod.
 *
 * gold's bootloader passes initcall_blacklist=ufshcd_core_init, so the
 * kernel's own core never registers the driver of the UFS device
 * well-known LU, the logical units wait for it, and no disk comes up.
 * Stock's module carries the core for this; so does this one, from the
 * kernel tree it is built against. Its symbols stay inside the module,
 * as the kernel exports the same names.
 *
 * This build warns about non-literal format strings where the kernel's does
 * not; the core passes a buffer it formatted to alloc_ordered_workqueue().
 */
#ifndef _UFS_CORE_H
#define _UFS_CORE_H

#pragma clang diagnostic ignored "-Wformat-security"

#include <linux/export.h>
#include <linux/init.h>
#include <linux/module.h>

#undef EXPORT_SYMBOL
#undef EXPORT_SYMBOL_GPL
#define EXPORT_SYMBOL(sym)
#define EXPORT_SYMBOL_GPL(sym)

/* The core's init and exit, which ufs-mediatek.c's own call */
#undef module_init
#undef module_exit
#define module_init(fn) int __init ufs_mtk_core_init(void) { return fn(); }
#define module_exit(fn) void ufs_mtk_core_exit(void) { fn(); }

#endif
