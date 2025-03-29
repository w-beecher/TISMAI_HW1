#include "symtab.h"
#include "type.h"
#include "tac.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

//stack pointer
SymbolTable symtab_top = NULL;
//function counter for arg tracking
Symbol curr_func = NULL;
//global symbol table pointer
SymbolTable sym_root = NULL;
extern const char *yyname(int cat);
extern int SYMOUT;
int blockcount = 1;

char *make_block_name(char *type);

/////////////////////////
///AST Scope Traversal///
/////////////////////////

//return root on success
SymbolTable scope(struct tree *root){
    scope_enter("global");
    sym_root = symtab_top;
    //before population, add built-in functions
    //currently only supports println! and read!
    make_built_ins();
    //populate performs both static scope and mutability checks, and generates a visibility tree at sym_root
    populate(root);
    scope_exit();
    if(SYMOUT) print_symtree(sym_root, 0);
    return sym_root;
}

//this is just a helper function so is not neccessarily documented lol
void print_symtree(SymbolTable node, int depth){
    printf("%*s--- Symbol table for: %s ---\n", 3*depth, "", node->name);
    //print out current symbol table contents via iterator function
    hti table = ht_iterator(node->tbl);
    while(ht_next(&table)){
        char *tn = (((Symbol)table.value)->type == NULL) ? "null" : get_type_name(((Symbol)table.value)->type); 
        if(strcmp(tn, "FUNC") == 0) {
            printf("%*s%s: %s (%s)  //  ", 3*depth, "", ((Symbol)table.value)->name, tn, get_type_name(((Symbol)table.value)->type->u.func.type));
            printf("NParams: %d\n", ((Symbol)table.value)->type->u.func.nparams);
        }
        else if(((Symbol)table.value)->type->is_mut) printf("%*s%s: %s (Mutable)\n", 3*depth, "", ((Symbol)table.value)->name, tn);
        else printf("%*s%s: %s\n", 3*depth, "", ((Symbol)table.value)->name, tn);
    }
    //recurse for all children (nested scopes)
    if(node->nKids > 0){
        printf("\n");
        SymbolTable clist = node->children;
        while(clist != NULL){
            print_symtree(clist, depth + 1);
            clist = clist->next;
        }
    }
    //end scope line
    printf("%*s---\n\n", 3*depth, "");
}

