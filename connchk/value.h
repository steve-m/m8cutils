/*
 * value.h - Selection of drive values
 *
 * Written 2006 by Werner Almesberger
 * Copyright 2006 Werner Almesberger
 */


#ifndef VALUE_H
#define VALUE_H

#include <stdint.h>


#define MAX_PINS	64


#define	VALUE_Z		(1 << 0)
#define	VALUE_0R	(1 << 1)
#define	VALUE_1R	(1 << 2)
#define	VALUE_0		(1 << 3)
#define	VALUE_1		(1 << 4)
//#define NUM_VALUES	5


extern uint8_t values[MAX_PINS];   /* current pin configuration */
extern uint8_t allow[MAX_PINS];	   /* allowed pin configurations */
extern uint8_t external[MAX_PINS]; /* possible external drives */


int weak_low(int bit);
int weak_high(int bit);
int strong_low(int bit);
int strong_high(int bit);

int expected(int bit);	/* -1 for undecidable */

const char *value_name(uint8_t value);

#endif /* VALUE_H */
