/*
 * Three Address Code - skeleton for CS 423
 */
#include "tac.h"
#include "type.h"
#include "symtab.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

extern SymbolTable sym_root;

//technically obselete dont worry much about these
int LBL_SIZE = 0;
//char *LBL_REGION = NULL;

//string constant region (global)
int STR_SIZE = 0;
int STR_INDEX = 0;
char **STR_REGION = NULL;
//other global region (also global obviously)
int GLB_SIZE = 0;
//label region (also global (?); treated as just an int)
int LBL_COUNT = 0;
//float region (also global)
double *FLT_REGION = NULL;
int FLT_SIZE = 0;
int NUM_FLOATS = 0;

extern char *deescape(char *text);

void print_item(struct instr *item, FILE *file);
void print_string_region(FILE *file);
void print_float_region(FILE *file);
void print_global_region(FILE *file, struct tree *root);
void print_code_region(SymbolTable s, FILE *file);
//void print_labels();

const char *OPNAMES[] = {
    "ADD",
    "SUB",
    "MUL",
    "DIV",
    "NEG",
    "ASN",
    "ADDR",
    "LCONT",
    "SCONT",
    "GOTO",
    "BLT",
    "BLE",
    "BGT",
    "BGE",
    "BEQ",
    "BNE",
    "BIF",
    "BNIF",
    "PARAM",
    "CALL",
    "RET",
    "D_GLOB",
    "D_PROC",
    "D_LOCAL",
    "D_LABEL",
    "D_END",
    "O_MOD",
    "O_FADD",
    "O_FSUB",
    "O_FMUL",
    "O_FDIV",
    "NOP",
    "DEREF"
};

const char *REGNAMES[] = {
   "globl",
   "loc",
   "class",
   "label",
   "str",
   "float",
   "const"
};




///////////////////////////////////////
   /////////// functions ///////////
///////////////////////////////////////

struct addr *address(char *name, int region, int offset, struct typeinfo *type){
   struct addr *ret = malloc(sizeof(struct addr));
   ret->isname = (name == NULL) ? 0 : 1;
   ret->region = region;
   ret->offset = offset;
   ret->name = name;
   ret->type = type;
   ret->index_offset = NULL;
   return ret;
}

void tacprint(SymbolTable symtab,  struct tree *root, FILE *file){
   if(file == NULL) return;
   print_string_region(file);
   print_float_region(file);
   print_global_region(file, root);
   print_code_region(symtab, file);
}

void print_code_region(SymbolTable s, FILE *file){
   if(s->instructions != NULL){
      fprintf(file, "%s:\n", s->name);
      fprintf(file, ".code %d\n", s->stacksize);
      struct instr *iter = s->instructions;
      do{
         print_item(iter, file);
         iter = iter->next;
      } while(iter != NULL);
   }
   SymbolTable next = s->children;
   while(next != NULL) {
      print_code_region(next, file);
      next = next->next;
   }
   //print_labels();
}

void add_global_n(int n) { GLB_SIZE += n * 8; }

struct addr *genglobal(struct typeinfo *type) { return genglobal_n(1, type); }
struct addr *genglobal_n(int n, struct typeinfo *type){
   GLB_SIZE += n * 8;
   return address(NULL, R_GLOBAL, GLB_SIZE - (n*8), type);
}

//stack size ops...?
struct addr *genlocal(SymbolTable symtab, struct typeinfo *type) { return genlocal_n(1, symtab, type); }
struct addr *genlocal_n(int n, SymbolTable symtab, struct typeinfo *type){
   SymbolTable tmp = symtab;
   while(tmp->parent != sym_root) tmp = tmp->parent;
   tmp->stacksize += n * 8;
   return address(NULL, R_LOCAL, tmp->stacksize - (n*8), type);
}

//labels (we love integers)
struct addr *genlabel(){ return address(NULL, R_LABEL, ++LBL_COUNT, NULL); }

///////////
//helpers//
///////////