void populate(struct tree *node){
    //PREORDER TRAVERSAL: epsilon rules in grammar leave null nodes
    if(node == NULL) return;
    
    //first, add pointer to current scope to the node we're looking at!
    node->scope = symtab_top;
    //printf("Rule: %s\n", get_rule(node->prodrule)); (debug)
    
    /*
        1) rules which cause a symbol creation: assign_stm, static_assign_stm, function_def
        2) rules which create a new scope, recurse, and exit scope after recurse returns: function_def, single_if, else, while_stm. 
        3) rules which reference a symbol: update_stm, IDENT
    */

    ///////////////////
    //symbol creation//
    ///////////////////
    if((strcmp(get_rule(node->prodrule), "assign_stm") == 0) || (strcmp(get_rule(node->prodrule), "static_assign_stm") == 0)){
        make_entry_assign(node);
    }
    /////////////
    //functions//
    /////////////
    else if(strcmp(get_rule(node->prodrule), "function_def") == 0){
        //adds to current scope and then generates a new scope
        Symbol func = make_entry_assign(node);
        scope_enter(func->name);
        //current func is a pointer to the innermost function.  ignores block statements like if or while.
        curr_func = func;

        //we've entered a new scope: overwrite the old one
        node->scope = symtab_top;
    }
    ////////////////////////
    ///Macros (Built-ins)///
    ////////////////////////
    
    //removed; macro processing delegated to future passes
    
    /*else if(strcmp(get_rule(node->prodrule), "macro") == 0){
        //macros have unique handling due to bang (!) being its own lex token and not wrapped into the name
        //get name
        //char *macro_name = ckalloc(strlen(node->kids[0]->leaf->text) + 2);
        char *macro_name = node->kids[0]->leaf->text;
        //strcpy(macro_name, node->kids[0]->leaf->text);
        //strcat(macro_name, "!");
        //search in the symbol table for the function -- i beleive the only built-ins are println! and read!, but still.
        if(scope_lookup(macro_name, symtab_top) == NULL){
            fprintf(stderr, "Unknown macro %s at line %d\n", macro_name, node->kids[0]->leaf->lineno);
            exit(3);
        } 
        //now, for future use, update the identifier's name.  this is so that macros are handled appropriately.
        //this could be avoided if macros are treated as normal symbols but whatever right!!
        //node->kids[0]->leaf->text = ckalloc(strlen(macro_name) + 1);
        //strcat(node->kids[0]->leaf->text, macro_name);
        //return
        return;
    }*/
    
    
    //////////////////
    //scope creation//
    //////////////////
    
    
    //if
    else if(strcmp(get_rule(node->prodrule), "single_if") == 0){
        //enter new scope
        scope_enter(make_block_name("IF_BLOCK_"));
        node->scope = symtab_top;
        //control processing not in the scope (ha ha) of symbol table
    }
    //else
    else if(strcmp(get_rule(node->prodrule), "else") == 0) {
        //enter new scope
        scope_enter(make_block_name("ELSE_BLOCK_"));
        node->scope = symtab_top;
    }
    //while
    else if(strcmp(get_rule(node->prodrule), "while_stm") == 0) {
        //enter new scope
        scope_enter(make_block_name("WHILE_BLOCK_"));
        node->scope = symtab_top;
        //technically not a syntax error but convenient time for a simple semantics check
        if(node->kids[3] == NULL){
            fprintf(stderr, "While loop at line %d has empty body, creating an infinite loop\n", node->kids[0]->leaf->lineno);
            exit(3);
        }
    }
    //for
    else if(strcmp(get_rule(node->prodrule), "for_stm") == 0) {
        //enter new scope
        scope_enter(make_block_name("FOR_BLOCK_"));
        node->scope = symtab_top;
        //add a new variable:
        Symbol sym = ckalloc(sizeof(struct sym));
        sym->table = symtab_top;
        sym->lex_type = I64_TYPE;
        sym->addr = 0;
        sym->size = 8;
        char *tn = node->kids[1]->leaf->text;
        sym->name = ckalloc(strlen(tn) + 1);
        strcpy(sym->name, tn);
        sym->type = alctype(I64_TYPE);
        sym->type->is_mut = 1;
        scope_bind(sym);
    }
    ////////////////////////////////////
    //symbol references and evaluation//
    ////////////////////////////////////
    
    //evaluation is currently unimplemented
    //this would require creation of additional functions for post-order traversal

    //first-pass static checks:
    //terminal (reference checking)
    else if(node->leaf != NULL){
        //identifier
        if(strcmp(yyname(node->leaf->category), "IDENT") == 0){
            //unknown reference?
            if(scope_lookup(node->leaf->text, symtab_top) == NULL){
                fprintf(stderr, "Unknown Variable reference %s at line %d\n", node->leaf->text, node->leaf->lineno);
                exit(3);
            }
        }
        else if(node->prodrule == '}'){
            scope_exit();
        }
        //other terminal sub-cases go here, if needed
    }
    //variable update statement (mutability checking)
    else if(strcmp(get_rule(node->prodrule), "update_stm") == 0){
        Symbol updated = NULL;
        //array index update
        if(strcmp(get_rule(node->kids[0]->prodrule), "array_index") == 0){
            //check *array* ident name
            struct tree *arr = node->kids[0]->kids[0];
            updated = scope_lookup(arr->leaf->text, symtab_top);
        }
        //normal update
        else {
            //get ident name (always child 0 for this type of statement)
            updated = scope_lookup(node->kids[0]->leaf->text, symtab_top);
        }
        //symbol may not exist; may as well handle the unknown reference while we're here
        if(updated == NULL){
            fprintf(stderr, "Unknown Variable reference %s at line %d\n", node->kids[0]->leaf->text, node->kids[0]->leaf->lineno);
            exit(3);
        } 
        //if the symbol exists, ensure it is mutable
        else if(updated->type->is_mut == 0){
            fprintf(stderr, "Non-Mutable variable %s is modified at line %d\n", node->kids[0]->leaf->text, node->kids[0]->leaf->lineno);
            exit(3);
        }
    }

    /////////////
    //BASE CASE//
    /////////////
    
    //efficient for symtable only work (no eval): recurse only on crate, StatementList, Statement, NT_statement, T_statement, 
    //additionally, recurse for fn_param_list, fn_params, and make an entry for fn_param

    //fn_param handling
    else if(strcmp(get_rule(node->prodrule), "fn_param") == 0) make_entry_param(node);

    //traverse entire tree (Eval)
    for(int i = 0; i < node->nkids; i++){
        populate(node->kids[i]);
    }
    //if none of the above options, this is not a terminal that matters for the purposes of symbol table generation
}

