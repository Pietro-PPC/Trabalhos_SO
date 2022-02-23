CFLAGS="-Wall"

all: main

main: ppos_core.o
	gcc ppos_core.o -o main $(CFLAGS)

test: ppos_core.o test.c
	gcc ppos_core.o test.c -o test $(CFLAGS)

ppos_core.o: ppos.h ppos_data.h ppos_core.c
	gcc -c ppos_core.c -o ppos_core.o $(CFLAGS)

