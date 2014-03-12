/***************************************************************************
 *   Copyright (C) 2010 by Roberto Maar                                    *
 *   robi6@users.sf.net                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>.  *
 ***************************************************************************/
// A special doubly linked ringbuffer for manage journalinode

#ifndef RING_BUF_H
#define RING_BUF_H

/* ext3/4 libraries */
#include <ext2fs/ext2fs.h>


typedef struct transaction_range{
__u32 start;
__u32 end;
} trans_range_t;


struct ring_buf_item {
	struct ring_buf_item *prev;
	struct ring_buf_item *next;
	struct ext2_inode_large *inode;
	trans_range_t transaction;
};

struct ring_buf {
	struct ring_buf_item * begin;
	int count;
	int i_size;
	__u32 nr;
	char del_flag;
	char reuse_flag;
};

typedef struct ring_buf_item r_item;


static inline r_item* r_first(struct ring_buf *head){
	return (head->count) ? head->begin : NULL ;}

static inline r_item* r_last(struct ring_buf *head){
	return (head->count) ? head->begin->prev : NULL ; }

static inline r_item* r_next(r_item *item){
	return item->next;}

static inline r_item* r_prev(r_item *item){ 
	return item->prev;}

static inline int r_get_count(struct ring_buf *head){
	return head->count;}

 
r_item* r_item_new( int );
struct ring_buf* ring_new( int , __u32);
r_item* r_item_add(struct ring_buf*);
void ring_del(struct ring_buf*);
r_item* r_begin(struct ring_buf*);
#endif
