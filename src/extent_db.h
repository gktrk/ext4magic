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
// Extent_db.h contains the header for it.

#ifndef EXTENT_DB_H
#define EXTENT_DB_H

/* ext3/4 libraries */
#include <ext2fs/ext2fs.h>

struct extent_area {
	blk_t		blocknr;
	__u16		depth;
	blk_t		l_start;
	blk_t		l_end;
	blk_t		start_b;
	blk_t		end_b;
	__u16		len;
	__u64		size;
	__u32		b_count;
};



struct extent_db_item {
	struct extent_db_item		*next;
	struct extent_area		*entry;
	struct extent_db_item		*last;
	
};


struct extent_db_t {
	struct extent_db_item		*next;
	__u32				count;
	__u16				max_depth;
	ext2fs_block_bitmap		d_bmap;
	
};
	
	

struct extent_area* new_extent_area();
struct extent_db_t* extent_db_init(ext2fs_block_bitmap);
int extent_db_add (struct extent_db_t* db, struct extent_area*, int);
int extent_db_del(struct extent_db_t* ,blk_t);
void extent_db_clear(struct extent_db_t*);
__u32 extentd_db_find( struct extent_db_t*, __u32, struct extent_area*);




#endif