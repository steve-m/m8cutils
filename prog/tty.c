/*
 * tty.c - Helper functions for acessing a serial line
 *
 * Written 2006 by Werner Almesberger
 * Copyright 2006 Werner Almesberger
 */


#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/ioctl.h>

#include "interact.h"

#include "tty.h"


static struct termios termios;
static int fd;


static int modem_ioctl(int cmd,int arg)
{
    if (ioctl(fd,cmd,&arg) < 0) {
        perror("ioctl");
        exit(1);
    }
    return arg;
}


void tty_dtr(int on)
{
    modem_ioctl(on ? TIOCMBIS : TIOCMBIC,TIOCM_DTR);
}


void tty_rts(int on)
{
    modem_ioctl(on ? TIOCMBIS : TIOCMBIC,TIOCM_RTS);
}


void tty_td(int on)
{
    modem_ioctl(on ? TIOCSBRK : TIOCCBRK,0);
}


int tty_cts(void)
{
    return !!(modem_ioctl(TIOCMGET,0) & TIOCM_CTS);
}


int tty_dsr(void)
{
    return !!(modem_ioctl(TIOCMGET,0) & TIOCM_DSR);
}


int tty_dcd(void)
{
    return !!(modem_ioctl(TIOCMGET,0) & TIOCM_CD);
}


int tty_ri(void)
{
    return !!(modem_ioctl(TIOCMGET,0) & TIOCM_RI);
}


int tty_open_raw(const char *path,speed_t bps)
{
    struct termios t;
    long flags;

    if (!path)
	path = DEFAULT_TTY;
    fd = open(path,O_RDWR | O_NDELAY);
    if (fd < 0) {
	perror(path);
	exit(1);
    }
    if (tcgetattr(fd,&termios) < 0) {
	perror("tcgetattr");
	exit(1);
    }
    t = termios;
    cfmakeraw(&t);
    t.c_iflag &= ~(IXON | IXOFF);
    t.c_cflag |= CLOCAL;
    t.c_cflag &= ~CRTSCTS;
    if (cfsetispeed(&t,bps) < 0) {
	perror("cfsetispeed");
	exit(1);
    }
    if (cfsetospeed(&t,bps) < 0) {
	perror("cfsetospeed");
	exit(1);
    }
    if (tcsetattr(fd,TCSANOW,&t) < 0) {
	perror("tcsetattr");
	exit(1);
    }
    flags = fcntl(fd,F_GETFL);
    if (flags < 0) {
	perror("fcntl F_GETFL");
	exit(1);
    }
    if (fcntl(fd,F_SETFL,flags & ~O_NONBLOCK) < 0) {
	perror("fcntl F_GETFL");
	exit(1);
    }
    return fd;
}


void tty_write(const void *data,size_t size)
{
    if (verbose > 2) {
	int i;

	fprintf(stderr,"TTY WRITE:");
	for (i = 0; i != size; i++)
	    fprintf(stderr," %02x",((uint8_t *)data)[i]);
	putc('\n',stderr);
    }
    while (size) {
	ssize_t wrote;

	wrote = write(fd,data,size);
	if (wrote < 0) {
	    perror("write from serial port");
	    exit(1);
	}
	if (!wrote) {
	    perror("write 0 on serial port");
	    exit(1);
	}
	size -= wrote;
	data += wrote;
    }
}


void tty_read(void *data,size_t size,int timeout)
{
    const uint8_t *start = data;
    size_t left = size;

    while (left) {
	fd_set set;
	struct timeval to = { .tv_sec = timeout, .tv_usec = 0 };
	int res;
	ssize_t got;

	FD_ZERO(&set);
	FD_SET(fd,&set);
	res = select(fd+1,&set,NULL,NULL,&to);
	if (res < 0) {
	    perror("select");
	    exit(1);
	}
	if (!res) {
	    fprintf(stderr,"timeout reading from programmer\n");
	    exit(1);
	}
	got = read(fd,data,left);
	if (got < 0) {
	    perror("read from serial port");
	    exit(1);
	}
	if (!got) {
	    perror("EOF on serial port");
	    exit(1);
	}
	left -= got;
	data += got;
    }
    if (verbose > 2) {
	int i;

	fprintf(stderr,"TTY READ:");
	for (i = 0; i != size; i++)
	    fprintf(stderr," %02x",start[i]);
	putc('\n',stderr);
    }
}


uint8_t tty_read_byte(int timeout)
{
    uint8_t c;

   tty_read(&c,1,timeout);
   return c;
}


void tty_close(void)
{
    if (tcsetattr(fd,TCSANOW,&termios) < 0) {
	perror("tcsetattr");
	exit(1);
    }
    (void) close(fd);
}
