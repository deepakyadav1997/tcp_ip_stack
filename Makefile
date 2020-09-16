CC=gcc
CFLAGS=-g
TARGET:test.exe

OBJS=glthreads_lib/glthread.o \
		  graph.o 		   \
		  topologies.o

test.exe:testapp.o ${OBJS}
	${CC} ${CFLAGS} testapp.o ${OBJS} -o test.exe

testapp.o:testapp.c
	${CC} ${CFLAGS} -c testapp.c -o testapp.o

glthreads_lib/glthread.o:glthreads_lib/glthread.c
	${CC} ${CFLAGS} -c -I gluethread glthreads_lib/glthread.c -o glthreads_lib/glthread.o
graph.o:graph.c
	${CC} ${CFLAGS} -c -I . graph.c -o graph.o
topologies.o:topologies.c
	${CC} ${CFLAGS} -c -I . topologies.c -o topologies.o

clean:
	rm *.o
	rm glthreads_lib/glthread.o
	rm *exe