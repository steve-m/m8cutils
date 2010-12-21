/*
 * cpp.c - CPP subprocess
 *
 * Written 2002-2004, 2006 by Werner Almesberger
 * Copyright 2002,2003 California Institute of Technology
 * Copyright 2004, 2006 Werner Almesberger
 */


#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "cpp.h"


static pid_t cpp_pid;
static int cpp_argc = 0;
static const char **cpp_argv;
static int real_stdin = -1;


void add_cpp_arg(const char *arg)
{
    if (!cpp_argc)
	cpp_argc = 1;
    cpp_argv = realloc(cpp_argv,sizeof(const char *)*(cpp_argc+1));
    if (!cpp_argv) {
	perror("realloc");
	exit(1);
    }
    if (cpp_argc == 1)
	cpp_argv[0] = CPP;
    if (arg) {
	arg = strdup(arg);
	if (!arg) {
	    perror("strdup");
	    exit(1);
	}
    }
    cpp_argv[cpp_argc++] = arg;
}


void add_cpp_Wp(const char *arg)
{
    char *tmp = strdup(arg);
    char *curr,*end;

    if (!tmp) {
	perror("strdup");
	exit(1);
    }
    curr = tmp;
    do {
	end = strchr(curr,',');
	if (end)
	    *end++ = 0;
	add_cpp_arg(curr);
	curr = end;
    }
    while (end);
    free(tmp);
}


static void kill_cpp(void)
{
    if (cpp_pid)
	(void) kill(cpp_pid,SIGTERM);
}


static void run_cpp(const char *name,int fd,int close_fd)
{
    char **arg;
    int fds[2];

    if (pipe(fds) < 0) {
        perror("pipe");
        exit(1);
    }
    if (name)
	add_cpp_arg(name);
    add_cpp_arg(NULL);
    cpp_pid = fork();
    if (cpp_pid < 0) {
        perror("fork");
        exit(1);
    }
    if (!cpp_pid) {
	if (close(fds[0]) < 0) {
	    perror("close");
	    exit(1);
	}
	if (close_fd != -1 && close(close_fd) < 0) {
	    perror("close");
	    exit(1);
	}
	if (fd != -1 && dup2(fd,0) < 0) {
	    perror("dup2");
	    exit(1);
	}
	if (dup2(fds[1],1) < 0) {
	    perror("dup2");
	    exit(1);
	}
	if (execvp(CPP,(char **) cpp_argv) < 0) { /* prototype is weird */
	    perror("execvp " CPP);
	    exit(1);
	}
	/* not reached */
    }
    if (close(fds[1]) < 0) {
	perror("close");
	exit(1);
    }
    real_stdin = dup(0);
    if (real_stdin < 0) {
	perror("dup");
	exit(1);
    }
    if (fd != -1 && close(fd) < 0) {
	perror("close");
	exit(1);
    }
    if (dup2(fds[0],0) < 0) {
	perror("dup2");
	exit(1);
    }
    for (arg = (char **) cpp_argv+1; *arg; arg++)
	free(*arg);
    free(cpp_argv);
}


void run_cpp_on_file(const char *name)
{
    run_cpp(name,name ? -1 : 0,-1);
    atexit(kill_cpp);
}


void run_cpp_on_string(const char *str)
{
    int fds[2];
    pid_t pid;
    int left,wrote;

    if (pipe(fds) < 0) {
        perror("pipe");
        exit(1);
    }
    run_cpp(NULL,fds[0],fds[1]);
    pid = fork();
    if (pid < 0) {
	perror("fork");
	exit(1);
    }
    if (!pid) {
	for (left = strlen(str); left; left -= wrote) {
	    wrote = write(fds[1],str,left);
	    if (wrote < 0)
		break; /* die silently */
	    str += wrote;
	}
	exit(0);
    }
    if (close(fds[1]) < 0) {
	perror("close");
	exit(1);
    }
    atexit(kill_cpp);
}


void reap_cpp(void)
{
    int status;

    cpp_pid = 0;
    if (waitpid(cpp_pid,&status,0) < 0) {
	perror("waitpid");
	exit(1);
    }
    if (!status) {
	if (dup2(real_stdin,0) < 0) {
	    perror("dup2");
	    exit(1);
	}
	return;
    }
    if (WIFEXITED(status))
	exit(WEXITSTATUS(status));
    if (WIFSIGNALED(status))
	fprintf(stderr,"cpp terminated with signal %d\n",WTERMSIG(status));
    else
	fprintf(stderr,"cpp terminated with incomprehensible status %d\n",
	  status);
    exit(1);
}
