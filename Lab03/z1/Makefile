CC=gcc
AR=ar
ARFLAGS=cr

main: main.c libgroup.a
	${CC} -o main main.c -L. -lgroup

libgroup.a: lib.o
	${AR} ${ARFLAGS} libgroup.a lib.o

%.o: %.c
	${CC} -c $<