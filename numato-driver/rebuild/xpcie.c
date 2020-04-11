#include "xpcie.h"


int xpcie_init(struct xpcie *xpcie_dev){
    int i, err = 0;
    struct pci_dev *pci_dev = xpcie_dev->pci_dev;
    /**
     * init lock
     */
    spin_lock_init(&xpcie_dev->lock);
    /**
     * init pcie 
     */
    err = xpcie_setup_resource(xpcie_dev,pci_dev);
    if (err){
        x_info_failed("FAILED");
    }
    xpcie_dev->pci_dev = pci_dev;
    /**
     * init engine
     * init xcdev
     * init xcryptodev
     */
    return 0;
}

void xpcie_exit(struct xpcie *dev){
    fxpcie_free_resource(dev);
    kfree(dev);
}


void fxpcie_free_resource(struct xpcie *xpcie_dev){

}


int xpcie_device_open(struct xpcie *xpcie_dev,struct pci_dev *pci_dev)
{
    int rv;
    rv = pci_enable_device(pci_dev);
    if (rv){
        x_info_failed("pci_enable_device failed");
        goto err_enable;
    }
    // Keep INTx enabled
    pci_check_intr_pend(pci_dev);
    // Enable relaxed ordering
    // For silent: http://e2e.ti.com/support/processors/f/791/t/245290?PCI-Express-memory-cache-coherency-and-relaxed-ordering-in-DSP6678

    pci_enable_capability(pci_dev, PCI_EXP_DEVCTL_RELAX_EN);
    // Enable extended tag
    // For silent: https://patchwork.kernel.org/patch/9349411/
    pci_enable_capability(pci_dev, PCI_EXP_DEVCTL_EXT_TAG);
    // Force MRRS to be 512
    // For silent: https://blog.linuxplumbersconf.org/2017/ocw/system/presentations/4737/original/mps.pdf
    rv = pcie_set_readrq(pci_dev, 512);
    if (rv)
        pr_info("device %s, error set PCI_EXP_DEVCTL_READRQ: %d.\n",
            dev_name(&pci_dev->dev), rv);

    // Enable bus master capability
    pci_set_master(pci_dev);

    rv = request_regions(xpcie_dev, pci_dev);
    if (rv)
        goto err_regions;

    // Map all bars and save info to xpcie_dev
    rv = map_bars(xpcie_dev, pci_dev);
    if (rv)
        goto err_map;
    
    rv = pcie_check_designated_config(xpcie_dev);
    if (rv)
        goto err_map;
    
    rv = set_dma_mask(pci_dev);
    if (rv)
        goto err_mask;

    /**
     * interrupt disable and print
     * 
     */
    // check_nonzero_interrupt_status(xdev);
	// /* explicitely zero all interrupt enable masks */
	// channel_interrupts_disable(xdev, ~0);
	// user_interrupts_disable(xdev, ~0);
	// read_interrupts(xdev);

    rv = probe_engines(xpcie_dev);

err_mask:
    unmap_bars(xpcie_dev, pci_dev);
err_map:
    if (xpcie_dev->got_regions)
        pci_release_regions(pci_dev);
err_regions:
    if (xpcie_dev->regions_in_use)
        pci_disable_device(pci_dev);
err_enable:
    return rv;
}
#define MAGIC_DEVICE (0xDDDDDDDDUL)
#define MAX_USER_IRQ (16)
#define MAX_CHANNEL_NUM (4)
struct xpcie * alloc_xpcie_dev(struct pci_dev *pci_dev){

    struct xpcie * xpcie_dev;
    xpcie_dev = (struct xpcie *)kmalloc(sizeof(*xpcie_dev), GFP_KERNEL);
    if (!xpcie_dev)
        return NULL;
    memset(xpcie_dev, 0, sizeof(*xpcie_dev));

    xpcie_dev->magic = MAGIC_DEVICE;
    xpcie_dev->pci_dev = pci_dev;
    xpcie_dev->user_max = MAX_USER_IRQ;
    xpcie_dev->h2c_channel_max = MAX_CHANNEL_NUM;
    xpcie_dev->c2h_channel_max = MAX_CHANNEL_NUM;
    xpcie_dev->cnt++;
    
    return xpcie_dev;
}

