#include "libxdma.h"

void pci_check_intr_pend(struct pci_dev *pdev)
{
    u16 v;

    pci_read_config_word(pdev, PCI_STATUS, &v);
    if (v & PCI_STATUS_INTERRUPT){
        pr_info("%s PCI STATUS Interrupt pending 0x%x. \n",
            dev_name(&pdev->dev), v);
        pci_write_config_word(pdev, PCI_STATUS, PCI_STATUS_INTERRUPT);
    }
}

void pci_enable_capability(struct pci_dev *pdev, int cap)
{
    pcie_capability_set_word(pdev, PCI_EXP_DEVCTL, cap);
}

int request_regions(struct xpcie *xpcie_dev, struct pci_dev *pci_dev)
{
    int rv;
    if (!xpcie_dev){
        pr_err("Invalid xpcie_dev\n");
        return -EINVAL;
    }

    if (!pci_dev){
        pr_err("Invalid pci_dev\n");
        return -EINVAL;
    }

    dbg_init("pci_request_regions()\n");
    rv = pci_request_regions(pci_dev, xpcie_dev->name);
    // could not request all regions
    if (rv){
        dbg_init("pci_request_region() = %d, device in use ?\n", rv);
        xpcie_dev->regions_in_use = 1;
    } else {
        xpcie_dev->got_regions = 1;
    }
    return rv;
        
}

void unmap_bars(struct xpcie *xpcie_dev, struct pci_dev *pci_dev)
{
    int i;

    for (i=0; i < MAX_BAR_NUM; i++){
        if (xpcie_dev->bar[i]){
            pci_iounmap(pci_dev, xpcie_dev->bar[i]);
            xpcie_dev->bar[i] = NULL;
        }
    }
}

int map_single_bar(struct xpcie *xpcie_dev, struct pci_dev *pci_dev, int i)
{
    resource_size_t start;
    resource_size_t len;
    resource_size_t map_len;

    start = pci_resource_start(pci_dev, i);
    len = pci_resource_len(pci_dev, i);
    map_len = len;

    xpcie_dev->bar[i] = NULL;

    if (!len){
        pr_info("BAR %d is not present - skipping\n", i);
        return 0;
    }

    if (len > INT_MAX){
        pr_info("Limit bar %d size from %llu to %d bytes", i,
            (u64)len, INT_MAX);
        map_len = (resource_size_t)INT_MAX;
    }

    xpcie_dev->bar[i] = pci_iomap(pci_dev, i, map_len);

    if (!xpcie_dev->bar[i]){
        pr_info("Could not map BAR %d\n", i);
        return -1;
    }
    pr_info("BAR%d at 0x%llx mapped at 0x%p, length=%llu(/%llu)\n", i,
        (u64)start, xpcie_dev->bar[i], (u64)map_len, (u64)len);

    return (int)map_len;
}

int map_bars(struct xpcie *xpcie_dev, struct pci_dev *pci_dev)
{
    int i, rv, active_bar;
    active_bar = 0;
    for (i = 0; i < MAX_BAR_NUM; i++){

        int bar_len;
        bar_len = map_single_bar(xpcie_dev, pci_dev, i);

        if (bar_len == 0){
            xpcie_dev->bar_len[i] = 0;
            continue;
        } else if (bar_len < 0){
            rv = -EINVAL;
            goto fail;
        }
        xpcie_dev->bar_len[i] = bar_len;
        active_bar++;
    }

    xpcie_dev->active_bar = active_bar;
    return 0;

fail:
    unmap_bars(xpcie_dev, pci_dev);
    return rv;
}

int pcie_check_designated_config(struct xpcie *xpcie_dev){

#ifdef AXI_LITE_BAR_CONFIG
    if (xpcie_dev->bar_len[AXI_LITE_BAR_CONFIG] == 0)
        goto failed;
#endif

#ifdef AXI_DMA_BYPASS_CONFIG
    if (xpcie_dev->bar_len[AXI_DMA_BYPASS_CONFIG] == 0)
        goto failed;
#endif
    if (xpcie_dev->bar_len[AXI_DMA_CONFIG] == 0)
        goto failed;

    pr_info("Configuration seem right\n");
    return 0;

failed:
    pr_info("The configuration is not suit\n");
    return -1;
}

