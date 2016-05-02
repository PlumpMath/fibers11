.syntax unified
.global __fibers_longjmp
.type __fibers_longjmp,%function
__fibers_longjmp:
	mov ip,r0
	movs r0,r1
	moveq r0,#1
	ldmia ip!, {v1,v2,v3,v4,v5,v6,sl,fp,sp,lr}

	tst r2,#0x260
	beq 3f
	tst r2,#0x20
	beq 2f
	ldc p2, cr4, [ip], #48
2:	tst r2,#0x40
	beq 2f
	.fpu vfp
	vldmia ip!, {d8-d15}
	.fpu softvfp
	.eabi_attribute 10, 0
	.eabi_attribute 27, 0
2:	tst r2,#0x200
	beq 3f
	ldcl p1, cr10, [ip], #8
	ldcl p1, cr11, [ip], #8
	ldcl p1, cr12, [ip], #8
	ldcl p1, cr13, [ip], #8
	ldcl p1, cr14, [ip], #8
	ldcl p1, cr15, [ip], #8

3:	bx lr
