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

int main(int argc, char* argv[]) {
    // TODO: load arguments
    
    enum { PC_START = 0x3000 };
    reg[R_PC] = PC_START;

    int running = 1;
    while (running) {
        // fetch the instruction and increment PC 
        uint16_t instr = mem_read(reg[R_PC]++);
        // Each instruction is 16 bits long, with the left 4 bits
        // storing the opcode
        uint16_t op = instr >> 12;

        switch (op) {
            case OP_ADD:
                // TODO: implement
                break;
            case OP_AND:
                // TODO: implement
                break;
            case OP_NOT:
                // TODO: implement
                break;
            case OP_BR:
                // TODO: implement
                break;
            case OP_JMP:
                // TODO: implement
                break;
            case OP_JSR:
                // TODO: implement
                break;
            case OP_LD:
                // TODO: implement
                break;
            case OP_LDI:
                // TODO: implement
                break;
            case OP_LDR:
                // TODO: implement
                break;
            case OP_LEA:
                // TODO: implement
                break;
            case OP_ST:
                // TODO: implement
                break;
            case OP_STI:
                // TODO: implement
                break;
            case OP_STR:
                // TODO: implement
                break;
            case OP_TRAP:
                // TODO: implement
                break;
            case OP_RES:
            case OP_RTI:
            default:
                // TODO: implement bas opcode
                break;
        }
    }
}