//hard-coded symbol table entries for built-in macros
void make_built_ins(void){
    //----- println! -----//
    Symbol s1 = ckalloc(sizeof(struct sym));
    s1->table = symtab_top;
    s1->lex_type = -1;
    s1->name = "println";
    s1->type = alctype(FUNC_TYPE);
    s1->size = 8;
    s1->type->u.func.symtab = symtab_top;
    s1->type->u.func.type = alctype(NULL_TYPE);
    s1->type->u.func.nparams = 2;
    s1->addr = NULL;
    paramlist newparam = ckalloc(sizeof(struct param));
    newparam->type = alctype(STRING_TYPE);
    newparam->next = ckalloc(sizeof(struct param));
    newparam->next->type = alctype(WILDCARD_TYPE);
    newparam->next->next = NULL;
    s1->type->u.func.params = newparam;
    scope_bind(s1);
    //----- read! -----//
    Symbol s2 = ckalloc(sizeof(struct sym));
    s2->table = symtab_top;
    s2->lex_type = -1;
    s2->name = "read";
    s2->type = alctype(FUNC_TYPE);
    s2->size = 8;
    s2->type->u.func.symtab = symtab_top;
    s2->type->u.func.type = alctype(I64_TYPE);
    s2->type->u.func.nparams = 1;
    s2->addr = NULL;
    newparam = ckalloc(sizeof(struct param));
    newparam->type = alctype(STRING_TYPE);
    newparam->next = NULL;
    s2->type->u.func.params = newparam;
    scope_bind(s2);
    //----- format! -----//
    Symbol s3 = ckalloc(sizeof(struct sym));
    s3->table = symtab_top;
    s3->lex_type = -1;
    s3->name = "format";
    s3->type = alctype(FUNC_TYPE);
    s3->size = 8;
    s3->type->u.func.symtab = symtab_top;
    s3->type->u.func.type = alctype(STRING_TYPE);
    s3->type->u.func.nparams = 3;
    s3->addr = NULL;
    newparam = ckalloc(sizeof(struct param));
    newparam->type = alctype(STRING_TYPE);
    newparam->next = ckalloc(sizeof(struct param));
    newparam->next->type = alctype(WILDCARD_TYPE);
    newparam->next->next = ckalloc(sizeof(struct param));
    newparam->next->next->type = alctype(WILDCARD_TYPE);
    newparam->next->next->next = ckalloc(sizeof(struct param));
    s3->type->u.func.params = newparam;
    scope_bind(s3);
}

