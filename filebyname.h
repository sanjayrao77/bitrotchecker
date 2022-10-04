/*
 * filebyname.h
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
void addnode2_filebyname(struct file_bitrot **root_inout, struct file_bitrot *node);
struct file_bitrot *find2_filebyname(struct file_bitrot *root, struct file_bitrot *match);
struct file_bitrot *filename_find2_filebyname(struct file_bitrot *root, char *filename);
unsigned int findmaxdepth_filebyname(struct file_bitrot *root);
