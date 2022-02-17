CFLAGS= -Wall

all: queue.o

test: queue.o test.c
	gcc queue.o test.c -o test $(CFLAGS)

queue.o: queue.c queue.h
	gcc -c queue.c $(CFLAGS)

clean: 
	rm -f *.o

purge: clean
	rm test;