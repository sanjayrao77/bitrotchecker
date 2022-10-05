/*
 * common/treeskel.c - btree skeleton code
 * Copyright (C) 2021 Sanjay Rao
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
#ifndef node_treeskel
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#if 0
#define DEBUG
#endif
#include "conventions.h"

#include "treeskel.h"

NEWFUNC(node_treeskel);
CLEARFUNC(node_treeskel);

#define find_treeskel find_treeskel
#define rmnode_treeskel rmnode_treeskel
#define findmaxdepth_treeskel findmaxdepth_treeskel
#define countnodes_treeskel countnodes_treeskel
#define verifynodes_treeskel verifynodes_treeskel
#define printnodes_treeskel printnodes_treeskel
#define LEFT(a)	((a)->left)
#define RIGHT(a)	((a)->right)
#define BALANCE(a)	((a)->balance)
#endif

#ifndef CMPRET
#define CMPRET	int
#endif

#ifndef CMPPARAMS
#define CMPPARAMS struct node_treeskel *a, struct node_treeskel *b
#define CMPPTR(a) (a)
#endif

static void rebalanceleftleft(struct node_treeskel **root_inout) {
struct node_treeskel *a=*root_inout;
struct node_treeskel *left=LEFT(a);
LEFT(a)=RIGHT(left);
RIGHT(left)=a;
*root_inout=left;
BALANCE(left)=0;
BALANCE(a)=0;
}

static void rebalancerightright(struct node_treeskel **root_inout) {
struct node_treeskel *a=*root_inout;
struct node_treeskel *right=RIGHT(a);
RIGHT(a)=LEFT(right);
LEFT(right)=a;
*root_inout=right;
BALANCE(right)=0;
BALANCE(a)=0;
}

static void rebalanceleftright(struct node_treeskel **root_inout) {
struct node_treeskel *a=*root_inout;
struct node_treeskel *left=LEFT(a);
struct node_treeskel *gchild=RIGHT(left);
int b;
RIGHT(left)=LEFT(gchild);
LEFT(gchild)=left;
LEFT(a)=RIGHT(gchild);
RIGHT(gchild)=a;
*root_inout=gchild;
b=BALANCE(gchild);
#if 1
BALANCE(a)=-(b>0);
BALANCE(left)=(b<0);
#else
if (b>0) {
		BALANCE(a)=-1;
		BALANCE(left)=0;
} else if (b<0) {
		BALANCE(a)=0;
		BALANCE(left)=1;
} else { // doesn't happen by insertion, can happen by deletion
		BALANCE(a)=BALANCE(left)=0;
}
#endif
BALANCE(gchild)=0;
}

static void rebalancerightleft(struct node_treeskel **root_inout) {
struct node_treeskel *a=*root_inout;
struct node_treeskel *right=RIGHT(a);
struct node_treeskel *gchild=LEFT(right);
int b;
LEFT(right)=RIGHT(gchild);
RIGHT(gchild)=right;
RIGHT(a)=LEFT(gchild);
LEFT(gchild)=a;
*root_inout=gchild;
b=BALANCE(gchild);
#if 1
BALANCE(a)=(b<0);
BALANCE(right)=-(b>0);
#else
if (b<0) {
		BALANCE(a)=1;
		BALANCE(right)=0;
} else if (b>0) {
		BALANCE(a)=0;
		BALANCE(right)=-1;
} else { // doesn't happen by insertion, can happen by deletion
		BALANCE(a)=BALANCE(right)=0;
}
#endif
BALANCE(gchild)=0;
}

#if defined rmnode_treeskel || defined rmnode2_treeskel
static void rebalanceleftbalance(struct node_treeskel **root_inout) {
struct node_treeskel *a=*root_inout;
struct node_treeskel *left=LEFT(a);
LEFT(a)=RIGHT(left);
RIGHT(left)=a;
*root_inout=left;
BALANCE(left)=-1;
/* redundant */
// BALANCE(a)=1;
}

static void rebalancerightbalance(struct node_treeskel **root_inout) {
struct node_treeskel *a=*root_inout;
struct node_treeskel *right=RIGHT(a);
RIGHT(a)=LEFT(right);
LEFT(right)=a;
*root_inout=right;
BALANCE(right)=1;
/* redundant */
// BALANCE(a)=-1;
}

