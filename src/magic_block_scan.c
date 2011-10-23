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


//#define DEBUG_MAGIC_SCAN

extern ext2_filsys     current_fs ;
ext2fs_block_bitmap 	  	d_bmap = NULL ; 


static __u32 get_block_len(unsigned char *buf){
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



static struct found_data_t* new_file_data(blk_t blk,__u32 scan,char *magic_buf, unsigned char* buf, __u32 *f){
	struct found_data_t 		*new;
	int				str_len;
	__u32				name_len;
	char				*c;
	int				def_len = 20;
	char				name_str[20];

	new = malloc(sizeof(struct found_data_t));
	if (!new) return NULL;

	new->first = blk;
	new->last  = blk;
	new->leng  = 0;
	new->scan  = scan;
	new->size  = 0;
	new->h_size = 0;
	new->func = NULL;
	if ( ident_file(new,&scan,magic_buf,buf)){
	new->type = scan;
	str_len = strlen(magic_buf) + 1;
	c = strpbrk(magic_buf,";:, ");
	if (c){	
		*c = 0;
		name_len = c - magic_buf + def_len;
	} 
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
		strncpy(new->name, magic_buf , name_len - def_len+1);
		sprintf(name_str,"/%010lu",blk);
		strcat(new->name,name_str);
		get_file_property(new);
		new->func(buf,&name_len,scan,2,new);
	}
}
	*f++;
return new;
}




static struct found_data_t* recover_file_data(char *des_dir, struct found_data_t* this, __u32 *follow){
	recover_file(des_dir,"MAGIC-3",this->name,(struct ext2_inode*)this->inode, 0 , 1);
	free_file_data(this);
	*follow = 0;
	return NULL;
}




static struct found_data_t* forget_file_data(struct found_data_t* this, __u32 *p_follow){
#ifdef  DEBUG_MAGIC_SCAN
	printf("TRASH :  %s  : leng  %lu :  begin  %lu\n", this->name, this->inode->i_size , this->first);
//	dump_inode(stdout, "",0, (struct ext2_inode *)this->inode, 1);
#endif
	free_file_data(this);
	*p_follow = 0;
	return NULL;
}




static __u32 add_file_data(struct found_data_t* this, blk_t blk, __u32 scan ,__u32 *f){
	__u32   retval = 0;
	
        if (inode_add_block(this->inode , blk , current_fs->blocksize)){
		this->leng++;
		this->last = blk;
		(*f)++ ;
	}
return *f ;
}



static int file_data_correct_size(struct found_data_t* this, int size){
	unsigned long long  	i_size;
	int			flag=0;
	int			i,ref;
	
	if (size <= current_fs->blocksize){
		flag = 1;
		i_size = (((unsigned long long)this->inode->i_size_high) << 32) + this->inode->i_size;
		i_size -= (current_fs->blocksize - size);
		this->inode->i_size = i_size & 0xffffffff ;
		this->inode->i_size_high = i_size >> 32 ;
		if ((i_size <= (12 * current_fs->blocksize))&&(!(this->inode->i_flags & EXT4_EXTENTS_FL))) {
 			if(this->inode->i_block[EXT2_IND_BLOCK]){
				this->inode->i_block[EXT2_IND_BLOCK] = 0;
				this->inode->i_block[EXT2_DIND_BLOCK] = 0;
				this->inode->i_block[EXT2_TIND_BLOCK] = 0;
			}
			for (i = current_fs->blocksize,ref=1; i<i_size; i+=current_fs->blocksize,ref++);
			while (ref < EXT2_IND_BLOCK){
				this->inode->i_block[ref] = 0;
				ref++;
			}
					
			
		}
	}
  return flag;
}
	


static int check_file_data_end(struct found_data_t* this,unsigned char *buf, __u32 mask){
	int 		size;
	int 		ret = 0;

	size = get_block_len(buf);
	ret = (this->func(buf, &size ,this->scan ,0 , this) & mask) ;

	if (ret) 			
           ret = file_data_correct_size(this,size);
	return ret;
}




//?????
static int check_data_passage(char *a_buf, unsigned char *b_buf){
	int		 i, blocksize;
	int		sum[4][2];
	
	

}



static int check_file_data_possible(struct found_data_t* this, __u32 scan ,unsigned char *buf){
	int ret = 0;
	int size ;
	size = (scan & M_SIZE );
	ret = this->func(buf, &size ,scan ,1 , this);
	return ret;
}




