/*
 * file.c - File I/O for the M8C utilities
 *
 * Written 2006 by Werner Almesberger
 * Copyright 2006 Werner Almesberger
 */


#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <sys/types.h>

#include "file.h"


#define SECURITY_AREA	0x100000
#define CHECKSUM_AREA	0x200000
#define MAX_RECORD 255


size_t program_size = 0;
size_t security_size = 0;

uint8_t program[PROGRAM_SIZE];
uint8_t security[SECURITY_SIZE];

static int have_checksum = 0;
static uint8_t checksum[2];


uint16_t do_checksum(void)
{
    uint16_t sum = 0;
    int i;

    for (i = 0; i != program_size; i++)
	sum += program[i];
    return sum;
}


static void read_binary(FILE *file)
{
    program_size = fread(program,1,PROGRAM_SIZE,file);
    if (ferror(file)) {
	perror("fread");
	exit(1);
    }
    if (!feof(file)) {
	fprintf(stderr,"file bigger than maximum memory\n");
	exit(1);
    }
}


static int is_hex(char c)
{
    return (c >= '0' && c <= '9') ||
       (c >= 'A' && c <= 'F') ||
       (c >= 'a' && c <= 'f');
}


static int hex_value(char c)
{
    return isdigit(c) ? c-'0' : toupper(c)-'A'+10;
}


static void read_rom(FILE *file)
{
    int second = 0,c;
    uint8_t value;

    while ((c = fgetc(file)) != EOF) {
	if (is_hex(c)) {
	    if (second) {
		if (program_size == PROGRAM_SIZE) {
		    fprintf(stderr,"file bigger than maximum memory\n");
		    exit(1);
		}
		program[program_size++] = (value << 4) | hex_value(c);
		second = 0;
	    }
	    else {
		value = hex_value(c);
		second = 1;
	    }
	}
	else {
	    if (second) {
		fprintf(stderr,
		  "syntax error: byte contains a non-hex character\n");
		exit(1);
	    }
	    if (c == ' ' || c == '\t' || c == '\r' || c == '\n')
		continue;
	    fprintf(stderr,
	      "syntax error: file contains non-hex non-blank character\n");
	    exit(1);
	}
    }
}


static uint8_t get_byte(const char *p)
{
    if (!is_hex(p[0]) || !is_hex(p[1])) {
	fprintf(stderr,"syntax error: missing or invalid hex digit\n");
	exit(1);
    }
    return (hex_value(p[0]) << 4) | hex_value(p[1]);
}


/*
 * The length returned also contains the address and the record type, so it's
 * the number of data bytes plus 3.
 */

static int read_record(FILE *file,uint8_t *data,int size)
{
    char line[MAX_RECORD*3]; /* leave plenty of space */
    const char *p;
    uint8_t sum;
    int length,i;

    if (!fgets(line,sizeof(line),file))
	return 0;
    if (*line != ':') {
	fprintf(stderr,"syntax error: record does not begin with a colon\n");
	exit(1);
    }
    length = get_byte(line+1)+3;
    if (length+1 > size) {
	fprintf(stderr,
	  "record is too long for buffer (%d bytes >= %d bytes)\n",
	  length,size);
	exit(1);
    }
    sum = length-3;
    for (i = 0; i != length+1; i++) {
	data[i] = get_byte(line+2*i+3);
	sum += data[i];
    }
    if (sum) {
	fprintf(stderr,"record checksum error (0x%02x)\n",data[length]);
	exit(1);
    }
    for (p = line+2*length+5; *p; p++)
	if (*p != '\n' && *p != '\r') {
	    fprintf(stderr,"record ends in \\%03o instead of \\n or \\r\n",
	      *p);
	    exit(1);
	}
    return length;
}


