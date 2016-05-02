/* Copyright 2011-2012 Nicholas J. Kain, licensed under standard MIT license */
.global __fibers_setjmp
.type __fibers_setjmp,@function
__fibers_setjmp:
	mov 4(%esp), %eax
	mov    %ebx, (%eax)
	mov    %esi, 4(%eax)
	mov    %edi, 8(%eax)
	mov    %ebp, 12(%eax)
	lea 4(%esp), %ecx
	mov    %ecx, 16(%eax)
	mov  (%esp), %ecx
	mov    %ecx, 20(%eax)
	xor    %eax, %eax
	ret
