#ifndef __HEADER_XPCIE__
#define __HEADER_XPCIE__

#include <linux/errno.h>
#include <linux/types.h>
#include <linux/ioctl.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/version.h>
#include <linux/pci.h>
#include <linux/spinlock.h>
#include <linux/kthread.h>


#include "engine.h"
#include "xcdev.h"
#include "xcryptodev.h"


#define MAX_NAME_LENGTH 64

struct xpcie_data {
    char name[MAX_NAME_LENGTH];
    struct pci_dev *pci_dev;
    int total_engines;
    int total_interrupt_lines;
    struct engine_struct *engine;
};

struct xpcie_ops {

};
struct xpcie {
    /**
     * Common operation
     */
    int   (*finit)(struct xpcie *xpcie_dev);
    void  (*fexit)(struct xpcie *xpcie_dev);
    int   (*fsetup_routine)(struct xpcie *xpcie_dev);
    /**
     * Object Data
     */
    spinlock_t lock;
    struct xpcie_data *data;
    /**
     * Specific Operations
     */
    struct xpcie_ops *ops;
};

struct xpcie * alloc_xpcie_dev(struct pci_dev *dev);

#endif