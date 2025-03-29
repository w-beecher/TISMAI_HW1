#include "intercode.h"
#include "tac.h"
#include "typecheck.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

extern const char *yyname(int cat);
int iter_items(struct tree *node, int count, int start);
struct instr *alloc_arr_item(struct tree *node, int offset);
int count_items(struct tree *node);
int iter_args(struct tree *node);
void paddr(struct tree *node);
void make_globals(struct tree *root, int flag);
void make_globals_litgen(struct tree *node);
int iter_items_glob(struct tree *node, int count, int start);
struct instr *alloc_arr_item_glob(struct tree *node, int offset);
extern int GLB_SIZE;

void intermediate_code(struct tree *root, SymbolTable symroot, FILE *file){
    assign_first(root);
    assign_follow(root);
    make_globals(root, 0);
    if(root->icode != NULL){
        struct instr *tmp = root->icode;
        root->icode = gen(D_GLOB, address("__fec_init_glb__", R_CONST, 0, NULL), address(NULL, R_CONST, GLB_SIZE, NULL), NULL);
        root->icode = append(root->icode, tmp);
        //root->icode = append(root->icode, gen(D_END, NULL, NULL, NULL));
    }
    assign_icode(root);
    tacprint(symroot, root, file);
}

void assign_first(struct tree *node){
    if(node == NULL) return;
    for(int i = 0; i < node->nkids; i++) assign_first(node->kids[i]);
    //I'm just giving firsts to identifiers (since these could be, like, var references idk)
    //and all non-terminal nodes.  This could be optimized but i dont have the time for that
    //this is truly the most optimal use of an if-else chain ever
    if(node->leaf != NULL){
        if(strcmp(yyname(node->leaf->category), "IDENT") == 0){
            node->first = genlabel();
        }
        else if(strcmp(yyname(node->leaf->category), "LIT_INTEGER") == 0){
            node->first = genlabel();
        }
        else if(strcmp(yyname(node->leaf->category), "TRUE") == 0){
            node->first = genlabel();
        }
        else if(strcmp(yyname(node->leaf->category), "FALSE") == 0){
            node->first = genlabel();
        }       
        else if(strcmp(yyname(node->leaf->category), "LIT_FLOAT") == 0){
            node->first = genlabel();
        }        
        else if(strcmp(yyname(node->leaf->category), "LIT_STR") == 0){
            node->first = genlabel();
        }
        else if(strcmp(yyname(node->leaf->category), "LIT_CHAR") == 0){
            node->first = genlabel();
        }  
    } else node->first = genlabel();
}

