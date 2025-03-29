#include "finalcode.h"
#include <string.h>

//setup & debug function
extern void print_item(struct instr *item, FILE *file);
void test_ic_list(struct tree *node){
    struct instr *icode = node->icode;
    while(icode != NULL) {
        print_item(icode, stdout);
        icode = icode->next;
    }
}

//actual stuff
//tac.c global region exports
extern int STR_SIZE;
extern int STR_INDEX;
extern char **STR_REGION;
extern int GLB_SIZE;
extern double *FLT_REGION;
extern int FLT_SIZE;
extern int NUM_FLOATS;

//file io
extern char *outname;
FILE *tac_c = NULL;

//flag
int GLOB_DEFINED = 0;
int maxparam;
int paramflag = 0;
int lastparamflag = 0;
char *tacout = NULL;

void codegen_recur(struct instr *icode, SymbolTable sym_root);
char *atos_addr(struct addr *a);
char *atos_index(struct addr *a);
void setup_regions(void);
void setup_builtins(void);
void setup_string(void);
void setup_float(void);
int max_argc(SymbolTable sym);

void codegen(struct tree *root, SymbolTable sym_root){
    //make file
    char *tacout = calloc(strlen(outname)+3, sizeof(char));
    strcpy(tacout, outname);
    strcat(tacout, ".c");
    tac_c = fopen(tacout, "w");
    fprintf(tac_c, "#include <stdlib.h>\n#include <stdio.h>\n#include <string.h>\n");
    fprintf(tac_c, "\n\n//---- Compiler Code: Initialize memory regions ----//\n\n");
    setup_regions();
    fprintf(tac_c, "\n\n//---- Compiler Code: Built-in macros ----//\n\n");
    setup_builtins();
    //codegen time
    fprintf(tac_c, "\n\n//---- Generated Code: Initilization & function definitions ----//\n\n");
    maxparam = max_argc(sym_root);
    codegen_recur(root->icode, sym_root);
    //finish
    fprintf(tac_c, "\n\n//---- Compiler Code: Call defined irony main ----//\n\n");
    //check for init definition
    if(!GLOB_DEFINED) fprintf(tac_c, "int __fec_init_glb__(){}\n");
    //add real ass main
    fprintf(tac_c, "\nint main(){ \n\t__fec_init_glb__();\n\tchar *par; \n\t__fec_main__(par); \n\treturn 0; \n}\n");
    //close file
    fclose(tac_c);


}

void setup_regions(void){
    //fprintf(tac_c, "char inputbuf[200];");
    //fprintf(tac_c, "char formatbuf[200];");

    fprintf(tac_c, "char global[%d];\n", GLB_SIZE);
    fprintf(tac_c, "char rstring[] = \"");
    setup_string();
    fprintf(tac_c, "\";\n");
    fprintf(tac_c, "char rfloat[%d] = {", FLT_SIZE);
    setup_float();
    fprintf(tac_c, "};\n");
}


void setup_string(void){
    if(STR_REGION == NULL) return;
    //loop through
    for(int i = 0; i < STR_INDEX; i++){
        for(int j = 0; j < strlen(STR_REGION[i])+1; j++){
            switch(STR_REGION[i][j]){
            case '\0':
                fprintf(tac_c, "\\0");
                break;
            case '\n':
                fprintf(tac_c, "\\n");
                break;
            case '\t':
                fprintf(tac_c, "\\t");
                break;
            case '\r':
                fprintf(tac_c, "\\r");
                break;
            case '\'':
                fprintf(tac_c, "\\\'");
                break;
            case '\"':
                fprintf(tac_c, "\\\"");
                break;
            case '\\':
                fprintf(tac_c, "\\\\");
                break;
            default:
                fprintf(tac_c, "%c", STR_REGION[i][j]);
                break;
            }
        }
    }
    return;
}

void setup_float(void){
    if(FLT_REGION == NULL) return;
    char data[sizeof(double *)];
    memcpy(data, &FLT_REGION[0], sizeof(FLT_REGION[0]));
    fprintf(tac_c, "'");
    if(data[0] == 0) fprintf(tac_c, "\\0");
    else fwrite(data, 1, 1, tac_c);
    fprintf(tac_c, "'");
    for(int j = 1; j < sizeof(data); j++){
        fprintf(tac_c, ", '");
        if(data[j] == 0) fprintf(tac_c, "\\0");
        else fwrite(data+j, 1, 1, tac_c);
        fprintf(tac_c, "'");
    }

    for(int i = 1; i < NUM_FLOATS; i++){
        memcpy(data, &FLT_REGION[i], sizeof(FLT_REGION[i]));
        for(int j = 0; j < sizeof(data); j++){
            fprintf(tac_c, ", '");
            if(data[j] == 0) fprintf(tac_c, "\\0");
            else fwrite(data+j, 1, 1, tac_c);
            fprintf(tac_c, "'");
        }
    }

}

