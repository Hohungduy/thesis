#ifndef __HEADER_XCDEV__
#define __HEADER_XCDEV__

#include <linux/errno.h>
#include <linux/types.h>
#include <linux/ioctl.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/version.h>
#include <linux/pci.h>
#include <linux/spinlock.h>
#include <linux/kthread.h>

#include "engine.h"
/**
 * Object and Operations to interact with userspace
 */

struct xcdev_ops {
    
};

struct xcdev_struct {

    spinlock_t lock;
    struct list_head list;
    struct engine_struct *engine;

};

int xcdev_setup_routine(struct xcdev_struct *xcdev);


#endif