#ifndef __HEADER_LIBXDMA__
#define __HEADER_LIBXDMA__

#include <linux/errno.h>
#include <linux/types.h>
#include <linux/ioctl.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/version.h>
#include <linux/pci.h>
#include <linux/spinlock.h>
#include <linux/uaccess.h>
#include <linux/init.h>
#include <linux/workqueue.h>

#include "xdebug.h"
#include "xpcie.h"
#include "engine.h"

/**
 * Declaration
 * Avoid loop include
 */
struct xpcie;
/**
 * Keep interrupt enabled
 */
void pci_check_intr_pend(struct pci_dev *pdev);
/**
 * Enable capability for PCI Express
 * For silent: How to find struct capability
 * meaning of struct capability
 * 
 */
void pci_enable_capability(struct pci_dev *pdev, int cap);
/**
 * 
 */
int request_regions(struct xpcie *xpcie_dev, struct pci_dev *pci_dev);
/**
 * 
 */
int map_bars(struct xpcie *xpcie_dev, struct pci_dev *pci_dev);
void unmap_bars(struct xpcie *xpcie_dev, struct pci_dev *pci_dev);

/**
 * 
 */
int map_single_bar(struct xpcie *xpcie_dev, struct pci_dev *pci_dev, int i);

/**
 * 
 */
#define AXI_LITE_BAR_CONFIG (0)
#define AXI_DMA_BYPASS_CONFIG (1)
#define AXI_DMA_CONFIG (2)

int pcie_check_designated_config(struct xpcie *xpcie_dev);

/**
 * 
 */
int set_dma_mask(struct pci_dev *pci_dev);
/**
 * 
 */
int probe_engines(struct xpcie *xpcie_dev);
int probe_for_engine(struct xpcie *xpcie_dev, enum dma_data_direction dir,
                        int channel);
/**
 * 
 */
int get_engine_id(struct engine_regs *regs);
int get_engine_channel_id(struct engine_regs *regs);

#endif