void setup_builtins(void){
    //type format: 1 = *(double *), 2 = **(double **), 3 = STRING, 4 = *(int *)

    //println takes a string pointer  -- *(char **) -- and 1 wildcard
    fprintf(tac_c, "int __fec_builtins_println__(char *par, int type);\n");
    fprintf(tac_c, "int __fec_builtins_println__(char *par, int type){\n");
    fprintf(tac_c, "\tchar *out = *(char **)(par);\n");
    fprintf(tac_c, "\tif(out == NULL) return 0;\n");
    fprintf(tac_c, "\tint i = 0;\n");
    fprintf(tac_c, "\twhile(out[i] != '\\0'){\n");
    //printout
    fprintf(tac_c, "\t\tif(out[i] == '{' && out[i+1] == '}'){\n");
    fprintf(tac_c, "\t\t\tswitch(type){\n");
    fprintf(tac_c, "\t\t\tcase 1:\n");
    fprintf(tac_c, "\t\t\t\tprintf(\"%%lf\", *(double *)(par+8));\n");
    fprintf(tac_c, "\t\t\t\tbreak;\n");
    fprintf(tac_c, "\t\t\tcase 2:\n");
    fprintf(tac_c, "\t\t\t\tprintf(\"%%lf\", **(double **)(par+8));\n");
    fprintf(tac_c, "\t\t\t\tbreak;\n");
    fprintf(tac_c, "\t\t\tcase 3:\n");
    fprintf(tac_c, "\t\t\t\tprintf(\"%%s\", *(char **)(par+8));\n");
    fprintf(tac_c, "\t\t\t\tbreak;\n");
    fprintf(tac_c, "\t\t\tdefault:\n");
    fprintf(tac_c, "\t\t\t\tprintf(\"%%ld\", *(long *)(par+8));\n");
    fprintf(tac_c, "\t\t\t\tbreak;\n");
    fprintf(tac_c, "\t\t\t}\n");
    fprintf(tac_c, "\t\ti++;\n");
    fprintf(tac_c, "\t\t}\n");
    //just print the character
    fprintf(tac_c, "\t\telse printf(\"%%c\", out[i]);\n\n");
    fprintf(tac_c, "\t\ti++;");
    fprintf(tac_c, "\t}\n");
    //newline
    fprintf(tac_c, "\tprintf(\"\\n\");\n");
    fprintf(tac_c, "}\n");

    //read takes a string pointer -- *(char **) -- and pulls from stdin via scanf
    fprintf(tac_c, "int __fec_builtins_read__(char *par);\n");
    fprintf(tac_c, "int __fec_builtins_read__(char *par){\n");
    fprintf(tac_c, "\tchar *in = *(char **)(par);\n");
    fprintf(tac_c, "\tchar *out = malloc(sizeof(in));\n");
    //reformat
    fprintf(tac_c, "\tfor(int i = 0; i < strlen(in)+1; i++){\n");
    fprintf(tac_c, "\t\tif(in[i] == '{' && in[i+1] == '}'){\n");
    fprintf(tac_c, "\t\t\tout[i] = '%%';\n");
    fprintf(tac_c, "\t\t\tout[i+1] = 'd';\n");
    fprintf(tac_c, "\t\t\ti++;\n");
    fprintf(tac_c, "\t\t} else out[i] = in[i];\n");
    fprintf(tac_c, "\t}\n");
    //scanf
    fprintf(tac_c, "\tint ret = 0;\n");
    fprintf(tac_c, "\tscanf(out, &ret);\n");
    fprintf(tac_c, "\tfree(out);\n");
    fprintf(tac_c, "\treturn ret;\n");;
    fprintf(tac_c, "}\n");

    //format takes a string pointer -- *(char **) -- and 2 wildcards
    fprintf(tac_c, "char *__fec_builtins_format__(char *par, int type1, int type2);\n");
    fprintf(tac_c, "char *__fec_builtins_format__(char *par, int type1, int type2){\n");
    fprintf(tac_c, "\tchar *in = *(char **)(par);\n");
    fprintf(tac_c, "\tchar out[200];\n");
    fprintf(tac_c, "\tchar buf[200];\n");
    fprintf(tac_c, "\tint flag = 1;\n");
    fprintf(tac_c, "\tint tmp = 0;\n");
    fprintf(tac_c, "\tint j = 0;\n");
    //reformat
    fprintf(tac_c, "\tfor(int i = 0; i < strlen(in)+1; i++){\n");
    fprintf(tac_c, "\t\tif(in[i] == '{' && in[i+1] == '}'){\n");
    fprintf(tac_c, "\t\t\tout[j] = '%%';\n");
    fprintf(tac_c, "\t\t\tj++;\n");
    //which arg were we formatting on
    fprintf(tac_c, "\t\t\tif(flag == 1) tmp = type1;\n");
    fprintf(tac_c, "\t\t\telse tmp = type2;\n");
    fprintf(tac_c, "\t\t\tflag = 2;\n");
    //switch for the format type
    //%lf (2 characters, so inc j by 1)
    fprintf(tac_c, "\t\t\tswitch(tmp){\n");
    fprintf(tac_c, "\t\t\tcase 1:\n");
    fprintf(tac_c, "\t\t\tcase 2:\n");
    fprintf(tac_c, "\t\t\t\tout[j] = 'l';\n");
    fprintf(tac_c, "\t\t\t\tout[j+1] = 'f';\n");
    fprintf(tac_c, "\t\t\t\tj++;\n");
    fprintf(tac_c, "\t\t\t\tbreak;\n");
    //%s
    fprintf(tac_c, "\t\t\tcase 3:\n");
    fprintf(tac_c, "\t\t\t\tout[j] = 's';\n");
    fprintf(tac_c, "\t\t\t\tbreak;\n");
    //%d
    fprintf(tac_c, "\t\t\tdefault:\n");
    fprintf(tac_c, "\t\t\t\tout[j] = 'l';\n");
    fprintf(tac_c, "\t\t\t\tout[j] = 'f';\n");
    fprintf(tac_c, "\t\t\t\tj++;\n");
    fprintf(tac_c, "\t\t\t\tbreak;\n");
    fprintf(tac_c, "\t\t\t}\n");
    fprintf(tac_c, "\t\t\ti++;\n");
    fprintf(tac_c, "\t\t} else out[j] = in[i];\n");
    fprintf(tac_c, "\t\tj++;\n");
    fprintf(tac_c, "\t}\n");
    //get type casts for arguments
    fprintf(tac_c, "\tswitch(type1){\n");
    //a1 is *(double *)
    fprintf(tac_c, "\tcase 1:\n");
    fprintf(tac_c, "\t\tswitch(type2){ \n");
    //a2 is *(double *)
    fprintf(tac_c, "\t\tcase 1:\n");
    fprintf(tac_c, "\t\t\t sprintf(buf, out, *(double *)(par+8), *(double *)(par+16));\n");
    fprintf(tac_c, "\t\t\tbreak;\n");
    //a2 is **(double **)
    fprintf(tac_c, "\t\tcase 2:\n");
    fprintf(tac_c, "\t\t\t sprintf(buf, out, *(double *)(par+8), **(double **)(par+16));\n");
    fprintf(tac_c, "\t\t\tbreak;\n");
    //a2 is *(char **)
    fprintf(tac_c, "\t\tcase 3:\n");
    fprintf(tac_c, "\t\t\t sprintf(buf, out, *(double *)(par+8), *(char **)(par+16));\n");
    fprintf(tac_c, "\t\t\tbreak;\n");
    //a2 is *(int *)
    fprintf(tac_c, "\t\tdefault:\n");
    fprintf(tac_c, "\t\t\t sprintf(buf, out, *(double *)(par+8), *(long *)(par+16));\n");
    fprintf(tac_c, "\t\t\tbreak;\n");
    fprintf(tac_c, "\t\t}\n");
    fprintf(tac_c, "\t\tbreak;\n");

    //a1 is **(double **)
    fprintf(tac_c, "\tcase 2:\n");
    fprintf(tac_c, "\t\tswitch(type2){ \n");
    //a2 is *(double *)
    fprintf(tac_c, "\t\tcase 1:\n");
    fprintf(tac_c, "\t\t\t sprintf(buf, out, **(double **)(par+8), *(double *)(par+16));\n");
    fprintf(tac_c, "\t\t\tbreak;\n");
    //a2 is **(double **)
    fprintf(tac_c, "\t\tcase 2:\n");
    fprintf(tac_c, "\t\t\t sprintf(buf, out, **(double **)(par+8), **(double **)(par+16));\n");
    fprintf(tac_c, "\t\t\tbreak;\n");
    //a2 is *(char **)
    fprintf(tac_c, "\t\tcase 3:\n");
    fprintf(tac_c, "\t\t\t sprintf(buf, out, **(double **)(par+8), *(char **)(par+16));\n");
    fprintf(tac_c, "\t\t\tbreak;\n");
    //a2 is *(int *)
    fprintf(tac_c, "\t\tdefault:\n");
    fprintf(tac_c, "\t\t\t sprintf(buf, out, **(double **)(par+8), *(long *)(par+16));\n");
    fprintf(tac_c, "\t\t\tbreak;\n");
    fprintf(tac_c, "\t\t}\n");
    fprintf(tac_c, "\t\tbreak;\n");

    //a1 is *(char **)
    fprintf(tac_c, "\tcase 3:\n");
    fprintf(tac_c, "\t\tswitch(type2){ \n");
    //a2 is *(double *)
    fprintf(tac_c, "\t\tcase 1:\n");
    fprintf(tac_c, "\t\t\t sprintf(buf, out, *(char **)(par+8), *(double *)(par+16));\n");
    fprintf(tac_c, "\t\t\tbreak;\n");
    //a2 is **(double **)
    fprintf(tac_c, "\t\tcase 2:\n");
    fprintf(tac_c, "\t\t\t sprintf(buf, out, *(char **)(par+8), **(double **)(par+16));\n");
    fprintf(tac_c, "\t\t\tbreak;\n");
    //a2 is *(char **)
    fprintf(tac_c, "\t\tcase 3:\n");
    fprintf(tac_c, "\t\t\t sprintf(buf, out, *(char **)(par+8), *(char **)(par+16));\n");
    fprintf(tac_c, "\t\t\tbreak;\n");
    //a2 is *(int *)
    fprintf(tac_c, "\t\tdefault:\n");
    fprintf(tac_c, "\t\t\t sprintf(buf, out, *(char **)(par+8), *(long *)(par+16));\n");
    fprintf(tac_c, "\t\t\tbreak;\n");
    fprintf(tac_c, "\t\t}\n");
    fprintf(tac_c, "\t\tbreak;\n");

    //a1 is *(int *)
    fprintf(tac_c, "\tdefault:\n");
    fprintf(tac_c, "\t\tswitch(type2){ \n");
    //a2 is *(double *)
    fprintf(tac_c, "\t\tcase 1:\n");
    fprintf(tac_c, "\t\t\t sprintf(buf, out, *(long *)(par+8), *(double *)(par+16));\n");
    fprintf(tac_c, "\t\t\tbreak;\n");
    //a2 is **(double **)
    fprintf(tac_c, "\t\tcase 2:\n");
    fprintf(tac_c, "\t\t\t sprintf(buf, out, *(long *)(par+8), **(double **)(par+16));\n");
    fprintf(tac_c, "\t\t\tbreak;\n");
    //a2 is *(char **)
    fprintf(tac_c, "\t\tcase 3:\n");
    fprintf(tac_c, "\t\t\t sprintf(buf, out, *(long *)(par+8), *(char **)(par+16));\n");
    fprintf(tac_c, "\t\t\tbreak;\n");
    //a2 is *(int *)
    fprintf(tac_c, "\t\tdefault:\n");
    fprintf(tac_c, "\t\t\t sprintf(buf, out, *(long *)(par+8), *(long *)(par+16));\n");
    fprintf(tac_c, "\t\t\tbreak;\n");
    fprintf(tac_c, "\t\t}\n");
    fprintf(tac_c, "\t\tbreak;\n");
    fprintf(tac_c, "\t}\n");

    //frees and return
    fprintf(tac_c, "\tchar *dat = malloc(strlen(buf)+1);\n");
    //fprintf(tac_c, "\tchar **ret = malloc(sizeof(char **));\n");
    //fprintf(tac_c, "\tret = &dat;\n");
    fprintf(tac_c, "\tstrcpy(dat, buf);\n");
    //fprintf(tac_c, "\tfree(out);\n");
    //fprintf(tac_c, "\tfree(buf);\n");
    //return a pointer to the array to keep the *(char **) format
    //fprintf(tac_c, "\treturn ret;\n");
    fprintf(tac_c, "\treturn dat;\n");
    fprintf(tac_c, "}\n");
}


