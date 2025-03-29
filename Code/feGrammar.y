%{
    #include <stdlib.h>
    #include <stdarg.h>
    #include "tree.h"

    extern int yylineno;
    extern char *yytext;

    extern int yylex();
    void yyerror(char const *s);

    struct tree *root;
    struct tree *alcnode(int rule, char *name, int num_children, ...);

    int serial = 0;
%}
%debug

%token-table
%code provides {
  const char* yyname(int sym);
}

//union
%union {
    struct tree *treeptr;
}

//// TOKENS ////
%token <treeptr> LE
%token <treeptr> EQEQ
%token <treeptr> NE
%token <treeptr> GE
%token <treeptr> ANDAND
%token <treeptr> OROR
%token <treeptr> MINUSEQ
%token <treeptr> PLUSEQ
%token <treeptr> DOTDOT
%token <treeptr> DOTDOTDOT
%token <treeptr> MOD_SEP
%token <treeptr> RARROW
%token <treeptr> LARROW
%token <treeptr> FAT_ARROW
%token <treeptr> LIT_BYTE
%token <treeptr> LIT_CHAR
%token <treeptr> LIT_INTEGER
%token <treeptr> LIT_FLOAT
%token <treeptr> LIT_STR
%token <treeptr> IDENT
%token <treeptr> UNDERSCORE
%token <treeptr> LIFETIME
//types
%token <treeptr> I32
%token <treeptr> I64
%token <treeptr> F32
%token <treeptr> F64
%token <treeptr> STR
%token <treeptr> STRING
%token <treeptr> BOOL


// keywords
%token <treeptr> SELF
%token <treeptr> STATIC
%token <treeptr> ABSTRACT
%token <treeptr> ALIGNOF
%token <treeptr> AS
%token <treeptr> BECOME
%token <treeptr> BREAK
%token <treeptr> CATCH
%token <treeptr> CRATE
%token <treeptr> DO
%token <treeptr> ELSE
%token <treeptr> ENUM
%token <treeptr> EXTERN
%token <treeptr> FALSE
%token <treeptr> FINAL
%token <treeptr> FN
%token <treeptr> FOR
%token <treeptr> IF
%token <treeptr> IMPL
%token <treeptr> IN
%token <treeptr> LET
%token <treeptr> LOOP
%token <treeptr> MACRO
%token <treeptr> MATCH
%token <treeptr> MOD
%token <treeptr> MOVE
%token <treeptr> MUT
%token <treeptr> OFFSETOF
%token <treeptr> OVERRIDE
%token <treeptr> PRIV
%token <treeptr> PUB
%token <treeptr> PURE
%token <treeptr> REF
%token <treeptr> RETURN
%token <treeptr> SIZEOF
%token <treeptr> STRUCT
%token <treeptr> SUPER
%token <treeptr> UNION
%token <treeptr> UNSIZED
%token <treeptr> TRUE
%token <treeptr> TRAIT
%token <treeptr> TYPE
%token <treeptr> UNSAFE
%token <treeptr> VIRTUAL
%token <treeptr> YIELD
%token <treeptr> DEFAULT
%token <treeptr> USE
%token <treeptr> WHILE
%token <treeptr> CONTINUE
%token <treeptr> PROC
%token <treeptr> BOX
%token <treeptr> CONST
%token <treeptr> WHERE
%token <treeptr> TYPEOF
%token <treeptr> INNER_DOC_COMMENT
%token <treeptr> OUTER_DOC_COMMENT

%token <treeptr> SHEBANG
%token <treeptr> SHEBANG_LINE
%token <treeptr> STATIC_LIFETIME

%token <treeptr> '>'
%token <treeptr> '<'
%token <treeptr> '='
%token <treeptr> '!'
%token <treeptr> '+'
%token <treeptr> '-'
%token <treeptr> '&'
%token <treeptr> '*'
%token <treeptr> '%'
%token <treeptr> '/'
%token <treeptr> '.'
%token <treeptr> ','
%token <treeptr> '('
%token <treeptr> ')'
%token <treeptr> '{'
%token <treeptr> '}'
%token <treeptr> '['
%token <treeptr> ']'
%token <treeptr> ';'
%token <treeptr> ':'

