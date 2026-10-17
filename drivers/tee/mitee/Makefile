# SPDX-License-Identifier: GPL-2.0
# Xiaomi's mitee: the OP-TEE driver for gold's trusted OS, built with its own
# copy of the TEE subsystem as in stock's module, so that it needs no kernel TEE
# support (CONFIG_TEE stays off, as in gki_defconfig). The 5.10 TEE headers in
# include/ come before the kernel's.
LINUXINCLUDE := -I$(srctree)/$(src)/include -I$(srctree)/$(src)/include/uapi $(LINUXINCLUDE)

obj-m += mitee.o
mitee-y :=	core.o \
		call.o \
		rpc.o \
		supp.o \
		shm_pool.o \
		device.o \
		bench.o \
		mitee_memlog.o \
		tee_core.o \
		tee_shm.o \
		tee_shm_pool.o

subdir-ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/
