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
#include "inode.h"

#include "magic.h"

#define MAX_RANGE 16



extern ext2_filsys     current_fs ;
ext2fs_block_bitmap 	  	d_bmap = NULL ; 


__u32 get_block_len(char *buf){
	int		len = current_fs->blocksize -1;

	while ((len >= 0) && (!(*(buf + len)))) 
			len--;
	return ++len;
}



struct found_data_t* free_file_data(struct found_data_t* old){
	if(old->inode) free(old->inode);
	if(old->scan_result) free(old->scan_result);
	if(old->name)  free(old->name);
	free(old);
	old = NULL;
	return old;
}



struct found_data_t* new_file_data(blk_t blk,__u32 scan,char *magic_buf, char* buf, __u32 *f){
	struct found_data_t 		*new;
	int				str_len;
	__u32				name_len;
	char				*c;
	int				def_len = 20;

	new = malloc(sizeof(struct found_data_t));
	if (!new) return NULL;

	new->first = blk;
	new->last  = blk;
	new->leng  = 0;
	new->scan  = scan;
	new->size  = 0;
	new->func = NULL;
	if ( ident_file(new,&scan,magic_buf,buf)){
	new->type = scan;
	str_len = strlen(magic_buf) + 1;
	c = strpbrk(magic_buf,";:, ");
	if (c)
		name_len = c - magic_buf + def_len; 
	else
		name_len = str_len + def_len;
	
	new->scan_result = malloc(str_len);
	new->name	 = malloc(name_len);
	new->inode	 = new_inode();
	if((!new->scan_result) || (!new->name) || (!new->inode)){
		new = free_file_data(new);
		fprintf(stderr,"ERROR; while allocate memory for found file struct\n");
	}
	else{
		strcpy(new->scan_result,magic_buf);
		strncpy(new->name, magic_buf , name_len - 20);
		sprintf(new->name + name_len - 20,"/%010u",blk);
		get_file_property(new);
		new->func(buf,&name_len,scan,2,new);
	}
}
	*f++;
return new;
}




struct found_data_t* recover_file_data(char *des_dir, struct found_data_t* this, __u32 *follow){
	recover_file(des_dir,"MAGIC_3",this->name,(struct ext2_inode*)this->inode, 0 , 1);
	free_file_data(this);
	*follow = 0;
	return NULL;
}




struct found_data_t* forget_file_data(struct found_data_t* this, __u32 *p_follow){
	printf("TRASH :  %s  : leng  %lu :  begin  %lu\n", this->name, this->inode->i_size , this->first);
	free_file_data(this);
	*p_follow = 0;
	return NULL;
}




__u32 add_file_data(struct found_data_t* this, blk_t blk, __u32 scan ,__u32 *f){
	__u32   retval = 0;
	
        if (inode_add_block(this->inode , blk , current_fs->blocksize)){
		this->leng++;
		this->last = blk;
		*f++ ;
	}
return *f ;
}



int file_data_correct_size(struct found_data_t* this, int size){
	unsigned long long  	i_size;
	int			flag=0;
	
	if (size <= current_fs->blocksize){
		flag = 1;
		i_size = (((unsigned long long)this->inode->i_size_high) << 32) + this->inode->i_size;
		i_size -= (current_fs->blocksize - size);
		this->inode->i_size = i_size & 0xffffffff ;
		this->inode->i_size_high = i_size >> 32 ;
	}
  return flag;
}
	


int check_file_data_end(struct found_data_t* this,char *buf){
	int 		size;
	int 		ret = 0;

	size = get_block_len(buf);
	ret = this->func(buf, &size ,this->scan ,0 , this);

	if (ret) 			
           ret = file_data_correct_size(this,size);
	return ret;
}




//?????
int check_data_passage(char *a_buf, char *b_buf){
	int		 i, blocksize;
	int		sum[4][2];
	
	

}


