;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; WASPIC - Werner's slow PIC-based programmer
;
; Written 2006 by Werner Almesberger
; Copyright 2006 Werner Almesberger
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;
; Protocol:
;
; From host		To host		Description
; 0x01 0x02 0x03	'+'		reset programmer
; 0x00					zero vector
; 0x#4 addr data			write to memory
; 0x#5 addr		data		read from memory
; 0x#6 addr data			write to register
; 0x#7 addr		data		read from register
; 0x#e addr data	'.'		write to register, triggers an SSC
;
; # is the XOR of all the other nibbles in the message.
;


	title	"WASPIC"

	list	P = 16F88

	include	"p16f88.inc"

config = _CP_OFF & _DEBUG_OFF & _WRT_PROTECT_256 & _CPD_OFF
config	= config & _LVP_OFF & _BODEN_OFF & _MCLR_OFF & _PWRTE_ON & _WDT_OFF
config	= config & _INTRC_IO

	__config	config

; PICP mis-programs the 16F88. Therefore, we pretend it's a 16F819.
; Unfortunately, the 16F819 doesn't have a CONFIG2 word, so we can't define it
; here.
;
;	__config        _CONFIG2,_IESO_OFF & _FCMEN_OFF

	radix	dec
	errorlevel -302		; yes, I DO make sure RP0 has the right value


	cblock	0x20
byte0				; messages are up to three bytes long
byte1
byte2
vec				; eight bits of vector to send
tail				; three bit to send as "tail" of writes
tmp
tmp2
	endc

#define XRES	PORTA,2		; RA2 is XRES
#define SDATA	PORTA,3		; RA3 is SDATA
#define SCLK	PORTA,4		; RA4 is SCLK
#define TRIGGER	PORTA,1		; RA1 is external trigger


in	macro
	bsf	STATUS,RP0	; make SDATA an input
	bsf	SDATA
	bcf	STATUS,RP0
	endm

out	macro
	bsf	STATUS,RP0	; make SDATA an output
	bcf	SDATA
	bcf	STATUS,RP0
	endm


	org	0

; Set internal osc to 4 MHz, so that Tcy = 1us

	bsf	STATUS,RP0
	movlw	(1 << IRCF2) | (1 << IRCF1)
	movwf	OSCCON
	; RP0 is set

; Make trigger (RA1), TX (RB5), RA2 (XRES), and RA4 (SCLK) outputs

	movlw	~((1 << 1) | (1 << 2) | (1 << 4))
	movwf	TRISA
	movlw	~(1 << 5)
	movwf	TRISB

; Get rid of the ADC

	clrf	ANSEL
	bcf	STATUS,RP0

; Set up the UART

	bcf	INTCON,GIE	; no interrupts

	bsf	STATUS,RP0
	movlw	12		; 19200 bps
	movwf	SPBRG
	movlw	(1 << TXEN) | (1 << BRGH) ; 19200 bps, enable transmit
	movwf	TXSTA
	bcf	STATUS,RP0
	movlw	(1 << SPEN) | (1 << CREN) ; enable serial port and receiver
	movwf	RCSTA

	movlw	'*'		; say something (good for cable testing etc.)
	movwf	TXREG

start:
	call	reset

	call	rx		; get a byte
	xorlw	1		; 1 ?
	btfss	STATUS,Z
	goto	start		; no -> wait some more

start1:
	call	rx		; get a byte
	xorlw	2		; 2 ?
	btfsc	STATUS,Z
	goto	start2
	xorlw	2^1		; 1 ?
	btfsc	STATUS,Z
	goto	start1		; yes -> short loop
	goto	start

start2:
	call	rx		; get a byte
	xorlw	3		; 3 ?
	btfsc	STATUS,Z
	goto	ready
	xorlw	3^1		; 1 ?
	btfsc	STATUS,Z
	goto	start1		; yes -> short loop
	goto	start

ready:
	movlw	'+'		; confirm that we're ready
	movwf	TXREG

rxloop:
	call	rx		; get the command byte
	movwf	byte0
	btfsc	STATUS,Z	; 0 -> send 22 zeroes
	goto	rxz22

	xorlw	1		; is this the beginning of a reset sequence ?
	btfsc	STATUS,Z
	goto	start1		; yes -> reset

	andlw	~3		; is it part of a reset sequence ?
	btfsc	STATUS,Z
	goto	start		; yes -> reset

rx2:
	btfsc	byte0,0		; is it a read ?
	goto	rx2r		; yes

