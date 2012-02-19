/***************************************************************************
 *   Copyright (C) 2011 by Roberto Maar  				   *
 *   robi@users.berlios.de  						   *
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
// The Magic functions for ext4 need a small stack to the cache found extents
// The extent_db.c functions provide the interface for this.

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include "extent_db.h"

extern ext2_filsys	current_fs;

static int mark_extent_len(struct extent_db_t*, blk_t, void*);


struct extent_area* new_extent_area(){
	struct extent_area *ea;
	ea = malloc(sizeof(struct extent_area));
	if (ea)
		memset(ea, 0, sizeof(struct extent_area));
return (ea) ? ea : NULL;
}


static struct extent_db_item* new_item(struct extent_db_item *last){
	struct extent_db_item 	*this = NULL;
	this = malloc(sizeof(struct extent_db_item));
	if (this){
		this->last = last;
		this->entry = NULL;
		this->next = NULL;
		return this;
	}
return NULL;
}



struct extent_db_t* extent_db_init(ext2fs_block_bitmap	d_bmap){
	struct extent_db_t*  db = NULL;

	db = malloc(sizeof(struct extent_db_t));
	if (db){
		db->count = 0;
		db->max_depth = 0;
		db->d_bmap = d_bmap;
		db->next = new_item((struct extent_db_item*)db);
	}
return db;
} 


int extent_db_add (struct extent_db_t* db, struct extent_area *ea, int flag){
	struct extent_db_item	*pointer;

	pointer = db->next;
	while (pointer->entry)
		pointer = pointer->next;

//FIXME
	if ((pointer->last != (struct extent_db_item*)db) && (pointer->last->entry) && (pointer->last->entry->end_b == ea->start_b -1) && (ea->l_start)){
		pointer->last->entry->len += ea->len;
		pointer->last->entry->end_b += ea->len;
		pointer->last->entry->size += (unsigned long long)ea->len * current_fs->blocksize;
		free (ea);
		return db->count;	
	}

	pointer->next = new_item(pointer);
	if (pointer->next){
		pointer->entry = ea;
		if (db->max_depth < ea->depth) db->max_depth = ea->depth;
		(db->count)++;
		if (flag) mark_extent_len(db, ea->blocknr, NULL );
		else {
			ea->end_b = ea->start_b + ea->len -1;
			ea->size = (unsigned long long)ea->len * current_fs->blocksize;
		}
		return db->count;
	}
return 0;
}

__u32 extentd_db_find( struct extent_db_t*db ,__u32 start, struct extent_area *ea) {
	struct extent_db_item	*pointer;
	__u32			end = 0;
	__u16			depth;

	depth = db->max_depth+1;

	while ((depth) && (!end)){
		pointer = db->next;
		while ((pointer->entry) && ((pointer->entry->depth != depth - 1) || (pointer->entry->l_start != start)))
			pointer = pointer->next;
	
		if (pointer->entry){
			memcpy(ea,pointer->entry,sizeof(struct extent_area));
			end = pointer->entry->l_end;
		}
		else{
			if ((depth - 1) && (!start))
				(db->max_depth)--;
			depth--;
		}
	} //depth
return end;
}



int extent_db_del(struct extent_db_t* db,blk_t blk){
	struct extent_db_item	*pointer;
	int ret = 0;

	pointer = db->next;
	while ((pointer->entry) && (pointer->entry->blocknr < blk))
		pointer = pointer->next;
	if ((pointer->entry) && (pointer->entry->blocknr == blk)){
		(pointer->last)->next = pointer->next;
		pointer->next->last = pointer->last;
		if(pointer->entry) free(pointer->entry);
		if (pointer) free(pointer);
		(db->count)--;
//		printf("Extend DB : entry %lu deleted\n",blk);
		ret = 1;
	}
return ret;
}



static void clear_entry(struct extent_db_item *p){
	if (p->next)
		clear_entry(p->next);
	if (p->entry) {
//		printf("EXT-DB : %10u  %d  %10lu  %10lu  %10lu\n",p->entry->blocknr, (p->entry->l_start)?1:0, p->entry->start_b, p->entry->end_b, p->entry->len);
		free(p->entry);
	}
	if (p) free(p);
return;
}


void extent_db_clear(struct extent_db_t* db){
//	printf("Extent DB has %lu Entries ; clean now\n",db->count);
	clear_entry(db->next);
	if(db) free(db);
return;
}


static int mark_extent_len(struct extent_db_t* db, blk_t blk, void * buf){
	void				*buf_tmp = NULL;
	int				bufset = 0;
	int 				ret = 0;
	struct ext3_extent_idx		*idx;
	struct ext3_extent 		*extent;
	struct ext3_extent_header 	*header;
	int				entry,i;
	
	if (!buf){	
		buf = malloc(current_fs->blocksize);
		io_channel_read_blk (current_fs->io, blk, 1, buf );
		bufset=1;
	}
	buf_tmp = malloc(current_fs->blocksize);
	if ((!buf_tmp) || (!buf))
		return 1;

	header = buf;
	ret = ext2fs_extent_header_verify((void*) header, current_fs->blocksize);
	ext2fs_unmark_generic_bitmap(db->d_bmap, blk);
	if(!ret){
		for (entry =1; ((!ret) && (entry <= ext2fs_le16_to_cpu(header->eh_entries))) ; entry++){
 			if (ext2fs_le16_to_cpu(header->eh_depth)){
				idx = (struct ext3_extent_idx*) (header + entry);
				if(io_channel_read_blk ( current_fs->io,ext2fs_le32_to_cpu(idx->ei_leaf), 1, buf_tmp )){
					fprintf(stderr,"Error read block %lu\n", (long unsigned int)ext2fs_le32_to_cpu(idx->ei_leaf));
					ret = 2;
				}
				else{
					//Recursion
					if (! extent_db_del(db,ext2fs_le32_to_cpu(idx->ei_leaf)))
						ret = mark_extent_len(db, ext2fs_le32_to_cpu(idx->ei_leaf),buf_tmp);
				}
			}
			else{
				extent  = (struct ext3_extent*)(header + entry);
				for (i = 0; i< ext2fs_le16_to_cpu(extent->ee_len); i++){
					ext2fs_unmark_generic_bitmap(db->d_bmap, ext2fs_le32_to_cpu(extent->ee_start) + i );
				}
			}
		}
	}
	if (buf_tmp) free(buf_tmp);
	if (bufset && buf) free(buf);
return ret;
}