//giant recursive function adding handling for every induvidual instruction
void codegen_recur(struct instr *icode, SymbolTable sym_root){
    if(icode == NULL) return;
    //differentiate via opcode, going down the list,
    //FINALLY something i can actually use a switch statement on
    switch (icode->opcode){
    //-----normal instructions-----//
    case O_ADD:
        //X := Y + Z
        fprintf(tac_c, "\t%s = %s + %s;\n", atos(icode->dest), atos(icode->src1), atos(icode->src2));
        //int only
        break;
    case O_SUB:
        //X := Y - Z
        fprintf(tac_c, "\t%s = %s - %s;\n", atos(icode->dest), atos(icode->src1), atos(icode->src2));
        //int only
        break;
    case O_MUL:
        //X := Y * Z
        fprintf(tac_c, "\t%s = %s * %s;\n", atos(icode->dest), atos(icode->src1), atos(icode->src2));
        //int only
        break;
    case O_DIV:
        //X := Y / Z
        fprintf(tac_c, "\t%s = %s / %s;\n", atos(icode->dest), atos(icode->src1), atos(icode->src2));
        //int only
        break;
    case O_NEG:
        //X := -Y
        fprintf(tac_c, "\t%s = -%s;\n", atos(icode->dest), atos(icode->src1));
        //int or float
        break;
    case O_ASN:
        //X := Y
        if(icode->dest->type->basetype == F64_LIT_NP) fprintf(tac_c, "\t*%s = *%s;\n", atos(icode->dest), atos(icode->src1));
        else fprintf(tac_c, "\t%s = %s;\n", atos(icode->dest), atos(icode->src1));
        //any type
        break;
    case O_ADDR:
        //X := &Y
        fprintf(tac_c, "\t%s = %s;\n", atos(icode->dest), atos_addr(icode->src1));
        //any type; float or string, or array index
        break;
    case O_GOTO:
        //goto label
        fprintf(tac_c, "\tgoto %s;\n", atos(icode->dest));
        break;
    case O_BLT:
        //X = Y < Z
        fprintf(tac_c, "\t%s = %s < %s;\n", atos(icode->dest), atos(icode->src1), atos(icode->src2));
        //any type but assigns a boolean
        break;
    case O_BLE:
        //X = Y <= Z
        fprintf(tac_c, "\t%s = %s <= %s;\n", atos(icode->dest), atos(icode->src1), atos(icode->src2));
        //any type but assigns a boolean
        break;
    case O_BGT:
        //X = Y > Z
        fprintf(tac_c, "\t%s = %s > %s;\n", atos(icode->dest), atos(icode->src1), atos(icode->src2));
        //any type but assigns a boolean
        break;
    case O_BGE:
        //IF X >= Y goto Z
        fprintf(tac_c, "\t%s = %s >= %s;\n", atos(icode->dest), atos(icode->src1), atos(icode->src2));
        //any type but assigns a boolean
        break;
    case O_BEQ:
        //X = Y == Z
        fprintf(tac_c, "\t%s = %s == %s;\n", atos(icode->dest), atos(icode->src1), atos(icode->src2));
        //any type but assigns a boolean
        break;
    case O_BNE:
        //X = Y != Z
        fprintf(tac_c, "\t%s = %s != %s;\n", atos(icode->dest), atos(icode->src1), atos(icode->src2));
        //any type but assigns a boolean
        break;
    case O_BIF:
        //if X goto Y
        fprintf(tac_c, "\tif (%s) goto %s;\n", atos(icode->dest), atos(icode->src1));
        //bool only (y is a label)
        break;
    case O_PARM:
        //param X
        fprintf(tac_c, "\t%s(locparams + paroff) = %s;\n", atot(icode->dest), atos(icode->dest));
        fprintf(tac_c, "\t paroff += 8;\n");
        //parameter added: keep track of this for builtins
        lastparamflag = paramflag;
        switch(icode->dest->type->basetype){
            case F64_LIT_NP:
                paramflag = 1;
                break;
            case F64_TYPE:
                paramflag = 2;
                break;
            case STRING_TYPE:
                paramflag = 3;
                break;
            default:
                paramflag = 4;
                break;
        }
        //any type
        break;
    case O_CALL:
        //call X Y Z
        //handle for built-ins
        if(strcmp(icode->dest->name, "println") == 0) fprintf(tac_c, "\t__fec_builtins_println__(locparams, %d);\n", paramflag);
        else if(strcmp(icode->dest->name, "read") == 0) fprintf(tac_c, "\t%s = __fec_builtins_read__(locparams);\n", atos(icode->src2));
        else if(strcmp(icode->dest->name, "format") == 0) fprintf(tac_c, "\t%s = __fec_builtins_format__(locparams, %d, %d);\n", atos(icode->src2), lastparamflag, paramflag);
        //non-built-ins
        else fprintf(tac_c, "\t%s = __fec_%s__(locparams)\n;\n", atos(icode->src2), icode->dest->name);
        //reset parameter offset in case multiple function calls
        fprintf(tac_c, "\tparoff = 0;\n");
        //X is a name, y is number of arguments, z is return place.
        //interestingly, i dont think we use argument y here becauase you just send a memory region
        break;
    case O_RET:
        //return X
        fprintf(tac_c, "\treturn %s;\n", atos(icode->dest));
        //any type
        break;
    case O_MOD:
        //X = Y % Z
        fprintf(tac_c, "\t%s = %s %% %s;\n", atos(icode->dest), atos(icode->src1), atos(icode->src2));
        //int only
        break;
    case O_FADD: 
        //X = Y + Z
        fprintf(tac_c, "\t*%s = *%s + *%s;\n", atos(icode->dest), atos(icode->src1), atos(icode->src2));
        //fprintf(tac_c, "\t*(double *)(%s+%d) = *%s + *%s", atos(icode->src1), atos(icode->src2));
        //float only
        break;
    case O_FSUB:
        //X = Y - Z
        fprintf(tac_c, "\t*%s = *%s - *%s;\n", atos(icode->dest), atos(icode->src1), atos(icode->src2));
        //float only
        break;
    case O_FMUL: 
        //X = Y * Z
        fprintf(tac_c, "\t*%s = *%s * *%s;\n", atos(icode->dest), atos(icode->src1), atos(icode->src2));
        //float only
        break;
    case O_FDIV:
        //X = Y / Z
        fprintf(tac_c, "\t*%s = *%s / *%s;\n", atos(icode->dest), atos(icode->src1), atos(icode->src2));
        //float only
        break;
    case NOP:
        //any noppers in chat
        break;


    //-----special instructions-----//
    case DEREF:
        //deref X
        //indicates that in all subsequent uses of memory address X, it need be dereferenced
        //in TAC_C, that means *X.  in ASM, that means (x) where x is a register.
        break;
    case D_LABEL:
        //label X
        fprintf(tac_c, "\t%s:\n", atos(icode->dest));
        //X is a direct mode integer address.
        //in TAC_C, this means just do `label_x:` as the output line. 
        break;
    
    //-----functions-----//
    //parameters are ALWAYS the first addresses of the procedure's local region, in order.    
    //implicit to all procedure calls the "char *params[]" in TAC_C must be copied to the first n param addresses.
    //this functionality requires use of the symbol table, both to identify number of parameters and their types.
     
    case D_GLOB:
        //glob __fec_init_glb__ X
        fprintf(tac_c, "int %s(){\n", icode->dest->name);
        //fprintf(tac_c, "\tglobal = calloc(%d, sizeof(char));\n", icode->src1->offset);
        //x is the stack size for the global init procedure.
        //this is a FUNCTION which initializes the global memory region. if this region is defined it MUST be called.
        GLOB_DEFINED = 1;
        break;
    case D_PROC:
        //proc X, Y
        //get symbol
        Symbol funcsym = scope_lookup(icode->dest->name, sym_root);
        int paramcount = funcsym->type->u.func.nparams;
        //prototype
        fprintf(tac_c, "int __fec_%s__(char *par);\n", icode->dest->name);
        //function setup
        fprintf(tac_c, "int __fec_%s__(char *par){\n", icode->dest->name);
        fprintf(tac_c, "\tchar local[%d];\n", icode->src1->offset);
        fprintf(tac_c, "\tchar locparams[%d];\n \tint paroff = 0;\n", maxparam*8);
        //alloc locals
        paramlist params = funcsym->type->u.func.params;
        for(int i = 0; i < paramcount; i++){
            fprintf(tac_c, "\t*%s(local+%d) = *%s(par+%d);\n", ttot(params->type), i * 8, ttot(params->type), i * 8);
            params = params->next;
        }
        //x is a name, y is the stack size.  indicates a non-global callable procedure
        break;
    case D_END:
        //no args
        fprintf(tac_c, "}\n\n");
        //indicates end of current procedure.
        //for TAC_C, this is `}`
        break;

    //smile
    default:
        break;
    }
    //recurse
    codegen_recur(icode->next, sym_root);
}


