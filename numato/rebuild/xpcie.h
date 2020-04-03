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
#define MAX_ENGINES 4

struct xpcie_data {
    char name[MAX_NAME_LENGTH];
    struct pci_dev *pci_dev;
    int total_engines;
    int total_interrupt_lines;
    struct engine_struct engine[MAX_ENGINES];
    struct xcdev_struct xcdev[MAX_ENGINES];
    struct xcryptodev_struct xcryptodev[MAX_ENGINES];
};

struct xpcie_resouce {

};

struct xpcie {
    spinlock_t lock;
    struct xpcie_data *data;
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
struct xpcie_resource *xpcie_setup_resource(struct pci_dev *pci_dev);
int xpcie_init(struct xpcie *xpcie_dev);
void xpcie_exit(struct xpcie *dev);



#endif