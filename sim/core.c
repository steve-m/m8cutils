/*
 * core.c - M8C core simulation
 *
 * Written 2006 by Werner Almesberger
 * Copyright 2006 Werner Almesberger
 */

/*
 * Note: the paging logic is not generic for the entire M8C family. What we
 * have here is the logic found in CY8C2xxxx chips.
 */


#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include "m8c.h"
#include "chips.h"
#include "disasm.h"
#include "file.h"

#include "util.h"
#include "reg.h"
#include "sim.h"
#include "int.h"
#include "core.h"


int interrupt_poll_interval = 1;


typedef uint16_t fast_math_type;

union fast_math {
    fast_math_type v16;
    uint8_t v8[sizeof(fast_math_type)];
};

/* ----- A ----------------------------------------------------------------- */


uint8_t a;


/* ----- T ----------------------------------------------------------------- */


static union fast_math t_union;

#ifdef LITTLE_ENDIAN
#define t		(t_union.v8[0])
#define t_overflow	(t_union.v8[1])
#else
#define t		(t_union.v8[sizeof(fast_math_type)-1])
#define t_overflow	(t_union.v8[sizeof(fast_math_type)-2])
#endif /* !LITTLE_ENDIAN */

#define t16		t_union.v16


/* ----- F ----------------------------------------------------------------- */


union f_union f_union;


static inline void cf_zf_t(void)
{
    cf = !!t_overflow;
    zf = !t;
}


/* ----- RAM --------------------------------------------------------------- */


uint8_t ram[2048]; /* maximum RAM in CY8C2xxxx parts */

static uint8_t *direct = ram;
static uint8_t *indexed = ram;
static uint8_t *mvi_read = ram;
static uint8_t *mvi_write = ram;
uint8_t *stack = ram;

static uint8_t cur_pp,stk_pp,idx_pp,mvr_pp,mvw_pp;


static void update_pages(void)
{
    switch (pgmode) {
	case 0:
	    direct = ram;
	    indexed = ram;
	    break;
	case 1:
	    direct = ram;
	    indexed = ram+(stk_pp << 8);
	    break;
	case 2:
	    direct = ram+(cur_pp << 8);
	    indexed = ram+(idx_pp << 8);
	    break;
	case 3:
	    direct = ram+(cur_pp << 8);
	    indexed = ram+(stk_pp << 8);
	    break;
	default:
	    abort();
    }
    stack = ram+(stk_pp << 8);
    mvi_read = ram+(mvr_pp << 8);
    mvi_write = ram+(mvw_pp << 8);
}


static void pp_write(struct reg *reg,uint8_t value)
{
    if (value > chip->pages)
	exception("%s: select page %d of %d",reg->name,value,chip->pages);
    *(uint8_t *) reg->user = value;
}


static const struct reg_ops pp_reg_ops = {
    .cpu_read = read_user,
    .cpu_write = pp_write,
};


static void pp_reg(int num,uint8_t *var)
{
    regs[num].ops = &pp_reg_ops;
    regs[num].user = var;
}


/* ----- ROM --------------------------------------------------------------- */


uint8_t rom[0x10000];


/* ----- PC, X, SP --------------------------------------------------------- */


uint16_t pc;
uint8_t x;
uint8_t sp;


/* ----- Addressing mode patterns ----- */


#define REGULAR_CPU_OPS(base,op,a)		\
    case base: /* op cpu_reg,expr */		\
	op((a),rom[pc]);			\
	pc++;					\
	break;					\
    case base+1: /* op cpu_reg,[expr] */	\
	op((a),direct[rom[pc]]);		\
	pc++;					\
	break;					\
    case base+2: /* op cpu_regreg,[X+expr] */	\
	op((a),indexed[(x+rom[pc]) & 0xff]);	\
	pc++;					\
	break

#define REGULAR_MEM_OPS(base,op)		\
    case base: /* op [expr],A */		\
	op(direct[rom[pc]],a);			\
	pc++;					\
	break;					\
    case base+1: /* op [X+expr],A */		\
	op(indexed[(x+rom[pc]) & 0xff],a);	\
	pc++;					\
	break;					\
    case base+2: /* op [expr],expr */		\
	op(direct[rom[pc]],rom[pc+1]);		\
	pc += 2;				\
	break;					\
    case base+3: /* op [X+expr],expr */		\
	op(indexed[(x+rom[pc]) & 0xff],rom[pc+1]); \
	pc += 2;				\
	break

