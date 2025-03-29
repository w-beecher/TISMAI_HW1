#include "typecheck.h"
#include "type.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

extern SymbolTable sym_root;
extern const char *yyname(int cat);
void pcheck(struct token *leaf);

void typecheck(struct tree *node){
    //post-order traverse
    if(node == NULL) return;
    for(int i = 0; i < node->nkids; i++) typecheck(node->kids[i]);
    //Lexer handles induvidual terminals in syntax tree
    
    //ensure references are typed
    if(node->leaf != NULL){
        //check for idents; if not present, will need to perform a search on the symbol table.
        if(strcmp(yyname(node->leaf->category), "IDENT") == 0){
            //if type is missing, must grab it from symbol table
            //printf("ident: %s\n", node->leaf->text); (debug)
            if(node->type == NULL){
                //code has been passed through scoping, so all idents should have a reference in the symbol table
                //just make sure to add it to the node's type in the syntax tree
                Symbol sym = scope_lookup(node->leaf->text, node->scope);
                node->type = sym->type;
            }
            return;
        }
    }
    //ensure arrays are typed
    else if(strcmp(get_rule(node->prodrule), "array") == 0){
        //arrays are treated as outer type containing inner arrtype field
        //inner type is for items in the array to differentiate operations
        node->type = alctype(ARRAY_TYPE);
        type_array(node->type, node->kids[1]);
    }
    //array indexing
    else if(strcmp(get_rule(node->prodrule), "array_index") == 0){
        //is this not an array? type error.
        if(!check_type(node->kids[0]->type, ARRAY_TYPE)){
            fprintf(stderr, "Line %d: non-array reference %s indexed as array\n", node->kids[0]->leaf->lineno, node->kids[0]->leaf->text);
            exit(3);
        }
        //is it not indexed with an integer value? also a type error
        //note that this works for variables with non-integer types
        if(!check_type(node->kids[2]->type, I64_TYPE)){
            fprintf(stderr, "Line %d: index on array reference %s is non-integer\n", node->kids[0]->leaf->lineno, node->kids[0]->leaf->text);
            exit(3);
        }
        
        //cannot check if the index is out of bounds at this point in time because nodes have not yet been evaluated.
        //this is delegated to a future pass
        
        //if no problems, set node type and move on
        node->type = type_to_get(node->kids[0]->type->u.arr.elemtype);
        return;
    }
    //ensure macro and functions are typed
    else if(strcmp(get_rule(node->prodrule), "func") == 0){
        //parameter type check
        if(check_params(node->kids[2], node->kids[0]->type, node->kids[0]->leaf) < node->kids[0]->type->u.func.nparams){
            fprintf(stderr, "Line %d: function %s has too few params\n", node->kids[0]->leaf->lineno, node->kids[0]->leaf->text);
            exit(3);
        }
        //update node type
        node->type = type_to_get(node->kids[0]->type);
        return;
    }
    //function return value checking 
    else if(strcmp(get_rule(node->prodrule), "function_def") == 0){
        //helper on the statement list, which is the 2nd to last item
        //first: is there no function body
        if(node->kids[node->nkids-3] == NULL){
            if(node->kids[1]->type->u.func.type->basetype != NULL_TYPE){
                fprintf(stderr, "Line %d: function %s missing returned value\n", node->kids[0]->leaf->lineno, node->kids[1]->leaf->text);
                exit(3);
            }
        } 
        //second: if there is a function body, process it
        else {
            //check_returns checks all the return values.
            //if this is false, that means that no returns were encountered.
            if(!check_returns(node->kids[1]->type->u.func.type, node->kids[node->nkids-2])){
                //in this case, ensure that the function does not need to return
                if(node->kids[1]->type->u.func.type->basetype != NULL_TYPE){
                    fprintf(stderr, "Line %d: function %s missing returned value\n", node->kids[0]->leaf->lineno, node->kids[1]->leaf->text);
                    exit(3);
                }
            }
        }
        return;
    }
    else if (strcmp(get_rule(node->prodrule), "macro") == 0){
        //parameter type check
        if(check_params(node->kids[3], node->kids[0]->type, node->kids[0]->leaf) < node->kids[0]->type->u.func.nparams){
            fprintf(stderr, "Line %d: function %s has too few params\n", node->kids[0]->leaf->lineno, node->kids[0]->leaf->text);
            exit(3);
        }
        //update node type
        node->type = type_to_get(node->kids[0]->type);
        return;
    }
    //assignment statements
    else if((strcmp(get_rule(node->prodrule), "assign_stm") == 0) || (strcmp(get_rule(node->prodrule), "static_assign_stm") == 0)){
        //check type matches definition
        int i = 0;
        for(; i < node->nkids; i++) if(strcmp(yyname(node->kids[i]->leaf->category), "IDENT") == 0) break;
        //non-array
        if(node->kids[i+2]->prodrule != '[') {
            //is there no starting value
            if(i+4 >= node->nkids) return;
            //if there is a starting value, type check it
            if(!compare_types(node->kids[i]->type, node->kids[i+4]->type)) invalid_type_error(node->kids[i+4], get_type_name(node->kids[i]->type));
        }
        //array
        else {
            //is there no starting value
            if(i+8 >= node->nkids) return;
            //if there is, type check
            if(!compare_types(node->kids[i]->type, node->kids[i+8]->type)) invalid_type_error(node->kids[i+8], get_type_name(node->kids[i]->type));
        }
    }
    //update statements
    else if(strcmp(get_rule(node->prodrule), "update_stm") == 0){
        //both arrays and non array updates are the same syntactically so only one case is present.
        if(!check_type(node->kids[0]->type, ARRAY_TYPE)){
            if(!compare_types(node->kids[0]->type, node->kids[2]->type)) invalid_type_error(node->kids[2], get_type_name(node->kids[0]->type));
        }
    }
    //control statements
    //if and while require boolean expressions; for requires own expression handling branches
    else if(strcmp(get_rule(node->prodrule), "if_stm") == 0){
        if(!check_type(node->kids[1]->type, BOOL_TYPE)) invalid_type_error(node->kids[1], "BOOL");
    }
    else if(strcmp(get_rule(node->prodrule), "while_stm") == 0){
        if(!check_type(node->kids[1]->type, BOOL_TYPE)) invalid_type_error(node->kids[1], "BOOL");
    }
    else if(strcmp(get_rule(node->prodrule), "for_expr") == 0){
        //form: FOR IDENT IN bor .. (=)? bor { StatementList }
        //ensure both bor are integers
        if(!check_type(node->kids[0]->type, I64_TYPE)) invalid_type_error(node->kids[0], "I64");
        if(node->kids[2]->prodrule != '='){
            if(!check_type(node->kids[2]->type, I64_TYPE)) invalid_type_error(node->kids[2], "I64");
        } else {
            if(!check_type(node->kids[3]->type, I64_TYPE)) invalid_type_error(node->kids[3], "I64");
        }
    }

    ///////////////////
    /// EXPRESSIONS ///
    ///////////////////
    //expression list: bor ban beq bno exp ter fac
    //|| and && have the same handling as they are both boolean
    else if((strcmp(get_rule(node->prodrule), "bor") == 0) || (strcmp(get_rule(node->prodrule), "ban") == 0)){
        if(node->nkids > 1){
            //BOR || BAN or BAN && BEQ or BEQ (logic op) BNO; the handling is the same either way
            if(!check_type(node->kids[0]->type, BOOL_TYPE)) invalid_type_error(node->kids[0], "BOOL");
            if(!check_type(node->kids[2]->type, BOOL_TYPE)) invalid_type_error(node->kids[2], "BOOL");
        }
        node->type = alctype(BOOL_TYPE);
        return;
    }
    else if(strcmp(get_rule(node->prodrule), "beq") == 0) {
        if(node->nkids > 1){
            //BEQ (logic op) BNO: fec allows this to be performed on any type - including strings - so long as the two match
            //match for vectors follows C rules
            if(!compare_types(node->kids[0]->type, node->kids[2]->type)) invalid_type_error(node->kids[2], get_type_name(node->kids[0]->type));
        }
        node->type = alctype(BOOL_TYPE);
        return;
    }
    else if(strcmp(get_rule(node->prodrule), "bno") == 0) {
        if(node->nkids > 1){
            // form: ! BNO
            if(!check_type(node->kids[1]->type, BOOL_TYPE)) invalid_type_error(node->kids[1], "BOOL");
        }
        node->type = alctype(BOOL_TYPE);
        return;
    }
    //+ - and * / % have the same handling (aside from type restrictions)
    else if((strcmp(get_rule(node->prodrule), "exp") == 0) || (strcmp(get_rule(node->prodrule), "ter") == 0)) {
        //valid only on I64 and F64, where both operands match
        if(node->nkids > 1){
            //first check for % operator: mod requires ints
            if(node->kids[1]->prodrule == '%'){
                if(!check_type(node->kids[0]->type, I64_TYPE)) invalid_type_error(node->kids[0], "I64");
                if(!check_type(node->kids[2]->type, I64_TYPE)) invalid_type_error(node->kids[2], "I64");
            } 
            //other operations aren't integer limited
            else if(check_type(node->kids[0]->type, I64_TYPE)){
                //check for match
                if(!check_type(node->kids[2]->type, I64_TYPE)) invalid_type_error(node->kids[2], "I64");
                node->type = alctype(I64_TYPE);
            }

            else if(check_type(node->kids[0]->type, F64_TYPE)){
                //check for match
                if(!check_type(node->kids[2]->type, F64_TYPE)){
                    invalid_type_error(node->kids[2], "F64");
                } 
                node->type = alctype(F64_TYPE);
            }
            else invalid_type_error(node->kids[0], "I64 or F64");
        }
        else node->type = type_to_get(node->kids[0]->type);
        return;
    }
    else if(strcmp(get_rule(node->prodrule), "fac") == 0) {
        if(node->nkids > 2){
            // ( BOR ), cascading up BOR's type
            if(node->kids[0]-> leaf != NULL) node->type = type_to_get(node->kids[1]->type);
            // FAC . FUNC or FAC . MACRO, grabbing return value type
            else node->type = type_to_get(node->kids[3]->type);
        }
        else if(node->nkids > 1){
            // - FAC
            if(!check_type(node->kids[1]->type, F64_TYPE) && !check_type(node->kids[1]->type, I64_TYPE)){
                invalid_type_error(node->kids[1], "I64 or F64");
                if(check_type(node->kids[1]->type, FUNC_TYPE))
                node->type = type_to_get(node->kids[1]->type);
            }
        }
        else node->type = type_to_get(node->kids[0]->type);
        return;
    }


}