//called on a node, and creates a binding in the current scope for the first ident encountered
//this works for both functions and variable declarations, but does not handle function parameters
//returns the name of the token
Symbol make_entry_assign(struct tree *node){
    Symbol sym = ckalloc(sizeof(struct sym));
    sym->table = symtab_top;
    sym->lex_type = -1;
    sym->size = 8;
    //find first ident and grab it
    char *tmp = NULL;
    int i = 0;
    int line = 0;
    for(; i < node->nkids; i++){
        if(strcmp(yyname(node->kids[i]->leaf->category), "IDENT") == 0){
            tmp = node->kids[i]->leaf->text;
            line = node->kids[i]->leaf->lineno;
            break;
        }
    }
    sym->name = ckalloc(strlen(tmp) + 1);
    strcpy(sym->name, tmp);
    sym->addr = NULL;
    //check for redefine in local:
    if(scope_lookup_current(tmp, symtab_top) != NULL){
        fprintf(stderr, "Variable %s at line %d is being redefined in current scope\n", tmp, line);
        exit(3);
    }
    //check for redefine in upper:
    if(scope_lookup(tmp, symtab_top) != NULL){
        fprintf(stderr, "Variable %s at line %d is being redefined from enclosing scope\n", tmp, line);
        exit(3);
    }
    //check for redefine in global:
    if(scope_lookup_global(tmp) != NULL){
        fprintf(stderr, "Variable %s at line %d redefines a global variable\n", tmp, line);
        exit(3);
    }

    ////////////////////////////////
    //check for type specification//
    ////////////////////////////////
    //non-array
    if((node->kids[i+1]->prodrule == ':') && (node->kids[i+2]->prodrule != '[')) {
        //as defined in tab.h file:
            //I32 = 280,                     /* I32  */
            //I64 = 281,                     /* I64  */
            //F32 = 282,                     /* F32  */
            //F64 = 283,                     /* F64  */
            //STR = 284,                     /* STR  */
            //STRING = 285,                  /* STRING  */
        //this field was not removed from initial tree implementation because it provides disambiguations between datastype sizing
        sym->lex_type = node->kids[i+2]->prodrule;
        //descriptive data structure -- this contains more detailed type info
        //I32 = 280, I32_TYPE = 1000000; to convert one to the other just add 999720
        //patch for bools smile
        if(sym->lex_type == 286) sym->type = alctype(BOOL_TYPE);
        else sym->type = alctype(node->kids[i+2]->prodrule+999720);
    }    
    //bounds check
    else if(i+2 < node->nkids){
        //array
        if((node->kids[i+2] != NULL) && (node->kids[i+2]->prodrule == '[')){
            //array's type goes here as before
            sym->lex_type = node->kids[i+3]->prodrule;
            sym->type = alctype(ARRAY_TYPE);
            //im just going to assume that arrays are a normal
            sym->type->u.arr.elemtype = alctype(node->kids[i+3]->prodrule+999720);
            //for the size, it is technically possible that this is not specified as an integer.
            //this handling here supports that, and marks this as -1 for later use.
            //this may need to be handled by adding the tree node itself into the symbol info, which would allow this to be resolved later during eval
            if(strcmp(yyname(node->kids[i+5]->prodrule), "LIT_INTEGER") == 0){
                sym->type->u.arr.size = atoi(node->kids[i+5]->leaf->text);
            } else {
                sym->type->u.arr.size = -1;
            }
        }
        //handle functions
        else {
            sym->type = alctype(FUNC_TYPE);
            sym->type->u.func.symtab = symtab_top;
            //look for return type
            int flag = 0;
            int j = i;
            for(; j < node->nkids; j++){
                if(node->kids[j] == NULL) continue;
                if(node->kids[j]->leaf == NULL) continue;
                if(strcmp(yyname(node->kids[j]->leaf->category), "RARROW") == 0){
                    flag = 1;
                    break;
                }
            }
            //if return type was found, it will be j + 1
            if(flag) sym->type->u.func.type = alctype(node->kids[j+1]->prodrule+999720);
            else     sym->type->u.func.type = alctype(NULL_TYPE);
        }
    }
    //mutability: only valid for vars defined in format LET MUT IDENT ... -- that is, is child 1 is MUT
    if((node->kids[1] != NULL) && (node->kids[1]->leaf != NULL) && (strcmp(yyname(node->kids[1]->leaf->category), "MUT") == 0)){
        sym->type->is_mut = 1;
    }
    //symbol is read at this point.

    //add to symbol table.  this function i beleive handles existing lookup already.
    scope_bind(sym);
    //add type info to node
    node->type = sym->type;
    //return
    return sym;
}

//called on a function parameter. format is IDENT : TYPE, always (unless there are references or mutables in which case perish)
Symbol make_entry_param(struct tree *node){
    Symbol sym = ckalloc(sizeof(struct sym));
    sym->table = symtab_top;
    sym->lex_type = -1;
    sym->type = NULL;
    //8 bytes for a pointer
    sym->size = 8;
    sym->addr = NULL;
    typeptr param_type = NULL;
    //grab the ident and copy the name to the structure
    char *tn = node->kids[0]->leaf->text;
    sym->name = ckalloc(strlen(tn) + 1);
    strcpy(sym->name, tn);
    //check for type specification. 
    //note this MAY be an array type; format is ident : type // ident : [type;size]
    //non-array
    if(node->kids[2]->prodrule != '[') {
        sym->lex_type = node->kids[2]->prodrule;
        //type struct info
        //descriptive data structure -- this contains more detailed type info
        //I32 = 280, I32_TYPE = 1000000; to convert one to the other just add 999720
        param_type = alctype(node->kids[2]->prodrule+999720);
    } 
    //array
    else {
        //array's type goes here as before
        sym->lex_type = node->kids[3]->prodrule;
        //struct, as before, but with additional union wrapper to insert both type and size
        param_type = alctype(ARRAY_TYPE);
        //im just going to assume that arrays are a normal
        param_type->u.arr.elemtype = alctype(node->kids[3]->prodrule+999720);
        //size will always be defined as an integer
        param_type->u.arr.size = atoi(node->kids[5]->leaf->text);
    }
    sym->type = param_type;
    //this goes in a wrapper
    paramlist newentry = ckalloc(sizeof(struct param));
    newentry->type = param_type;
    newentry->next = NULL;
    //update function type info: number of params and param list.
    curr_func->type->u.func.nparams++;
    paramlist tmp = curr_func->type->u.func.params;
    //if there is a param list defined for the function already, add this one to the end
    if(tmp != NULL){
        //get the last param in the list
        while(tmp->next != NULL){
            tmp = tmp->next;
        }
        //update the pointer in the last param
        tmp->next = newentry;
    } 
    //first function param: set the function's pointer.
    else { 
        curr_func->type->u.func.params = newentry;
    }

    //add to symbol table. 
    scope_bind(sym);
    //add type info to node
    node->kids[0]->type = sym->type;
    //return
    return sym;
}




