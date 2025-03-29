#ifndef TYPECHECK_H
#define TYPECHECK_H

#include <stdlib.h>
#include "symtab.h"
#include "type.h"
#include "tree.h"
#include "./hash/ht.h"

void typecheck(struct tree *node);
bool compare_types(typeptr l, typeptr r);
bool check_type(typeptr t, int CAT);
bool check_returns(typeptr t, struct tree *stmt);
typeptr type_to_get(typeptr t);
int check_params(struct tree *node, typeptr type, struct token *leaf);
void check_array();
void type_array(typeptr arrtype, struct tree *item_list);
void unknown_type_error(struct tree *node);
void invalid_type_error(struct tree *node, char *allowed);

#endif