//this treats all function definitions as separate follow chains to allow for multiple code regions
//if that is not what ends up being needed, pass down a follow to the StatementList in function_def
void assign_follow(struct tree *node){
    if(node == NULL) return;

    //function definition statement
    if(strcmp(get_rule(node->prodrule), "function_def") == 0){
        //contains a return value loop
        if(node->nkids > 8){
            //the function body is empty: add a NOP to the symbol table.
            //this is for compatibility reasons for intermediate code & label placement
            if(node->kids[8] == NULL){
                node->follow = genlabel();
                node->icode = append(node->icode, gen(D_LABEL, node->first, NULL, NULL));
                node->icode = append(node->icode, gen(D_LABEL, node->follow, NULL, NULL));
                node->icode = append(node->icode, gen(NOP, NULL, NULL, NULL));
                node->scope->instructions = node->icode;
                return;
            }
            //there is a defined function body
            else {
                node->kids[8]->follow = genlabel();
                //additional NOP just to be safe
                //append(node->kids[8]->follow, gen(NOP, NULL, NULL, NULL));
            }
        }
        //function contains no return value
        //tl;dr insert a return 0
        else {
            if(node->kids[6] == NULL){
                node->follow = genlabel();
                node->icode = gen(D_LABEL, node->first, NULL, NULL);
                node->icode = append(node->icode, gen(D_LABEL, node->follow, NULL, NULL));
                node->icode = append(node->icode, gen(NOP, NULL, NULL, NULL));
                node->scope->instructions = node->icode;
                return;
            }
            else {
                node->kids[6]->follow = genlabel();
                //append(node->kids[6]->follow, gen(NOP, NULL, NULL, NULL));
            }
        }
    }

    //general case: statement, statementlist, etc, can just be chained
    //statementlist
    else if(strcmp(get_rule(node->prodrule), "StatementList") == 0){
        //no check for if this node has 0 children; handled automatically by NULL checks in func & control blocks
        //1 statements
        if(node->kids[1] == NULL) {
            node->kids[0]->follow = genlabel();
        }
        //2 statements
        else {
            node->kids[0]->follow = node->kids[1]->first;
        }
    }

    
    //terminating statement has semicolons; just pass the follow along since it doesn't matter here
    else if(strcmp(get_rule(node->prodrule), "T_statement") == 0) {
        //case 1: empty statement.  just skip this.
        if(node->kids[0]->prodrule == ';') return;
        //case 2: actual statement followed by a semicolon
        node->kids[0]->follow = node->follow;
    }

    //control blocks
    //general if: only appears if there is an else
    else if(strcmp(get_rule(node->prodrule), "if_type_stm") == 0){
        node->kids[0]->follow = node->kids[1]->first;
        node->kids[1]->follow = node->follow;
        //for later
        node->kids[0]->onfalse = node->follow;
    }
    //if
    else if(strcmp(get_rule(node->prodrule), "single_if") == 0){
        //the expression cannot be null, but statement body can
        //has a body
        if (node->kids[3] != NULL) {
            node->kids[1]->follow = node->follow;
            node->kids[1]->ontrue = node->kids[3]->first;
            node->kids[1]->onfalse = node->follow;
            node->kids[3]->follow = node->follow;
            //check for if/else
        }
        //no body
        else{
            node->kids[1]->follow = node->follow;
            node->kids[1]->ontrue = node->follow;
            node->kids[1]->onfalse = node->follow;
        }        
    }
    //else
    else if(strcmp(get_rule(node->prodrule), "else") == 0){
        //else statement
        if(node->nkids > 2){
            //has body
            if(node->kids[2] != NULL) node->kids[2]->follow = node->follow;
            //has no body
            else node->first = node->follow;
        }
        //else if statement
        else node->kids[1]->follow = node->follow;
    }
    //while
    else if(strcmp(get_rule(node->prodrule), "while_stm") == 0){
        //// handle body ////
        //has body
        if(node->kids[3] != NULL){
            node->kids[1]->ontrue = genlabel();
            node->kids[1]->onfalse = node->follow;
            node->kids[3]->follow = node->kids[1]->first;
        }
        //has none
        else {
            //this should be an error automatically but just in case
            node->kids[1]->ontrue = genlabel();
            node->kids[1]->onfalse = node->follow;
        }
    }
    //for (similar to while)
    else if(strcmp(get_rule(node->prodrule), "for_stm") == 0){
        //has a body
        if(node->kids[5] != NULL){
            node->kids[3]->ontrue = genlabel();
            node->kids[3]->onfalse = node->follow;
            //label used here; in icode generation, the increment statement must be added manually
            //the order is statement >> increment >> expression  
            node->kids[5]-> follow = genlabel();
        }
        //no body
        else {
            //in this case the icode increment will be added after the ontrue
            //order is expression >> increment >> expression
            node->kids[3]->ontrue = genlabel();
            node->kids[3]->onfalse = node->follow;
        }        
    }

    /////expressions////
    //don't need to check single-child options as it will cascade to an ident or literal

    //booleans
    else if(strcmp(get_rule(node->prodrule), "bor") == 0){
        node->kids[0]->ontrue = node->ontrue;
        node->kids[0]->onfalse = node->kids[2]->first;
        node->kids[2]->ontrue = node->ontrue;
        node->kids[2]->onfalse = node->onfalse;        
    }
    else if(strcmp(get_rule(node->prodrule), "ban") == 0){
        node->kids[0]->ontrue = node->kids[2]->first;
        node->kids[0]->onfalse = node->onfalse;
        node->kids[2]->ontrue = node->ontrue;
        node->kids[2]->onfalse = node->onfalse;
    }
    else if(strcmp(get_rule(node->prodrule), "bno") == 0){
        node->kids[1]->ontrue = node->onfalse;
        node->kids[1]->onfalse = node->ontrue;
    }
    //fac needs to be here because it needs to pass ontrue and false to children
    else if(strcmp(get_rule(node->prodrule), "fac") == 0){
        if((node->nkids > 2) && (node->ontrue != NULL)){
            node->kids[1]->ontrue = node->ontrue;
            node->kids[1]->onfalse = node->onfalse;
        }
    }

    //non-boolean expressions use neither first/follow nor onfalse/ontrue
    //this is because they need to be evaluated in the ICODE, creating a self-contained subtree.
    //given that such a process is recursive, it doesn't rely on labels because there are no jumps possible.
    //this includes relational operators (beq non-terminal)

    //recurse
    for(int i = 0; i < node->nkids; i++) assign_follow(node->kids[i]);
}

