#pragma once

#ifndef BYTECODE_H_
#define BYTECODE_H_

typedef unsigned int instr_t;

#define INSTRUCTION_MASK   0xff000000
#define PARAMETER_MASK     0x00ffffff

#define INSTRUCTION(CODE, ARG) ((((CODE) << 24) & INSTRUCTION_MASK) | ((ARG) & PARAMETER_MASK))

#define INSTRUCTION_PART(I) (((I) >> 24) & 0xff)
#define ARGUMENT_PART(I) ((I) & PARAMETER_MASK)

#define INSTR_HALT         0x00
#define INSTR_PUSH_CONST   0x01
#define INSTR_LOOKUP_CONST 0x02
#define INSTR_JMP          0x03
#define INSTR_JMP_IF_NOT   0x04
#define INSTR_CALL         0x05
#define INSTR_TAILCALL     0x06
#define INSTR_SET_CONST    0x07
#define INSTR_DEFINE_CONST 0x07  /* Currently the same as BIND */
#define INSTR_POP          0x08  /* POP n, pops n objects */
#define INSTR_MAKE_CLOSURE 0x09

#endif
