#include "xpcie.h"

int xpcie_init(struct xpcie *xpcie_dev){
    enum engine_type type;
    int engine_num;
    int i;
    struct engine_struct *engine;
    /**
     * init lock
     */
    spin_lock_init(&xpcie_dev->lock);
    /**
     * init pcie 
     */

    /**
     * alloc engines
     */
    for (i = 0; i < engine_num; i++){
        engine = xpcie_dev->ops->falloc_engine(type);
    }
    
    /**
     * call each init function in each engine
     */
    return 0;
}

void xpcie_exit(struct xpcie_dev *dev){

}

int xpcie_setup_routine(struct xpcie_dev *dev){

    return 0;
}
struct xpcie_ops ops = {
    .falloc_engine = alloc_engine
};

struct xpcie * alloc_xpcie_dev(struct pci_dev *pci_dev){

    struct xpcie * xpcie_dev;
    struct xpcie_data* xpcie_data;

    xpcie_dev = (struct xpcie *)kmalloc(sizeof(*xpcie_dev), GFP_KERNEL);
    if (!xpcie_dev)
        return NULL;
    memset(xpcie_dev, 0, sizeof(*xpcie_dev));

    xpcie_data = (struct xpcie_data *)kmalloc(sizeof(struct xpcie_data), GFP_KERNEL);
    if (!xpcie_data)
        return NULL;
    memset(xpcie_data, 0, sizeof(*xpcie_data));
    
    xpcie_dev->finit = xpcie_init;
    xpcie_dev->fexit = xpcie_exit;
    xpcie_dev->fsetup_routine = xpcie_setup_routine;
    xpcie_dev->data = xpcie_data;
    xpcie_data->pci_dev = pci_dev;
    xpcie_dev->ops = &ops;
    return xpcie_dev;
}