void assign_icode(struct tree *node){
    if(node == NULL) return;
    //so turns out in a for loop what happens is the variable is implicitly defined in the loop statement.
    //but a postorder traversal means the definition happens AFTER the definition of the loop body.  so that's an issue.
    //here's my workaround lol its a little fucked up but what can you do, really.
    if(strcmp(get_rule(node->prodrule), "for_stm") == 0){
        Symbol s = scope_lookup(node->kids[1]->leaf->text, node->scope);
        if(s->addr == NULL) s->addr = genlocal(node->scope, s->type);
    }
    //recurse
    //we truly do live in a post-order society
    for(int i = 0; i < node->nkids; i++) assign_icode(node->kids[i]);
    
    //ops
    //this is going to look VERY similar to assign_follow
    //this however needs to generate for every single possible statement rule and permutation smile

    //leaves
    if(node->leaf != NULL){
        //identifiers
        if(strcmp(yyname(node->leaf->category), "IDENT") == 0){
            //lookup
            Symbol s = scope_lookup(node->leaf->text, node->scope);
            //check if address has already been generated. if not, generate it.
            //if(s->addr == NULL) s->addr = genlocal(node->scope);
            node->addr = s->addr;
            node->icode = NULL;
            //could be an ident used in a contorl statement if the symbol is boolean
            if(s->type->basetype == BOOL_TYPE){
                if(node->ontrue != NULL) {
                    node->icode = append(node->icode, gen(O_BIF, node->addr, node->ontrue, NULL));
                    node->icode = append(node->icode, gen(O_GOTO, node->onfalse, NULL, NULL));
                }
            }
        }
        //literals; strings and floats need to bring an address in from the global region
        //string
        else if((strcmp(yyname(node->leaf->category), "LIT_STR") == 0) || strcmp(yyname(node->leaf->category), "LIT_CHAR") == 0){
            if(node->icode != NULL) return;
            struct addr *local = genlocal(node->scope, alctype(STRING_TYPE));
            node->icode = NULL;
            //node->icode = gen(D_LABEL, node->first, NULL, NULL);
            node->icode = append(node->icode, gen(O_ADDR, local, node->addr, NULL));
            node->addr = local;
        }
        //float
        else if(strcmp(yyname(node->leaf->category), "LIT_FLOAT") == 0){
            if(node->icode != NULL) return;
            struct addr *local = genlocal(node->scope, alctype(F64_TYPE));
            node->icode = NULL;
            //node->icode = gen(D_LABEL, node->first, NULL, NULL);
            node->icode = append(node->icode, gen(O_ADDR, local, node->addr, NULL));
            node->addr = local;
        }
        else if(strcmp(yyname(node->leaf->category), "LIT_INTEGER") == 0){
            node->addr = address(NULL, R_CONST, atoi(node->leaf->text), alctype(I64_TYPE));
            node->icode = NULL;
        }
        else if(strcmp(yyname(node->leaf->category), "TRUE") == 0){
            node->addr = address(NULL, R_CONST, 1, alctype(BOOL_TYPE));
            node->icode = NULL;
        }
        else if(strcmp(yyname(node->leaf->category), "FALSE") == 0){
            node->addr = address(NULL, R_CONST, 0, alctype(BOOL_TYPE));
            node->icode = NULL;
        }

        //well, it's possible that a control statement could be using this for boolean statements. check it.
        if((strcmp(yyname(node->leaf->category), "TRUE") == 0)  || (strcmp(yyname(node->leaf->category), "FALSE") == 0)){
            //if boolean is used in a control block or short circuit expression, make GOTOs
            if(node->ontrue != NULL) {
                node->icode = append(node->icode, gen(O_BIF, node->addr, node->ontrue, NULL));
                node->icode = append(node->icode, gen(O_GOTO, node->onfalse, NULL, NULL));
            }
        }
    }

    //array literals
    else if(strcmp(get_rule(node->prodrule), "array") == 0){
        //avoid redefinition of global var
        if(node->icode != NULL) return;
        //has values in it
        if(node->kids[1] != NULL){
            int count = count_items(node->kids[1]); 
            node->addr = genlocal_n(count, node->scope, node->type);
            //helper function which probably generates this array literal in main memory
            iter_items(node->kids[1], 0, node->addr->offset);
            //now generate a pointer to that memory region  
            //node->addr = genlocal(node->scope);
            node->icode = append(node->icode, node->kids[1]->icode);
            //node->icode = append(node->icode, gen(O_ADDR, node->addr, node->arrbase, NULL));
        }
        //no values.
        else node->addr = address(NULL, R_CONST, 0, alctype(NULL_TYPE));
    }

    //array index
    else if(strcmp(get_rule(node->prodrule), "array_index") == 0){
        //first, grab the symbol for the array base address
        Symbol s = scope_lookup(node->kids[0]->leaf->text, node->scope);
        int base = s->arrbase->offset;
        int region = s->addr->region;
        int typevalue = 0;
        if(s->type->u.arr.elemtype->basetype == I64_TYPE) typevalue = I64_TYPE;
        else if(s->type->u.arr.elemtype->basetype == F64_TYPE) typevalue = F64_TYPE;
        else typevalue = STRING_TYPE;
        //the offset is the integer which acts as the index
        //immediate mode: direct address is possible.
        if(strcmp(yyname(node->kids[2]->leaf->category), "LIT_INTEGER") == 0) {
            int offset = node->kids[2]->addr->offset * 8;
            struct addr *location = address(NULL, region, base + offset, alctype(typevalue));
            node->addr = location;
        }
        //non immediate mode: evaluate index
        else{
            //eval
            node->icode = append(node->icode, node->kids[2]->icode);
            //index pseudo instruction: bound pseudo-address, base address, index offset
            node->addr = address(NULL, region, base, alctype(typevalue));
            node->addr->index_offset = node->kids[2]->addr;
            node->icode = append(node->icode, gen(DEREF, s->arrbase, node->kids[2]->addr, NULL));
        }
    }

    //function calls
    else if(strcmp(get_rule(node->prodrule), "func") == 0){
        node->addr = genlocal(node->scope, node->type);
        char *name = node->kids[0]->leaf->text;
        //args:
        if(node->kids[2] != NULL){
            //helper function on argument list
            int argc = iter_args(node->kids[2]);
            node->icode = node->kids[2]->icode;
            node->icode = append(node->icode, gen(O_CALL, address(name, R_CONST, 0, NULL), address(NULL, R_CONST, argc, alctype(I64_TYPE)), node->addr));
        } 
        //no args: just call
        else node->icode = gen(O_CALL, address(name, R_CONST, 0, NULL), address(NULL, R_CONST, 0, alctype(I64_TYPE)), node->addr);
    }
    //macro calls (the same as function really)
    else if(strcmp(get_rule(node->prodrule), "macro") == 0){
        node->addr = genlocal(node->scope, node->type);
        char *name = node->kids[0]->leaf->text;
        //args:
        if(node->nkids > 2 && node->kids[3] != NULL){
            //helper function on argument list
            int argc = iter_args(node->kids[3]);
            node->icode = node->kids[3]->icode;
            node->icode = append(node->icode, gen(O_CALL, address(name, R_CONST, 0, NULL), address(NULL, R_CONST, argc, alctype(I64_TYPE)), node->addr));
        } 
        //no args: just call
        else node->icode = gen(O_CALL, address(name, R_CONST, 0, NULL), address(NULL, R_CONST, 0, alctype(I64_TYPE)), node->addr);
    }

    //unlike the other functions it helps me understand this one by starting at the bottom and going up
    else if(strcmp(get_rule(node->prodrule), "bor") == 0){
        node->icode = node->kids[0]->icode;
        node->icode = append(node->icode, gen(D_LABEL, node->kids[2]->first, NULL, NULL));
        node->icode = append(node->icode, node->kids[2]->icode);
    }
    else if(strcmp(get_rule(node->prodrule), "ban") == 0){
        node->icode = node->kids[0]->icode;
        node->icode = append(node->icode, gen(D_LABEL, node->kids[2]->first, NULL, NULL));
        node->icode = append(node->icode, node->kids[2]->icode);
    }
    else if(strcmp(get_rule(node->prodrule), "beq") == 0){
        //get operator;
        //the beq non-terminal supports all logical comparisons so there's a lot
        int base = NOP;
        if (strcmp(yyname(node->kids[1]->leaf->category), "EQEQ") == 0) base = O_BEQ;
        else if (strcmp(yyname(node->kids[1]->leaf->category), "LE") == 0) base = O_BLE;
        else if (strcmp(yyname(node->kids[1]->leaf->category), "GE") == 0) base = O_BGE;
        else if (strcmp(yyname(node->kids[1]->leaf->category), "NE") == 0) base = O_BNE;
        else if (node->kids[1]->prodrule == '<') base = O_BLT;
        else if (node->kids[1]->prodrule == '>') base = O_BGT;

        //icode ops:
        node->addr = genlocal(node->scope, alctype(BOOL_TYPE));
        node->icode = node->kids[0]->icode;
        node->icode = append(node->icode, node->kids[2]->icode);
        node->icode = append(node->icode, gen(base, node->addr, node->kids[0]->addr, node->kids[2]->addr));
        //if boolean is used in a control block or short circuit expression, make GOTOs
        if(node->ontrue != NULL) {
            node->icode = append(node->icode, gen(O_BIF, node->addr, node->ontrue, NULL));
            node->icode = append(node->icode, gen(O_GOTO, node->onfalse, NULL, NULL));
        }
    }
    else if(strcmp(get_rule(node->prodrule), "bno") == 0){
        //node->addr = genlocal(node->scope, alctype(BOOL_TYPE));
        node->icode = append(node->icode, node->kids[1]->icode);
        //if boolean is used in a control block or short circuit expression, make GOTOs
    }
    else if(strcmp(get_rule(node->prodrule), "exp") == 0){
        //4 cases: plus, minus, and floating point variants
        int off = (type_to_get(node->kids[0]->type)->basetype == I64_TYPE) ? 0 : 27;
        typeptr t = (off == 0) ? alctype(I64_TYPE) : alctype(F64_LIT_NP);
        int base = (node->kids[1]->prodrule == '+') ? O_ADD : O_SUB;
        //ops
        node->addr = genlocal(node->scope, t);
        node->icode = node->kids[0]->icode;
        node->icode = append(node->icode, node->kids[2]->icode);
        node->icode = append(node->icode, gen(base + off, node->addr, node->kids[0]->addr, node->kids[2]->addr));
    }
    else if(strcmp(get_rule(node->prodrule), "ter") == 0){
        int off = (type_to_get(node->kids[0]->type)->basetype == I64_TYPE) ? 0 : 27;
        typeptr t = (off == 0) ? alctype(I64_TYPE) : alctype(F64_LIT_NP);
        int base = NOP;
        switch(node->kids[1]->prodrule){
            case '*':
                base = O_MUL;
                break;
            case '/':
                base = O_DIV;
                break;
            case '%':
                base = O_MOD;
                break;
        }
        node->addr = genlocal(node->scope, t);
        node->icode = node->kids[0]->icode;
        node->icode = append(node->icode, node->kids[2]->icode);
        node->icode = append(node->icode, gen(base + off, node->addr, node->kids[0]->addr, node->kids[2]->addr));
    }
    else if(strcmp(get_rule(node->prodrule), "fac") == 0){
        // ( bor )
        if(node->nkids == 3){
            node->addr = node->kids[1]->addr;
            node->icode = node->kids[1]->icode;
        }
        // - bor
        else {
            int bt = (type_to_get(node->kids[0]->type)->basetype == I64_TYPE) ? I64_TYPE : F64_LIT_NP;
            node->addr = genlocal(node->scope, alctype(bt));
            node->icode = node->kids[1]->icode;
            node->icode = append(node->icode, gen(O_NEG, node->addr, node->kids[1]->addr, NULL));
        }
    }

    //assignment
    else if(strcmp(get_rule(node->prodrule), "assign_stm") == 0){
        //avoid redefinition of global var
        if(node->icode != NULL) return;
        int i = (strcmp(yyname(node->kids[1]->leaf->category), "IDENT") == 0) ? 1 : 2; 
        //new variable needs to set its stuff
        Symbol s = scope_lookup(node->kids[i]->leaf->text, node->scope);
        s->addr = genlocal(node->scope, s->type);
        node->kids[i]->addr = s->addr;
        node->addr = node->kids[i]->addr;
        //array assignment
        if(node->nkids > 7){
            //has a value set to it
            if(node->nkids > 9) {
                //evaluate literal
                node->icode = append(node->icode, node->kids[node->nkids - 1]->icode); 
                //set a pointer to the literal address
                node->icode = append(node->icode, gen(O_ADDR, node->addr, node->kids[node->nkids - 1]->addr, NULL));
                //set base address in symbol table for future use
                s->arrbase = node->kids[node->nkids - 1]->addr;
            }
            //does not have a value set to it, well pointer is already allocated so who cares
        }  
        //non-array assignment
        else {
            //has a value set to it
            if(node->nkids > 5){
                if(node->kids[i+4]->icode != NULL){
                    node->icode = node->kids[i+4]->icode;
                    node->icode = append(node->icode, gen(O_ASN, node->kids[i]->addr, node->kids[i+4]->addr, NULL));
                } else {
                    node->icode = gen(O_ASN, node->kids[i]->addr, node->kids[i+4]->addr, NULL);
                }
            }
            //it it does not have a value set to it (only declaring the variable), do nothing ig
            else node->icode = NULL;
        }
    }
    //update statements
    else if(strcmp(get_rule(node->prodrule), "update_stm") == 0){
        Symbol s = NULL;
        //if this is a variable and not an array index
        if(strcmp(get_rule(node->kids[0]->prodrule), "array_index") != 0) {
            //check for var definition 
            Symbol s = scope_lookup(node->kids[0]->leaf->text, node->scope);
            if(s->addr == NULL) {
                s->addr = genlocal(node->scope, s->type);
                node->kids[0]->addr = s->addr;
            }
        } 
        //if this is an array index, evaluate that
        else node->icode = append(node->icode, node->kids[0]->icode);
        //begin
        node->addr = node->kids[0]->addr;
        node->icode = append(node->icode, node->kids[2]->icode);
        //case 1: =
        if(node->kids[1]->prodrule == '=') {
            //array reassignment?
            if((s != NULL) && (s->type->basetype == ARRAY_TYPE)) {
                s->arrbase = node->kids[2]->addr;
                node->icode = append(node->icode, gen(O_ADDR, node->kids[0]->addr, node->kids[2]->addr, NULL));
            }
            //normal reassignment
            else node->icode = append(node->icode, gen(O_ASN, node->kids[0]->addr, node->kids[2]->addr, NULL));
        }
        //case 2: +=
        else if(strcmp(yyname(node->kids[1]->leaf->category), "PLUSEQ") == 0){
            int off = (type_to_get(node->kids[0]->type)->basetype == I64_TYPE) ? 0 : 27;
            int bt = (type_to_get(node->kids[0]->type)->basetype == I64_TYPE) ? I64_TYPE : F64_LIT_NP;
            struct addr *tmp = genlocal(node->scope, alctype(bt));
            node->icode = append(node->icode, gen(O_ADD + off, tmp, node->kids[0]->addr, node->kids[2]->addr));
            node->icode = append(node->icode, gen(O_ASN, node->kids[0]->addr, tmp, NULL));
        }
        //case 3: -= genlabel();
        else if(strcmp(yyname(node->kids[1]->leaf->category), "MINUSEQ") == 0){
            int off = (type_to_get(node->kids[0]->type)->basetype == I64_TYPE) ? 0 : 27;
            int bt = (type_to_get(node->kids[0]->type)->basetype == I64_TYPE) ? I64_TYPE : F64_LIT_NP;
            struct addr *tmp = genlocal(node->scope, alctype(bt));
            node->icode = append(node->icode, gen(O_SUB + off, tmp, node->kids[0]->addr, node->kids[2]->addr));
            node->icode = append(node->icode, gen(O_ASN, node->kids[0]->addr, tmp, NULL));
        }
    }

    //return
    else if(strcmp(get_rule(node->prodrule), "ret_stmt") == 0){
        //return an expression
        if(node->nkids > 2){
            node->icode = append(node->icode, node->kids[1]->icode);
            node->icode = append(node->icode, gen(O_RET, node->kids[1]->addr, NULL, NULL));
        }
        //return nothing (in this case just return 0)
        //change this if need be, like if you can actually return nothing in asm
        else node->icode = gen(O_RET, address(NULL, R_CONST, 0, alctype(I64_TYPE)), NULL, NULL);
    }

    //for loop
    else if(strcmp(get_rule(node->prodrule), "for_stm") == 0){
        //get expression
        struct tree *expr = node->kids[3];
        //get counter var
        struct tree *counter = node->kids[1];
        //start handling counter var: allocate and assign
        Symbol s = scope_lookup(counter->leaf->text, node->scope);
        if(s->addr == NULL) s->addr = genlocal(node->scope, s->type);
        counter->addr = s->addr;
        node->addr = counter->addr;
        counter->icode = append(expr->icode, gen(O_ASN, counter->addr, expr->kids[0]->addr, NULL));
        //start handling expression: 
        //first: symbol
        
        //next: evaluate bounds
        expr->icode = append(expr->icode, expr->kids[0]->icode);
        struct addr *lt = genlabel();
        struct addr *gt = genlabel();
        // bor .. bor
        if(expr->nkids == 3){
            expr->icode = append(expr->icode, expr->kids[2]->icode);
            //check greater than
            expr->icode = append(expr->icode, gen(D_LABEL, gt, NULL, NULL));
            struct addr *temp = genlocal(node->scope, alctype(BOOL_TYPE));
            expr->icode = append(expr->icode, gen(O_BGE, temp, counter->addr, expr->kids[0]->addr));
            expr->icode = append(expr->icode, gen(O_BIF, temp, lt, NULL));
            expr->icode = append(expr->icode, gen(O_GOTO, expr->onfalse, NULL, NULL));
            //check less than
            expr->icode = append(expr->icode, gen(D_LABEL, lt, NULL, NULL));
            expr->icode = append(expr->icode, gen(O_BLT, temp, counter->addr, expr->kids[2]->addr));
            expr->icode = append(expr->icode, gen(O_BIF, temp, expr->ontrue, NULL));
            expr->icode = append(expr->icode, gen(O_GOTO, expr->onfalse, NULL, NULL));

        }
        // bor .. =bor
        else{
            expr->icode = append(expr->icode, expr->kids[3]->icode);
            //check greater than
            expr->icode = append(expr->icode, gen(D_LABEL, gt, NULL, NULL));
            struct addr *temp = genlocal(node->scope, alctype(BOOL_TYPE));
            expr->icode = append(expr->icode, gen(O_BGE, temp, counter->addr, expr->kids[0]->addr));
            expr->icode = append(expr->icode, gen(O_BIF, temp, lt, NULL));
            expr->icode = append(expr->icode, gen(O_GOTO, expr->onfalse, NULL, NULL));
            //check less than
            expr->icode = append(expr->icode, gen(D_LABEL, lt, NULL, NULL));
            expr->icode = append(expr->icode, gen(O_BLE, temp, counter->addr, expr->kids[3]->addr));
            expr->icode = append(expr->icode, gen(O_BIF, temp, expr->ontrue, NULL));
            expr->icode = append(expr->icode, gen(O_GOTO, expr->onfalse, NULL, NULL));
        }
        //now handle the actual for statement:
        //first, set up the loop labels
        node->icode = append(node->icode, counter->icode);
        node->icode = append(node->icode, gen(D_LABEL, expr->first, NULL, NULL));
        node->icode = append(node->icode, expr->icode);
        node->icode = append(node->icode, gen(D_LABEL, expr->ontrue, NULL, NULL));
        //if the loop has a body, add it here.  if not, then don't.  obviously.
        if(node->kids[5] != NULL) node->icode = append(node->icode, node->kids[5]->icode); 
        //increment the counter var
        struct addr *increment = genlocal(node->scope, alctype(I64_TYPE));
        node->icode = append(node->icode, gen(O_ADD, increment, counter->addr, address(NULL, R_CONST, 1, alctype(I64_TYPE))));
        node->icode = append(node->icode, gen(O_ASN, counter->addr, increment, NULL));
        //go back to the start of the loop
        node->icode = append(node->icode, gen(O_GOTO, gt, NULL, NULL));
    }

    //while loop
    else if(strcmp(get_rule(node->prodrule), "while_stm") == 0){
        node->icode = append(node->icode, gen(D_LABEL, node->kids[1]->first, NULL, NULL));
        node->icode = append(node->icode, node->kids[1]->icode);
        node->icode = append(node->icode, gen(D_LABEL, node->kids[1]->ontrue, NULL, NULL));
        if(node->kids[3] != NULL) node->icode = append(node->icode, node->kids[3]->icode); 
        node->icode = append(node->icode, gen(O_GOTO, node->kids[1]->first, NULL, NULL));
    }

    //ifs
    else if(strcmp(get_rule(node->prodrule), "if_type_stm") == 0){
        node->icode = append(node->icode, node->kids[0]->icode);
        node->icode = append(node->icode, node->kids[1]->icode);
    }
    else if(strcmp(get_rule(node->prodrule), "single_if") == 0){
        //boolean
        node->icode = append(node->icode, node->kids[1]->icode);
        //jump
        //node->icode = append(node->icode, gen(O_BIF, node->kids[1]->addr, node->kids[1]->ontrue, NULL));
        //node->icode = append(node->icode, gen(O_GOTO, node->onfalse, NULL, NULL));
        //has body: add the statement after evaluating the expression
        if(node->kids[3] != NULL) node->icode = append(node->icode, node->kids[3]->icode);
        //if-else: add an extra goto to skip the else statement
        if(node->onfalse != NULL) node->icode = append(node->icode, gen(O_GOTO, node->onfalse, NULL, NULL));
    }
    else if(strcmp(get_rule(node->prodrule), "else") == 0){
        node->icode = append(node->icode, gen(D_LABEL, node->first, NULL, NULL));
        //single else statement
        if(node->nkids > 2){
            //has body
            if(node->kids[2] != NULL) node->icode = append(node->icode, node->kids[2]->icode);
        }
        //else if statement
        else node->icode = append(node->icode, node->kids[1]->icode);
    }

    //function definition
    else if(strcmp(get_rule(node->prodrule), "function_def") == 0){
        struct addr *proc = address(node->kids[1]->leaf->text, R_CONST, 0, NULL);
        struct addr *ssize = address(NULL, R_CONST, node->scope->stacksize, alctype(I64_TYPE));
        node->icode = append(node->icode, gen(D_PROC, proc, ssize, NULL));
        //has return value
        if(node->nkids > 8){
            //does this have a body?  if not, already handled by assign_follow().
            if(node->kids[8] != NULL){
                node->icode = append(node->icode, node->kids[8]->icode);
                //there might be things which point to the end of the function body
                node->icode = append(node->icode, gen(D_LABEL, node->kids[8]->follow, NULL, NULL));
                //if there are, add an additional nothing return and a nop, just to be safe.
                node->icode = append(node->icode, gen(O_RET, address(NULL, R_CONST, 0, alctype(I64_TYPE)), NULL, NULL));
                //append(node->icode, gen(NOP, NULL, NULL, NULL));
                //add this instruction set to the symbol table for this function
                node->scope->instructions = node->icode;
            }
        }
        //no return value
        else {
            //does this have a body?  if not, already handled by assign_follow().
            if(node->kids[6] != NULL){
                node->icode = append(node->icode, node->kids[6]->icode);
                //there might be things which point to the end of the function body
                node->icode = append(node->icode, gen(D_LABEL, node->kids[6]->follow, NULL, NULL));
                //if there are, add an additional nothing return and a nop, just to be safe.
                node->icode = append(node->icode, gen(O_RET, address(NULL, R_CONST, 0, alctype(I64_TYPE)), NULL, NULL));
                //append(node->icode, gen(NOP, NULL, NULL, NULL));
                //add this instruction set to the symbol table for this function
                node->scope->instructions = node->icode;
            }
        }
        node->icode = append(node->icode, gen(D_END, NULL, NULL, NULL));
    }
    //function parameter handling: allocate a local
    //due to how this is processed, the arguments are actually the first things in the local region, and should be in order!
    else if(strcmp(get_rule(node->prodrule), "fn_param") == 0){
        //new variable needs to set its stuff
        Symbol s = scope_lookup(node->kids[0]->leaf->text, node->scope);
        s->addr = genlocal(node->scope, s->type);
        node->kids[0]->addr = s->addr;
        node->addr = s->addr;
        node->icode = NULL;
    }

    //statement list
    else if(strcmp(get_rule(node->prodrule), "StatementList") == 0){
        //no check for if this node has 0 children; handled automatically by NULL checks in func & control blocks
        //2 statements: chain them
        node->icode = gen(D_LABEL, node->first, NULL, NULL);
        if(node->kids[1] != NULL) {
            node->icode = append(node->icode, node->kids[0]->icode);
            node->icode = append(node->icode, node->kids[1]->icode);
        }
        //1 statemen: make a label and append a NOP at the end
        else {
            node->icode = append(node->icode, node->kids[0]->icode);
            node->icode = append(node->icode, gen(D_LABEL, node->kids[0]->follow, NULL, NULL));
            //node->icode = append(node->icode, gen(NOP, NULL, NULL, NULL));
        }
    }
    //terminating statement 
    else if(strcmp(get_rule(node->prodrule), "T_statement") == 0) {
        //node->icode = gen(D_LABEL, node->first, NULL, NULL);
        //case 1: empty statement.  just skip this.
        if(node->kids[0]->prodrule == ';') {
            //node->icode = append(node->icode, gen(NOP, NULL, NULL, NULL));
            return;
        }
        //case 2: actual statement followed by a semicolon
        else {
            node->addr = node->kids[0]->addr;
            node->icode = append(node->icode, node->kids[0]->icode);
        }
    }

    else if(strcmp(get_rule(node->prodrule), "crate") == 0) {
        //fn_def crate
        if(node->nkids == 2){
            node->icode = append(node->icode, node->kids[0]->icode);
            node->icode = append(node->icode, node->kids[1]->icode);
        }
        //static assign present
        else node->icode = append(node->icode, node->kids[2]->icode);
    }

    //debug
    //paddr(node);
}

