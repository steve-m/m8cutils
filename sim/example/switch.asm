#include "m8c.inc"

	area	vars(ram)
stack:	blk	16
cntl:	blk	1
cnth:	blk	1
mask:	blk	1

	area	text

; reset vector

	org	0
	ljmp	start

; our interrupt handler is so tiny that it fits into the four bytes allocated
; in the interrupt table.

	org	0x1c
	xor	[mask],3	; 1 becomes 2, 2 becomes 1
	reti

; past the interrupt table

	org	0x68

start:
	; we will blink LED B first

	mov	[mask],2

	; make P0[2:0] outputs, P0[2] gets a pull-down on 0,
	; leave the rest at analog Z

	or	f,CPU_F_XIO
	mov	reg[PRT0DM0],0b00000011
	mov	reg[PRT0DM1],0b11111000
	and	f,~CPU_F_XIO
	mov	reg[PRT0DM2],0b11111000

	; enable interrupt-on-high for P0[2]

	or	f,CPU_F_XIO
	or	reg[PRT0IC1],4
	and	f,~CPU_F_XIO
	or	reg[PRT0IE],4

	; enable GPIO interrupts and then interrupts in general

	or	reg[INT_MSK0],INT_MSK0_GPIO
	or	f,CPU_F_GIE
	
ledloop:
	; invert with the mask
	mov	a,reg[PRT0DR]
	xor	a,[mask]
	and	a,3		; clear all other bits. In particular, NEVER
				; set PRT0DR[2] high, or it will "stick".
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
