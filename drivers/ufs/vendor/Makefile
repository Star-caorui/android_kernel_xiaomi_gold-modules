# SPDX-License-Identifier: GPL-2.0
# Mediatek UFS extended features makefile
ccflags-y += -I$(DEVICE_MODULES_PATH)/drivers/clk/mediatek/
ccflags-y += -I$(DEVICE_MODULES_PATH)/drivers/misc/mediatek/include/
ccflags-y += -I$(DEVICE_MODULES_PATH)/drivers/misc/mediatek/power_throttling/
ccflags-y += -I$(DEVICE_MODULES_PATH)/drivers/ufs/
ccflags-y += -I$(DEVICE_MODULES_PATH)/drivers/ufs/vendor/

# must after -I$(DEVICE_MODULES_PATH)/drivers/ufs/
ccflags-y += -I$(srctree)/drivers/ufs/
ccflags-y += -I$(srctree)/drivers/ufs/core/
ccflags-y += -I$(srctree)/drivers/ufs/host/

# sd.h, for Xiaomi's firmware update
CFLAGS_../xiaomi/feature/ffu/ufsffu.o += -I$(srctree)/drivers/scsi/

# Xiaomi's copy of the UFS core (gold's bootloader keeps the kernel's own
# from starting), with Xiaomi's firmware update and defrag
UFS_XIAOMI := ../xiaomi/core/ufshcd.o ../xiaomi/core/ufs-sysfs.o ../xiaomi/core/ufs-mcq.o ../xiaomi/core/ufshcd-pltfrm.o
UFS_XIAOMI_FEATURES := ../xiaomi/feature/ffu/ufsffu.o \
		       ../xiaomi/feature/cld/ufscld.o ../xiaomi/feature/cld/ufscld_sysfs.o \
		       ../xiaomi/feature/cld/skhynix.o ../xiaomi/feature/cld/samsung.o \
		       ../xiaomi/feature/cld/kioxia.o ../xiaomi/feature/cld/micron.o

# UFS KO without iSE
obj-$(CONFIG_DEVICE_MODULES_SCSI_UFS_MEDIATEK)  += ufs-mediatek-mod.o
ifeq ($(CONFIG_UFS_MEDIATEK_MT6893),y)
ufs-mediatek-mod-y				+= ../ufs-mediatek-perf.o ../ufs-mediatek-sysfs.o ../ufs-mediatek-mimic.o ../ufs-mediatek-mbrain.o
else
ufs-mediatek-mod-y                              += ../ufs-mediatek.o ../ufs-mediatek-sysfs.o ../ufs-mediatek-mimic.o ../ufs-mediatek-mbrain.o
endif

ufs-mediatek-mod-y                                  += $(UFS_XIAOMI)
ufs-mediatek-mod-$(CONFIG_DEBUG_FS)             += ../xiaomi/core/ufs-debugfs.o
ufs-mediatek-mod-$(CONFIG_SCSI_UFS_BSG)         += ../xiaomi/core/ufs_bsg.o
ufs-mediatek-mod-$(CONFIG_SCSI_UFS_CRYPTO)      += ../xiaomi/core/ufshcd-crypto.o
ufs-mediatek-mod-y                                  += $(UFS_XIAOMI_FEATURES)
ufs-mediatek-mod-$(CONFIG_MTK_BLOCK_IO_TRACER)  += ../ufs-mediatek-btag.o
ufs-mediatek-mod-$(CONFIG_RPMB)                 += ../ufs-mediatek-rpmb.o
ufs-mediatek-mod-$(CONFIG_RPMB)                 += ../ufs-mediatek-ise-dummy.o
ufs-mediatek-mod-$(CONFIG_UFSFEATURE)           += ufsfeature.o ufsfeature-mimic.o
ufs-mediatek-mod-$(CONFIG_UFSSHPB)              += ufsshpb.o
ufs-mediatek-mod-$(CONFIG_UFSTW)                += ufstw.o
ufs-mediatek-mod-$(CONFIG_UFSHID)               += ufshid.o
ufs-mediatek-mod-$(CONFIG_UFSRINGBUF)           += ufsringbuf.o


# UFS KO with iSE enabled
obj-$(CONFIG_DEVICE_MODULES_SCSI_UFS_MEDIATEK)      += ufs-mediatek-mod-ise.o
ifeq ($(CONFIG_UFS_MEDIATEK_MT6893),y)
ufs-mediatek-mod-ise-y                              += ../ufs-mediatek-perf.o ../ufs-mediatek-sysfs.o ../ufs-mediatek-mimic.o ../ufs-mediatek-mbrain.o
else
ufs-mediatek-mod-ise-y                              += ../ufs-mediatek.o ../ufs-mediatek-sysfs.o ../ufs-mediatek-mimic.o ../ufs-mediatek-mbrain.o
endif

ufs-mediatek-mod-ise-y                              += $(UFS_XIAOMI)
ufs-mediatek-mod-ise-$(CONFIG_DEBUG_FS)             += ../xiaomi/core/ufs-debugfs.o
ufs-mediatek-mod-ise-$(CONFIG_SCSI_UFS_BSG)         += ../xiaomi/core/ufs_bsg.o
ufs-mediatek-mod-ise-$(CONFIG_SCSI_UFS_CRYPTO)      += ../xiaomi/core/ufshcd-crypto.o
ufs-mediatek-mod-ise-y                              += $(UFS_XIAOMI_FEATURES)
ufs-mediatek-mod-ise-$(CONFIG_MTK_BLOCK_IO_TRACER)  += ../ufs-mediatek-btag.o
ufs-mediatek-mod-ise-$(CONFIG_RPMB)                 += ../ufs-mediatek-rpmb.o
ufs-mediatek-mod-ise-$(CONFIG_RPMB)                 += ../ufs-mediatek-ise.o
ufs-mediatek-mod-ise-$(CONFIG_UFSFEATURE)           += ufsfeature.o ufsfeature-mimic.o
ufs-mediatek-mod-ise-$(CONFIG_UFSSHPB)              += ufsshpb.o
ufs-mediatek-mod-ise-$(CONFIG_UFSTW)                += ufstw.o
ufs-mediatek-mod-ise-$(CONFIG_UFSHID)               += ufshid.o
ufs-mediatek-mod-ise-$(CONFIG_UFSRINGBUF)           += ufsringbuf.o
