/*
 * test.c - Generate and perform the tests
 *
 * Written 2006 by Werner Almesberger
 * Copyright 2006 Werner Almesberger
 */


#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "util.h"
#include "prog.h"
#include "interact.h"
#include "vectors.h"

#include "port.h"
#include "value.h"
#include "out.h"
#include "test.h"


uint64_t defined;

struct alternatives *alternatives = NULL;
struct alternatives **next_alt = &alternatives;

int all = 0;
int do_map = 0;
int dry_run = 0;

static uint64_t selected;
static int current_bit,current_high;


/* ----- Read back ports and check ----------------------------------------- */


static int check_pin(int bit,int got)
{
    int want;

    if (!((selected >> bit) & 1))
	return 1;
    want = expected(bit);
    if (want == -1)
	return 1;
    return expect(bit,want,got,current_bit,current_high);
}


static int check(void)
{
    int i,ok = 1;

    for (i = 0; i != MAX_PINS; i++)
	if ((defined >> i) & 1)
	    set_value(i,values[i]);
    if (dry_run) {
	print_dry_run(selected);
	for (i = 0; i != MAX_PINS; i++)
	    if ((selected >> i) & 1)
		tests++;
	return 1;
    }
    commit(current_bit,current_high);
    for (i = 0; i != MAX_PINS/8; i++) {
	uint8_t bits = selected >> i*8;
	uint8_t got;
	int j;

	if (!bits)
	    continue;
	got = read_reg(PRTxDR(i));
	for (j = 0; j != 8; j++)
	    if ((bits >> j) & 1)
		if (!check_pin(i*8+j,(got >> j) & 1))
		    ok = 0;
    }
    check_target(current_bit,current_high);
    return ok;
}


/* ----- Test database ----------------------------------------------------- */

/*
 * We collect all configurations before actually doing any testing, so that we
 * can weed out redundant ones. Setups with lots of alternative branches can
 * produce quite a bit of redundancy.
 *
 * Note that we could probably do better, by also checking whether there are
 * configurations that are different but where one is more "demanding" than the
 * other (e.g., by making the "background" pins stronger or the "foreground"
 * pin weaker). In fact, the program is designed with this type of optimization
 * in mind. However, experimental evidence that this would be useful is still
 * lacking.
 *
 * Actually, there is value in keeping "inferior" tests, namely when we build
 * up "aggressiveness". E.g., driving a pin accidently shorted to ground with a
 * strong "1" may cause our supply voltage to collapse or even damage the chip,
 * so it's a good idea to try with something a little weaker, e.g. "1R", and to
 * proceed only if this one works. This is what the program does at the moment.
 */

/*
 * Since we don't have all the bells and whistles in place right now, we can do
 * the testing right when posting, and run_tests only drops the old data. Also,
 * "struct setup.expect" isn't used at the moment.
 *
 * Oh yes, and we're real speed demons with duplicate eliminiation taking a
 * cheerful O(n^2).
 */


static struct setup {
    uint8_t values[MAX_PINS];
    uint8_t expect[MAX_PINS]; /* unused */
    struct setup *next;
} *setups = NULL;


static int post(void)
{
    struct setup **walk;
    int i;

    for (walk = &setups; *walk; walk = &(*walk)->next) {
	for (i = 0; i != MAX_PINS; i++)
	    if ((*walk)->values[i] != values[i])
		break;
	if (i == MAX_PINS)
	    return 1;
    }
    patterns++;
    *walk = alloc_type(struct setup);
    memcpy((*walk)->values,values,sizeof(values));
    (*walk)->next = NULL;
    return check();
}


#if 0
static void clean_setups(void)
{
    while (setups) {
	struct setup *next;

	next = setups->next;
	free(setups);
	setups = next;
    }
}
#endif


/* ----- Test setup -------------------------------------------------------- */


static void remove_useless(void)
{
    int i;

    for (i = 0; i != MAX_PINS; i++)
	if (((selected >> i) & 1) && !allow[i])
	    selected &= ~((uint64_t) 1 << i);
}