char *atos(struct addr *a){
    //non-address cases
    if(a == NULL) {
        fprintf(stderr, "something weird happened in codegen gamers an address is being printed that's null\n");
        return "";
    }

    if(a->isname) return a->name;

    //actual address cases

    //get region
    char *loc;
    int lab = 0;
    int con = 0;
    int glo = 0;
    switch (a->region){
    case R_GLOBAL:
        loc = "global";
        break;
    case R_LOCAL:
        loc = "local";
        break;
    case R_LABEL:
        loc = "label";
        lab = 1;
        break;
    case R_STRING:
        loc = "rstring";
        glo = 1;
        break;
    case R_FLOAT:
        loc = "rfloat";
        glo = 1;
        break;
    case R_CONST:
        loc = "rconst";
        con = 1;
        break;
    default:
        loc = NULL;
        break;
    }

    //get type
    char *type;
    if(a->type == NULL) type = NULL;
    else{
        //strings and floats are actually pointers to a global region space.
        //if these reference the global region space (R_STRING or R_FLOAT) then these are a pointer. otherwise, they're a double pointer.
        //string ops inherently carry a level of indirection
        switch (a->type->basetype){
        case F64_LIT_NP:
            type = "(double *)";
            break;
        case F64_TYPE:
            type = glo ? "*(double *)" : "*(double **)";
            break;
        case STRING_TYPE:
            type = glo ? "(char *)" : "*(char **)";
            break;
        //arrays are pointers to a memory region
        //for float and char arrays, this means they're ALWAYS an array of pointers, so they're actually TRIPLE POINTERS
        //HOWEVER, these must retain a level of indirection because they themselves are pointers
        case ARRAY_TYPE:
            switch(a->type->u.arr.elemtype->basetype){
            case F64_TYPE:
                type = "*(double **)";
                break;
            case STRING_TYPE:
                type = "**(char ***)";
                break;
            //bool type, null type, int type, and general default are all ints
            default:
                type = "*(long **)";
                break;
            }
            break;
        default:
            type = "*(long *)";
            break;
        }
    }    
    //get offset
    int tmp = snprintf(NULL, 0, "%d", a->offset);
    char *off = ckalloc(tmp+1);
    snprintf(off, tmp+1, "%d", a->offset);
    //if int constant just return the value
    if(con) return off;
    //if label return label_[labelnumber]
    if(lab) {
        char *r = ckalloc(strlen(loc) + tmp + 2);
        strcpy(r, loc);
        strcat(r, "_");
        strcat(r, off);
        return r;
    }

    char *index = atos_index(a->index_offset);

    //non-array index
    if(index == NULL){
        //if it's an actual value its type and region and offset 
        char *ret = ckalloc(strlen(type) + strlen(loc) + tmp + 4);
        strcpy(ret, type);
        strcat(ret, "(");
        strcat(ret, loc);
        strcat(ret, "+");
        strcat(ret, off);
        strcat(ret, ")");
        return ret;
    }
    //array index
    else {
        //if it's an actual value its type and region and offset 
        char *ret = ckalloc(strlen(type) + strlen(loc) + strlen(index) + tmp + 5);
        strcpy(ret, type);
        strcat(ret, "(");
        strcat(ret, loc);
        strcat(ret, "+");
        strcat(ret, off);
        strcat(ret, "+");
        strcat(ret, index);
        strcat(ret, ")");
        return ret;
    }
}

