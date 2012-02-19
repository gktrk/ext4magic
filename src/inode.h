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
#ifndef INODE_H
#define INODE_H


#include <ext2fs/ext2fs.h>
#include "util.h"
#include "ext4magic.h"
#include "journal.h"
#include "ring_buf.h" 
#include "extent_db.h"

#define DUMP_LEAF_EXTENTS       0x01
#define DUMP_NODE_EXTENTS       0x02
#define DUMP_EXTENT_TABLE       0x04


//buffersize für magic_block_scan3
#define MAX_RANGE 16


//help struct for list inode data blocks
struct list_blocks_struct {
        FILE            *f;
        e2_blkcnt_t     total;
        blk_t           first_block, last_block;
        e2_blkcnt_t     first_bcnt, last_bcnt;
        e2_blkcnt_t     first;
};


/*
struct extent_area {
	blk_t		blocknr;
	__u16		depth;
	blk_t		l_start;
	blk_t		l_end;
	blk_t		start_b;
	blk_t		end_b;
};
*/


//functions for external use
int intern_read_inode_full(ext2_ino_t, struct ext2_inode*, int);// read real fs inode 128++
int intern_read_inode(ext2_ino_t, struct ext2_inode*);// read real fs inode 128
r_item* get_undel_inode(struct ring_buf*, __u32, __u32);// return the last undelete inode in journal after ->  <-before
r_item* get_last_undel_inode(struct ring_buf* );// return the last undeleted inode in journal
blk_t  get_inode_pos(struct ext2_super_block* , struct inode_pos_struct*, ext2_ino_t, int);//calculate the position of inode in FS
void print_j_inode(struct ext2_inode_large* , ext2_ino_t , __u32, int );//function for dump_inode_list
int get_transaction_inode(ext2_ino_t, __u32, struct ext2_inode_large*);// get journalinode from transactionnumber 
void dump_inode_list(struct ring_buf* , int);//print the contents of all copy of inode in the journal
void dump_inode(FILE*, const char*, ext2_ino_t, struct ext2_inode*,int);//print the contents of inode
int read_journal_inode( ext2_ino_t, struct ext2_inode*, __u32);// get the first Journal Inode by transaction
int read_time_match_inode( ext2_ino_t, struct ext2_inode*, __u32);// get the first Journal Inode by time_stamp	
struct ring_buf* get_j_inode_list(struct ext2_super_block*, ext2_ino_t);//fill all inode found in the Journal in the inode-ringbuffer

//functions for the magic scanner
struct ext2_inode_large* new_inode(); //create a new inode
int inode_add_block(struct ext2_inode_large* , blk_t); //add a block to inode
int inode_add_meta_block(struct ext2_inode_large*, blk_t, blk_t*, blk_t*,char* ); //add the ext3  indirect Blocks to the inode

//functions in develop
int inode_add_extent(struct ext2_inode_large*, struct extent_area*, __u32*, int );
//blk_t get_last_block_ext4(struct ext2_inode_large*); //search the last data block ext4-inode

#endif
