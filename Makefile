CC = gcc
CFLAGS = -O2
#CFLAGS = -O2 -DSHOW_TIMINGS
LDFLAGS = -ldivsufsort

all: mk_bwts unbwts

mk_bwts: mk_bwts_sa.o map_file.o
	$(CC) -o $@ $(CFLAGS) mk_bwts_sa.o map_file.o $(LDFLAGS)

unbwts: unbwts.o map_file.o
	$(CC) -o $@ $(CFLAGS) unbwts.o map_file.o $(LDFLAGS)

install: mk_bwts
	cp mk_bwts $(HOME)/bin/

clean:
	$(RM) *.o mk_bwts FOO BAR

test: mk_bwts TEST
	./mk_bwts TEST FOO ; mk_bwts TEST BAR ; cmp FOO BAR && echo "Match"
