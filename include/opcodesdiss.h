#ifndef _OPCODESDISS_H
#define _OPCODESDISS_H


#define INSTRUCTION_MAX_SIZE	16


void init_x86_64_diss(void);

void print_instruction(unsigned char *instruction);


#endif