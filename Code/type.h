#ifndef TYPE_H
#define TYPE_H

#include "tree.h"
typedef struct param {
    struct typeinfo *type;
    struct param *next;
} *paramlist;

//base types
//as defined in tab.h:
    //I32 = 280,                     /* I32  */
    //I64 = 281,                     /* I64  */
    //F32 = 282,                     /* F32  */
    //F64 = 283,                     /* F64  */
    //STR = 284,                     /* STR  */
    //STRING = 285,                  /* STRING  */
    //BOOL = 286,                    /* BOOL  */
//enum not updated because i dont want to mess with this stuff sorry
enum basetypes {
    I32_TYPE = 1000000,
    I64_TYPE,
    F32_TYPE,
    F64_TYPE,
    STR_TYPE,
    STRING_TYPE,
    ARRAY_TYPE,
    FUNC_TYPE,
    BOOL_TYPE,
    NULL_TYPE,
    WILDCARD_TYPE,
    F64_LIT_NP,
    STRING_LIT_NP
};

typedef struct typeinfo {
    int basetype;
    int is_mut;
    union {
        /*  struct classinfo {
        *       struct sym_table *st;
        *   } c;
        */
        struct arrayinfo {
            int size;
            struct typeinfo *elemtype;
        }arr;
        struct funcinfo {
            struct sym_table *symtab;
            struct typeinfo *type;
            int nparams;
            struct param *params;
        }func;
    } u;
} *typeptr;

extern struct typeinfo integer_type;
extern struct sym_table *foo;

typeptr alctype(int);
char *get_type_name(typeptr t);

extern typeptr integer_typeptr;
extern typeptr null_typeptr;
extern typeptr String_typeptr;
extern char *typenam[];

#endif