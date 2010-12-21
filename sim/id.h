/*
 * id.h - Identifiers
 *
 * Written 2006 by Werner Almesberger
 * Copyright 2006 Werner Almesberger
 */


#ifndef ID_H
#define ID_H

#include <stdint.h>


/*
 * id->new is used to protect us from partial definitions due to syntax error.
 * E.g., the syntax error in the first line of
 *
 * define FOO reg[123
 * define FOO reg[123]
 *
 * still leaves a new ID called "FOO", so we need to know that it's still new
 * when trying again on the second line.
 */

struct id {
    const char *name;
    int new;		/* 1 if new (see above) */
    uint16_t reg;	/* register number, 0xffff if value */
    uint8_t field;	/* field mask, 0 if register */
};


struct id *id_lookup(const char *name);
struct id *id_new(const char *name);

void id_reg(struct id *id,uint16_t reg);
void id_field(struct id *id,uint16_t reg,uint8_t mask);
void id_value(struct id *id,uint8_t value);

void id_init(void);

#endif /* !ID_H */
