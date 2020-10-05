CC=gcc
CFLAGS=-g
TARGET:test.exe
LIBS=-lpthread -L ./CommandParser -lcli
OBJS=glthreads_lib/glthread.o \
		  graph.o 		   \
		  topologies.o	\
		  net.o			\
		  utils.o		\
		  nwcli.o		\
		  communication.o\
		  Layer2/layer2.o \
		  pkt_dump.o	  \
		  Layer2/l2switch.o \
		  Layer3/layer3.o	\
		  Layer5/ping.o		\
		  Layer5/layer5.o

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
nwcli.o:nwcli.c
	${CC} ${CFLAGS} -c -I . nwcli.c -o nwcli.o
communication.o:communication.c
	${CC} ${CFLAGS}  -c -I . communication.c -o communication.o 
Layer2/layer2.o:Layer2/layer2.c
	${CC} ${CFLAGS}  -c  Layer2/layer2.c -o Layer2/layer2.o 
pkt_dump.o:pkt_dump.c
	${CC} ${CFLAGS} -c -I . pkt_dump.c -o pkt_dump.o
Layer2/l2switch.o:Layer2/l2switch.c
	${CC} ${CFLAGS} -c -I . Layer2/l2switch.c -o Layer2/l2switch.o
Layer3/Layer3.o:Layer3/layer3.c
	${CC} ${CFLAGS} -c -I . Layer3/layer3.c -o Layer3/layer3.o
Layer5/ping.o:Layer5/ping.c
	${CC} ${CFLAGS} -c -I . Layer5/ping.c -o Layer5/ping.o
Layer5/Layer5.o:Layer5/layer5.c
	${CC} ${CFLAGS} -c -I . Layer5/layer5.c -o Layer5/layer5.o


# CommandParser/libcli.a:
# 	(cd CommandParser;make)

clean:
	rm *.o
	rm glthreads_lib/glthread.o
	rm *exe
	rm Layer2/*.o
clean_all:
	clean
	(cd CommandParser;make clean)
all:
	(cd CommandParser;make)
	make
	