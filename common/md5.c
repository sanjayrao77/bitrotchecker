/*
 * md5.c
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
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>

#include "md5.h"

void clear_context_md5(struct context_md5 *ctx) {
static struct context_md5 blank={ .A=0x67452301, .B=0xefcdab89, .C=0x98badcfe, .D=0x10325476 };
*ctx=blank;
}

static inline void copyblocktoX(uint32_t *X, unsigned char *block) {
uint32_t u;

block+=3; u=*block; block--; u=u<<8; u|=*block; block--; u=u<<8; u|=*block; block--; u=u<<8; u|=*block; *X=u; X++;
block+=7; u=*block; block--; u=u<<8; u|=*block; block--; u=u<<8; u|=*block; block--; u=u<<8; u|=*block; *X=u; X++;
block+=7; u=*block; block--; u=u<<8; u|=*block; block--; u=u<<8; u|=*block; block--; u=u<<8; u|=*block; *X=u; X++;
block+=7; u=*block; block--; u=u<<8; u|=*block; block--; u=u<<8; u|=*block; block--; u=u<<8; u|=*block; *X=u; X++;
block+=7; u=*block; block--; u=u<<8; u|=*block; block--; u=u<<8; u|=*block; block--; u=u<<8; u|=*block; *X=u; X++;
block+=7; u=*block; block--; u=u<<8; u|=*block; block--; u=u<<8; u|=*block; block--; u=u<<8; u|=*block; *X=u; X++;
block+=7; u=*block; block--; u=u<<8; u|=*block; block--; u=u<<8; u|=*block; block--; u=u<<8; u|=*block; *X=u; X++;
block+=7; u=*block; block--; u=u<<8; u|=*block; block--; u=u<<8; u|=*block; block--; u=u<<8; u|=*block; *X=u; X++;
block+=7; u=*block; block--; u=u<<8; u|=*block; block--; u=u<<8; u|=*block; block--; u=u<<8; u|=*block; *X=u; X++;
block+=7; u=*block; block--; u=u<<8; u|=*block; block--; u=u<<8; u|=*block; block--; u=u<<8; u|=*block; *X=u; X++;
block+=7; u=*block; block--; u=u<<8; u|=*block; block--; u=u<<8; u|=*block; block--; u=u<<8; u|=*block; *X=u; X++;
block+=7; u=*block; block--; u=u<<8; u|=*block; block--; u=u<<8; u|=*block; block--; u=u<<8; u|=*block; *X=u; X++;
block+=7; u=*block; block--; u=u<<8; u|=*block; block--; u=u<<8; u|=*block; block--; u=u<<8; u|=*block; *X=u; X++;
block+=7; u=*block; block--; u=u<<8; u|=*block; block--; u=u<<8; u|=*block; block--; u=u<<8; u|=*block; *X=u; X++;
block+=7; u=*block; block--; u=u<<8; u|=*block; block--; u=u<<8; u|=*block; block--; u=u<<8; u|=*block; *X=u; X++;
block+=7; u=*block; block--; u=u<<8; u|=*block; block--; u=u<<8; u|=*block; block--; u=u<<8; u|=*block; *X=u;
}

#define T_1 0xd76aa478
#define T_2 0xe8c7b756
#define T_3 0x242070db
#define T_4 0xc1bdceee 
#define T_5 0xf57c0faf
#define T_6 0x4787c62a
#define T_7 0xa8304613
#define T_8 0xfd469501 
#define T_9 0x698098d8
#define T_10 0x8b44f7af
#define T_11 0xffff5bb1
#define T_12 0x895cd7be 
#define T_13 0x6b901122
#define T_14 0xfd987193
#define T_15 0xa679438e
#define T_16 0x49b40821 
#define T_17 0xf61e2562
#define T_18 0xc040b340
#define T_19 0x265e5a51
#define T_20 0xe9b6c7aa 
#define T_21 0xd62f105d
#define T_22 0x02441453
#define T_23 0xd8a1e681
#define T_24 0xe7d3fbc8 
#define T_25 0x21e1cde6
#define T_26 0xc33707d6
#define T_27 0xf4d50d87
#define T_28 0x455a14ed 
#define T_29 0xa9e3e905
#define T_30 0xfcefa3f8
#define T_31 0x676f02d9
#define T_32 0x8d2a4c8a 
#define T_33 0xfffa3942
#define T_34 0x8771f681
#define T_35 0x6d9d6122
#define T_36 0xfde5380c 
#define T_37 0xa4beea44
#define T_38 0x4bdecfa9
#define T_39 0xf6bb4b60
#define T_40 0xbebfbc70 
#define T_41 0x289b7ec6
#define T_42 0xeaa127fa
#define T_43 0xd4ef3085
#define T_44 0x04881d05 
#define T_45 0xd9d4d039
#define T_46 0xe6db99e5
#define T_47 0x1fa27cf8
#define T_48 0xc4ac5665 
#define T_49 0xf4292244
#define T_50 0x432aff97
#define T_51 0xab9423a7
#define T_52 0xfc93a039 
#define T_53 0x655b59c3
#define T_54 0x8f0ccc92
#define T_55 0xffeff47d
#define T_56 0x85845dd1 
#define T_57 0x6fa87e4f
#define T_58 0xfe2ce6e0
#define T_59 0xa3014314
#define T_60 0x4e0811a1 
#define T_61 0xf7537e82
#define T_62 0xbd3af235
#define T_63 0x2ad7d2bb
#define T_64 0xeb86d391 

#define circleleft4(a) ( ((a)<<4)|((a)>>(32-4)) )
#define circleleft5(a) ( ((a)<<5)|((a)>>(32-5)) )
#define circleleft6(a) ( ((a)<<6)|((a)>>(32-6)) )
#define circleleft7(a) ( ((a)<<7)|((a)>>(32-7)) )
#define circleleft9(a) ( ((a)<<9)|((a)>>(32-9)) )
#define circleleft10(a) ( ((a)<<10)|((a)>>(32-10)) )
#define circleleft11(a) ( ((a)<<11)|((a)>>(32-11)) )
#define circleleft12(a) ( ((a)<<12)|((a)>>(32-12)) )
#define circleleft14(a) ( ((a)<<14)|((a)>>(32-14)) )
#define circleleft15(a) ( ((a)<<15)|((a)>>(32-15)) )
#define circleleft16(a) ( ((a)<<16)|((a)>>(32-16)) )
#define circleleft17(a) ( ((a)<<17)|((a)>>(32-17)) )
#define circleleft20(a) ( ((a)<<20)|((a)>>(32-20)) )
#define circleleft21(a) ( ((a)<<21)|((a)>>(32-21)) )
#define circleleft22(a) ( ((a)<<22)|((a)>>(32-22)) )
#define circleleft23(a) ( ((a)<<23)|((a)>>(32-23)) )

static inline uint32_t F(uint32_t x, uint32_t y, uint32_t z) { return (x&y)|((~x)&z); }
static inline uint32_t G(uint32_t x, uint32_t y, uint32_t z) { return (x&z)|(y&(~z)); }
static inline uint32_t H(uint32_t x, uint32_t y, uint32_t z) { return x^y^z; }
static inline uint32_t I(uint32_t x, uint32_t y, uint32_t z) { return y^(x|(~z)); }

static inline void addblock(struct context_md5 *ctx, unsigned char *block) {
uint32_t X[16];
uint32_t A,B,C,D;

(void)copyblocktoX(X,block);

A=ctx->A;
B=ctx->B;
C=ctx->C;
D=ctx->D;

#define ROUND1(a,b,c,d,k,s,i) do { a = b + circleleft##s(a + F(b,c,d) + X[k] + T_##i); } while (0)

ROUND1(A,B,C,D,0,7,1);
ROUND1(D,A,B,C,1,12,2);
ROUND1(C,D,A,B,2,17,3);
ROUND1(B,C,D,A,3,22,4);

ROUND1(A,B,C,D,4,7,5);
ROUND1(D,A,B,C,5,12,6);
ROUND1(C,D,A,B,6,17,7);
ROUND1(B,C,D,A,7,22,8);

ROUND1(A,B,C,D,8,7,9);
ROUND1(D,A,B,C,9,12,10);
ROUND1(C,D,A,B,10,17,11);
ROUND1(B,C,D,A,11,22,12);

ROUND1(A,B,C,D,12,7,13);
ROUND1(D,A,B,C,13,12,14);
ROUND1(C,D,A,B,14,17,15);
ROUND1(B,C,D,A,15,22,16);

#define ROUND2(a,b,c,d,k,s,i) do { a = b + circleleft##s(a + G(b,c,d) + X[k] + T_##i); } while (0)

ROUND2(A,B,C,D,1,5,17);
ROUND2(D,A,B,C,6,9,18);
ROUND2(C,D,A,B,11,14,19);
ROUND2(B,C,D,A,0,20,20);

ROUND2(A,B,C,D,5,5,21);
ROUND2(D,A,B,C,10,9,22);
ROUND2(C,D,A,B,15,14,23);
ROUND2(B,C,D,A,4,20,24);

ROUND2(A,B,C,D,9,5,25);
ROUND2(D,A,B,C,14,9,26);
ROUND2(C,D,A,B,3,14,27);
ROUND2(B,C,D,A,8,20,28);

ROUND2(A,B,C,D,13,5,29);
ROUND2(D,A,B,C,2,9,30);
ROUND2(C,D,A,B,7,14,31);
ROUND2(B,C,D,A,12,20,32);

#define ROUND3(a,b,c,d,k,s,i) do { a = b + circleleft##s(a + H(b,c,d) + X[k] + T_##i); } while (0)

ROUND3(A,B,C,D,5,4,33);
ROUND3(D,A,B,C,8,11,34);
ROUND3(C,D,A,B,11,16,35);
ROUND3(B,C,D,A,14,23,36);

ROUND3(A,B,C,D,1,4,37);
ROUND3(D,A,B,C,4,11,38);
ROUND3(C,D,A,B,7,16,39);
ROUND3(B,C,D,A,10,23,40);

ROUND3(A,B,C,D,13,4,41);
ROUND3(D,A,B,C,0,11,42);
ROUND3(C,D,A,B,3,16,43);
ROUND3(B,C,D,A,6,23,44);

ROUND3(A,B,C,D,9,4,45);
ROUND3(D,A,B,C,12,11,46);
ROUND3(C,D,A,B,15,16,47);
ROUND3(B,C,D,A,2,23,48);

#define ROUND4(a,b,c,d,k,s,i) do { a = b + circleleft##s(a + I(b,c,d) + X[k] + T_##i); } while (0)

ROUND4(A,B,C,D,0,6,49);
ROUND4(D,A,B,C,7,10,50);
ROUND4(C,D,A,B,14,15,51);
ROUND4(B,C,D,A,5,21,52);

ROUND4(A,B,C,D,12,6,53);
ROUND4(D,A,B,C,3,10,54);
ROUND4(C,D,A,B,10,15,55);
ROUND4(B,C,D,A,1,21,56);

ROUND4(A,B,C,D,8,6,57);
ROUND4(D,A,B,C,15,10,58);
ROUND4(C,D,A,B,6,15,59);
ROUND4(B,C,D,A,13,21,60);

ROUND4(A,B,C,D,4,6,61);
ROUND4(D,A,B,C,11,10,62);
ROUND4(C,D,A,B,2,15,63);
ROUND4(B,C,D,A,9,21,64);

ctx->A+=A;
ctx->B+=B;
ctx->C+=C;
ctx->D+=D;
}

void addbytes_context_md5(struct context_md5 *ctx, unsigned char *bytes, unsigned int len) {
ctx->bitcount64+=len*8;
if (ctx->unreadbytecount) {
	int needed;
	needed=64-ctx->unreadbytecount;
	if (len<needed) {
		memcpy(ctx->unreadbuffer+ctx->unreadbytecount,bytes,len);
		ctx->unreadbytecount+=len;
		return;
	}
	memcpy(ctx->unreadbuffer+ctx->unreadbytecount,bytes,needed);
	bytes+=needed;
	len-=needed;
	ctx->unreadbytecount=0;
	(void)addblock(ctx,ctx->unreadbuffer);
}
while (len&(~63)) { // >=64
	(void)addblock(ctx,bytes);
	bytes+=64;
	len-=64;
}
if (len&63) {
	memcpy(ctx->unreadbuffer,bytes,len);
	ctx->unreadbytecount=len;
}
}

static inline void addu64(unsigned char *dest, uint64_t u64) {
*dest=u64&0xff;dest++;u64=u64>>8;
*dest=u64&0xff;dest++;u64=u64>>8;
*dest=u64&0xff;dest++;u64=u64>>8;
*dest=u64&0xff;dest++;u64=u64>>8;
*dest=u64&0xff;dest++;u64=u64>>8;
*dest=u64&0xff;dest++;u64=u64>>8;
*dest=u64&0xff;dest++;u64=u64>>8;
*dest=u64&0xff;
}

static inline void addpadding(struct context_md5 *ctx) {
unsigned char *urb;
int ubc;
urb=ctx->unreadbuffer;
ubc=ctx->unreadbytecount;
urb[ubc]=128;
if (ubc<=55) {
	if (ubc!=55) {
		memset(urb+ubc+1,0,55-ubc); // we haven't counted the 1 byte, hence +1
	}
} else {
	memset(urb+ubc+1,0,63-ubc);
	(void)addblock(ctx,urb);
	memset(urb,0,56);
}
(void)addu64(urb+56,ctx->bitcount64);
(void)addblock(ctx,urb);
}

void finish_context_md5(unsigned char *dest, struct context_md5 *ctx) {
uint32_t u;

(void)addpadding(ctx);

u=ctx->A;
*dest=u&0xff; dest++; u=u>>8;
*dest=u&0xff; dest++; u=u>>8;
*dest=u&0xff; dest++; u=u>>8;
*dest=u&0xff; dest++;
u=ctx->B;
*dest=u&0xff; dest++; u=u>>8;
*dest=u&0xff; dest++; u=u>>8;
*dest=u&0xff; dest++; u=u>>8;
*dest=u&0xff; dest++;
u=ctx->C;
*dest=u&0xff; dest++; u=u>>8;
*dest=u&0xff; dest++; u=u>>8;
*dest=u&0xff; dest++; u=u>>8;
*dest=u&0xff; dest++;
u=ctx->D;
*dest=u&0xff; dest++; u=u>>8;
*dest=u&0xff; dest++; u=u>>8;
*dest=u&0xff; dest++; u=u>>8;
*dest=u&0xff;
}