//non-Terminals

%type <treeptr> crate
%type <treeptr> bor
%type <treeptr> ban
%type <treeptr> beq
%type <treeptr> bno
%type <treeptr> exp
%type <treeptr> ter
%type <treeptr> fac
%type <treeptr> literals
%type <treeptr> type_specifier
%type <treeptr> StatementList
%type <treeptr> Statement
%type <treeptr> ret_stmt
%type <treeptr> NT_Statement
%type <treeptr> T_Statement
%type <treeptr> assign_stm
%type <treeptr> let_assign
%type <treeptr> static_assign
%type <treeptr> array
%type <treeptr> item_list
%type <treeptr> items
%type <treeptr> array_index
%type <treeptr> update_stm
%type <treeptr> if_type_stm
%type <treeptr> single_if
%type <treeptr> else
%type <treeptr> while_stm
%type <treeptr> for_stm
%type <treeptr> for_expr
%type <treeptr> macro
%type <treeptr> func
%type <treeptr> input_list
%type <treeptr> inputs
%type <treeptr> function_def
%type <treeptr> fn_param_list
%type <treeptr> fn_params
%type <treeptr> fn_param

%start fe_start

%%
//this rule is for setting the global tree start variable
fe_start : crate    { root = $1; }
//actual grammar starts here
crate : function_def crate  { $$ = alcnode(PR_CRATE, "Crate", 2, $1, $2);}
      | static_assign ';' crate { $$ = alcnode(PR_CRATE, "Crate", 3, $1, $2, $3);}
      | function_def        { $$ = $1; /*$$ = alcnode(PR_CRATE, "Crate", 1, $1);    */}
      ;

////////////////////////////////////////////////////////////////////////
// Literal expressions and macros
////////////////////////////////////////////////////////////////////////

//logical (lower precedence than numeric)
//DO NOT BE CONFUSED HERE 'BOR' IS THE GENERIC EXPRESSION NON TERMINAL, ***NOT EXP*** 
bor : bor OROR ban	{ $$ = alcnode(PR_BOR, "Expression: lor", 3, $1, $2, $3); }
    | ban           { $$ = $1; /*$$ = alcnode(PR_BOR, "Expression: lor", 1, $1);*/ }
    ;

ban : ban ANDAND beq	{ $$ = alcnode(PR_BAN, "Expression: land", 3, $1, $2, $3); }
    | beq               { $$ = $1; /*$$ = alcnode(PR_BAN, "Expression: land", 1, $1);*/ }
    ;
    
beq : beq EQEQ bno	{ $$ = alcnode(PR_BEQ, "Expression: lcomp", 3, $1, $2, $3); }
    | beq LE bno	{ $$ = alcnode(PR_BEQ, "Expression: lcomp", 3, $1, $2, $3); }
    | beq GE bno	{ $$ = alcnode(PR_BEQ, "Expression: lcomp", 3, $1, $2, $3); }
    | beq NE bno	{ $$ = alcnode(PR_BEQ, "Expression: lcomp", 3, $1, $2, $3); }
    | beq '>' bno	{ $$ = alcnode(PR_BEQ, "Expression: lcomp", 3, $1, $2, $3); }
    | beq '<' bno	{ $$ = alcnode(PR_BEQ, "Expression: lcomp", 3, $1, $2, $3); }
    | bno		    { $$ = $1; /*$$ = alcnode(PR_BEQ, "Expression: lcomp", 1);*/ }
    ;

bno : '!' bno		{ $$ = alcnode(PR_BNO, "lnot", 2, $1, $2); }
    | exp		    { $$ = $1; /*$$ = alcnode(PR_BNO, "lnot", 1, $1);*/ }
    ;
  
//numeric
exp : exp '+' ter   { $$ = alcnode(PR_EXP, "exp", 3, $1, $2, $3); }
    | exp '-' ter   { $$ = alcnode(PR_EXP, "exp", 3, $1, $2, $3); }
    | ter           { $$ = $1; /*$$ = alcnode(PR_EXP, "exp", 1, $1);*/ }
    ;

