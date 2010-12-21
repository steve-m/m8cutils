/*
 * out.c - Produce human-readable output
 *
 * Written 2006 by Werner Almesberger
 * Copyright 2006 Werner Almesberger
 */


#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "interact.h"

#include "port.h"
#include "test.h"
#include "out.h"


#define DELAY_MS	50	/* delay for each pin in verbose mode */


int tests = 0;
int patterns = 0;

static int errors = 0;


/* ----- Progress and status output ---------------------------------------- */


void current(int bit,int value)
{
    int i;

    fputc('\r',stderr);
    for (i = 0; i != MAX_PINS; i++) {
	if (!(defined >> i))
	    break;
	if (i && !(i & 7))
	    fputc(' ',stderr);
	fputc(i == bit ? value ? '^' : 'v' :
	  (defined >> i) & 1 ? '-' : '.',stderr);
    }
    if (bit != -1)
	fprintf(stderr,"  P%d[%d]",bit/8,bit & 7);
    fflush(stderr);
    usleep(DELAY_MS*1000);
}


void status(void)
{
    int i;

    fprintf(stderr,"\n");

    for (i = 0; i != 8; i++) {
	if (!(defined >> i*8))
	    break;
	fprintf(stderr,"P[%d]     ",i);
    }
    fprintf(stderr,"\n");
    
    for (i = 0; i != 8; i++) {
	if (!(defined >> i*8))
	    break;
	fprintf(stderr,"01234567 ");
    }
    fprintf(stderr,"\n\n");
 
    for (i = 0; i != MAX_PINS; i++) {
	int value;

	if (!(defined >> i))
	    break;
	if (i && !(i & 7))
	    fputc(' ',stderr);
	value  = decode_value(i);
	switch (value) {
	    case VALUE_Z:
		fputc('Z',stderr);
		break;
	    case VALUE_0:
	    case VALUE_0R:
		fputc('0',stderr);
		break;
	    case VALUE_1:
	    case VALUE_1R:
		fputc('1',stderr);
		break;
	    default:
		abort();
	}
    }
    fprintf(stderr,"\n");
 
    for (i = 0; i != MAX_PINS; i++) {
	int value;

	if (!(defined >> i))
	    break;
	if (i && !(i & 7))
	    fputc(' ',stderr);
	value = decode_value(i);
	switch (value) {
	    case VALUE_Z:
	    case VALUE_0:
	    case VALUE_1:
		fputc(' ',stderr);
		break;
	    case VALUE_0R:
	    case VALUE_1R:
		fputc('R',stderr);
		break;
	    default:
		abort();
	}
    }
    fprintf(stderr,"\n");
}


/* ----- Condensed output for dry runs ------------------------------------- */


void print_dry_run(uint64_t set)
{
    int first = 1;
    int i,want;

    for (i = 0; i != MAX_PINS; i++) {
	if (!((set >> i) & 1))
	    continue;
	if (!first)
	    printf(", ");
	want = expected(i);
	printf("P%d[%d] = %s (%c)",i/8,i & 7,value_name(decode_value(i)),
	  want == -1 ? 'X' : want ? '1' : '0');
	first = 0;
    }
    if (!first)
	printf("\n");
}


/* ----- Test maps --------------------------------------------------------- */


#define MAP_VISITED 1
#define MAP_ERROR 2


static char map_base[2][MAX_PINS];
static char map[2][MAX_PINS][MAX_PINS];


static void set_map(int bit,int current_bit,int current_value,int flag)
{
    if (current_bit == -1)
	map_base[current_value][bit] |= flag;
    else
	map[current_value][current_bit][bit] |= flag;
}


static char map_char(const char *row,int n)
{
    if (!((defined >> n) & 1))
	return '.';
    if (row[n] & MAP_ERROR)
	return '*';
    if (row[n] & MAP_VISITED)
	return '-';
    return '?';
}


static void map_row(FILE *file,const char *row)
{
    int i,j;

    for (i = 0; i != MAX_PINS; i += 8)
	if ((defined >> i) & 0xff) {
	    fputc(' ',file);
	    for (j = 0; j != 8; j++)
		fputc(map_char(row,i+j),file);
	}
    fputc('\n',file);
}


