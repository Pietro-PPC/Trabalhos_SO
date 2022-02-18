CFLAGS= -Wall

all: queue.o

testafila: queue.o testafila.c
	gcc queue.o testafila.c -o testafila $(CFLAGS)

test: queue.o test.c
	gcc queue.o test.c -o test $(CFLAGS)

queue.o: queue.c queue.h
	gcc -c queue.c $(CFLAGS)

clean: 
	rm -f *.o

purge: clean
	rm test testafila