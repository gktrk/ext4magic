/***************************************************************************
 *   Copyright (C) 2010 by Roberto Maar                                    *
 *   robi@users.berlios.de                                                 *
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
 *                                                                         *
 *                                                                         *
 *   C Implementation: magic_block_scan                                    * 
 ***************************************************************************/
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "util.h"

#include "magic.h"

extern ext2_filsys     current_fs ;



ext2fs_block_bitmap 	  	d_bmap = NULL ; 


void magic_block_scan(char* des_dir, __u32 t_after){
	int					 	retval;
	blk_t  						blk, o_blk;
	int 						ret, load_db;
	struct 	ext2fs_struct_loc_generic_bitmap 	*ds_bmap;
	char 						*block_buf = NULL;
	magic_t 					cookie;
	int						m_result;

	cookie = magic_open(MAGIC_NO_CHECK_COMPRESS | MAGIC_NO_CHECK_ELF | MAGIC_NO_CHECK_TAR); 
	if ((cookie) && (! magic_load(cookie, NULL))){

		retval = init_block_bitmap_list(&d_bmap, t_after);
		while (retval){
			retval = next_block_bitmap(d_bmap);
			if (retval == 2 ) 
				continue;

			if (d_bmap && retval ){

				ds_bmap = (struct ext2fs_struct_loc_generic_bitmap *) d_bmap;
				block_buf = malloc(current_fs->blocksize);
				if (block_buf){
					for (blk = ds_bmap->start; blk <ds_bmap->real_end; blk++){
						if (! (blk & 0x7)){
							o_blk = blk >> 3;
							while ((! *(ds_bmap->bitmap + o_blk)) && (o_blk <= (ds_bmap->real_end >> 3)))  
								o_blk ++;
							blk = o_blk << 3;
						}
						if (blk >= ds_bmap->real_end)
							break;
//FIXME in work								
						if (ext2fs_test_block_bitmap ( d_bmap, blk) && (! ext2fs_test_block_bitmap( bmap, blk))){
							printf(" read block : %u\n",blk);	
							ret = io_channel_read_blk ( current_fs->io,  blk,  1,  block_buf );
							if (ret){
								fprintf(stderr,"ERROR: while read block %10u\n",blk);
							}
							m_result = magic_check_block(block_buf,cookie,current_fs->blocksize,blk);
						}

					}
					free(block_buf);
				}
				else
					printf("can't allocate memory for blocksearch\n");
			}
			clear_block_bitmap_list(d_bmap);
		}
		if(cookie) magic_close(cookie);
	}
//FIXME only if needed
if(d_bmap)
	ext2fs_free_block_bitmap(d_bmap);

}
