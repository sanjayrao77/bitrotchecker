/*
 * main.c
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
#ifdef OSX
#include "common/osx.h"
#endif
#include "common/conventions.h"
#include "common/blockmem.h"

#include "bitrot.h"
#include "tarvars.h"

static int writen(int fd, unsigned char *msg, unsigned int len) {
while (len) {
	ssize_t k;
	k=write(fd,(char *)msg,len);
	if (k<=0) {
		if (k && (errno==EINTR)) continue;
		return -1;
	}
	len-=k;
	msg+=k;
}
return 0;
}

static void printhelp(int isstderr) {
FILE *fout;
fout=stdout;
if (isstderr) fout=stderr;
fprintf(fout,"bitrotchecker scans a directory for changes, using a file listing md5 digests, compatible with md5sum\n");
fprintf(fout,"Usage: bitrotchecker [options] checksumfile directory\n");
fprintf(fout,"  --dry-run: don't overwrite checksumfile\n");
fprintf(fout,"  --follow: follow symlinks\n");
fprintf(fout,"  --nothingnew: only process files in checksumfile\n");
fprintf(fout,"  --nottoday: skip files that have changed recently\n");
fprintf(fout,"  --one-file-system: don't cross filesystems when scanning directory\n");
fprintf(fout,"  --progress: print filenames along the way\n");
fprintf(fout,"  --savechanges: update md5 values for files that have changed\n");
fprintf(fout,"  --slow: limit reading to approx 13MB/sec\n");
fprintf(fout,"  --slower: limit reading to approx 1.3MB/sec\n");
fprintf(fout,"  --slowest: limit reading to approx 130KB/sec\n");
fprintf(fout,"  --tar: read a tar file from stdin instead of scanning\n");
fprintf(fout,"  --tar-stdout: relay tar file to stdout\n");
fprintf(fout,"  --verbose: print extra information\n");
fprintf(fout,"Examples:\n");
fprintf(fout,"To build digests: \"$ bitrotchecker --progress  /tmp/md5s.txt /home/myhome\"\n");
fprintf(fout,"To update digests: \"$ bitrotchecker /tmp/md5s.txt /home/myhome\"\n");
fprintf(fout,"To verify old files: \"$ bitrotchecker --nothingnew /tmp/md5s.txt /home/myhome\"\n");
fprintf(fout,"To verify old files, with md5sum: \"$ cd /home/myhome ; md5sum -c /tmp/md5s.txt\"\n");
}

int main(int argc, char **argv) {
struct bitrot bitrot;
struct tarvars_bitrot tarvars;
char *sumfile=NULL;
char *rootdir=NULL;
unsigned char *tarbuffer=NULL;
int istar=0,istarstdout=0;
/*
--slow			: throttle io
--nottoday	: ignore mtimes newer than 24 hours
--one-file-system	: don't cross filesystems
--dry-run		: don't writefile_bitrot
--verbose		: print more
--progress	: print interactive progress
--nothingnew : don't add new files from scan
*/


#ifdef OSX
if (sizeof(off64_t)!=8) GOTOERROR;
#endif

clear_bitrot(&bitrot);
clear_tarvars_bitrot(&tarvars);

