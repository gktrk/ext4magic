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
#include <time.h>
#include "magic.h"
#include "journal.h"
#include "block.h"
#include "hard_link_stack.h"

extern ext2_filsys 	current_fs;
extern time_t		now_time ;


struct privat {
	int 		count;
	int 		error;
	unsigned char*	buf;
	char 		flag; 
};


//Subfunction for  "local_block_iterate3()" for load the first blocks to identify filetype 
int first_blocks ( ext2_filsys fs, blk64_t *blocknr, e2_blkcnt_t blockcnt,
                  blk64_t /*ref_blk*/x, int /*ref_offset*/y, void *priv )
{
        char *charbuf = NULL;

	errcode_t retval;
	int blocksize = fs->blocksize;

	if ((blockcnt >= 12) || ((struct privat*)priv)->count >=12)
		return BLOCK_ABORT;
	charbuf = (char*)((struct privat*)priv)->buf + (blocksize * blockcnt);

	if (((struct privat*)priv)->flag){
        	int allocated = ext2fs_test_block_bitmap ( fs->block_map, *blocknr );
        	if ( allocated ){
			((struct privat*)priv)->error = BLOCK_ABORT | BLOCK_ERROR ;
                	return (BLOCK_ABORT | BLOCK_ERROR);
		}
	}

	retval = io_channel_read_blk ( fs->io,  *blocknr,  1,  charbuf );
	((struct privat*)priv)->count = blockcnt;
	if (retval){
		 ((struct privat*)priv)->error = BLOCK_ERROR ;
		 return (BLOCK_ERROR);
	}
return retval;
}


static char* get_pathname(blk_t inode_nr, char* i_pathname, char *magic_buf, unsigned char* buf){
	struct found_data_t 		*help_data = NULL;
	int				str_len;
	__u32				name_len;
	char				*c;
	int				def_len = 22;
	char				name_str[22];
	__u32				scan = 0;

	help_data = malloc(sizeof(struct found_data_t));
	if (!help_data) return NULL;
	memset (help_data,0, sizeof(struct found_data_t));

	if ( ident_file(help_data,&scan,magic_buf,buf)){
	help_data->type = scan;
	str_len = strlen(magic_buf) + 1;
	c = strpbrk(magic_buf,";:, ");
	if (c){	
		*c = 0;
		name_len = c - magic_buf + def_len;
	} 
	else
		name_len = str_len + def_len;
	
	help_data->scan_result = malloc(str_len);
	help_data->name	 = malloc(name_len);
	if((!help_data->name) || (!help_data->scan_result)){
		free_file_data( help_data );
		fprintf(stderr,"ERROR: allocate memory\n");
	}
	else{
		strcpy(help_data->scan_result,magic_buf);
		strncpy(help_data->name, magic_buf , name_len - def_len+1);
		sprintf(name_str,"/I_%010lu",(long unsigned int)inode_nr);
		strcat(help_data->name,name_str);
		get_file_property(help_data);
	}
	i_pathname = malloc(name_len);
	if (i_pathname)
		strncpy (i_pathname,help_data->name,name_len); 
	}
	free_file_data(help_data);
return i_pathname;
}


static magic_t 		cookie = 0;


char* identify_filename(char* i_pathname, unsigned char *tmp_buf, struct ext2_inode* inode, blk_t inode_nr){
	struct privat 	priv ;
	char 		magic_buf[100];
	int		retval= 0;
	
	priv.count = priv.error = priv.flag = 0;
	priv.buf = tmp_buf;
	memset(magic_buf,0,100);
	if ((inode->i_mode  & LINUX_S_IFMT) == LINUX_S_IFREG){
		memset(tmp_buf,0,12 * current_fs->blocksize);
		// iterate first 12 Data Blocks
		retval = local_block_iterate3 ( current_fs, *inode, BLOCK_FLAG_DATA_ONLY, NULL, first_blocks, &priv );
		if (priv.count <12){
			strncpy(magic_buf, magic_buffer(cookie , tmp_buf,
				((inode->i_size < 12 * current_fs->blocksize) ?  inode->i_size : (12 * current_fs->blocksize))), 60);
			i_pathname = get_pathname(inode_nr, i_pathname, magic_buf, tmp_buf);
		}
	}
	return i_pathname;
}


