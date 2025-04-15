CC=gcc
AR=ar
ARFLAGS=cr

main : main.c libthread.a libtimer.a
	${CC} -pthread -o main main.c -L. -lthread -ltimer

libtimer.a: timer.o
	${AR} ${ARFLAGS} libtimer.a timer.o

libthread.a: thread.o
	${AR} ${ARFLAGS} libthread.a thread.o

%.o: %.c
	${CC} -c $<