rx2w:
	call	rx		; get address and data
	movwf	byte1
	call	rx
	movwf	byte2

	bcf	XRES		; drop XRES

	out			; make SDATA an output

	movf	byte0,W		; send the vector
	movwf	vec
	movlw	3
	call	send

	movf	byte1,W
	movwf	vec
	movlw	8
	call	send

	movf	byte2,W
	movwf	vec
	movlw	8
	call	send

	movf	tail,W		; send tail of vector
	movwf	vec
	movlw	3
	call	send

	movlw	7		; tail of writes is 111 after first vector
	movwf	tail

	in			; make SDATA an input

	btfss	byte0,3		; WAIT-AND-POLL ?
	goto	rxloop		; nope

	bsf	SCLK		; pulse Z
	bcf	SCLK

	movlw	4		; wait for SDATA to rise (20us)
	movwf	tmp
poll0:
	btfss	SDATA
	goto	polld
	decfsz	tmp,F
	goto	poll0

	; if we didn't see it rise within 20us, perhaps it has already dropped,
	; so we just proceed

polld:
	movlw	255		; 255*255 cycles (~ 325 ms)
	movwf	tmp
poll1:
	movlw	255
	movwf	tmp2
poll2:
	btfss	SDATA		; wait until L
	goto	polldone

	decfsz	tmp2,F
	goto	poll2
	decfsz	tmp,F
	goto	poll1

	movlw	'@'		; complain
	movwf	TXREG

	goto	start		; restart

polldone:
	movlw	'.'		; send an acknowledgement
	movwf	TXREG		; (transmitter is empty)

	out			; make SDATA an output
	movlw	40		; send 40 zero bits
	call	zeroes
	in			; make SDATA an input

	goto	rxloop		; next command

rxz22:
	out			; make SDATA an output
	movlw	22		; send 22 zero bits
	call	zeroes
	in			; make SDATA an input

	goto	rxloop		; next command

rx2r:
	call	rx		; get the address
	movwf	byte1

	out			; make SDATA an output

	movf	byte0,W		; send the vector
	movwf	vec
	movlw	3
	call	send

	movf	byte1,W
	movwf	vec
	movlw	8
	call	send

	in			; make SDATA an input

	bsf	SCLK		; clock Z
	bcf	SCLK

	call	receive		; receive one byte
	movwf	TXREG		; send it (transmitter is empty)

	bsf	SCLK		; clock Z
	bcf	SCLK

	out			; make SDATA an output

	bsf	SDATA		; send a one
	bsf	SCLK
	bcf	SCLK

	in			; make SDATA an input

	goto	rxloop		; loop again

; reset status: drive XRES high, SCLK low, make SDATA an input

reset:
	bcf	SDATA
	bcf	SCLK
	bsf	XRES
	bcf	TRIGGER
	bsf	STATUS,RP0
	bsf	SDATA
	bcf	STATUS,RP0
	clrf	tail
	return

; receive one byte from the serial port

rx:
	btfsc	RCSTA,OERR	; overrun
	goto	rxovr

	btfsc	RCSTA,FERR	; framing error
	goto	rxfrm

	btfss	PIR1,RCIF	; got data to receive
	goto	rx		; no -> keep on looping

	movf	RCREG,W		; get the received byte
	return			; done

rxfrm:
	movf	RCREG,W		; get the garbled byte
	goto	rx

rxovr:
	bcf	RCSTA,CREN	; reset the receiver
	bsf	RCSTA,CREN
	goto	rx

; send the W lowest bits from "vec"

send:
	movwf	tmp
sndalp:
	andlw	7		; first bit is left-aligned ?
	btfsc	STATUS,Z
	goto	sndali		; yes -> send them
	rlf	vec,F		; rotate by one bit
	addlw	1
	goto	sndalp

sndali:
	bcf	SDATA		; send one bit
	rlf	vec,F
	btfsc	STATUS,C
	bsf	SDATA
	bsf	SCLK
	bcf	SCLK
	decfsz	tmp,F		; next one
	goto	sndali

	return			; done

; send W zero bits

zeroes:
	bcf	SDATA
	movwf	tmp
zlp:
	bsf	SCLK
	bcf	SCLK
	decfsz	tmp,F
	goto	zlp

	return

; receive one byte into W

receive:
	movlw	8		; 8 bits
	movwf	tmp
recv1:
	bsf	SCLK		; one pulse to get the bit
	bcf	SCLK
	bcf	STATUS,C	; read the bit
	btfsc	SDATA
	bsf	STATUS,C
	rlf	vec,F		; shift it into "vec"
	decfsz	tmp,F		; next one
	goto	recv1

	movf	vec,W		; return result
	return

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	end
