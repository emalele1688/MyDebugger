#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/user.h>
#include <sys/reg.h>

#define PACKAGE "libgrive"
#include <dis-asm.h>

#include "MyDebugger.h"


#define WORD_SIZE	8
#define INSTR_MAX_SIZE	16


#define SECURE_CALL(_syscall)				\
do{							\
  if(_syscall == -1)					\
  {							\
    fprintf(stderr, "%s\n", strerror(errno));		\
    if(debugger->traced_id != 0)			\
      kill(SIGKILL, debugger->traced_id);		\
    exit(EXIT_FAILURE);					\
  }							\
}while(0)


struct disassemble_info diss;

char trapInstruction[8] = 
{
  0xcc, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0
};


/* Wait for the traced process and control its exited status. 
 * Return 0 if the process terminate the execution, 1 if the process is stopped by a signal (usually a SIGTRAP)
 */
int waitMyProcess(MyDebugger *debugger)
{
  int status = 0;
  
  if(getState(debugger, RUNNING_STATE) == 0)
  {
    printf("Error code 001 (The process is not running).\n");
    abort();
  }
  
  while(wait(&status) == -1)
  {
    if(errno == EINTR)
      continue;
    else
    {
      fprintf(stderr, "%s\n", strerror(errno));
      if(errno != ECHILD)
	kill(SIGTERM, debugger->traced_id);
      
      exit(-1);
    }
  }
  
  if(WIFEXITED(status))
  {
    printf("The process is terminated with code %d\n", WEXITSTATUS(status));
    //running = 0;
    debugger->flags &= ~RUNNING_STATE;
    return 0;
  }
  
  if(WIFSIGNALED(status))
  {
    printf("The process is terminated by signal %d\n", WTERMSIG(status));
    //running = 0;
    debugger->flags &= ~RUNNING_STATE;
    return 0;
  }
  
  if(WIFSTOPPED(status) && (WSTOPSIG(status) == SIGSEGV))
  {
    printf("Segmentation fault\n");
    debugger->flags |= INT_SEGV;
    return 0;
  }
  
  if(WIFSTOPPED(status) && (WSTOPSIG(status) != SIGTRAP))
    printf("The process receive a %d signal number\n", WSTOPSIG(status));
    
  return 1;
}

void printInstruction(unsigned char *instruction, uint64_t addr)
{ 
  printf("%lx:\t", addr);
  diss.buffer = instruction;
  print_insn_i386(0, &diss);
  printf("\n");
}

/* Start the process to trace it */
void runProcess(MyDebugger *debugger, const char *executablePath, char *const argv[])
{
  if(getState(debugger, RUNNING_STATE))
  {
    printf("The process is already running\n");
    return;
  }
  
  SECURE_CALL( (debugger->traced_id = fork()) );
  if(debugger->traced_id == 0)
  {
    /*** Child process ***/
    SECURE_CALL( ptrace(PTRACE_TRACEME, 0, NULL, NULL) );
    SECURE_CALL( execv(executablePath, argv) );
  }
  
  //running = 1;
  debugger->flags |= RUNNING_STATE;
  
  if(waitMyProcess(debugger) == 0)
    printf("The process was terminated before trace it. It's a mistake!\n");
  else
    printf("Ok, the process is traced! pid: %d\n", debugger->traced_id);
}

int continueExecution(MyDebugger *debugger)
{
  uint64_t breakAddress;
  int ret;
  unsigned char instruction[INSTR_MAX_SIZE];
  
  if(getState(debugger, RUNNING_STATE) == 0)
  {
    printf("The process is not running\n");
    return 0;
  }
  
  memset(instruction, 0, INSTR_MAX_SIZE);

  SECURE_CALL( ptrace(PTRACE_CONT, debugger->traced_id, NULL, NULL) );
  
  ret = waitMyProcess(debugger);
  
  if(ret == 1) /* If the process still running and it reached a breakpoint change the trap instruction with the original instruction */
  {
    SECURE_CALL( (breakAddress = ptrace(PTRACE_PEEKUSER, debugger->traced_id, WORD_SIZE * RIP, NULL)) );
    breakAddress -= 1;
    getData(debugger, breakAddress, instruction, INSTR_MAX_SIZE);
    if(*((uint64_t*)instruction) == *((uint64_t*)trapInstruction))
    {
      printf("break at: %lx\n", breakAddress);
      SECURE_CALL( ptrace(PTRACE_POKETEXT, debugger->traced_id, breakAddress, debugger->sv_instruction) );
      SECURE_CALL( ptrace(PTRACE_POKEUSER, debugger->traced_id, WORD_SIZE * RIP, breakAddress) );
      return 1;
    }
  }
  else if(getState(debugger, INT_SEGV)) /* if a SIGSEGV signal is received */
  {
    SECURE_CALL( (breakAddress = ptrace(PTRACE_PEEKUSER, debugger->traced_id, WORD_SIZE * RIP, NULL)) );
    getData(debugger, breakAddress, instruction, INSTR_MAX_SIZE);
    printf("Segmentation fault at: ");
    printInstruction(instruction, breakAddress);
  }
  
  return ret;
}

