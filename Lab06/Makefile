CC=gcc
AR=ar
ARFLAGS=cr

main: main.c libmemg.a
	${CC} -o main main.c -L. -lmemg

libmemg.a: lib.o
	${AR} ${ARFLAGS} libmemg.a lib.o

%.o: %.cr
	%{CC} -c $<