static int check_meta3_block(unsigned char *block_buf, blk_t blk, __u32 size, int flag){
	blk_t 		block, *pb_block;
	int		i,j ;

	size = (size + 3) & 0xFFFFFC ;
	if (! size ) return 0;

	for (i = size-4 ; i>=0 ; i -= 4){ 
		pb_block = (blk_t*)(block_buf+i);
		block = ext2fs_le32_to_cpu(*pb_block);
		if( block && (block < current_fs->super->s_blocks_count) &&
		   (ext2fs_test_block_bitmap( d_bmap, block)) && ( ((!flag) && (!ext2fs_test_block_bitmap(bmap,block))) ||
		   (((flag == 1) && (i == (size -4)) && (i>4))||(!ext2fs_test_block_bitmap(bmap,block))) ||
		   (flag == 2) ) ){

 
			if (i) {  //work not by sparse file
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




static int check_indirect_meta3(unsigned char *block_buf){
	blk_t  	*pb_block;
	blk_t	last;
	int 	i = current_fs->blocksize/sizeof(blk_t) -1;
	int	count=0;
	int	next =0;
	
	pb_block = (blk_t*)block_buf;
	last = ext2fs_le32_to_cpu(*pb_block);
	while (i && last ) {
		pb_block++;
		i--;
		count++;
		if ((ext2fs_le32_to_cpu(*pb_block) -1) == last)
			next++;
		if (ext2fs_le32_to_cpu(*pb_block))
			last = ext2fs_le32_to_cpu(*pb_block);
		else 
			break;
	}
	return (next >= (count>>1)) ? 1 : 0;
}



static int check_dindirect_meta3(unsigned char * block_buf){
	__u32			*p_blk;
	__u32			block;
	unsigned char  		*buf = NULL;
	int 			ret = 0;
	
	buf  = malloc(current_fs->blocksize);
	if (buf) {
		p_blk = (__u32*) block_buf;
		block = ext2fs_le32_to_cpu(*p_blk);
		io_channel_read_blk ( current_fs->io, block, 1, buf );
		if (check_meta3_block(buf, block, get_block_len(buf),2)){
			ret = check_indirect_meta3(buf);
		}		
	free (buf);
	}

return ret;
}



static int check_tindirect_meta3(unsigned char * block_buf){
	__u32			*p_blk;
	__u32			block;
	unsigned char  		*buf = NULL;
	int 			ret = 0;
	
	buf  = malloc(current_fs->blocksize);
	if (buf) {
		p_blk = (__u32*) block_buf;
		block = ext2fs_le32_to_cpu(*p_blk);
		io_channel_read_blk ( current_fs->io, block, 1, buf );
		if (check_meta3_block(buf, block, get_block_len(buf),2)){
			ret = check_dindirect_meta3(buf);
		}		
	free (buf);
	}

return ret;
}



static int check_meta4_block(unsigned char *block_buf, blk_t blk, __u32 size){
	__u16		*p_h16;
	__u16		h16;

	if(!((block_buf[0] == 0x0a) && (block_buf[1] == (unsigned char)0xf3)))
		return 0;
	p_h16 = (__u16*)(block_buf+2);
	h16 = ext2fs_le16_to_cpu(*p_h16);
	p_h16 = (__u16*)(block_buf+4);
	if (ext2fs_le16_to_cpu(*p_h16) != (__u16)((current_fs->blocksize -12)/ 12))
		return 0;
	if ((!h16) || (h16 > ext2fs_le16_to_cpu(*p_h16)))
		return 0;
	return 1 ;
}



static int check_dir_block(unsigned char *block_buf, blk_t blk, __u32 size){
        struct ext2_dir_entry_2         *dir_entry;
        ext2_ino_t                      inode_nr;
        __u16                           len;
        __u16                           p_len;

        dir_entry = (struct ext2_dir_entry_2*)block_buf;

        inode_nr = ext2fs_le32_to_cpu(dir_entry->inode);
        if ((inode_nr && (inode_nr < EXT2_GOOD_OLD_FIRST_INO)) || (inode_nr > current_fs->super->s_inodes_count))
                return 0;
        len = ext2fs_le16_to_cpu(dir_entry->rec_len);
        if ((len < 8) || (len % 4) || (len> current_fs->blocksize) ||(len < (((unsigned)dir_entry->name_len + 11) & ~3 ))||(!dir_entry->name_len) || (! dir_entry->file_type) || (dir_entry->file_type & 0xf8))
                return 0;
        //if ((dir_entry->name_len & 0x01) &&  dir_entry->name[(unsigned)dir_entry->name_len])
        //      return 0;
        p_len =len;
        if (len < current_fs->blocksize - 12){
                dir_entry = (struct ext2_dir_entry_2*) (block_buf + len);
                len = ext2fs_le16_to_cpu(dir_entry->rec_len);
                if ((len < 8) || (len % 4) || ((len+p_len) > current_fs->blocksize) ||(len < (((unsigned)dir_entry->name_len + 11) & ~3 ))||(!dir_entry->name_len) || (! dir_entry->file_type) || (dir_entry->file_type & 0xf8) )
                        return 0;
                if ((dir_entry->name_len & 0x01) &&  dir_entry->name[(unsigned)dir_entry->name_len])
                return 0;
        }else
                if (block_buf[current_fs->blocksize -1])
                        return 0;
        return 1;
}



static int check_acl_block(unsigned char *block_buf, blk_t blk, __u32 size){
	__u32		*p32;
	int 		i;
	
	p32 = (__u32*)block_buf;
	if ((!(*p32)) || (ext2fs_le32_to_cpu(*p32) & (~ 0xEA030000)))
		return 0;
	p32 += 2;
	if ((!(*p32)) || (ext2fs_le32_to_cpu(*p32) > 0xff))
		return 0;
	p32++;
	if (! (*p32))
		return 0;
	p32++;
	for (i = 0; i<4; i++){
		if (*p32)
			return 0;
		p32++;
	}
	return (size > (current_fs->blocksize-32)) ? 1 : 0;
}



static int add_ext4_extent_idx_data(struct found_data_t* this, void *buf, blk_t blk){
	int ret;
	ret = inode_add_extent(this->inode ,blk, buf, 1);
	this->last = get_last_block_ext4(this->inode);
	return ret;
}



static int add_ext3_file_meta_data(struct found_data_t* this, unsigned char *buf, blk_t blk){
	blk_t   next_meta = 0;
	blk_t	meta = 0;
	blk_t	last_data = 0;
	int 	ret = 0;
	

	if (check_indirect_meta3(buf)){
		ret = inode_add_meta_block(this->inode , blk, &last_data, &next_meta, buf );
		this->last = last_data;
	}
	
	if (ret && (next_meta > 10 && (ext2fs_test_block_bitmap ( d_bmap, next_meta) && (! ext2fs_test_block_bitmap( bmap, next_meta))))){
		meta = next_meta;
		next_meta = 0;
		io_channel_read_blk ( current_fs->io, meta, 1, buf );

		if (check_meta3_block(buf, meta, get_block_len(buf),1) &&  check_dindirect_meta3(buf)){
			ret = inode_add_meta_block(this->inode , meta, &last_data, &next_meta, buf );
			this->last = last_data;

			if (ret && (next_meta > 10 && (ext2fs_test_block_bitmap ( d_bmap, next_meta) && (! ext2fs_test_block_bitmap( bmap, next_meta))))){
				meta = next_meta;
				next_meta = 0;
				io_channel_read_blk ( current_fs->io, meta, 1, buf );

				if (check_meta3_block(buf, meta, get_block_len(buf),1) && check_tindirect_meta3(buf)){
					ret = inode_add_meta_block(this->inode , meta, &last_data, &next_meta, buf );
					this->last = last_data;
				}
				
			}
					
		}
	
	}
return ret ;
}



// skip not of interest blocks : return 1 if skiped
static int skip_block(blk_t *p_blk ,struct ext2fs_struct_loc_generic_bitmap *ds_bmap ){
	struct 	ext2fs_struct_loc_generic_bitmap 	*p_bmap;
	blk_t		o_blk;
	int 		flag = 0;

	o_blk = (*p_blk) >> 3;
	p_bmap = (struct ext2fs_struct_loc_generic_bitmap *) bmap;
	while (((! *(ds_bmap->bitmap + o_blk)) || (*(p_bmap->bitmap + o_blk) == (unsigned char)0xff) || 
		(*(ds_bmap->bitmap + o_blk) == *(p_bmap->bitmap + o_blk))) && (o_blk < (ds_bmap->end >> 3))){  
		o_blk ++;
		flag = 1;
	}
	*p_blk = (o_blk << 3)+1 ;
						
return flag;
} 


//magic scanner 
static int magic_check_block(unsigned char* buf,magic_t cookie , magic_t cookie_f, char *magic_buf, __u32 size, blk_t blk, int deep){
	int	count = current_fs->blocksize -1 ;
	int	*i , len;
	char	text[100] = "";
	char 	*p_search;
	__u32	retval = 0;
	char	token[20]; 

	while ((count >= 0) && (*(buf+count) == 0)) count-- ;
	if (ext2fs_le32_to_cpu(*(blk_t*)buf) == blk +1){
		if (check_meta3_block(buf, blk, count+1, 0)){
				retval = M_EXT3_META ;	
				goto out;
		}
	}

	strncpy(text,magic_buffer(cookie_f,buf , size),60);
	strncpy(magic_buf, magic_buffer(cookie , buf , size),60);
/*#ifdef  DEBUG_MAGIC_SCAN
	printf("Scan Result :  %s    %d\n", magic_buf , count+1) ;
	printf("RESULT : %s \n",text);
#endif	*/

	if (!strncmp(text,"data",4)){
		if (count == -1) {
			retval |= M_BLANK;
			goto out;
		}	
	}
	if (strstr(magic_buf,"application/vnd.oasis.opendocument")){
		retval |= (M_APPLI | M_BINARY | M_CLASS_1);
		goto out;
	}


	if((strstr(magic_buf,"text/")) || (strstr(magic_buf,"application/") && (strstr(text,"text")))){
		retval |= M_TXT ;
		if (deep && count && (count < current_fs->blocksize))
			strncpy(magic_buf, magic_buffer(cookie , buf , count+1),60);
	}

	if (strstr(magic_buf,"charset=binary")){
			retval |= M_BINARY ;
	}
	//FIXME test: catch of properties from file-5.04 
        if ((retval & M_TXT) && 
		((retval & M_BINARY) || (strstr(magic_buf,"charset=unknown-8bit") && (count > 8)) ||
		 (strstr(text,"very long lines, with no")))){
		
		 retval	|= M_DATA;
	}
	else{
		if ((strstr(magic_buf,"application/octet-stream")) && (!(strncmp(text,"data",4)))){
			retval |= M_DATA;
		}
	}

/*	
//FIXME only for test
if ((strstr(magic_buf,"application/octet-stream")) && (!(strncmp(text,"text",4))))
	printf("application/octet-stream + text in BlockNR; %ul\n",blk);
*/

	if ((retval & M_DATA) || (*(buf+7) < EXT2_FT_MAX) || (count < 32) ) {
		if (check_meta4_block(buf, blk, count+1)){
				retval = M_EXT4_META ;	
				goto out;
		}
		if (check_dir_block(buf, blk, count+1)){
				retval = M_DIR ;
				goto out;
		}
		if (check_acl_block(buf, blk, count+1)){
				retval = M_ACL ;
				goto out ;
		}
	}

	if (retval & M_DATA)
		goto out;

	if (deep && (retval & M_TXT)){
		char	searchstr[] = "html PGP rtf texmacs vnd.graphviz x-awk x-gawk x-info x-msdos-batch x-nawk x-perl x-php x-shellscript x-texinfo x-tex x-vcard x-xmcd xml ";
		p_search = searchstr;
		while (*p_search){
			len=0;
			while((*p_search) != 0x20){
				token[len] = *p_search;
				p_search++;
				len++;
			}
			token[len] = 0;
			if (strstr(magic_buf,token)){
				//strncpy(magic_buf,text,60);
				retval |= M_CLASS_1;
				break;
			}
			p_search++;
		}
		if (! (retval & M_CLASS_1))
			retval |= M_CLASS_2 ;
		goto out;
	}

	if (strstr(magic_buf,"application/octet-stream")){
		char	searchstr[] = "7-zip cpio CD-ROM MPEG 9660 Targa Kernel boot SQLite OpenOffice.org VMWare3 VMware4 JPEG ART PCX IFF DIF RIFF ATSC ScreamTracker ";
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
				strncpy(magic_buf,text,60);
				retval |= ( M_APPLI | M_ARCHIV | M_CLASS_1 );
				break;
			}
			p_search++;
		}
		if (! (retval & M_APPLI))
			retval |= M_DATA ;
		goto out;
	}
	else {
		if (strstr(magic_buf,"application/")){
			if (strstr(magic_buf,"x-tar"))
				retval |= ( M_APPLI | M_TAR | M_CLASS_1) ;
			else{
				if(strstr(magic_buf,"x-elc") || strstr(magic_buf,"keyring") || strstr(magic_buf,"x-arc"))
					retval = M_DATA;
				else {
					if (strstr(magic_buf,"encrypted") || strstr(magic_buf,"x-tex-tfm"))
						retval |= ( M_APPLI | M_CLASS_2 );
					else
						retval |= ( M_APPLI | M_CLASS_1 );
				}
			}	
			goto out;
		}
		else {
		
			if (strstr(magic_buf,"image/")){
				if (strstr(magic_buf,"x-portable-") && (! isspace( *(buf+2)))){
					retval = M_DATA;
				}
				else 
					retval |= ( M_IMAGE | M_CLASS_1 );
				goto out;
			}
			else {
				if (strstr(magic_buf,"audio/")){
					if (strstr(magic_buf,"x-mp4a-latm") || strstr(magic_buf,"x-hx-aac-adts"))
						retval = M_DATA;
					else {	
						if(strstr(magic_buf,"mpeg"))
							retval |= ( M_AUDIO | M_CLASS_2 );
						else
							retval |= ( M_AUDIO | M_CLASS_1 );
					}
					goto out;
				}
				else{
					if (strstr(magic_buf,"video/")){
						retval |= ( M_VIDEO | M_CLASS_1 );
						goto out;
					}
					else {
						if (strstr(magic_buf,"message/")){
							retval |= ( M_MESSAGE | M_CLASS_2);
							goto out;
						}
						else{
						
							if (strstr(magic_buf,"model/")){
								retval |= ( M_MODEL | M_CLASS_2);
								goto out;
							}
							else{
								if (strstr(magic_buf,"CDF V2 Document")){
									retval |= (M_APPLI | M_ARCHIV | M_CLASS_1 );
									goto out;
								}
							}
						}
					}
				}
			}
		}
	}

out:

#ifdef  DEBUG_MAGIC_SCAN
	printf("BLOCK_SCAN : Block = %010u  ;  0x%08x\n",blk, retval & 0xffffe000);
	blockhex(stdout,buf,0,(count < (64)) ? count  : 64 );
#endif

	retval |= (count+1);
	
	return retval;
}




static struct found_data_t* soft_border(char *des_dir, unsigned char *buf, struct found_data_t* file_data, __u32* follow, blk_t blk ,blk_t* last_rec, __u32 mask){
	if ( check_file_data_end(file_data, buf ,mask )){
		file_data = recover_file_data(des_dir, file_data, follow);
		*last_rec = blk;
	}
	else{ 
		file_data = forget_file_data(file_data, follow);	
	}
return file_data;
}


static int get_range(blk_t* p_blk ,struct ext2fs_struct_loc_generic_bitmap *ds_bmap, unsigned char* buf){
	blk_t  			begin;
	blk_t			end;
	int 			count=1;
	for (begin = *p_blk; begin <= ds_bmap->end ; begin++){
		if((!((begin-1) & 0x7)) && skip_block(&begin, ds_bmap)){

#ifdef  DEBUG_MAGIC_SCAN
			printf("jump to %lu \n",begin);
#endif
		}
		*p_blk = begin;
		if (begin > ds_bmap->end)
			return 0;
		if (ext2fs_test_block_bitmap ( d_bmap, begin) && (! ext2fs_test_block_bitmap( bmap, begin)))
			break;
	}
	
	if (begin > ds_bmap->end)
		return 0;

	for (end = begin,count=1 ; ((count < MAX_RANGE) && (end < ds_bmap->end-1)); ){
		if (ext2fs_test_block_bitmap(d_bmap, end+1) && (! ext2fs_test_block_bitmap( bmap, end+1))){
			end++;
			count++;
		}
		else break;
		
	}
	*(p_blk+1) = end;
	//add the previous block to the end of the buffer
	if (io_channel_read_blk ( current_fs->io,(*p_blk)-1,1,buf - current_fs->blocksize) ||
		(io_channel_read_blk ( current_fs->io, begin , count,  buf ))){
		
		fprintf(stderr,"ERROR: while read block %10u + %d\n",begin,count);
		return 0;
	}
	if (count && count < MAX_RANGE){
		memset(buf+(count * current_fs->blocksize), 255, current_fs->blocksize);
	}	
return count;
}	



static int get_full_range(blk_t* p_blk ,struct ext2fs_struct_loc_generic_bitmap *ds_bmap, unsigned char* buf, blk_t* p_flag){
	blk_t  			begin;
	blk_t			end;
	int 			count=0;
	int			i;
	errcode_t		x;

	for (begin = *p_blk; begin <= ds_bmap->end ; begin++){
		if((!((begin-1) & 0x7)) && skip_block(&begin, ds_bmap)){

#ifdef  DEBUG_MAGIC_SCAN
			printf("jump to %lu \n",begin);
#endif
		}
		*p_blk = begin;
		if (begin > ds_bmap->end)
			return 0;
		if (ext2fs_test_block_bitmap ( d_bmap, begin) && (! ext2fs_test_block_bitmap( bmap, begin)))
			break;
	}
	
	i = 0;
	//add the previous block to the end of the buffer
	if(io_channel_read_blk ( current_fs->io,begin-1,1,buf - current_fs->blocksize)){
		fprintf(stderr,"ERROR: while read block %10lu + %d  %lu\n",begin,i,count-1);
		return 0;
	}

	for (end = begin,count=1 ; ((count <= MAX_RANGE) && (end < ds_bmap->end)); ){
		if (ext2fs_test_block_bitmap(d_bmap, end) && (! ext2fs_test_block_bitmap( bmap, end))){
			count++;
			if (!begin)
				begin = end;
			*p_flag = end++;
			p_flag++;
			i++;
		}
		else { 
			if (i){
				if (io_channel_read_blk ( current_fs->io, begin , i,  buf )){
					fprintf(stderr,"ERROR: while read block %10lu + %d\n",begin,i);
					return 0;
				}
				buf += (current_fs->blocksize *i);
				begin = 0;
				i = 0;
			}
			end++;
		}
	}
	*(p_blk+1) = end;
	if (i){
		if (io_channel_read_blk ( current_fs->io, begin , i,  buf )){
			fprintf(stderr,"ERROR: while read block %10lu + %d  %lu\n",begin,i,count-1);
			return 0;
		}
	}
	
return count-1;
}	



static blk_t block_backward(blk_t blk , int count){
	int 	i=count;
	while (i && blk){
		if (ext2fs_test_block_bitmap(d_bmap, blk) && (! ext2fs_test_block_bitmap( bmap, blk)))
			i--;
		blk--;
	}
	return (!i) ? blk+1 : 0;
}




//main of the magic_scan_engine for ext3
int magic_block_scan3(char* des_dir, __u32 t_after){
magic_t 					cookie = 0;
magic_t						cookie_f = 0;
struct 	ext2fs_struct_loc_generic_bitmap 	*ds_bmap;
struct found_data_t				*file_data = NULL;
blk_t						blk[2] ;
blk_t						j;
blk_t						flag[MAX_RANGE];
blk_t						fragment_flag;
blk_t						last_rec;
unsigned char					*v_buf = NULL;
unsigned char 					*buf ;
char						*magic_buf = NULL;
unsigned char					*tmp_buf = NULL;
int						blocksize, ds_retval,count,i,ret;
__u32						scan,follow, size;


printf("MAGIC-3 : start ext3-magic-scan search. Please wait, this may take a long time\n");
blocksize = current_fs->blocksize ;
count = 0;
blk[0] = 0;
cookie = magic_open(MAGIC_MIME | MAGIC_NO_CHECK_COMPRESS | MAGIC_NO_CHECK_ELF );
cookie_f = magic_open(MAGIC_NO_CHECK_COMPRESS | MAGIC_NO_CHECK_ELF | MAGIC_RAW );
if ((! cookie) ||  magic_load(cookie, NULL) || (! cookie_f) || magic_load(cookie_f, NULL)){
	fprintf(stderr,"ERROR: can't find libmagic\n");
	goto errout;
}
v_buf = malloc(blocksize * (MAX_RANGE+1));
buf = v_buf + blocksize ; 
tmp_buf = malloc(blocksize);
magic_buf = malloc(100);
if ((! v_buf) || (! magic_buf) || (! tmp_buf)){
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
	
	count = 0;
	blk[0] = 1;	
	count = get_range(blk ,ds_bmap, buf);

	while (count){
#ifdef  DEBUG_MAGIC_SCAN
		printf(" %lu    %lu    %d\n", blk[0],blk[1],count);
#endif
		for (i = 0; i< ((count>12) ? MAX_RANGE - 12 : count) ;i++){
			scan = magic_check_block(buf+(i*blocksize), cookie, cookie_f , magic_buf , blocksize * (((count-i) >=9) ? 9 : 1) ,blk[0]+i , 0);
			if(scan & (M_DATA | M_BLANK | M_IS_META | M_TXT)){
				if (scan & (M_ACL | M_EXT4_META | M_DIR))
					ext2fs_mark_generic_bitmap(bmap, blk[0]+i);
				continue;
			}	
#ifdef  DEBUG_MAGIC_SCAN		
			printf("SCAN %lu : %09x : %s\n",blk[0]+i,scan,magic_buf);
#endif
			if (((count -i) > 12) && (ext2fs_le32_to_cpu(*(__u32*)(buf +((i+12)*blocksize))) == blk[0] + i +1 + 12)){
				follow = 0;
				file_data = new_file_data(blk[0]+i,scan,magic_buf,buf+(i*blocksize),&follow);
				for(j=blk[0]+i; j<(blk[0]+i+12);j++)
					 add_file_data(file_data, j, scan ,&follow);
				scan = magic_check_block(buf+((i+12)*blocksize), cookie, cookie_f , magic_buf , blocksize ,blk[0]+i+12, 0);	
				if ((scan & M_EXT3_META) && (check_indirect_meta3( buf+((i+12)*blocksize)))){
					if (add_ext3_file_meta_data(file_data, buf+((i+12)*blocksize), j)){
						io_channel_read_blk (current_fs->io, file_data->last,  1, tmp_buf);
						file_data = soft_border(des_dir, tmp_buf, file_data, &follow, 0 ,&last_rec, 0x7);
						i++ ;
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
			}	
		} 
		blk[0] += (i)?i:1;
		count = get_range(blk ,ds_bmap,buf);
	}
	count = 0;
	blk[0] = 1;
	last_rec = 0;
	fragment_flag = 0;
	count = get_full_range(blk ,ds_bmap, buf,flag);
	while (count){
#ifdef  DEBUG_MAGIC_SCAN
		printf("%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu - %d\n",flag[0],flag[1],flag[2],flag[3],flag[4],flag[5],flag[6],flag[7],flag[8],flag[9],flag[10],flag[11],flag[12],flag[13],flag[14],flag[15],count);
#endif
		for (i = 0; i< ((count>12) ? MAX_RANGE - 12 : count) ;i++){
			follow = 0;
			scan = magic_check_block(buf+(i*blocksize), cookie, cookie_f , magic_buf , blocksize ,flag[i], 1);
			if(scan & (M_DATA | M_BLANK | M_IS_META)){
				if ((!fragment_flag) && (scan & M_EXT3_META) && (check_indirect_meta3(buf+(i*blocksize)))){

					blk[1] = block_backward(flag[i] , 13);
					if (blk[1] && (blk[1] < last_rec) && (blk[1] < (flag[i]-12))){
						blk[0] = blk[1];
						fragment_flag = flag[i];
#ifdef  DEBUG_MAGIC_SCAN
						printf("Try a fragment recover for metablock: %lu at blk: %lu\n",flag[i],blk[1] );
#endif
						goto load_new;
					}
				}
				else
					continue;
			}
#ifdef  DEBUG_MAGIC_SCAN	
			printf("SCAN %lu : %09x : %s\n",flag[i],scan,magic_buf);
#endif
			if (((count -i) > 12) && (ext2fs_le32_to_cpu(*(__u32*)(buf +((i+12)*blocksize))) == flag[12+i]+1)){
				file_data = new_file_data(flag[i],scan,magic_buf,buf+(i*blocksize),&follow);
				for(j=i; j<(12+i);j++)
					add_file_data(file_data, flag[j], scan ,&follow);
				scan = magic_check_block(buf+((i+12)*blocksize), cookie, cookie_f , magic_buf , blocksize ,flag[i+12],0);	
				if (scan & M_EXT3_META){
					if (add_ext3_file_meta_data(file_data, buf+((i+12)*blocksize), flag[j])){
						io_channel_read_blk (current_fs->io, file_data->last,  1, tmp_buf);
						file_data = soft_border(des_dir, tmp_buf, file_data, &follow, flag[i], &last_rec, 0x7);
						i++;
						break;
					}
					else
						file_data = forget_file_data(file_data, &follow);				
				}
				else
					file_data = forget_file_data(file_data, &follow);
			}
			else{ 
				if (scan & M_IS_FILE){
					
					for (j=i; j< 12+i; j++){
						if (!follow){
							file_data = new_file_data(flag[i],scan,magic_buf,buf+(i*blocksize),&follow);
							add_file_data(file_data, flag[i], scan ,&follow);
						}
						else{
							scan = magic_check_block(buf+(j*blocksize), cookie, cookie_f , magic_buf , blocksize ,flag[j] , 0);
							if ( check_file_data_possible(file_data, scan ,buf+(j*blocksize))){
								add_file_data(file_data, flag[j], scan ,&follow);
							}
							else{	
								file_data = soft_border(des_dir,buf+((j-1)*blocksize), file_data, &follow, flag[i], &last_rec,0x7); 
								 if ((!fragment_flag) && (scan & M_EXT3_META) && (check_indirect_meta3(buf+(j*blocksize)))){
									blk[1] = block_backward(flag[j] , 13);
									if (blk[1] && (blk[1] < last_rec) && (blk[1] < flag[j]-12)){
										blk[0] = blk[1];
										fragment_flag = flag[j];
#ifdef  DEBUG_MAGIC_SCAN
										printf("Try fragment recover for metablock: %lu at  blk: %lu\n",flag[j], blk[0]);
#endif
										goto load_new;
									}
								}
								break;
							}

						}
						size = (scan & M_SIZE ); //get_block_len(buf+(i*blocksize));
						ret = file_data->func(buf+(j*blocksize), &size ,scan ,0 , file_data);
						if (ret == 1){
							 if (file_data_correct_size(file_data,size)){
								file_data = recover_file_data(des_dir, file_data, &follow);
								last_rec = flag[i];
							}
							else{ 
								file_data = forget_file_data(file_data, &follow);
#ifdef  DEBUG_MAGIC_SCAN
								printf("Don't recover this file, current block %lu \n",flag[i]);
#endif
							}
							break;
						}
					}
					if (follow){
#ifdef  DEBUG_MAGIC_SCAN
						printf("stop no file-end\n");
#endif
						file_data = soft_border(des_dir,buf+((j-1)*blocksize), file_data, &follow, flag[i], &last_rec, 0x3);
						i = j - 12;
					}
					else
						i = j;
				}	
				
			}		
		}
		blk[0]+=(i)?i:1;
		if (fragment_flag && (last_rec > fragment_flag))
			fragment_flag = 0;
load_new :
		count = get_full_range(blk ,ds_bmap, buf,flag);
		
	}

	}//operation
} //while transactions


errout:
clear_block_bitmap_list(d_bmap);
if (file_data) free_file_data(file_data);
if (v_buf) free(v_buf);
if (tmp_buf) free(tmp_buf);
if (magic_buf) free(magic_buf);
if (cookie) magic_close(cookie); 
if (cookie_f) magic_close(cookie_f);
} //end



// obsolete and no activ in this version  ---------------------------------------------------------------------------------
//main of the magic_scan_engine for ext4
/*int magic_block_scan4(char* des_dir, __u32 t_after){
magic_t 					cookie = 0;
magic_t						cookie_f = 0;
struct ext3_extent_header			*p_extent_header;
struct ext3_extent				*p_extent;
struct ext3_extent_idx				*p_extent_idx;
struct 	ext2fs_struct_loc_generic_bitmap 	*ds_bmap;
struct found_data_t				*file_data = NULL;
blk_t						blk[2] ;
blk_t						first_b;
blk_t						last_rec;
unsigned char					*v_buf = NULL;
unsigned char 					*buf ;
char						*magic_buf = NULL;
unsigned char					*tmp_buf = NULL;
int						blocksize, ds_retval,count,i,ret;
__u32						scan,follow;//, size;


printf("MAGIC-3 : start ext4-magic-scan search. Experimental in develop \n");
blocksize = current_fs->blocksize ;
count = 0;
blk[0] = 0;
cookie = magic_open(MAGIC_MIME | MAGIC_NO_CHECK_COMPRESS | MAGIC_NO_CHECK_ELF );
cookie_f = magic_open(MAGIC_NO_CHECK_COMPRESS | MAGIC_NO_CHECK_ELF | MAGIC_RAW );
if ((! cookie) ||  magic_load(cookie, NULL) || (! cookie_f) || magic_load(cookie_f, NULL)){
	fprintf(stderr,"ERROR: can't find libmagic\n");
	goto errout;
}
v_buf = malloc(blocksize * (MAX_RANGE+1));
buf = v_buf + blocksize ; 
tmp_buf = malloc(blocksize);
magic_buf = malloc(100);
if ((! v_buf) || (! magic_buf) || (! tmp_buf)){
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
	
	count = 0;
	blk[0] = 0;
	follow = 0;	
	count = get_range(blk ,ds_bmap, buf);

	while (count){
#ifdef  DEBUG_MAGIC_SCAN
		printf(" %lu    %lu    %d\n", blk[0],blk[1],count);
#endif
		for (i=0 ; i< count; i++){
//			printf(">>>>> %lu \n",blk[0]+i);	
		
			if ((check_dir_block(buf +(i*blocksize), blk[0]+i,1)) ||
				(check_acl_block(buf+(i*blocksize), blk[0]+i,1))){
				ext2fs_mark_generic_bitmap(bmap, blk[0]+i);
//				printf ("<<> %lu    DIR-BLOCK \n",blk[0]+i);
				continue;
			}
			if (check_meta4_block(buf+(i*blocksize), blk[0]+i,1)){
//				printf ("<<> %lu    IDX  \n",blk[0]+i);	
				p_extent_header=(struct ext3_extent_header*)(buf+(i*blocksize));
				if (! ext2fs_le16_to_cpu(p_extent_header->eh_depth)){ //FIXME   Indextiefe
					p_extent=(struct ext3_extent*)(p_extent_header +1) ;
					if (! ext2fs_le32_to_cpu(p_extent->ee_block)) { //FIXME   Fileanfang
						first_b = ext2fs_le32_to_cpu(p_extent->ee_start);
						if(( ext2fs_test_block_bitmap( bmap,first_b)) || 
							(io_channel_read_blk (current_fs->io,first_b,1,tmp_buf ))){
								fprintf(stderr,"ERROR: while read block %10lu\n",first_b);
						}
						else{
							scan = magic_check_block(tmp_buf, cookie, cookie_f , magic_buf , blocksize ,first_b, 1);
							file_data = new_file_data(first_b,scan,magic_buf,tmp_buf,&follow);
							if ((file_data) && 
								(add_ext4_extent_idx_data(file_data, (void*) p_extent_header, blk[0]+i))){
								io_channel_read_blk (current_fs->io, file_data->last,  1, tmp_buf);
								file_data = soft_border(des_dir, tmp_buf, file_data, &follow, 0 ,&last_rec, 0x7);
							}
						}
					}
				}
			blk[0] = blk[0] - count + i;
			i = count;		
			} //ext4meta

		} //for i
		blk[0] += (i)?i:1;
		count = get_range(blk ,ds_bmap,buf);
	} //count
	} //ds_bmap
}//ds_retval

errout:
if (v_buf) free(v_buf);
if (tmp_buf) free(tmp_buf);
if (magic_buf) free(magic_buf);
if (cookie) magic_close(cookie); 
if (cookie_f) magic_close(cookie_f);
}//funcion*/
//--------END----------
