/*
 * Three Address Code - skeleton for CSE 423
 */
#ifndef TAC_H
#define TAC_H

#include <stdlib.h>
#include <stdio.h>
#include "symtab.h"
#include "tree.h"
#include "type.h"

struct addr {
    int region;
    int isname;
    int offset;
    char *name;
    struct typeinfo *type;
    struct addr *index_offset;
};

/* Regions: */
#define R_GLOBAL 2001 /* can assemble as relative to the pc */
#define R_LOCAL  2002 /* can assemble as relative to the ebp */
#define R_CLASS  2003 /* can assemble as relative to the 'this' register */
#define R_LABEL  2004 /* pseudo-region for labels in the code region */
#define R_STRING 2005 /* pseudo-region for labels in the string (static) region */
#define R_FLOAT  2006 /* pseudo-region for labels in the string (static) region */
#define R_CONST  2007 /* pseudo-region for immediate mode constants */
//two thousand and late
#define R_INDEX  2008 /* pseudo-region for immediate mode constants */

struct instr {
   int opcode;
   struct addr *dest, *src1, *src2;
   struct instr *next;
};
/* Opcodes, per lecture notes */
#define O_ADD   3001
#define O_SUB   3002
#define O_MUL   3003
#define O_DIV   3004
#define O_NEG   3005
#define O_ASN   3006
#define O_ADDR  3007
#define O_LCONT 3008
#define O_SCONT 3009
#define O_GOTO  3010
#define O_BLT   3011
#define O_BLE   3012
#define O_BGT   3013
#define O_BGE   3014
#define O_BEQ   3015
#define O_BNE   3016
#define O_BIF   3017
#define O_BNIF  3018
#define O_PARM  3019
#define O_CALL  3020
#define O_RET   3021
/* declarations/pseudo instructions */
#define D_GLOB  3022
#define D_PROC  3023
#define D_LOCAL 3024
#define D_LABEL 3025
#define D_END   3026
//floating points & other ops
#define O_MOD   3027
#define O_FADD  3028
#define O_FSUB  3029
#define O_FMUL  3030
#define O_FDIV  3031
//idk
#define NOP     3032
//indicates that the address provided ought be used in dereference mode
//i.e., instead of `addr`, use `[addr]`
#define DEREF   3033

struct addr *address(char *name, int region, int offset, struct typeinfo *type);

void tacprint(SymbolTable symtab, struct tree *root, FILE *file);
void tacprint_test(struct instr *start, SymbolTable symtab);
int alloc_string(char *str);
struct addr *alloc_float(double d);
//void alloc_lable(char *str);
void add_global(int n);
struct addr *genglobal(struct typeinfo *type);
struct addr *genglobal_n(int n, struct typeinfo *type);
struct addr *genlocal(SymbolTable symtab, struct typeinfo *type);
struct addr *genlocal_n(int n, SymbolTable symtab, struct typeinfo *type);
struct addr *genlabel();
struct instr *gen(int, struct addr *, struct addr *, struct addr *);
struct instr *concat(struct instr *, struct instr *);
struct instr *copylist(struct instr *l);
struct instr *append(struct instr *l1, struct instr *l2);
struct instr *concat(struct instr *l1, struct instr *l2);

void ictest(SymbolTable s);


#endif