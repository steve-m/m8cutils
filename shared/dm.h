/*
 * dm.h - Helper functions for encoding/decoding drive mode values
 *
 * Written 2006 by Werner Almesberger
 * Copyright 2006 Werner Almesberger
 */


#ifndef DM_H
#define DM_H


#include <stdint.h>


/*
 * We make the values powers of two, so that programs like m8cbscan can also
 * use these definitions.
 */

#define VALUE_0         (1 << 0)
#define VALUE_0R        (1 << 1)
#define VALUE_Z         (1 << 2)
#define VALUE_1R        (1 << 3)
#define VALUE_1         (1 << 4)


const uint8_t dm_value[16];
uint8_t decode_dm_value(int dm2,int dm1,int dm0,int dr);
const char *dm_value_name(uint8_t value);

#endif /* DM_H */