//FIXME obsolete: must switch to FLAG=1 of func() of the filetype
int check_file_data_possible(struct found_data_t* this, __u32 scan ,char *buf, __u32 follow){
	int ret = 0;
	switch (this->scan & M_IS_FILE){
//		case (M_TXT | M_APPLI) :
		case M_TXT :
			if (scan & M_TXT) ret = 1;
		break;
		case M_VIDEO :
		case M_AUDIO :
		case M_IMAGE :
			if (!(scan & M_IS_META)) ret = 1;
		break;
		case M_APPLI :
			if (!(scan & M_IS_META)) ret = 1;
		break;
	}
	if (!(this->scan & M_IS_FILE))
		ret = 1; 
	return ret;
}




int add_ext3_file_meta_data(struct found_data_t* this, char *buf, blk_t blk){
	blk_t   next_meta;
	blk_t	last_data = 0;
	next_meta = inode_add_meta_block(this->inode , blk, &last_data, buf );
	this->last = last_data;
	
	if (next_meta > 10 && (ext2fs_test_block_bitmap ( d_bmap, next_meta) && (! ext2fs_test_block_bitmap( bmap, next_meta)))){
		io_channel_read_blk ( current_fs->io,  next_meta, 1, buf );

		if (check_meta3_block(buf, blk, get_block_len(buf))){
			next_meta = inode_add_meta_block(this->inode , next_meta , &last_data, buf );
			this->last = last_data;

			if (next_meta > 10 && (ext2fs_test_block_bitmap ( d_bmap, next_meta) && (! ext2fs_test_block_bitmap( bmap, next_meta)))){
				io_channel_read_blk ( current_fs->io,  next_meta, 1, buf );

				if (check_meta3_block(buf, blk, get_block_len(buf))){
					next_meta = inode_add_meta_block(this->inode , next_meta , &last_data, buf );
					this->last = last_data;
				}
			}			
		}
	}
return (next_meta) ? 0 : 1 ;
}



blk_t check_indirect_meta3(char *block_buf,blk_t blk, __u32 blocksize){
	blk_t  	*pb_block;
	blk_t	last;
	int 	i = blocksize/sizeof(blk_t);
	int	count=0;
	
	pb_block = (blk_t*)block_buf;
	last = ext2fs_le32_to_cpu(*pb_block);
	while (i && last ) {
		pb_block++;
		i--;
		if ((ext2fs_le32_to_cpu(*pb_block) -1) == last)
			count++;
		if (ext2fs_le32_to_cpu(*pb_block))
			last = ext2fs_le32_to_cpu(*pb_block);
		else 
			break;
	}
	return (count>3) ? last : blk ;
}



int check_meta3_block(char *block_buf, blk_t blk, __u32 size){
	blk_t 		block, *pb_block;
	int		i,j ;

	size = (size + 3) & 0xFFFFFC ;
	if (! size ) return 0;

	for (i = size-4 ; i>=0 ; i -= 4){ 
		pb_block = (blk_t*)(block_buf+i);
		block = ext2fs_le32_to_cpu(*pb_block);
		if( block && (block < current_fs->super->s_blocks_count) &&
		 (ext2fs_test_block_bitmap( d_bmap, block)) && (!ext2fs_test_block_bitmap(bmap,block))){
			if (i) {
				for (j = i - 4 ; j >= 0 ;j -= 4){
					if (block == ext2fs_le32_to_cpu(*((blk_t*)(block_buf+j))))
						return 0;
				}
			}
			else {
				if (block == blk+1)
					return 1;
			}
				
		}
		else
			return 0;
	}
}



int check_meta4_block(char *block_buf, blk_t blk, __u32 size){
	__u16		*p_h16;
	__u16		h16;

	if(!((block_buf[0] == (char)0x0a) && (block_buf[1] == (char)0xf3)))
		return 0;
	p_h16 = (__u16*)block_buf+2;
	h16 = ext2fs_le16_to_cpu(*p_h16);
	p_h16 = (__u16*)block_buf+4;
	if (ext2fs_le16_to_cpu(*p_h16) != (__u16)((current_fs->blocksize -12)/ 12))
		return 0;
	if ((!h16) || (h16 > ext2fs_le16_to_cpu(*p_h16)))
		return 0;
	
	return 1 ;
}




