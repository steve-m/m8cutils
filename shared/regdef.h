/*
 * regdef.h - Register definitions
 *
 * Written 2006 by Werner Almesberger
 * Copyright 2006 Werner Almesberger
 */


#ifndef REGDEF_H
#define REGDEF_H

#include <stdint.h>


struct psoc_regdef_value {
    const char *name; /* NULL if last */
    uint8_t value;
};

/*
 * Fields must be in the order highest to lowest bit, covering all eight bits.
 * Unused bits are encoded with name = NULL. The end of the fields list is
 * detected simply by the bit could having reached eight.
 */

struct psoc_regdef_field {
    const char *name; /* NULL if unassigned */
    int bits;
    const struct psoc_regdef_value *values; /* NULL if just numbers */
};

struct psoc_regdef_register {
    const char *name; /* NULL if last */
    uint16_t addr;
    const struct psoc_regdef_field *fields; /* NULL if unstructured */
};


extern const struct psoc_regdef_register psoc_regdef[];


#endif /* REGDEF_H */
