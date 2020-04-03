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
    struct list_head *engine_list;
    struct list_head *xcdev_list;
    struct list_head *xcryptodev_list;
};

struct xpcie_resouce {

};

struct xpcie {
    /**
     * Common operation
     */
    int   (*finit)(struct xpcie *xpcie_dev);
    void  (*fexit)(struct xpcie *xpcie_dev);
    int   (*fsetup_routine)(struct xpcie *xpcie_dev);
    void  (*fremove_routine)(struct xpcie *xpcie_dev);
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

struct xpcie_ops {
    /**
     * alloc 3 major objects
     */
    struct engine_struct *(*falloc_engine)(enum engine_type type);
    struct xcryptodev_struct *(*falloc_xcryptodev)(struct engine_struct *engine);
    struct xcdev_struct *(*falloc_xcdev)(struct engine_struct *engine);



    struct xpcie_resource *(*fxpcie_setup_resource)(struct pci_dev *pci_dev);
    void (*fxpcie_free_resource)(struct xpcie *dev);
    // void (*ffree_xpcie_dev)(struct xpcie *xpcie);
};


struct xpcie * alloc_xpcie_dev(struct pci_dev *pci_dev);
void free_xpcie_dev(struct xpcie *xpcie);

struct xpcie_resource *xpcie_setup_resource(struct pci_dev *pci_dev);

#endif