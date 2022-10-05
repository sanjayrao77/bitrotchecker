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
#define LEN_MD5_BITROT	16
#define INVALID_DEVT_BITROT	0

#define ISFOUND_FLAG_BITROT		1
#define ISINFILE_FLAG_BITROT		2
#define ISMATCHED_FLAG_BITROT	4
#define ISMISMATCH_FLAG_BITROT	8

#define READCHUNK_BITROT	(128*1024)

struct file_bitrot {
	char *name;
	unsigned int flags;
	unsigned char md5[LEN_MD5_BITROT];
	struct {
		signed char balance;
		struct file_bitrot *left,*right;
	} treevars;
};

struct dir_bitrot {
	struct dir_bitrot *parent;
	char *name;
	unsigned int flags;
	struct {
		struct dir_bitrot *topnode;
	} children;
	struct {
		struct file_bitrot *topnode;
	} files;
	struct {
		signed char balance;
		struct dir_bitrot *left,*right;
	} treevars;
};

struct bitrot {
	struct {
		dev_t xdev;
		char *path;
	} rootdir;
	struct {
		uint64_t mtime; // don't print mismatches if a file mtime is newer than this
		char *name;
	} sumfile;
	struct {
		unsigned int ptrmax;
		unsigned char *ptr;
#ifdef USEMMAP
	} _iobuffer;
#else
	} iobuffer;
#endif
	struct {
		unsigned int changecount;
		uint64_t bytesprocessed;
	} stats;
	struct {
		time_t nextupdate;
		int isprinted;
		int linelength;
#define MAX_LINE_PROGRESS_BITROT	127
		char line[MAX_LINE_PROGRESS_BITROT+1];
	} progress;
	struct {
		FILE *msgout;
		uint64_t ceiling_mtime; // don't collect files newer than this
		unsigned int readusleep;
		int isonefilesystem;
		int isprogress;
		int isverbose;
		int isdryrun;
		int isnothingnew;
		int isfollow; // follow symlinks
		int issavechanges;
	} options;
	struct dir_bitrot topdir;
	struct blockmem blockmem;
};
H_CLEARFUNC(bitrot);

int init_bitrot(struct bitrot *bitrot, int isnoio);
void deinit_bitrot(struct bitrot *bitrot);
void unprintprogress_bitrot(struct bitrot *b);
int loadfile_bitrot(int *isnotfound_out, struct bitrot *bitrot, char *sumfile);
int writefile_bitrot(struct bitrot *b, char *filename);
int printtree_bitrot(struct bitrot *b, FILE *fout);
int scandir_bitrot(struct bitrot *b, char *dirname);
