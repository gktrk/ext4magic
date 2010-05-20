/***************************************************************************
 *   Copyright (C) 2010 by Roberto Maar   *
 *   robi@users.berlios.de   *
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
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

// A special doubly linked ringbuffer for manage journalinode

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include "ring_buf.h"


r_item* r_item_new(int i_size){
	r_item* this = NULL;
	
	this = (r_item*) malloc(sizeof(r_item)) ;
	if (! this) return NULL;
	this->prev = this;
	this->next = this;
	this->transaction.start = 0;
	this->transaction.end = 0;
	this->inode = (struct ext2_inode_large*) malloc(i_size);
	if (! this->inode ) {
		free(this);
		return NULL;
	}
return this;
};

struct ring_buf* ring_new(int i_size, __u32 nr){
	struct ring_buf *head;
	head = (struct ring_buf*) malloc(sizeof(struct ring_buf));
	if (! head ) return NULL;
	head->count = 0;
	head->del_flag = 0;
	head->reuse_flag = 0;
	head->i_size = i_size;
	head->nr = nr;
return head;	
};

r_item* r_item_add(struct ring_buf *head){
	r_item* item = r_item_new(head->i_size);
	if (item) {
		if (! head->count) { head->begin = item;} 
		else {
		item->next = head->begin;
		item->prev = head->begin->prev;
		head->begin->prev->next = item;
		head->begin->prev = item;
		}
		head->count++;
//		item->transaction = *trans;
	}
	return item;
};

void ring_del(struct ring_buf *head){
	r_item* item;
	r_item* old;
	old = NULL;
	if (head->count){
		item = head->begin->next;
		head->begin->next = NULL;
		while (item){
			if (item->inode) free(item->inode);
			old = item;
			item = item->next;
			free(old);
		}
	}
	free(head);
	head = NULL;
};  

r_item* r_begin(struct ring_buf *head){
	if (head->count > 1)
	while (head->begin->inode->i_ctime  >  head->begin->prev->inode->i_ctime)
		head->begin = head->begin->prev ;
	return head->begin;
}



