# use Makefile.openssl for openssl instead
# use Makefile.gnutls for gnutls instead
CFLAGS=-g -Wall -O2 -DLINUX -DUSEMMAP
all: bitrotchecker
bitrotchecker: main.o bitrot.o dirbyname.o filebyname.o common/blockmem.o common/mmapwrapper.o common/md5.o
	gcc -o $@ $^
clean:
	rm -f bitrotchecker core *.o common/*.o
backup: clean
	tar -jcf - . | jbackup src.bitrot.tar.bz2
