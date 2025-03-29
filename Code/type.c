#include <stdio.h>
#include <stdlib.h>
#include "type.h"
#include "symtab.h"

//these are statically allocated type info things because they contain no dynamic info
//just throw a pointer at them as needed in allocation
//struct typeinfo i32_type = { I32_TYPE };
//struct typeinfo i64_type = { I64_TYPE };
//struct typeinfo null_type = { NULL_TYPE };
//typeptr null_typeptr = &null_type;
//typeptr i32_typeptr = &i32_type;
//typeptr i64_typeptr = &i64_type;
//typeptr String_typeptr;

typeptr alctype(int base){
    typeptr rv;
    //these are actually useless because of mutability
    //if (base == I32_TYPE) return i32_typeptr;
    //else if (base == I64_TYPE) return i64_typeptr;
    rv = (typeptr) calloc(1, sizeof(struct typeinfo));
    if (rv == NULL){
        fprintf(stderr, "out of memory for request of %d bytes\n", 1);
        exit(4);
    }
    rv->basetype = base;
    //mutability defaults to 0 due to calloc
    return rv;
}

//name dereferencing
char *typenam[] = {
    "I32",
    "I64",
    "F32",
    "F64",
    "STR",
    "STRING",
    "ARRAY",
    "FUNC",
    "BOOL",
    "NULL",
    "WILDCARD",
    "F64_LIT_NP",
    "STRING_LIT_NP"
};
//function
char *get_type_name(typeptr t){
    if (!t) return "(NULL)";
    else if(t->basetype == -1) return "EVAL REQUIRED";
    else return typenam[t->basetype-1000000];
}
