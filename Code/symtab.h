#ifndef SYMTAB_H
#define SYMTAB_H

#include <stdlib.h>
#include "tree.h"
//#include "tac.h"
#include "./hash/ht.h"
#include "type.h"

///////////
//structs//
///////////

typedef struct sym {
   struct sym_table *table; /* table this symbol belongs to */
   char *name;	             /* the symbol itself */
   int lex_type;            /* integer type code based on lexer */
   struct sym *next;        /* next item in scope */
   //symbol information
   struct typeinfo *type;
   struct addr *addr;
   struct addr *arrbase;
   int size;
} *Symbol;

typedef struct sym_table {
   int nEntries;			    /* # of symbols in the table */
   char *name;
   struct sym_table *parent;	/* enclosing scope, superclass etc. */
   struct ht *tbl;      /* list of entries in this scope; should be defined and resized as needed */
   // Linked list fields for children
   int nKids;
   struct sym_table *children; 
   struct sym_table *next; /* in case of linked list */
   struct instr *instructions; /* instruction information */
   int stacksize;
   // int LBL_COUNT;
} *SymbolTable;

//////////////
//prototypes//
//////////////

const char *get_rule(int rule);

void print_symtree(SymbolTable node, int depth);

SymbolTable scope(struct tree *root);
void populate(struct tree *node);
void make_built_ins(void);
Symbol make_entry_assign(struct tree *node);
Symbol make_entry_param(struct tree *node);

Symbol create_sym(int type, char *name);
SymbolTable create_symtab(char *name);
void scope_enter(char *name);
void scope_exit();
int scope_level();

void scope_bind(Symbol sym);
Symbol scope_lookup(char *name, SymbolTable current_scope);
Symbol scope_lookup_current(char *name, SymbolTable current_scope);
Symbol scope_lookup_global(char *name);

#endif
