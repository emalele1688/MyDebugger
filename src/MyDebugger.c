#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/user.h>

#include "opcodesdiss.h"
#include "MyDebugger.h"


#define SECURE_SCALL(_syscall)				\
do{							\
  if(_syscall == -1)					\
  {							\
    fprintf(stderr, "%s\n", strerror(errno));		\
    destroy_debugger();					\
    exit(EXIT_FAILURE);					\
  }							\
}while(0)



struct MyDebugger;
struct breakpoint_data_restore;

struct breakpoint_data_restore
{
  uint64_t address_at;
  uint64_t orig_instruction;
};

struct MyDebugger
{
  struct breakpoint_data_restore *bdr;
  pid_t traced_id;
  int breakpoints_counter;
  char state_flags;
};


static int wait_process(void);
static void _delete_breakpoint(uint64_t address);
static int _next_instruction(void);
static int detect_breakpoint(uint64_t address);
static void restore_after_breakpoint(uint64_t address);
static void get_data(uint64_t address, void *wbuffer, size_t length);
static void set_data(uint64_t address, const void *rbuffer, size_t length);
static uint64_t get_register(unsigned int regaddr);
static void set_register(unsigned int regaddr, uint64_t value);


struct MyDebugger *mdbg = 0;
static int bdr_size = 8;

// breakpoint instruction
char trap_instruction[8] = 
{
  0xcc, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0
};


void init_debugger(void)
{
  if(mdbg != 0)
    return;
  
  mdbg = malloc(sizeof(struct MyDebugger));
  memset(mdbg, 0, sizeof(struct MyDebugger));
  
  mdbg->bdr = malloc(sizeof(struct breakpoint_data_restore) * bdr_size);
  memset(mdbg->bdr, 0, sizeof(struct breakpoint_data_restore) * bdr_size);
  
  init_x86_64_diss();
}

void destroy_debugger(void)
{
  if(mdbg->state_flags != DISABLED)
    kill_process();
  
  free(mdbg->bdr);
  free(mdbg);
  
  mdbg = 0;
}

void clean_debugger(void)
{
  if(mdbg->state_flags != DISABLED && mdbg->state_flags != FAULT)
        printf("Traced process is still running. You have to terminate it and then clean the debugger\n");
  else
  {
    mdbg->traced_id = 0;
    mdbg->state_flags = DISABLED;
    //All clean operation
  }
}

void run_process(const char *executablePath, char *const argv[])
{
  int c_pid;
  
  if(mdbg->state_flags != DISABLED)
  {
    printf("The process is already running\n");
    return;
  }
  
  SECURE_SCALL( (c_pid = fork()) );
  if(c_pid == 0)
  {
    /*** Child process ***/
    SECURE_SCALL( ptrace(PTRACE_TRACEME, 0, NULL, NULL) );
    SECURE_SCALL( execv(executablePath, argv) );
  }
  
  mdbg->traced_id = c_pid;
  mdbg->state_flags = RUNNING;
    
  if(wait_process() == 1)
    printf("Ok, the process is traced! pid: %d\n", mdbg->traced_id);
}

int continue_execution(void)
{
  uint64_t address;
  int ret;
  
  if(mdbg->state_flags == FAULT)
  {
    printf("Process received a segmentation fault\n");
    return 1;
  }
  
  if(mdbg->state_flags != INTERRUPTED)
  {
    printf("Process is not running\n");
    return 0;
  }
  
  mdbg->state_flags = RUNNING;
  SECURE_SCALL( ptrace(PTRACE_CONT, mdbg->traced_id, NULL, NULL) );
  
  ret = wait_process();
  //If traced process has been interrupted checks if a breakpoint has occurred
  if(ret == 1)
  {
    //The last instruction executed at one byte first, is it INT3 ?
    address = get_register(RIP);
    address -= 1;
    if(detect_breakpoint(address))
    {
      printf("Breakpoint at %lx\n", address);
      restore_after_breakpoint(address);
    }
  }
  
  return ret;
}

int trace_execution(void)
{
  int ret;
  
  if(mdbg->state_flags == FAULT)
  {
    printf("Process received a segmentation fault\n");
    return 1;
  }
  
  if(mdbg->state_flags != INTERRUPTED)
  {
    printf("Process is not running\n");
    return 0;
  }
  
  do
  {
    if(mdbg->state_flags == FAULT || _next_instruction())
      return 1;
  }
  while(( ret = wait_process()));
  
  return ret;
}