void paddr(struct tree *node){
    if(node->addr != NULL){
        if(node->addr->isname){
            printf("Name Addr: %s\n", node->addr->name);
        } else {
            printf("Data Addr: %d:%d\n", node->addr->region, node->addr->offset);
        }
    }
}

int count_items(struct tree *node){
    int count = 0;
    //3 cases:
    if(node->nkids == 3){
        //case 1: RECURSE ',' bor
        if(strcmp(get_rule(node->kids[0]->prodrule), "items") == 0){
            count = count_items(node->kids[0]);
            count++;
        }
        //case 2: bor ',' bor
        else if(node->kids[1]->prodrule == ',') count = 2;
    }
    //case 3: bor
    else count = 1;
    return count;
}

//, struct addr *address, 
//, int off
int iter_items(struct tree *node, int count, int start){
    //3 cases:
    int ret = count;
    //case 1: RECURSE ',' bor
    if((node->nkids == 3) && (strcmp(get_rule(node->kids[0]->prodrule), "items") == 0)){
        //recurse
        ret = iter_items(node->kids[0], count, start);
        node->icode = append(node->icode, node->kids[0]->icode);
        //handle other expression:
        node->icode = append(node->icode, alloc_arr_item(node->kids[2], start + 8 * (ret++)));
    }
    //case 2: bor ',' bor
    else if((node->nkids == 3) && (node->kids[1]->prodrule == ',')){
        //handle expressions
        node->icode = append(node->icode, alloc_arr_item(node->kids[0], start + 8 * (ret++)));
        node->icode = append(node->icode, alloc_arr_item(node->kids[2], start + 8 * (ret++)));
    }
    //case 3: bor
    else node->icode = append(node->icode, alloc_arr_item(node, start + 8 * (ret++)));
    //return
    return ret;
}

