#ifndef __HEADER_XDMA_MOD__
#define __HEADER_XDMA_MOD__


#define KBUILD_MODNAME "my-xdma"

#include <linux/errno.h>
#include <linux/types.h>
#include <linux/ioctl.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/version.h>
#include <linux/pci.h>
#include <linux/spinlock.h>
#include <linux/kthread.h>

#include "xdma_thread.h"

#define DRV_MOD_MAJOR 2020
#define DRV_MOD_MINOR 3
#define DRV_MOD_PATCHLEVEL 51
#define NUMATO_VENDOR_ID 0x10ee
#define NUMATO_DEVICE_ID 0x7024
#define DRIVER_MODULE_NAME "NUMATO-XDMA"
#define DRIVER_MODULE_DESCRIPTION "Long - XDMA rebuild"
#define DRIVER_MODULE_DATE "22 Mar 2020"
#define DRIVER_MODULE_AUTHOR "thanhlongvt98@gmail.com"
#define MAX_NAME_LENGTH 64
#define MAX_MODULE_LENGTH_NAME 64
#define DRIVER_MODULE_VERSION      \
	__stringify(DRV_MOD_MAJOR) "." \
	__stringify(DRV_MOD_MINOR) "." \
	__stringify(DRV_MOD_PATCHLEVEL)

#define DRV_MOD_VERSION_NUMBER  \
	((DRV_MOD_MAJOR)*1000 + (DRV_MOD_MINOR)*100 + DRV_MOD_PATCHLEVEL)

struct xpcie_dev;

static struct xdma_device_struct {
	struct xdma_thread_info * thread_info;
} xdma_device;

struct xdma_thread_info_struct {
	spinlock_t lock;
};
struct engine_dma_h2c {

};
struct engine_dma_c2h {

};
struct engine_bypass_h2c {

};
struct engine_bypass_c2h {

};

struct xdma_cdev {
	union {
		struct engine_dma_h2c engine_dma_h2c;
		struct engine_dma_c2h engine_dma_c2h;
		struct engine_bypass_h2c engine_bypass_h2c;
		struct engine_bypass_c2h engine_bypass_c2h;
	} engine;
	int type;
	/**
	 * cdev related fields
	 */


};

struct xpcie_crypto_ops {
	/**
	 * function for crypto subsystem
	 */
};
struct xpcie_ops {
	/**
	 * Read bars & alloc engines
	 */
	int   (*finit_dma_h2c_channel)(struct xpcie_dev *xpcie);
	void  (*fexit_dma_h2c_channel)(struct xpcie_dev *xpcie);
	int   (*finit_bypass_h2c_channel)(struct xpcie_dev *xpcie);
	void  (*fexit_bypass_h2c_channel)(struct xpcie_dev *xpcie);
	/**
	 * Setup engines:
	 *  + Related fields for interacting with card
	 */
	int   (*fsetup_dma_h2c_channel)(struct engine_dma_h2c *engine);
	int   (*fsetup_dma_c2h_channel)(struct engine_dma_c2h *engine);
	int   (*fsetup_bypass_h2c_channel)(struct engine_bypass_h2c *engine);
	int   (*fsetup_bypass_c2h_channel)(struct engine_bypass_c2h *engine);
	/**
	 * Setup character devices:
	 *  + Related fields for interacting with usr-space
	 */
	int   (*fsetup_cdev)(struct xdma_cdev *cdev);
	/**
	 * Setup crypto interfaces:
	 *  + Related fileds for interacting with cryptosubsystem
	 */
	int   (*fsetup_crypto)(struct xpcie_dev *xpcie);
	int   (*fsetup_crypto_f)(struct xpcie_dev *xpcie, 
					struct xpcie_crypto_ops *ops);
};

struct xpcie_dev_private {

};

struct xpcie_dev {
	char name[MAX_NAME_LENGTH];
	char module_name[MAX_MODULE_LENGTH_NAME];
	// pci_dev struct from probe
	struct pci_dev *pci_dev;
	// number of dma host 2 card channel
	int num_dma_h2c_channel;
	// number of dma card 2 host channel
	int num_dma_c2h_channel;
	// number of bypass h2c channel
	int num_bypass_h2c_channel;
	// number of bypass c2h channel
	int num_bypass_c2h_channel;
	// number of message interrupt
	int num_msi;
	// data
	struct xpcie_dev_private *private;
	// operation with xpcie_dev
	struct xpcie_ops *ops;
	// lock if needed
	spinlock_t lock;
};

#endif