char *atos_addr(struct addr *a){
    //non-address cases
    if(a == NULL) {
        fprintf(stderr, "something weird happened in codegen gamers an address is being printed that's null\n");
        return NULL;
    }

    if(a->isname) return a->name;

    //actual address cases

    //get region
    char *loc;
    int lab = 0;
    int con = 0;
    int glo = 0;
    switch (a->region){
    case R_GLOBAL:
        loc = "global";
        break;
    case R_LOCAL:
        loc = "local";
        break;
    case R_LABEL:
        loc = "label_";
        lab = 1;
        break;
    case R_STRING:
        loc = "rstring";
        glo = 1;
        break;
    case R_FLOAT:
        loc = "rfloat";
        glo = 1;
        break;
    case R_CONST:
        loc = "rconst";
        con = 1;
        break;
    default:
        loc = NULL;
        break;
    }

    //get type
    char *type;
    if(a->type == NULL) type = NULL;
    else{
        //strings and floats are actually pointers to a global region space.
        //if these reference the global region space (R_STRING or R_FLOAT) then these are a pointer. otherwise, they're a double pointer.
        //string ops inherently carry a level of indirection
        switch (a->type->basetype){
        case F64_LIT_NP:
            type = "(double *)";
            break;
        case F64_TYPE:
            type = glo ? "(double *)" : "*(double **)";
            break;
        case STRING_TYPE:
            type = glo ? "(char *)" : "*(char **)";
            break;
        //arrays are pointers to a memory region
        //for float and char arrays, this means they're ALWAYS an array of pointers, so they're actually TRIPLE POINTERS
        //HOWEVER, these must retain a level of indirection because they themselves are pointers
        case ARRAY_TYPE:
            switch(a->type->u.arr.elemtype->basetype){
            case F64_TYPE:
                type = "*(double **)";
                break;
            case STRING_TYPE:
                type = "*(char ***)";
                break;
            //bool type, null type, int type, and general default are all ints
            default:
                type = "(long *)";
                break;
            }
            break;
        default:
            type = "(long *)";
            break;
        }
    }
    
    //get offset
    int tmp = snprintf(NULL, 0, "%d", a->offset);
    char *off = ckalloc(tmp+1);
    snprintf(off, tmp+1, "%d", a->offset);
    //if int constant just return the value
    if(con) return off;
    //if label return label_[labelnumber]
    if(lab) {
        char *r = ckalloc(strlen(loc) + tmp + 2);
        strcpy(r, loc);
        strcat(r, "_");
        strcat(r, off);
        return r;
    }

    char *index = atos_index(a->index_offset);

    //no array index
    if(index == NULL){
        //if it's an actual value its type and region and offset 
        char *ret = ckalloc(strlen(type) + strlen(loc) + tmp + 4);
        strcpy(ret, type);
        strcat(ret, "(");
        strcat(ret, loc);
        strcat(ret, "+");
        strcat(ret, off);
        strcat(ret, ")");
        return ret;
    }
    //array index
    else {
        //if it's an actual value its type and region and offset 
        char *ret = ckalloc(strlen(type) + strlen(loc) + strlen(index) + tmp + 5);
        strcpy(ret, type);
        strcat(ret, "(");
        strcat(ret, loc);
        strcat(ret, "+");
        strcat(ret, off);
        strcat(ret, "+");
        strcat(ret, index);
        strcat(ret, ")");
        return ret;
    }
}

