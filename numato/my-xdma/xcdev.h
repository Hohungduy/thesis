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
    /**
     * Common operations
     */
    int  (*finit)(struct xcdev_struct *xcdev);
    void (*fexit)(struct xcdev_struct *xcdev);
    int  (*fsetup_routine)(struct xcdev_struct *xcdev);
    /**
     * Object data
     */
    spinlock_t lock;
    struct {
        struct engine_struct engine;
    } data;
    /**
     * Specific Operations
     */
    struct xcdev_ops *ops;
};
#endif