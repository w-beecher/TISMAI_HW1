#ifndef FINALCODE_H
#define FINALCODE_H

#include <stdlib.h>
#include <stdio.h>
#include "symtab.h"
#include "tree.h"
#include "tac.h"
#include "type.h"
#include "intercode.h"
#include "./hash/ht.h"

void codegen(struct tree *root, SymbolTable sym_root);
char *atos(struct addr *a);
char *atot(struct addr *a);
char *ttot(typeptr t);
void test_ic_list(struct tree *node);


#endif
