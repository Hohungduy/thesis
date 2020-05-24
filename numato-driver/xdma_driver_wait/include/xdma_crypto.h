#ifndef __xdma_crypto_h__
#define __xdma_crypto_h__

#include "xdma_mod.h"
#include <linux/module.h>
#include "xdma_region.h"
#include "libxdma_api.h"
#include "linux/atomic.h"
#include "linux/workqueue.h"
#include "linux/kthread.h"

#define BUFF_LENGTH (REGION_NUM)
#define BACKLOG_MAX_LENGTH (50)

#define CORE_NUM (2)

struct xdma_pci_dev;

/** BUFFER */
// #include "common.h"

struct mycrypto_context {

};

struct xfer_req{
    int (*complete)(void *data, int res);
    void *data;
    struct scatterlist *sg;
    struct mycrypto_context ctx;
    int id; 
    struct list_head list;
};

/** LED */

enum led_state {
    RED,
    BLUE,
    RED_BLUE,
};

struct blinky {
    struct timer_list timer;
    unsigned int interval;
    enum led_state led;
};

struct event {
    struct list_head lh;
    bool stop;
    bool print;
};

struct rcv_handler {
    struct task_struct *xfer_rcv_task[CORE_NUM];
    struct list_head callback_queue[CORE_NUM];
    spinlock_t callback_queue_lock[CORE_NUM];
    struct task_struct *xfer_deliver_task;

    struct wait_queue_head wq_event;
    struct list_head events_list;
    spinlock_t events_lock;
};

struct xdma_crdev {
    struct xdma_pci_dev* xpdev;
    struct xdma_dev *xdev;
    spinlock_t lock;
    struct blinky blinky;
    struct list_head req_processing;
    struct list_head req_queue;
    struct transport_engine *transport;
    struct rcv_handler rcv_data;
    // struct 
};

struct transport_engine {
    spinlock_t lock;
    int channel[CORE_NUM];
};
 

int crdev_create(struct xdma_pci_dev *xpdev);
void crdev_cleanup(void);



void print_req_queue(void);
void print_req_processing(void);



struct xfer_req *alloc_xfer_req(void);
ssize_t xdma_xfer_submit_queue(struct xfer_req *xfer_req);
void free_xfer_req(struct xfer_req *req);


#endif