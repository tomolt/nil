#pragma once

#ifndef COMPILER_H_
#define COMPILER_H_


#include "bytecode.h"

#include "object.h"



struct code {
    objptr_t constant_vector;
    unsigned int code_size;
    unsigned int code_alloc;
    instr_t *codes;
};


unsigned int code_push_instruction(struct code*, instr_t);

void compile(objptr_t, struct code*);

void init_code(struct code*);
void terminate_code(struct code*);

#endif