#define REGULAR_OPS(base,op)			\
    REGULAR_CPU_OPS(base,op,a);			\
    REGULAR_MEM_OPS(base+3,op)

#define REG_OP(base,off)			\
    case base: /* * *,reg[*]* */		\
	tmp = (rom[pc]+(off)) & 0xff;		\
	if (xio)				\
	    tmp |= 0x100;
	
#define FROM_REG_OP(base,op,a,off_b)		\
	REG_OP(base,off_b);			\
	r8 = reg_read(regs+tmp);		\
	op(a,r8)

#define MOD_REG_OP(base,op,off_a,b)		\
	REG_OP(base,off_a);			\
	r8 = reg_read(regs+tmp);		\
	op(r8,b);				\
	reg_write(regs+tmp,r8)

#define TO_REG_OP(base,op,off_a,b)		\
	REG_OP(base,off_a);			\
	op(r8,b);				\
	reg_write(regs+tmp,r8)


/* ----- Arithmetic ----- */


#define ARITH(op,a,b)		\
    t16 = (a) op (b);		\
    (a) = t;			\
    cf_zf_t();

#define ADC(a,b) \
    ARITH(+,a,(b)+cf)

#define ADD(a,b) \
    ARITH(+,a,b)

#define SBB(a,b) \
    ARITH(-,a,(b)+cf)

#define SUB(a,b) \
    ARITH(-,a,b)

#define ARITH_OPS(base,op) \
    REGULAR_OPS(base,op)


/* ----- Bit operations ----- */


#define BIT(op,a,b) \
    (a) = (a) op (b); \
    zf = !(a)

#define AND(a,b) \
    BIT(&,a,b)

#define OR(a,b) \
    BIT(|,a,b)

#define XOR(a,b) \
    BIT(^,a,b)

#define BIT_OPS(base,op)			\
    REGULAR_OPS(base,op)

#define BIT_OPS_REG(base,op)			\
    MOD_REG_OP(base,op,0,rom[pc+1]);		\
	pc += 2;				\
	break;					\
    MOD_REG_OP(base+1,op,x,rom[pc+1]);		\
	pc += 2;				\
	break
    
#define BIT_OPS_F(base,op)			\
    case base: /* op F,expr */			\
	f = f op rom[pc++];			\
	update_pages();				\
	break


/* ----- Shift and rotate ----- */


#define ASL(a)			\
    cf = !!((a) & 0x80);	\
    (a) <<= 1;			\
    zf = !(a)

#define ASR(a)			\
    cf = (a) & 1;		\
    (a) = ((a) >> 1) | ((a) & 0x80); \
    zf = !(a)

#define RLC(a)			\
    tmp = cf;			\
    cf = !!((a) & 0x80);	\
    (a) = ((a) << 1) | tmp;	\
    zf = !(a)

#define RRC(a)			\
    tmp = cf;			\
    cf = (a) & 1;		\
    (a) = ((a) >> 1) | (tmp ? 0x80 : 0); \
    zf = !(a)

#define SHIFT_OPS(base,op)			\
    case base: /* op A */			\
	op(a);					\
	break;					\
    case base+1: /* op [expr] */		\
	op(direct[rom[pc]]);			\
	pc++;					\
	break;					\
    case base+2: /* op [X+expr] */		\
	op(indexed[(x+rom[pc]) & 0xff]);	\
	pc++;					\
	break


/* ----- Instructions >= 0x80 ----- */


#define CALL(offset) 				\
    stack[sp++] = pc >> 8;			\
    stack[sp++] = pc;				\
    pc = (pc+offset) & 0xffff

#define INDEX(offset)				\
    a = rom[(pc+offset+a) & 0xffff];		\
    zf = !a

#define JMP(offset)				\
    pc = (pc+offset-1) & 0xffff

#define JC(offset) \
    if (cf)					\
	JMP(offset)

#define JNC(offset) \
    if (!cf)					\
	JMP(offset)

#define JZ(offset) \
    if (zf)					\
	JMP(offset)

#define JNZ(offset) \
    if (!zf)					\
	JMP(offset)

#define JACC(offset)				\
    JMP((offset)+a)

#define OFFSET_OP(base,op)			\
    case base...base+15: /* op expr */		\
	offset = (rom[pc-1] & 0xf) << 8 | rom[pc]; \
	if (offset & 0x800)			\
	    offset |= 0xf000;			\
	pc += 1;				\
	op(offset);				\
	break


/* ----- CMP ----- */


#define CMP(a,b) \
    cf = (a) < (b); \
    zf = (a) == (b)

