
CC = gcc -m64 -Wno-unused-function -Wno-format-overflow -Wno-deprecated-declarations

CFLAGS =-Wall -DCBUG -lintl -g

SRC = collaudo.o vbInterface.o

all: doprogram collaudo

doprogram: doprogram.o  vbInterface.o
	$(CC) -Wall doprogram.o  vbInterface.o -o doprogram

doprogram.o: collaudo.c
	$(CC) -c collaudo.c -o doprogram.o $(CFLAGS) -DSENDFW

collaudo: $(SRC)
	$(CC) -Wall $(SRC) -o collaudo

collaudo.o: collaudo.c
	$(CC) -c collaudo.c -o collaudo.o $(CFLAGS)

vbInterface.o: vbInterface.c
	$(CC) -c  vbInterface.c -o  vbInterface.o $(CFLAGS)

clean: 
	rm -f *.o collaudo doprogram

