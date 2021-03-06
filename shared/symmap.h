/*
 * symmap.h - Symbol map reader
 *
 * Written 2006 by Werner Almesberger
 * Copyright 2006 Werner Almesberger
 */


#ifndef SYMMAP_H
#define SYMMAP_H

#include <stdint.h>
#include <stdio.h>
#include <ctype.h>


#define SYM_ATTR_ROM	0x0001
#define SYM_ATTR_RAM	0x0002	
#define SYM_ATTR_REG	0x0004	/* not yet used */
#define SYM_ATTR_EQU	0x0008	
#define SYM_ATTR_GLOBAL	0x0010
#define SYM_ATTR_LOCAL	0x0020
#define SYM_ATTR_MORE	0x8000	/* indicator that there are more results */

#define SYM_ATTR_ANY	0x003f


static inline int sym_is_redefinable(const char *name)
{
    return name[0] == '.' && isdigit(name[1]);
}


void sym_read_file(FILE *file);
void sym_read_file_by_name(const char *name);

/*
 * sym_by_name: "dir" specifies the direction relative to "pc":
 * -1: backwards
 *  0: return first match
 *  1: forwards
 */

const uint32_t *sym_by_name(const char *name,int attr_mask,int *attr,
  uint16_t pc,int dir);
const char *sym_by_value(uint32_t value,int attr_mask,int *attr);

#endif /* !SYMMAP_H */
