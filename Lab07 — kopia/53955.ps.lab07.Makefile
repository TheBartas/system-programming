CC=gcc
AR=ar
ARFLAGS=cr

main : 53955.ps.lab07.main.c libthread.a libtimer.a
	${CC} -pthread -o main 53955.ps.lab07.main.c -L. -lthread -ltimer

libtimer.a: 53955.ps.lab07.libtimer.o
	${AR} ${ARFLAGS} libtimer.a 53955.ps.lab07.libtimer.o

libthread.a: 53955.ps.lab07.libthread.o
	${AR} ${ARFLAGS} libthread.a 53955.ps.lab07.libthread.o

%.o: %.c
	${CC} -c $<