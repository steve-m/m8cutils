/*
 * value.c - Selection of drive values
 *
 * Written 2006 by Werner Almesberger
 * Copyright 2006 Werner Almesberger
 */


#include <stdint.h>
#include <stdlib.h>

#include "prog.h"
#include "interact.h"
#include "vectors.h"

#include "port.h"
#include "value.h"


uint8_t values[MAX_PINS];
uint8_t allow[MAX_PINS];
uint8_t external[MAX_PINS];


/* ----- Set pin to a "gentle" drive --------------------------------------- */


/*
 * We choose a drive mode that goes in the desired direction, but that avoids
 * the strong drives, i.e., "0" or "1". This way, we don't get any high-current
 * shorts if model and real circuit disagree.
 *
 * This is very similar to "weak", but "gentle" will give up sooner.
 */


static int gentle(int bit,int value_x,int value_xr,int value_y,int value_yr)
{
    uint8_t a,e;

    a = allow[bit];
//fprintf(stderr,"bit %d a %d x %d\n",bit,a,value_x);
    e = external[bit];
    if ((a & VALUE_Z) &&
      (e & (value_x | value_xr)) && !(e & (value_y | value_yr | VALUE_Z)))
	return VALUE_Z;
    if ((a & value_xr) &&
      (e & (value_x | value_xr | VALUE_Z)) && !(e & (value_y | value_yr)))
	return value_xr;
    if ((a & value_yr) && e == value_x)
	return value_yr;
    return -1;
}


static int gentle_default(int bit,
  int value_x,int value_xr,int value_y,int value_yr)
{
    uint8_t a;

    a = allow[bit];
    if (a & VALUE_Z)
	return VALUE_Z;
    if (a & value_xr)
	return value_xr;
    if (a & value_yr)
	return value_yr;
    return -1;
}


int gentle_low(int bit)
{
    int value;

    value = gentle(bit,VALUE_0,VALUE_0R,VALUE_1,VALUE_1R);
    if (value == -1)
	value = gentle_default(bit,VALUE_0,VALUE_0R,VALUE_1,VALUE_1R);
    values[bit] = value == -1 ? VALUE_Z : value;
    return value != -1;
}


int gentle_high(int bit)
{
    int value;

    value = gentle(bit,VALUE_1,VALUE_1R,VALUE_0,VALUE_0R);
    if (value == -1)
	value = gentle_default(bit,VALUE_1,VALUE_1R,VALUE_0,VALUE_0R);
    values[bit] = value == -1 ? VALUE_Z : value;
    return value != -1;
}


/* ----- Set pin to weakest drive possible --------------------------------- */


/*
 * We try to find the weakest drive that still guarantees a clear response.
 * If no such setting exists, we can't test this pin, so we settle for
 * something that goes at least in the desired direction, without upsetting the
 * neighbours.
 */


static int weak(int bit,int value_x,int value_xr,int value_y,int value_yr)
{
    uint8_t a,e;

    a = allow[bit];
//fprintf(stderr,"bit %d a %d x %d\n",bit,a,value_x);
    e = external[bit];

    if ((a & value_yr) && e == value_x)
	return value_yr;
    if ((a & VALUE_Z) &&
      (e & (value_x | value_xr)) && !(e & (value_y | value_yr | VALUE_Z)))
	return VALUE_Z;
    if ((a & value_xr) &&
      (e & (value_x | value_xr | VALUE_Z)) && !(e & (value_y | value_yr)))
	return value_xr;
    if ((a & value_xr) &&
      (e & (value_x | value_xr | VALUE_Z | value_yr)) && !(e & value_y))
	return value_x;

    return -1;
}


static int weak_default(int bit,
  int value_x,int value_xr,int value_y,int value_yr)
{
    uint8_t a;

    a = allow[bit];
    if (a & VALUE_Z)
	return VALUE_Z;
    if (a & value_xr)
	return value_xr;
    if (a & value_yr)
	return value_yr;
    if (a & value_y)
	return value_y;
    if (a & value_x)
	return value_x;
    return -1;
}


int weak_low(int bit)
{
    int value;

    value = weak(bit,VALUE_0,VALUE_0R,VALUE_1,VALUE_1R);
    if (value == -1)
	value = weak_default(bit,VALUE_0,VALUE_0R,VALUE_1,VALUE_1R);
    values[bit] = value == -1 ? VALUE_Z : value;
    return value != -1;
}


int weak_high(int bit)
{
    int value;

    value = weak(bit,VALUE_1,VALUE_1R,VALUE_0,VALUE_0R);
    if (value == -1)
	value = weak_default(bit,VALUE_1,VALUE_1R,VALUE_0,VALUE_0R);
    values[bit] = value == -1 ? VALUE_Z : value;
    return value != -1;
}


/* ----- Set pin to strongest drive possible ------------------------------- */


static int strong(int bit,int value_x,int value_xr,int value_y,int value_yr)
{
    uint8_t a,e;

    a = allow[bit];
    e = external[bit];
    if (a & value_x)
	return value_x;
    if (a & value_xr)
	return e & value_y ? -1 : value_xr;
    if (a & VALUE_Z)
	return (e & value_x) || (e & value_xr) ? VALUE_Z : -1;
    if (a & value_yr)
	return e & value_x ? value_yr : -1;
    return -1;
}


int strong_low(int bit)
{
    int value;

    value = strong(bit,VALUE_0,VALUE_0R,VALUE_1,VALUE_1R);
    values[bit] = value == -1 ? VALUE_1R : value;
    return value != -1;
}


int strong_high(int bit)
{
    int value;

    value = strong(bit,VALUE_1,VALUE_1R,VALUE_0,VALUE_0R);
    values[bit] = value == -1 ? VALUE_0R : value;
    return value != -1;
}


/* ----- Read back ports and check ----------------------------------------- */


int expected(int bit)
{
    uint8_t value = values[bit];
    uint8_t e = external[bit];

    switch (value) {
	case VALUE_Z:
	    if (e & VALUE_Z)
		return -1;
	    if ((e & (VALUE_0 | VALUE_0R)) && !(e & (VALUE_1 | VALUE_1R)))
		return 0;
	    if ((e & (VALUE_1 | VALUE_1R)) && !(e & (VALUE_0 | VALUE_0R)))
		return 1;
	    break;
	case VALUE_0R:
	    if (e & VALUE_1R)
		return -1;
	    if ((e & (VALUE_0 | VALUE_0R | VALUE_Z)) && !(e & VALUE_1))
		return 0;
	    if ((e & VALUE_1) && !(e & (VALUE_0 | VALUE_0R | VALUE_Z)))
		return 1;
	    break;
	case VALUE_1R:
	    if (e & VALUE_0R)
		return -1;
	    if ((e & (VALUE_1 | VALUE_1R | VALUE_Z)) && !(e & VALUE_0))
		return 1;
	    if ((e & VALUE_0) && !(e & (VALUE_1 | VALUE_1R | VALUE_Z)))
		return 0;
	    break;
	case VALUE_0:
	    return 0;
	case VALUE_1:
	    return 1;
    }
    return -1;
}
