#include "xpcie.h"

int xpcie_init(struct xpcie_dev *dev){
    /**
     * 
     */

    return 0;
}

void xpcie_exit(struct xpcie_dev *dev){

}

int xpcie_setup_routine(struct xpcie_dev *dev){

    return 0;
}
struct xpcie_ops {

} ops;

struct xpcie * alloc_xpcie_dev(struct pci_dev *dev){

    struct xpcie * xpcie_dev;
    struct xpcie_data* xpcie_data;

    xpcie_dev = (struct xpcie *)kmalloc(sizeof(*dev), GFP_KERNEL);
    if (!xpcie_dev)
        return NULL;

    xpcie_data = (struct xpcie_data *)kmalloc(sizeof(struct xpcie_data), GFP_KERNEL);
    if (!xpcie_data)
        return NULL;
    
    xpcie_dev->finit = xpcie_init;
    xpcie_dev->fexit = xpcie_exit;
    xpcie_dev->fsetup_routine = xpcie_setup_routine;
    xpcie_dev->data = xpcie_data;
    xpcie_data->pci_dev = dev;
    xpcie_dev->ops = &ops;

    return xpcie_dev;
}