ter : ter '*' fac   { $$ = alcnode(PR_TER, "ter", 3, $1, $2, $3); }
    | ter '/' fac   { $$ = alcnode(PR_TER, "ter", 3, $1, $2, $3); }
    | ter '%' fac   { $$ = alcnode(PR_TER, "ter", 3, $1, $2, $3); }
    | fac           { $$ = $1; /*$$ = alcnode(PR_TER, "ter", 1, $1);*/ }
    ;

fac : '(' bor ')'   { $$ = alcnode(PR_FAC, "fac", 3, $1, $2, $3); }
// unary minus operator
    | '-' fac       { $$ = alcnode(PR_FAC, "fac", 2, $1, $2); }
//no power operator ** b/c irony uses the 'pow' function
//expressions can be function returns
    | func          { $$ = $1; /*$$ = alcnode(PR_FAC, "fac", 1, $1);*/ }
//could technically be macros
    | macro         { $$ = $1; }
//could be an array or an index
    | array         { $$ = $1; }
    | array_index   { $$ = $1; }
//expressions can be int, float, bool, or can be idententifiers
    | literals      { $$ = $1; }
    ;

literals : FALSE         { $$ = $1; /*$$ = alcnode(PR_FAC, "fac", 1, $1);*/ }
         | TRUE          { $$ = $1; /*$$ = alcnode(PR_FAC, "fac", 1, $1);*/ }
         | LIT_INTEGER   { $$ = $1; /*$$ = alcnode(PR_FAC, "fac", 1, $1);*/ }
         | LIT_FLOAT     { $$ = $1; /*$$ = alcnode(PR_FAC, "fac", 1, $1);*/ }
         | LIT_STR       { $$ = $1; /*$$ = alcnode(PR_FAC, "fac", 1, $1);*/ }
         | LIT_CHAR      { $$ = $1; /*$$ = alcnode(PR_FAC, "fac", 1, $1);*/ }
         | IDENT         { $$ = $1; /*$$ = alcnode(PR_FAC, "fac", 1, $1);*/ }
         ;


type_specifier : I32    { $$ = $1; /*$$ = alcnode(PR_TYPE_SPECIFIER, "Type Specifier", 1, $1);*/ }
               | I64    { $$ = $1; /*$$ = alcnode(PR_TYPE_SPECIFIER, "Type Specifier", 1, $1);*/ }
               | F32    { $$ = $1; /*$$ = alcnode(PR_TYPE_SPECIFIER, "Type Specifier", 1, $1);*/ }
               | F64    { $$ = $1; /*$$ = alcnode(PR_TYPE_SPECIFIER, "Type Specifier", 1, $1);*/ }
               | STR    { $$ = $1; /*$$ = alcnode(PR_TYPE_SPECIFIER, "Type Specifier", 1, $1);*/ }
               | STRING { $$ = $1; /*$$ = alcnode(PR_TYPE_SPECIFIER, "Type Specifier", 1, $1);*/ }
               | BOOL   { $$ = $1; }
               ;

////////////////////////////////////////////////////////////////////////
// Upper-Level Statement Non-Terminals
////////////////////////////////////////////////////////////////////////

//StatementList can be used to refer to the body of any closure;
//ideally the final line termination should be optional, but unfortuantely that caused reduce conflicts
StatementList : Statement StatementList     { $$ = alcnode(PR_STATEMENTLIST, "Statement List", 2, $1, $2); }
              | %empty                      { $$ = NULL; }
              ;

Statement : NT_Statement        { $$ = $1; /*$$ = alcnode(PR_STATEMENT, "Statement", 1, $1);*/ }
          | T_Statement         { $$ = $1; /*$$ = alcnode(PR_STATEMENT, "Statement", 1, $1);*/ }
          ;

ret_stmt : RETURN bor ';'       { $$ = alcnode(PR_RET_STMT, "Return Statement", 3, $1, $2, $3); }
         | RETURN ';'           { $$ = alcnode(PR_RET_STMT, "Return Statement", 2, $1, $2); }