int check_dir_block(char *block_buf, blk_t blk, __u32 size){
	struct ext2_dir_entry_2      *dir_entry;
	ext2_ino_t	inode_nr;
	__u16		len;
	
	dir_entry = (struct ext2_dir_entry_2*)block_buf;
	
	inode_nr = ext2fs_le32_to_cpu(dir_entry->inode);
	if ((inode_nr && (inode_nr < EXT2_GOOD_OLD_FIRST_INO)) || (inode_nr > current_fs->super->s_inodes_count))
		return 0;
	len = ext2fs_le16_to_cpu(dir_entry->rec_len);
	if ((len < 8) || (len % 4) || (len > current_fs->blocksize) ||(len < (((unsigned)dir_entry->name_len + 11) & ~3 ))) 
		return 0;
	if ((dir_entry->name_len & 0x01) &&  dir_entry->name[(unsigned)dir_entry->name_len])
		return 0;
	if (len < current_fs->blocksize - 12){
		dir_entry = (struct ext2_dir_entry_2*) (block_buf + len);
		len = ext2fs_le16_to_cpu(dir_entry->rec_len);
		if ((len < 8) || (len % 4) || (len > current_fs->blocksize) ||(len < (((unsigned)dir_entry->name_len + 11) & ~3 ))) 
			return 0;
		if ((dir_entry->name_len & 0x01) &&  dir_entry->name[(unsigned)dir_entry->name_len])
		return 0;
	}
	return 1;
}


// skip not of interest blocks : return 1 if skiped
static int skip_block(blk_t *p_blk ,struct ext2fs_struct_loc_generic_bitmap *ds_bmap ){
	struct 	ext2fs_struct_loc_generic_bitmap 	*p_bmap;
	blk_t		o_blk;
	int 		flag = 0;

	o_blk = (*p_blk) >> 3;
	p_bmap = (struct ext2fs_struct_loc_generic_bitmap *) bmap;
	while (((! *(ds_bmap->bitmap + o_blk)) || (*(ds_bmap->bitmap + o_blk) == 0xff)) && (o_blk < (ds_bmap->real_end >> 3))){  
		o_blk ++;
		flag = 1;
	}
	*p_blk = o_blk << 3 ;
						
return flag;
} 


