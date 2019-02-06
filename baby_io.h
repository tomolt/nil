#pragma once

#ifndef BABY_IO_H_
#define BABY_IO_H_

#include <stdio.h>
#include <stdbool.h>

#include "object.h"


objptr_t baby_read(FILE*, bool*);
void baby_print(objptr_t);


#endif
