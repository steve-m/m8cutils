/*
 * rt.c - Helper functions to enter/leave real-time mode
 *
 * Written 2004,2005 by Werner Almesberger
 *
 * (C) Copyright Koninklijke Philips Electronics NV 2004, 2005. All rights
 * reserved. For licensing and warranty information, see the file
 * ../COPYING.GPLv2.
 */


#include <stdlib.h>
#include <stdio.h>
#include <sched.h>
#include <sys/mman.h>

#include "rt.h"


void realtimize(void)
{
    struct sched_param prm;

    if (mlockall(MCL_CURRENT | MCL_FUTURE) < 0) {
        perror("mlockall");
        exit(1);
    }
    prm.sched_priority = sched_get_priority_max(SCHED_FIFO);
    if (prm.sched_priority < 0) {
        perror("sched_get_priority_max SCHED_FIFO");
        exit(1);
    }
    if (sched_setscheduler(0,SCHED_FIFO,&prm) < 0) {
        perror("sched_setscheduler SCHED_FIFO");
        exit(1);
    }
}


void unrealtime(void)
{
    struct sched_param prm = { .sched_priority = 0 };

    if (sched_setscheduler(0,SCHED_OTHER,&prm) < 0) {
        perror("sched_setscheduler SCHED_OTHER");
        exit(1);
    }
    if (munlockall() < 0) {
        perror("munlockall");
        exit(1);
    }
}
