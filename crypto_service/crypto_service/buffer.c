#include "buffer.h"
#include <linux/circ_buf.h>

struct circle_buff* alloc_circle_buff(int size, int mem_size)
{
    struct circle_buff *cbuff;
    void *buff;
    buff = kzalloc(size*mem_size, GFP_KERNEL);
    if (!buff){
        return -1;
    }
    cbuff = kzalloc(sizeof(*cbuff), GFP_KERNEL);
    if (!cbuff){
        return -1;
    }
    cbuff->buff = buff;
    cbuff->head = 0;
    cbuff->tail = 0;
    cbuff->memsize = mem_size;
}

bool is_cbuff_full(struct circle_buff* cbuff)
{
    if ((cbuff->head % cbuff->size) == 
        ((cbuff->tail - 1)% cbuff->size))
        return 1;
    else
        return 0;
}

bool is_cbuff_empty(struct circle_buff* cbuff)
{
    if ((cbuff->head % cbuff->size) == 
        ((cbuff->tail)% cbuff->size))
        return 1;
    else
        return 0;
}

int push(struct circle_buff *cbuff, void *data)
{
    if (is_cbuff_full(cbuff))
        return -1;

    cbuff->head = (cbuff->head + 1) % cbuff->size;
    memccpy(cbuff->buff[cbuff->memsize * cbuff->head], 
        data, cbuff->memsize, 0);
    return 0;
}

int pull(struct circle_buff *cbuff, void *data)
{
    if (is_cbuff_empty(cbuff))
        return -1;
    
    memccpy(data, cbuff->buff[cbuff->memsize * cbuff->head], 
        cbuff->memsize, 0);
    cbuff->tail = (cbuff->tail + 1) % cbuff->size;
    return 0;
}

