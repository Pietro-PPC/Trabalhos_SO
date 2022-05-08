CFLAGS= -Wall
DEBUG= -DDEBUG
LIBS= -lm

MAINFILE= pingpong-mqueue.c

all: main

main: ppos_core.o queue.o $(MAINFILE)
	gcc ppos_core.o queue.o $(MAINFILE) -o main $(LIBS) $(CFLAGS) 

debug: ppos_core_debug.o queue.o $(MAINFILE)
	gcc ppos_core_debug.o queue.o $(MAINFILE) -o debug $(LIBS) $(CFLAGS)

test: ppos_core.o queue.o test.c
	gcc ppos_core.o queue.o test.c -o test $(CFLAGS)

ppos_core_debug.o: ppos.h ppos_data.h ppos_core.c
	gcc -c ppos_core.c -o ppos_core_debug.o $(CFLAGS) $(DEBUG)

ppos_core.o: ppos.h ppos_data.h ppos_core.c
	gcc -c ppos_core.c -o ppos_core.o $(CFLAGS)

queue.o: queue.c queue.h
	gcc -c queue.c -o queue.o $(CFLAGS)

clean:
	rm *.o

purge: clean
	rm test main debug