//magic scanner 
int magic_check_block(char* buf,magic_t cookie , magic_t cookie_f, char *magic_buf, __u32 size, blk_t blk){
	int	count = size-1;
	int	*i , len;
	char	text[100];
	char 	*p_search;
	__u32	retval = 0;
	char	searchstr[] = "7-zip cpio image Image filesystem CD-ROM MPEG 9660 Targa Kernel boot Linux x86 ";
	char	token[10]; 
 	
	strncpy(text,magic_buffer(cookie_f,buf , size),99);
	strncpy(magic_buf, magic_buffer(cookie , buf , size),99);
	while (count >= 0 && (*(buf+count) == 0)) count-- ;

	printf("Scan Result :  %s    %d\n", magic_buf , count+1) ;
	printf("RESULT : %s \n",text);
	

	if (!strncmp(text,"data",4)){
		if (count == -1) {
			retval |= M_BLANK;
			goto out;
		}	
	}

	if((strstr(magic_buf,"text/")) && (strstr(text,"text"))){
		retval |= M_TXT ;
	}

	if (strstr(magic_buf,"charset=binary")){
			retval |= M_BINARY ;
	}
	
	if (!(retval & M_TXT) && (strstr(magic_buf,"application/octet-stream")) && (!(strncmp(text,"data",4)))){
		retval |= M_DATA;
	}

	if ((retval & M_DATA) || (count < 32) || (ext2fs_le32_to_cpu(*(blk_t*)buf) == blk +1)) {
		if (check_meta3_block(buf, blk, count+1)){
				retval = M_EXT3_META ;	
				goto out;
		}
		if (check_meta4_block(buf, blk, count+1)){
				retval = M_EXT4_META ;	
				goto out;
		}
		if (check_dir_block(buf, blk, count+1)){
				retval = M_DIR ;
				goto out;
		}
	}

	if (retval & M_DATA)
		goto out;

	if (retval & M_TXT){
		if (strstr(magic_buf,"ascii")){
			retval |= M_ASCII ;
		}

		if(strstr(magic_buf,"iso")){
			retval |= M_ASCII ;
		}	

		if(strstr(magic_buf,"utf")){
			retval |= M_UTF ;
		}	
		goto out;
	}


	if (strstr(magic_buf,"application/octet-stream")){
		p_search = searchstr;
		while (*p_search){
			len=0;
			while((*p_search) != 0x20){
				token[len] = *p_search;
				p_search++;
				len++;
			}
			token[len] = 0;
			if (strstr(text,token)){
				strncpy(magic_buf,text,99);
				retval |= (M_APPLI | M_ARCHIV);
				break;
			}
			p_search++;
		}
		goto out;
	}

	if (strstr(magic_buf,"application/")){
		retval |= M_APPLI;
		goto out;
	}
	
	if (strstr(magic_buf,"image/")){
		retval |= M_IMAGE;
		goto out;
	}
	
	if (strstr(magic_buf,"audio/")){
		retval |= M_AUDIO;
		goto out;
	}

	if (strstr(magic_buf,"video/")){
		retval |= M_VIDEO;
		goto out;
	}
	
	if (strstr(magic_buf,"message/")){
		retval |= M_MESSAGE;
		goto out;
	}
	
	if (strstr(magic_buf,"model/")){
		retval |= M_MODEL;
		goto out;
	}

out:
	printf("BLOCK_SCAN : 0x%08x\n",retval & 0xffffe000);
	blockhex(stdout,buf,0,(count < (64)) ? count  : 64 );

	retval |= (count+1);
	
	return retval;
}




struct found_data_t* soft_border(char *des_dir, char *buf, struct found_data_t* file_data, __u32* follow, blk_t blk){
	if ( check_file_data_end(file_data, buf ))
		file_data = recover_file_data(des_dir, file_data, follow);
	else{ 
		file_data = forget_file_data(file_data, follow);	
//		printf("Don't recover this file, current block %d \n",blk);
	}
return file_data;
}


int get_range(blk_t* p_blk ,struct ext2fs_struct_loc_generic_bitmap *ds_bmap, char* buf){
	blk_t  			begin;
	blk_t			end;
	int 			count=1;
	for (begin = *p_blk; begin < ds_bmap->real_end ; begin++){
		if((!(begin & 0x7)) && skip_block(&begin, ds_bmap))
			printf("jump to %d \n",begin);

		if (ext2fs_test_block_bitmap ( d_bmap, begin) && (! ext2fs_test_block_bitmap( bmap, begin)))
			break;
	}
	*p_blk = begin;
	if (begin >= ds_bmap->real_end)
		return 0;

	for (end = begin,count=1 ; count < MAX_RANGE ; ){
		if (ext2fs_test_block_bitmap(d_bmap, end+1) && (! ext2fs_test_block_bitmap( bmap, end+1))){
			end++;
			count++;
		}
		else break;
		
	}
	*(p_blk+1) = end;
	if (io_channel_read_blk ( current_fs->io, begin , count,  buf )){
		fprintf(stderr,"ERROR: while read block %10u + %d\n",begin,count);
		return 0;
	}
return count;
}	