//non-terminating statement (no semicolon)
NT_Statement : if_type_stm      { $$ = $1; /*$$ = alcnode(PR_NT_STATEMENT, "Non-Terminating Statement", 1, $1);*/ }
             | while_stm        { $$ = $1; /*$$ = alcnode(PR_NT_STATEMENT, "Non-Terminating Statement", 1, $1);*/ }
             | function_def     { $$ = $1; /*$$ = alcnode(PR_NT_STATEMENT, "Non-Terminating Statement", 1, $1);*/ }
             | for_stm          { $$ = $1; }
             ;

//terminating statement (contains a semicolon)
T_Statement : assign_stm ';'        { $$ = alcnode(PR_T_STATEMENT, "Terminating Statement", 2, $1, $2); }
            | update_stm ';'        { $$ = alcnode(PR_T_STATEMENT, "Terminating Statement", 2, $1, $2); }
//expressions are valid statement; this includes literals and functions
            | bor ';'               { $$ = alcnode(PR_T_STATEMENT, "Terminating Statement", 2, $1, $2); }
//return; could be either with or without an expression
            | ret_stmt              { $$ = $1; }
//          | RETURN bor ';'        { $$ = alcnode(PR_T_STATEMENT, "Terminating Statement", 3, $1, $2, $3); }
//          | RETURN ';'            { $$ = alcnode(PR_T_STATEMENT, "Terminating Statement", 2, $1, $2); }
//technically nothing is also a statement
            | ';'                   { $$ = $1; /*$$ = alcnode(PR_T_STATEMENT, "Terminating Statement", 1, $1);*/ }
            ;

////////////////////////////////////////////////////////////////////////
// Terminating Statements
////////////////////////////////////////////////////////////////////////

///////// ASSIGNMENT //////////

//assignment at the moment does not support references, i don't think
//we are not allowing static mutables
assign_stm : let_assign     { $$ = $1; }
           | static_assign  { $$ = $1; }


let_assign : LET IDENT ':' type_specifier '=' bor           { $$ = alcnode(PR_ASSIGN_STM, "Assignment Statement", 6, $1, $2, $3, $4, $5, $6); }
           | LET MUT IDENT ':' type_specifier '=' bor       { $$ = alcnode(PR_ASSIGN_STM, "Assignment Statement", 7, $1, $2, $3, $4, $5, $6, $7); }
           | LET IDENT ':' type_specifier       { $$ = alcnode(PR_ASSIGN_STM, "Assignment Statement", 4, $1, $2, $3, $4); }
           | LET MUT IDENT ':' type_specifier   { $$ = alcnode(PR_ASSIGN_STM, "Assignment Statement", 5, $1, $2, $3, $4, $5); }
//array assignments (defined similarly)
           | LET IDENT ':' '[' type_specifier ';' LIT_INTEGER ']' '=' array     { $$ = alcnode(PR_ASSIGN_STM, "Assignment Statement", 10, 
                                                                                        $1, $2, $3, $4, $5, $6, $7, $8, $9, $10); }
           | LET MUT IDENT ':' '[' type_specifier ';' LIT_INTEGER ']' '=' array { $$ = alcnode(PR_ASSIGN_STM, "Assignment Statement", 11, 
                                                                                        $1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11); }
           | LET IDENT ':' '[' type_specifier ';' LIT_INTEGER ']'  { $$ = alcnode(PR_ASSIGN_STM, "Assignment Statement", 8, $1, $2, $3, $4, $5, $6, $7, $8); }
           | LET MUT IDENT ':' '[' type_specifier ';' LIT_INTEGER ']' { $$ = alcnode(PR_ASSIGN_STM, "Assignment Statement", 9, $1, $2, $3, $4, $5, $6, $7, $8, $9); }
           ;

static_assign : STATIC IDENT ':' type_specifier '=' literals { $$ = alcnode(PR_STATIC_ASSIGN_STM, "Static Assignment Statement", 6, $1, $2, $3, $4, $5, $6); }
              | CONST IDENT ':' type_specifier '=' literals  { $$ = alcnode(PR_STATIC_ASSIGN_STM, "Static Assignment Statement", 6, $1, $2, $3, $4, $5, $6); }
