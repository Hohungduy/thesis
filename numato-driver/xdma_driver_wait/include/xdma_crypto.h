#ifndef __xdma_crypto_h__
#define __xdma_crypto_h__

#include "xdma_mod.h"
#include <linux/module.h>
#include "xdma_region.h"
#include "libxdma_api.h"
#include "linux/atomic.h"
#include "linux/workqueue.h"
#include "linux/kthread.h"
#include "linux/delay.h"

#define BUFF_LENGTH (REGION_NUM)
#define BACKLOG_MAX_LENGTH (50)
#define AGENT_NUM (1)
#define CORE_NUM (2)

struct xdma_pci_dev;

/** BUFFER */
// #include "common.h"

struct mycrypto_context {

};

struct xfer_req{
    int (*crypto_complete)(void *data, int res);
    struct mycrypto_context ctx;

    int id;
    int res;
    struct scatterlist *sg;
    struct sg_table sg_table;
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
    bool rcv_thread[CORE_NUM];
    bool deliver_thread;
};

struct rcv_thread {
    struct task_struct *xfer_rcv_task;
    struct list_head callback_queue;
    spinlock_t callback_queue_lock;
    int cpu;
};

struct rcv_handler {
    struct rcv_thread data[CORE_NUM];
    struct task_struct *xfer_deliver_task;
    spinlock_t region_lock;
};

enum xmit_status {
    XMIT_STATUS_SLEEP,
    XMIT_STATUS_ACTIVE,
    XMIT_STATUS_STOP
};

#define CHANNEL_NUM (2)

struct task_data {
    int idx;
    struct xmit_handler *xmit;
};

struct xmit_handler {
    struct task_struct *deliver_task;
    struct list_head deliver_list;
    spinlock_t deliver_list_lock;
    
    enum xmit_status status;
    spinlock_t region_lock;

    struct wait_queue_head wq_xmit_event;
    struct list_head xmit_events_list;
    spinlock_t xmit_events_list_lock;

    struct task_struct *xmit_task[CHANNEL_NUM];
    spinlock_t xmit_queue_lock[CHANNEL_NUM];
    struct list_head xmit_queue[CHANNEL_NUM];
    struct task_data task_data[CHANNEL_NUM];
};

struct crypto_agent {
    struct xmit_handler xmit;
    struct rcv_handler rcv;

    struct list_head processing_queue;
    int agent_idx;
    int xfer_idex;
    spinlock_t agent_lock;
};

struct xdma_crdev {
    struct xdma_pci_dev* xpdev;
    struct xdma_dev *xdev;
    struct blinky blinky;

    spinlock_t channel_lock;
    int channel_load[CHANNEL_NUM];  

    struct crypto_agent agent[AGENT_NUM];
    
};

struct transport_engine {

};
 

int crdev_create(struct xdma_pci_dev *xpdev);
void crdev_cleanup(void);



void print_req_queue(void);
void print_req_processing(void);



struct xfer_req *alloc_xfer_req(void);
int xdma_xfer_submit_queue(struct xfer_req *xfer_req);
void free_xfer_req(struct xfer_req *req);


#endif