//these functions need to recurse down until they find a line number
//throw the node on which there is an error

void unknown_type_error(struct tree *node){
    if(node->leaf == NULL){
        for(int i = 0; i < node->nkids; i++){
            if(compare_types(node->type, node->kids[i]->type)) unknown_type_error(node->kids[i]);
        }
        return;
    }
    fprintf(stderr, "Line %d: Unknown type in expression\n", node->leaf->lineno);
    exit(3);
}

void invalid_type_error(struct tree *node, char *allowed){
    if(node->leaf == NULL){
        //check for arrays because it causes problems
        if(strcmp(get_rule(node->prodrule), "array") == 0) {
            fprintf(stderr, "Line %d: Expected %s but received %s\n", node->kids[0]->leaf->lineno, allowed, get_type_name(type_to_get(node->type)));
            exit(3);
        }
        //recursive
        for(int i = 0; i < node->nkids; i++){
            if(compare_types(node->type, node->kids[i]->type)) invalid_type_error(node->kids[i], allowed);
        }
        return;
    }
    fprintf(stderr, "Line %d: Expected %s but received %s\n", node->leaf->lineno, allowed, get_type_name(type_to_get(node->type)));
    exit(3);
}

//helpers

typeptr type_to_get(typeptr t){
    if(t->basetype == FUNC_TYPE) return t->u.func.type;
    return t;
}