//array assignments
              | STATIC IDENT ':' '[' type_specifier ';' LIT_INTEGER ']' '=' array { $$ = alcnode(PR_STATIC_ASSIGN_STM, "Static Assignment Statement", 10, 
                                                                                        $1, $2, $3, $4, $5, $6, $7, $8, $9, $10); }
              | CONST IDENT ':' '[' type_specifier ';' LIT_INTEGER ']' '=' array { $$ = alcnode(PR_STATIC_ASSIGN_STM, "Static Assignment Statement", 10, 
                                                                                        $1, $2, $3, $4, $5, $6, $7, $8, $9, $10); }
              ;

array : '[' item_list ']'   { $$ = alcnode(PR_ARRAY, "Array", 3, $1, $2, $3); }

item_list : items   { $$ = $1; }
          | %empty  { $$ = NULL; }
          ;

items : items ',' bor   { $$ = alcnode(PR_ITEMS, "Array Item List", 3, $1, $2, $3); }
      | bor             { $$ = $1; }
      ;

// array index
array_index : IDENT '[' bor ']' { $$ = alcnode(PR_ARRAY_INDEX, "Array Index", 4, $1, $2, $3, $4); }

///////// UPDATE //////////

update_stm : IDENT '=' bor          { $$ = alcnode(PR_UPDATE_STM, "Update Statement", 3, $1, $2, $3); }
           | IDENT PLUSEQ bor       { $$ = alcnode(PR_UPDATE_STM, "Update Statement", 3, $1, $2, $3); }
           | IDENT MINUSEQ bor      { $$ = alcnode(PR_UPDATE_STM, "Update Statement", 3, $1, $2, $3); }
//could be updating an array index; same operations
           | array_index '=' bor    { $$ = alcnode(PR_UPDATE_STM, "Update Statement", 3, $1, $2, $3); }
           | array_index PLUSEQ bor    { $$ = alcnode(PR_UPDATE_STM, "Update Statement", 3, $1, $2, $3); }
           | array_index MINUSEQ bor    { $$ = alcnode(PR_UPDATE_STM, "Update Statement", 3, $1, $2, $3); }
           ;

////////////////////////////////////////////////////////////////////////
// Non-Terminating Statements
////////////////////////////////////////////////////////////////////////
// This includes most upper-level NTs for control statements; 
// strictly speaking, this refers to any statement which may or may not contain a semicolon terminating it


///////// IF-ELSE //////////

if_type_stm : single_if                             { $$ = $1; /*$$ = alcnode(PR_IF_TYPE_STM, "General If Statement", 2, $1);*/ }
            | single_if else                        { $$ = alcnode(PR_IF_TYPE_STM, "General If Statement", 2, $1, $2); }
            ;

single_if : IF bor '{' StatementList '}'    { $$ = alcnode(PR_SINGLE_IF, "If Component", 5, $1, $2, $3, $4, $5); }
          ;

//grammatically, else-if statements are treated as nested if else {if else ...}, etc etc
//this generally speaking should follow the same semantics, at least for our purposes here on this languague subset
else : ELSE '{' StatementList '}'           { $$ = alcnode(PR_ELSE, "Else Component", 4, $1, $2, $3, $4); }
     | ELSE if_type_stm                     { $$ = alcnode(PR_ELSE, "Else Component", 2, $1, $2); }
     ;

///////// WHILE //////////

//while and if loops dont actually need parethenses around their expressions, so the exp non-terminal is valid here as that works for anything which can be evaled
while_stm : WHILE bor '{' StatementList '}'   { $$ = alcnode(PR_WHILE_STM, "While Statement", 5, $1, $2, $3, $4, $5); }
          ;

///////// FOR //////////

for_stm : FOR IDENT IN for_expr '{' StatementList '}' { $$ = alcnode(PR_FOR_STM, "For Statement", 7, $1, $2, $3, $4, $5, $6, $7); }
        ;

//for IN statement; e.g., for i in 1 .. 2 is for i in range(1,2)
for_expr : bor DOTDOT bor       { $$ = alcnode(PR_FOR_EXPR, "For Stmr Expression", 3, $1, $2, $3); }
         | bor DOTDOT '=' bor   { $$ = alcnode(PR_FOR_EXPR, "For Stmr Expression", 4, $1, $2, $3, $4); }
         ;

