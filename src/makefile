OBJS=xsnow.o
EFILE=xrain
CFILE=xsnow.c
HFILE=snowmap.h snow05.xbm snow04.xbm snow01.xbm snow02.xbm snow03.xbm
CFLAGS=-Wall -g 
CC = gcc
LIBS = -L/usr/X11/lib -lX11 -lXpm 

$(EFILE): $(OBJS)
	$(CC) $(OBJS) $(LIBS) -o $(EFILE) $(CFLAGS)

$(OBJS): $(CFILE) $(HFILE)
	$(CC) -c $(CFILE) $(CFLAGS)

clean:
	rm -f $(OBJS)

clean_all: 
	rm -f $(OBJS) $(EFILE)

develop:
	strip $(EFILE)