struct instr *alloc_arr_item(struct tree *node, int offset){
    node->icode = append(node->icode, gen(O_ASN, address(NULL, R_LOCAL, offset, node->type), node->addr, NULL));
    return node->icode;
}

int iter_args(struct tree *node){
    //returns a counter of the number of arguments
    int count = 0;
    struct instr *params = NULL;
    //1 arg (cascaded out of input non terminal)
    if(node->nkids == -1){
        params = append(params, node->icode);
        params = append(params, gen(O_PARM, node->addr, NULL, NULL));
        count ++;
    }
    //function inputs can be: arg // arg , arg // RECURSE , arg
    //1 arg
    else if(node->nkids == 1){
        params = append(params, node->kids[0]->icode);
        params = append(params, gen(O_PARM, node->kids[0]->addr, NULL, NULL));
        count ++;
    }
    //2 args
    else if(strcmp(get_rule(node->kids[0]->prodrule), "inputs") != 0){
        params = append(params, node->kids[0]->icode);
        params = append(params, node->kids[2]->icode);
        params = append(params, gen(O_PARM, node->kids[0]->addr, NULL, NULL));
        params = append(params, gen(O_PARM, node->kids[2]->addr, NULL, NULL));
        count += 2;
    } 
    //recurse
    else {
        count += iter_args(node->kids[0]);
        params = append(params, node->kids[0]->icode);
        params = append(params, node->kids[2]->icode);
        params = append(params, gen(O_PARM, node->kids[2]->addr, NULL, NULL));
        count ++;
    }
    //add instruction to the current node
    node->icode = params; 
    return count;
}

