#include <stdint.h>

// 65536 memory locations
uint16_t memory[UINT16_MAX];

// registers for LC-3
enum {
    R_R0 = 0,
    R_R1,
    R_R2,
    R_R3,
    R_R4,
    R_R5,
    R_R6,
    R_R7,
    R_PC, // program counter
    R_COND,
    R_COUNT
};

uint16_t reg[R_COUNT];

// Instruction set
enum {
    OP_BR = 0,  // branch
    OP_ADD,     // add
    OP_LD,      // load 
    OP_ST,      // store
    OP_JSR,     // jump register
    OP_AND,     // bitwise and  
    OP_LDR,     // load register 
    OP_STR,     // store register 
    OP_RTI,     // unused
    OP_NOT,     // bitwise not 
    OP_LDI,     // load indirect 
    OP_STI,     // store indirect
    OP_JMP,     // jump
    OP_RES,     // reserved
    OP_LEA,     // load effective address
    OP_TRAP     // execute trap
};

// Condition flags
enum {
    FL_POS = 1 << 0, // positive
    FL_ZRO = 1 << 1, // zero
    FL_NEG = 1 << 2, // negative
};


