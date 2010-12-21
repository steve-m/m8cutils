/*
 * watpp.c -  Werner's trivial parallel port programmer
 *
 * Written 2006 by Werner Almesberger
 * Copyright 2006 Werner Almesberger
 */

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "pp.h"
#include "prog.h"


/*
 * Programmer connections:
 *
 *
 * D0	Programmer power
 * D1	Target power
 * D2	XRES
 * D3	nSCLK H
 * D4	nSCLK L
 * D5	nSDATA H
 * D6	nSDATA L
 * ACK	nSDATA in
 */

#define PWR_PROG_BIT	1
#define	PWR_TARGET_BIT 	2
#define XRES_BIT	4
#define nSCLK_H_BIT	8
#define nSCLK_L_BIT	16
#define nSDATA_H_BIT	32
#define nSDATA_L_BIT	64

#define PWR_PROG(on)	(data = (data & ~1) | (on))
#define PWR_TARGET(on)	(data = (data & ~2) | ((on) << 1))
#define XRES(on)	(data = (data & ~4) | ((on)  << 2))
#define SCLK(on)	(data = (data & ~24) | (!(on) << 3) | (!(on) << 4))
#define SDATA(on)	(data = (data & ~96) | (!(on) << 5) | (!(on) << 6))
#define	COMMIT()	pp_write_data(data)
#define	SDATA_IN()	(!(pp_read_status() & PARPORT_STATUS_ACK))
#define Z()		(data = (data & ~nSDATA_L_BIT) | nSDATA_H_BIT)
#define SCLK_Z()	(data = (data & ~nSCLK_L_BIT) | nSCLK_H_BIT)


static uint8_t data;
static int fd;


static int watpp_open(const char *dev,int voltage,int power_on)
{
    if (power_on && voltage != 5)
	return -1;
    fd = pp_open(dev,0);
    PWR_PROG(1);
    PWR_TARGET(0);
    SCLK(0);
    Z();
    if (power_on) {
	XRES(0);
	/*
	 * With the power off, we now wait for capacitors on the board to
	 * discharge. Using v(t) = v(0)*exp(-t/RC) and
	 * v(0) = 5V, R = 4.7kOhm, t = 100ms, we can discharge up to
	 * C = 10uF down to a diode drop of 0.6V.
	 */
	usleep(100000); /* 100 ms */
    }
    else {
	XRES(1);
	usleep(T_XRES);
    }
    COMMIT();
    return voltage;
}


static void watpp_initialize(int power_on)
{
    if (power_on) {
	struct timeval t0;

	PWR_TARGET(1);
	COMMIT();
	start_time(&t0);
	/*
	 * This is much safer than usleep.
	 */
	while (delta_time_us(&t0) < T_VDDWAIT);
    }
    else {
	XRES(0);
	COMMIT();
    }
}


static void watpp_send_bit(int bit)
{
    SDATA(bit);
    SCLK(1);
    COMMIT();
    SCLK(0);
    COMMIT();
}


static void watpp_send_z(void)
{
    Z();
    SCLK(1);
    COMMIT();
    SCLK(0);
    COMMIT();
}


static int watpp_read_bit(void)
{
    return SDATA_IN();
}


static void watpp_close(void)
{
    XRES(1);
    Z();
    SCLK(0);
    COMMIT();
    PWR_TARGET(0);
    PWR_PROG(0);
    COMMIT();
    pp_close();
}


static void watpp_detach(void)
{
    XRES(1);
    Z();
    SCLK_Z();
    COMMIT();
    XRES(0);
    COMMIT();
    pp_close();
}


struct prog_ops watpp_ops = {
    .name = "watpp",
    .open = watpp_open,
    .initialize = watpp_initialize,
    .send_bit = watpp_send_bit,
    .send_z = watpp_send_z,
    .read_bit = watpp_read_bit,
    .close = watpp_close,
    .detach = watpp_detach,
};
