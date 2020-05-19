#include "just_4_test.h"

void write_mem(void *base)
{
    struct crypto_engine *engine = (struct crypto_engine *)base;
    int i, j;
    struct region * region;


    iowrite32(ENGINE_NUM, &engine->comm.c2h_buff_size);
    iowrite32(ENGINE_NUM, &engine->comm.h2c_buff_size);


    iowrite32(7, &engine->comm.tail_inb);
    iowrite32(1, &engine->comm.head_inb);

    for (i = 1; i < 8; i++){
        region = &engine->in.region[i];
        iowrite32(0xAAAAAAAA, &region->region_dsc);
        iowrite32(0x00000000 + i, &region->xfer_id);
        iowrite32(0x010105B4, &region->xfer_dsc);

        for (j = 0; j < 1500; j ++){
            iowrite32(i + j, &region->data);
        }
    }

}