static int extracthighest(struct node_treeskel **root_inout, struct node_treeskel **node_out) {
/* returns 1 if depth decreased, else 0 */
struct node_treeskel *root=*root_inout;

if (RIGHT(root)) {
	int r=0;
	if (extracthighest(&RIGHT(root),node_out)) {
		int b;
		b=BALANCE(root);
		if (b<0) {
			BALANCE(root)=0; r=1;
		} else if (!b) {
			BALANCE(root)=1;
		} else {
				if (!BALANCE(LEFT(root))) {
					(void)rebalanceleftbalance(root_inout); 
				} else if (BALANCE(LEFT(root))==1) {
					(void)rebalanceleftleft(root_inout); 
					r=1;
				} else {
					(void)rebalanceleftright(root_inout);
					r=1;
				}
		}
	}
	return r;
} else {
	/* remove ourselves */
	*node_out=root;
	*root_inout=LEFT(root);		
	return 1;
}
}

static int extractlowest(struct node_treeskel **root_inout, struct node_treeskel **node_out) {
/* returns 1 if depth decreased, else 0 */
struct node_treeskel *root=*root_inout;

if (LEFT(root)) {
	int r=0;
	if (extractlowest(&LEFT(root),node_out)) {
		int b;
		b=BALANCE(root);
		if (b>0) {
			BALANCE(root)=0; r=1;
		} else if (!b) {
			BALANCE(root)=-1;
		} else {
				if (!BALANCE(RIGHT(root))) {
					(void)rebalancerightbalance(root_inout);
				} else if (BALANCE(RIGHT((root)))==-1) {
					(void)rebalancerightright(root_inout);
					r=1;
				} else {
					(void)rebalancerightleft(root_inout);
					r=1;
				}
		}
	}
	return r;
} else {
	/* remove ourselves */
	*node_out=root;
	*root_inout=RIGHT(root);
	return 1;
}
}


static int rmnode(struct node_treeskel **root_inout, struct node_treeskel *node, struct node_treeskel **node_out,
	CMPRET (*cmp)(CMPPARAMS)) {
/* returns 1 if depth decreased, else 0 */
struct node_treeskel *root=*root_inout;
int r=0;
CMPRET c;

if (!root) { D2WHEREAMI; return 0; } /* if the node doesn't exist in the tree */

c=cmp(node,root);
if (c<0) {
	if (rmnode(&LEFT(root),node,node_out,cmp)) {
		int b;
		b=BALANCE(root);
		if (b>0) {
				BALANCE(root)=0;
				r=1;
		} else if (!b) {
				BALANCE(root)=-1;
		} else {
				if (!BALANCE(RIGHT(root))) {
					(void)rebalancerightbalance(root_inout);
				} else if (BALANCE(RIGHT(root))<0) {
					(void)rebalancerightright(root_inout);
					r=1;
				} else {
					(void)rebalancerightleft(root_inout);
					r=1;
				}
		}
	}
} else if (c>0) {
	if (rmnode(&RIGHT(root),node,node_out,cmp)) {
		int b;
		b=BALANCE(root);
		if (b<0) {
				BALANCE(root)=0;
				r=1;
		} else if (!b) {
				BALANCE(root)=1;
		} else {
				if (!BALANCE(LEFT(root))) {
					(void)rebalanceleftbalance(root_inout);
				} else if (BALANCE(LEFT(root))>0) {
					(void)rebalanceleftleft(root_inout);
					r=1;
				} else {
					(void)rebalanceleftright(root_inout);
					r=1;
				}
		}
	}
} else {
	/* found it */
	struct node_treeskel *temp;
	*node_out=root;
	if (BALANCE(root)==1) {
		if (extracthighest(&LEFT(root),&temp)) {
			BALANCE(temp)=0;
			r=1;
		} else {
			BALANCE(temp)=1;
		}
		LEFT(temp)=LEFT(root);
		RIGHT(temp)=RIGHT(root);
		*root_inout=temp;
	} else if (BALANCE(root)==-1) {
		if (extractlowest(&RIGHT(root),&temp)) {
			BALANCE(temp)=0;
			r=1;
		} else {
			BALANCE(temp)=-1;
		}
		LEFT(temp)=LEFT(root);
		RIGHT(temp)=RIGHT(root);
		*root_inout=temp;
	} else { /* balance 0 */
		if (LEFT(root)) {
			if (extracthighest(&LEFT(root),&temp)) {
				BALANCE(temp)=-1;
			} else {
				BALANCE(temp)=0;
			}
			LEFT(temp)=LEFT(root);
			RIGHT(temp)=RIGHT(root);
			*root_inout=temp;
		} else {
			*root_inout=NULL;
			r=1;
		}
	}
}

return r;
}
#ifdef cmp
void rmnode2_treeskel(struct node_treeskel **root_inout, struct node_treeskel *node, struct node_treeskel **found_out) {
/* found_out can be NULL if node is the true pointer */
/* if found_out is NULL and node is _NOT_ the true pointer, then the true pointer will be lost. Don't do that */
struct node_treeskel *found=NULL;
(ignore)rmnode(root_inout,node,&found,cmp);
if (found_out) *found_out=found;
}
#else
void rmnode_treeskel(struct node_treeskel **root_inout, struct node_treeskel *node, struct node_treeskel **found_out,
	CMPRET (*cmp)(CMPPARAMS)) {
/* found_out can be NULL if node is the true pointer */
/* if found_out is NULL and node is _NOT_ the true pointer, then the true pointer will be lost. Don't do that */
struct node_treeskel *found=NULL;
(ignore)rmnode(root_inout,node,&found,cmp);
if (found_out) *found_out=found;
}
#endif
#endif

