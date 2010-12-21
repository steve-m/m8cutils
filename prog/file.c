/*
 * file.c - File I/O
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


#define MAX_RECORD 255


size_t file_size = 0;
int have_security = 0;

uint8_t program[PROGRAM_SIZE];
uint8_t security[SECURITY_SIZE];

static int have_checksum = 0;
static uint8_t checksum[2];


uint16_t do_checksum(void)
{
    uint16_t sum = 0;
    int i;

    for (i = 0; i != file_size; i++)
	sum += program[i];
    return sum;
}


static void read_binary(FILE *file)
{
    file_size = fread(program,1,PROGRAM_SIZE,file);
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
		    file_size = last_address;
		else if (last_address == 0x100002)
		    have_checksum = 1;
		else if (last_address == 0x200040)
		    have_security = 1;
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
		else if (extended == 0x100000) {
		    limit = SECURITY_SIZE;
		    p = security;
		}
		else if (extended == 0x200000) {
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
    file_size = 0;
    have_checksum = have_security = 0;
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
	else
	    read_binary(file);
    }
    if (file_size % BLOCK_SIZE) {
	fprintf(stderr,
	  "warning: padding file to a block boundary\n");
	while (file_size % BLOCK_SIZE)
	    program[file_size++] = PAD_BYTE;
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

    wrote = fwrite(program,1,file_size,file);
    if (wrote == file_size)
	return;
    perror("fwrite");
    exit(1);
}


static void write_hex(FILE *file)
{
    int i = 0;
    int sum;

    while (i != file_size) {
	sum = 0x40+(i >> 8)+i;
	if (fprintf(file,":40%04X00",i) < 0)
	    goto error;
	do {
	    if (fprintf(file,"%02X",program[i]) < 0)
		goto error;
	    sum += program[i];
	    i++;
	}
	while (i % BLOCK_SIZE);
	if (fprintf(file,"%02X\r\n",(-sum) & 0xff) < 0)
	    goto error;
    }
    sum = do_checksum();
    if (fprintf(file,":020000040020DA\r\n:02000000%02X%02X%02X\r\n",
      (sum >> 8) & 0xff,sum & 0xff,-(2+(sum >> 8)+sum) & 0xff) < 0)
	goto error;
    if (fprintf(file,":00000001FF\r\n") < 0)
	goto error;
    return;

error:
    perror("fprintf");
    exit(1);
}


void write_file(const char *name,int binary)
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
    else
	write_hex(file);
    if (fclose(file)) {
	perror(name);
	exit(1);
    }
}
