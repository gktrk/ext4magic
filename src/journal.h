/***************************************************************************
 *   Copyright (C) 2010 by Roberto Maar   *
 *   robi@users.berlios.de  *
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
#ifndef JOURNAL_H
#define JOURNAL_H


//struct for cache the descriptor data
typedef struct journal_descriptor_tag_s
{
	blk64_t		f_blocknr;	/* The on-disk block number */
	__u32		j_blocknr;	/* The on-journal block number */
	__u32		transaction; 	/* Transaction Sequence*/
} journal_descriptor_tag_t;


//struct for cache the block bitmap data
typedef struct journal_bitmap_tag_s
{
	blk64_t		blockgroup;	/* The blockgroup of bitmap */
	__u32		j_blocknr;	/* The on-journal block number */
	__u32		transaction;	/* Transaction Sequence*/
} journal_bitmap_tag_t;



//head struct for collect the Block Bitmaps of Journal
struct j_bitmap_list_t
{
	int 			count;
	journal_bitmap_tag_t 	*list;
	__u32			groups;
	__u32			first_trans;
	__u32			last_trans;
	journal_bitmap_tag_t 	*pointer;	
	unsigned int 		blocksize ;
	unsigned int 		blocklen ;
	unsigned int 		last_blocklen ;
	char*			block_buf;
		
};


void dump_journal_superblock( void); //print journal superblock
extern int journal_open(char* , int );// open an extract the blocklist from journal 
extern int journal_close(void); // close the journal (last function in main() if the journal open)
int init_journal(void); // main for extract the journal to the local private data
const char *type_to_name(int);
int get_block_list_count(blk64_t);//get count of journal blocklist 
__u32 get_trans_time( __u32); //get the transactiontime of a transactionnumber
int get_block_list(journal_descriptor_tag_t*, blk64_t, int);//get a sortet list of all copys of a filesystemblock
int read_journal_block(off_t, char*, int, unsigned int*);
void print_block_list(int);
int dump_journal_block( __u32 , int );
void print_block_transaction(blk64_t,int);//print all transactions for a fs-block
__u32 get_journal_blocknr(blk64_t, __u32);// get the last dir-block for transaction from journal or if not found the real block

int get_last_block(char*,  blk64_t*, __u32, __u32);// get the last dir-block for transaction from journal or if not, found the real block
int get_block_bitmap_list( journal_bitmap_tag_t**);//get a list of all copies of blockbitmap from journal
int init_block_bitmap_list(ext2fs_block_bitmap* , __u32); //create and init the the journal block bitmap
void clear_block_bitmap_list(ext2fs_block_bitmap); //destroy the journal block bitmap
int next_block_bitmap(ext2fs_block_bitmap); //produces a differential block bitmap for a transaction from the Journal
int get_pool_block(char*); 
#endif
