#include "xdma_region.h"

static struct global_mem *config_base;

void set_base(void *base)
{
    config_base = base;
}

static inline u32 get_next_inb_region_addr(void)
{
    return ioread32(&config_base->
        cb.NEXT_INB_REGION);
}
static inline u32 get_next_outb_region_addr(void)
{
    return ioread32(&config_base->
        cb.NEXT_OUTB_REGION);
}

static inline u32 get_final_inb_region_addr(void)
{
    return ioread32(&config_base->
        cb.FINAL_INB_REGION);
}

static inline u32 get_final_outb_region_addr(void)
{
    return ioread32(&config_base->
        cb.FINAL_OUTB_REGION);
}

inline u32 get_h2c_buffer_size(void)
{
    return ioread32(&config_base->
        cb.H2C_BUFFER_SIZE);
}

inline u32 get_c2h_buffer_size(void)
{
    return ioread32(&config_base->
        cb.C2H_BUFFER_SIZE);
}

inline u32 get_inb_base(void)
{
    return config_base + INB_BASE;
}

// #ifdef DEBUG_REGION
// #define debug_mem debug_mem

// static void _debug_mem(u32 start, u32 end)
// {
//     u32 idx = 0;
//     u8 *buff;

//     start = (start >> 2) << 2;
//     end   = (end >> 2) << 2;

//     if (end < start){
//         pr_err(" end < start ?\n");
//         return;
//     }
//     buff = (u8 *)kzalloc(end - start, GFP_KERNEL);
//     memcpy_fromio(buff, config_base + start, end - start);

//     for (idx = start; idx < end; idx += 4*4){
//         pr_info("%08x = %08x %08x %08x %08x", idx,  
//             ioread32(config_base + idx), ioread32(config_base + idx + 4),
//             ioread32(config_base + idx + 8), ioread32(config_base + idx +12));
//     }
//     kfree(buff);
// }

// void debug_mem(void)
// {
//     _debug_mem(0x00000000, MAX_REGION_ADDR_TEST);
// }

// #else
// #define debug_mem(...)
// #endif

// void reset_mem(void)
// {
//     memset(config_base, 0, MAX_REGION_ADDR_TEST);
// }

int is_real_mem_available(void)
{

    return 0;
}

int is_buff_available(void)
{
    
}

int is_backlog_available(void)
{

}