static int setup(int current_strong)
{
    int i,work = 0;

    for (i = 0; i != MAX_PINS; i++) {
	if (!((selected >> i) & 1))
	    values[i] = VALUE_Z;
	else {
	    if (current_high) {
		if (weak_low(i))
		    work = 1;
	    }
	    else {
		if (weak_high(i))
		    work = 1;
	    }
	}
    }
    if (current_bit == -1)
	return work;
    if (current_high)
	return current_strong ?
	  strong_high(current_bit) : weak_high(current_bit);
    else
	return current_strong ?
	  strong_low(current_bit) : weak_low(current_bit);
}


/*
 * Set all pins to weakly oppose the single pin we're about to test, then check
 * that they indeed read back correctly. Failure to do so indicates shorts to
 * GND, Vdd, or outputs of external components.
 */

static int check_base(void)
{
    assert(current_bit == -1);
    return setup(0) ? post() : 1;
}


/*
 * Set the pin we're testing to a weak setting, then check that it reads back
 * properly. (Since we don't have a single-pin check at this point, we'll just
 * test all the pins.)
 *
 * Errors at this point indicate that something drives the pin in an unexpected
 * way.
 */

static int check_weak(void)
{
    assert(current_bit != -1);
    return setup(0) ? post() : 1;
}


/*
 * Set the pin to the strongest setting available, then check that none of the
 * other pins have changed in unexpected ways.
 *
 * This is the main test, which finds shorts and other weirness connecting
 * pins in unwanted ways.
 */

static int check_strong(void)
{
    assert(current_bit != -1);
    return setup(1) ? post() : 1;
}


/* ----- Iterate through all possible configurations ----------------------- */


struct continuation {
    const struct alternatives *next_alt;
    const struct continuation *next;
};


static int recurse(int (*fn)(void),const struct alternatives *alt,
  const struct continuation *cont)
{
    uint64_t old_selected = selected;
    struct choice *walk;
    int n = 0,ok = 1;

    while (!alt) {
	if (!cont) {
	    int res;

	    if (verbose > 1)
		fprintf(stderr,"*");
	    remove_useless();
	    res = fn();
	    selected = old_selected;
	    return res;
	}
	if (verbose > 1)
	    fprintf(stderr,"^");
	alt = cont->next_alt;
	cont = cont->next;
    }

    for (walk = alt->choices; walk; walk = walk->next) {
	struct continuation my_cont = {
	    .next_alt = alt->next,
	    .next = cont,
	};
	const struct alternatives *scan;
	int i;

	for (i = 0; i != MAX_PINS; i++)
	    if ((walk->defined >> i) & 1) {
		allow[i] = walk->allow[i];
		external[i] = walk->external[i];
	    }
	if (verbose > 1)
	    fprintf(stderr,"%d(",n);
	selected = old_selected | walk->defined;
	for (scan = walk->alternatives; scan; scan = scan->next)
	    selected &= ~scan->defined;
	if (!recurse(fn,walk->alternatives,&my_cont))
	    ok = 0;
	if (verbose > 1)
	    fprintf(stderr,")");
	n++;
    }
    selected = old_selected;
    return ok;
}


static int iterate(int (*fn)(void))
{
    const struct alternatives *alt;
    int res;

    selected = defined;
    for (alt = alternatives; alt; alt = alt->next)
	selected &= ~alt->defined;
    res = recurse(fn,alternatives,NULL);
    if (verbose > 1)
	fprintf(stderr,".\n");
    return res;
}


/* ----- Run the tests ----------------------------------------------------- */


void do_tests(void)
{
    if (!dry_run)
	init_ports();

    for (current_high = 0; current_high != 2; current_high++) {
	current_bit = -1;
	if (iterate(check_base))
	    for (current_bit = 0; current_bit != MAX_PINS; current_bit++)
		if ((defined >> current_bit) & 1) {
		    if (!quiet)
			current(current_bit,current_high);
		    if (iterate(check_weak))
			iterate(check_strong);
		}
    }

    finish();
}
