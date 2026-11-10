SHELL=/bin/sh
LEX=flex
YACC=bison
CC=gcc
CFLAGS=-g -std=c2x -pedantic -Wall -Werror -Wwrite-strings \
       -DxVERBOSE \
       -DxOPTIMIZE
OPT=
LDFLAGS=
# --nounput: ne génère pas la fonction yyunput() inutile
# --DYY_NO_INPUT: ne prend pas en compte la fonction input() inutile
# -D_POSIX_SOURCE: déclare la fonction fileno()
LEXOPTS=-D_POSIX_SOURCE -DYY_NO_INPUT --nounput
YACCOPTS=

PROG=compil

$(PROG): lex.yy.o $(PROG).tab.o hashtable.o  AST.o commun.o
	$(CC) $+ -o $@ $(LDFLAGS)

lex.yy.c: $(PROG).l $(PROG).tab.h
	$(LEX) $(LEXOPTS) $<

lex.yy.h: $(PROG).l
	$(LEX) $(LEXOPTS) --header-file=$@ $<

$(PROG).tab.c $(PROG).tab.h: $(PROG).y lex.yy.h
	$(YACC) $(YACCOPTS) $< -d -v --graph

%.o: %.c Makefile
	$(CC) $(CFLAGS) $(OPT) $< -c


.PHONY: make1 make2 make3 run clean

TEST=test.tex
ASM=out.asm
OBJ=out.sipro

make1: $(PROG)
	@./$(PROG) < $(TEST) > $(ASM)

make2: make1
	@asipro $(ASM) $(OBJ) > /dev/null 2>&1
     
make3: make2
	@echo ""  
	@sipro $(OBJ)

run: make3
        
clean:
	rm -f $(PROG) *.o *.output *.vcg
	rm -f lex.yy.* $(PROG).tab.*
	rm -f compil.gv $(ASM) $(OBJ) gmon.out