char *atos_index(struct addr *a){
    if(a == NULL) return NULL;
    char *type = "*(long *)";
    //region
    char *reg;
    switch(a->region){
    case R_GLOBAL:
        reg = "global";
        break;
    case R_LOCAL:
        reg = "local";
        break;
    case R_CONST:
        reg = "rconst";
        break;
    default:
        reg = NULL;
        break;
    }
    //offset
    int tmp = snprintf(NULL, 0, "%d", a->offset);
    char *off = ckalloc(tmp+1);
    snprintf(off, tmp+1, "%d", a->offset);
    //if it's an actual value its type and region and offset 
    char *ret = ckalloc(strlen(type) + strlen(reg) + tmp + 6);
    strcpy(ret, type);
    strcat(ret, "(");
    strcat(ret, reg);
    strcat(ret, "+");
    strcat(ret, off);
    strcat(ret, ")");
    strcat(ret, "*8");
    return ret;
}

char *atot(struct addr *a){
    if(a == NULL) {
        fprintf(stderr, "something weird happened in codegen gamers an address is being printed that's null\n");
        return NULL;
    }
    //get region
    int glo = 0;
    switch (a->region){
    case R_STRING:
        glo = 1;
        break;
    case R_FLOAT:
        glo = 1;
        break;
    default:
        break;
    }

    //get type
    char *type;
    if(a->type == NULL) return NULL;
    else{
        //strings and floats are actually pointers to a global region space.
        //if these reference the global region space (R_STRING or R_FLOAT) then these are a pointer. otherwise, they're a double pointer.
        //string ops inherently carry a level of indirection
        switch (a->type->basetype){
        case F64_LIT_NP:
            glo = 1;
        case F64_TYPE:
            type = glo ? "*(double *)" : "*(double **)";
            break;
        case STRING_TYPE:
            type = glo ? "(char *)" : "*(char **)";
            break;
        //arrays are pointers to a memory region
        //for float and char arrays, this means they're ALWAYS an array of pointers, so they're actually TRIPLE POINTERS
        //HOWEVER, these must retain a level of indirection because they themselves are pointers
        case ARRAY_TYPE:
            switch(a->type->u.arr.elemtype->basetype){
            case F64_TYPE:
                type = "*(double **)";
                break;
            case STRING_TYPE:
                type = "*(char ***)";
                break;
            //bool type, null type, int type, and general default are all ints
            default:
                type = "*(long **)";
                break;
            }
            break;
        default:
            type = "*(long *)";
            break;
        }
    }
    return type;
}