///////// FUNCTIONS & MACROS //////////
//These are hard-coded macros which in code generation replace with pre-generated optimized code segments.
//hard-coded macros are inserted into the global region in the intermediate and assembler code depending on when insertion occurs
//user defined functions and macros are handled by semantic analyzer portion of the project


//e.g., printf
macro : IDENT '!'                       { $$ = alcnode(PR_MACRO, "Macro", 2, $1, $2); }
      | IDENT '!' '(' input_list ')'    { $$ = alcnode(PR_MACRO, "Macro w/ args", 5, $1, $2, $3, $4, $5); }
      ;

func : IDENT '(' input_list ')'         { $$ = alcnode(PR_FUNC, "Function", 4, $1, $2, $3, $4); }
     ;

input_list : inputs             { $$ = $1; }
           | %empty             { $$ = NULL; }
           ;

inputs : inputs ',' bor         { $$ = alcnode(PR_INPUTS, "Function Inputs", 3, $1, $2, $3); }
       | bor                    { $$ = $1; }
       ;

function_def : FN IDENT '(' fn_param_list ')' '{' StatementList '}'   { $$ = alcnode(PR_FUNCTION_DEF, "Function Definition", 8, 
                                                                             $1, $2, $3, $4, $5, $6, $7, $8); }
//could have a return type
             | FN IDENT '(' fn_param_list ')' RARROW type_specifier '{' StatementList '}'       { $$ = alcnode(PR_FUNCTION_DEF, "Function Definition", 10, 
                                                                                                               $1, $2, $3, $4, $5, $6, $7, $8, $9, $10); }
             ;

//EITHER a comma separated list, or nothing.  extra non-terminal to prevent trailing commas and other weird behavior
fn_param_list : fn_params   { $$ = $1; /*$$ = alcnode(PR_FN_PARAM_LIST, "General Function Parameters", 1, $1);*/ }
              | %empty      { $$ = NULL; }
              ;

//1 or more parameter in a comma separated list
fn_params : fn_params ',' fn_param      { $$ = alcnode(PR_FN_PARAMS, "Param List", 3, $1, $2, $3); }
          | fn_param                    { $$ = $1; /*$$ = alcnode(PR_FN_PARAMS, "Param List", 1, $1);*/ }
          ;

fn_param : IDENT ':' type_specifier     { $$ = alcnode(PR_FN_PARAM, "Single Param", 3, $1, $2, $3); }
//could be an array
         | IDENT ':' '[' type_specifier ';' LIT_INTEGER ']'     { $$ = alcnode(PR_FN_PARAM, "Single Param", 7, $1, $2, $3, $4, $5, $6, $7); }
         ;
%%

void yyerror(const char *str){
    fprintf(stderr, "%s: Line %d at symbol %s\n", str, yylineno, yytext);
    exit(2);
}

struct tree *alcnode(int rule, char *name, int num_children, ...){
    struct tree *ret = ckalloc(sizeof(struct tree));
    ret->prodrule = rule;
    ret->symbolname = name;
    ret->nkids = num_children;
    ret->id = serial++;
    //yippeeeeee
    ret->leaf = NULL;
    ret->scope = NULL;
    ret->type = NULL;
    ret->addr = NULL;
    ret->icode = NULL;
    ret->first = NULL;
    ret->follow = NULL;
    ret->ontrue = NULL;
    ret->onfalse = NULL;
    ret->arrbase = NULL;

    //variable argument sequence handling for children (never make me use this again)
    va_list ap;
    va_start(ap, num_children);
    for(int i = 0; i < num_children; i++){
        ret->kids[i] = va_arg(ap, struct tree *);
    }
    va_end(ap);

    //return
    return ret;
}

const char* yyname(int sym) { 
    //technically yyymbol_name is the correct thing to use but WHATEVER MAN im over it
    return yytname[YYTRANSLATE(sym)];
    //return yysymbol_name(YYTRANSLATE(sym));
}
