// SPDX-License-Identifier: GPL-2.0
/* The kernel's ufshcd.c, built into ufs-mediatek-mod: see ufs-core.h */
#include "ufs-core.h"
/* ufs_mtk_module_init() calls the core's exit when the platform driver fails to register */
#undef __exit
#define __exit
#include <drivers/ufs/core/ufshcd.c>
