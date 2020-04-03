#include "engine.h"

struct engine_ops engine_ops = {

};

struct engine_struct *alloc_engine(enum engine_type type)
{
    struct engine_struct *engine;
    x_info("alloc_engine started\n");
    
    engine = (struct engine_struct *)kmalloc(sizeof(*engine), GFP_KERNEL);
    if (!engine){
        x_info_failed("alloc engine failed\n");
        return NULL;
    }


    engine->finit = engine_init;
    engine->fexit = engine_exit;
    engine->fsetup_routine = engine_setup_routine;
    engine->ops = &engine_ops;

    return engine;
}

int engine_init(struct engine_struct *engine)
{
    x_info("engine_init started\n");
    spin_lock_init(&engine->lock);
    INIT_LIST_HEAD(&engine->next);
    
    return 0;
}

void engine_exit(struct engine_struct *engine)
{
    kfree(engine);
}
int engine_setup_routine(struct engine_struct *engine)
{
    return 0;
}