bool compare_types(typeptr l, typeptr r){
    if((l == NULL) || (r == NULL)) return false;
    //function handling
    typeptr left = (l->basetype == FUNC_TYPE) ? l->u.func.type : l;
    typeptr right = (r->basetype == FUNC_TYPE) ? r->u.func.type : r;
    //wildcard
    if((l->basetype == WILDCARD_TYPE) || (r->basetype == WILDCARD_TYPE)) return true;
    //array handling
    if(left->basetype == ARRAY_TYPE){
        if(right->basetype != ARRAY_TYPE) return false;
        if(left->u.arr.size != right->u.arr.size){
            fprintf(stderr, "Incompatible Array Sizes\n");
            return false;
        }
        if((left->u.arr.elemtype->basetype == WILDCARD_TYPE) || (right->u.arr.elemtype->basetype == WILDCARD_TYPE)) return true;
        if(left->u.arr.elemtype->basetype != right->u.arr.elemtype->basetype){
            fprintf(stderr, "Incompatible Array Base Element Types\n");
            return false;
        }
        return true;
    }
    //normal type handling
    else return left->basetype == right->basetype;
}

bool check_type(typeptr t, int CAT){
    //function handling
    if(CAT == FUNC_TYPE) return (t->basetype == FUNC_TYPE);
    typeptr target = (t->basetype == FUNC_TYPE) ? t->u.func.type : t;
    //wildcard
    if((t->basetype == WILDCARD_TYPE) || (CAT == WILDCARD_TYPE)) return true;
    //array handling
    if(target->basetype == ARRAY_TYPE){
        //if(CAT != ARRAY_TYPE) return false;
        //if(target->u.arr.elemtype->basetype != CAT) return false;
        return CAT == ARRAY_TYPE;
    }
    //normal type handling
    else return (target->basetype) == CAT;
}

