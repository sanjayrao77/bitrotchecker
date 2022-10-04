/*
 * dirbyname.h
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
#define _FILE_OFFSET_BITS	64
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <dirent.h>
#include <sys/stat.h>
#include <inttypes.h>
#include "common/conventions.h"
#include "common/blockmem.h"

#include "bitrot.h"
#include "dirbyname.h"

#define node_treeskel dir_bitrot
#define find2_treeskel find2_dirbyname
#define addnode2_treeskel addnode2_dirbyname
#define LEFT(a)	((a)->treevars.left)
#define RIGHT(a)	((a)->treevars.right)
#define BALANCE(a)	((a)->treevars.balance)
#define cmp dirbynamecmp

struct dir_bitrot *filename_find2_dirbyname(struct dir_bitrot *root, char *name) {
struct dir_bitrot match;
match.name=name;
return find2_dirbyname(root,&match);
}


static int dirbynamecmp(struct dir_bitrot *a, struct dir_bitrot *b) {
return strcmp(a->name,b->name);
}

#line 1 "dirbyname.c/common/treeskel.c"
#include "common/treeskel.c"
