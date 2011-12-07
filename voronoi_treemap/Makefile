OBJS = vorotree.o dSFMT.o color.o
CFLAGS = -O0 -ggdb -march=core2 -frounding-math -I/usr/include/cairo
LIBS = -pthread -lmpfr -lgmp -lCGAL -lcairo -lboost_thread
CC = g++
COMPILE = $(CC) $(CFLAGS)

vorotree: $(OBJS)
	$(CC) -o $@ $(OBJS) $(LIBS)

.c.o:
	$(COMPILE) -c $<
.cpp.o:
	$(COMPILE) -c $<

vorotree.o: vorotree.cpp treedata.inc
dSFMT.o: dSFMT.c

all: vorotree
clean:
	rm -rf vorotree *.o
