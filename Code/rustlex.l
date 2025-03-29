/* -*-bison-*- */
%option noyywrap
%option noinput
%option nounput
%option yylineno
%pointer
%{
			#include "tree.h"
			#include "type.h"
			#include "tac.h"
			#include "feGrammar.tab.h"
			//these are included for range checking
			#include <limits.h>
			#include <float.h>
        	#include <errno.h>
			#include <math.h>

        	int cur_line = 1;
        	struct token *yytoken = NULL;
        	char *deescape(char *text);
	    	int newToken(int cat);
			void checkRange(int cat);

	    	extern char *targetfile;
			extern YYSTYPE yylval;
			extern int serial;

			extern struct addr *address(char *name, int region, int offset, struct typeinfo *type);
			extern typeptr alctype(int);
%}

%%
<<EOF>>	{ cur_line = 1; return -1; }
[\t ]+	{  }
\/\/.*	{  }
\n	{ cur_line++; }

[0-9]\.[0-9]*e-?[0-9]+	{ fprintf(stderr, "Error: \"%s\" at line %d; E-Notation not supported in Irony\n", yytext, cur_line); exit(1); }
[0-9]e-?[0-9]+	{ fprintf(stderr, "Error: \"%s\" at line %d; E-Notation not supported in Irony\n", yytext, cur_line); exit(1); }
"-"?[0-9]+	{ checkRange(LIT_INTEGER); return newToken(LIT_INTEGER); }
("NULL"|"null")	{ yytext = "0"; return newToken(LIT_INTEGER); }

0b[0-9]*	{ fprintf(stderr, "Error: \"%s\" at line %d; Binary literals not supported in Irony\n", yytext, cur_line); exit(1); }
0o[0-9]*	{ fprintf(stderr, "Error: \"%s\" at line %d; Octal literals not supported in Irony\n", yytext, cur_line); exit(1); }
0x[a-zA-Z0-9]*	{ fprintf(stderr, "Error: \"%s\" at line %d; Hex literals not supported in Irony\n", yytext, cur_line); exit(1); }
"-"?[0-9]+\.[0-9]+	{ checkRange(LIT_FLOAT); return newToken(LIT_FLOAT); }

\"([^"\n]|("\\\""))*(\\[^ntr\'\"]).*\"	{ fprintf(stderr, "Error: \"%s\" at line %d; An escape sequence attached to this string literal is either invalid or unsupported\n", yytext, cur_line); exit(1); }
\"([^"\n]|("\\\""))*\\\"	{ fprintf(stderr, "Error: \"%s\" at line %d; String literal closing quotes are escaped\n", yytext, cur_line); exit(1); }
\"(([^"\n]|("\\\""))*[^\\])?\"	{ return newToken(LIT_STR); }

br\#\"([^\"\\]|\\[nrt\'\"\\])*\"\#	{ fprintf(stderr, "Error: \"%s\" at line %d; Raw byte strings not accepted\n", yytext, cur_line); exit(1); }
r\#\"([^\"\\]|\\[nrt\'\"\\])*\"\#	{ fprintf(stderr, "Error: \"%s\" at line %d; Byte strings not accepted\n", yytext, cur_line); exit(1); }

