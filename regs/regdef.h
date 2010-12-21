/*
 * regdef.h - Register definitions
 *
 * Written 2006 by Werner Almesberger
 * Copyright 2006 Werner Almesberger
 */


#ifndef REGDEF_H
#define REGDEF_H

#include <stdint.h>


#define PSOC_REG_ADDR_X	0x200	/* register exists in both banks */


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
    const char *chips; /* chips for which this definition applies, '0' or '1' */
    const struct psoc_regdef_field *fields; /* NULL if unstructured */
};

struct psoc_regdef_chip {
    const char *name; /* NULL if last */
    int ind; /* index into struct psoc_regdef_register.chips */
};


extern const struct psoc_regdef_register psoc_regdef[];
extern const struct psoc_regdef_chip proc_regdef_chips[];

/*
 * psoc_regdef_find_chip looks up a chip name and returns the index of the
 * chip. If no such chip exists, psoc_regdef_find_chip returns -1. The search
 * is case-insensitive. The bare chip name or the name with the number of pins
 * appended can be used, e.g., CY8C24894 or CY8C24894_56.
 */

int psoc_regdef_find_chip(const char *name);

/*
 * psoc_regdef_applicable returns a non-zero value if the register definition
 * can be used with the specified chip. "chip" is an index returned by
 * psoc_regdef_find_chip. If "chip" is -1, psoc_regdef_applicable will always
 * return a non-zero value.
 */

int psoc_regdef_applicable(const struct psoc_regdef_register *reg,int chip);

#endif /* REGDEF_H */
