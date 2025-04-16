
CC = gcc -m64 -Wno-unused-function

CFLAGS =-Wall -DCBUG -DFR_LINUX -lintl 

SRC = prova.o

all: prova

prova: $(SRC)
	$(CC) -Wall $(SRC) -o prova

prova.o: prova.c
	$(CC) -c prova.c -o prova.o $(CFLAGS)

clean: 
	rm -f *.o prova

