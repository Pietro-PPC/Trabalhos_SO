CFLAGS= -Wall
DEBUG= -DDEBUG

MAINFILE= pingpong-contab-prio.c

all: main

main: ppos_core.o queue.o $(MAINFILE)
	gcc ppos_core.o queue.o $(MAINFILE) -o main $(CFLAGS)

debug: ppos_core_debug.o queue.o $(MAINFILE)
	gcc ppos_core_debug.o queue.o $(MAINFILE) -o debug $(CFLAGS)

test: ppos_core.o test.c
	gcc ppos_core.o test.c -o test $(CFLAGS)

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