void print_string_region(FILE *file){
   if(STR_REGION == NULL) return;
   fprintf(file, ".string %d\n", STR_SIZE);
   //loop through
   fprintf(file, "\t");
   for(int i = 0; i < STR_INDEX; i++){
      for(int j = 0; j < strlen(STR_REGION[i])+1; j++){
         switch(STR_REGION[i][j]){
            case '\0':
               fprintf(file, "\\0");
               break;
            case '\n':
               fprintf(file, "\\n");
               break;
            case '\t':
               fprintf(file, "\\t");
               break;
            case '\r':
               fprintf(file, "\\r");
               break;
            case '\'':
               fprintf(file, "\\\'");
               break;
            case '\"':
               fprintf(file, "\\\"");
               break;
            case '\\':
               fprintf(file, "\\\\");
               break;
            default:
               fprintf(file, "%c", STR_REGION[i][j]);
               break;
         }
      }
   }
   fprintf(file, "\n");
   return;
}

void print_float_region(FILE *file){
   if(FLT_REGION == NULL) return;
   fprintf(file, ".float %d\n", FLT_SIZE);
   for(int i = 0; i < NUM_FLOATS; i++) fprintf(file, "\t%d: %lf\n", i * 8, FLT_REGION[i]);
}


void print_global_region(FILE *file, struct tree *root){
   if(root->icode == NULL) return;
   //don't printout main twice if there's no globals lol
   if(root->icode->opcode == 3023) return;
   //parse global region
   fprintf(file, ".glb %d\n", GLB_SIZE);
   struct instr *iter = root->icode;
   do{
      print_item(iter, file);
      iter = iter->next;
   } while (iter != NULL);
}
/*
void print_labels(){
   if(LBL_REGION == NULL) return;
   printf("\n\nLabels, for reference:\n");
   //loop through
   printf("\t");
   for(int i = 0; i < LBL_SIZE; i++){
      if(LBL_REGION[i] == '\0') printf("\\0");
      else printf("%c", LBL_REGION[i]);
   }
   printf("\n");
   return;
}
*/

void print_item(struct instr *item, FILE *file){
   const char *opname = OPNAMES[item->opcode - 3001];
   char *a1r, *a2r, *a3r;
   int a1o = -1, a2o = -1, a3o = -1;

   if(strcmp(opname, "D_LABEL") == 0){
      fprintf(file, "\tlabel:%d\n", item->dest->offset);
      return;
   }
   
   fprintf(file, "\t\t%s\t", opname);
    
   if(item->dest != NULL){
      if(item->dest->isname){
         a1r = item->dest->name;
         fprintf(file, "%s", a1r);
      } 
      else {
         a1r = (char *)REGNAMES[item->dest->region - 2001];
         a1o = item->dest->offset;
         fprintf(file, "%s:", a1r);
         fprintf(file, "%d",a1o);
      }
   }

   if(item->src1 != NULL){
      if(item->src1->isname){
         a2r = item->src1->name;
         fprintf(file, ", %s", a2r);
      } 
      else {
         a2r = (char *)REGNAMES[item->src1->region - 2001];
         a2o = item->src1->offset;
         fprintf(file, ", %s:%d", a2r,a2o);
      }
   }

   if(item->src2 != NULL){
      if(item->src2->isname){
         a3r = item->src2->name;
         fprintf(file, ", %s", a3r);
      }
      else {
         a3r = (char *)REGNAMES[item->src2->region - 2001];
         a3o = item->src2->offset;
         fprintf(file, ", %s:%d", a3r,a3o);
      }
   }
   fprintf(file, "\n");
   //call (takes a label so just print that out)
   //if(item->opcode == O_CALL) printf("\t%s\t %s,%d,%s:%d\n", opname, &LBL_REGION[a1o], a2o, a3r, a3o);
   //not call (addresses)
   //else if(a3o != -1) printf("\t%s\t %s:%d,%s:%d,%s:%d\n", opname, a1r, a1o, a2r, a2o, a3r, a3o);
   //else if (a2o != -1) printf("\t%s\t %s:%d,%s:%d\n", opname, a1r, a1o, a2r, a2o);
   //else printf("\t%s\t %s:%d\n", opname, a1r, a1o);
}

int alloc_string(char *str){
   //alloc
   if(STR_REGION == NULL) {
      STR_REGION = malloc(sizeof(STR_REGION));  
      if(STR_REGION == NULL){
         fprintf(stderr, "Out of memory\n");
         exit(4);
      }
   } else {
      STR_REGION = realloc(STR_REGION, sizeof(STR_REGION) * (STR_INDEX + 1)); 
      if(STR_REGION == NULL){
         fprintf(stderr, "Out of memory\n");
         exit(4);
      }
   }
   //copy
   STR_REGION[STR_INDEX] = str;
   int ret = STR_SIZE;
   STR_SIZE += strlen(str) + 1;
   STR_INDEX++;
   return ret;
}


