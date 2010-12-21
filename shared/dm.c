/*
 * dm.c - Helper functions for encoding/decoding drive mode values
 *
 * Written 2006 by Werner Almesberger
 * Copyright 2006 Werner Almesberger
 */


#include <stdlib.h>
#include <stdint.h>

#include "dm.h"



const uint8_t dm_value[16] = {
    VALUE_0R,	VALUE_1,
    VALUE_0,	VALUE_1,
    VALUE_Z,	VALUE_Z,
    VALUE_0,	VALUE_1R,
    VALUE_Z,	VALUE_1,
    VALUE_0,	VALUE_1,
    VALUE_Z,	VALUE_Z,
    VALUE_0,	VALUE_Z,
};


uint8_t decode_dm_value(int dm2,int dm1,int dm0,int dr)
{
    return dm_value[(dm2 << 3) | (dm1 << 2) | (dm0 << 1) | dr];
}


const char *dm_value_name(uint8_t value)
{
    switch (value) {
	case VALUE_Z:
	    return "Z";
	case VALUE_0R:
	    return "0R";
	case VALUE_0:
	    return "0";
	case VALUE_1R:
	    return "1R";
	case VALUE_1:
	    return "1";
	default:
	    abort();
    }
}