void make_globals(struct tree *root, int flag){
    //only worry about top-level global region
    if(strcmp(get_rule(root->prodrule), "crate") == 0){
        //check for specific case:
        //static_assign ';' crate
        if(root->nkids == 3){
            //symbol
            struct tree *node = root->kids[0];
            Symbol s = scope_lookup(node->kids[1]->leaf->text, node->scope);
            //eval
            make_globals_litgen(node->kids[node->nkids-1]);
            //check for array assignment
            if(node->nkids > 6){
                    node->icode = append(node->icode, node->kids[node->nkids-1]->icode); 
                    node->addr = genglobal(node->kids[node->nkids-1]->type);
                    node->icode = append(node->icode, gen(O_ADDR, node->addr, node->kids[node->nkids-1]->addr, NULL));
                    s->arrbase = node->kids[node->nkids-1]->addr;
            }
            //non-array assignment
            else {
                //non-immediate
                if(node->kids[node->nkids-1]->icode != NULL){
                    node->icode = append(node->icode, node->kids[node->nkids-1]->icode);
                    node->addr = node->kids[node->nkids-1]->addr;
                } 
                //immediate mode value
                else {
                    node->addr = genglobal(node->kids[node->nkids-1]->type);
                    node->icode = append(node->icode, gen(O_ASN, node->addr, node->kids[node->nkids-1]->addr, NULL));
                }
            }
            s->addr = node->addr;
            //recurse
            root->icode = append(root->icode, node->icode);
            make_globals(root->kids[2], 1);
            if(strcmp(get_rule(root->kids[2]->prodrule), "function_def") == 0){
                root->icode = append(root->icode, gen(D_END, NULL, NULL, NULL));
            }
            //root->icode = append(root->icode, root->kids[2]->icode);
        }
        else if(strcmp(get_rule(root->kids[0]->prodrule), "function_def") == 0){
                root->icode = append(root->icode, gen(D_END, NULL, NULL, NULL));
        }
    }
}

