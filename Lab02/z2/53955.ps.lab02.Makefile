CC=gcc

main : 53955.ps.lab02.main.o 53955.ps.lab02.lib.o
	${CC} -o main 53955.ps.lab02.main.o 53955.ps.lab02.lib.o

%.o : %.c 
	${CC} -c $<