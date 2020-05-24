all:
	gcc -Wall -c main.c read_bkn.c json.c
	gcc main.o read_bkn.o json.o -o bkn_reader
clean:
	rm -rf bkn_reader
