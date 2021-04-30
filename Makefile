CC = gcc
CFLAGS = -O2
#CFLAGS = -O2 -DSHOW_TIMINGS
LDFLAGS = -ldivsufsort
BINARY_FILES = mk_bwts unbwts mk_bwts_new_algo #mbwt_new mbwt_verbose 

all: $(BINARY_FILES)

mk_bwts: mk_bwts_sa.o map_file.o
	$(CC) -o $@ $(CFLAGS) $^ $(LDFLAGS)

mk_bwts_new_algo: mk_bwts_new_algo.o map_file.o
	$(CC) -o $@ $(CFLAGS) $^ $(LDFLAGS)

mbwt_verbose: mk_bwts_sa_new_verbose.c map_file.o
	$(CC) -o $@ $(CFLAGS) $^ $(LDFLAGS)

mbwt_new: mk_bwts_sa_new.o map_file.o
	$(CC) -o $@ $(CFLAGS) $^ $(LDFLAGS)

unbwts: unbwts.o map_file.o
	$(CC) -o $@ $(CFLAGS) $^ $(LDFLAGS)

install: mk_bwts
	cp mk_bwts $(HOME)/bin/

clean:
	$(RM) *.o $(BINARY_FILES)

test: mk_bwts_new_algo
	rm -f testdata/testjunk.newalgo.bwts
	./mk_bwts_new_algo testdata/testjunk testdata/testjunk.newalgo.bwts
	cmp testdata/testjunk.newalgo.bwts testdata/testjunk.bwts && echo "Match"

test-enwik8: mk_bwts_new_algo
	rm -f testdata/enwik8.newalgo.bwts
	./mk_bwts_new_algo testdata/enwik8 testdata/enwik8.newalgo.bwts
	cmp testdata/enwik8.newalgo.bwts testdata/enwik8.bwts && echo "Match"

