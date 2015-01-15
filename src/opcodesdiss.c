#include <stdio.h>

#include "opcodesdiss.h"

#define PACKAGE "libgrive"
#include <dis-asm.h>


static struct disassemble_info xdiss;


void init_x86_64_diss(void)
{  
  init_disassemble_info(&xdiss, stdout, (fprintf_ftype)fprintf);
  xdiss.mach = bfd_mach_x86_64;
  xdiss.arch = bfd_arch_i386;
  xdiss.endian = BFD_ENDIAN_LITTLE;
  xdiss.buffer_length = INSTRUCTION_MAX_SIZE;
  
}

void print_instruction(unsigned char *instruction)
{
  xdiss.buffer = instruction;
  print_insn_i386(0, &xdiss);
  printf("\n");
}
