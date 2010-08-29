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
 * C Implementation: imap_search                                           *
 ***************************************************************************/

//header  util.h

#include "util.h"
#include "inode.h"
#include <sys/stat.h> 
#include <errno.h>

extern ext2_filsys current_fs;



// search inode by use imap (step1: flag 1 = only directory ; step2: flag 0 = only file)
static void search_imap_inode(char* des_dir, __u32 t_after, __u32 t_before, int flag)
{
struct ext2_group_desc 			*gdp;
struct 	ext2_inode_large 		*inode;
struct dir_list_head_t 			*dir = NULL;
struct ring_buf* 			i_list = NULL;
r_item*					item = NULL;
int  					zero_flag, retval, load, x ,i ;
char 					*pathname = NULL;
char 					*buf= NULL;
__u32 					blocksize, inodesize, inode_max, inode_per_group, block_count;
__u16 					inode_per_block , inode_block_group, group;
blk_t 					block_nr;
__u32   				c_time, d_time, mode;
ext2_ino_t 				first_block_inode_nr , inode_nr;


pathname = malloc(26);
blocksize = current_fs->blocksize;
inodesize = current_fs->super->s_inode_size;
inode_max = current_fs->super->s_inodes_count;
inode_per_group = current_fs->super->s_inodes_per_group;
buf = malloc(blocksize);

inode_per_block = blocksize / inodesize;
inode_block_group = inode_per_group / inode_per_block;

for (group = 0 ; group < current_fs->group_desc_count ; group++){
	gdp = &current_fs->group_desc[group];
	zero_flag = 0;

	// NEXT GROUP IF INODE NOT INIT
	if (gdp->bg_flags & (EXT2_BG_INODE_UNINIT)) continue;

	// SET ZERO-FLAG IF FREE INODES == INODE/GROUP for fast ext3 
	if (gdp->bg_free_inodes_count == inode_per_group) zero_flag = 1;

//FIXME for struct ext4_group_desc 48/64BIT	
	for (block_nr = gdp->bg_inode_table , block_count = 0 ;
			 block_nr < (gdp->bg_inode_table + inode_block_group); block_nr++, block_count++) {

		// break if the first block only zero inode
		if ((block_count ==1) && (zero_flag == (inode_per_block + 1))) break;
//FIXME  inode_max ????	
		first_block_inode_nr = (group * inode_per_group) + (block_count * inode_per_block) + 1;
		load = 0;
		for (i = 0; i<inode_per_block;i++){
			if ( ! ext2fs_test_block_bitmap(imap,first_block_inode_nr + i)){
				load++;
				break;
			}
		}

		if (load){		 
			retval = read_block ( current_fs , &block_nr , buf);
			if (retval) return;

			for (inode_nr = first_block_inode_nr ,x = 0; x < inode_per_block ; inode_nr++ , x++){

				if ( ! ext2fs_test_block_bitmap(imap,inode_nr)){
	
					inode = (struct ext2_inode_large*) (buf + (x*inodesize));
					c_time = ext2fs_le32_to_cpu(inode->i_ctime);
					mode = ext2fs_le32_to_cpu(inode->i_mode);
					if ((! c_time ) && (!(inode->i_mode & LINUX_S_IFMT)) ) {
						if(zero_flag) zero_flag++ ;
					 continue;
					}

					d_time = ext2fs_le32_to_cpu(inode->i_dtime);
					if ( (! d_time) || d_time <= t_after){
						ext2fs_mark_generic_bitmap(imap,inode_nr);
					  continue;
					}
// 1. magical step 
					if (LINUX_S_ISDIR(mode) && flag ){ 
						sprintf(pathname,"<%lu>",inode_nr);

	
						struct dir_list_head_t * dir = NULL;
						dir = get_dir3(NULL,0, inode_nr , "MAGIC-1",pathname, t_after,t_before, DELETED_OPT);
						if (dir) {
							lookup_local(des_dir, dir,t_after,t_before, DELETED_OPT | RECOV_ALL | LOST_DIR_SEARCH );
							clear_dir_list(dir);
						}
						
					}

// 2. magical step
					if (!flag){
						i_list = get_j_inode_list(current_fs->super, inode_nr);
						item = get_undel_inode(i_list,t_after,t_before);
						ext2fs_mark_generic_bitmap(imap,inode_nr);

						if (item) {
							if (! LINUX_S_ISDIR(item->inode->i_mode) ) {
								sprintf(pathname,"<%lu>",inode_nr);
								recover_file(des_dir,"MAGIC-2", pathname, (struct ext2_inode*)item->inode, inode_nr, 0);
							}
						}
						if (i_list) ring_del(i_list);
					}	
				}
			}
		}
	}
}
	if (pathname)
		 free(pathname);

	if(buf) {
		free(buf);
		buf = NULL;
	}
return;
} 




//check if the directory always recovert. if, then move to right place 
//return 0 if directory is moved
//this function is called from lookup_local() during 1. magical step
int check_find_dir(char *des_dir,ext2_ino_t inode_nr,char *pathname,char *filename){
char *recovername = NULL;
char *dirname = NULL;
struct stat     filestat;
int retval = 0;

recovername = malloc(strlen(des_dir) + strlen(pathname) + 30);
dirname = malloc(strlen(des_dir) + strlen(pathname) + strlen(filename) +10);
if (recovername && dirname){
	sprintf(recovername,"%s/MAGIC-1/<%lu>",des_dir,inode_nr);
	sprintf(dirname,"%s/%s/%s",des_dir,pathname,filename);

	retval = stat (recovername, &filestat);
	if ((!retval) && (S_ISDIR(filestat.st_mode))){
		retval = stat (dirname, &filestat);
		if (! retval){
			printf("Warning :can't move %s to %s ;file exist; will recover it again\n",recovername,dirname);
			retval = 1;
		}
		else{	
			if(check_dir(dirname)){
					fprintf(stderr,"Unknown error at target directory by file: %s\ntrying to continue normal\n", dirname);
			}else{
				retval = rename(recovername, dirname);
				rename_hardlink_path(recovername, dirname);
			}
//#ifdef DEBUG
			printf("move return: %d : ernno %d : %s -> %s\n",retval, errno, recovername, dirname);
//#endif
		}
	}	
	free(recovername);
	free(dirname);

return retval;
}
}


//2 step search for journalinode, will find some lost directory and files 
void imap_search(char* des_dir, __u32 t_after, __u32 t_before){
	printf("MAGIC-1 : start lost directory search\n"); 
	search_imap_inode(des_dir, t_after, t_before, 1); //search for lost fragments of directorys
	printf("MAGIC-2 : start lost file search\n");
	search_imap_inode(des_dir, t_after, t_before, 0); //search for lost files
return;
}


