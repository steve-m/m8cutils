/*
 * ecb_at91.c - AT91RM9200 Programmer, This programmer uses AT91' GPIO lines  
 *
 * Written 2006 by Carlos Camargo 
 * Based in Werner Almesberger's TrivialSerial Programmer
 */


#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <termios.h>


#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>

#include "ecb_at91.h"
#include "prog.h"

/*
 * Programmer connections:
 *
 * XRES		PC22
 * SCLK		PC20
 * SDATA	PC18
 */

static int reset, dummy;
static AT91PS_PIO pio;
static void *base;


/* ----- PIO --------------------------------------------------------------- */


static void pio_out (int mask, int val)
{
	if(mask == SDATA){
		pio->PIO_OER	= SDATA | SCLK | XRES;
	}

	if (val==0)
		pio->PIO_CODR = mask;
	else
		pio->PIO_SODR = mask;
}


static int pio_in(void)
{
	pio->PIO_ODR	= SDATA; 
	pio->PIO_IFER	= SDATA;
//	printf("%x\n",(pio->PIO_PDSR & SDATA));
	return (pio->PIO_PDSR & SDATA);
}

static void pio_setup(void)
{
	pio->PIO_PER	= SDATA | SCLK | XRES;	/* Enable PIO */
	pio->PIO_OER	= SDATA | SCLK | XRES;	/* Set SDATA, SCLK and XRES as outputs */
	pio->PIO_CODR	= SDATA | SCLK | XRES;	/* SDATA = SCLK = XRES = 0 */
	pio->PIO_IDR	= SDATA | SCLK | XRES;	/* Disable pin interrupts */
	pio->PIO_MDDR	= SDATA | SCLK | XRES;  /* Disable Multiple Diver*/
	pio->PIO_PPUDR	= SDATA | SCLK | XRES;  /* Disable Pull-Ups*/
	pio->PIO_OWDR	= SDATA | SCLK | XRES;	/* Synchronous Data Output Write in PIO_*/
}
	
static int pio_map(void)
{
	static int fd;

	off_t addr = 0xFFFFF800;	// PIO controller C

        if ((fd = open ("/dev/mem", O_RDWR | O_SYNC)) == -1)
        {
                printf ("Cannot open /dev/mem.\n");
                return 0;
        }
        printf ("/dev/mem opened.\n");

	base = mmap (0, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, addr & ~MAP_MASK);
        if (base == (void *) -1)
        {
                printf ("Cannot mmap.\n");
                return 0;
        }
        printf ("Memory mapped at address %p.\n", base);

	pio = base + (addr & MAP_MASK);
	return 1;
}


/* ----- Bit --------------------------------------------------------------- */


static void ecb_at91_send_bit(int bit)
{
	if (reset) {
		pio_out(XRES,0);
		reset = 0;
	}
	pio_out(SDATA,bit);
	pio_out(SCLK,1);
	pio_out(SCLK,0);
}


static void ecb_at91_send_z(void)
{
	dummy = pio_in();
	pio_out(SCLK,1);
	pio_out(SCLK,0);
}


static int ecb_at91_read_bit(void)
{
    return pio_in();
}


static struct prog_bit ecb_at91_bit = {
    .send_bit = ecb_at91_send_bit,
    .send_z = ecb_at91_send_z,
    .read_bit = ecb_at91_read_bit,
};


/* ----- Common ------------------------------------------------------------ */


static int ecb_at91_open(const char *dev,int voltage,int power_on,
  const char *args[])
{
	prog_init(NULL,NULL,NULL,&ecb_at91_bit);
	if (power_on)
		return -1;
	if (!pio_map ())
		return -1;
	pio_setup();

	pio_out(XRES,1);
	pio_out(SCLK,0);
	dummy = pio_in();
	reset = 1;
	return voltage;

}


static void ecb_at91_close(void)
{
	pio_out(XRES,1);
	pio_out(XRES,0);
	dummy = pio_in();
	pio_out(SCLK,0);

	munmap(base, MAP_SIZE);
	printf ("Memory unmapped at address %p.\n", base);
}


struct prog_common ecb_at91 = {
    .name = "ecb_at91",
    .open = ecb_at91_open,
    .close = ecb_at91_close,
};
