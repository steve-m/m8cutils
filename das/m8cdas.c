/*
 * m8cdas.c - M8C disassembler
 *
 * Written 2006 by Werner Almesberger
 * Copyright 2006 Werner Almesberger
 */


#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "disasm.h"
#include "symmap.h"
#include "file.h"


static void usage(const char *name)
{
    fprintf(stderr,
"usage: %s [-p initial_pc] [-s|-t] [-m symbol_file] [-b] [file]\n"
"       %s [-p initial_pc] [-s|-t] [-m symbol_file] -e byte ...\n"
"       %s -V\n\n"
"  -b             override file type detection and force binary mode\n"
"  -e             use the following bytes as code\n"
"  -m symbol_file load the symbol map from the specified file\n"
"  -p initial_pc  set the initial PC to this value, accepts hexadecimal\n"
"                 with leading \"0x\" (default: 0)\n"
"  -s             short output, do not include PC and code\n"
"  -t             include instruction timing\n"
"  -V             only print the version number and exit\n"
"  file           binary or hex file to read, \"-\" for stdin (default:\n"
"                 stdin)\n"
"  byte           hexadecimal code byte, use of \"0x\" is optional\n",
  name,name,name);
    exit(1);
}


int main(int argc,char **argv)
{
    uint8_t code[PROGRAM_SIZE+2]; /* space to disassemble beyond EOF */
    char buf[MAX_DISASM_LINE];
    unsigned long pc = 0,pos = 0;
    const char *symbols = NULL;
    char *end;
    int from_file = 1,hex = 1,timing = 0,binary = 0;
    int c;

    while ((c = getopt(argc,argv,"bem:p:stV")) != EOF)
	switch (c) {
	    case 'b':
		binary = 1;
		break;
	    case 'e':
		from_file = 0;
		break;
	    case 'm':
		symbols = optarg;
		break;
	    case 'p':
		pc = strtoul(optarg,&end,0);
		if (*end) {
		    fprintf(stderr,"invalid number \"%s\"\n",optarg);
		    return 1;
		}
		if (pc > 0xffff) {
		    fprintf(stderr,"PC (0x%lx) out of range (0...0xffff)\n",
		      pc);
		    return 1;
		}
		break;
	    case 's':
		hex = 0;
		break;
	    case 't':
		timing = 1;
		break;
	    case 'V':
		printf("m8cdas from m8cutils version %s\n",VERSION);
		exit(0);
	    default:
		usage(*argv);
	}
    if (from_file) {
	switch (argc-optind) {
	    case 0:
		read_file("-",binary);
		break;
	    case 1:
		read_file(argv[optind],binary);
		break;
	    default:
		usage(*argv);
	}
	memcpy(code,program,program_size);
    }
    else {
	int i;

	if (argc-optind > PROGRAM_SIZE) {
	    fprintf(stderr,"code too long\n"); /* duh ... */
	    exit(1);
	}
	for (i = optind; i != argc; i++) {
	    unsigned long v;

	    v = strtoul(argv[i],&end,16);
	    if (*end || v > 0xff) {
		fprintf(stderr,"invalid byte \"%s\"\n",argv[i]);
		exit(1);
	    }
	    code[i-optind] = v;
	}
	program_size = argc-optind;
    }
    code[program_size] = code[program_size+1] = 0;

    if (symbols)
	sym_read_file_by_name(symbols);

    while (pos < program_size) {
	int size,i,attr;
	const char *sym;

	sym = sym_by_value(pc+pos,SYM_ATTR_ROM,&attr);
	if (sym)
	    printf("%s:%s\n",sym,attr & SYM_ATTR_GLOBAL ? ":" : "");
	size = m8c_disassemble(pc+pos,code+pos,buf,sizeof(buf));
	if (hex) {
	    if (printf("%04lX ",(pc+pos) & 0xffff) < 0)
		goto error;
	    for (i = 0; i != size; i++)
		if (printf(" %02X",code[pos+i]) < 0)
		    goto error;
	    if (putchar('\t') == EOF)
		goto error;
	}
	if (timing)
	    if (printf("[%02d]\t",m8c_cycles[code[pos]]) < 0)
		goto error;
	if (printf("%s\n",buf) < 0)
	    goto error;
	pos += size;
	if (pos > program_size) {
	    fflush(stdout);
	    fprintf(stderr,"disassembled beyond end of code\n");
	    exit(1);
	}
    }
    if (fclose(stdout) == EOF) {
	perror("fclose");
	exit(1);
    }
    return 0;

error:
    perror("printf");
    exit(1);
}
