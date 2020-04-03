#include "xpcie.h"

LIST_HEAD(engine_list);
LIST_HEAD(xcdev_list);
LIST_HEAD(xcryptodev_list);


int xpcie_init(struct xpcie *xpcie_dev){
    enum engine_type type = ENGINE_TYPE_UNKNOWN;
    int engine_num = MAX_ENGINES;//, xcdev_num = 2, xcryptodev_num = 2;
    int i, err = 0;
    struct xpcie_data *xpcie_data = xpcie_dev->data;
    struct xpcie_resource *resource;
    struct pci_dev *pci_dev = xpcie_data->pci_dev;
    /**
     * init lock
     */
    spin_lock_init(&xpcie_dev->lock);
    /**
     * init pcie 
     */
    resource = xpcie_setup_resource(pci_dev);
    xpcie_data->total_engines = engine_num;
    xpcie_data->pci_dev = pci_dev;
    /**
     * init engine
     * init xcdev
     * init xcryptodev
     */

    for (i = 0; i < MAX_ENGINES; i++){
        spin_lock_init(&(xpcie_data->engine + i)->lock);
        spin_lock_init(&(xpcie_data->xcdev + i)->lock);
        spin_lock_init(&(xpcie_data->xcryptodev + i)->lock);

    }

    return 0;
}

void xpcie_exit(struct xpcie *dev){
    kfree(dev->data);
    kfree(dev);
}

int xpcie_setup_routine(struct xpcie *dev){
    x_info("xpcie_setup_routine \n");
    return 0;
}

void xpcie_remove_routine(struct xpcie *xpcie_dev)
{
    
    x_info("xpcie_remove_routine\n");

}

void free_xpcie_dev(struct xpcie *xpcie)
{
    
}


void fxpcie_free_resource(struct xpcie *dev);

struct xpcie_ops ops = {

};

struct xpcie * alloc_xpcie_dev(struct pci_dev *pci_dev){

    struct xpcie * xpcie_dev;
    struct xpcie_data* xpcie_data;
    int i;

    xpcie_dev = (struct xpcie *)kmalloc(sizeof(*xpcie_dev), GFP_KERNEL);
    if (!xpcie_dev)
        return NULL;
    memset(xpcie_dev, 0, sizeof(*xpcie_dev));

    xpcie_data = (struct xpcie_data *)kmalloc(sizeof(struct xpcie_data), GFP_KERNEL);
    if (!xpcie_data)
        return NULL;
    memset(xpcie_data, 0, sizeof(*xpcie_data));
    xpcie_dev->data = xpcie_data;
    return xpcie_dev;
}




struct xpcie_resource *xpcie_setup_resource(struct pci_dev *pci_dev)
{
    return NULL;
}

void fxpcie_free_resource(struct xpcie *dev)
{

}