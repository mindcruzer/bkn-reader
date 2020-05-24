all:
	gcc -Wall -c read_bkn.c json.c
	gcc read_bkn.o json.o -o read_BKN
clean:
	rm -rf read_BKN
