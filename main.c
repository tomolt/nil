#include <stdio.h>

#include "character.h"
#include "vector.h"
#include "symbol.h"

#include "compiler.h"
#include "fiber.h"
#include "baby_io.h"


void go()
{
    bool fail;
    objptr_t func;
    
    FILE *f = fopen("./lib/boot.scm", "r");
    func = compile_to_thunk(baby_read(f, &fail), EMPTY_LIST);
    declare_root_object(func);
    start_in_fiber(func);
    run_main_loop();
    fclose(f);
}


void init()
{
    init_memory_system();
    // TODO: More type inits
    init_characters();
    init_vectors();
    init_symbols();
}


void terminate()
{
    end_memory_system();
    // TODO: Free all type instances, like this:
    terminate_symbols();
    terminate_vectors();
    terminate_characters();
}


int main(int argc, char *argv[])
{
    init();
    go();
    terminate();
    return 0;
}