//magic_block_scan_main
int magic_block_scan(char* des_dir, __u32 t_after){
magic_t 					cookie = 0;
magic_t						cookie_f = 0;
struct 	ext2fs_struct_loc_generic_bitmap 	*ds_bmap;
struct found_data_t				*file_data = NULL;
blk_t						blk[2] ;
blk_t						j;
char 						*buf = NULL;
char						*magic_buf = NULL;
char						*tmp_buf = NULL;
int						blocksize, ds_retval,count,i;// step;
__u32						scan,follow;


blocksize = current_fs->blocksize ;
count = 0;
blk[0] = 0;
cookie = magic_open(MAGIC_MIME | MAGIC_NO_CHECK_COMPRESS | MAGIC_NO_CHECK_ELF );
cookie_f = magic_open(MAGIC_NO_CHECK_COMPRESS | MAGIC_NO_CHECK_ELF );
if ((! cookie) ||  magic_load(cookie, NULL) || (! cookie_f) || magic_load(cookie_f, NULL)){
	fprintf(stderr,"ERROR: can't find libmagic\n");
	goto errout;
}
buf = malloc(blocksize * MAX_RANGE);
tmp_buf = malloc(blocksize);
magic_buf = malloc(200);
if ((! buf) || (! magic_buf) || (! tmp_buf)){
	fprintf(stderr,"ERROR: can't allocate memory\n");
	goto errout;
}

ds_retval = init_block_bitmap_list(&d_bmap, t_after);
while (ds_retval){
	ds_retval = next_block_bitmap(d_bmap);
	if (ds_retval == 2 ) 
		continue;

	if (d_bmap && ds_retval ){
		ds_bmap = (struct ext2fs_struct_loc_generic_bitmap *) d_bmap;
		 
	count = get_range(blk ,ds_bmap, buf);

	while (count){
		printf(" %d    %d    %d\n", blk[0],blk[1],count);
		for (i = 0; i< ((count>12) ? MAX_RANGE - 12 : count) ;i++){
			scan = magic_check_block(buf+(i*blocksize), cookie, cookie_f , magic_buf , blocksize ,blk[0]+i);
			if(scan & (M_DATA | M_BLANK | M_IS_META))
				continue;
			
			printf("SCAN %d : %09x : %s\n",blk[0]+i,scan,magic_buf);
			if (((count -i) > 12) && (ext2fs_le32_to_cpu(*(__u32*)(buf +((i+12)*blocksize))) == blk[0] + i +1 + 12)){
				follow = 0;
				file_data = new_file_data(blk[0]+i,scan,magic_buf,buf+(i*blocksize),&follow);
				for(j=blk[0]+i; j<(blk[0]+i+12);j++)
					 add_file_data(file_data, j, scan ,&follow);
				scan = magic_check_block(buf+((i+12)*blocksize), cookie, cookie_f , magic_buf , blocksize ,blk[0]+i+12);	
				if (scan & M_EXT3_META){
					if (add_ext3_file_meta_data(file_data, buf+((i+12)*blocksize), j)){
						io_channel_read_blk (current_fs->io, file_data->last,  1, tmp_buf);
						file_data = soft_border(des_dir, tmp_buf, file_data, &follow, j);
						break;
					}
					else
						file_data = forget_file_data(file_data, &follow);				
				}
				else
					file_data = forget_file_data(file_data, &follow);					
			}
			else{
				// no matches
				// In the first step we are taking nothing
	
//				printf(" %d not matches %d    == %d  \n\n\n",  blk[0]+i, ext2fs_le32_to_cpu(*(buf +((i+12)*blocksize))), blk[0] + i + 12+1);
			}	
		} 
				


		
		blk[0] += (i)?i:1;
		count = get_range(blk ,ds_bmap,buf);
		
	}


	}//operation
} //while transactions


errout:
clear_block_bitmap_list(d_bmap);
if (file_data) free_file_data(file_data);
if (buf) free(buf);
if (tmp_buf) free(tmp_buf);
if (magic_buf) free(magic_buf);
if (cookie) magic_close(cookie); 
if (cookie_f) magic_close(cookie_f);
} //end