int traceExecution(MyDebugger *debugger)
{
  uint64_t instruction_pointer = 0;
  int ret;
  unsigned char instruction_traced[INSTR_MAX_SIZE];
  
  if(getState(debugger, RUNNING_STATE) == 0)
  {
    printf("The process is not running\n");
    return 0;
  }
  
  memset(instruction_traced, 0, INSTR_MAX_SIZE);

  do
  {
    if(*((uint64_t*)instruction_traced) == *((uint64_t*)trapInstruction))
    {
      printf("break at: %lx\n", instruction_pointer);
      SECURE_CALL( ptrace(PTRACE_POKETEXT, debugger->traced_id, instruction_pointer, debugger->sv_instruction) );
      SECURE_CALL( ptrace(PTRACE_POKEUSER, debugger->traced_id, WORD_SIZE * RIP, instruction_pointer) );
      return 1;
    }
    
    SECURE_CALL( (instruction_pointer = ptrace(PTRACE_PEEKUSER, debugger->traced_id, WORD_SIZE * RIP, NULL)) );

    getData(debugger, instruction_pointer, instruction_traced, INSTR_MAX_SIZE);
    
    printInstruction(instruction_traced, instruction_pointer);

    SECURE_CALL( ptrace(PTRACE_SINGLESTEP, debugger->traced_id, NULL, NULL) );
  }
  while((ret = waitMyProcess(debugger)));
  
  return ret;
}

int nextInstruction(MyDebugger *debugger)
{
  uint64_t rip;
  unsigned char instruction[INSTR_MAX_SIZE];
  
  if(getState(debugger, RUNNING_STATE) == 0)
  {
    printf("The process is not running\n");
    return 0;
  }

  SECURE_CALL( (rip = ptrace(PTRACE_PEEKUSER, debugger->traced_id, WORD_SIZE * RIP, NULL)) );
  getData(debugger, rip, instruction, INSTR_MAX_SIZE);
  
  if(*((uint64_t*)instruction) == *((uint64_t*)trapInstruction))
  {
    SECURE_CALL( ptrace(PTRACE_POKETEXT, debugger->traced_id, rip, debugger->sv_instruction) );
    *((uint64_t*)instruction) = debugger->sv_instruction;
  }
  
  printInstruction(instruction, rip);
  
  SECURE_CALL( ptrace(PTRACE_SINGLESTEP, debugger->traced_id, NULL, NULL) );
  
  return waitMyProcess(debugger);
}

void setBreakpoint(MyDebugger *debugger, uint64_t address)
{
  if(getState(debugger, RUNNING_STATE) == 0)
  {
    printf("The process is not running\n");
    return;
  }
  
  getData(debugger, address, &debugger->sv_instruction, sizeof(trapInstruction));
  debugger->sv_instruction_addr = address;
  setData(debugger, address, &trapInstruction, sizeof(trapInstruction));  
  
  printf("Breakpoint set at address %lx\n", address);
}

uint64_t getRegisterValue(MyDebugger* debugger, unsigned int regNum)
{
  uint64_t regVal;
  
  if(getState(debugger, RUNNING_STATE) == 0)
  {
    printf("The process is not running\n");
    return 0;
  }
  
  regVal = ptrace(PTRACE_PEEKUSER, debugger->traced_id, regNum * WORD_SIZE, NULL);
  
  return regVal;
}

void getData(MyDebugger *debugger, uint64_t address, void *wbuffer, size_t length)
{
  size_t size_blocks = length / WORD_SIZE;
  char *curr = (char*)wbuffer;
  uint64_t block;
  int c;
  
  for(c = 0; c < size_blocks; c++)
  {
    SECURE_CALL( (block = ptrace(PTRACE_PEEKDATA, debugger->traced_id, address + (c * WORD_SIZE), NULL)) );
    memcpy(curr, &block, WORD_SIZE);
    curr += WORD_SIZE;
  }
  if((length % WORD_SIZE) != 0)
  {
    SECURE_CALL( (block = ptrace(PTRACE_PEEKDATA, debugger->traced_id, address + (c * WORD_SIZE), NULL)) );
    memcpy(curr, &block, WORD_SIZE);
  }  
}

void setData(MyDebugger *debugger, uint64_t address, const void *rbuffer, size_t length)
{
  size_t size_blocks = length / WORD_SIZE;
  const char *curr = (char*)rbuffer;
  uint64_t block;
  int c;
  
  for(c = 0; c < size_blocks; c++)
  {
    memcpy(&block, curr, WORD_SIZE);
    SECURE_CALL( (ptrace(PTRACE_POKEDATA, debugger->traced_id, address + (c * WORD_SIZE), block)) );
    curr += WORD_SIZE;
  }
  if((length % WORD_SIZE) != 0)
  {
    memcpy(&block, curr, WORD_SIZE);
    SECURE_CALL( (ptrace(PTRACE_POKEDATA, debugger->traced_id, address + (c * WORD_SIZE), block)) );
  }  
}

void initDebugger(MyDebugger *debugger)
{
  memset(debugger, 0, sizeof(MyDebugger));
  
  init_disassemble_info(&diss, stdout, (fprintf_ftype)fprintf);
  diss.mach = bfd_mach_x86_64;
  diss.arch = bfd_arch_i386;
  diss.endian = BFD_ENDIAN_LITTLE;
  diss.buffer_length = INSTR_MAX_SIZE;
}

int getState(MyDebugger *debugger, char state)
{
  if((debugger->flags & state) > 0)
    return 1;
  else
    return 0;
}
