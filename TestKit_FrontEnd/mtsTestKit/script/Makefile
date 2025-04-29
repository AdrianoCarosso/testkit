TARGET1=collaudo
TARGET2=doprogram

CC = gcc -m64 -Wno-unused-function -Wno-format-overflow -Wno-deprecated-declarations
CFLAGS =-Wall -DCBUG -lintl -g

SRC = collaudo.o vbInterface.o

all: $(TARGET1) $(TARGET2)

$(TARGET2): $(TARGET2).o  vbInterface.o
	$(CC) -Wall $(TARGET2).o  vbInterface.o -o $(TARGET2)
	@echo "cp $(TARGET2) ../app"
	@cp $(TARGET2) ../app

$(TARGET2).o: $(TARGET1).c
	$(CC) -c $(TARGET1).c -o $(TARGET2).o $(CFLAGS) -DSENDFW

$(TARGET1): $(SRC)
	$(CC) -Wall $(SRC) -o $(TARGET1)
	@echo "cp $(TARGET1) ../app"
	@cp $(TARGET1) ../app

$(TARGET1).o: $(TARGET1).c
	$(CC) -c $(TARGET1).c -o $(TARGET1).o $(CFLAGS)

vbInterface.o: vbInterface.c
	$(CC) -c  vbInterface.c -o  vbInterface.o $(CFLAGS)

clean: 
	rm -f *.o $(TARGET1) $(TARGET2)