static void print_map(FILE *file,const char base[MAX_PINS],
  char m[MAX_PINS][MAX_PINS])
{
    int i;

    fprintf(file,"FG/BG ");
    for (i = 0; i != MAX_PINS; i += 8)
	if ((defined >> i) & 0xff)
	    fprintf(file," P%d[%d]   ",i/8,i & 7);
    fprintf(file,"\n%6s","");
    for (i = 0; i != MAX_PINS; i += 8)
	if ((defined >> i) & 0xff)
	    fprintf(file," 01234567");

    fprintf(file,"\n(base)");
    map_row(file,base);

    for (i = 0; i != MAX_PINS; i++)
	if ((defined >> i) & 1) {
	    fprintf(file,"P%d[%d] ",i/8,i & 7);
	    map_row(file,m[i]);
	}
}


static void print_maps(FILE *file)
{
    fprintf(file,"Foreground = 0, background = 1:\n");
    print_map(file,map_base[0],map[0]);   
    fprintf(file,"\nForeground = 1, background = 0:\n");
    print_map(file,map_base[1],map[1]);   
}


/* ----- Read back ports and check ----------------------------------------- */


static void print_reg(int addr)
{
    uint8_t data;
    int i;

    data = read_reg(addr);
    fputc(' ',stderr);
    for (i = 0; i != 8; i++)
	fprintf(stderr,"%d",(data >> i) & 1);
}


int expect(int bit,int want,int got,int current_bit,int current_value)
{
    int i;

    tests++;
    set_map(bit,current_bit,current_value,MAP_VISITED);
    if (want == got)
	return 1;
    set_map(bit,current_bit,current_value,MAP_ERROR);
    if (errors && !all)
	return 0;
    current(current_bit,current_value);
    status();
    fprintf(stderr,"%*s%*s^\n",bit/8*9,"",bit & 7,"");
    fprintf(stderr,"P%d[%d]: expected %d, got %d\n",bit/8,bit & 7,want,got);
    fprintf(stderr,"\nPort    PRTxDM2  PRTxDM1  PRTxDM0  PRTxDR");
    fprintf(stderr,"\n        01234567 01234567 01234567 01234567\n");
    for (i = 0; i != 8; i++) {
	if (!(defined >> i*8))
	    break;
	fprintf(stderr,"%d      ",i);
	print_reg(PRTxDMy(i,2));
	print_reg(PRTxDMy(i,1));
	print_reg(PRTxDMy(i,0));
	print_reg(PRTxDR(i));
	fprintf(stderr,"\n");
    }
    errors++;
    if (!all && !do_map)
	exit(1);
    fprintf(stderr,"\n");
    for (i = 0; i != 79; i++)
	fputc('-',stderr);
    fprintf(stderr,"\n\n");
    return 0;
}


/* ----- Communication loss ------------------------------------------------ */


void lost_comm(int current_bit,int current_value)
{
    current(current_bit,current_value);
    status();
    fprintf(stderr,"lost communication with the target\n");
    exit(1);
}


/* ----- Final report ------------------------------------------------------ */


int finish(void)
{
    int i;

    if (!quiet)
	fprintf(stderr,"\r%79s\r","");
    if (do_map)
	print_maps(stdout);
    if (verbose) {
	int pins = 0;

	for (i = 0; i != MAX_PINS; i++)
	    if ((defined >> i) & 1)
		pins++;
	fprintf(stderr,"%d pin%s, %d pattern%s, %d test%s.\n",
	  pins,pins == 1 ? "" : "s",
	  patterns,patterns == 1 ? "" : "s",
	  tests,tests == 1 ? "" : "s");
	if (!errors && !dry_run)
	    fprintf(stderr,"Connection check passed.\n");
    }
    if (errors)
	fprintf(stderr,"Found %d error%s.\n",
	  errors,errors == 1 ? "" : "s");

    return !!errors;
}
