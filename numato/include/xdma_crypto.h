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
#include "linux/list.h"
#include "stdbool.h"
// #include "linux/types.h "

#define BUFF_LENGTH (REGION_NUM)
#define BACKLOG_MAX_LENGTH (50)
#define AGENT_NUM (1)
#define CORE_NUM (2)

#ifndef TRUE
#define TRUE (1)
#endif


#ifndef FALSE
#define FALSE (0)
#endif

struct xdma_pci_dev;

/** BUFFER */
#include "common.h"

// struct mycrypto_context {

// };

struct xfer_req{
    int (*crypto_complete)(void *data, int res);
    struct mycrypto_context ctx;
    struct scatterlist *sg;
    int res;

    int id;
    
    u64 data_ep_addr;
    struct region *in_region;
    struct region *out_region;
    int region_idx;
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

struct task_data {
    int idx;
    struct xmit_handler *xmit;
    struct rcv_handler *rcv;
};

#define CHANNEL_NUM (2)

enum rcv_status {
    RCV_STATUS_SLEEP,
    RCV_STATUS_ACTIVE,
    RCV_STATUS_STOP
};

struct rcv_handler {
    struct task_struct *rcv_deliver_task;

    struct task_struct *rcv_task[CHANNEL_NUM];
    spinlock_t rcv_queue_lock[CHANNEL_NUM];
    struct list_head rcv_queue[CHANNEL_NUM];
    struct task_data task_data[CHANNEL_NUM];

    struct task_struct *rcv_callback_task[CHANNEL_NUM];
    spinlock_t rcv_callback_queue_lock[CHANNEL_NUM];
    struct list_head rcv_callback_queue[CHANNEL_NUM];
    struct task_data task_callback_data[CHANNEL_NUM];

    enum rcv_status status;
    spinlock_t region_lock;
    
    struct wait_queue_head wq_rcv_event;
    struct list_head rcv_events_list;
    spinlock_t rcv_events_list_lock;

    u32 booking;
};

enum xmit_status {
    XMIT_STATUS_SLEEP,
    XMIT_STATUS_ACTIVE,
    XMIT_STATUS_STOP
};

struct xmit_handler {
    struct task_struct *xmit_deliver_task;
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

    u32 booking;
};

struct crypto_agent {
    struct xmit_handler xmit;
    struct rcv_handler rcv;

    struct list_head processing_queue;
    int agent_idx;
    u32 xfer_idex;
    int need_deliver;
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

int crdev_create(struct xdma_pci_dev *xpdev);
void crdev_cleanup(void);



void print_deliver_list(void);
void print_processing_list(void);
void print_xmit_list(void);

void delete_deliver_list(struct xdma_crdev *crdev);


struct xfer_req *alloc_xfer_req(void);
int set_sg(struct xfer_req *req, struct scatterlist *sg);
int set_callback(struct xfer_req *req, void *cb);
int set_ctx(struct xfer_req *req, struct mycrypto_context ctx);
int get_result(struct xfer_req *req, int *res);

int xdma_xfer_submit_queue(struct xfer_req *xfer_req);
void free_xfer_req(struct xfer_req *req);


#endif