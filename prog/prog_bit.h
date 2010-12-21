/*
 * prog_bit.h - Bit-banging operations
 *
 * Written 2006 by Werner Almesberger
 * Copyright 2006 Werner Almesberger
 */


#ifndef PROG_BIT_H
#define PROG_BIT_H


struct prog_bit {
    void (*send_bit)(int bit);
    void (*send_z)(void);
    int (*read_bit)(void);
    void (*invert_phase)(void);  /* optional */
};


extern struct prog_bit prog_bit;


#define prog_bit_send(bit)	prog_bit.send_bit(bit)
#define	prog_bit_z()		prog_bit.send_z()
#define prog_bit_read()		prog_bit.read_bit()


void prog_bit_invert(void);
void prog_bit_init(const struct prog_bit *bit);

#endif /* !PROG_BIT_H */
