# common KOs
obj-$(CONFIG_MTK_LOW_POWER_MODULE) += v1/
obj-$(CONFIG_MTK_LOW_POWER_MODULE) += v2/

ifeq ($(CONFIG_MTK_LPM_MT6983),m)
ifneq ($(wildcard $(srctree)/$(src)/mt6983),)
endif
endif

ifeq ($(CONFIG_MTK_LPM_MT6985),m)
ifneq ($(wildcard $(srctree)/$(src)/mt6985),)
endif
endif

ifeq ($(CONFIG_MTK_LPM_MT6886),m)
ifneq ($(wildcard $(srctree)/$(src)/mt6886),)
endif
endif

ifeq ($(CONFIG_MTK_LPM_MT6897),m)
ifneq ($(wildcard $(srctree)/$(src)/mt6897),)
endif
endif

ifeq ($(CONFIG_MTK_LPM_MT6899),m)
ifneq ($(wildcard $(srctree)/$(src)/mt6899),)
obj-$(CONFIG_MTK_LPM_MT6899) += mt6899/
endif
endif

ifeq ($(CONFIG_MTK_LPM_MT6989),m)
ifneq ($(wildcard $(srctree)/$(src)/mt6989),)
endif
endif

ifeq ($(CONFIG_MTK_LPM_MT6991),m)
ifneq ($(wildcard $(srctree)/$(src)/mt6991),)
obj-$(CONFIG_MTK_LPM_MT6991) += mt6991/
endif
endif
