CC = gcc
#CPPFLAGS = -DSHOW_DEBUG
#CPPFLAGS =  -DNDEBUG -DSHOW_TIMINGS
CFLAGS = -Wall -Wno-maybe-uninitialized -DSHOW_DEBUG
LDFLAGS = -ldivsufsort -L/usr/local/lib

SOURCES = mk_bwts_sa.c binsearch_sa.c map_file.c

all: optimized

optimized: CFLAGS += -O2
optimized: CPPFLAGS = -DNDEBUG -DSHOW_TIMINGS
optimized: mk_bwts

debug: CFLAGS += -ggdb
debug: mk_bwts

mk_bwts: $(SOURCES:.c=.o)
	$(CC) -o $@ $^ $(LDFLAGS)

#mk_bwts_sa_orig: mk_bwts_sa_orig.o map_file.o binsearch_sa.o
#$(CC) -o $@ mk_bwts_sa_orig.o map_file.o binsearch_sa.o $(LDFLAGS)

%.d: %.c
	$(CC) -MM $(CPPFLAGS) $< | sed 's/\(.*\):\(.*\)/\1 $@:\2/' > $@

.PHONY: install clean test test-orig debug optimized

install: mk_bwts
	cp mk_bwts $(HOME)/bin/

clean:
	$(RM) *.o *.d mk_bwts FOO BAR

BAR: TEST
	mk_bwts TEST BAR 

test: mk_bwts TEST BAR
	./mk_bwts TEST FOO
	cmp FOO BAR && echo "Match"

#test-orig: mk_bwts_sa_orig TEST BAR
#./mk_bwts_sa_orig TEST FOO
#cmp FOO BAR && echo "Match"

include $(SOURCES:.c=.d)
