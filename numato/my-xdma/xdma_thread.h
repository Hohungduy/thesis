#ifndef __HEADER_XDMA_THREAD__
#define __HEADER_XDMA_THREAD__

#include <linux/errno.h>
#include <linux/types.h>
#include <linux/ioctl.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/version.h>
#include <linux/pci.h>
#include <linux/spinlock.h>
#include <linux/kthread.h>
#include <linux/workqueue.h>

struct xdma_kthread_struct {
    // lock for thread
    spinlock_t lock;
    // task_struct fr_thread
    struct task_struct *task;
    // name
    char name[64];
    //cpu
    int cpu;
    //
    // task count
    int cnt;
    // init function for kthread
    int (*f_init_thread)(struct xdma_kthread_struct *kthread);
    // exit function for kthread
    void (* f_exit_thread)(struct xdma_kthread_struct *kthread);
    // request queue for this kthread
};


#endif