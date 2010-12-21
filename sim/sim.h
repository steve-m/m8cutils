/* 
 * sim.h - M8C simulator
 *
 * Written 2006 by Werner Almesberger
 * Copyright 2006 Werner Almesberger
 */


#ifndef SIM_H
#define SIM_H


extern const struct chip *chip;
extern int ice;

void exception(const char *msg,...);


#endif /* !SIM_H */
