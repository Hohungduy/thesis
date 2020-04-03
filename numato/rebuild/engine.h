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
    /**
     * Common operations
     */
    int  (*finit)(struct engine_struct *engine);
    void (*fexit)(struct engine_struct *engine);
    int  (*fsetup_routine)(struct engine_struct *engine);
    /**
     * Object data
     */
    spinlock_t lock;
    struct list_head next;
    struct {
        enum engine_type type;
    } data;
    /**
     * Specific Operations
     */
    struct engine_ops *ops;
};


struct engine_struct *alloc_engine(enum engine_type type);
int engine_init(struct engine_struct *engine);
void engine_exit(struct engine_struct *engine);
int engine_setup_routine(struct engine_struct *engine);

#endif