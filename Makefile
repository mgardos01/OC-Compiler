DEPSFILE  = Makefile.deps
NOINCLUDE = ci clean spotless
NEEDINCL  = ${filter ${NOINCLUDE}, ${MAKECMDGOALS}}
WARNINGS  = -Wall -Wextra -Wpedantic -Wshadow -Wold-style-cast
GPPOPTS   = -std=gnu++2a -g -O0 -fdiagnostics-color=never
GPP       = g++ ${GPPOPTS} ${WARNINGS}
MKDEPS    = g++ -std=gnu++2a -MM

MODULES   = astree auxlib cpp_pipe debug lyutils symbols emit
HDRSRC    = ${MODULES:=.h}
CPPSRC    = ${MODULES:=.cpp} main.cpp
FLEXSRC   = scanner.l
BISONSRC  = parser.y
PARSEHDR  = yyparse.h
LEXCPP    = yylex.cpp
PARSECPP  = yyparse.cpp
CGENS     = ${LEXCPP} ${PARSECPP}
ALLGENS   = ${PARSEHDR} ${CGENS}
EXECBIN   = oc
ALLCSRC   = ${CPPSRC} ${CGENS}
OBJECTS   = ${ALLCSRC:.cpp=.o}
YYREPORT  = yyparse.output
MODSRC    = ${foreach MOD, ${MODULES}, ${MOD}.h ${MOD}.cpp}
MISCSRC   = ${filter-out ${MODSRC}, ${HDRSRC} ${CPPSRC}}
ALLSRC    = ${FLEXSRC} ${BISONSRC} ${MODSRC} ${MISCSRC} Makefile
LISTSRC   = ${ALLSRC} ${DEPSFILE} ${PARSEHDR}

export PATH := ${PATH}:/afs/cats.ucsc.edu/courses/cse110a-wm/bin

all : ${EXECBIN}

${EXECBIN} : ${OBJECTS}
	${GPP} -o${EXECBIN} ${OBJECTS}

%.o : %.cpp
	- [[ $< == yy*.cpp ]] || cpplint.py.perl $<
	${GPP} -c $<

${LEXCPP} : ${FLEXSRC}
	flex --outfile=${LEXCPP} ${FLEXSRC}

${PARSECPP} ${PARSEHDR} : ${BISONSRC}
	bison --defines=${PARSEHDR} --output=${PARSECPP} ${BISONSRC}

ci : ${ALLSRC}
	- cpplint.py.perl ${CPPSRC}
	- checksource ${ALLSRC}
	cid -is ${ALLSRC}

clean :
	rm -r symbol_files token_files \
	ast_files valgrind_files ocil_files \
	*.symbols *.tokens *.ast *.ocil \
	${OBJECTS} ${ALLGENS} \
	${YYREPORT} ${DEPSFILE} 

spotless : clean
	- rm ${EXECBIN}

deps : ${ALLCSRC}
	@ echo "# ${DEPSFILE} created $$(LC_TIME=C date)" >${DEPSFILE}
	${MKDEPS} ${ALLCSRC} >>${DEPSFILE}

${DEPSFILE} :
	@ touch ${DEPSFILE}
	${MAKE} --no-print-directory deps

again :
	gmake --no-print-directory spotless deps ci all
	
ifeq "${NEEDINCL}" ""
include ${DEPSFILE}
endif

