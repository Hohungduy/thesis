#include "xcdev.h"

struct xcdev_ops xcdev_ops;

struct xcdev_struct *alloc_xcdev(struct engine_struct *engine)
{
    struct xcdev_struct * xcdev;
    x_info("alloc_xcdev started\n");
    
    xcdev = (struct xcdev_struct *)kmalloc(sizeof(*xcdev), GFP_KERNEL);
    if (!xcdev){
        x_info_failed("alloc xcdev failed\n");
        return NULL;
    }


    xcdev->finit = xcdev_init;
    xcdev->fexit = xcdev_exit;
    xcdev->fsetup_routine = xcdev_setup_routine;
    xcdev->data.engine = engine;
    xcdev->ops = &xcdev_ops;

    return xcdev;
}

int xcdev_init(struct xcdev_struct *xcdev)
{
    x_info("xcdev_init started\n");
    spin_lock_init(&xcdev->lock);
    INIT_LIST_HEAD(&xcdev->next);
    return 0;
}

void xcdev_exit(struct xcdev_struct *xcdev)
{

}
int xcdev_setup_routine(struct xcdev_struct *xcdev)
{
    return 0;
}