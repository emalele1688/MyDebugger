#include <asm/unistd.h>
#include <syscall.h>

.data

hello:
	.string "Hello, Linux!\n"
	len = .- hello

.text

.globl _start
_start:

	pushq	%rbp
	movq	%rsp, %rbp	
	movl	$1, %edi
	movl	$hello, %esi
	movl	$len, %edx
	movq	$SYS_write, %rax
	syscall
	
	movl	$0, %edi
	movq	$SYS_exit, %rax
	syscall