////////////////////
//Helper functions//
////////////////////




const char *get_rule(int rule){
    if(rule < 10000) return yyname(rule);
    return prodnames[rule-10000];
}

//create a symbol
Symbol create_sym(int type, char *name){
    Symbol s = ckalloc(sizeof(struct sym));
    s->lex_type = type;
    s->name = ckalloc(strlen(name) + 1);
    strcpy(s->name, name);
    s->next = NULL;
    s->table = NULL;
    s->type = NULL;
    s->addr = NULL;
    s->arrbase = NULL;
    //defualt size
    s->size = 8;
    return s;
}

//create a symbol table
SymbolTable create_symtab(char *name){
    SymbolTable st = ckalloc(sizeof(struct sym_table));
    st->nEntries = 0;
    st->parent = NULL;
    st->nKids = 0;
    st->children = NULL;
    st->instructions = NULL;
    st->stacksize = 0;
    st->next = NULL;
    st->name = ckalloc(strlen(name) + 1);
    strcpy(st->name, name);
    //hash table create
    st->tbl = ht_create();
    return st;
}

//generate a new scope
void scope_enter(char *name){
    //make a new symbol table for the new scope
    SymbolTable new = create_symtab(name);
    new->parent = symtab_top;
    //if the parent exists, add this table as a child (non-global table)
    if(symtab_top != NULL){
        symtab_top->nKids = symtab_top->nKids + 1;
        if(symtab_top->children == NULL) symtab_top->children = new;
        else {
            SymbolTable st = symtab_top->children;
            while(st->next != NULL){
                st = st->next;
            }
            st->next = new;
        }
    }
    //no else case, as else case is simply global, which there should only be 1 of
    //now set the new symbol table as the top of stack
    symtab_top = new;
}

//exit current scope
void scope_exit(){
    //remove from stack
    SymbolTable old = symtab_top;
    symtab_top = old->parent;
    //memory management frees go here
    //free tokens in table (probably merge with above while loop honestly)
    //ht_destroy(old->tbl);
    //free rest of table
}

//current stack size
int scope_level(){
    int ret = 0;
    SymbolTable scan = symtab_top;
    while(scan != NULL){
        scan = scan->parent;
        ret++;
    }
    return ret;
}

//add an entry to scope on top of stack (current scope)
void scope_bind(Symbol sym){
    const char *key = sym->name;
    if(ht_set(symtab_top->tbl, key, sym) == NULL){
        printf("out of memory\n");
        exit(4);
    }
}

//search for entry.  returns NULL if none found.
Symbol scope_lookup(char *name, SymbolTable current_scope){
    if(current_scope == NULL) return NULL;
    //flags and vars
    const char *key = name;
    SymbolTable st = current_scope;
    Symbol s = NULL;
    int flag = 0;
    //iterate for all scopes, search in current hash table, if found set flag and break.
    do {
        s = (Symbol) ht_get(st->tbl, key);
        if(s != NULL) {
            flag = 1;
            break;
        }
        st = st->parent;
    } while (st != NULL);
    //if found:
    if(flag) return s;
    else return NULL;
}

//same but only for topmost table (checking for re-defining same value)
Symbol scope_lookup_current(char *name, SymbolTable current_scope){
    if(current_scope == NULL) return NULL;
    const char *key = name;
    Symbol s = (Symbol) ht_get(current_scope->tbl, key);
    return s;
}

//same but only for root table (checking for re-defining same value)
Symbol scope_lookup_global(char *name){
    if(sym_root == NULL) return NULL;
    const char *key = name;
    Symbol s = (Symbol) ht_get(sym_root->tbl, key);
    return s;
}


char *make_block_name(char *type){
    int n = blockcount;
    int c = 1;
    while(n != 0) {
        n = n / 10;
        //hoooooooooly
        c++;
    }
    char *num = ckalloc(c + 1);
    sprintf(num, "%d", blockcount);
    char *ret = ckalloc(strlen(type) + strlen(num) + 1);
    strcpy(ret, type);
    strcat(ret, num);
    blockcount++;
    return ret;
}
