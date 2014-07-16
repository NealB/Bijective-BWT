CC = gcc
#CFLAGS = -O2
CFLAGS = -O3 -DSHOW_TIMINGS -Wall
#CFLAGS = -ggdb -DSHOW_TIMINGS
LDFLAGS = -ldivsufsort

mk_bwts_sa: mk_bwts_sa.o map_file.o binsearch_sa.o
	$(CC) -o $@ mk_bwts_sa.o map_file.o binsearch_sa.o $(LDFLAGS)

install: mk_bwts_sa
	cp mk_bwts_sa $(HOME)/bin/

clean:
	$(RM) *.o mk_bwts_sa FOO BAR

BAR: TEST
	mk_bwts TEST BAR 

test: mk_bwts_sa TEST BAR
	./mk_bwts_sa TEST FOO; cmp FOO BAR && echo "Match"
