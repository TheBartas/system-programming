CC=gcc
wrk=worker
mng=manager

all: ${mng} ${wrk}

${mng}: ${mng}.c 
	${CC} -o ${mng} ${mng}.c

${wrk}: ${wrk}.c	
	${CC} -o ${wrk} ${wrk}.c