#define CMP_OPS(base,op)			\
    REGULAR_CPU_OPS(base,op,a);			\
    case base+3: /* op [expr],expr */		\
	op(direct[rom[pc]],rom[pc+1]);		\
	pc += 2;				\
	break;					\
    case base+4: /* op [X+expr],expr */		\
	op(indexed[(x+rom[pc]) & 0xff],rom[pc+1]); \
	pc += 2;				\
	break


/* ----- Increment and decrement ----- */


#define INC(a) \
    (a)++; \
    cf = zf = !(a)

#define DEC(a) \
    cf = !(a); \
    (a)--; \
    zf = !(a)

#define INC_OPS(base,op)			\
    case base: /* op A */			\
	op(a);					\
	break;					\
    case base+1: /* op X */			\
	op(x);					\
	break;					\
    case base+2: /* op [expr] */		\
	op(direct[rom[pc]]);			\
	pc++;					\
	break;					\
    case base+3: /* op [X+expr] */		\
	op(indexed[(x+rom[pc]) & 0xff]);	\
	pc++;					\
	break


/* ---- MOV ----- */

#define MOV_A(a,b) \
    (a) = (b); \
    zf = !(a)

#define MOV(a,b)  \
    (a) = (b)

#define MOV_REG_TO_A(base,off_b)		\
    FROM_REG_OP(base,MOV_A,a,off_b);		\
	pc++;					\
	break


/* ----- TST ----- */


#define TST(a,b) \
    zf = !((a) & (b))


/* ----- Code execution ---------------------------------------------------- */


static uint32_t m8c_check_interrupt(uint32_t cycles)
{
    if (!gie)
	return 0;
    int_poll();
    if (!int_vc)
	return 0;
    if (cycles && cycles < 13)
	return 0;
    int_handle();
    return 13;
}


static uint32_t m8c_prep(uint32_t cycles)
{
    uint8_t op;
    uint32_t tmp;

    op = rom[pc];
    tmp = m8c_cycles[op];
    if ((pc ^ (pc+m8c_bytes[op])) & 0xff00)
	tmp++;
    if (((unsigned) pc+m8c_bytes[op]) & ~0xffff) {
	exception("PC would wrap");
	return 0;
    }
    if (cycles && tmp > cycles)
	return 0;
    return tmp;
}