void make_globals_litgen(struct tree *node){
    if(node == NULL) return;
    if(node->leaf != NULL){
        //literals; strings and floats need to bring an address in from the global region
        //string
        if((strcmp(yyname(node->leaf->category), "LIT_STR") == 0) || strcmp(yyname(node->leaf->category), "LIT_CHAR") == 0){
            struct addr *a = genglobal(alctype(STRING_TYPE));
            node->icode = NULL;
            //node->icode = gen(D_LABEL, node->first, NULL, NULL);
            node->icode = append(node->icode, gen(O_ADDR, a, node->addr, NULL));
            node->addr = a;
        }
        //float
        else if(strcmp(yyname(node->leaf->category), "LIT_FLOAT") == 0){
            struct addr *a = genglobal(alctype(F64_TYPE));
            node->icode = NULL;
            //node->icode = gen(D_LABEL, node->first, NULL, NULL);
            node->icode = append(node->icode, gen(O_ADDR, a, node->addr, NULL));
            node->addr = a;
        }
        else if(strcmp(yyname(node->leaf->category), "LIT_INTEGER") == 0){
            node->addr = address(NULL, R_CONST, atoi(node->leaf->text), alctype(I64_TYPE));
            node->icode = NULL;
        }
        else if(strcmp(yyname(node->leaf->category), "TRUE") == 0){
            node->addr = address(NULL, R_CONST, 1, alctype(BOOL_TYPE));
            node->icode = NULL;
        }
        else if(strcmp(yyname(node->leaf->category), "FALSE") == 0){
            node->addr = address(NULL, R_CONST, 0, alctype(BOOL_TYPE));
            node->icode = NULL;
        }
    }
    //array literals
    else if(strcmp(get_rule(node->prodrule), "array") == 0){
        //has values in it
        if(node->kids[1] != NULL){
            int count = count_items(node->kids[1]); 
            node->addr = genglobal_n(count, node->type);
            //helper function which probably generates this array literal in main memory
            iter_items_glob(node->kids[1], 0, node->addr->offset);
            //now generate a pointer to that memory region  
            //node->addr = genlocal(node->scope);
            node->icode = append(node->icode, node->kids[1]->icode);
            //node->icode = append(node->icode, gen(O_ADDR, node->addr, node->arrbase, NULL));
            //struct addr *p = genglobal();
            //node->icode = append(node->icode, gen(O_ADDR, p, node->addr, NULL));
            //node->addr = p;
        }
        //no values.
        else node->addr = address(NULL, R_CONST, 0, alctype(WILDCARD_TYPE));
    }
    //recurse
    for(int i = 0; i < node->nkids; i++) make_globals_litgen(node->kids[i]);
}

