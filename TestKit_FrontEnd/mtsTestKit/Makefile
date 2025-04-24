TARGET=mtsTestKit

CC = gcc -m64 -DMYNAME=\"$(TARGET)\" -Wno-unused-function -Wno-deprecated-declarations
#CC = gcc-4.8

CFLAGS = -Wall -I./ 
	
# library path and names    
LIBS=-L/usr/lib/x86_64-linux-gnu -ldl -lm

GTKPAR=`pkg-config --cflags --libs gtk+-2.0 gmodule-2.0`
SRC = forlinux.c $(TARGET).c listClassDevices.c sequence.c cmdpslave.c mts_tk_man.c

$(TARGET): $(SRC) $(TARGET).h mtsGTK.h video_def.h
	$(CC) $(SRC) -o $(TARGET) $(LIBS) $(GTKPAR)

video_def.h: mtsTestKit.glade
	./glade2include mtsTestKit.glade video_def

glade2include: glade2include.c
	$(CC) glade2include.c -o glade2include $(LIBS)
clean: 
	rm -f *.o $(TARGET)

