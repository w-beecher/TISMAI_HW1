#ifndef INTERCODE_H
#define INTERCODE_H

#include "tree.h"
#include "symtab.h"
#include <stdlib.h>
#include <stdio.h>

void intermediate_code(struct tree *root, SymbolTable symroot, FILE *file);
void assign_first(struct tree *node);
void assign_follow(struct tree *node);
void assign_icode(struct tree *node);

#endif