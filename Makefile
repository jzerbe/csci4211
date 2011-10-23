# csci4211 Project Master Makefile
# Jason Zerbe - 3830775

#FLAGS = -Wall -g -DDEBUG #debug (includes gdb extensions)
FLAGS = -Wall #normal
#FLAGS = -Wall -O3 -funroll-loops -march=native #optimized

CC = gcc #g++
CEXT = c #cpp
LIBOPTS = #-lnsl

.PHONY: all
all: peer

peer: sockcomm.o
	${CC} ${LIBOPTS} ${FLAGS} src/$@.${CEXT} $^ -o $@

sockcomm.o:
	${CC} ${FLAGS} -c src/sockcomm.${CEXT} -o $@

# removes binaries and compiled object files from build directory
.PHONY: clean
clean:
	rm -f *.o
	rm -f peer
