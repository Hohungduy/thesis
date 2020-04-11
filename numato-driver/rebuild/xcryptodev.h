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

    spinlock_t lock;
    struct list_head list;
    struct engine_struct *engine;

};

int xcryptodev_setup_routine(struct xcryptodev_struct *xcryptodev);


#endif