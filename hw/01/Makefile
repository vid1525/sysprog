CFLAGS=-O2 -Werror -Wall -std=gnu11

all: main

test: main
	python3 generate_exec_test.py -n 10 -c 10000 -m 100000

main: main.o sort_file.o merge.o coro.o
	gcc ${CFLAGS} coro.o sort_file.o merge.o main.o -o main

sort_file.o: src/sort_file.c src/sort_file.h
	gcc -c ${CFLAGS} src/sort_file.c -o sort_file.o

merge.o: src/merge_files.c src/merge_files.h
	gcc -c ${CFLAGS} src/merge_files.c -o merge.o

coro.o: src/coro.c src/coro.h
	gcc -c ${CFLAGS} src/coro.c -o coro.o

main.o: src/main.c
	gcc -c ${CFLAGS} src/main.c -o main.o

clean:
	rm -rf *.o
	rm -rf test*