\'"\\u{"[a-fA-F0-9][a-fA-F0-9][a-fA-F0-9][a-fA-F0-9]"}"\'	{ fprintf(stderr, "Error: \"%s\" at line %d; Unicode characters are not supported\n", yytext, cur_line); exit(1); }
(\'.\')|(\'(\\[ntr\'\"])\')	{ return newToken(LIT_CHAR); }
(\'.\')|(\'(\\[^ntr\'\"])\')	{ fprintf(stderr, "Error: \"%s\" at line %d; Char literal contains unsupported escape character\n", yytext, cur_line); exit(1); }
\'..+\'	{ fprintf(stderr, "Error: \"%s\" at line %d; Char literal contains more than one character\n", yytext, cur_line); exit(1); }

"True"|"TRUE"	{ fprintf(stderr, "Error: \"%s\" at line %d; Please use lowercase for boolean literals\n", yytext, cur_line); exit(1); }
"False"|"FALSE"	{ fprintf(stderr, "Error: \"%s\" at line %d; Please use lowercase for boolean literals\n", yytext, cur_line); exit(1); }
true	{ return newToken(TRUE); }
false	{ return newToken(FALSE); }

async	{ fprintf(stderr, "Error: Token \"%s\" at line %d represents a Rust feature not in Irony\n", yytext, cur_line); exit(1); }
await	{ fprintf(stderr, "Error: Token \"%s\" at line %d represents a Rust feature not in Irony\n", yytext, cur_line); exit(1); }
dyn	{ fprintf(stderr, "Error: Token \"%s\" at line %d represents a Rust feature not in Irony\n", yytext, cur_line); exit(1); }
abstract	{ fprintf(stderr, "Error: Token \"%s\" at line %d represents a Rust feature not in Irony\n", yytext, cur_line); exit(1); }
become	{ fprintf(stderr, "Error: Token \"%s\" at line %d represents a Rust feature not in Irony\n", yytext, cur_line); exit(1); }
box	{ fprintf(stderr, "Error: Token \"%s\" at line %d represents a Rust feature not in Irony\n", yytext, cur_line); exit(1); }
final	{ fprintf(stderr, "Error: Token \"%s\" at line %d represents a Rust feature not in Irony\n", yytext, cur_line); exit(1); }
macro	{ fprintf(stderr, "Error: Token \"%s\" at line %d represents a Rust feature not in Irony\n", yytext, cur_line); exit(1); }
override	{ fprintf(stderr, "Error: Token \"%s\" at line %d represents a Rust feature not in Irony\n", yytext, cur_line); exit(1); }
priv	{ fprintf(stderr, "Error: Token \"%s\" at line %d represents a Rust feature not in Irony\n", yytext, cur_line); exit(1); }
typeof	{ fprintf(stderr, "Error: Token \"%s\" at line %d represents a Rust feature not in Irony\n", yytext, cur_line); exit(1); }
unsized	{ fprintf(stderr, "Error: Token \"%s\" at line %d represents a Rust feature not in Irony\n", yytext, cur_line); exit(1); }
virtual	{ fprintf(stderr, "Error: Token \"%s\" at line %d represents a Rust feature not in Irony\n", yytext, cur_line); exit(1); }
yield	{ fprintf(stderr, "Error: Token \"%s\" at line %d represents a Rust feature not in Irony\n", yytext, cur_line); exit(1); }
try	{ fprintf(stderr, "Error: Token \"%s\" at line %d represents a Rust feature not in Irony\n", yytext, cur_line); exit(1); }
macro_rules	{ fprintf(stderr, "Error: Token \"%s\" at line %d represents a Rust feature not in Irony\n", yytext, cur_line); exit(1); }
union	{ fprintf(stderr, "Error: Token \"%s\" at line %d represents a Rust feature not in Irony\n", yytext, cur_line); exit(1); }
as	{ fprintf(stderr, "Error: Token \"%s\" at line %d represents a Rust feature not in Irony\n", yytext, cur_line); exit(1); }
continue	{ fprintf(stderr, "Error: Token \"%s\" at line %d represents a Rust feature not in Irony\n", yytext, cur_line); exit(1); }
crate	{ fprintf(stderr, "Error: Token \"%s\" at line %d represents a Rust feature not in Irony\n", yytext, cur_line); exit(1); }
enum	{ fprintf(stderr, "Error: Token \"%s\" at line %d represents a Rust feature not in Irony\n", yytext, cur_line); exit(1); }
extern	{ fprintf(stderr, "Error: Token \"%s\" at line %d represents a Rust feature not in Irony\n", yytext, cur_line); exit(1); }
impl	{ fprintf(stderr, "Error: Token \"%s\" at line %d represents a Rust feature not in Irony\n", yytext, cur_line); exit(1); }
match	{ fprintf(stderr, "Error: Token \"%s\" at line %d represents a Rust feature not in Irony\n", yytext, cur_line); exit(1); }
mod	{ fprintf(stderr, "Error: Token \"%s\" at line %d represents a Rust feature not in Irony\n", yytext, cur_line); exit(1); }
move	{ fprintf(stderr, "Error: Token \"%s\" at line %d represents a Rust feature not in Irony\n", yytext, cur_line); exit(1); }
pub	{ fprintf(stderr, "Error: Token \"%s\" at line %d represents a Rust feature not in Irony\n", yytext, cur_line); exit(1); }
self | Self	{ fprintf(stderr, "Error: Token \"%s\" at line %d represents a Rust feature not in Irony\n", yytext, cur_line); exit(1); }
struct	{ fprintf(stderr, "Error: Token \"%s\" at line %d represents a Rust feature not in Irony\n", yytext, cur_line); exit(1); }
super	{ fprintf(stderr, "Error: Token \"%s\" at line %d represents a Rust feature not in Irony\n", yytext, cur_line); exit(1); }
trait	{ fprintf(stderr, "Error: Token \"%s\" at line %d represents a Rust feature not in Irony\n", yytext, cur_line); exit(1); }
type	{ fprintf(stderr, "Error: Token \"%s\" at line %d represents a Rust feature not in Irony\n", yytext, cur_line); exit(1); }
unsafe	{ fprintf(stderr, "Error: Token \"%s\" at line %d represents a Rust feature not in Irony\n", yytext, cur_line); exit(1); }
use	{ fprintf(stderr, "Error: Token \"%s\" at line %d represents a Rust feature not in Irony\n", yytext, cur_line); exit(1); }
where	{ fprintf(stderr, "Error: Token \"%s\" at line %d represents a Rust feature not in Irony\n", yytext, cur_line); exit(1); }
catch	{ fprintf(stderr, "Error: Token \"%s\" at line %d represents a Rust feature not in Irony\n", yytext, cur_line); exit(1); }
offset_of	{ fprintf(stderr, "Error: Token \"%s\" at line %d represents a Rust feature not in Irony\n", yytext, cur_line); exit(1); }
pure	{ fprintf(stderr, "Error: Token \"%s\" at line %d represents a Rust feature not in Irony\n", yytext, cur_line); exit(1); }
size_of	{ fprintf(stderr, "Error: Token \"%s\" at line %d represents a Rust feature not in Irony\n", yytext, cur_line); exit(1); }
default	{ fprintf(stderr, "Error: Token \"%s\" at line %d represents a Rust feature not in Irony\n", yytext, cur_line); exit(1); }

break	{ return newToken(BREAK); }
const	{ return newToken(CONST); }
else	{ return newToken(ELSE); }
fn	{ return newToken(FN); }
for	{ return newToken(FOR); }
if	{ return newToken(IF); }
in	{ return newToken(IN); }
let	{ return newToken(LET); }
mut	{ return newToken(MUT); }
ref	{ return newToken(REF); }
return	{ return newToken(RETURN); }
static	{ return newToken(STATIC); }
while	{ return newToken(WHILE); }
do	{ return newToken(DO); }
loop	{ return newToken(LOOP); }

String	{ return newToken(STRING); }
str	{ fprintf(stderr, "Error: Token \"%s\" at line %d represents a Rust feature not in Irony\n", yytext, cur_line); exit(1); }
i32	{ fprintf(stderr, "Error: Token \"%s\" at line %d represents a Rust feature not in Irony\n", yytext, cur_line); exit(1); } 
i64	{ return newToken(I64); } 
f32	{ fprintf(stderr, "Error: Token \"%s\" at line %d represents a Rust feature not in Irony\n", yytext, cur_line); exit(1); } 
f64	{ return newToken(F64); } 
bool	{ return newToken(BOOL); } 

">>="	{ fprintf(stderr, "Error: Token \"%s\" at line %d represents a Rust feature not in Irony\n", yytext, cur_line); exit(1); }
"<<="	{ fprintf(stderr, "Error: Token \"%s\" at line %d represents a Rust feature not in Irony\n", yytext, cur_line); exit(1); }
"<<"	{ fprintf(stderr, "Error: Token \"%s\" at line %d represents a Rust feature not in Irony\n", yytext, cur_line); exit(1); }
">>"	{ fprintf(stderr, "Error: Token \"%s\" at line %d represents a Rust feature not in Irony\n", yytext, cur_line); exit(1); }
"&="	{ fprintf(stderr, "Error: Token \"%s\" at line %d represents a Rust feature not in Irony\n", yytext, cur_line); exit(1); }
"|="	{ fprintf(stderr, "Error: Token \"%s\" at line %d represents a Rust feature not in Irony\n", yytext, cur_line); exit(1); }
"^="	{ fprintf(stderr, "Error: Token \"%s\" at line %d represents a Rust feature not in Irony\n", yytext, cur_line); exit(1); }
"**"	{ fprintf(stderr, "Error: Token \"%s\" at line %d is not a valid operator\n", yytext, cur_line); exit(1); }


"+="	{ return newToken(PLUSEQ); }
"-="	{ return newToken(MINUSEQ); }
"=="	{ return newToken(EQEQ); }
"!="	{ return newToken(NE); }
">="	{ return newToken(GE); }
"<="	{ return newToken(LE); }
"&&"	{ return newToken(ANDAND); }
"||"	{ return newToken(OROR); }
"->"	{ return newToken(RARROW); }
"<-"	{ return newToken(LARROW); }
".."	{ return newToken(DOTDOT); }
"..."	{ return newToken(DOTDOTDOT); }
"::"	{ return newToken(MOD_SEP); }

"%"	{ return newToken('%'); }
"="	{ return newToken('='); }
"+"	{ return newToken('+'); }
"-"	{ return newToken('-'); }
"*"	{ return newToken('*'); }
">"	{ return newToken('>'); }
"<"	{ return newToken('<'); }
"["	{ return newToken('['); }
"]"	{ return newToken(']'); }
"."	{ return newToken('.'); }
"("	{ return newToken('('); }
")"	{ return newToken(')'); }
","	{ return newToken(','); }
"{"	{ return newToken('{'); }
"}"	{ return newToken('}'); }
":"	{ return newToken(':'); }
";"	{ return newToken(';'); }
"!"	{ return newToken('!'); }
"/"	{ return newToken('/'); }

"\'"	{ fprintf(stderr, "Error: Token \"%s\" at line %d is either an unclosed character literal, or represents a Rust feature not in Irony\n", yytext, cur_line); exit(1); }
"~"	{ fprintf(stderr, "Error: Token \"%s\" at line %d represents a Rust feature not in Irony\n", yytext, cur_line); exit(1); }
"^"	{ fprintf(stderr, "Error: Token \"%s\" at line %d represents a Rust feature not in Irony\n", yytext, cur_line); exit(1); }
"&"	{ fprintf(stderr, "Error: Token \"%s\" at line %d represents a Rust feature not in Irony\n", yytext, cur_line); exit(1); }
"|"	{ fprintf(stderr, "Error: Token \"%s\" at line %d represents a Rust feature not in Irony\n", yytext, cur_line); exit(1); }

[0-9][a-zA-Z0-9_]*	{ fprintf(stderr, "Error: \"%s\" at line %d is an invalid token. You may be missing an operator in this block, or defining an identifier using improper syntax (identifiers cannot start with a number)\n", yytext, cur_line); exit(1); }
[a-zA-Z_][a-zA-Z0-9_]*	{ return newToken(IDENT); }

.	{ fprintf(stderr, "Error: unexpected chracter %s at line %d\n", yytext, cur_line); exit(1); }

%%
char *deescape(char *text){
	//remove quotation marks at either end of the text
	int len = strlen(text);
	char buf1[len+1];
	strcpy(buf1, text);
	if(len == 2){
		char *ret = ckalloc(1);
		strcpy(ret, "");
		return ret;
	} else {
		for(int i = 0; i < len-1; i++) {
			buf1[i] = buf1[i+1];	
		}
		buf1[len-2] = '\0';
	}
	//re-encode escape
	len = strlen(buf1);
	char buf2[len+1];
	int i = 0;
	int j = 0;
	while(i < len){
		//escape found
		if(buf1[i] == '\\'){
			i++;
			//while technically this has problems for strings ending in \, selection regex for that specific edge case makes it impossible
			switch(buf1[i]){
				case 'n':
					buf2[j] = '\n';
					break;
				case 't':
					buf2[j] = '\t';
					break;
				case 'r':
					buf2[j] = '\r';
					break;
				case '\'':
					buf2[j] = '\'';
					break;
				case '\"':
					buf2[j] = '\"';
					break;
				case '\\':
					buf2[j] = '\\';
					break;
				//no default needed as these are the only possible cases for the regex
			}
		}
		//copy the character over
		else {
			buf2[j] = buf1[i];
		}
		//inc
		i++;
		j++;
	}
	buf2[j] = '\0';
	//copy buf2 into a string pointer that is dynamically allocated
	char *ret = ckalloc(strlen(buf2)+1);
	strcpy(ret, buf2);
	return ret;
}

int newToken(int cat){
	/////// tree alloc ///////
	yylval.treeptr = ckalloc(sizeof (struct tree));
	yylval.treeptr->prodrule = cat;
	yylval.treeptr->symbolname = "Terminal";
	yylval.treeptr->nkids = -1;
	yylval.treeptr->id = serial++;
	yylval.treeptr->scope = NULL;

	//type for literals
	yylval.treeptr->type = NULL;

	//fun times
	yylval.treeptr->addr = NULL;
	yylval.treeptr->icode = NULL;
	yylval.treeptr->first = NULL;
	yylval.treeptr->follow = NULL;
	yylval.treeptr->ontrue = NULL;
	yylval.treeptr->onfalse = NULL;
	yylval.treeptr->arrbase = NULL;

	/////// token alloc ///////
	yylval.treeptr->leaf = ckalloc(sizeof(struct token));
	//text
	yylval.treeptr->leaf->text = ckalloc(strlen(yytext)+1);
	strcpy(yylval.treeptr->leaf->text, yytext);
	//category
	yylval.treeptr->leaf->category = cat;
	//value (if neccessary)
	switch(cat){
		case LIT_STR:
		case LIT_CHAR:
			//deescape gives a pre-allocated string pointer
			yylval.treeptr->leaf->sval = deescape(yytext);
			yylval.treeptr->type = alctype(STRING_TYPE);
			yylval.treeptr->addr = address(NULL, R_STRING, alloc_string(deescape(yytext)), yylval.treeptr->type);
			break;
		case LIT_INTEGER:
			yylval.treeptr->leaf->ival = atoi(yytext);
			yylval.treeptr->type = alctype(I64_TYPE);
			yylval.treeptr->addr = address(NULL, R_CONST, atoi(yytext), yylval.treeptr->type);
			break;
		case LIT_FLOAT:
			yylval.treeptr->leaf->dval = strtod(yytext, NULL);
			yylval.treeptr->type = alctype(F64_TYPE);
			yylval.treeptr->addr = alloc_float(strtod(yytext, NULL));
			break;	
		case TRUE:
			yylval.treeptr->leaf->bval = 1;
			yylval.treeptr->type = alctype(BOOL_TYPE);
			yylval.treeptr->addr = address(NULL, R_CONST, 1, yylval.treeptr->type);
			break;
		case FALSE:
			yylval.treeptr->leaf->bval = 0;
			yylval.treeptr->type = alctype(BOOL_TYPE);
			yylval.treeptr->addr = address(NULL, R_CONST, 0, yylval.treeptr->type);
			break;
	}
	//filename
	yylval.treeptr->leaf->filename = ckalloc(strlen(targetfile)+1);
	strcpy(yylval.treeptr->leaf->filename, targetfile);
	//line
	yylval.treeptr->leaf->lineno = cur_line;
	//return int category for simplicity
	return cat;
}

void checkRange(int cat){
	errno = 0;
	switch(cat){
		//integers are by default checked as LIT_I32
		case LIT_INTEGER:
		//case I32:
			long ivalue = strtol(yytext, 0, 10);
			if ((ivalue >= INT_MAX)){
				fprintf(stderr, "Error: \"%s\" at line %d; Integer literal is largeer that its type (i32) maximum\n", yytext, cur_line); 
				exit(1); 
			} else if ((ivalue <= INT_MIN)) {
				fprintf(stderr, "Error: \"%s\" at line %d; Integer literal is smaller than its type (i32) minimum\n", yytext, cur_line); 
				exit(1); 
			}
			break;
		//technically speaking this cannot happen, since the regex cannot tell the difference between i32 and i64.  here just in case.
		//case I64:
		//	long ilvalue = strtol(yytext, 0, 10);
		//	if ((errno == ERANGE) && (ilvalue >= LONG_MAX)){
		//		fprintf(stderr, "Error: \"%s\" at line %d; Integer literal is largeer that its type (i64) maximum\n", yytext, cur_line); 
		//		exit(1); 
		//	} else if ((errno == ERANGE) && (ilvalue <= LONG_MIN)) {
		//		fprintf(stderr, "Error: \"%s\" at line %d; Integer literal is smaller than its type (i64) minimum\n", yytext, cur_line); 
		//		exit(1); 
		//	}
		//	break;
		//same as i64.
		//case F32:
		//	float fvalue = strtof(yytext, NULL);
		//	if ((errno == ERANGE) && (fvalue == ULONG_MAX)){
		//		fprintf(stderr, "Error: \"%s\" at line %d; Float literal is largeer that its type (f34) maximum\n", yytext, cur_line); 
		//		exit(1); 
		//	} else if ((errno == ERANGE) && (fvalue <= FLT_MIN)) {
		//		fprintf(stderr, "Error: \"%s\" at line %d; Float literal is smaller than its type (f32) minimum\n", yytext, cur_line); 
		//		exit(1); 
		//	}
		//	break;
		//floats are by default checked as LIT_F64
		case LIT_FLOAT:
		//case F64:
			double lfvalue = strtod(yytext, NULL);
			if ((errno == ERANGE) && (lfvalue >= HUGE_VAL)){
				fprintf(stderr, "Error: \"%s\" at line %d; Float literal is largeer that its type (f64) maximum\n", yytext, cur_line); 
				exit(1); 
			} else if ((errno == ERANGE) && (lfvalue <= DBL_MIN)) {
				fprintf(stderr, "Error: \"%s\" at line %d; Float literal is smaller than its type (f64) minimum\n", yytext, cur_line); 
				exit(1); 
			}
			break;	
	}


}










