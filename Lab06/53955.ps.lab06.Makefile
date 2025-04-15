CC=gcc
AR=ar
ARFLAGS=cr

main: 53955.ps.lab06.main.c libmemg.a
	${CC} -o main 53955.ps.lab06.main.c -L. -lmemg

libmemg.a: 53955.ps.lab06.lib.o
	${AR} ${ARFLAGS} libmemg.a 53955.ps.lab06.lib.o

%.o: %.c
	%{CC} -c $<
