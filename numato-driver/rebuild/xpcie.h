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

#include "libxdma.h"
#include "engine.h"
#include "xcdev.h"
#include "xcryptodev.h"

#define MAX_NAME_LENGTH (64)
#define MAX_ENGINES (4)
#define MAX_BAR_NUM (6)

struct xpcie {
    spinlock_t lock;
    int cnt;
    char name[MAX_NAME_LENGTH];
    struct pci_dev *pci_dev;
    /**
     * 
     */
    struct engine_struct engine_h2c[MAX_ENGINES];
    struct engine_struct engine_c2h[MAX_ENGINES];

    struct xcdev_struct xcdev[MAX_ENGINES];
    struct xcryptodev_struct xcryptodev[MAX_ENGINES];


    /**
     * PCIe Related
     */

    /**
     * PCIe BAR management
     */
    void *bar[MAX_BAR_NUM];
    int bar_len[MAX_BAR_NUM];
    int active_bar;
    int axi_lite_bar_idx;
    int axi_dma_bar_idx;
    int axi_dma_bypass_bar_idx;

    int c2h_channel_max;
    int h2c_channel_max;

    int regions_in_use;
    int got_regions;

    /**
     * Interrupt management
     */
    int user_max;
    int magic;
};


struct xpcie * alloc_xpcie_dev(struct pci_dev *pci_dev);
int xpcie_device_open(struct xpcie *xpcie_dev,struct pci_dev *pci_dev);
void fxpcie_free_resource(struct xpcie *xpcie_dev);

int xpcie_init(struct xpcie *xpcie_dev);
void xpcie_exit(struct xpcie *dev);

#endif