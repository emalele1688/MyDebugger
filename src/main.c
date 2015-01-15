#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <readline/readline.h>
#include <readline/history.h>

#include "MyDebugger.h"


typedef struct 
{
  char *commandName;
  void (*action)(char *);
  const char *description;
  char sort_comm;
} command_type;


void _run(char *), 
  _kill(char *),
  _break(char *),
  _delb(char *),
  _continue(char *),
  _flow(char *),
  _next(char *),
  _backtrace(char *),
  _printgr(char *),
  _help(char *),
  _quit(char *);


command_type commands[] =
{
  { "run", 		_run, 		"run [process] [argument] ......start to tracing the process",'r' },
  { "kill", 		_kill, 		"kill ..........................kill the traced process",'k' },
  { "break",		_break, 	"break [address] ...............set a breakpoint", 'b' },
  { "delb", 		_delb, 		"delb [address] ................delete a breakpoint", 'd' },
  { "continue", 	_continue, 	"continue ......................continue the exectution after breakpoint or the process started", 'c' },
  { "flow", 		_flow, 		"flow ..........................execute the process printing all instruction executed, until the first breakpoint (INT 3 instruction)", 'f' },
  { "next", 		_next, 		"next ..........................execute the next instruction", 'n' },
  { "backtrace", 	_backtrace,	"backtrace .....................print the stack call trace", 's' },
  { "printgr",		_printgr, 	"printgr .......................print all general purpose register value", 'p' },
  { "help",		_help, 		"help ..........................print all commands", 'h' },
  { "quit",		_quit, 		"quit ..........................quit from the MyDebugger", 0 }
};


static const int commands_size = sizeof(commands) / sizeof(command_type);
static int execute = 1;


char *next_string(char *src)
{
  int i = 0;
  
  while(src[i] != ' ')
  {
    if(src[i] == '\0' || src[i] == 0)
      return 0;
    i++;
  }
  
  while(src[i] == ' ')
    i++;
  
  if(src[i] == '\0' || src[i] == 0)
    return 0;
  
  return src + i;
}

void close_whitespace(char *src)
{
  int i = 0;
  
  while(src[i] != ' ' && src[i] != '\0' && src[i] != 0)
    i++;

  src[i] = '\0';
}

void (*find_command(const char *strcomm))(char *)
{
  int i;
  char command[256];
  
  memset(command, 0, 256);
  
  // copy strcomm in command until the first white space
  for(i = 0; i < 256; i++)
  {
    if(strcomm[i] == ' ' || strcomm[i] == '\0' || strcomm[i] == 0)
      break;
    command[i] = strcomm[i];
  }
  
  if(strlen(command) == 1)
  {
    for(i = 0; i < commands_size; i++)
      if(commands[i].sort_comm == command[0])
	return commands[i].action;
  }
  else
  {
    for(i = 0; i < commands_size; i++)
      if(strcmp(commands[i].commandName, command) == 0)
	return commands[i].action;
  }
  
  return 0;
}

void _run(char *str_comm)
{
  char *str, *executable_path;
  char **arguments;
  int i = 0, size = 0;
    
  //Get executable path string pointer from str_comm string
  if((str = next_string(str_comm)) == 0)
  {
    printf("Enter a valid executable path\n");
    return;
  }
  
  executable_path = str;
  //Get arguments string poiter
  str = next_string(str);
  close_whitespace(executable_path);
  
  if(str != 0)
  {
    i = 0;
    size = 8;
    arguments = calloc(size, sizeof(char*));
    do
    {
      if(i == (size-1))
      {
	arguments = realloc(arguments, size * sizeof(char*) * 2);
	size *= 2;
      }
      arguments[i] = str;
      str = next_string(str);
      close_whitespace(arguments[i]);
      i++;
    }
    while(str != 0);
    arguments[i] = "\0";
  }
  else
    arguments = 0;
      
  run_process(executable_path, arguments);
  
  free(arguments);
}

void _kill(char *str_comm)
{
  kill_process();
}

void _break(char *str_comm)
{
  char *_braddr;
  uint64_t addr;
  
  if( (_braddr = next_string(str_comm)) == 0)
  {
    printf("Enter a breakpoint address\n");
    return;
  }
  
  //close_whitespace(_braddr);
  addr = strtol(_braddr, NULL, 16);
  
  set_breakpoint(addr);
}

void _delb(char *str_comm)
{
  char *_braddr;
  uint64_t addr;
  
  if( (_braddr = next_string(str_comm)) == 0)
  {
    printf("Enter a breakpoint address\n");
    return;
  }
  
  //close_whitespace(_braddr);
  addr = strtol(_braddr, NULL, 16);
  
  delete_breakpoint(addr);
}

void _continue(char *str_comm)
{
  if(continue_execution() == 0)
    clean_debugger();
}

void _flow(char *str_comm)
{
  if(trace_execution() == 0)
    clean_debugger();
}

void _next(char *str_comm)
{
  if(next_instruction() == 0)
    clean_debugger();
}

void _backtrace(char *str_comm)
{
  //TODO
}

void _printgr(char *str_comm)
{
  printf("R15: %lx\n", get_register_value(R15));
  printf("R14: %lx\n", get_register_value(R14));
  printf("R13: %lx\n", get_register_value(R13));
  printf("R12: %lx\n", get_register_value(R12));
  printf("R11: %lx\n", get_register_value(R11));
  printf("R10: %lx\n", get_register_value(R10));
  printf("R9: %lx\n", get_register_value(R9));
  printf("R8: %lx\n", get_register_value(R8));
  printf("RBP: %lx\n", get_register_value(RBP));
  printf("RSP: %lx\n", get_register_value(RSP));
  printf("RSI: %lx\n", get_register_value(RSI));
  printf("RDI: %lx\n", get_register_value(RDI));
  printf("RDX: %lx\n", get_register_value(RDX));
  printf("RCX: %lx\n", get_register_value(RCX));
  printf("RBX: %lx\n", get_register_value(RBX));
  printf("RAX: %lx\n", get_register_value(RAX));
  printf("RIP: %lx\n", get_register_value(RIP));
  printf("CS: %lx\n", get_register_value(CS));
  printf("SS: %lx\n", get_register_value(SS));
  printf("EFLAGS: %lx\n", get_register_value(EFLAGS));
  printf("FS_BASE: %lx\n", get_register_value(FS_BASE));
  printf("GS_BASE: %lx\n", get_register_value(GS_BASE));
  printf("DS: %lx\n", get_register_value(DS));
  printf("ES: %lx\n", get_register_value(ES));
  printf("FS: %lx\n", get_register_value(FS));
  printf("GS: %lx\n", get_register_value(GS)); 
}

void _help(char *str_comm)
{
  int i;
  for(i = 0; i < commands_size; i++)
    printf("%s\n", commands[i].description);
}

void _quit(char *str_comm)
{
  execute = 0;
}

void mainLoop(void)
{
  char *command_buffer;
  void (*action)(char *);
  
  while(execute)
  {
    command_buffer = readline("\n> ");
    if( (action = find_command(command_buffer)) != 0)
    {
      add_history(command_buffer);
      action(command_buffer);
    }
    else
      printf("Enter a valid command. Digit 'help' (or h) for help\n");
  }
}

int main(int argc, char *argv[])
{
  init_debugger();
  
  printf("\n");
  _help(0);
  
  mainLoop();
  
  destroy_debugger();
  
  printf("Bye :)\n");
  
  return 0;
}
