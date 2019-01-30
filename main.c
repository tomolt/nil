#include <stdio.h>

#include "object.h"
#include "character.h"
#include "symbol.h"
#include "vector.h"

#include "compiler.h"


void go()
{
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
