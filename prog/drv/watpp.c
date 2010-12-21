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


static uint8_t data;
static int fd;
static int reset;


static int watpp_open(const char *dev,int voltage)
{
    fd = pp_open(dev,0);
    PWR_PROG(1);
    PWR_TARGET(1);
    XRES(1);
    SCLK(0);
    Z();
    COMMIT();
usleep(10000);
#if 0
while (1) {
   SCLK(0);
   SDATA(1);
   COMMIT();
    sleep(1);
   SCLK(1);
   SDATA(0);
   COMMIT();
    sleep(1);
}
//sleep(100);
#endif
    reset = 1;
    return voltage;
}


static void watpp_send_bit(int bit)
{
    if (reset) {
	XRES(0);
	COMMIT();
	reset = 0;
    }
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


struct prog_ops watpp_ops = {
    .name = "watpp",
    .open = watpp_open,
    .send_bit = watpp_send_bit,
    .send_z = watpp_send_z,
    .read_bit = watpp_read_bit,
    .close = watpp_close,
};