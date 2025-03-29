#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include "tree.h"
#include "feGrammar.tab.h"
#include "symtab.h"
#include "type.h"
#include "dot.h"
#include "typecheck.h"
#include "tac.h"
#include "intercode.h"
#include "finalcode.h"

int PRINTOUT = 0;
int SYMOUT = 0;
int DOTOUT = 0;
int INTERCODE = 0;
int ICOUT = 0;
int FINALCODE = 0;
int OBJFILE = 0;

extern int yydebug;
extern FILE *yyin;
//lexer
extern int yylex(void);
extern int yylex_destroy(void);
extern int yyparse();
char *targetfile;
char *outname = NULL;
//parser
void freeTokens(struct tree *node, int depth,int childnum);
const char *yyname(int cat);
extern struct tree *root;
//symbol table
extern SymbolTable sym_root;
//icode
char cwd[300];
FILE *icoutput = NULL;

int main(int argc, char *argv[]){
    //yydebug
    //yydebug = 1;
    //args
    if(argc == 1) return 0;
    int a = 1;
    for(; a < argc; a++){
        if(strcmp(argv[a], "-dot") == 0) DOTOUT = 1;
        if(strcmp(argv[a], "-symtab") == 0) SYMOUT = 1;
        if(strcmp(argv[a], "-tree") == 0) PRINTOUT = 1;
        if(strcmp(argv[a], "-ifile") == 0) INTERCODE = 1;
        if(strcmp(argv[a], "-icode") == 0) ICOUT = 1;
        if(strcmp(argv[a], "-s") == 0) FINALCODE = 1;
        if(strcmp(argv[a], "-c") == 0) OBJFILE = 1;
    }
    //file io
    targetfile = argv[a-1];
    yyin = fopen(targetfile, "r");
    //parse
    int ret = yyparse();
    if(PRINTOUT) printf("yyparse returns: %d\n", ret);
    //outputs and memory management
    //note that the dot is NOT memory safe
    if(DOTOUT) print_graph(root, "treeprint.dot");
    scope(root);
    typecheck(root);
    //icode
    //icode file output
    //note that this is made a little bit buggy as a result of some of the icode changes
    if(INTERCODE){
        char *newname = calloc(strlen(targetfile)+1, sizeof(char));
        strncpy(newname, targetfile, strlen(targetfile)-3);
        strncat(newname, ".ic", 4);
        icoutput = fopen(newname, "w");
    }
    //if no flag, icoutput is NULL and the write function returns.
    intermediate_code(root, sym_root, icoutput);
    //final code file io
    outname = calloc(strlen(targetfile)+1, sizeof(char));
    strncpy(outname, targetfile, strlen(targetfile)-3);
    if(ICOUT) test_ic_list(root);
    //final code: generate TAC-C File
    codegen(root, sym_root);
    char *tacout = ckalloc(strlen(outname)+3);
    strcpy(tacout, outname);
    strcat(tacout, ".c");
    //executable: call gcc on TAC-C File to make object file
    if(OBJFILE){
        int pid = fork();
        if(pid == 0){
            char *objout = ckalloc(strlen(outname)+3);
            strcpy(objout, outname);
            strcat(objout, ".o");
            char *args[] = {"gcc", "-c", tacout, "-o", objout, NULL};
            execv("/usr/bin/gcc", args);
        } else if (pid == -1) {
            fprintf(stderr, "An unknown error occured; could not fork to compile into object file\n");
            exit(4);
        }
    }
    //executable: if no -c flag, link to executable
    else {
        int pid = fork();
        if(pid == 0){
            char *args[] = {"gcc", "-g", tacout, "-o", outname, NULL};
            execv("/usr/bin/gcc", args);
        } else if (pid == -1) {
            fprintf(stderr, "An unknown error occured; could not fork to compile into executable\n");
            exit(4);
        }
    }
    //cleanup
    if(INTERCODE) fclose(icoutput);
    if(!FINALCODE) {
        wait(NULL);
        remove(tacout);
        //int i = remove(tacout);
        //printf("Removing %s: %d\n", tacout, i);
    }

    freeTokens(root, 0, 0);
    fclose(yyin);
    yylex_destroy();
    //printf("\n");
    //if(!PRINTOUT && !DOTOUT && !SYMOUT) printf("No Errors Encountered\n");
    return 0;
}

// "checked" allocation
void *ckalloc(int n) {
    void *p = malloc(n);
    if (p == NULL) {
        fprintf(stderr, "out of memory for request of %d bytes\n", n);
        exit(4);
    }
    return p;
}

//Depth-first recursive free function
//this ALSO will print nodes following left to right depth first traversal if PRINTOUT is true
void freeTokens(struct tree *node, int depth, int childnum){
	if(node == NULL) return;
    for(int i = 0; i < depth; i++){
        if(PRINTOUT) printf("\t");
    }
    //non-leaf nodes (production rules)
	if(node->nkids > 0){
        //printout happens before recursion, so that tree is printed top to bottom
        if(PRINTOUT) printf("Production Rule: %d %s\n", node->prodrule, node->symbolname);
        //Recurse left-to-right on all kids until base is found
        for(int i = 0; i < node->nkids; i++){
            freeTokens(node->kids[i], depth+1, i+1);
            node->kids[i] = NULL;
        }
        //i dont think symbolname is dynamic allocated but just in case its here to be uncommented
        //free(node->symbolname);
        //symbolname = NULL;
        free(node);
        node = NULL;
	} 
    //leaf nodes
    else {
        //could be a leaf
        if(node->nkids == -1){
            //print w/ token information
            if(PRINTOUT) printf("Terminal: %d %s\n", node->prodrule, node->leaf->text);
            //free token
            free(node->leaf->text);
            free(node->leaf->filename);
            if((node->leaf->category == LIT_STR) || (node->leaf->category == LIT_CHAR)) free(node->leaf->sval);
            free(node->leaf);
        }
        //could be a non-terminal
        else if(PRINTOUT) printf("Production Rule: %d %s\n", node->prodrule, node->symbolname);
        free(node);
        node = NULL;
    }       
}


