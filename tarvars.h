/*
 * tarvars.h
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


#define ERROR_STATE_TARVARS_BITROT	0
// collecting header
#define HEADER_STATE_TARVARS_BITROT	1
#define CONSIDER_STATE_TARVARS_BITROT	2
#define CHECKSUM_STATE_TARVARS_BITROT	3
#define ENDFILE_STATE_TARVARS_BITROT	4
#define SKIPPING_STATE_TARVARS_BITROT	5
// 2 blank blocks at the end
#define ENDBLOCKS_STATE_TARVARS_BITROT	6
#define FINISHED_STATE_TARVARS_BITROT	7
struct tarvars_bitrot {
	int state;
	struct {
		struct {
			unsigned char *f_name;
			unsigned char *f_mode;
			unsigned char *f_uid;
			unsigned char *f_gid;
			unsigned char *f_size;
			unsigned char *f_mtime;
			unsigned char *f_chksum;
			unsigned char *f_typeflag;
			unsigned char *f_linkname;
			unsigned char *f_magic;
			unsigned char *f_version;
			unsigned char *f_uname;
			unsigned char *f_gname;
			unsigned char *f_devmajor;
			unsigned char *f_devminor;
			unsigned char *f_prefix;
		} fields;
		struct {
			unsigned int namelen;
			uint64_t size;
			uint64_t mtime;
			unsigned int prefixlen;
		} parsed;
		unsigned int bytesleft;
		unsigned char bytes[512];
	} header;
	struct {
		unsigned char md5[LEN_MD5_BITROT];
		uint64_t inputbytesleft; // aligned to blocksize
		uint64_t databytesleft; // set to 0 to skip checksum
		MD5_CTX ctx;
	} checksum;
	struct {
		unsigned int bytesleft;
	} endblocks;
};
H_CLEARFUNC(tarvars_bitrot);

void voidinit_tarvars_bitrot(struct tarvars_bitrot *tb);
int scantar_bitrot(struct bitrot *b, struct tarvars_bitrot *tb, unsigned char *bytes, unsigned int len);
