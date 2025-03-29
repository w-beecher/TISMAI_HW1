#include "dot.h"
#include <stdlib.h>
#include <string.h>

////////////////////////////////////
//this is for printing tree images//
////////////////////////////////////

char *escape(char *s) {
    char *s2 = malloc(strlen(s)+4);
    if (s[0] == '\"') {
        sprintf(s2, "\\%s", s);
        s2[strlen(s2)-1] = '\0';
        strcat(s2+strlen(s2)-1, "\\\"");
        return s2;
    }
    else return s;
}

char *pretty_print_name(struct tree *t) {
    char *s2 = malloc(40);
    if (t->leaf == NULL) {
        sprintf(s2, "%s#%d", t->symbolname, t->prodrule%10);
        return s2;
    }
    else {
        sprintf(s2,"%s:%d", escape(t->leaf->text), t->leaf->category);
        return s2;
    }
}

void print_branch(struct tree *t, FILE *f) {
    fprintf(f, "N%d [shape=box label=\"%s\"];\n", t->id, pretty_print_name(t));
}

void print_leaf(struct tree *t, FILE *f) {
    const char * s = yyname(t->leaf->category);
    // print_branch(t, f);
    fprintf(f, "N%d [shape=box style=dotted label=\" %s \\n ", t->id, s);
    fprintf(f, "text = %s \\l lineno = %d \\l\"];\n", escape(t->leaf->text), t->leaf->lineno);
}

void print_graph2(struct tree *t, FILE *f) {
    int i;
    if (t->leaf != NULL) {
        print_leaf(t, f);
        return;
        }
    /* not a leaf ==> internal node */
    print_branch(t, f);
    for(i=0; i < t->nkids; i++) {
        if (t->kids[i] != NULL) {
            fprintf(f, "N%d -> N%d;\n", t->id, t->kids[i]->id);
            print_graph2(t->kids[i], f);
        }
        else { /* NULL kid, epsilon production or something */
            fprintf(f, "N%d -> N%d%d;\n", t->id, t->id, serial);
            fprintf(f, "N%d%d [label=\"%s\"];\n", t->id, serial, "Empty rule");
            serial++;
        }
    }
}

void print_graph(struct tree *t, char *filename){
    FILE *f = fopen(filename, "w"); /* should check for NULL */
    fprintf(f, "digraph {\n");
    print_graph2(t, f);
    fprintf(f,"}\n");
    fclose(f);
}