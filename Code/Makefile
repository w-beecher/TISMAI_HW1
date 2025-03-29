CC = gcc

default: full

full: lex.yy.o feGrammar.tab.o main.o dot.o symtab.o hash.o type.o typecheck.o tac.o intercode.o finalcode.o
	$(CC) -g -Wall lex.yy.o main.o feGrammar.tab.o dot.o symtab.o hash.o type.o typecheck.o tac.o intercode.o finalcode.o -ll -o fec

lex.yy.o: lex.yy.c feGrammar.tab.h tree.h type.h tac.h
	$(CC) -g -Wall -c lex.yy.c

lex.yy.c: rustlex.l
	flex rustlex.l

main.o: main.c tree.h feGrammar.tab.h dot.h type.h typecheck.h tac.h
	$(CC) -g -Wall -c main.c -o main.o

feGrammar.tab.h: feGrammar.y
	bison -d feGrammar.y

feGrammar.tab.o: feGrammar.tab.c tree.h tac.h
	$(CC) -g -c -Wall feGrammar.tab.c

feGrammar.tab.c: feGrammar.y
	bison feGrammar.y

dot.o: dot.c dot.h tree.h tac.h
	$(CC) -g -Wall -c dot.c -o dot.o

symtab.o: symtab.c symtab.h tree.h ./hash/ht.h type.h tac.h
	$(CC) -g -Wall -c symtab.c -o symtab.o

hash.o: ./hash/ht.c ./hash/ht.h
	$(CC) -g -Wall -c ./hash/ht.c -o hash.o

type.o : type.c type.h
	$(CC) -g -Wall -c type.c -o type.o

typecheck.o: typecheck.c typecheck.h symtab.h tree.h type.h ./hash/ht.h tac.h
	$(CC) -g -Wall -c typecheck.c -o typecheck.o

tac.o: tac.c tac.h symtab.h rustlex.l type.h
	$(CC) -g -Wall -c tac.c -o tac.o

intercode.o: intercode.c intercode.h tac.h tree.h symtab.h typecheck.h
	$(CC) -g -Wall -c intercode.c -o intercode.o

finalcode.o: finalcode.c finalcode.h tree.h tac.h type.h symtab.h intercode.h ./hash/ht.h
	$(CC) -g -Wall -c finalcode.c -o finalcode.o

clean:
	rm lex.yy.c
	rm lex.yy.o
	rm main.o
	rm feGrammar.tab.o
	rm feGrammar.tab.c
	rm feGrammar.tab.h
	rm dot.o
	rm symtab.o
	rm hash.o
	rm type.o
	rm typecheck.o
	rm intercode.o
	rm finalcode.o
	rm tac.o
	rm fec


