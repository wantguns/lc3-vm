#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

// unix
#include <signal.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/termios.h>

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

// Trap codes
enum {
    TRAP_GETC = 0x20,   // get char, not echoed
    TRAP_OUT = 0x21,    // print a char 
    TRAP_PUTS = 0x22,   // print a word string
    TRAP_IN = 0x23,     // get char, echoed
    TRAP_PUTSP = 0x24,  // print a byte string
    TRAP_HALT = 0x25,   // halt the program
};

// memory mapped registers 
enum {
    MR_KBSR = 0xFE00,   // keyboard status
    MR_KBDR = 0xFE02    // keyboard data
};

// Unix specific code 
uint16_t check_key()
{
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(STDIN_FILENO, &readfds);

    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
    return select(1, &readfds, NULL, NULL, &timeout) != 0;
}


// Input Buffering
struct termios original_tio;

void disable_input_buffering()
{
    tcgetattr(STDIN_FILENO, &original_tio);
    struct termios new_tio = original_tio;
    new_tio.c_lflag &= ~ICANON & ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &new_tio);
}

void restore_input_buffering()
{
    tcsetattr(STDIN_FILENO, TCSANOW, &original_tio);
}

// Handle Interrupt
void handle_interrupt(int signal)
{
    restore_input_buffering();
    printf("\n");
    exit(-2);
}

void mem_write(uint16_t addr, uint16_t val) {
    memory[addr] = val;
}

uint16_t mem_read(uint16_t addr) {
    if (addr == MR_KBSR) {
        if (check_key()) {
            memory[MR_KBSR] = (1 << 15);
            memory[MR_KBDR] = getchar();
        } else {
            memory[MR_KBSR] = 0;
        }
    }

    return memory[addr];
}

// LC3 is big endian, we need to convert to little endian 
uint16_t swap16(uint16_t x) {
    return (x << 8) | (x >> 8);
}

// read from file 
void read_image_file(FILE* file) {
    // origin is the first address to check.
    // it tells us where our program should start.
    // hence it is hardcoded
    uint16_t origin;
    fread(&origin, sizeof(origin), 1, file);
    origin = swap16(origin);

    // we know the maximum file size (the memory array), we can be done
    // with a single fread.
    uint16_t max_read = UINT16_MAX - origin;
    uint16_t* p = memory + origin;
    size_t read = fread(p, sizeof(uint16_t), max_read, file);

    // swap to little endian 
    while (read-- > 0) {
        *p = swap16(*p);
        ++p;
    }
}

int read_image(const char* image_path) {
    FILE* file = fopen(image_path, "rb");
    if (!file) return 0;
    read_image_file(file);
    fclose(file);

    return 1;
}

// sign_extend for certain operations when operation occurs in immediate
// mode
uint16_t sign_extend(uint16_t x, int bit_count) {
    if (x >> (bit_count - 1) & 1) {
        // if the first bit is 1 
        // or the number is negative 

        x |= (0xFFFF << bit_count);

        // lets assume we want to extend 11111 (-1 in 5 bits) to 16 bits 
        // the operation which occurs is 11111 OR 1111111111110000
        // which gives us 1111111111111111, which is -1 in 16 bits
    }

    return x;
}

void update_flags (uint16_t r) {
    if (reg[r] == 0)
        reg[R_COND] = FL_ZRO;
    else if (reg[r] >> 15)
        reg[R_COND] = FL_NEG;
    else 
        reg[R_COND] = FL_POS;
}

