#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <fcntl.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#define DEBUG
#include "conventions.h"

#include "mmapwrapper.h"

CLEARFUNC(mmapwrapper);

int initreadfile_mmapwrapper(struct mmapwrapper *m, char *filename) {
int fd=-1;

if (!strcmp(filename,"-")) return slurpfd_mmapwrapper(m,STDIN_FILENO);

fd=open(filename,O_RDONLY);
if (fd<0) GOTOERROR;
if (initreadfd_mmapwrapper(m,fd)) GOTOERROR;
(ignore)close(fd);
return 0;
error:
	ifclose(fd);
	return -1;
}
void deinit_mmapwrapper(struct mmapwrapper *mmap) {
ifmunmap(mmap->unmapaddr,mmap->addrlength);
iffree(mmap->freeaddr);
}

int slurpfd_mmapwrapper(struct mmapwrapper *m, int fd) {
uint64_t max=0,num=0,left=0;
unsigned char *addr=NULL,*cursor=NULL;

while (1) {
	int k;
	if (!left) {
		unsigned char *temp;
		left=1024*1024;
		max+=left;
		if (!(temp=realloc(addr,max))) GOTOERROR;
		addr=temp;
		cursor=addr+num;
	}
	k=read(fd,cursor,left);
	if (k<=0) {
		if (!k) break;
		GOTOERROR;
	}
	num+=k;
	left-=k;
	cursor+=k;
}

m->addrlength=max;
m->filesize=num;
m->addr=addr;
m->freeaddr=addr;
return 0;
error:
	iffree(addr);
	return -1;
}


int initreadfd2_mmapwrapper(struct mmapwrapper *m, int fd, uint64_t filesize) {
uint64_t addrlength;
void *addr=NULL;

if (filesize) {
	addrlength=filesize;
	addr=mmap(NULL,addrlength,PROT_READ,MAP_SHARED,fd,0);
	if (addr==MAP_FAILED) {
		GOTOERROR;
	}
} else if (!filesize) {
	int pagesize;
	// can't map 0 size
	pagesize=sysconf(_SC_PAGESIZE);
	addrlength=pagesize;
	addr=mmap(NULL,addrlength,PROT_READ,MAP_SHARED|MAP_ANONYMOUS,-1,0);
	if (addr==MAP_FAILED) GOTOERROR;
}
m->addr=m->unmapaddr=addr;
m->addrlength=addrlength;
m->filesize=filesize;
return 0;
error:
	ifmunmap(addr,addrlength);
	return -1;
}
#if 0
int initreadfd_mmapwrapper(struct mmapwrapper *m, int fd) {
uint64_t addrlength;
off64_t filesize;
int pagesize;
void *addr=NULL;

//pagesize=getpagesize();
pagesize=sysconf(_SC_PAGESIZE);

filesize=lseek(fd,0,SEEK_END);
if (filesize<0) GOTOERROR;
if (filesize) {
	addrlength=filesize;
	addr=mmap(NULL,addrlength,PROT_READ,MAP_SHARED,fd,0);
	if (addr==MAP_FAILED) {
		GOTOERROR;
	}
} else if (!filesize) {
	// can't map 0 size
	addrlength=pagesize;
	addr=mmap(NULL,addrlength,PROT_READ,MAP_SHARED|MAP_ANONYMOUS,-1,0);
	if (addr==MAP_FAILED) GOTOERROR;
}
m->addr=m->unmapaddr=addr;
m->addrlength=addrlength;
m->filesize=filesize;
return 0;
error:
	ifmunmap(addr,addrlength);
	return -1;
}
#endif
int initreadfd_mmapwrapper(struct mmapwrapper *m, int fd) {
off64_t filesize;

filesize=lseek(fd,0,SEEK_END);
if (filesize<0) GOTOERROR;
if (initreadfd2_mmapwrapper(m,fd,(uint64_t)filesize)) GOTOERROR;
return 0;
error:
	return -1;
}