static int m8c_one(void)
{
    uint16_t offset;
    uint8_t r8;
    uint32_t tmp;

    switch (rom[pc++]) {
	case 0x00: /* SSC */		/* 0x00 */
	    exception("SSC");
	    return 1;
	ARITH_OPS(0x01,ADD);		/* 0x01-0x07 */
	case 0x08: /* PUSH A */		/* 0x08 */
	    stack[sp++] = a;
	    break;
	ARITH_OPS(0x09,ADC);  		/* 0x09-0x0f */
	case 0x10: /* PUSH X */		/* 0x10 */
	    stack[sp++] = x;
	    break;
	ARITH_OPS(0x11,SUB);  		/* 0x11-0x17 */
	case 0x18: /* POP A */		/* 0x18 */
	    a = stack[--sp];
	    zf = !a;
	    break;
	ARITH_OPS(0x19,SBB);  		/* 0x19-0x1f */
	case 0x20: /* POP X */		/* 0x20 */
	    x = stack[--sp];
	    break;
	BIT_OPS(0x21,AND);		/* 0x21-0x27 */
	case 0x28: /* ROMX */		/* 0x28 */
	    a = rom[(a << 8) | x];
	    zf = !a;
	    break;
	BIT_OPS(0x29,OR);		/* 0x29-0x2f */
	case 0x30: /* HALT */		/* 0x30 */
	    fflush(stdout);
	    fprintf(stderr,"HALT\n");
	    return 1;
	BIT_OPS(0x31,XOR);		/* 0x31-0x37 */
	case 0x38: /* ADD SP,expr */	/* 0x38 */
	    sp += rom[pc];
	    pc++;
	    break;
	CMP_OPS(0x39,CMP);		/* 0x39-0x3d */
	case 0x3e: /* MVI A,[expr] */	/* 0x3e */
	    /* @@@ is "direct" correct ? TRM is unclear */
	    a = mvi_read[direct[rom[pc++]]++];
	    zf = !a;
	    break;
	case 0x3f: /* MVI [expr],A */	/* 0x3f */
	    /* @@@ is "direct" correct ? TRM is unclear */
	    mvi_write[direct[rom[pc++]]++] = a;
	    break;
	case 0x40: /* NOP */		/* 0x40 */
	    break;
	BIT_OPS_REG(0x41,AND);		/* 0x41-0x42 */
	BIT_OPS_REG(0x43,OR);		/* 0x43-0x44 */
	BIT_OPS_REG(0x45,XOR);		/* 0x45-0x46 */
	case 0x47: /* TST [expr],expr *//* 0x47 */
	    zf = !(direct[rom[pc]] & rom[pc+1]);
	    pc += 2;
	    break;
	case 0x48: /* TST [X+expr],expr *//* 0x48 */
	    zf = !(indexed[(x+rom[pc]) & 0xff] & rom[pc+1]);
	    pc += 2;
	    break;
	FROM_REG_OP(0x49,TST,rom[pc+1],0); /* 0x49 */
	    pc += 2;
	    break;
	FROM_REG_OP(0x4a,TST,rom[pc+1],x); /* 0x4a */
	    pc += 2;
	    break;
	case 0x4b: /* SWAP A,X */	/* 0x4b */
	    r8 = x;
	    x = a;
	    a = r8;
	    zf = !a;
	    break;
	case 0x4c: /* SWAP A,[expr] */	/* 0x4c */
	    r8 = direct[rom[pc]];
	    direct[rom[pc++]] = a;
	    a = r8;
	    zf = !a;
	    break;
	case 0x4d: /* SWAP X,[expr] */	/* 0x4d */
	    r8 = direct[rom[pc]];
	    direct[rom[pc++]] = x;
	    x = r8;
	    break;
	case 0x4e: /* SWAP A,SP */	/* 0x4e */
	    r8 = sp;
	    sp = a;
	    a = r8;
	    zf = !a;
	    break;
	case 0x4f: /* MOV X,SP */	/* 0x4f */
	    x = sp;
	    break;
	REGULAR_CPU_OPS(0x50,MOV_A,a);	/* 0x50-0x52 */
	REGULAR_MEM_OPS(0x53,MOV);	/* 0x53-0x56 */
	REGULAR_CPU_OPS(0x57,MOV,x);	/* 0x57-0x59 */
	case 0x5a: /* MOV [expr],X */	/* 0x5a */
	    direct[rom[pc]] = x;
	    pc++;
	    break;
	case 0x5b: /* MOV A,X */	/* 0x5b */
	    a = x;
	    zf = !a;
	    break;
	case 0x5c: /* MOV X,A */	/* 0x5c */
	    x = a;
	    break;
	MOV_REG_TO_A(0x5d,0);		/* 0x5d */
	MOV_REG_TO_A(0x5e,x);		/* 0x5e */
	case 0x5f: /* MOV [expr],[expr] *//* 0x5f */
	    direct[rom[pc]] = direct[rom[pc+1]];
	    pc += 2;
	    break;
	 TO_REG_OP(0x60,MOV,0,a);	/* 0x60 */
	    pc++;
	    break;
	 TO_REG_OP(0x61,MOV,x,a);	/* 0x61 */
	    pc++;
	    break;
	 TO_REG_OP(0x62,MOV,0,rom[pc+1]); /* 0x62 */
	    pc += 2;
	    break;
	 TO_REG_OP(0x63,MOV,x,rom[pc+1]); /* 0x63 */
	    pc += 2;
	    break;
	SHIFT_OPS(0x64,ASL);		/* 0x64-0x66 */
	SHIFT_OPS(0x67,ASR);		/* 0x67-0x69 */	
	SHIFT_OPS(0x6a,RLC);		/* 0x6a-0x6c */	
	SHIFT_OPS(0x6d,RRC);		/* 0x6d-0x6f */	
	BIT_OPS_F(0x70,&);		/* 0x70 */
	BIT_OPS_F(0x71,|);		/* 0x71 */
	BIT_OPS_F(0x72,^);		/* 0x72 */
	case 0x73: /* CPL A */		/* 0x73 */
	    a = ~a;
	    zf = !a;
	    break;
	INC_OPS(0x74,INC);		/* 0x74-0x77 */
	INC_OPS(0x78,DEC);		/* 0x78-0x7b */
	case 0x7c: /* LCALL */		/* 0x7c */
	    stack[sp++] = (pc+2) >> 8;
	    stack[sp++] = pc+2;
	    /* fall through */
	case 0x7d: /* LJMP */		/* 0x7d */
	    pc = (rom[pc] << 8) | rom[pc+1];
	    break;
	case 0x7e: /* RETI */		/* 0x7e */
	    f = stack[--sp];
	    update_pages();
	    /* fall through */
	case 0x7f: /* RET */		/* 0x7f */
	    pc = stack[--sp];
	    pc += stack[--sp] << 8;
	    break;
	OFFSET_OP(0x80,JMP);		/* 0x80-0x8f */
	OFFSET_OP(0x90,CALL);		/* 0x90-0x9f */
	OFFSET_OP(0xa0,JZ);		/* 0xa0-0xaf */
	OFFSET_OP(0xb0,JNZ);		/* 0xb0-0xbf */
	OFFSET_OP(0xc0,JC);		/* 0xc0-0xcf */
	OFFSET_OP(0xd0,JNC);		/* 0xd0-0xdf */
	OFFSET_OP(0xe0,JACC);		/* 0xe0-0xef */
	OFFSET_OP(0xf0,INDEX);		/* 0xf0-0xff */
	default:
	    abort();
    }
    return 0;
}


