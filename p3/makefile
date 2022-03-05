CFLAGS= -Wall
DEBUG= -DDEBUG

all: main

main: ppos_core.o queue.o pingpong-dispatcher.c
	gcc ppos_core.o queue.o pingpong-dispatcher.c -o main $(CFLAGS)

debug: ppos_core_debug.o queue.o pingpong-dispatcher.c
	gcc ppos_core.o queue.o pingpong-dispatcher.c -o debug $(CFLAGS)

test: ppos_core.o test3.c
	gcc ppos_core.o test3.c -o test $(CFLAGS)

ppos_core_debug.o: ppos.h ppos_data.h ppos_core.c
	gcc -c ppos_core.c -o ppos_core.o $(CFLAGS) $(DEBUG)

ppos_core.o: ppos.h ppos_data.h ppos_core.c
	gcc -c ppos_core.c -o ppos_core.o $(CFLAGS)

queue.o: queue.c queue.h
	gcc -c queue.c -o queue.o $(CFLAGS)

clean:
	rm *.o

purge: clean
	rm test main debug
