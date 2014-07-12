CC = gcc
#CFLAGS = -O2
CFLAGS = -O2 -DSHOW_TIMINGS
#CFLAGS = -ggdb -DSHOW_TIMINGS
LDFLAGS = -ldivsufsort

mk_bwts: mk_bwts_sa.o map_file.o binsearch_sa.o
	$(CC) -o $@ $(CFLAGS) mk_bwts_sa.o map_file.o binsearch_sa.o $(LDFLAGS)

install: mk_bwts
	cp mk_bwts $(HOME)/bin/

clean:
	$(RM) *.o mk_bwts FOO BAR

BAR: TEST
	mk_bwts TEST BAR 

test: mk_bwts TEST BAR
	./mk_bwts TEST FOO; cmp FOO BAR && echo "Match"