int next_instruction(void)
{
  if(mdbg->state_flags == FAULT)
  {
    printf("Process received a segmentation fault\n");
    return 1;
  }
  
  if(mdbg->state_flags != INTERRUPTED)
  {
    printf("Process is not running\n");
    return 0;
  }
  
  if(_next_instruction())
    return 1;
  
  return wait_process();
}

/* Execute the next instruction. If it is a break instruction don't execute it and return 1. */
int _next_instruction(void)
{
  uint64_t address;
  unsigned char instruction_traced[INSTRUCTION_MAX_SIZE];
  
  address = get_register(RIP);
  if(detect_breakpoint(address))
  {
    printf("Breakpoint at %lx\n", address);
    restore_after_breakpoint(address);
    return 1;
  }
  
  get_data(address, instruction_traced, INSTRUCTION_MAX_SIZE);
  
  printf("%lx: \t\t", address);
  print_instruction(instruction_traced);
  mdbg->state_flags = RUNNING;
  SECURE_SCALL( ptrace(PTRACE_SINGLESTEP, mdbg->traced_id, NULL, NULL) );
  
  return 0;
}

void kill_process(void)
{
  if(mdbg->state_flags == DISABLED)
    printf("The traced process is not running\n");
  else
  {
    printf("Killing the traced process\n");
    kill(mdbg->traced_id, SIGKILL);
    if(wait(0) == -1)
      printf("Traced process is in a zombie state because an error has occurred: %s\n", strerror(errno));
    mdbg->state_flags = DISABLED;
    clean_debugger();
  }
}

void set_breakpoint(uint64_t address)
{
  struct breakpoint_data_restore *curr;
  uint64_t instruction;
  
  if(mdbg->state_flags == FAULT)
  {
    printf("Process received a segmentation fault\n");
    return;
  }
  
  if(mdbg->state_flags == DISABLED)
  {
    printf("The traced process is not running\n");
    return;
  }
    
  if(mdbg->breakpoints_counter == bdr_size)
  {
    bdr_size *= 2;
    mdbg->bdr = realloc(mdbg->bdr, sizeof(struct breakpoint_data_restore) * bdr_size);
  }
  
  get_data(address, &instruction, X86_64_WORD_SIZE);
  
  curr = mdbg->bdr + mdbg->breakpoints_counter;
  curr->address_at = address;
  curr->orig_instruction = instruction;
  mdbg->breakpoints_counter++;
  
  set_data(address, trap_instruction, X86_64_WORD_SIZE);
}

void delete_breakpoint(uint64_t address)
{
  if(mdbg->state_flags == DISABLED)
  {
    printf("The traced process is not running\n");
    return;
  }
  
  _delete_breakpoint(address);
}

void _delete_breakpoint(uint64_t address)
{
  struct breakpoint_data_restore *curr, *last;
  int i;
    
  last = mdbg->bdr + (mdbg->breakpoints_counter - 1); 
  
  for(i = 0; i < mdbg->breakpoints_counter; i++)
  {
    curr = mdbg->bdr + i;
    if(curr->address_at == address)
    {
      set_data(curr->address_at, &curr->orig_instruction, X86_64_WORD_SIZE);
      /* Moves last block on the deleted block - Deletes last block and decrements the breakpoints_counter */
      curr->address_at = last->address_at;
      curr->orig_instruction = last->orig_instruction;
      last->address_at = 0;
      last->orig_instruction = 0;
      mdbg->breakpoints_counter--;
      
      return;
    }
  }
  
  printf("Breakpoint doesn't exist\n");
}

uint64_t get_register_value(unsigned int index)
{
  if(mdbg->state_flags == DISABLED)
  {
    printf("The traced process is not running\n");
    return 0;
  }
  
  return get_register(index);
}

static int detect_breakpoint(uint64_t address)
{
  uint64_t intr;
  
  get_data(address, &intr, X86_64_WORD_SIZE);
  if(intr == *((uint64_t*)trap_instruction))
    return 1;
  
  return 0;
}

void restore_after_breakpoint(uint64_t address)
{
  //Restore the RIP register after the breakpoint instruction
  _delete_breakpoint(address);
  set_register(RIP, address);
}

