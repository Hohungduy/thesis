#include "xpcie.h"

LIST_HEAD(engine_list);
LIST_HEAD(xcdev_list);
LIST_HEAD(xcryptodev_list);


int xpcie_init(struct xpcie *xpcie_dev){
    enum engine_type type = ENGINE_TYPE_UNKNOWN;
    int engine_num = 2;//, xcdev_num = 2, xcryptodev_num = 2;
    int i, err = 0;
    struct engine_struct *engine;
    struct xcdev_struct *xcdev;
    struct xcryptodev_struct *xcryptodev;
    struct xpcie_resource *resource;
    struct pci_dev *pci_dev = xpcie_dev->data->pci_dev;
    /**
     * init lock
     */
    spin_lock_init(&xpcie_dev->lock);
    /**
     * init list
     */
    xpcie_dev->data->engine_list = &engine_list;
    xpcie_dev->data->xcdev_list  = &xcdev_list;
    xpcie_dev->data->xcryptodev_list = &xcryptodev_list;
    /**
     * init pcie 
     */
    resource = xpcie_dev->ops->fxpcie_setup_resource(pci_dev);
    xpcie_dev->data->total_engines = engine_num;

    for (i = 0; i < engine_num; i++){
        /**
         * alloc & init engines
         */
        engine = xpcie_dev->ops->falloc_engine(type);
        err = engine->finit(engine);
        if (err){
            /**
             * fail
             */
        }
        list_add(&engine->next, xpcie_dev->data->engine_list);
        /**
         * alloc & init xcdev
         */
        xcdev = xpcie_dev->ops->falloc_xcdev(engine);
        err = xcdev->finit(xcdev);
        if (err){
            /**
             * fail
             */
        }
        list_add(&xcdev->next,  xpcie_dev->data->xcdev_list);
        /**
         * alloc & init xcrypto dev
         */
        xcryptodev = xpcie_dev->ops->falloc_xcryptodev(engine);
        err = xcryptodev->finit(xcryptodev);
        if (err){
            /**
             * fail
             */
        }
        list_add(&xcdev->next, xpcie_dev->data->xcryptodev_list);
    }
    





    return 0;
}

void xpcie_exit(struct xpcie *dev){
    struct list_head *p, *n;
    struct xcdev_struct *xcdev;
    struct xcryptodev_struct *xcryptodev;
    struct engine_struct *engine;

    dev->fremove_routine(dev);  

    list_for_each_safe(p,  n, dev->data->xcdev_list){
        xcdev = list_entry(p, struct xcdev_struct, next);
        list_del(&xcdev->next);
        kfree(xcdev);
    }

    list_for_each_safe(p, n, dev->data->xcryptodev_list){
        xcryptodev = list_entry(p, struct xcryptodev_struct, next);
        list_del(&xcryptodev->next);
        kfree(xcryptodev);
    }

    list_for_each_safe(p, n, dev->data->engine_list){
        engine = list_entry(p, struct engine_struct, next);
        list_del(&engine->next);
        kfree(engine);
    }
    
    dev->ops->fxpcie_free_resource(dev);
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
    .falloc_engine = alloc_engine,
    .falloc_xcryptodev = alloc_xcryptodev,
    .falloc_xcdev = alloc_xcdev,
    // .ffree_xpcie_dev = free_xpcie_dev,

    .fxpcie_setup_resource = xpcie_setup_resource,
    .fxpcie_free_resource = fxpcie_free_resource
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
    xpcie_dev->fremove_routine = xpcie_remove_routine;
    xpcie_dev->data = xpcie_data;
    xpcie_data->pci_dev = pci_dev;
    xpcie_dev->ops = &ops;
    return xpcie_dev;
}




struct xpcie_resource *xpcie_setup_resource(struct pci_dev *pci_dev)
{
    return NULL;
}

void fxpcie_free_resource(struct xpcie *dev)
{

}