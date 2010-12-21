/*                  
 * pp.h - Helper functions for acessing a parallel port
 *   
 * Written 2006 by Werner Almesberger
 * Copyright 2006 Werner Almesberger
 */


#ifndef PP_H
#define PP_H

#include <stdint.h>
#include <sys/types.h>
#include <linux/parport.h>


#define DEFAULT_PARPORT	"/dev/parport0"
#define PP_TIMEOUT_S	2	/* a programmer that's silent for 2s is dead */


int pp_open(const char *path,int need_epp);
void pp_write_data(uint8_t data);
uint8_t pp_read_data(void);
void pp_write_control(uint8_t mask,uint8_t data); /* PARPORT_CONTROL_* */
uint8_t pp_read_status(void); /* PARPORT_STATUS_* */
void pp_write_epp(const uint8_t *data,size_t size);
void pp_read_epp(uint8_t *data,size_t size);
int pp_poll_interrupt(void);
void pp_close(void);

#endif /* !PP_H */
