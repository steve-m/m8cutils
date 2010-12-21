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
extern int prog_power_on;


#define PROG_OPTIONS	"35d:Dlp:O:PRv"
#define PROG_SYNOPSIS	\
  "[-3|-5] [-d driver] [-D] [-p port] [-O option] [-P] [-R] [-v ...]"


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

/*
 * prog_close_cli ends the programmer session. The state in which programmer
 * and target are left depends on the capabilities of the programmer hardware
 * and driver, and whether the -D flag was set.
 */

void prog_close_cli(int reuse);

#endif /* !CLI_H */