uint32_t m8c_run(uint32_t cycles)
{
//fprintf(stderr,"pc = 0x%x\n",pc);
    int until_poll = interrupt_poll_interval;
    uint32_t done = 0;

    interrupted = 0;
    running = 1;
    while ((!cycles || cycles != done) && !interrupted) {
	uint32_t tmp;

	if (until_poll && !--until_poll) {
	    until_poll = interrupt_poll_interval;
	    tmp = m8c_check_interrupt(cycles-done);
	    if (tmp) {
		done += tmp;
		continue;
	    }
	}
	tmp = m8c_prep(cycles-done);
	if (!tmp)
	    break;
	done += tmp;
	if (m8c_one())
	    break;
    }
    running = 0;
    return done;
}


uint32_t m8c_step(void)
{
//fprintf(stderr,"pc = 0x%x\n",pc);
    uint32_t done;

    done = m8c_check_interrupt(0);
    if (done)
	return done;
    done = m8c_prep(0);
    if (!done)
	return 0;
    m8c_one();
    return done;
}


/* ----- CPU_F, CPU_SCR0, and CPU_SCR1 ------------------------------------- */


static uint8_t cpu_scr0 = CPU_SCR0_PORS;
static uint8_t cpu_scr1;


static uint8_t read_cpu_f(struct reg *reg)
{
    return f;
}


static void write_cpu_f(struct reg *reg,uint8_t value)
{
    if (value & 0x28)
	exception("CPU_F: trying to write 0x%02x",value);
    f = value;
    update_pages();
}


static const struct reg_ops cpu_f_ops = {
    .cpu_read = read_cpu_f,
    .cpu_write = forbidden_write,
    .sim_write = write_cpu_f,
};


static uint8_t read_cpu_scr0(struct reg *reg)
{
    return cpu_scr1 | (gie << 7);
}


static void write_cpu_scr0(struct reg *reg,uint8_t value)
{
    if (value & 0x46)
	exception("CPU_SCR0: trying to write 0x%02x",value);
    cpu_scr1 = (cpu_scr1 | ~0x30) & value & 0x7f;
}


static const struct reg_ops cpu_scr0_ops = {
    .cpu_read = read_cpu_scr0,
    .cpu_write = write_cpu_scr0,
};


static void write_cpu_scr1(struct reg *reg,uint8_t value)
{
    if (value & 0x62)
	exception("CPU_SCR1: trying to write 0x%02x",value);
    cpu_scr1 = value & 0x7f;
}


static const struct reg_ops cpu_scr1_ops = {
    .cpu_read = read_user,
    .cpu_write = write_cpu_scr1,
};


/* ----- CPU setup --------------------------------------------------------- */


void m8c_init(void)
{
    if (chip->pages > 1) {
	generic_reg(TMP_DR0,0,0xff);
	generic_reg(TMP_DR1,0,0xff);
	generic_reg(TMP_DR2,0,0xff);
	generic_reg(TMP_DR3,0,0xff);
    }
    pp_reg(CUR_PP,&cur_pp);
    pp_reg(STK_PP,&stk_pp);
    pp_reg(IDX_PP,&idx_pp);
    pp_reg(MVR_PP,&mvr_pp);
    pp_reg(MVW_PP,&mvw_pp);
    regs[CPU_F].ops = &cpu_f_ops;
    regs[CPU_F+0x100].ops = &cpu_f_ops;
    regs[CPU_SCR0].ops = &cpu_scr0_ops;
    regs[CPU_SCR0].user = &cpu_scr0;
    regs[CPU_SCR1].ops = &cpu_scr1_ops;
}
