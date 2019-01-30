#pragma once

#ifndef BYTECODE_H_
#define BYTECODE_H_

typedef unsigned int instr_t;

#define INSTRUCTION_MASK   0xff000000
#define PARAMETER_MASK     0x00ffffff

#define INSTRUCTION(CODE, ARG) ((((CODE) << 24) & INSTRUCTION_MASK) | ((ARG) & PARAMETER_MASK))

#define INSTR_HALT         0x00
#define INSTR_PUSH_CONST   0x01
#define INSTR_LOOKUP_CONST 0x02
#define INSTR_JMP          0x03
#define INSTR_JUMP_IF      0x04
#define INSTR_CALL         0x05
#define INSTR_TAILCALL     0x06

#endif