char *ttot(typeptr t){
    //get type
    char *type;
    if(t == NULL) return NULL;
    else{
        //strings and floats are actually pointers to a global region space.
        switch (t->basetype){
        //unless they're not
        case F64_LIT_NP:
            type = "*(double *)";
            break;
        case F64_TYPE:
            type = "*(double **)";
            break;
        case STR_TYPE:
            type = "*(char **)";
            break;
        //arrays are pointers to a memory region
        //for float and char arrays, this means they're ALWAYS an array of pointers, so they're actually TRIPLE POINTERS
        //HOWEVER, these must retain a level of indirection because they themselves are pointers
        case ARRAY_TYPE:
            switch(t->u.arr.elemtype->basetype){
            case F64_TYPE:
                type = "*(double **)";
                break;
            case STRING_TYPE:
                type = "*(char ***)";
                break;
            //bool type, null type, int type, and general default are all ints
            default:
                type = "*(long **)";
                break;
            }
            break;
        default:
            type = "(long *)";
            break;
        }
    }
    return type;    
}

int max_argc(SymbolTable sym){
    hti table = ht_iterator(sym->tbl);
    int ret = 0;
    //step through table
    while(ht_next(&table)){
        //if this is a function, set ret to the maximum of itself and the function's parameter count
        if(((Symbol)table.value)->type->basetype == FUNC_TYPE) {
            ret = (((Symbol)table.value)->type->u.func.nparams > ret) ? ((Symbol)table.value)->type->u.func.nparams : ret;
        }
    }
    //ret should be the maximum parameter count of any function
    return ret;
}