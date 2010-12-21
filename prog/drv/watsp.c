/*
 * watsp.c - Werner's trivial serial programmer
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
 * DTR  out     Active   Reset     reset
 * CTS  in      !SDATA   SDATA     data
 * RTS  out     !SDATA/Z SDATA     data
 * TD   out     !SCLK    SCLK      clock
 */

/*
 * Note: XRES and SDATA_IN are inverted by the circuit, hence the nots.
 */

#define XRES(on)	tty_dtr(!(on))
#define SDATA(on)	tty_rts(!(on))
#define SCLK(on)	tty_td(!(on))
#define	SDATA_IN()	(!tty_cts())
#define Z()		SDATA(0)


static int fd;
static int reset;


static int watsp_open(const char *dev,int voltage)
{
    if (voltage == 3)
	fprintf(stderr,
	  "warning: 3V operation is not reliable. Trying anyway ...\n");
    fd = tty_open_raw(dev,B19200);
    XRES(1);
    SCLK(0);
    Z();
    reset = 1;
    return voltage;
}


static void watsp_send_bit(int bit)
{
    if (reset) {
	XRES(0);
	reset = 0;
    }
    SDATA(bit);
    SCLK(1);
    SCLK(0);
}


static void watsp_send_z(void)
{
    Z();
    SCLK(1);
    SCLK(0);
}


static int watsp_read_bit(void)
{
    return SDATA_IN();
}


static void watsp_close(void)
{
    XRES(1);
    Z();
    SCLK(0);
    tty_close();
}


struct prog_ops watsp_ops = {
    .name = "watsp",
    .open = watsp_open,
    .send_bit = watsp_send_bit,
    .send_z = watsp_send_z,
    .read_bit = watsp_read_bit,
    .close = watsp_close,
};
