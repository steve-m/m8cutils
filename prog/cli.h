/*
 * cli.h - Command line interface, back-end
 *
 * Written 2006 by Werner Almesberger
 * Copyright 2006 Werner Almesberger
 */


#ifndef CLI_H
#define CLI_H

extern int prog_voltage;
extern const char *prog_driver;
extern const char *prog_port;
extern int real_time; /* try to obtain real-time priority for acquisition */


#define PROG_OPTIONS	"35d:lp:Rv"
#define PROG_SYNOPSIS	"[-3|-5] [-d driver] [-p port] [-R] [-v ...]"


/*
 * prog_usage prints usage information for the options recognized by
 * prog_option to standard error. Note that prog_usage does _not_ terminate the
 * program.
 *
 * Also note that PROG_SYNOPSIS does not include the option -l, because it
 * should be displayed as a separate invocation.
 */

void prog_usage(void);

/*
 * prog_option returns 1 if the option is recognized and could be successfully
 * processed, 0 if it is not recognized or if not used correctly. In the latter
 * case, the caller should display its "usage" message and exit.
 */

int prog_option(char option,const char *arg);

/*
 * prog_open_cli returns the voltage that was selected (if any) by the driver.
 */

int prog_open_cli(void);

#endif /* !CLI_H */
