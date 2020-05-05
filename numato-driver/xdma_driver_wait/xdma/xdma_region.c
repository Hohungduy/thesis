#include "xdma_region.h"

struct dsc {
    u32 region_dsc;
    u32 xfer_id;
    u8 xfer_dsc;
    u8 crypto_res;
    u8 data_len;
    u8 magic;
};

static void *config_base;

void set_base(void *base)
{
    config_base = base;
}

inline u32 get_remaining_request(void)
{
    return ioread32(config_base + REMAINING_REQUEST_OFFSET);
}

static inline u32 get_next_dsc_req_addr(void)
{
    return ioread32(config_base + NEXT_DSC_REQUEST_ADDRESS_OFFSET);
}

static inline u32 get_final_dsc_req_addr(void)
{
    return ioread32(config_base + FINAL_DSC_REQUEST_ADDRESS_OFFSET);
}

inline u32 get_h2c_buffer_size(int channel)
{
    if (channel == 0)
        return ioread32(config_base + H2C_CH0_BUFFER_SIZE_OFFSET );
    else
        return ioread32(config_base + H2C_CH1_BUFFER_SIZE_OFFSET);
}

inline u32 get_c2h_buffer_size(int channel)
{
    if (channel == 0)
        return ioread32(config_base + C2H_CH0_BUFFER_SIZE_OFFSET );
    else
        return ioread32(config_base + C2H_CH1_BUFFER_SIZE_OFFSET);
}

void get_next_dsc_req(struct dsc *dsc)
{
    u32 ep_addr;
    ep_addr = get_next_dsc_req_addr();
    memcpy_fromio(dsc, config_base + ep_addr, sizeof(struct dsc) );
}

void get_last_dsc_req(struct dsc *dsc)
{
    u32 ep_addr;
    ep_addr = get_final_dsc_req_addr();
    memcpy_fromio(dsc, config_base + ep_addr, sizeof(struct dsc) );
}

inline bool is_only_last_req(void)
{
    return (get_next_dsc_req_addr() == get_final_dsc_req_addr());
}

inline bool is_free_req(void)
{
    return !get_final_dsc_req_addr() && !get_next_dsc_req_addr();
}

inline bool is_dsc_valid(struct dsc *dsc)
{
    return (dsc->magic == MAGIC_BTYE);
}

inline enum region_state get_region_state(struct dsc *dsc)
{
    switch (dsc->region_dsc)
    {
    case 0xAAAAAAAA:
        return REGION_STATE_EMPTY;
        break;
    case 0xBBBBBBBB:
        return REGION_STATE_NEED_PROCESS;
        break;
    case 0xCCCCCCCC:
        return REGION_STATE_PROCESSING;
        break;
    case 0xDDDDDDDD:
        return REGION_STATE_PROCESSED;
        break;
    default:
    return REGION_STATE_UNKNOWN;
        break;
    }
}

inline bool is_c2h_xfer(struct dsc *dsc)
{
    return (dsc->xfer_dsc == (1 << 0));
}

#ifdef DEBUG_REGION
#define debug_mem debug_mem

static void _debug_mem(u32 start, u32 end)
{
    u32 idx = 0;
    u8 *buff;

    start = (start >> 2) << 2;
    end   = (end >> 2) << 2;

    if (end < start){
        pr_err(" end < start ?\n");
        return;
    }
    buff = (u8 *)kzalloc(end - start, GFP_KERNEL);
    memcpy_fromio(buff, config_base + start, end - start);
    // for (idx = start; idx < end; idx += 4*4){
    //     pr_info("offset = %08x addr %08llx = %08x %08x %08x %08x\n",
    //         idx, (u64)config_base + idx, *((u32 *)buff), 
    //         *((u32 *)buff + 1), *((u32 *)buff + 2), *((u32 *)buff + 3));
    // }
    for (idx = start; idx < end; idx += 4*4){
        pr_info("%08x = %08x %08x %08x %08x", idx,  
            ioread32(config_base + idx), ioread32(config_base + idx + 4),
            ioread32(config_base + idx + 8), ioread32(config_base + idx +12));
    }
    kfree(buff);
}

void debug_mem(void)
{
    _debug_mem(0x000000A0, MAX_REGION_ADDR_TEST);
}

#else
#define debug_mem(...)
#endif