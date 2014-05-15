
.SUFFIXES=.c .h .o .l .y .tab.c

FLEX=flex
BISON=bison
CC=gcc
LD=gcc

#CCFLAGS=-c -pg -g -Wall -DYYDEBUG=1 -DYYERROR_VERBOSE=1 -D PROC_TRACE
#CCFLAGS=-c -g -Wall -DYYDEBUG=1 -DYYERROR_VERBOSE=1 -D DEBUG_MEM -D RUNTIME_STAT -D PROC_TRACE
CCFLAGS=-c -O2 -DNDEBUG -DYYDEBUG=1 -DYYERROR_VERBOSE=1 #-D DEBUG_MEM #-D RUNTIME_STAT #-DPROC_TRACE
#CCFLAGS=-c -g -DNDEBUG -DYYDEBUG=1 -DYYERROR_VERBOSE=1 -D DEBUG_MEM #-D RUNTIME_STAT #-DPROC_TRACE
LDFLAGS=  -lm #-pg -lc_p 

EXDBMLIB = ./lib/eXdbm/libeXdbm.a

BISONFLAGS= -t -d -v 
FLEXARGS= 

INCPATH= ./lib/eXdbm

LIBPATH= ./lib/eXdbm

LIBSRCS= cubealloc.c cubemisc.c cubeast.c cubeprim.c cubestr.c cubecfg.c cubestat.c cubedef.c cubeval.c cubechan.c cubetuple.c cubesched.c

LIBOBJS= cubealloc.o cubemisc.o cubeast.o cubeprim.o cubestr.o cubecfg.o cubestat.o cubedef.o cubeval.o cubechan.o cubetuple.o cubesched.o

PARSEOBJS= cubeparse.tab.o lex.yy.o

all: copilot

copilot: $(PARSEOBJS) copilot.o $(LIBOBJS)
	$(LD) $(LDFLAGS) $^ $(EXDBMLIB) -o $@	

cubetest2: cubetest2.o $(LIBOBJS)
	$(LD) $(LDFLAGS) $^ -o $@	

cubetest3: cubetest3.o $(LIBOBJS)
	$(LD) $(LDFLAGS) $^ -o $@	

lex.yy.c : cubeparse.l
	$(FLEX) $(FLEXARGS) $<

cubeparse.tab.c : cubeparse.y
	$(BISON) $(BISONFLAGS) $<

.y.c:
	$(BISON) $(BISONFLAGS) $<

.c.o:
	$(CC) $(CCFLAGS) -I $(INCPATH) $< -o $@

clean:
	rm -f *.o
	rm -f *~
	rm -f examples/*~
	rm -f bench/*~
	rm -f cubeparse.tab.c
	rm -f cubeparse.tab.h
	rm -f lex.yy.c

cleanall: clean
	rm -f copilot
	rm -f gmon.out
	rm -f gprof.mon
	rm -f cubeparse.output
	rm -f core.*
	rm -f ast.output
