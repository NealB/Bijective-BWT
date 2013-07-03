CC = gcc
CFLAGS = -O2
LDFLAGS = -ldivsufsort

mk_bwts: mk_bwts_sa.o map_file.o
	$(CC) -o $@ mk_bwts_sa.o map_file.o $(LDFLAGS)

install: mk_bwts
	cp mk_bwts $(HOME)/bin/

clean:
	$(RM) *.o mk_bwts
