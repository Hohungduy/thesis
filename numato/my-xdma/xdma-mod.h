#ifndef __XDMA_VERSION_H__
#define __XDMA_VERSION_H__

#define KBUILD_MODNAME "my-xdma"

#include <linux/errno.h>
#include <linux/types.h>
#include <linux/ioctl.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/version.h>
#include <linux/pci.h>

#define DRV_MOD_MAJOR 2020
#define DRV_MOD_MINOR 3
#define DRV_MOD_PATCHLEVEL 51
#define NUMATO_VENDOR_ID 0x10ee
#define NUMATO_DEVICE_ID 0x7024
#define DRIVER_MODULE_NAME "NUMATO-XDMA"
#define DRIVER_MODULE_DESCRIPTION "Long - XDMA rebuild"
#define DRIVER_MODULE_DATE "22 Mar 2020"
#define DRIVER_MODULE_AUTHOR "thanhlongvt98@gmail.com"
#define DRIVER_MODULE_VERSION      \
	__stringify(DRV_MOD_MAJOR) "." \
	__stringify(DRV_MOD_MINOR) "." \
	__stringify(DRV_MOD_PATCHLEVEL)

#define DRV_MOD_VERSION_NUMBER  \
	((DRV_MOD_MAJOR)*1000 + (DRV_MOD_MINOR)*100 + DRV_MOD_PATCHLEVEL)

#endif /* ifndef __XDMA_VERSION_H__ */
