/*
 * bitrot.h
 * Copyright (C) 2022 Sanjay Rao
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#define _FILE_OFFSET_BITS 64
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <ctype.h>
#include <dirent.h>
#include <fcntl.h>
#include <errno.h>
#ifdef OPENSSL
#include <openssl/md5.h>
#elif GNUTLS
#include <gnutls/openssl.h>
#else
#include "common/md5.h"
#endif
#define DEBUG
#include "common/conventions.h"
#include "common/blockmem.h"
#include "common/mmapwrapper.h"

#include "bitrot.h"
#include "tarvars.h"
#include "dirbyname.h"
#include "filebyname.h"

SCLEARFUNC(file_bitrot);
SCLEARFUNC(dir_bitrot);

void clear_bitrot(struct bitrot *bitrot) {
static struct bitrot blank={.rootdir.xdev=INVALID_DEVT_BITROT,.options.ceiling_mtime=-1};
*bitrot=blank;
}

int init_bitrot(struct bitrot *bitrot, int isnoio) {
if (init_blockmem(&bitrot->blockmem,0)) GOTOERROR;
bitrot->topdir.name="";
#ifndef USEMMAP
bitrot->iobuffer.ptrmax=READCHUNK_BITROT;
if (!isnoio) {
	if (!(bitrot->iobuffer.ptr=malloc(bitrot->iobuffer.ptrmax))) GOTOERROR;
}
#endif
return 0;
error:
	return -1;
}

void deinit_bitrot(struct bitrot *bitrot) {
#ifdef USEMMAP
iffree(bitrot->_iobuffer.ptr);
#else
iffree(bitrot->iobuffer.ptr);
#endif
deinit_blockmem(&bitrot->blockmem);
}

static int loadhex(unsigned char *dest, unsigned int destlen, char *src) {
while (1) {
	unsigned int high,low,c;
	if (!destlen) break;
	c=*src;
	src++;
	switch (c) {
		case '0': high=0<<4; break; case '1': high=1<<4; break; case '2': high=2<<4; break; case '3': high=3<<4; break;
		case '4': high=4<<4; break; case '5': high=5<<4; break; case '6': high=6<<4; break; case '7': high=7<<4; break;
		case '8': high=8<<4; break; case '9': high=9<<4; break;
		case 'a': high=10<<4; break; case 'b': high=11<<4; break; case 'c': high=12<<4; break;
		case 'd': high=13<<4; break; case 'e': high=14<<4; break; case 'f': high=15<<4; break;
		case 'A': high=10<<4; break; case 'B': high=11<<4; break; case 'C': high=12<<4; break;
		case 'D': high=13<<4; break; case 'E': high=14<<4; break; case 'F': high=15<<4; break;
		default: GOTOERROR;
	}
	c=*src;
	src++;
	switch (c) {
		case '0': low=0; break; case '1': low=1; break; case '2': low=2; break; case '3': low=3; break; case '4': low=4; break;
		case '5': low=5; break; case '6': low=6; break; case '7': low=7; break; case '8': low=8; break; case '9': low=9; break;
		case 'a': low=10; break; case 'b': low=11; break; case 'c': low=12; break;
		case 'd': low=13; break; case 'e': low=14; break; case 'f': low=15; break;
		case 'A': low=10; break; case 'B': low=11; break; case 'C': low=12; break;
		case 'D': low=13; break; case 'E': low=14; break; case 'F': low=15; break;
		default: GOTOERROR;
	}
	*dest=high|low;
	destlen--;
	dest++;
}
return 0;
error:
	return -1;
}

static int findoradd_dir(struct dir_bitrot **dir_out, struct bitrot *bitrot, struct dir_bitrot *parent, char *name,
		unsigned int flags) {
struct dir_bitrot *dir;

dir=filename_find2_dirbyname(parent->children.topnode,name);
#if 0
{
	fprintf(stderr,"%s:%d looking for %s\n",__FILE__,__LINE__,name);
	(ignore)writedirtofile(parent,stderr);
}
#endif
if (dir) {
	dir->flags|=flags;
	*dir_out=dir;
	return 0;
}

if (!(dir=ALLOC_blockmem(&bitrot->blockmem,struct dir_bitrot))) GOTOERROR;
clear_dir_bitrot(dir);
if (!(dir->name=strdup_blockmem(&bitrot->blockmem,name))) GOTOERROR;
dir->parent=parent;
dir->flags=flags;
(void)addnode2_dirbyname(&parent->children.topnode,dir);

*dir_out=dir;
return 0;
error:
	return -1;
}

static int addfileentry(struct bitrot *bitrot, char *filename, unsigned char *md5sum) {
struct file_bitrot *file;
struct dir_bitrot *dir;

dir=&bitrot->topdir;
while (1) {
	char *slash;
	slash=strchr(filename,'/');
	if (!slash) break;
	*slash=0;
	if (!strcmp(filename,".")) {
	} else {
		if (findoradd_dir(&dir,bitrot,dir,filename,ISINFILE_FLAG_BITROT)) GOTOERROR;
	}
	
	filename=slash+1;
}

file=filename_find2_filebyname(dir->files.topnode,filename);
if (file) {
	if (memcmp(file->md5,md5sum,LEN_MD5_BITROT)) {
		fprintf(stderr,"%s:%d duplicate file entry for \"%s\"\n",__FILE__,__LINE__,filename);
		GOTOERROR;
	}
} else {
	if (!(file=ALLOC_blockmem(&bitrot->blockmem,struct file_bitrot))) GOTOERROR;
	clear_file_bitrot(file);
	if (!(file->name=strdup_blockmem(&bitrot->blockmem,filename))) GOTOERROR;
	file->flags=ISINFILE_FLAG_BITROT;
	memcpy(file->md5,md5sum,LEN_MD5_BITROT);
	(void)addnode2_filebyname(&dir->files.topnode,file);
}
return 0;
error:
	return -1;
}

// it's hard to get filenames this long but with utf16 and ././@LongLink, it gets big
#define MAXLINELEN	2048
int loadfile_bitrot(int *isnotfound_out, struct bitrot *bitrot, char *sumfile) {
FILE *ff=NULL;
char *oneline=NULL;

if (!(bitrot->sumfile.name=strdup_blockmem(&bitrot->blockmem,sumfile))) GOTOERROR;

if (access(sumfile,F_OK)) {
	if (errno==ENOENT) {
		*isnotfound_out=1;
		return 0;
	}
	GOTOERROR;
}
if (!(ff=fopen(sumfile,"r"))) GOTOERROR;
{
	struct stat statbuf;
	if (fstat(fileno(ff),&statbuf)) GOTOERROR;
	bitrot->sumfile.mtime=statbuf.st_mtim.tv_sec;
}
if (!(oneline=malloc(MAXLINELEN))) GOTOERROR;
while (1) {
	int n;
	unsigned char buff16[LEN_MD5_BITROT];
	if (!fgets(oneline,MAXLINELEN,ff)) break;
	n=strlen(oneline);
	if (!n) GOTOERROR;
	if (n==1) continue;
	n--;
	if (oneline[n]!='\n') {
		fprintf(stderr,"%s:%d input line is too long in %s\n",__FILE__,__LINE__,sumfile);
		GOTOERROR;
	}
	oneline[n]='\0';
	if (oneline[0]=='#') continue;
	if (n<LEN_MD5_BITROT*2+2+1) {
		fprintf(stderr,"%s:%d bad line in %s, \"%s\"\n",__FILE__,__LINE__,sumfile,oneline);
		GOTOERROR;
	}
	if (loadhex(buff16,16,oneline)) {
		fprintf(stderr,"%s:%d bad hash in %s, \"%s\"\n",__FILE__,__LINE__,sumfile,oneline);
		GOTOERROR;
	}
	if ( (oneline[LEN_MD5_BITROT*2]!=' ') || (oneline[LEN_MD5_BITROT*2+1]!=' ') ) {
		fprintf(stderr,"%s:%d bad delimiter in %s, \"%s\"\n",__FILE__,__LINE__,sumfile,oneline);
		GOTOERROR;
	}
	if (addfileentry(bitrot,oneline+LEN_MD5_BITROT*2+2,buff16)) GOTOERROR;
}

if (ferror(ff)) GOTOERROR;
free(oneline);
fclose(ff);
*isnotfound_out=0;
return 0;
error:
	iffree(oneline);
	iffclose(ff);
	return -1;
}

static int printpath(struct dir_bitrot *dir, FILE *ff) {
if (dir->parent) {
	if (printpath(dir->parent,ff)) GOTOERROR;
}
if (dir->name[0]) {
	if (0>fputs(dir->name,ff)) GOTOERROR;
	if (0>fputc('/',ff)) GOTOERROR;
}
return 0;
error:
	return -1;
}

static inline void sethexbuff(unsigned char *dest, unsigned char *src, int len) {
char hexvals[]="0123456789abcdef";
while (1) {
	unsigned int ui;
	ui=*src;
	*dest=hexvals[ui>>4];
	dest++;
	*dest=hexvals[ui&15];
	dest++;
	len--;
	if (!len) break;
	src++;
}
dest[0]=' ';
dest[1]=' ';
}

static int writefiletofile(struct dir_bitrot *dir, struct file_bitrot *file, FILE *ff) {
unsigned char hexbuff[LEN_MD5_BITROT*2+2];
if (file->treevars.left) {
	if (writefiletofile(dir,file->treevars.left,ff)) GOTOERROR;
}

if (file->flags&ISFOUND_FLAG_BITROT) {
	(void)sethexbuff(hexbuff,file->md5,LEN_MD5_BITROT);
	if (1!=fwrite(hexbuff,LEN_MD5_BITROT*2+2,1,ff)) GOTOERROR;
	if (printpath(dir,ff)) GOTOERROR;
	if (0>fputs(file->name,ff)) GOTOERROR;
	if (0>fputc('\n',ff)) GOTOERROR;
}

if (file->treevars.right) {
	if (writefiletofile(dir,file->treevars.right,ff)) GOTOERROR;
}
return 0;
error:
	return -1;
}

static int writedirtofile(struct dir_bitrot *dir, FILE *ff) {
if (dir->treevars.left) {
	if (writedirtofile(dir->treevars.left,ff)) GOTOERROR;
}
if (dir->children.topnode) {
	if (writedirtofile(dir->children.topnode,ff)) GOTOERROR;
}
if (dir->files.topnode) {
	if (writefiletofile(dir,dir->files.topnode,ff)) GOTOERROR;
}
if (dir->treevars.right) {
	if (writedirtofile(dir->treevars.right,ff)) GOTOERROR;
}
return 0;
error:
	return -1;
}

int writefile_bitrot(struct bitrot *b, char *filename) {
FILE *ff=NULL;
if (!(ff=fopen(filename,"w"))) GOTOERROR;

if (writedirtofile(&b->topdir,ff)) GOTOERROR;

if (ferror(ff)) GOTOERROR;
if (fclose(ff)) {
	ff=NULL;
	GOTOERROR;
}
return 0;
error:
	iffclose(ff);
	return -1;
}

static int printdirtree(struct dir_bitrot *dir, int depth, FILE *fout) {
if (dir->treevars.left) {
	(ignore)printdirtree(dir->treevars.left,depth,fout);
}
int i;
for (i=0;i<depth;i++) fputc(' ',fout);
fputs(dir->name,fout);
fputc('\n',fout);
if (dir->children.topnode) printdirtree(dir->children.topnode,depth+1,fout);

if (dir->treevars.right) {
	(ignore)printdirtree(dir->treevars.right,depth,fout);
}
return 0;
}

int printtree_bitrot(struct bitrot *b, FILE *fout) {
// TODO handle errors
struct dir_bitrot *dir;

dir=&b->topdir;
(ignore)printdirtree(dir,1,fout);
return 0;
}

static void printprogress(char *name) {
// scantar needs x<=100 in strnlen(name,x)
int n;
n=strnlen(name,60);
(ignore)fputs("md5: ",stderr);
(ignore)fwrite(name,n,1,stderr);
}
static void unprintprogress(char *name) {
int n;
n=strnlen(name,60);
n+=5;
(ignore)fputc('\r',stderr);
for (;n>=0;n--) (ignore)fputc(' ',stderr);
(ignore)fputc('\r',stderr);
}

#ifdef USEMMAP
static int getmd5(int *isnofile_out, struct bitrot *b, unsigned char *dest, int dfd, char *name, struct stat *statbuf) {
MD5_CTX ctx;
unsigned char *ptr;
unsigned int readusleep;
struct mmapwrapper mw;
uint64_t left;
int fd=-1;

clear_mmapwrapper(&mw);

readusleep=b->options.readusleep;

fd=openat(dfd,name,O_RDONLY);
if (0>fd) {
	if ((errno==EACCES) || (errno==EPERM)) {
		*isnofile_out=1;
		return 0;
	}
	fprintf(stderr,"%s:%d error opening %s, (%s)\n",__FILE__,__LINE__,name,strerror(errno));
	GOTOERROR;
}

#ifdef OPENSSL
if (1!=MD5_Init(&ctx)) GOTOERROR; // probably can't happen
#elif GNUTLS
(void)MD5_Init(&ctx);
#else
(void)clear_context_md5(&ctx);
#endif

if (statbuf->st_size) {
	if (initreadfd2_mmapwrapper(&mw,fd,statbuf->st_size)) GOTOERROR;

	ptr=mw.addr;
	left=mw.filesize;

	while (1) {
		if (!left) break;
		if (left>READCHUNK_BITROT) {
#ifdef OPENSSL
			if (1!=MD5_Update(&ctx,ptr,READCHUNK_BITROT)) GOTOERROR;
#elif GNUTLS
			(void)MD5_Update(&ctx,ptr,READCHUNK_BITROT);
#else
			(void)addbytes_context_md5(&ctx,ptr,READCHUNK_BITROT);
#endif
			ptr+=READCHUNK_BITROT;
			left-=READCHUNK_BITROT;
		} else {
			int k;
			k=left;
#ifdef OPENSSL
			if (1!=MD5_Update(&ctx,ptr,k)) GOTOERROR;
#elif GNUTLS
			(void)MD5_Update(&ctx,ptr,k);
#else
			(void)addbytes_context_md5(&ctx,ptr,k);
#endif
			break;
		}
		if (readusleep) usleep(readusleep);
	}
}

#ifdef OPENSSL
if (1!=MD5_Final(dest,&ctx)) GOTOERROR;
#elif GNUTLS
(void)MD5_Final(dest,&ctx);
#else
(void)finish_context_md5(dest,&ctx);
#endif

*isnofile_out=0;

deinit_mmapwrapper(&mw);
(ignore)close(fd);
return 0;
error:
	deinit_mmapwrapper(&mw);
	ifclose(fd);
	return -1;
}
#endif
#ifndef USEMMAP
static int getmd5(int *isnofile_out, struct bitrot *b, unsigned char *dest, int dfd, char *name, struct stat *statbuf) {
MD5_CTX ctx;
unsigned char *ptr;
unsigned int ptrmax;
unsigned int readusleep;
int fd=-1;

readusleep=b->options.readusleep;

fd=openat(dfd,name,O_RDONLY);
if (0>fd) {
	if ((errno==EACCES) || (errno==EPERM)) {
		*isnofile_out=1;
		return 0;
	}
	fprintf(stderr,"%s:%d error opening %s, (%s)\n",__FILE__,__LINE__,name,strerror(errno));
	GOTOERROR;
}

#ifdef OPENSSL
if (1!=MD5_Init(&ctx)) GOTOERROR; // probably can't happen
#elif GNUTLS
(void)MD5_Init(&ctx);
#else
(void)clear_context_md5(&ctx);
#endif

ptr=b->iobuffer.ptr;
ptrmax=b->iobuffer.ptrmax;

while (1) {
	int k;
	k=read(fd,ptr,ptrmax);
	if (k<=0) {
		if (!k) break;
		GOTOERROR;
	}
#ifdef OPENSSL
	if (1!=MD5_Update(&ctx,ptr,k)) GOTOERROR;
#elif GNUTLS
	(void)MD5_Update(&ctx,ptr,k);
#else
	(void)addbytes_context_md5(&ctx,ptr,k);
#endif
	if (readusleep) usleep(readusleep);
}

#ifdef OPENSSL
if (1!=MD5_Final(dest,&ctx)) GOTOERROR;
#elif GNUTLS
(void)MD5_Final(dest,&ctx);
#else
(void)finish_context_md5(dest,&ctx);
#endif

*isnofile_out=0;

(ignore)close(fd);
return 0;
error:
	ifclose(fd);
	return -1;
}
#endif

static int scandirB(struct bitrot *b, struct dir_bitrot *db, DIR *parentdir, char *dirname) {
DIR *dir=NULL;
struct stat statbuf;
int isverbose;
int fstatatflags;
FILE *msgout=b->options.msgout;

#if 0
fprintf(stderr,"Entering directory %s\n",dirname);
#endif

isverbose=b->options.isverbose;

if (!parentdir) { // topdir
	if (!(dir=opendir(dirname))) GOTOERROR;
	if (b->options.isonefilesystem) {
		if (fstat(dirfd(dir),&statbuf)) GOTOERROR;
		b->rootdir.xdev=statbuf.st_dev;
		if (b->rootdir.xdev==INVALID_DEVT_BITROT) {
			fprintf(stderr,"%s:%d Top directory has unexpected dev_t value that conflicts with --one-file-system\n",__FILE__,__LINE__);
			GOTOERROR;
		}
	}
} else { // all other cases
	int fd;
	fd=openat(dirfd(parentdir),dirname,O_RDONLY);
	if (fd<0) GOTOERROR;
	if (b->options.isonefilesystem) {
		if (fstat(fd,&statbuf)) GOTOERROR;
		if (b->rootdir.xdev!=statbuf.st_dev) { // maybe a --bind mount
			(ignore)close(fd);
			if (isverbose) {
				if (0>fputs("skipping xdev dir: ",msgout)) GOTOERROR;
				if (printpath(db,msgout)) GOTOERROR;
				if (0>fputs(dirname,msgout)) GOTOERROR;
				if (0>fputc('\n',msgout)) GOTOERROR;
			}
			return 0;
		}
	}
	if (!(dir=fdopendir(fd))) {
		(ignore)close(fd);
		GOTOERROR;
	}
	fd=-1;
}

if (b->options.isfollow) {
	fstatatflags=0;
} else {
	fstatatflags=AT_SYMLINK_NOFOLLOW;
}

while (1) {
	struct dirent *de;
	struct file_bitrot *file;
	unsigned char md5[LEN_MD5_BITROT];
	errno=0;
	de=readdir(dir);
	if (!de) {
		if (errno) GOTOERROR;
		break;
	}
	if (!strcmp(de->d_name,".")) continue;
	if (!strcmp(de->d_name,"..")) continue;
	if (fstatat(dirfd(dir),de->d_name,&statbuf,fstatatflags)) GOTOERROR;
	if (b->options.isonefilesystem) { // skip dirs and files that are on other devices, possibly from symlinks
		if (b->rootdir.xdev!=statbuf.st_dev) {
			if (isverbose) {
				if (S_ISREG(statbuf.st_mode)) {
					if (0>fputs("skipping xdev file: ",msgout)) GOTOERROR;
				} else if (S_ISDIR(statbuf.st_mode)) {
					if (0>fputs("skipping xdev dir: ",msgout)) GOTOERROR;
				} else {
					if (0>fputs("skipping xdev special: ",msgout)) GOTOERROR;
				}
				if (printpath(db,msgout)) GOTOERROR;
				if (0>fputs(de->d_name,msgout)) GOTOERROR;
				if (0>fputc('\n',msgout)) GOTOERROR;
			}
			continue;
		}
	}
	if (S_ISREG(statbuf.st_mode)) {
		if (statbuf.st_mtim.tv_sec>=b->options.ceiling_mtime) {
			if (isverbose) {
				if (0>fputs("skipping recently changed: ",msgout)) GOTOERROR;
				if (printpath(db,msgout)) GOTOERROR;
				if (0>fputs(de->d_name,msgout)) GOTOERROR;
				if (0>fputc('\n',msgout)) GOTOERROR;
			}
			continue; // ignore files that are too new
		}
		{
			int isnofile;
			if (b->options.isprogress) {
				(void)printprogress(de->d_name);
				if (getmd5(&isnofile,b,md5,dirfd(dir),de->d_name,&statbuf)) GOTOERROR;
				(void)unprintprogress(de->d_name);
			} else {
				if (getmd5(&isnofile,b,md5,dirfd(dir),de->d_name,&statbuf)) GOTOERROR;
			}
			if (isnofile) {
				if (isverbose) {
					if (0>fputs("Unable to read: ",msgout)) GOTOERROR;
					if (printpath(db,msgout)) GOTOERROR;
					if (0>fputs(de->d_name,msgout)) GOTOERROR;
					if (0>fputc('\n',msgout)) GOTOERROR;
				}
				continue;
			}
		}
		file=filename_find2_filebyname(db->files.topnode,de->d_name);
		if (file) {
			file->flags|=ISFOUND_FLAG_BITROT;
			if (memcmp(md5,file->md5,LEN_MD5_BITROT)) {
				int issave=0;
				file->flags|=ISMISMATCH_FLAG_BITROT;
				if (statbuf.st_mtim.tv_sec>=b->sumfile.mtime) { // if the mtime is updated, the file changing is not odd
					issave=1;
					if (isverbose) {
						if (0>fputs("file changed: ",msgout)) GOTOERROR;
						if (printpath(db,msgout)) GOTOERROR;
						if (0>fputs(de->d_name,msgout)) GOTOERROR;
						if (0>fputc('\n',msgout)) GOTOERROR;
					}
				} else { // don't want to auto-update the md5 in case there was corruption
					if (b->options.issavechanges) {
						issave=1; 
						if (0>fputs("Updating new MD5: ",msgout)) GOTOERROR;
					} else {
						if (0>fputs("MD5 has changed: ",msgout)) GOTOERROR;
					}
					{
						if (printpath(db,msgout)) GOTOERROR;
						if (0>fputs(de->d_name,msgout)) GOTOERROR;
						if (0>fputc('\n',msgout)) GOTOERROR;
					}
				}
				if (issave) {
					memcpy(file->md5,md5,LEN_MD5_BITROT);
					b->stats.changecount+=1;
				}
			} else {
				file->flags|=ISMATCHED_FLAG_BITROT;
				if (isverbose) {
					if (0>fputs("matched: ",msgout)) GOTOERROR;
					if (printpath(db,msgout)) GOTOERROR;
					if (0>fputs(de->d_name,msgout)) GOTOERROR;
					if (0>fputc('\n',msgout)) GOTOERROR;
				}
			}
		} else {
			if (b->options.isnothingnew) {
				if (isverbose) {
					if (0>fputs("skipping new file: ",msgout)) GOTOERROR;
					if (printpath(db,msgout)) GOTOERROR;
					if (0>fputs(de->d_name,msgout)) GOTOERROR;
					if (0>fputc('\n',msgout)) GOTOERROR;
				}
			} else {
				if (!(file=ALLOC_blockmem(&b->blockmem,struct file_bitrot))) GOTOERROR;
				clear_file_bitrot(file);
				if (!(file->name=strdup_blockmem(&b->blockmem,de->d_name))) GOTOERROR;
				file->flags=ISFOUND_FLAG_BITROT;
				memcpy(file->md5,md5,LEN_MD5_BITROT);
				(void)addnode2_filebyname(&db->files.topnode,file);
				b->stats.changecount+=1;
				if (isverbose) {
					if (0>fputs("new file: ",msgout)) GOTOERROR;
					if (printpath(db,msgout)) GOTOERROR;
					if (0>fputs(de->d_name,msgout)) GOTOERROR;
					if (0>fputc('\n',msgout)) GOTOERROR;
				}
			}
		}
	// if S_ISREG
	} else if (S_ISDIR(statbuf.st_mode)) {
		struct dir_bitrot *ndb;
		if (findoradd_dir(&ndb,b,db,de->d_name,ISFOUND_FLAG_BITROT)) GOTOERROR;
		if (scandirB(b,ndb,dir,de->d_name)) GOTOERROR;
	// if S_ISDIR
	} else { // special file
		if (isverbose) {
			if (0>fputs("ignoring special: ",msgout)) GOTOERROR;
			if (printpath(db,msgout)) GOTOERROR;
			if (0>fputs(de->d_name,msgout)) GOTOERROR;
			if (0>fputc('\n',msgout)) GOTOERROR;
		}
	}
}

(ignore)closedir(dir);
return 0;
error:
	if (dir) closedir(dir);
	return -1;
}

int scandir_bitrot(struct bitrot *b, char *dirname) {
if (scandirB(b,&b->topdir,NULL,dirname)) GOTOERROR;
return 0;
error:
	return -1;
}

CLEARFUNC(tarvars_bitrot);

int init_tarvars_bitrot(struct tarvars_bitrot *tb) {
unsigned char *bytes;
bytes=tb->header.bytes;
tb->header.fields.f_name=bytes+0;
tb->header.fields.f_mode=bytes+100;
tb->header.fields.f_uid=bytes+108;
tb->header.fields.f_gid=bytes+116;
tb->header.fields.f_size=bytes+124;
tb->header.fields.f_mtime=bytes+136;
tb->header.fields.f_chksum=bytes+148;
tb->header.fields.f_typeflag=bytes+156;
tb->header.fields.f_linkname=bytes+157;
tb->header.fields.f_magic=bytes+257;
tb->header.fields.f_version=bytes+263;
tb->header.fields.f_uname=bytes+265;
tb->header.fields.f_gname=bytes+297;
tb->header.fields.f_devmajor=bytes+329;
tb->header.fields.f_devminor=bytes+337;
tb->header.fields.f_prefix=bytes+345;

tb->state=HEADER_STATE_TARVARS_BITROT;
tb->header.bytesleft=512;

if (!(tb->filename=malloc(MAX_FILENAME_TARVARS_BITROT+1))) GOTOERROR;
return 0;
error:
	return -1;
}

void deinit_tarvars_bitrot(struct tarvars_bitrot *tb) {
iffree(tb->filename);
}

static uint64_t octal12tou64(unsigned char *str) {
// reads at most 12 bytes
uint64_t u64;
switch (*str) {
	case '0': u64=0; break;
	case '1': u64=1; break;
	case '2': u64=2; break;
	case '3': u64=3; break;
	case '4': u64=4; break;
	case '5': u64=5; break;
	case '6': u64=6; break;
	case '7': u64=7; break;
	default: return 0;
}
unsigned char *strlast=str+11;
str++;
while (1) {
	switch (*str) {
		case '0': u64=u64*8; break;
		case '1': u64=u64*8+1; break;
		case '2': u64=u64*8+2; break;
		case '3': u64=u64*8+3; break;
		case '4': u64=u64*8+4; break;
		case '5': u64=u64*8+5; break;
		case '6': u64=u64*8+6; break;
		case '7': u64=u64*8+7; break;
		default: return u64;
	}
	if (str==strlast) return u64;
	str++;
}
}

static int parsefields_tarvars(struct tarvars_bitrot *tb) {
unsigned char gnumagic[]={'u','s','t','a','r',' ',' ',0};
unsigned char posix1988[]={'u','s','t','a','r',0,'0','0'};
tb->header.parsed.namelen=strnlen((char *)tb->header.fields.f_name,100);
tb->header.parsed.size=octal12tou64(tb->header.fields.f_size);
tb->header.parsed.mtime=octal12tou64(tb->header.fields.f_mtime);
if (memcmp(tb->header.fields.f_magic,gnumagic,8)) {
} else if (memcmp(tb->header.fields.f_magic,posix1988,8)) {
} else {
	int i;
	fprintf(stderr,"%s:%d file has unsupported magic value/format ",__FILE__,__LINE__);
	for (i=0;i<8;i++) {
		fprintf(stderr,"%02x",tb->header.fields.f_magic[i]);
	}
	fprintf(stderr,"\n");
	GOTOERROR;
}
tb->header.parsed.prefixlen=strnlen((char *)tb->header.fields.f_name,155);
return 0;
error:
	return -1;
}

static inline int is512zeros(void *p) {
uint64_t *u64s;
int i;
u64s=p;
for (i=64;i>0;i--) {
	if (*u64s) return 0;
	u64s+=1;
}
return 1;
}

static int header_scantar(unsigned int *consumed_out,
		struct bitrot *b, struct tarvars_bitrot *tb, unsigned char *bytes, unsigned int len) {
unsigned int bytesleft;

bytesleft=tb->header.bytesleft;
if (len<bytesleft) { // staying in header_
	memcpy(tb->header.bytes+512-bytesleft,bytes,len);
	tb->header.bytesleft=bytesleft-len;
	*consumed_out=len;
} else {
	memcpy(tb->header.bytes+512-bytesleft,bytes,bytesleft);
	if (is512zeros(tb->header.bytes)) {
		tb->state=ENDBLOCKS_STATE_TARVARS_BITROT;
		tb->endblocks.bytesleft=512;
		*consumed_out=bytesleft;
	} else {
		if (parsefields_tarvars(tb)) GOTOERROR;
		tb->state=CONSIDER_STATE_TARVARS_BITROT;
		*consumed_out=bytesleft;
	}
}
return 0;
error:
	return -1;
}

static inline int isregular_scantar(struct tarvars_bitrot *tb) {
// note: in old (non-magic) files, dirs also have typeflag='0' and are distinguished by '/' tails in f_name
if (*tb->header.fields.f_typeflag=='0') return 1; // regular file, or old-style directory as well
if (*tb->header.fields.f_typeflag=='7') return 1; // contiguous file
if (*tb->header.fields.f_typeflag==0) return 1; // supported alternative
return 0;
}
static inline int islonglink_scantar(struct tarvars_bitrot *tb) {
if (*tb->header.fields.f_typeflag=='L') return 1; // GNU LongLink
return 0;
}
static inline int isignored_scantar(struct bitrot *b, struct tarvars_bitrot *tb) {
if (tb->header.parsed.mtime>b->options.ceiling_mtime) return 1;
return 0;
}

static int consider_scantar(struct bitrot *b, struct tarvars_bitrot *tb, unsigned char *bytes, unsigned int len) {
uint64_t size;

#if 0
fprintf(stderr,"%s:%d",__FILE__,__LINE__);
fputs(" name: ",stderr);
fwrite(tb->header.fields.f_name,tb->header.parsed.namelen,1,stderr);
fputs(" prefix: ",stderr);
fwrite(tb->header.fields.f_prefix,tb->header.parsed.prefixlen,1,stderr);
fprintf(stderr," size: %llu",tb->header.parsed.size);
fprintf(stderr," mtime: %llu",tb->header.parsed.mtime);
fprintf(stderr," typeflag: %u",*tb->header.fields.f_typeflag);
fputs("\n",stderr);
#endif

size=tb->header.parsed.size;
if (isregular_scantar(tb) && !isignored_scantar(b,tb)) {
	if (tb->header.parsed.filetype==LONGLINK_FILETYPE_TARVARS_BITROT) tb->header.parsed.filetype=LONGFILE_FILETYPE_TARVARS_BITROT;
	else tb->header.parsed.filetype=REGULAR_FILETYPE_TARVARS_BITROT;
	if (!size) {
#if LEN_MD5_BITROT != 16
#error
#endif
		static unsigned char zeromd5[16]={0xd4,0x1d,0x8c,0xd9,0x8f,0x00,0xb2,0x04,0xe9,0x80,0x09,0x98,0xec,0xf8,0x42,0x7e};
		tb->state=ENDFILE_STATE_TARVARS_BITROT;
		memcpy(tb->checksum.md5,zeromd5,16);
	} else {
		tb->state=CHECKSUM_STATE_TARVARS_BITROT;
		tb->checksum.inputbytesleft=((size-1)|511)+1;
		tb->checksum.databytesleft=size;
#ifdef OPENSSL
		if (1!=MD5_Init(&tb->checksum.ctx)) GOTOERROR;
#elif GNUTLS
		(void)MD5_Init(&tb->checksum.ctx);
#else
		(void)clear_context_md5(&tb->checksum.ctx);
#endif
		if (b->options.isprogress) {
			(void)printprogress((char *)tb->header.fields.f_name); // limits to 60
		}
	}
} else if (islonglink_scantar(tb)) {
	tb->header.parsed.filetype=LONGLINK_FILETYPE_TARVARS_BITROT;
	if (strcmp((char *)tb->header.fields.f_name,"././@LongLink")) {
		fprintf(stderr,"%s:%d Tar type L without recognized ././@LongLink\n",__FILE__,__LINE__);
		GOTOERROR;
	}
	if (!size) {
		fprintf(stderr,"%s:%d Tar type L without size\n",__FILE__,__LINE__);
		GOTOERROR;
	}
	if (size>MAX_FILENAME_TARVARS_BITROT) {
		fprintf(stderr,"%s:%d Tar type L size is too large: %llu\n",__FILE__,__LINE__,size);
		GOTOERROR;
	}
	tb->state=SLURP_STATE_TARVARS_BITROT;
	tb->slurp.inputbytesleft=((size-1)|511)+1;
	tb->slurp.databytesleft=size;
	tb->slurp.cursor=(unsigned char *)tb->filename;
	tb->filename[size]='\0';
} else {
	tb->header.parsed.filetype=NONE_FILETYPE_TARVARS_BITROT; // need to clear a previous LongLink record
	if (!size) {
		tb->state=HEADER_STATE_TARVARS_BITROT;
		tb->header.bytesleft=512;
	} else {
		tb->state=SKIPPING_STATE_TARVARS_BITROT;
		tb->checksum.inputbytesleft=((size-1)|511)+1;
	}
}

return 0;
error:
	return -1;
}

static int slurp_scantar(unsigned int *consumed_out,
		struct bitrot *b, struct tarvars_bitrot *tb, unsigned char *bytes, unsigned int len) {
unsigned int consumed;
uint64_t dbl;
dbl=tb->slurp.databytesleft;
if (dbl) {
	if (dbl<=len) {
		tb->slurp.databytesleft=0;
		tb->slurp.inputbytesleft-=dbl;
		memcpy(tb->slurp.cursor,bytes,dbl);
//		tb->slurp.cursor+=dbl;
		if (!tb->slurp.inputbytesleft) {
			tb->state=ENDSLURP_STATE_TARVARS_BITROT;
		}
		consumed=dbl;
	} else {
		tb->slurp.databytesleft=dbl-len;
		tb->slurp.inputbytesleft-=len;
		memcpy(tb->slurp.cursor,bytes,len);
		tb->slurp.cursor+=len;
		consumed=len;
	}
} else {
	uint64_t ibl;
	ibl=tb->slurp.inputbytesleft;
	if (ibl<=len) {
		tb->state=ENDSLURP_STATE_TARVARS_BITROT;
		consumed=ibl;
	} else {
		tb->slurp.inputbytesleft-=len;
		consumed=len;
	}
}
*consumed_out=consumed;
return 0;
}

static void endslurp_scantar(struct bitrot *b, struct tarvars_bitrot *tb, unsigned char *bytes, unsigned int len) {
tb->state=HEADER_STATE_TARVARS_BITROT;
tb->header.bytesleft=512;
}

static int checksum_scantar(unsigned int *consumed_out,
		struct bitrot *b, struct tarvars_bitrot *tb, unsigned char *bytes, unsigned int len) {
unsigned int consumed;
uint64_t dbl;
dbl=tb->checksum.databytesleft;
if (dbl) {
	if (dbl<=len) {
#ifdef OPENSSL
		if (1!=MD5_Update(&tb->checksum.ctx,bytes,dbl)) GOTOERROR;
#elif GNUTLS
		(void)MD5_Update(&tb->checksum.ctx,bytes,dbl);
#else
		(void)addbytes_context_md5(&tb->checksum.ctx,bytes,dbl);
#endif
#ifdef OPENSSL
		if (1!=MD5_Final(tb->checksum.md5,&tb->checksum.ctx)) GOTOERROR;
#elif GNUTLS
		(void)MD5_Final(tb->checksum.md5,&tb->checksum.ctx);
#else
		(void)finish_context_md5(tb->checksum.md5,&tb->checksum.ctx);
#endif
		tb->checksum.databytesleft=0;
		tb->checksum.inputbytesleft-=dbl;
		if (!tb->checksum.inputbytesleft) {
			tb->state=ENDFILE_STATE_TARVARS_BITROT;
		}
		consumed=dbl;
	} else {
#ifdef OPENSSL
		if (1!=MD5_Update(&tb->checksum.ctx,bytes,len)) GOTOERROR;
#elif GNUTLS
		(void)MD5_Update(&tb->checksum.ctx,bytes,len);
#else
		(void)addbytes_context_md5(&tb->checksum.ctx,bytes,len);
#endif
		tb->checksum.databytesleft=dbl-len;
		tb->checksum.inputbytesleft-=len;
		consumed=len;
	}
} else {
	uint64_t ibl;
	ibl=tb->checksum.inputbytesleft;
	if (ibl<=len) {
		tb->state=ENDFILE_STATE_TARVARS_BITROT;
		consumed=ibl;
	} else {
		tb->checksum.inputbytesleft-=len;
		consumed=len;
	}
}
*consumed_out=consumed;
return 0;
}

static inline void getfullpath_tarvars(char *dest, struct tarvars_bitrot *tb) {
// dest should be 257 chars
unsigned int namelen=100,prefixlen=155;
unsigned char *name,*prefix;
name=tb->header.fields.f_name;
prefix=tb->header.fields.f_prefix;
if (*prefix) {
	while (1) {
		*dest=*prefix;
		dest++;
		prefixlen--;
		if (!prefixlen) break;
		prefix++;
		if (!*prefix) break;
	}
	*dest='/';
	dest++;
}

while (1) {
	if (!*name) break;
	*dest=*name;
	dest++;
	namelen--;
	if (!namelen) break;
	name++;
}
*dest=0;
}

static int endfile_scantar(struct bitrot *b, struct tarvars_bitrot *tb, unsigned char *bytes, unsigned int len) {
char *fullpath;
struct file_bitrot *file;
struct dir_bitrot *dir;
char *filename;
FILE *msgout;

msgout=b->options.msgout;
fullpath=tb->filename;

tb->state=HEADER_STATE_TARVARS_BITROT;
tb->header.bytesleft=512;

if (tb->header.parsed.filetype==REGULAR_FILETYPE_TARVARS_BITROT) {
#if MAX_FILENAME_TARVARS_BITROT	< 256
#error
#endif
		(void)getfullpath_tarvars(fullpath,tb);
}
// nothing to do for LONGFILE_FILETYPE_TARVARS_BITROT, name is already in fullpath

if (b->options.isprogress) {
	(void)unprintprogress((char *)tb->header.fields.f_name); // limits to 60
}

#if 0
fprintf(stderr,"%s:%d",__FILE__,__LINE__);
fputs(" fullpath: ",stderr);
fputs(fullpath,stderr);
fprintf(stderr," size: %llu",tb->header.parsed.size);
fprintf(stderr," mtime: %llu",tb->header.parsed.mtime);
fprintf(stderr," typeflag: %u",*tb->header.fields.f_typeflag);
fputs("\n",stderr);
#endif

dir=&b->topdir;
filename=fullpath;
while (1) {
	char *slash;
	slash=strchr(filename,'/');
	if (!slash) break;
	*slash=0;
	if (!strcmp(filename,".")) {
	} else {
		if (findoradd_dir(&dir,b,dir,filename,ISFOUND_FLAG_BITROT)) GOTOERROR;
	}
	*slash='/';
	filename=slash+1;
}

file=filename_find2_filebyname(dir->files.topnode,filename);
if (file) {
	file->flags|=ISFOUND_FLAG_BITROT;
	if (memcmp(tb->checksum.md5,file->md5,LEN_MD5_BITROT)) {
		int issave=0;
		file->flags|=ISMISMATCH_FLAG_BITROT;
		if (tb->header.parsed.mtime >=b->sumfile.mtime) { // if the mtime is updated, the file changing is not odd
			issave=1;
			if (b->options.isverbose) {
				if (0>fputs("file changed: ",msgout)) GOTOERROR;
				if (0>fputs(fullpath,msgout)) GOTOERROR;
				if (0>fputc('\n',msgout)) GOTOERROR;
			}
		} else { // don't want to auto-update the md5 in case there was corruption
			if (b->options.issavechanges) {
				issave=1; 
				if (0>fputs("Updating new MD5: ",msgout)) GOTOERROR;
			} else {
				if (0>fputs("MD5 has changed: ",msgout)) GOTOERROR;
			}
			{
				if (0>fputs(fullpath,msgout)) GOTOERROR;
				if (0>fputc('\n',msgout)) GOTOERROR;
			}
		}
		if (issave) {
			memcpy(file->md5,tb->checksum.md5,LEN_MD5_BITROT);
			b->stats.changecount+=1;
		}
	} else {
		file->flags|=ISMATCHED_FLAG_BITROT;
		if (b->options.isverbose) {
			if (0>fputs("matched: ",msgout)) GOTOERROR;
			if (0>fputs(fullpath,msgout)) GOTOERROR;
			if (0>fputc('\n',msgout)) GOTOERROR;
		}
	}
} else {
	if (b->options.isnothingnew) {
		if (b->options.isverbose) {
			if (0>fputs("skipping new file: ",msgout)) GOTOERROR;
			if (0>fputs(fullpath,msgout)) GOTOERROR;
			if (0>fputc('\n',msgout)) GOTOERROR;
		}
	} else {
		if (!(file=ALLOC_blockmem(&b->blockmem,struct file_bitrot))) GOTOERROR;
		clear_file_bitrot(file);
		if (!(file->name=strdup_blockmem(&b->blockmem,filename))) GOTOERROR;
		file->flags=ISFOUND_FLAG_BITROT;
		memcpy(file->md5,tb->checksum.md5,LEN_MD5_BITROT);
		(void)addnode2_filebyname(&dir->files.topnode,file);
		b->stats.changecount+=1;
		if (b->options.isverbose) {
			if (0>fputs("new file: ",msgout)) GOTOERROR;
			if (0>fputs(fullpath,msgout)) GOTOERROR;
			if (0>fputc('\n',msgout)) GOTOERROR;
		}
	}
}

return 0;
error:
	return -1;
}

static int skipping_scantar(unsigned int *consumed_out,
		struct bitrot *b, struct tarvars_bitrot *tb, unsigned char *bytes, unsigned int len) {
uint64_t ibl;
ibl=tb->checksum.inputbytesleft;
if (ibl<=len) {
	tb->state=HEADER_STATE_TARVARS_BITROT;
	tb->header.bytesleft=512;
	*consumed_out=ibl;
} else {
	tb->checksum.inputbytesleft-=len;
	*consumed_out=len;
}
return 0;
}

static int endblocks_scantar(unsigned int *consumed_out,
		struct bitrot *b, struct tarvars_bitrot *tb, unsigned char *bytes, unsigned int len) {
unsigned int bytesleft;
bytesleft=tb->endblocks.bytesleft;
if (bytesleft>len) {
	memcpy(tb->header.bytes+512-bytesleft,bytes,bytesleft);
	tb->endblocks.bytesleft=bytesleft-len;
	*consumed_out=len;
} else if (bytesleft<=len) {
	memcpy(tb->header.bytes+512-bytesleft,bytes,bytesleft);
	if (!is512zeros(tb->header.bytes)) GOTOERROR;
	tb->state=FINISHED_STATE_TARVARS_BITROT;
	*consumed_out=bytesleft;
}
return 0;
error:
	return -1;
}

int scantar_bitrot(struct bitrot *b, struct tarvars_bitrot *tb, unsigned char *bytes, unsigned int len) {
while (len) {
	unsigned int consumed;
	switch (tb->state) {
		case HEADER_STATE_TARVARS_BITROT:
			if (header_scantar(&consumed,b,tb,bytes,len)) GOTOERROR;
			break;
		case CONSIDER_STATE_TARVARS_BITROT:
			if (consider_scantar(b,tb,bytes,len)) GOTOERROR;
			continue;
		case SLURP_STATE_TARVARS_BITROT:
			(void)slurp_scantar(&consumed,b,tb,bytes,len);
			break;
		case ENDSLURP_STATE_TARVARS_BITROT:
			(void)endslurp_scantar(b,tb,bytes,len);
			continue;
		case CHECKSUM_STATE_TARVARS_BITROT:
			if (checksum_scantar(&consumed,b,tb,bytes,len)) GOTOERROR;
			break;
		case ENDFILE_STATE_TARVARS_BITROT:
			if (endfile_scantar(b,tb,bytes,len)) GOTOERROR;
			continue;
		case SKIPPING_STATE_TARVARS_BITROT:
			if (skipping_scantar(&consumed,b,tb,bytes,len)) GOTOERROR;
			break;
		case ENDBLOCKS_STATE_TARVARS_BITROT:
			if (endblocks_scantar(&consumed,b,tb,bytes,len)) GOTOERROR;
			break;
		case FINISHED_STATE_TARVARS_BITROT: return 0; // trailing bytes
		default: GOTOERROR;
	}
#if 1
	if (!consumed) GOTOERROR;
#endif
	bytes+=consumed;
	len-=consumed;
}
return 0;
error:
	return -1;
}
