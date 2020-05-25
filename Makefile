all:
	gcc -Wall -c main.c read_bkn.c
	gcc main.o read_bkn.o -o bkn_reader
clean:
	rm -rf bkn_reader
