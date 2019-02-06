#pragma once

#ifndef BABY_READER_H_
#define BABY_READER_H_

#include <stdio.h>
#include <stdbool.h>

#include "object.h"


objptr_t baby_read(FILE*, bool*);


#endif
