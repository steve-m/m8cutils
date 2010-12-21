/*
 * wadsp.c - Werner's discrete serial programmer
 *
 * Written 2006 by Werner Almesberger
 * Copyright 2006 Werner Almesberger
 */


#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <termios.h>

#include "tty.h"
#include "prog.h"


/*
 * Programmer connections:
 *
 *              Voltage
 *              +        -
 * RTS  out     Reset    Active	   reset
 * DCD  in      !SDATA   SDATA     data
 * TD   out     SDATA    !SDATA/Z  data
 * DTR  out     SCLK     !SCLK     clock
 */


static int fd;
static int reset,inverted,sdata;


/*
 * Note: XRES and SDATA_IN are inverted by the circuit, hence the nots.
 */

#define XRES(on)	tty_rts(on)
#define SCLK(on)	tty_dtr(on)
#define	SDATA_IN()	(!tty_dcd())
#define Z()		SDATA(0)


static inline void SDATA(int on)
{
    if (sdata != on) {
	tty_td(on);
	sdata = on;
    }
}


/* ----- Bit --------------------------------------------------------------- */


static void wadsp_send_bit(int bit)
{
    if (reset) {
	XRES(0);
	reset = 0;
    }
    SDATA(bit);
    SCLK(1);
    SCLK(0);
}


static void wadsp_send_z(void)
{
    Z();
    SCLK(!inverted);
    SCLK(inverted);
}


static int wadsp_read_bit(void)
{
    return SDATA_IN();
}


static void wadsp_invert_phase(void)
{
    inverted = !inverted;
    if (inverted)
	SCLK(1);
}


static struct prog_bit wadsp_bit = {
    .send_bit = wadsp_send_bit,
    .send_z = wadsp_send_z,
    .read_bit = wadsp_read_bit,
    .invert_phase = wadsp_invert_phase,
};


/* ----- Common ------------------------------------------------------------ */


static int wadsp_open(const char *dev,int voltage,int power_on,
  const char *args[])
{
    prog_init(NULL,NULL,NULL,&wadsp_bit);
    if (power_on)
	return -1;
    if (voltage == 3)
	fprintf(stderr,
	  "warning: 3V operation is not reliable. Trying anyway ...\n");
    fd = tty_open_raw(dev,B19200);
    XRES(1);
    SCLK(0);
    Z();
    reset = 1;
    inverted = 0;
    sdata = -1;
    return voltage;
}


static void wadsp_close(void)
{
    XRES(1);
    Z();
    SCLK(0);
    tty_close();
}


struct prog_common wadsp = {
    .name = "wadsp",
    .open = wadsp_open,
    .close = wadsp_close,
};
