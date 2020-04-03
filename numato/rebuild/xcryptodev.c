#include "xcryptodev.h"

struct xcryptodev_ops xcryptodev_ops;

struct xcryptodev_struct *alloc_xcryptodev(struct engine_struct *engine)
{
    struct xcryptodev_struct *xcryptodev;
    x_info("alloc_xcryptodev started\n");
    
    xcryptodev = (struct xcryptodev_struct *)kmalloc(sizeof(*xcryptodev), GFP_KERNEL);
    if (!xcryptodev){
        x_info_failed("alloc xcdev failed\n");
        return NULL;
    }


    xcryptodev->finit = xcryptodev_init;
    xcryptodev->fexit = xcryptodev_exit;
    xcryptodev->fsetup_routine = xcryptodev_setup_routine;
    xcryptodev->data.engine = engine;
    xcryptodev->ops = &xcryptodev_ops;

    return xcryptodev;
}

int xcryptodev_init(struct xcryptodev_struct *xcryptodev)
{
    x_info("xcryptodev_init started\n");
    spin_lock_init(&xcryptodev->lock);
    INIT_LIST_HEAD(&xcryptodev->next);
    return 0;
}

void xcryptodev_exit(struct xcryptodev_struct *xcryptodev)
{

}
int xcryptodev_setup_routine(struct xcryptodev_struct *xcryptodev)
{
    return 0;
}