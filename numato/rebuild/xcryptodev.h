#ifndef __HEADER_XCRYPTODEV__
#define __HEADER_XCRYPTODEV__

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
 * Object and Operations to interact with 
 * Crypto subsystem
 */

struct xcryptodev_ops {

};

struct xcryptodev_struct {
    /**
     * Common operations
     */
    int  (*finit)(struct xcryptodev_struct *xcryptodev);
    void (*fexit)(struct xcryptodev_struct *xcryptodev);
    int  (*fsetup_routine)(struct xcryptodev_struct *xcryptodev);
    /**
     * Object data
     */
    spinlock_t lock;
    struct list_head next;
    struct {
        struct engine_struct *engine;
    } data;
    /**
     * Specific Operations
     */
    struct xcryptodev_ops *ops;
};

struct xcryptodev_struct *alloc_xcryptodev(struct engine_struct *engine);
int xcryptodev_init(struct xcryptodev_struct *xcryptodev);
void xcryptodev_exit(struct xcryptodev_struct *xcryptodev);
int xcryptodev_setup_routine(struct xcryptodev_struct *xcryptodev);


#endif