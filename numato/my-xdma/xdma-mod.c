#include "xdma-mod.h"

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR(DRIVER_MODULE_AUTHOR);
MODULE_DESCRIPTION(DRIVER_MODULE_DESCRIPTION);
MODULE_VERSION(DRIVER_MODULE_VERSION);

const struct pci_device_id id_table[] = {
    {PCI_DEVICE( NUMATO_VENDOR_ID , NUMATO_DEVICE_ID ), }
};

int xdma_probe(struct pci_dev *dev, const struct pci_device_id *id)
{
    return 0;

}

void xdma_remove(struct pci_dev *dev)
{

}

int xdma_suspend(struct pci_dev *dev, pm_message_t state)
{
    return 0;

}
int  xdma_resume_early(struct pci_dev *dev)
{
    return 0;

}

int  xdma_resume(struct pci_dev *dev)
{
    return 0;

}

void  xdma_shutdown(struct pci_dev *dev)
{

}
pci_ers_result_t xdma_error_detected(struct pci_dev *dev,
					   enum pci_channel_state error)
{

    return PCI_ERS_RESULT_RECOVERED;
}
pci_ers_result_t xmda_mmio_enabled(struct pci_dev *dev)
{
    return PCI_ERS_RESULT_RECOVERED;
}
pci_ers_result_t xdma_slot_reset(struct pci_dev *dev)
{
    return PCI_ERS_RESULT_RECOVERED;
}

void xdma_reset_prepare(struct pci_dev *dev)
{

}
void xdma_reset_done(struct pci_dev *dev)
{

}

void xdma_error_resume(struct pci_dev *dev)
{

}

const struct pci_error_handlers xdma_err_handler = {
    .error_detected = xdma_error_detected,
    .mmio_enabled = xmda_mmio_enabled,
    .slot_reset = xdma_slot_reset,
    .reset_prepare = xdma_reset_prepare,
    .reset_done = xdma_reset_done,
    .resume = xdma_error_resume
};
struct pci_driver xdma_pci_driver = {
    .name = DRIVER_MODULE_NAME,
    .id_table = id_table,
    .probe = xdma_probe,
    .remove = xdma_remove,
    .suspend = xdma_suspend,
    .resume_early = xdma_resume_early,
    .resume = xdma_resume,
    .shutdown = xdma_shutdown,
    .err_handler = &xdma_err_handler
};

int xdma_cdev_init(void)
{
    return 0;
}
void xdma_cdev_cleanup(void)
{

}
static int xdma_init(void)
{
    int rv;
    pr_info("%s", "xdma_init\n");

    rv = xdma_cdev_init();
    if (rv < 0)
        return rv;

    return pci_register_driver(&xdma_pci_driver);
}

static void xdma_exit(void)
{

    pr_info("%s", "xdma_exit\n");
    pci_unregister_driver(&xdma_pci_driver);
    xdma_cdev_cleanup();

}

module_init(xdma_init);
module_exit(xdma_exit);