CC = gcc
#CFLAGS = -O2
#CPPFLAGS = -DSHOW_DEBUG
CPPFLAGS =  -DNDEBUG -DSHOW_TIMINGS
CFLAGS = -O2 -Wall -Wno-maybe-uninitialized
LDFLAGS = -ldivsufsort

SOURCES = mk_bwts_sa.c binsearch_sa.c map_file.c

mk_bwts_sa: $(SOURCES:.c=.o)
	$(CC) -o $@ $^ $(LDFLAGS)

#mk_bwts_sa_orig: mk_bwts_sa_orig.o map_file.o binsearch_sa.o
#$(CC) -o $@ mk_bwts_sa_orig.o map_file.o binsearch_sa.o $(LDFLAGS)

%.d: %.c
	$(CC) -MM $(CPPFLAGS) $< | sed 's/\(.*\):\(.*\)/\1 $@:\2/' > $@

.PHONY: install clean test test-orig

install: mk_bwts_sa
	cp mk_bwts_sa $(HOME)/bin/

clean:
	$(RM) *.o *.d mk_bwts_sa FOO BAR

BAR: TEST
	mk_bwts TEST BAR 

test: mk_bwts_sa TEST BAR
	./mk_bwts_sa TEST FOO
	cmp FOO BAR && echo "Match"

#test-orig: mk_bwts_sa_orig TEST BAR
#./mk_bwts_sa_orig TEST FOO
#cmp FOO BAR && echo "Match"

include $(SOURCES:.c=.d)
