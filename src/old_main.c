 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "MyDebugger.h"


MyDebugger *debugger;

void parseRunLine(const char *exString, char executablePath[], char *arguments[])
{
  int cont = 0;
  
  while(*exString == ' ')
    exString++;
  
  /* Get executable path name */
  while(*exString > 32 && *exString < 126)
  {
    executablePath[cont] = *exString;
    cont++;
    exString++;
  }
  executablePath[cont] = '\0';
  
  /* Get executable parameter */
}

void runMyProcess(const char *exString)
{
  char executablePath[256];
  char *arguments[253];
    
  memset(executablePath, 0, sizeof(executablePath));
  memset(arguments, 0, sizeof(arguments));
  
  parseRunLine(exString+1, executablePath, arguments);
  
  runProcess(debugger, executablePath, arguments);
}

void breakpoint(const char *command)
{
  uint64_t breakAddress;
  char saddr[8];
    
  command++;
  strncpy(saddr, command, 8);
  breakAddress = strtol(saddr, NULL, 16);
    
  setBreakpoint(debugger, breakAddress);
}

void printReg(void)
{
  printf("R15: %lx\n", getRegisterValue(debugger, 0));
  printf("R14: %lx\n", getRegisterValue(debugger, 1));
  printf("R13: %lx\n", getRegisterValue(debugger, 2));
  printf("R12: %lx\n", getRegisterValue(debugger, 3));
  printf("RBP: %lx\n", getRegisterValue(debugger, 4));
  printf("RBX: %lx\n", getRegisterValue(debugger, 5));
  printf("R11: %lx\n", getRegisterValue(debugger, 6));
  printf("R10: %lx\n", getRegisterValue(debugger, 7));
  printf("R9: %lx\n", getRegisterValue(debugger, 8));
  printf("R8: %lx\n", getRegisterValue(debugger, 9));
  printf("RAX: %lx\n", getRegisterValue(debugger, 10));
  printf("RCX: %lx\n", getRegisterValue(debugger, 11));
  printf("RDX: %lx\n", getRegisterValue(debugger, 12));
  printf("RSI: %lx\n", getRegisterValue(debugger, 13));
  printf("RDI: %lx\n", getRegisterValue(debugger, 14));
  printf("RIP: %lx\n", getRegisterValue(debugger, 16));
  printf("CS: %lx\n", getRegisterValue(debugger, 17));
  printf("EFLAGS: %lx\n", getRegisterValue(debugger, 18));
  printf("RSP: %lx\n", getRegisterValue(debugger, 19));
  printf("SS: %lx\n", getRegisterValue(debugger, 20));
  printf("FS_BASE: %lx\n", getRegisterValue(debugger, 21));
  printf("GS_BASE: %lx\n", getRegisterValue(debugger, 22));
  printf("DS: %lx\n", getRegisterValue(debugger, 23));
  printf("ES: %lx\n", getRegisterValue(debugger, 24));
  printf("FS: %lx\n", getRegisterValue(debugger, 25));
  printf("GS: %lx\n", getRegisterValue(debugger, 26)); 
}

void printHelp(void)
{
  printf("Enter the following commands:\n");
  printf(" r [executable] [arguments] => 	Start the process\n"
	 " b[address] =>			Setting a breakpoint\n"
	 " c => 				Continue the execution after start until a breakpoint is reached or the process terminate\n"
	 " t => 				Like c (continue) but it prints all instruction executed\n"
	 " n => 				Next instruction\n"
	 " p =>					Print all register value\n"
	 " h => 				Print this\n"
	 " q => 				Quit\n"
  );
}

int main(int argn, char *argv[])
{  
  char command[512];
  debugger = malloc(sizeof(MyDebugger));
  memset(command, 0, sizeof(command));
  initDebugger(debugger);
  
  printHelp();
  
  while(1)
  {
    printf("> ");
    fgets(command, sizeof(command), stdin);
    switch(command[0])
    {
      case 'h':
	printHelp();
	break;
	
      case 'q':
	exit(0);

      case 'r':
	runMyProcess(command);
	break;

      case 'b':
	breakpoint(command);
	break;
	
      case 'c':
	continueExecution(debugger);
	break;

      case 't':
	traceExecution(debugger);
	break;

      case 'n':
	nextInstruction(debugger);
	break;
	
      case 'p':
	printReg();
	break;

      default:
	printf("Command not defined\n");
	printHelp();
	break;
    }
    
    memset(command, 0, sizeof(command));
  }
  
  return 0;
}