CC=g++
LD=g++
LINKGSL=-lgsl -lgslcblas -lm
#DEBUG=-g
OPTIMIZE=-O2
library=libtwopunctures.a
# enable parallelization
#OMP=-fopenmp

INC=-I../
STD=--std=c++11

${library}: all
	ar rcs $@ *.o

all:
	${CC} -c ${DEBUG} ${OMP} ${OPTIMIZE} ${INC} ${STD} TP_Logging.cpp TP_Parameters.cpp Equations.cpp FuncAndJacobian.cpp TP_Utilities.cpp Newton.cpp TwoPunctures.cpp
	# we have a GSL dependency, make sure you have the GSL headers installed
	${CC} -c ${DEBUG} ${OMP} ${OPTIMIZE} ${INC} ${STD} CoordTransf.cpp

clean:
	rm -f *.o ${library}