int check_params(struct tree *node, typeptr type, struct token *leaf){
    //no param check
    if(node == NULL){
        if(type->u.func.nparams != 0){
            fprintf(stderr, "Line %d: function %s has too few params\n", leaf->lineno, leaf->text);
            exit(3);
        } else return 0;
    }
    //pre-order traverse
    int count = 1;
    paramlist check = type->u.func.params;
    //base case; this will be format: BOR (though BOR may be an array index)
    if((!(node->nkids > 1)) || (strcmp(get_rule(node->prodrule), "array_index") == 0)){
        //check first param
        if(check == NULL) pcheck(leaf);
        if(!compare_types(node->type, check->type)){
            fprintf(stderr, "Line %d: param #%d of function %s: ", leaf->lineno, count, leaf->text);
            fprintf(stderr, "Expected %s but received %s\n", get_type_name(check->type), get_type_name(type_to_get(node->type)));
            exit(3);
        }
        return count;
    }
    //recursion; this will be format: (prior iteration) , BOR
    else if(strcmp(get_rule(node->kids[0]->prodrule), "inputs") == 0){
        //get the current parameter number and grab that one from the linked list
        count = check_params(node->kids[0], type, leaf)+1;
        for(int i = 0; i < count-1; i++){
            check = check->next;
        }
        //check this paramater
        if(check == NULL) pcheck(leaf);
        if(!compare_types(node->kids[2]->type, check->type)){
            fprintf(stderr, "Line %d: param #%d of function %s: ", leaf->lineno, count, leaf->text);
            fprintf(stderr, "Expected %s but received %s\n", get_type_name(check->type), get_type_name(type_to_get(node->kids[2]->type)));
            exit(3);
        }
        return count;
    }
    //base case 2; this will be format: BOR , BOR
    else if(node->nkids > 2){
        //check first param
        if(check == NULL) pcheck(leaf);
        if(!compare_types(node->kids[0]->type, check->type)){
            fprintf(stderr, "Line %d: param #%d of function %s: ", leaf->lineno, count, leaf->text);
            fprintf(stderr, "Expected %s but received %s\n", get_type_name(check->type), get_type_name(type_to_get(node->kids[2]->type)));
            exit(3);
        }
        count += 1;
        check = check->next;
        //check first param
        if(check == NULL) pcheck(leaf);
        if(!compare_types(node->kids[2]->type, check->type)){
            fprintf(stderr, "Line %d: param #%d of function %s: ", leaf->lineno, count, leaf->text);
            fprintf(stderr, "Expected %s but received %s\n", get_type_name(check->type), get_type_name(type_to_get(node->kids[2]->type)));
            exit(3);
        }
        return count;
    } else {
        //idk lol something weird would have happened
        return -1;
    }
}