int i;
for (i=1;i<argc;i++) {
	char *arg=argv[i];
	if (!strcmp(arg,"--help")) {
		printhelp(0);
		return 0;
	} else if (!strcmp(arg,"--progress")) {
		bitrot.options.isprogress=1;
	} else if (!strcmp(arg,"--follow")) {
		bitrot.options.isfollow=1;
	} else if (!strcmp(arg,"--verbose")) {
		bitrot.options.isverbose=1;
	} else if (!strcmp(arg,"--slow")) {
		bitrot.options.readusleep=1000*10; // 100 reads per second, tops, roughly 13MB/sec
	} else if (!strcmp(arg,"--slower")) {
		bitrot.options.readusleep=1000*100; // 10 reads per second, tops, roughly 1.3MB/sec
	} else if (!strcmp(arg,"--slowest")) {
		bitrot.options.readusleep=1000*1000; // 1 reads per second, tops, roughly .13 MB/sec
	} else if (!strcmp(arg,"--nottoday")) {
		bitrot.options.ceiling_mtime=time(NULL)-24*60*60;
	} else if (!strcmp(arg,"--one-file-system")) {
		bitrot.options.isonefilesystem=1;
	} else if (!strcmp(arg,"--dry-run")) {
		bitrot.options.isdryrun=1;
	} else if (!strcmp(arg,"--savechanges")) {
		bitrot.options.issavechanges=1;
	} else if (!strcmp(arg,"--tar")) {
		istar=1;
	} else if (!strcmp(arg,"--tar-stdout")) {
		istarstdout=1;
	} else if (!strcmp(arg,"--nothingnew")) {
		bitrot.options.isnothingnew=1;
	} else if (!strncmp(arg,"--",2)) {
			fprintf(stderr,"%s:%d unknown parameter %s\n",__FILE__,__LINE__,arg);
			GOTOERROR;
	} else {
		struct stat statbuf;
		if (stat(arg,&statbuf)) {
			if (errno==ENOENT) {
				if (sumfile) {
					fprintf(stderr,"%s:%d two files specified: %s and %s\n",__FILE__,__LINE__,sumfile,arg);
					GOTOERROR;
				}
				sumfile=arg;
			} else {
				fprintf(stderr,"%s:%d error getting stat for %s\n",__FILE__,__LINE__,arg);
				GOTOERROR;
			}
		} else if (S_ISDIR(statbuf.st_mode)) {
			if (rootdir) {
				fprintf(stderr,"%s:%d two directories specified: %s and %s\n",__FILE__,__LINE__,rootdir,arg);
				GOTOERROR;
			}
			rootdir=arg;
		} else if (S_ISREG(statbuf.st_mode)) {
			if (sumfile) {
				fprintf(stderr,"%s:%d two files specified: %s and %s\n",__FILE__,__LINE__,sumfile,arg);
				GOTOERROR;
			}
			sumfile=arg;
		} else {
			fprintf(stderr,"%s:%d special file specified: %s\n",__FILE__,__LINE__,arg);
			GOTOERROR;
		}
	}
}

#ifdef TESTING
if (!rootdir) rootdir="/tmp/dirtest";
if (!sumfile) sumfile="/tmp/dirtest.md5";
#endif

if (!rootdir) {
	if (!istar) {
		printhelp(istarstdout);
		fprintf(stderr,"\n%s:%d a directory is required, to know what to scan\n",__FILE__,__LINE__);
		return 0;
	}
}
if (!sumfile) {
	printhelp(istarstdout);
	fprintf(stderr,"\n%s:%d a filename is required, to read and store md5 checksums\n",__FILE__,__LINE__);
	return 0;
}

if (init_bitrot(&bitrot)) GOTOERROR;

{
	int isnotfound;
	if (loadfile_bitrot(&isnotfound,&bitrot,sumfile)) GOTOERROR;
	if (isnotfound) {
		if (bitrot.options.isverbose) {
			fprintf(stderr,"%s:%d checksum file %s not found\n",__FILE__,__LINE__,sumfile);
		}
	}
#if 0
	fprintf(stderr,"%s:%d sumfile.mtime: %llu\n",__FILE__,__LINE__,bitrot.sumfile.mtime);
#endif
}
if (istar) {
	if (init_tarvars_bitrot(&tarvars)) GOTOERROR;
	bitrot.options.msgout=stderr;

	if (!(tarbuffer=malloc(READCHUNK_BITROT))) GOTOERROR;
	while (1) {
		int k;
		k=read(STDIN_FILENO,tarbuffer,READCHUNK_BITROT);
		if (k<=0) {
			if (!k) break;
			GOTOERROR;
		}
		if (istarstdout) {
			if (writen(STDOUT_FILENO,tarbuffer,k)) GOTOERROR;
		}
		if (scantar_bitrot(&bitrot,&tarvars,tarbuffer,k)) GOTOERROR;
		if (bitrot.options.readusleep) {
			usleep(bitrot.options.readusleep);
		}
	}
	if (tarvars.state!=FINISHED_STATE_TARVARS_BITROT) {
		if (tarvars.state!=HEADER_STATE_TARVARS_BITROT) {
			fprintf(stderr,"%s:%d Error reading tar file, archive is short\n",__FILE__,__LINE__);
			GOTOERROR;
		}
		fprintf(stderr,"%s:%d Warning reading tar file, no end-of-file\n",__FILE__,__LINE__);
	}
} else {
	bitrot.options.msgout=stdout;
	if (scandir_bitrot(&bitrot,rootdir)) GOTOERROR;
}
(void)unprintprogress_bitrot(&bitrot);

// printtree_bitrot(&bitrot,stderr);

if (!bitrot.options.isdryrun) {
	if (bitrot.stats.changecount) {
		if (writefile_bitrot(&bitrot,sumfile)) GOTOERROR;
	}
}

iffree(tarbuffer);
deinit_bitrot(&bitrot);
deinit_tarvars_bitrot(&tarvars);
return 0;
error:
	iffree(tarbuffer);
	deinit_bitrot(&bitrot);
	deinit_tarvars_bitrot(&tarvars);
	return -1;
}
