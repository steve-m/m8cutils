/*                  
 * pp.c - Helper functions for acessing a parallel port
 *   
 * Written 2006 by Werner Almesberger
 * Copyright 2006 Werner Almesberger
 */


#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/poll.h>
#include <sys/ioctl.h>
#include <linux/ppdev.h>

#include "interact.h"

#include "pp.h"


static int fd = -1;
static int mode = 0; /* 0: undefined */
static int input = -1; /* 1: input; 0: output; -1: undefined */
static int have_epp = 0;


/* ----- Mode and direction changes ---------------------------------------- */


static void set_mode(int new_mode)
{
    if (mode == new_mode)
	return;
    mode = new_mode;
    if (ioctl(fd,PPNEGOT,&mode) < 0) {
	perror("ioctl(PPNEGOT)");
	exit(1);
    }
    input = -1;
}


static void choose_compat(void)
{
    set_mode(IEEE1284_MODE_COMPAT);
}


static void choose_epp(void)
{
    if (!have_epp) {
	fprintf(stderr,"internal error: EPP was not requested on pp_open\n");
	exit(1);
    }
    set_mode(IEEE1284_MODE_EPP);
}


static void set_input(int new_input)
{
    if (input == new_input)
	return;
    input = new_input;
    if (ioctl(fd,PPDATADIR,&input) < 0) {
	perror("ioctl(PPDATADIR)");
	exit(1);
    }
}


/* ----- Access functions -------------------------------------------------- */


int pp_open(const char *path,int need_epp)
{
    struct timeval timeout = { .tv_sec = PP_TIMEOUT_S, .tv_usec = 0 };

    if (!path)
	path = DEFAULT_PARPORT;
    have_epp = need_epp;
    fd = open(path,O_RDWR);
    if (fd < 0) {
	perror(path);
	exit(1);
    }
#if 0
    if (ioctl(fd,PPEXCL,0) < 0) {
	perror("ioctl(PPEXCL)");
	exit(1);
    }
#endif
    if (ioctl(fd,PPCLAIM,0) < 0) {
	perror("ioctl(PPCLAIM)");
	exit(1);
    }
    if (ioctl(fd,PPSETTIME,&timeout) < 0) {
	perror("ioctl(SETTIME)");
	exit(1);
    }   
    return fd;
}


void pp_write_data(uint8_t data)
{
    choose_compat();
    set_input(0);
    if (verbose > 2)
	fprintf(stderr,"PP WRITE: %02x\n",data);
    if (ioctl(fd,PPWDATA,&data) < 0) {
	perror("ioctl(PPWDATA)");
	exit(1);
    }
}


uint8_t pp_read_data(void)
{
    uint8_t data;

    choose_compat();
    set_input(1);
    if (ioctl(fd,PPRDATA,&data) < 0) {
	perror("ioctl(PPRDATA)");
	exit(1);
    }
    if (verbose > 2)
	fprintf(stderr,"PP READ: %02x\n",data);
    return data;
}


void pp_write_control(uint8_t mask,uint8_t data)
{
    struct ppdev_frob_struct frob = { .mask = mask, .val = data };

    choose_compat();
    if (verbose > 2)
	fprintf(stderr,"PP CONTROL:%s%s%s%s /%s%s%s%s\n",
	  data & PARPORT_CONTROL_STROBE ? " STROBE" : "",
	  data & PARPORT_CONTROL_AUTOFD ? " AUTOFD" : "",
	  data & PARPORT_CONTROL_INIT ? " INIT" : "",
	  data & PARPORT_CONTROL_SELECT ? " SELECT" : "",
	  mask & PARPORT_CONTROL_STROBE ? " STROBE" : "",
	  mask & PARPORT_CONTROL_AUTOFD ? " AUTOFD" : "",
	  mask & PARPORT_CONTROL_INIT ? " INIT" : "",
	  mask & PARPORT_CONTROL_SELECT ? " SELECT" : "");
    if (ioctl(fd,PPFCONTROL,&frob) < 0) {
	perror("ioctl(PPFCONTROL)");
	exit(1);
    }
}


uint8_t pp_read_status(void)
{
    uint8_t data;

    choose_compat();
    if (ioctl(fd,PPRSTATUS,&data) < 0) {
	perror("ioctl(PPRSTATUS)");
	exit(1);
    }
    if (verbose > 2)
	fprintf(stderr,"PP STATUS:%s%s%s%s%s\n",
	  data & PARPORT_STATUS_ERROR ? " ERROR" : "",
	  data & PARPORT_STATUS_SELECT ? " SELECT" : "",
	  data & PARPORT_STATUS_PAPEROUT ? " PAPEROUT" : "",
	  data & PARPORT_STATUS_ACK ? " ACK" : "",
	  data & PARPORT_STATUS_BUSY ? " BUSY" : "");
    return data;
}


void pp_write_epp(const uint8_t *data,size_t size)
{
    choose_epp();
    if (verbose > 2) {
	int i;

	fprintf(stderr,"PP WRITE EPP:");
	for (i = 0; i != size; i++)
	    fprintf(stderr," %02x",data[i]);
	fputc('\n',stderr);
    }
    while (size) {
	ssize_t wrote;

	wrote = write(fd,data,size);
	if (wrote < 0) {
	    perror("write");
	    exit(1);
	}
	if (!wrote) {
	    fprintf(stderr,"EPP wrote 0 bytes\n");
	    exit(1);
	}
	size -= wrote;
	data += wrote;
    }
}


void pp_read_epp(uint8_t *data,size_t size)
{
    uint8_t *p = data;
    size_t left = size;

    choose_epp();
    while (left) {
	ssize_t got;

	got = read(fd,p,left);
	if (got < 0) {
	    perror("read");
	    exit(1);
	}
	if (!got) {
	    fprintf(stderr,"EPP read 0 bytes\n");
	    exit(1);
	}
	left -= got;
	p += got;
    }
    if (verbose > 2) {
	int i;

	fprintf(stderr,"PP READ EPP:");
	for (i = 0; i != size; i++)
	    fprintf(stderr," %02x",data[i]);
	fputc('\n',stderr);
    }
}


int pp_poll_interrupt(void)
{
    struct pollfd pollfd = { .fd = fd, .events = POLLIN };
    int res,count;
    res = poll(&pollfd,1,0);
    if (res < 0) {
	perror("poll");
	exit(1);
    }
    if (!res)
	return 0;
    if (verbose > 2)
	fprintf(stderr,"PP INTERRUPT\n");
    if (ioctl(fd,PPCLRIRQ,&count) < 0) {
	perror("ioctl(PPCLRIRQ)");
	exit(1);
    }
    return 1;
}


void pp_close(void)
{
    if (ioctl(fd,PPRELEASE,0) < 0) {
	perror("ioctl(PPRELEASE)");
	exit(1);
    }
    if (close(fd) < 0) {
	perror("close");
	exit(1);
    }
}