static void read_hex(FILE *file)
{
    uint8_t record[MAX_RECORD+4]; /* leave space for meta-data */
    uint32_t extended = 0,address,last_address = 0;
    int limit = PROGRAM_SIZE;
    uint8_t *p = program;

    while (1) {
	int got;

	got = read_record(file,record,sizeof(record));
	if (!got)
	    break;
	switch (record[2]) {
	    case 0:
		address = extended | (record[0] << 8) | record[1];
		if (address != last_address && address != extended) {
		    fprintf(stderr,
		      "non-sequential addressing is not supported\n");
		    exit(1);
		}
		if ((address & 0xffff)+got-3 > limit) {
		    fprintf(stderr,"record exceeds address space\n");
		    exit(1);
		}
		memcpy(p+(address & 0xffff),record+3,got-3);
		last_address = address+got-3;
		if (!(last_address & ~0xffff))
		    program_size = last_address;
		else if (last_address == CHECKSUM_AREA+2)
		    have_checksum = 1;
		else if ((last_address & ~0xffff) == SECURITY_AREA)
		    security_size = last_address-SECURITY_AREA;
		break;
	    case 1:
		if (got != 3) {
		    fprintf(stderr,
		      "end address record has length %d instead of 0\n",got-3);
		    exit(1);
		}
		return;
	    case 4:
		if (got != 5) {
		    fprintf(stderr,
		      "extended linear address record has length %d instead "
		      "of 2\n",got-3);
		    exit(1);
		}
		extended = (record[3] << 24) | (record[4] << 16);
		if (!extended) {
		    limit = PROGRAM_SIZE;
		    p = program;
		}
		else if (extended == SECURITY_AREA) {
		    limit = SECURITY_SIZE;
		    p = security;
		}
		else if (extended == CHECKSUM_AREA) {
		    limit = 2;
		    p = checksum;
		}
		else {
		    fprintf(stderr,
		      "extended address 0x%08x out of range\n",extended);
		    exit(1);
		}
		break;
	    default:
		fprintf(stderr,"unrecognized record type 0x%02x\n",record[2]);
		exit(1);
	}
    }
    fprintf(stderr,"no end record found\n");
    exit(1);
}


void read_file(const char *name,int binary)
{
    FILE *file;

    if (!strcmp(name,"-"))
	file = stdin;
    else {
	file = fopen(name,"r");
	if (!file) {
	    perror(name);
	    exit(1);
	}
    }
    program_size = security_size = 0;
    have_checksum = 0;
    if (binary)
	read_binary(file);
    else {
	int c;

	c = fgetc(file);
	if (c == EOF)
	    goto out;
	ungetc(c,file);
	if (c == ':')
	    read_hex(file);
	else if (is_hex(c))
	    read_rom(file);
	else
	    read_binary(file);
    }
    if (have_checksum) {
	uint16_t sum = (checksum[0] << 8) | checksum[1];

	if (do_checksum() != sum) {
	    fprintf(stderr,
	      "file checksum error: got 0x%04x, calculated 0x%04x\n",
	      sum,do_checksum());
	    exit(1);
	}
    }

out:
    (void) fclose(file);
}


static void write_binary(FILE *file)
{
    size_t wrote;

    wrote = fwrite(program,1,program_size,file);
    if (wrote == program_size)
	return;
    perror("fwrite");
    exit(1);
}


static void write_rom(FILE *file)
{
    int i;

    for (i = 0; i != program_size; i++)
	if (fprintf(file,"%02X%c",program[i],
	  (i & 7) == 7 || i == program_size-1 ? '\n' : ' ') < 0) {
	    perror("fprintf");
	    exit(1);
	}
}


static void write_hex_record(FILE *file,uint16_t addr,const uint8_t *data,
  size_t size)
{
    const uint8_t *end = data+size;
    int sum;

    sum = size+(addr >> 8)+addr;
    if (fprintf(file,":%02X%04X00",size,addr) < 0)
	goto error;
    while (data != end) {
	if (fprintf(file,"%02X",*data) < 0)
	    goto error;
	sum += *data;
	data++;
    }
    if (fprintf(file,"%02X\n",(-sum) & 0xff) < 0)
	goto error;
    return;

error:
    perror("fprintf");
    exit(1);
}


static void write_hex(FILE *file)
{
    int i = 0;
    int sum;

    for (i = 0; i < program_size; i += BLOCK_SIZE)
	write_hex_record(file,i,program+i,
	  program_size-i > BLOCK_SIZE ? BLOCK_SIZE : program_size-i);
    for (i = 0; i < security_size; i += BLOCK_SIZE)
	write_hex_record(file,i,security+i,
	  security_size-i > BLOCK_SIZE ? BLOCK_SIZE : security_size-i);
    sum = do_checksum();
    if (fprintf(file,":020000040020DA\n:02000000%02X%02X%02X\n",
      (sum >> 8) & 0xff,sum & 0xff,-(2+(sum >> 8)+sum) & 0xff) < 0)
	goto error;
    if (fprintf(file,":00000001FF\n") < 0)
	goto error;
    return;

error:
    perror("fprintf");
    exit(1);
}


void write_file(const char *name,int binary,int hex)
{
    FILE *file;

    if (!strcmp(name,"-"))
	file = stdout;
    else {
	file = fopen(name,"w");
	if (!file) {
	    perror(name);
	    exit(1);
	}
    }
    if (binary)
	write_binary(file);
    else if (!hex)
	write_rom(file);
    else
	write_hex(file);
    if (fclose(file)) {
	perror(name);
	exit(1);
    }
}


void pad_file(void)
{
    while (program_size & (BLOCK_SIZE-1))
	program[program_size++] = PAD_BYTE;
    while (security_size & (BLOCK_SIZE-1))
	security[security_size++] = PAD_BYTE;
}
