#ifndef __HEADER_ENGINE__
#define __HEADER_ENGINE__

#include <linux/errno.h>
#include <linux/types.h>
#include <linux/ioctl.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/version.h>
#include <linux/pci.h>
#include <linux/spinlock.h>
#include <linux/kthread.h>

#include "xdebug.h"

enum engine_type {
    ENGINE_TYPE_UNKNOWN,
    ENGINE_TYPE_AXILITE,
    ENGINE_TYPE_BYPASS,
    ENGINE_TYPE_DMA
};

struct engine_ops {

};

struct engine_struct {
    spinlock_t lock;
    enum engine_type type;
};


int engine_setup_routine(struct engine_struct *engine);

#endif