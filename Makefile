CC=gcc
CFLAGS=-g
TARGET:test.exe
LIBS=-lpthread -L ./CommandParser -lcli
OBJS=glthreads_lib/glthread.o \
		  graph.o 		   \
		  topologies.o	\
		  net.o			\
		  utils.o

test.exe:testapp.o ${OBJS}	CommandParser/libcli.a
	${CC} ${CFLAGS} testapp.o ${OBJS} -o test.exe ${LIBS}

testapp.o:testapp.c
	${CC} ${CFLAGS} -c testapp.c -o testapp.o

glthreads_lib/glthread.o:glthreads_lib/glthread.c
	${CC} ${CFLAGS} -c -I gluethread glthreads_lib/glthread.c -o glthreads_lib/glthread.o
graph.o:graph.c
	${CC} ${CFLAGS} -c -I . graph.c -o graph.o
topologies.o:topologies.c
	${CC} ${CFLAGS} -c -I . topologies.c -o topologies.o
net.o:net.c
	${CC} ${CFLAGS} -c -I . net.c -o net.o
utils.o:utils.c
	${CC} ${CFLAGS} -c -I . utils.c -o utils.o
CommandParser/libcli.a
	(cd CommandParser;make)

clean:
	rm *.o
	rm glthreads_lib/glthread.o
	rm *exe
	(cd CommandParser;make clean)
all:
	make
	(cd CommandParser;make)