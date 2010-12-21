#include "m8c.inc"

	area	vars(ram)
cntl:	blk	1
cnth:	blk	1

	area	text

	; make P0[1:0] outputs

	or	f,CPU_F_XIO
	or	reg[PRT0DM0],3
	and	reg[PRT0DM1],~3
	and	f,~CPU_F_XIO

ledloop:
	; make P0[0] blink at rate 2X, P0[1] at rate X
	mov	a,reg[PRT0DR]
	inc	a
	mov	reg[PRT0DR],a

	; wait for 256*(256*12+12) = 789504 CPUCLK cycles,
	; so the counter in DR[1:0] counts at 3.8 Hz if CPUCLK = 3 MHz

	mov	[cnth],0
	mov	[cntl],0
waitloop:			; 12 cycles
	dec	[cntl]
	jnz	waitloop
	dec	[cnth]		; 12 cycles
	jnz	waitloop

	jmp	ledloop