/* Wait for the traced process and control its exited status. 
 * Return 0 if the process terminates the execution, 1 if the process is stopped by a signal (usually a SIGTRAP) or a segmentation fault was occurred
 */
static int wait_process(void)
{
  int status = 0;
  
  if(mdbg->state_flags != RUNNING)
  {
    /* WARNING: This conditional statement should not never becomes true, otherwise abort the process */
    
    printf("Error code 001 (The process is not running).\n");
    abort();
  }
  
  while(wait(&status) == -1)
  {
    if(errno == EINTR)
      continue;
    else
    {
      /* WARNING: this conditional statement should not never becomes true, otherwise abort the process */
      
      fprintf(stderr, "%s\n", strerror(errno));
      if(errno != ECHILD)
	kill(SIGTERM, mdbg->traced_id);
      
      abort();
    }
  }
  
  //The traced process was terminated normally.
  if(WIFEXITED(status))
  {
    printf("The process is terminated with code %d\n", WEXITSTATUS(status));
    mdbg->state_flags = DISABLED;
    return 0;
  }
  
  //The traced process was terminated by a signal that it was not caught
  if(WIFSIGNALED(status))
  {
    printf("The process is terminated by signal %s\n", strsignal(WTERMSIG(status)));
    mdbg->state_flags = DISABLED;
    return 0;
  }
  
  //A segmentation fault has occurred in the traced process, so it is currently stopped
  if(WIFSTOPPED(status) && (WSTOPSIG(status) == SIGSEGV))
  {
    mdbg->state_flags = FAULT;
    printf("Segmentation fault :(\n");
    return 1;
  }

  mdbg->state_flags = INTERRUPTED;
  
  //if the process was interrupted by a signal then print the signal
  if(WIFSTOPPED(status) && (WSTOPSIG(status) != SIGTRAP))
    printf("The process receive a %s signal\n", strsignal(WSTOPSIG(status)));

  return 1;
}

static void get_data(uint64_t address, void *wbuffer, size_t length)
{
  size_t size_blocks = length / X86_64_WORD_SIZE;
  char *curr = (char*)wbuffer;
  uint64_t block;
  int c;
  
  for(c = 0; c < size_blocks; c++)
  {
    SECURE_SCALL( (block = ptrace(PTRACE_PEEKDATA, mdbg->traced_id, address + (c * X86_64_WORD_SIZE), NULL)) );
    memcpy(curr, &block, X86_64_WORD_SIZE);
    curr += X86_64_WORD_SIZE;
  }
  if((length % X86_64_WORD_SIZE) != 0)
  {
    SECURE_SCALL( (block = ptrace(PTRACE_PEEKDATA, mdbg->traced_id, address + (c * X86_64_WORD_SIZE), NULL)) );
    memcpy(curr, &block, X86_64_WORD_SIZE);
  }
}

static void set_data(uint64_t address, const void *rbuffer, size_t length)
{
  size_t size_blocks = length / X86_64_WORD_SIZE;
  const char *curr = (char*)rbuffer;
  uint64_t block;
  int c;
  
  for(c = 0; c < size_blocks; c++)
  {
    memcpy(&block, curr, X86_64_WORD_SIZE);
    SECURE_SCALL( (ptrace(PTRACE_POKEDATA, mdbg->traced_id, address + (c * X86_64_WORD_SIZE), block)) );
    curr += X86_64_WORD_SIZE;
  }
  if((length % X86_64_WORD_SIZE) != 0)
  {
    memcpy(&block, curr, X86_64_WORD_SIZE);
    SECURE_SCALL( (ptrace(PTRACE_POKEDATA, mdbg->traced_id, address + (c * X86_64_WORD_SIZE), block)) );
  }  
}

static uint64_t get_register(unsigned int regaddr)
{
  uint64_t value;
  
  SECURE_SCALL( (value = ptrace(PTRACE_PEEKUSER, mdbg->traced_id, regaddr * X86_64_WORD_SIZE, NULL)) );
  return value;
}

static void set_register(unsigned int regaddr, uint64_t value)
{
  SECURE_SCALL(( ptrace(PTRACE_POKEUSER, mdbg->traced_id, regaddr * X86_64_WORD_SIZE, value) ));
}


