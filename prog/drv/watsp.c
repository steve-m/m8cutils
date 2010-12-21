/*
 * watsp.c - Werner's trivial serial programmer
 *
 * Written 2006 by Werner Almesberger
 * Copyright 2006 Werner Almesberger
 */

/*
 * Very experimental and doesn't work yet. The problem seems to be in the
 * circuit (probably far too slow edges), not the control logic.
 */


#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <termios.h>

#include "vectors.h"
#include "cy8c2regs.h"
#include "tty.h"
#include "prog.h"


/*
 * Programmer connections:
 *
 *              Voltage
 *              +       -
 * DTR  out     Active  Reset           reset
 * CTS  in      !SDATA  SDATA (0V)      data
 * RTS  out     SDATA   !SDATA/Z        data
 * TD   out     SCLK    !SCLK           clock
 */

/*
 * Note: XRES and SDATA_IN are inverted by the circuit, hence the nots.
 */

#define XRES(on)	tty_dtr(!on)
#define SDATA(on)	tty_rts(on)
#define SCLK(on)	tty_td(on)
#define	SDATA_IN()	(!tty_cts())
#define Z()		SDATA(0)


static int fd;
static int reset;


static int watsp_open(const char *dev,int voltage)
{
    if (!voltage)
	return 0;
    fd = tty_open_raw(dev,B19200);
    
#if 0
tty_dtr(0);
tty_rts(0);
tty_td(0);
while (1) {
    tty_dtr(1);
fprintf(stderr,"DTR\n");
    sleep(4);
    tty_dtr(0);
    tty_rts(1);
fprintf(stderr,"RTS\n");
    sleep(4);
    tty_rts(0);
    tty_td(1);
fprintf(stderr,"TD\n");
    sleep(4);
    tty_td(0);
}
#endif
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
    SCLK(1);
    SCLK(0);
}


static void watsp_send_z(void)
{
    Z();
usleep(10);
    SCLK(1);
    SCLK(1);
    SCLK(0);
}


static int watsp_read_bit(void)
{
    usleep(20);
    return SDATA_IN();
}


static void watsp_close(void)
{
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