static int addnode(struct node_treeskel **root_inout, struct node_treeskel *node, CMPRET (*cmp)(CMPPARAMS)) {
/* returns 1 if depth increased, else 0 */
struct node_treeskel *root=*root_inout;
int r=0;

if (!root) {
	*root_inout=node;
	return 1;
}

if (cmp(CMPPTR(node),CMPPTR(root))<0) {
	if (addnode(&LEFT(root),node,cmp)) {
		int b;
		b=BALANCE(root);
		if (!b) {
			BALANCE(root)=1; r=1;
		} else if (b>0) {
				if (BALANCE(LEFT(root))>0) (void)rebalanceleftleft(root_inout); else (void)rebalanceleftright(root_inout);
		} else {
			BALANCE(root)=0;
		}
	}
} else {
	if (addnode(&RIGHT(root),node,cmp)) {
		int b;
		b=BALANCE(root);
		if (!b) {
			BALANCE(root)=-1; r=1;
		} else if (b>0) {
			BALANCE(root)=0;
		} else {
				if (BALANCE(RIGHT(root))<0) (void)rebalancerightright(root_inout); else (void)rebalancerightleft(root_inout);
		}
	}
}

return r;
}

#ifdef cmp
void addnode2_treeskel(struct node_treeskel **root_inout, struct node_treeskel *node) {
/* node should be 0'd out already (except for data) */
(ignore)addnode(root_inout,node,cmp);
}
#else
void addnode_treeskel(struct node_treeskel **root_inout, struct node_treeskel *node,
	CMPRET (*cmp)(CMPPARAMS)) {
/* node should be 0'd out already (except for data) */
(ignore)addnode(root_inout,node,cmp);
}
#endif

#ifdef findmaxdepth_treeskel
static void findmaxdepth(struct node_treeskel *root, unsigned int *maxdepth_inout, unsigned int depth) {
depth++;
if (depth>*maxdepth_inout) *maxdepth_inout=depth;
if (LEFT(root)) findmaxdepth(LEFT(root),maxdepth_inout,depth);
if (RIGHT(root)) findmaxdepth(RIGHT(root),maxdepth_inout,depth);
}

unsigned int findmaxdepth_treeskel(struct node_treeskel *root) {
unsigned int maxdepth=0;
if (!root) return 0;
(void)findmaxdepth(root,&maxdepth,0);
return maxdepth;
}
#endif

#ifdef countnodes_treeskel
static void countnodes(struct node_treeskel *root, unsigned int *count_inout) {
*count_inout+=1;
if (LEFT(root)) countnodes(LEFT(root),count_inout);
if (RIGHT(root)) countnodes(RIGHT(root),count_inout);
}
unsigned int countnodes_treeskel(struct node_treeskel *root) {
unsigned int ui=0;
if (!root) return 0;
(void)countnodes(root,&ui);
return ui;
}
#endif