struct addr *alloc_float(double d){
   NUM_FLOATS++;
   //printf("%d\n", NUM_FLOATS);
   //alloc
   if(FLT_REGION == NULL) {
      FLT_REGION = calloc(sizeof(double) * NUM_FLOATS, sizeof(double));  
      if(FLT_REGION == NULL){
         fprintf(stderr, "Out of memory\n");
         exit(4);
      }
   } else {
      FLT_REGION = realloc(FLT_REGION, sizeof(double) * NUM_FLOATS); 
      if(FLT_REGION == NULL){
         fprintf(stderr, "Out of memory\n");
         exit(4);
      }
   }
   //copy
   FLT_SIZE += 8;
   FLT_REGION[NUM_FLOATS-1] = d;
   return address(NULL, R_FLOAT, FLT_SIZE - 8, alctype(F64_TYPE));
}

/*
void alloc_lable(char *str){
   int start;
   //alloc
   if(LBL_REGION == NULL) {
      LBL_REGION = calloc(0, strlen(str) + 1);  
      start = 0;
   } else {
      LBL_REGION = realloc(LBL_REGION, strlen(LBL_REGION) + strlen(str) + 2); 
      start = strlen(LBL_REGION)+1;
   }
   //copy
   for(int i = 0; i < strlen(str)+1; i ++) {
      LBL_REGION[start + i] = str[i];
   }
   LBL_SIZE += strlen(str) + 1;
}
*/


///////////////////////
///////////////////////
////list operations////
///////////////////////
///////////////////////


struct instr *gen(int op, struct addr *a1, struct addr *a2, struct addr *a3){
  struct instr *rv = malloc(sizeof (struct instr));
  if (rv == NULL) {
     fprintf(stderr, "out of memory\n");
     exit(4);
     }
  rv->opcode = op;
  rv->dest = a1;
  rv->src1 = a2;
  rv->src2 = a3;
  rv->next = NULL;
  return rv;
}

struct instr *copylist(struct instr *l){
   if (l == NULL) return NULL;
   struct instr *lcopy = gen(l->opcode, l->dest, l->src1, l->src2);
   lcopy->next = copylist(l->next);
   return lcopy;
}

struct instr *append(struct instr *l1, struct instr *l2){
   if (l1 == NULL) return l2;
   struct instr *ltmp = l1;
   while(ltmp->next != NULL) ltmp = ltmp->next;
   ltmp->next = l2;
   return l1;
}

struct instr *concat(struct instr *l1, struct instr *l2){
   return append(copylist(l1), l2);
}



//test

void tacprint_test(struct instr *start, SymbolTable symtab){
   print_string_region(stdout);
   print_float_region(stdout);
   
   printf(".code\n");
   printf("%s:\n", symtab->name);
   struct instr *iter = start;
   do{
      print_item(iter, stdout);
      iter = iter->next;
   } while(iter != NULL);
   //print_labels();
}

void ictest(SymbolTable s){
   struct instr *start = gen(O_ASN, address(NULL, R_LOCAL, 0, NULL), address(NULL, R_CONST, 5, NULL), NULL);
   append(start, gen(O_MUL, address(NULL, R_LOCAL, 8, NULL), address(NULL, R_LOCAL, 8, NULL), address(NULL, R_LOCAL, 8, NULL)));
   append(start, gen(O_ADD, address(NULL, R_LOCAL, 16, NULL), address(NULL, R_LOCAL, 8, NULL), address(NULL, R_CONST, 1, NULL)));
   append(start, gen(O_ASN, address(NULL, R_LOCAL, 0, NULL), address(NULL, R_LOCAL, 16, NULL), NULL));
   append(start, gen(O_PARM, address(NULL, R_LOCAL, 0, NULL), NULL, NULL));
   append(start, gen(O_PARM, address(NULL, R_STRING, 0, NULL), NULL, NULL));
   append(start, gen(O_CALL, address("Foo", R_CONST, 0, NULL), address(NULL, R_CONST, 2, NULL), address(NULL, R_LOCAL, 24, NULL)));

   tacprint_test(start, s);
}