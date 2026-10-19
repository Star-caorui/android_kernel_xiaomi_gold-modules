#
# Makefile for the fingerprint device.
#
# Each configuration option enables a list of files.
#
ccflags-y += -I$(DEVICE_MODULES_PATH)/drivers/input/fingerprint/goodix
ccflags-y += -I$(DEVICE_MODULES_PATH)/drivers/spi/mediatek/$(MTK_PLATFORM)
ccflags-y += -I$(DEVICE_MODULES_PATH)/drivers/misc/mediatek/include
ccflags-y += -I$(DEVICE_MODULES_PATH)/drivers/misc/mediatek/include/mt-plat
ccflags-y += -I$(DEVICE_MODULES_PATH)/drivers/misc/mediatek/include/mt-plat/$(MTK_PLATFORM)/include
ccflags-y += -I$(DEVICE_MODULES_PATH)/drivers/misc/mediatek/base/power/include/
ccflags-y += -I$(DEVICE_MODULES_PATH)/drivers/misc/mediatek/base/power/$(MTK_PLATFORM)/
ccflags-y += -I$(DEVICE_MODULES_PATH)/drivers/devfreq/
obj-$(CONFIG_FPC_FINGERPRINT)	+= fpc1022_tee.o