// search inode by use imap (step1: flag 1 = only directory ; step2: flag 0 = only file)
static void search_imap_inode(char* des_dir, __u32 t_after, __u32 t_before, int flag)
{
struct ext2_group_desc 			*gdp;
struct 	ext2_inode_large 		*inode;
//struct dir_list_head_t 			*dir = NULL;
struct ring_buf* 			i_list = NULL;
r_item*					item = NULL;
int  					zero_flag, retval, load, x ,i ;
char 					*pathname = NULL;
char					*i_pathname = NULL;
char 					*buf= NULL;
unsigned char				*tmp_buf = NULL;
__u32 					blocksize, inodesize, inode_max, inode_per_group, block_count;
__u32 					inode_per_block , inode_block_group, group;
blk_t 					block_nr;
__u32   				c_time, d_time, mode;
ext2_ino_t 				first_block_inode_nr , inode_nr;


pathname = malloc(26);
blocksize = current_fs->blocksize;
inodesize = current_fs->super->s_inode_size;
inode_max = current_fs->super->s_inodes_count;
inode_per_group = current_fs->super->s_inodes_per_group;
buf = malloc(blocksize);
if (! (flag & 0x01) ){
	tmp_buf = malloc (12 * blocksize);
	if (!tmp_buf)
		goto errout;
	cookie = magic_open(MAGIC_MIME | MAGIC_NO_CHECK_COMPRESS | MAGIC_NO_CHECK_ELF | MAGIC_CONTINUE);
	if ((! cookie) ||  magic_load(cookie, NULL)){
		fprintf(stderr,"ERROR: can't find libmagic\n");
		goto errout;
	}
}

inode_per_block = blocksize / inodesize;
inode_block_group = inode_per_group / inode_per_block;

for (group = 0 ; group < current_fs->group_desc_count ; group++){
#ifdef EXT2_FLAG_64BITS
	gdp = ext2fs_group_desc(current_fs, current_fs->group_desc, group);
#else
	gdp = &current_fs->group_desc[group];
#endif
	zero_flag = 0;

	if (!(flag & 0x02)){ //skip this in disaster mode
		// NEXT GROUP IF INODE NOT INIT
		if (gdp->bg_flags & (EXT2_BG_INODE_UNINIT)) continue;
	
		// SET ZERO-FLAG IF FREE INODES == INODE/GROUP for fast ext3 
		if (gdp->bg_free_inodes_count == inode_per_group) zero_flag = 1;
	}

//FIXME for struct ext4_group_desc 48/64BIT	
	for (block_nr = gdp->bg_inode_table , block_count = 0 ;
			 block_nr < (gdp->bg_inode_table + inode_block_group); block_nr++, block_count++) {
		
		if (!(flag & 0x02)){ //skip this in disaster mode
			// break if the first block only zero inode
			if ((block_count ==1) && (zero_flag == (inode_per_block + 1))) break;
		}

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
					if ( ! ( flag & 0x02)) { 
						//no check this inode in disaster mode
 						if ((! c_time ) && (!(inode->i_mode & LINUX_S_IFMT)) ) {
							if(zero_flag) zero_flag++ ;
						continue;
						}
	
						d_time = ext2fs_le32_to_cpu(inode->i_dtime);
						if ( (! d_time) || d_time <= t_after){
							ext2fs_mark_generic_bitmap(imap,inode_nr);
						continue;
						}
					}
// 1. magical step 
					if (LINUX_S_ISDIR(mode) && ( flag & 0x01) && (pathname)){ 
						sprintf(pathname,"<%lu>",(long unsigned int)inode_nr);

	
						struct dir_list_head_t * dir = NULL;
						if (flag & 0x02){
							//disaster mode 
							//only search for undeleted entry 
							dir = get_dir3(NULL,0, inode_nr , "MAGIC-1",pathname, t_after,t_before, 0);
							if (dir) {
								lookup_local(des_dir, dir,t_after,t_before, RECOV_ALL | LOST_DIR_SEARCH );
								clear_dir_list(dir);
							}
						}
						else{   //search for all 
							dir = get_dir3(NULL,0, inode_nr , "MAGIC-1",pathname, t_after,t_before, DELETED_OPT);
							if (dir) {
								lookup_local(des_dir,dir,t_after,t_before,DELETED_OPT|RECOV_ALL|LOST_DIR_SEARCH);
								clear_dir_list(dir);
							}
						}

						
					}

// 2. magical step
					if (! (flag & 0x01) ){
						i_list = get_j_inode_list(current_fs->super, inode_nr);
						item = get_undel_inode(i_list,t_after,t_before);
						ext2fs_mark_generic_bitmap(imap,inode_nr);

						if (item) {
							if (! LINUX_S_ISDIR(item->inode->i_mode) ) {
								i_pathname = identify_filename(i_pathname, tmp_buf,
										(struct ext2_inode*)item->inode, inode_nr);
								sprintf(pathname,"<%lu>",(long unsigned int)inode_nr);
								recover_file(des_dir,"MAGIC-2", ((i_pathname)?i_pathname : pathname),
									     (struct ext2_inode*)item->inode, inode_nr, 0);
								if(i_pathname){
									free(i_pathname);
									i_pathname = NULL;
								}
							}
						}
						if (i_list) ring_del(i_list);
					}	
				}
			}
		}
	}
}
errout:
	if (pathname)
		 free(pathname);

	if(buf) {
		free(buf);
		buf = NULL;
	}

	if (tmp_buf){
		free(tmp_buf);
		tmp_buf = NULL;
	}
	if (cookie){
		magic_close(cookie);
		cookie = 0;
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
	sprintf(recovername,"%s/MAGIC-1/<%lu>",des_dir,(long unsigned int)inode_nr);
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
#ifdef DEBUG
			printf("move return: %d : ernno %d : %s -> %s\n",retval, errno, recovername, dirname);
#endif
		}
	}	
	free(recovername);
	free(dirname);

