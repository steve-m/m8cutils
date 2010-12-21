/*
 * value.h - Selection of drive values
 *
 * Written 2006 by Werner Almesberger
 * Copyright 2006 Werner Almesberger
 */


#ifndef VALUE_H
#define VALUE_H

#include <stdint.h>

#include "dm.h"


#define MAX_PINS	64


extern uint8_t values[MAX_PINS];   /* current pin configuration */
extern uint8_t allow[MAX_PINS];	   /* allowed pin configurations */
extern uint8_t external[MAX_PINS]; /* possible external drives */


int gentle_low(int bit);
int gentle_high(int bit);
int weak_low(int bit);
int weak_high(int bit);
int strong_low(int bit);
int strong_high(int bit);

int expected(int bit);	/* -1 for undecidable */

#endif /* VALUE_H */