void pcheck(struct token *leaf){
    fprintf(stderr, "Line %d: Function %s has too many params\n", leaf->lineno, leaf->text);
    exit(3);
}

void type_array(typeptr arrtype, struct tree *item_list){
    //null array
    if(item_list == NULL){
        arrtype->u.arr.size = 0;
        //the only instance of wildcard, since an empty array can go wherever really
        arrtype->u.arr.elemtype = alctype(WILDCARD_TYPE);
    }
    //1 element array
    if(item_list->leaf != NULL){
        arrtype->u.arr.size = 1;
        arrtype->u.arr.elemtype = type_to_get(item_list->type);
    }
    //base case 1: BOR
    else if(item_list->nkids == 1){
        arrtype->u.arr.size = 1;
        arrtype->u.arr.elemtype = type_to_get(item_list->kids[0]->type);
    }
    //recursive case: (prior iteration) , BOR
    else if(strcmp(get_rule(item_list->kids[0]->prodrule), "items") == 0){
        //recurse
        type_array(arrtype, item_list->kids[0]);
        //typecheck
        arrtype->u.arr.size += 1;
        if(!compare_types(arrtype->u.arr.elemtype, item_list->kids[2]->type)){
            fprintf(stderr, "Array Literal Contains Inconsistent Types\n");
            invalid_type_error(item_list->kids[2], get_type_name(arrtype->u.arr.elemtype));
        }
    }
    //base case 2: BOR , BOR
    else {
        arrtype->u.arr.size = 2;
        arrtype->u.arr.elemtype = type_to_get(item_list->kids[0]->type);
        if(!compare_types(arrtype->u.arr.elemtype, item_list->kids[2]->type)){
            invalid_type_error(item_list->kids[2], get_type_name(arrtype->u.arr.elemtype));
        }
    }
}

bool check_returns(typeptr t, struct tree *stmt){
    //base cases
    if(stmt == NULL) return false;
    if(stmt->leaf != NULL) return false;
    //check for return statement
    if(strcmp(get_rule(stmt->prodrule), "ret_stmt") == 0){
        //RETURN bor ';'
        if(stmt->nkids > 2){
            if(!compare_types(t, stmt->kids[1]->type)) invalid_type_error(stmt->kids[1], get_type_name(t)); 
        }
        //RETURN ';'
        else if(t->basetype != NULL_TYPE) {
            fprintf(stderr, "Line %d: return statement missing value\n", stmt->kids[0]->leaf->lineno);
            exit(3);
        }
        //no errors
        return true;
    }
    //recurse
    bool flag = false;
    for(int i = 0; i < stmt->nkids; i ++) {
        if(check_returns(t, stmt->kids[i])) flag = true;
    }
    return flag;
}