return retval;
}
return 0;
}



void search_journal_lost_inode(char* des_dir, __u32 t_after, __u32 t_before, int flag){

struct 	ext2_inode	*p_inode;
struct  ext2_inode	inode;
int  			i;
char 			*pathname = NULL;
char			*i_pathname = NULL;
char 			*buf= NULL;
unsigned char		*tmp_buf = NULL;
__u32 			blocksize, inodesize;
__u32 			inode_per_block;
ext2_ino_t 		inode_max, inode_nr;


pathname = malloc(26);
blocksize = current_fs->blocksize;
inodesize = current_fs->super->s_inode_size;
inode_max = current_fs->super->s_inodes_count;
inode_per_block = blocksize / inodesize;
inode_nr = inode_max ;

buf = malloc(blocksize);
if (! (flag & 0x01) ){
	tmp_buf = malloc (12 * blocksize);
	if (!tmp_buf)
		goto errout;
	cookie = magic_open(MAGIC_MIME | MAGIC_NO_CHECK_COMPRESS | MAGIC_NO_CHECK_ELF | MAGIC_CONTINUE);
	if ((! cookie) ||  magic_load(cookie, NULL)){
		fprintf(stderr,"ERROR: can't find libmagic\n");
		goto errout;
	}
}

while ( get_pool_block(buf) ){
	for (i=0 ;i < inode_per_block; i++){
		inode_nr++;
		p_inode = (struct ext2_inode*) (buf + (i * inodesize)); 

                memset(&inode, 0, sizeof(struct  ext2_inode));
#ifdef WORDS_BIGENDIAN
	 	ext2fs_swap_inode(current_fs, &inode, p_inode, 0);
#else
		memcpy(&inode, p_inode,128);
#endif
		if((inode.i_dtime) || (!inode.i_size) || (!inode.i_blocks) || (!LINUX_S_ISREG(inode.i_mode)))
			continue;
		if (check_file_stat(&inode)){
			i_pathname = identify_filename(i_pathname, tmp_buf, &inode, inode_nr);
			sprintf(pathname,"<%lu>",(long unsigned int)inode_nr);
			recover_file(des_dir,"MAGIC-2", ((i_pathname)?i_pathname : pathname), &inode, inode_nr, 1);
		}
		if(i_pathname){
			free(i_pathname);
			i_pathname = NULL;
		}
				
	}
}
errout:
	if (pathname)
		 free(pathname);

	if(buf) {
		free(buf);
		buf = NULL;
	}

	if (tmp_buf){
		free(tmp_buf);
		tmp_buf = NULL;
	}
	if (cookie){
		magic_close(cookie);
		cookie = 0;
	}
return;
}


//2 step search for journalinode, will find some lost directory and files 
void imap_search(char* des_dir, __u32 t_after, __u32 t_before , int disaster ){
	printf("MAGIC-1 : start lost directory search\n"); 
	search_imap_inode(des_dir, t_after, t_before, 1 | disaster ); //search for lost fragments of directorys
	printf("MAGIC-2 : start lost file search\n");
	search_imap_inode(des_dir, t_after, t_before, 0 | disaster ); //search for lost files
	if ((!disaster) && (t_before == (__u32)now_time)){
		printf("MAGIC-2 : start lost in journal search\n");
		search_journal_lost_inode(des_dir, t_after, t_before, 0);
	}	
return;
}

