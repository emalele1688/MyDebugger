#include <stdio.h>


void foo1()
{
  int *P = 0x1;
  *P = 0x0;
}

void foo()
{
  printf("Hello, world!\n");
  foo1();
}

int main(int argn, char *args[])
{
  foo();
  return 0;
}
 
