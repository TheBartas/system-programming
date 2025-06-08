CC=gcc


attack: 53955.ps.lab08.main.c 
	${CC} -pthread -o attack 53955.ps.lab08.main.c -lcrypt