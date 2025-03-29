#ifndef TREE_H
#define TREE_H

//things added on later
#define VAR 343

#include "type.h"
//#include "tac.h"

// "checked" allocation
void *ckalloc(int n);

//token structs
struct token {
   int category;   /* the integer code returned by yylex */
   char *text;     /* the actual string (lexeme) matched */
   int lineno;     /* the line number on which the token occurs */
   char *filename; /* the source file in which the token occurs */
   int bval;       //boolean
   int ival;       /* for integer constants, store binary value here */
   double dval;	   /* for real constants, store binary value here */
   char *sval;     /* for string constants, malloc space, de-escape, store */
                   /*    the string (less quotes and after escapes) here */
};
struct tokenlist {
      struct token *t;
      struct tokenlist *next;
};

//tree structs
struct tree {
    int id;  //for dot
    int prodrule;
    char *symbolname;
    int nkids;
    struct tree *kids[13];
    struct token *leaf;
    struct sym_table *scope;
    struct typeinfo *type; //type checking
    struct addr *addr; //code gen
    struct instr *icode; //code gen
    struct addr *first; //code gen
    struct addr *follow; //code gen
    struct addr *ontrue; //code gen
    struct addr *onfalse; //code gen
    struct addr *arrbase;

};

////////// ENUMS //////////

//non-production enum
enum productionrules {
    PR_CRATE = 10000,
    PR_EXP,
    PR_BOR,
    PR_BAN,
    PR_BEQ,
    PR_BNO,
    PR_EXPR,
    PR_TER,
    PR_FAC,
    PR_LITERALS,
    PR_TYPE_SPECIFIER,
    PR_STATEMENTLIST,
    PR_STATEMENT,
    PR_RET_STMT,
    PR_NT_STATEMENT,
    PR_T_STATEMENT,
    PR_ASSIGN_STM,
    PR_STATIC_ASSIGN_STM,
    PR_ARRAY,
    PR_ITEM_LIST,
    PR_ITEMS,
    PR_ARRAY_INDEX,
    PR_UPDATE_STM,
    PR_IF_TYPE_STM,
    PR_SINGLE_IF,
    PR_ELSE,
    PR_WHILE_STM,
    PR_FOR_STM,
    PR_FOR_EXPR,
    PR_MACRO,
    PR_FUNC,
    PR_INPUT_LIST,
    PR_INPUTS,
    PR_FUNCTION_DEF,
    PR_FN_PARAM_LIST,
    PR_FN_PARAMS,
    PR_FN_PARAM
};
//reverse prodrule enum
//non-production enum
static const char *const prodnames[] = {
    "crate",
    "exp",
    "bor",
    "ban",
    "beq",
    "bno",
    "expr",
    "ter",
    "fac",
    "literals",
    "type_specifier",
    "StatementList",
    "Statement",
    "ret_stmt",
    "NT_statement",
    "T_statement",
    "assign_stm",
    "static_assign_stm",
    "array",
    "item_list",
    "items",
    "array_index",
    "update_stm",
    "if_type_stm",
    "single_if",
    "else",
    "while_stm",
    "for_stm",
    "for_expr",
    "macro",
    "func",
    "input_list",
    "inputs",
    "function_def",
    "fn_param_list",
    "fn_params",
    "fn_param"
};

#endif