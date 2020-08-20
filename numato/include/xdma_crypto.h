#ifndef __xdma_crypto_h__
#define __xdma_crypto_h__

#include <linux/module.h>
#include "linux/atomic.h"
#include "linux/workqueue.h"
#include "linux/kthread.h"
#include "linux/delay.h"
#include "linux/list.h"
#include "linux/kernel.h"

#include "xdma_mod.h"
#include "libxdma_api.h"
#include "xdma_region.h"
#include "stdbool.h"

#define BUFF_LENGTH (REGION_NUM)
#define BACKLOG_MAX_LENGTH (50)
#define AGENT_NUM (1)
#define CORE_NUM (1)

#ifndef TRUE
#define TRUE (1)
#endif

#ifndef FALSE
#define FALSE (0)
#endif

#define COMM_REGION_OFFSET (0x11000)
#define IN_REGION_OFFSET   (0x00000)
#define OUT_REGION_OFFSET  (0x10000)
#define STATUS_REGION_OFFSET (0x20000)
#define IRQ_REIGON_OFFSET  (0x40000)

#define H2C_TARGET (0)
#define STATUS_OFFSET (0x40)
#define CONTROL_OFFSET (0x04)

#define DEFAULT_CHANNEL_IDX (0)
#define DEFAULT_CORE (1)
#define MAX_REQ_NUM (512)
#define CRYTO_DONE_IRQ (0)
#define XFER_WRITE (1)
#define XFER_READ (0)
#define DEFAULT_ENGINE (0)
struct xdma_pci_dev;

/** BUFFER */
#include "common.h"

// struct mycrypto_context {

// };

struct xfer_req{
// from user

    int (*crypto_complete)(struct xfer_req *data, int res);
    struct mycrypto_context ctx;
    struct crypto_async_request *base;
    struct scatterlist *sg_in;
    struct scatterlist *sg_out;
    struct scatterlist sg;// sg for transfering using dma
    // struct scatterlist sg_rcv;// sg for transfering using dma

    struct crypto_dsc_in crypto_dsc;
    int tag_offset;
    int tag_length;
    u32 *tag;
// Dont touch
    int res;
    int id;
    int region_idx;
    u64 data_ep_addr;
    // struct region_in *in_region;
    // struct region_out *out_region;
    // struct sg_table sg_table;
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

// #define CHANNEL_NUM (2)

// enum rcv_status {
//     RCV_STATUS_SLEEP,
//     RCV_STATUS_ACTIVE,
//     RCV_STATUS_STOP
// };

// struct rcv_handler {
//     struct task_struct *rcv_deliver_task;

//     struct task_struct *rcv_task[CHANNEL_NUM];
//     spinlock_t rcv_queue_lock[CHANNEL_NUM];
//     struct list_head rcv_queue[CHANNEL_NUM];
//     struct task_data task_data[CHANNEL_NUM];

//     struct task_struct *rcv_callback_task[CHANNEL_NUM];
//     spinlock_t rcv_callback_queue_lock[CHANNEL_NUM];
//     struct list_head rcv_callback_queue[CHANNEL_NUM];
//     struct task_data task_callback_data[CHANNEL_NUM];

//     enum rcv_status status;
//     spinlock_t region_lock;
    
//     struct wait_queue_head wq_rcv_event;
//     struct list_head rcv_events_list;
//     spinlock_t rcv_events_list_lock;

//     u32 booking;
// };

// enum xmit_status {
//     XMIT_STATUS_SLEEP,
//     XMIT_STATUS_ACTIVE,
//     XMIT_STATUS_STOP
// };

// struct xmit_handler {
//     struct task_struct *xmit_deliver_task;
//     struct list_head deliver_list;
//     spinlock_t deliver_list_lock;
    
//     enum xmit_status status;
//     spinlock_t region_lock;

//     struct wait_queue_head wq_xmit_event;
//     struct list_head xmit_events_list;
//     spinlock_t xmit_events_list_lock;

//     struct task_struct *xmit_task[CHANNEL_NUM];
//     spinlock_t xmit_queue_lock[CHANNEL_NUM];
//     struct list_head xmit_queue[CHANNEL_NUM];
//     struct task_data task_data[CHANNEL_NUM];

//     u32 booking;
// };

// enum XFER_STATUS {
//     XFER_STATUS_FREE,
//     XFER_STATUS_XMIT,
//     XFER_STATUS_RCV
// };

// struct crypto_agent {
//     struct xmit_handler xmit;
//     struct rcv_handler rcv;

//     int agent_idx;
//     int need_deliver;
//     enum agent_status status;
// };


struct xdma_crdev {


    struct xdma_pci_dev* xpdev;
    struct xdma_dev *xdev;
    struct blinky blinky;

    int channel_idx;
    u32 xfer_idex;
    spinlock_t agent_lock;

    struct list_head req_queue;
    struct list_head cb_queue;

    int req_num;

    struct wait_queue_head crypto_wq;
    struct wait_queue_head cb_wq;
    struct completion encrypt_done;

    struct task_struct *crypto_task;
    struct task_struct *callback_task;

    // struct crypto_agent agent;
    // enum XFER_STATUS xfer_status;
};

int crdev_create(struct xdma_pci_dev *xpdev);
void crdev_cleanup(void);
// void print_xmit_deliver_list(void);
// void print_processing_list(void);
// void print_xmit_list(void);
// void print_rcv_list(void);
// void print_callback_list(void);
// void delete_deliver_list(struct xdma_crdev *crdev);


struct xfer_req *alloc_xfer_req(void);
int set_sg_in(struct xfer_req *req, struct scatterlist *sg);
int set_sg_out(struct xfer_req *req, struct scatterlist *sg);
int set_callback(struct xfer_req *req, int (*cb)(struct xfer_req *req, int res));
int set_ctx(struct xfer_req *req, struct mycrypto_context ctx);
int get_result(struct xfer_req *req, int *res);
int set_tag(struct xfer_req *req, int length, int offset, u32 *buff);

int xdma_xfer_submit_queue(struct xfer_req *xfer_req);
void free_xfer_req(struct xfer_req *req);


#endif