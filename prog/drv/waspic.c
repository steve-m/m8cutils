/*
 * prog_waspic.c - Werner's slow PIC-based programmer
 *
 * Written 2006 by Werner Almesberger
 * Copyright 2006 Werner Almesberger
 */


#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <termios.h>

#include "vectors.h"
#include "tty.h"
#include "prog.h"


#define	READ_TIMEOUT	5	/* allow for enough time to let buffer drain */


static int fd;


/*
 * The greeting/reset message is 1 2 3. We add two more ones to get out of any
 * multibyte read.
 */

#define HELLO_STRING "\x01\x01\x01\x02\x03"
#define BYE_STRING "\x01\x01\x01"



/* ----- Vector ------------------------------------------------------------ */


static uint8_t waspic_vector(uint32_t v)
{
    uint8_t t[3];
    uint8_t c;

    t[0] = (v >> 16) & 7;
    t[1] = v >> 8;
    t[2] = v;
    if (!v) {
	tty_write(t,1);
	return 0;
    }
    if (IS_SSC(v))
	t[0] |= 8;
    c = t[0] ^
      ((t[1] >> 4) & 0xf) ^ (t[1] & 0xf) ^
      ((t[2] >> 4) & 0xf) ^ (t[2] & 0xf);
    t[0] |= c << 4;
    if (IS_WRITE(v)) {
	tty_write(t,3);
	if (IS_SSC(v)) {
	    c = tty_read_byte(READ_TIMEOUT);
	    if (c == '@') {
		fprintf(stderr,
		  "programmer timed out in WAIT-AND-POLL\n");
		exit(1);
	    }
	    if (c != '.') {
		fprintf(stderr,
		  "programmer returned 0x%02x instead of 0x%02x\n",c,'.');
		exit(1);
	    }
	}
	return 0;
    }
    else {
	tty_write(t,2);
	return tty_read_byte(READ_TIMEOUT);
    }
}


static struct prog_vector waspic_vec = {
    .acquire_reset = waspic_vector,
    .vector = waspic_vector,
};


/* ----- Common ------------------------------------------------------------ */


static int waspic_open(const char *dev,int voltage,int power_on,
  const char *args[])
{
    prog_init(NULL,NULL,&waspic_vec,NULL);
    if (power_on)
	return -1;
    fd = tty_open_raw(dev,B19200);
    tty_write(HELLO_STRING,5);
    while (tty_read_byte(READ_TIMEOUT) != '+');
    return voltage;
}


static void waspic_close(void)
{
    tty_write(BYE_STRING,3);
    tty_close();
}


struct prog_common waspic = {
    .name = "waspic",
    .open = waspic_open,
    .close = waspic_close,
};