int iter_items_glob(struct tree *node, int count, int start){
    //3 cases:
    int ret = count;
    //case 1: RECURSE ',' bor
    if((node->nkids == 3) && (strcmp(get_rule(node->kids[0]->prodrule), "items") == 0)){
        //recurse
        ret = iter_items_glob(node->kids[0], count, start);
        node->icode = append(node->icode, node->kids[0]->icode);
        //handle other expression:
        node->icode = append(node->icode, alloc_arr_item_glob(node->kids[2], start + 8 * (ret++)));
    }
    //case 2: bor ',' bor
    else if((node->nkids == 3) && (node->kids[1]->prodrule == ',')){
        //handle expressions
        node->icode = append(node->icode, alloc_arr_item_glob(node->kids[0], start + 8 * (ret++)));
        node->icode = append(node->icode, alloc_arr_item_glob(node->kids[2], start + 8 * (ret++)));
    }
    //case 3: bor
    else node->icode = append(node->icode, alloc_arr_item_glob(node, start + 8 * (ret++)));
    //return
    return ret;
}

struct instr *alloc_arr_item_glob(struct tree *node, int offset){
    node->icode = append(node->icode, gen(O_ASN, address(NULL, R_GLOBAL, offset, node->type), node->addr, NULL));
    return node->icode;
}