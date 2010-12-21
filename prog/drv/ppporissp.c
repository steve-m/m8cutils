/*
 * ppporissp.c - Parallel Port based PowerOnReset In System Serial Programmer
 *
 * Copyright (c) 2009 Michael Buesch <mb@bu3sch.de>
 * Derived from wadsp.c  Copyright 2006 Werner Almesberger
 * Derived from watpp.c  Copyright 2006 Werner Almesberger
 */

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "prog.h"
#include "pp.h"

/*
 * Programmer connections:
 *
 *						Logical
 *						1	0
 * Data 0 (pin 2)	SDATA to device		SDATA	!SDATA
 * Data 1 (pin 3)	SCLK to device		SCLK	!SCLK
 * Data 2 (pin 4)	Device power		PWR	!PWR
 * Status 3 (pin 15)	SDATA from device	SDATA	!SDATA
 */

#define PWR_TARGET(on)		((on) ? (data |= (1 << 2)) : (data &= ~(1 << 2)))
#define SCLK(on)		((on) ? (data |= (1 << 1)) : (data &= ~(1 << 1)))
#define SDATA_OUT(on)		((on) ? (data |= (1 << 0)) : (data &= ~(1 << 0)))
#define SDATA_IN()		(!!(pp_read_status() & (1 << 3)))
#define COMMIT()		pp_write_data(data)

static int fd;
static int inverted;
static uint8_t data;


void delay_hook(int rising);
void __attribute__((weak)) delay_hook(int rising)
{
}

static void ppporissp_send_bit(int bit)
{
	SDATA_OUT(bit);
	SCLK(1);
	COMMIT();
	delay_hook(1);
	SCLK(0);
	COMMIT();
	delay_hook(0);
}

static void ppporissp_send_z(void)
{
	SDATA_OUT(0);
	SCLK(!inverted);
	COMMIT();
	delay_hook(1);
	SCLK(inverted);
	COMMIT();
	delay_hook(0);
}

static int ppporissp_read_bit(void)
{
	return SDATA_IN();
}

static void ppporissp_invert_phase(void)
{
	inverted = !inverted;
	if (inverted) {
		SCLK(1);
		COMMIT();
	}
}

static struct prog_bit ppporissp_bit = {
	.send_bit	= ppporissp_send_bit,
	.send_z		= ppporissp_send_z,
	.read_bit	= ppporissp_read_bit,
	.invert_phase	= ppporissp_invert_phase,
};

static void ppporissp_initialize(int power_on)
{
	struct timeval t0;

	printf("Performing power-on-reset...\n");
	PWR_TARGET(1);
	COMMIT();
	start_time(&t0);
	/* This is much safer than usleep. */
	while (delta_time_us(&t0) < T_VDDWAIT);
}

static int ppporissp_open(const char *dev, int voltage, int power_on,
			  const char *args[])
{
	prog_init(NULL, NULL, NULL, &ppporissp_bit);

	if (!power_on) {
		fprintf(stderr, "ppporissp does not support XRES\n");
		return -1;
	}
	if (voltage != 5)
		printf("Warning: Not operating on 5V. Trying anyway...\n");

	fd = pp_open(dev, 0);

	inverted = 0;
	PWR_TARGET(0);
	SCLK(0);
	SDATA_OUT(0);
	COMMIT();
	/*
	 * With the power off, we now wait for capacitors on the board to
	 * discharge. Using v(t) = v(0)*exp(-t/RC) and
	 * v(0) = 5V, R = 4.7kOhm, t = 100ms, we can discharge up to
	 * C = 10uF down to a diode drop of 0.6V.
	 */
	printf("Waiting for device to power down...\n");
	sleep(2); /* 2000 ms */

	return voltage;
}

static void ppporissp_close(void)
{
	SCLK(0);
	PWR_TARGET(0);
	COMMIT();
	pp_close();
}

struct prog_common ppporissp = {
	.name		= "ppporissp",
	.open		= ppporissp_open,
	.close		= ppporissp_close,
	.initialize	= ppporissp_initialize,
};
