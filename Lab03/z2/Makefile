CC=gcc
AR=ar
ARFLAGS=cr

main: main.c libgroup.so
	${CC} -o main main.c -L. -ldl -lgroup -Wl,-rpath,.

libgroup.so: lib.o
	${CC} -shared -o libgroup.so lib.o

lib.o: lib.c
	${CC} -c -fPIC -ldl $<