int main(int argc, char* argv[]) {
    // load arguments
    if (argc < 2) {
        // show usage string 
        printf("lc3 [image-file1] ...\n");
        exit(2);
    }

    for (int j = 1; j < argc; ++j) {
        if (!read_image(argv[j])) {
            printf("failed to load image: %s\n", argv[j]);
            exit(1);
        }
    }

    // Setup
    signal(SIGINT, handle_interrupt);
    disable_input_buffering();
    
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
                {
                    // Destination register (DR)
                    // shifting 9 bits and then and-ing with 0b111 to
                    // get 3 bits
                    uint16_t r0 = (instr >> 9) & 0x7;

                    // first operand (SR1)
                    uint16_t r1 = (instr >> 6) & 0x7;

                    // Immediate mode flag
                    uint16_t imm_flag = (instr >> 5) & 0x1;

                    if (imm_flag) {
                        // extract the imm5 operand, and extend it to 16
                        // bits if it was negative.
                        uint16_t imm5 = sign_extend(instr & 0x1F, 5);

                        reg[r0] = reg[r1] + imm5;
                    } else {
                        // extract the right 3 bits
                        uint16_t r2 = instr & 0x7;
                        reg[r0] = reg[r1] + reg[r2];
                    }

                    update_flags(r0);
                }
                break;
            case OP_AND:
                {
                    // Destination register (DR)
                    // shifting 9 bits and then and-ing with 0b111 to
                    // get 3 bits
                    uint16_t r0 = (instr >> 9) & 0x7;

                    // first operand (SR1)
                    uint16_t r1 = (instr >> 6) & 0x7;

                    // Immediate mode flag
                    uint16_t imm_flag = (instr >> 5) & 0x1;

                    if (imm_flag) {
                        // extract the imm5 operand, and extend it to 16
                        // bits if it was negative.
                        uint16_t imm5 = sign_extend(instr & 0x1F, 5);

                        reg[r0] = reg[r1] & imm5;
                    } else {
                        // extract the right 3 bits
                        uint16_t r2 = instr & 0x7;
                        reg[r0] = reg[r1] & reg[r2];
                    }

                    update_flags(r0);
                }
                break;
            case OP_NOT:
                {
                    // destination register (dr)
                    uint16_t dr = (instr >> 9) & 0x7;
                    // source register (sr)
                    uint16_t sr = (instr >> 6) & 0x7;

                    reg[dr] = ~reg[sr];
                    update_flags(dr);
                }
                break;
            case OP_BR:
                {
                    uint16_t pc_offset = sign_extend(instr & 0x1FF, 9);
                    uint16_t cond_flag = (instr >> 9) & 0x7;

                    if (cond_flag & reg[R_COND])
                        reg[R_PC] += pc_offset;
                }
                break;
            case OP_JMP:
                { 
                    // also handle RET 
                    uint16_t jump_reg = (instr >> 6) & 0x7;
                    reg[R_PC] = reg[jump_reg];
                }
                break;
            case OP_JSR:
                {
                    reg[R_R7] = reg[R_PC];
                    
                    uint16_t indir_flag = (instr >> 11) & 0x1;
                    if (indir_flag) {
                        uint16_t pc_offset = instr & 0x7FF;
                        pc_offset = sign_extend(pc_offset, 11);

                        reg[R_PC] += pc_offset;
                    } else {
                        uint16_t baseR = (instr >> 6) & 0x7;
                        reg[R_PC] = reg[baseR];
                    }
                }
                break;
            case OP_LD:
                {
                    uint16_t dr = (instr >> 9) & 0x7;
                    uint16_t pc_offset = sign_extend(instr & 0xFF, 8);

                    reg[dr] = mem_read(reg[R_PC] + pc_offset);

                    update_flags(dr);
                }
                break;
            case OP_LDI:
                {
                    // destination register
                    uint16_t r0 = (instr >> 9) & 0x7;

                    // PCoffset9
                    uint16_t pc_offset = sign_extend(instr & 0x1FF, 9);

                    // Add pc_offset to current PC, look at that memory
                    // location to get the final address.
                    reg[r0] = mem_read(mem_read(reg[R_PC] + pc_offset));
                    update_flags(r0);
                }
                break;
            case OP_LDR:
                {
                    uint16_t dr = (instr >> 9) & 0x7;
                    uint16_t baseR = (instr >> 6) & 0x7;
                    uint16_t pc_offset = sign_extend(instr & 0x3F, 6);

                    reg[dr] = mem_read(reg[baseR] + pc_offset);

                    update_flags(dr);
                }
                break;
            case OP_LEA:
                {
                    uint16_t dr = (instr >> 9) & 0x7;
                    uint16_t pc_offset = sign_extend(instr & 0x1FF, 9);

                    reg[dr] = reg[R_PC] + pc_offset;
                    
                    update_flags(dr);
                }
                break;
            case OP_ST:
                {
                    uint16_t sr = (instr >> 9) & 0x7;
                    uint16_t pc_offset = sign_extend(instr & 0x1FF, 9);

                    mem_write(reg[R_PC] + pc_offset, reg[sr]);
                }
                break;
            case OP_STI:
                {
                    uint16_t sr = (instr >> 9) & 0x7;
                    uint16_t pc_offset = sign_extend(instr & 0x1FF, 9);

                    mem_write(mem_read(reg[R_PC] + pc_offset), reg[sr]);
                }
                break;
            case OP_STR:
                {
                    uint16_t sr = (instr >> 9) & 0x7;
                    uint16_t baseR = (instr >> 6) & 0x7;
                    uint16_t pc_offset = sign_extend(instr & 0x3F, 6);

                    mem_write(reg[baseR] + pc_offset, reg[sr]);
                }
                break;
            case OP_TRAP:
                switch (instr & 0xFF) {
                    case TRAP_GETC:
                        {
                            reg[R_R0] = (uint16_t)getchar();
                        }
                        break;
                    case TRAP_OUT:
                        {
                            putc((char)reg[R_R0], stdout);
                            fflush(stdout);
                        }
                        break;
                    case TRAP_PUTS:
                        {
                            uint16_t* c = memory + reg[R_R0];
                            while(*c) {
                                putc((char)*c, stdout);
                                ++c;
                            }

                            fflush(stdout);
                        }
                        break;
                    case TRAP_IN:
                        {
                            printf("Enter a character: ");
                            char c = getchar();
                            putc(c, stdout);

                            reg[R_R0] = (uint16_t)c;
                        }
                        break;
                    case TRAP_PUTSP:
                        {
                            // one char per byte (two bytes per word)
                            // here we need to swap back to the big
                            // endian format

                            // LC3 is 16 bit. the characters are 16 bit
                            // long, instead of the normal 8 bit (1 byte).
                            // 
                            // Therefore we have to chop off the extra
                            // bits

                            uint16_t* c = memory + reg[R_R0];
                            while (*c) {
                                char char1 = (*c) & 0xFF;
                                putc(char1, stdout);
                                char char2 = (*c) >> 8;
                                if (char2) 
                                    putc(char2, stdout);

                                ++c;
                            }

                            fflush(stdout);
                        }
                        break;
                    case TRAP_HALT:
                        {
                            puts("HALT");
                            fflush(stdout);

                            running = 0;
                        }
                        break;
                }
                break;
            case OP_RES:
            case OP_RTI:
            default:
                // bad opcode
                abort();
                break;
        }
    }

    // Shutdown
    restore_input_buffering();
}