/**
 * https://www.mjmwired.net/kernel/Documentation/DMA-mapping.txt
 */

int set_dma_mask(struct pci_dev *pci_dev)
{
    if (!pci_dev){
        pr_err("Invalid pdev\n");
        return -EINVAL;
    }

    dbg_init("sizeof(dma_addr_t) == %ld\n", sizeof(dma_addr_t));

    if (!pci_set_dma_mask(pci_dev, DMA_BIT_MASK(64))) {
        dbg_init("Using a 64-bit DMA mask\n");
        pci_set_consistent_dma_mask(pci_dev, DMA_BIT_MASK(32));
    } else if (!pci_set_dma_mask(pci_dev, DMA_BIT_MASK(32))){
        dbg_init("Could not set 64 bit DMA mask\n");
        pci_set_consistent_dma_mask(pci_dev, DMA_BIT_MASK(32));
    } else {
        dbg_init("No suitable DMA possible\n");
        return -EINVAL;
    }

    return 0;
}
/**
 * https://www.xilinx.com/support/documentation/ip_documentation/xdma/v4_1/pg195-pcie-dma.pdf
 * page 42
 */
#define CHANNEL_SPACING (0x100)
#define XDMA_ID_H2C (0x1fc0U)
#define H2C_CHANNEL_OFFSET (0x1000)
#define XDMA_ID_C2H (0x1fc1U)

int get_engine_id(struct engine_regs *regs)
{
    int value;
    if (!regs){

        return -EINVAL;
    }

    value = ioread32(&regs->identifier);
    return (value & 0xffff0000U) >> 16;
}

int get_engine_channel_id(struct engine_regs *regs)
{
    int value;

    if (!regs){

        return -EINVAL;
    }

    value = ioread32(&regs->identifier);
    return (value & 0x00000f00U) >> 8;
}

int probe_for_engine(struct xpcie *xpcie_dev, enum dma_data_direction dir,
                        int channel)
{
    struct engine_regs *regs;
    int offset = channel * CHANNEL_SPACING;
    u32 engine_id;
    u32 engine_id_expected;
    u32 channel_id;
    struct engine_struct *engine;
    int rv;

    switch (dir){
        case DMA_TO_DEVICE:
            engine_id_expected = XDMA_ID_H2C;
            engine = &xpcie_dev->engine_h2c[channel];
            break;
        case DMA_FROM_DEVICE:
            offset += H2C_CHANNEL_OFFSET;
            engine_id_expected = XDMA_ID_C2H;
            engine = &xpcie_dev->engine_c2h[channel];
            break;
        default:
            break;
    }

    regs = xpcie_dev->bar[AXI_DMA_CONFIG] + offset;
    engine_id = get_engine_id(regs);
    channel_id = get_engine_channel_id(regs);
    /** Core idenfifier 0x1fc */

    if ( (engine_id != engine_id_expected) || )

    return 0;
}

int probe_engines(struct xpcie *xpcie_dev){
    int i;
    int rv = 0;

    if (!xpcie_dev){
        pr_err("Invalid xpcie_dev\n");
        return -EINVAL;
    }

    for (i = 0; i < xpcie_dev->h2c_channel_max; i++) {
        rv = probe_for_engine(xpcie_dev, DMA_TO_DEVICE, i);
        if (rv)
            break;
    }
    xpcie_dev->h2c_channel_max = i;

    for (i = 0; i < xpcie_dev->c2h_channel_max; i++) {
        rv = probe_for_engine(xpcie_dev, DMA_FROM_DEVICE, i);
        if (rv)
            break;
    }
    xpcie_dev->c2h_channel_max = i;
    return 0;
}
