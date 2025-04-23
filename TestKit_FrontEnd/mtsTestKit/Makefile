
CC = gcc -m64 -Wno-unused-function -Wno-deprecated-declarations
#CC = gcc-4.8

CFLAGS = -Wall -DFR_LINUX -I./ 
	
#CFLAGS = -Wall			 	\
#	-DG_DISABLE_DEPRECATED 	 	\
#	-DGDK_DISABLE_DEPRECATED 	\
#	-DGDK_PIXBUF_DISABLE_DEPRECATED \
#	-DGTK_DISABLE_DEPRECATED -DFR_LINUX -DMTSTESTKIT -I./

# library path and names    
LIBS=-L/usr/lib/x86_64-linux-gnu -ldl -lm

GTKPAR=`pkg-config --cflags --libs gtk+-2.0 gmodule-2.0`
SRC = forlinux.c mtsTestKit.c ListClassDevices.c sequence.c cmdpslave.c mts_tk_man.c

mtsTestKit: $(SRC) mtsTestKit.h mtsGTK.h video_def.h
	$(CC) $(SRC) -o mtsTestKit $(LIBS) $(GTKPAR)

video_def.h: mtsTestKit.glade
	./glade2include mtsTestKit.glade video_def

clean: 
	rm -f *.o mtsTestKit

