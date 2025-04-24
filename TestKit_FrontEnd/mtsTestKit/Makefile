TARGET=mtsTestKit

CC = gcc 
CFLAGS = -std=gnu99 -m64 -DMYNAME=\"$(TARGET)\" -DUSE_GDB -Wno-unused-function -Wno-deprecated-declarations -Wall -I./ 
SRC = forlinux.c $(TARGET).c listClassDevices.c sequence.c cmdpslave.c mts_tk_man.c
# library path and names    
LIBS=-L/usr/lib/x86_64-linux-gnu -ldl -lm

GTKPAR=`pkg-config --cflags --libs gtk+-2.0 gmodule-2.0`

$(TARGET): $(SRC) $(TARGET).h mtsGTK.h video_def.h
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET) $(LIBS) $(GTKPAR)

video_def.h: mtsTestKit.glade
	./glade2include mtsTestKit.glade video_def

glade2include: glade2include.c
	$(CC) glade2include.c -o glade2include $(LIBS)
clean: 
	rm -f *.o $(TARGET)

