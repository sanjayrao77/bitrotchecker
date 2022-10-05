# this uses gnutls, use Makefile.openssl for openssl instead
CFLAGS=-g -Wall -O2 -DLINUX -DGNUTLS -DUSEMMAP
all: bitrotchecker
bitrotchecker: main.o bitrot.o dirbyname.o filebyname.o common/blockmem.o common/mmapwrapper.o
	gcc -o $@ $^ -lgnutls-openssl
clean:
	rm -f bitrotchecker core *.o common/*.o
backup: clean
	tar -jcf - . | jbackup src.bitrot.tar.bz2