#ifdef verifynodes_treeskel
static int verifybalance(struct node_treeskel *root) {
unsigned int left=0,right=0;
int b;
if (LEFT(root)) {
	if (verifybalance(LEFT(root))) GOTOERROR;
	left=findmaxdepth_treeskel(LEFT(root));
}
if (RIGHT(root)) {
	if (verifybalance(RIGHT(root))) GOTOERROR;
	right=findmaxdepth_treeskel(RIGHT(root));
}
b=BALANCE(root);
if (b>0) {
		if (left!=right+1) GOTOERROR;
} else if (!b) {
		if (left!=right) GOTOERROR;
} else {
		if (right!=left+1) GOTOERROR;
}
return 0;
error:
	return -1;
}
int verifynodes_treeskel(struct node_treeskel *root, unsigned int size) {
if (!root) return 0;
if (size) {
	if (countnodes_treeskel(root)!=size) GOTOERROR;
}
if (verifybalance(root)) GOTOERROR;
return 0;
error:
	return -1;
} 
#endif

#ifdef printnodes_treeskel
#if 0
void printnodes_treeskel(struct node_treeskel *root) {
if (LEFT(root) && RIGHT(root)) 
	fprintf(stderr,"%u <-- %u --> %u (%d)\n",
		LEFT(root)->data.ui,
		root->data.ui,
		RIGHT(root)->data.ui,
		BALANCE(root));
else if (LEFT(root))
	fprintf(stderr,"%u <-- %u (%d)\n",
		LEFT(root)->data.ui,
		root->data.ui,
		BALANCE(root));
else if (RIGHT(root))
	fprintf(stderr,"%u --> %u (%d)\n",
		root->data.ui,
		RIGHT(root)->data.ui,
		BALANCE(root));
else
	fprintf(stderr,"%u (%d)\n",
		root->data.ui,
		BALANCE(root));
if (LEFT(root)) printnodes_treeskel(LEFT(root));
//fprintf(stdout,"Data: %u\n",root->data.ui);
if (RIGHT(root)) printnodes_treeskel(RIGHT(root));
}
#endif
static void printnodes(struct node_treeskel *root, unsigned int indent) {
unsigned int plus;
plus=indent+1;
if (RIGHT(root)) printnodes(RIGHT(root),plus);
while (indent) {
	fputs("    ",stderr);
	indent--;
}
#ifdef voteondisktree_print
fprintf(stderr,"%u.%u: %u (%d)\n",
	root->record.uidtype,
	root->record.uid,
	root->record.vote,
	BALANCE(root));
#else
fprintf(stderr,"%p (%d)\n",root,BALANCE(root));
#endif
if (LEFT(root)) printnodes(LEFT(root),plus);
}
void printnodes_treeskel(struct node_treeskel *root) {
if (!root) return;
(void)printnodes(root,0);
}
#endif


#if defined find_treeskel || defined find2_treeskel
#ifdef cmp
struct node_treeskel *find2_treeskel(struct node_treeskel *root, struct node_treeskel *node) {
CMPRET b;
while (root) {
	b=cmp(node,root);
	if (b<0) {
		root=LEFT(root);
	} else if (!b) {
		return root;
	} else {
		root=RIGHT(root);
	}
}
return NULL;
}
#else
struct node_treeskel *find_treeskel(struct node_treeskel *root, struct node_treeskel *node,
	CMPRET (*cmp)(CMPPARAMS)) {
CMPRET b;
while (root) {
	b=cmp(CMPPTR(node),CMPPTR(root));
	if (b<0) {
		root=LEFT(root);
	} else if (!b) {
		return root;
	} else {
		root=RIGHT(root);
	}
}
return NULL;
}
#endif
#endif

#if defined forall_treeskel || defined forall2_treeskel
static void forall2_treeskel(struct node_treeskel *node, void(*f)(struct node_treeskel *a, void *b), void *ptr) {
struct node_treeskel *left,*right;
left=LEFT(node);
right=RIGHT(node);
if (left) forall2_treeskel(left,f,ptr);
f(node,ptr);
if (right) forall2_treeskel(right,f,ptr);
}
#endif
#ifdef forall_treeskel
static void forall_treeskel(struct node_treeskel *root, void(*f)(struct node_treeskel *a, void *b), void *ptr) {
if (root) forall2_treeskel(root,f,ptr);
}
#endif
