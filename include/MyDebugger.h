#ifndef _MYDEBUGGER_H
#define _MYDEBUGGER_H


#include <unistd.h>
#include <stdint.h>

#include <sys/reg.h>


#define X86_64_WORD_SIZE	sizeof(long int)


enum traced_process_state
{
  DISABLED =		0,
  RUNNING = 		1,
  INTERRUPTED = 	2,
  FAULT	=		3
};


void init_debugger(void);

void destroy_debugger(void);

void clean_debugger(void);

void run_process(const char *executablePath, char *const argv[]);

/* Continue the process execution. If the process is terminated the function returns 0, otherwise if the process is stopped, function returns 1 */
int continue_execution(void);

int trace_execution(void);

int next_instruction(void);

void kill_process(void);

void set_breakpoint(uint64_t address);

void delete_breakpoint(uint64_t address);

uint64_t get_register_value(unsigned int index);


#endif