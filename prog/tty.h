/*                  
 * tty.h - Helper functions for acessing a serial line
 *   
 * Written 2006 by Werner Almesberger
 * Copyright 2006 Werner Almesberger
 */


#ifndef TTY_H
#define TTY_H

#include <stdint.h>
#include <termios.h>
#include <sys/types.h>


#define DEFAULT_TTY "/dev/ttyS0"


int tty_open_raw(const char *path,speed_t bps);
void tty_write(const void *data,size_t size);
void tty_read(void *data,size_t size,int timeout);
uint8_t tty_read_byte(int timeout);
void tty_close(void);

#endif /* !TTY_H */
