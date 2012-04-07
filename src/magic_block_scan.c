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
#include "extent_db.h"

//#define DEBUG_MAGIC_SCAN

extern ext2_filsys 		current_fs ;
ext2fs_block_bitmap 		d_bmap = NULL ; 


static __u32 get_block_len(char *buf){
	int		len = current_fs->blocksize -1;

	while ((len >= 0) && (!(*(buf + len)))) 
			len--;
	return ++len;
}



struct found_data_t* free_file_data(struct found_data_t* old){
	int tmp;

	if(old->inode) free(old->inode);
	if(old->scan_result) free(old->scan_result);
	if(old->name)  free(old->name);
	if(old->priv){
		old->func(NULL,&tmp,0,4,old);
		old->priv = NULL;
	}		
	free(old);
	return NULL;
}




static struct found_data_t* copy_file_data(struct found_data_t* old){
	static struct found_data_t* 	new = NULL;
	int 				str_len = strlen(old->scan_result)+1;
	int				name_len = strlen(old->name)+1;

//FIXME private
	new = malloc(sizeof(struct found_data_t));
	if (!new) return NULL;
	memcpy(new,old,sizeof(struct found_data_t));
	new->scan_result = malloc(str_len);
	new->name	 = malloc(name_len);
	new->inode	 = new_inode();
	if(old->priv){
		 new->priv = malloc(old->priv_len);
		 if (new->priv)
			memcpy(new->priv,old->priv,old->priv_len);
		 else
			new->priv_len = 0;
	}
	if((!new->scan_result) || (!new->name) || (!new->inode)){
		if (new->priv){
			free (new->priv);
			new->priv = NULL;
		}
		new = free_file_data(new);
		fprintf(stderr,"ERROR; while allocate memory for found copy file struct\n");
	}
	else{
		memcpy(new->scan_result,old->scan_result,str_len);
		memcpy(new->name,old->name,name_len);
		memcpy(new->inode,old->inode,128);
	}
	return new;	
}




static struct found_data_t* new_file_data(blk_t blk,__u32 scan,char *magic_buf, unsigned char* buf, __u32 *f, __u32 buf_length){
	struct found_data_t 		*new;
	int				str_len;
	int				name_len;
	char				*c;
	int				def_len = 20;
	char				name_str[20];

	new = malloc(sizeof(struct found_data_t));
	if (!new) return NULL;

	new->first = blk;
	new->last  = blk;
	new->buf_length  =  buf_length;
	new->next_offset = 0;
	new->last_match_blk = 0;
	new->scantype = H_F_Carving;
	new->scan  = scan;
	new->size  = 0;
	new->h_size = 0;
	new->priv_len = 0;
	new->func = NULL;
	new->priv = NULL;
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
		sprintf(name_str,"/%010lu",(long unsigned int)blk);
		strcat(new->name,name_str);
		get_file_property(new);
		new->func(buf,&name_len,scan,2,new);
	}
	if ((!new->func) || (!new->first)) {
		*f = 0;
		return free_file_data(new);
	}
}
*f++;		//"gcc warning: value computed is not used" warning is okay  

return new;
}




static struct found_data_t* recover_file_data(char *des_dir, struct found_data_t* this, __u32 *follow){
	recover_file(des_dir,"MAGIC-3",this->name,(struct ext2_inode*)this->inode, 0 , 1);
	*follow = 0;
	return free_file_data(this);
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
        if (inode_add_block(this->inode , blk )){
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
		//correcting small ext3 inodes 
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

	size = get_block_len((char*)buf);
	ret = (this->func(buf, &size ,this->scan ,0 , this) & mask) ;

	if (ret) 			
           ret = file_data_correct_size(this,size);
	return (mask & FORCE_RECOVER) ? 1 : ret ;
}




static int check_file_data_possible(struct found_data_t* this, __u32 scan ,unsigned char *buf){
	int size ;
	size = (scan & M_SIZE );
	return this->func(buf, &size ,scan ,1 , this);
}




static int check_meta3_block(char *block_buf, blk_t blk, __u32 size, int flag){
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
	return 0; //never used
}




static int check_indirect_meta3(char *block_buf){
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



static int check_dindirect_meta3(char * block_buf){
	__u32			*p_blk;
	__u32			block;
	char			*buf = NULL;
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



static int check_tindirect_meta3(char * block_buf){
	__u32			*p_blk;
	__u32			block;
	char  			*buf = NULL;
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
return  (ext2fs_extent_header_verify((void*)block_buf, current_fs->blocksize)) ? 0 : 1;
/*
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
	return 1 ;*/
}




static int check_dir_block(unsigned char *block_buf, blk_t blk, __u32 size){
	struct ext2_dir_entry_2		*dir_entry;
	ext2_ino_t			inode_nr;
	__u16				len;
	__u16				p_len;
	int 				i;
	
	dir_entry = (struct ext2_dir_entry_2*)block_buf;
	
	inode_nr = ext2fs_le32_to_cpu(dir_entry->inode);
	if ((inode_nr && (inode_nr < EXT2_GOOD_OLD_FIRST_INO)) || (inode_nr > current_fs->super->s_inodes_count))
		return 0;
	len = ext2fs_le16_to_cpu(dir_entry->rec_len);
	if ((len < 8) || (len % 4) || (len> current_fs->blocksize) ||(len < (((unsigned)dir_entry->name_len + 11) & ~3 ))||(!dir_entry->name_len) || (! dir_entry->file_type) || (dir_entry->file_type & 0xf8)) 
		return 0;
	
	for (i=0; i<dir_entry->name_len;i++)
		if(!dir_entry->name[i])
			return 0;
	if((dir_entry->name_len == 1) && ((dir_entry->name[0] < 33) || (dir_entry->name[0] > 126)))
		return 0;
loop:
	p_len =len;
	if (len < current_fs->blocksize - 12){
		dir_entry = (struct ext2_dir_entry_2*) (((void*)dir_entry) + len);
		len = ext2fs_le16_to_cpu(dir_entry->rec_len);
		if ((len < 8) || (len % 4) || ((len+p_len) > current_fs->blocksize) ||(len < (((unsigned)dir_entry->name_len + 11) & ~3 ))||(!dir_entry->name_len) || (! dir_entry->file_type) || (dir_entry->file_type & 0xf8) ) 
			return 0;
		if ((dir_entry->name_len & 0x01) &&  dir_entry->name[(unsigned)dir_entry->name_len])
			goto loop;

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



static int add_ext4_extent_idx_data(struct found_data_t* this, struct extent_area* ea){
	return inode_add_extent(this->inode ,ea, &(this->last), 1);
}


//for future use ; gcc warning is okay
static int add_ext4_extent_data(struct found_data_t* this, struct extent_area* ea){
	return inode_add_extent(this->inode ,ea, &(this->last), 0);
}



static int add_ext3_file_meta_data(struct found_data_t* this, char *buf, blk_t blk){
	blk_t   next_meta = 0;
	blk_t	meta = 0;
	blk_t	last_data = 0;
	int 	ret = 0;
	

	if (check_indirect_meta3(buf)){
		ret = inode_add_meta_block(this->inode , blk, &last_data, &next_meta, buf );
		this->last = last_data;
		this->scantype |= DATA_METABLOCK;
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
	int 		diff = (current_fs->blocksize == 1024)?1:0;

	o_blk = (*p_blk) >> 3;
	p_bmap = (struct ext2fs_struct_loc_generic_bitmap *) bmap;
	while (((! *(ds_bmap->bitmap + o_blk)) || (*(p_bmap->bitmap + o_blk) == (unsigned char)0xff) || 
		(*(ds_bmap->bitmap + o_blk) == *(p_bmap->bitmap + o_blk))) && (o_blk < (ds_bmap->end >> 3))){  
		o_blk ++;
		flag = 1;
	}
	*p_blk = (o_blk << 3) + diff ;
						
return flag;
} 


static int is_ecryptfs(unsigned char* buf){
	int ret = 0;
	if ((!buf[0]) && (!buf[20]) && (!buf[21]) && (buf[22]) && (!buf[23]) && (!buf[24]) &&  
	    ((buf[8] ^ buf[12]) == 0x3c) && ((buf[9] ^ buf[13]) == 0x81) && ((buf[10] ^ buf[14]) == 0xb7) && ((buf[11] ^ buf[15]) == 0xf5))
		ret =1;
	return ret;
}


//magic scanner 
//FIXME 
static int magic_check_block(unsigned char* buf,magic_t cookie , magic_t cookie_f, char *magic_buf, __u32 size, blk_t blk, int deep){
	int	count = current_fs->blocksize -1 ;
	int	len;
	char	text[100] = "";
	char 	*p_search;
	__u32	retval = 0;
	char	token[20]; 
	
	memset(magic_buf,0,100); //FIXME
	memset(text,0,100);
	while ((count >= 0) && (*(buf+count) == 0)) count-- ;
	if (ext2fs_le32_to_cpu(*(blk_t*)buf) == blk +1){
		if (check_meta3_block((char*)buf, blk, count+1, 0)){
				retval = M_EXT3_META ;	
				goto out;
		}
	}

	if (size > current_fs->blocksize){
		strncpy(text,magic_buffer(cookie_f, buf, 512),60);
		if ((!strncmp(text,"data",4))|| (!strncmp((char*)buf,"ID3",3))){
			strncpy(text,magic_buffer(cookie_f,buf , size),60);
			strncpy(magic_buf, magic_buffer(cookie , buf , size),60);
		}
		else{
			strncpy(magic_buf, magic_buffer(cookie , buf , 512),60);
		}
	}
	else{
		strncpy(text,magic_buffer(cookie_f,buf , size),60);
		strncpy(magic_buf, magic_buffer(cookie , buf , size),60);
	}

	if (!strncmp(text,"data",4)){
		if (count == -1) {
			retval |= M_BLANK;
			goto out;
		}	
	}

	if((strstr(magic_buf,"text/")) || (strstr(magic_buf,"application/") && (strstr(text,"text")))){
		retval |= M_TXT ;
		if (deep && count && (count > 60))// current_fs->blocksize))
			strncpy(magic_buf, magic_buffer(cookie , buf , count-1),60);
	}
//loop:
	if ((strstr(magic_buf,"text/plain"))||(strstr(magic_buf,"text/html"))){
		char 	*type;
		char	searchstr[] = "bin/perl python PEM SGML OpenSSH libtool M3U Tcl=script Perl=POD module=source PPD make=config bin/make awk bin/ruby bin/sed bin/expect bash ";
		char	searchtype[] = "text/x-perl text/x-python text/PEM text/SGML text/ssh text/libtool text/M3U text/tcl text/POD text/x-perl text/PPD text/configure text/x-makefile text/x-awk text/ruby text/sed text/expect text/x-shellscript ";
		if (deep  && (count > 250) && (!strncmp((char*)buf,"From ",5))){
			p_search = (char*)buf + 6;
			for (len = 0; (len < (count -7)) ; len++){
				if( *(p_search++) == 0x40)
					break;
			}
			for (;len<(count -7); len++){
				if( *(p_search++) == 0x0a)
					break;
			}
			if ((len < (count - 180)) && ((!strncmp(p_search,"From:",5))||(!strncmp(p_search,"Return-Path:",12)))){
				strncpy(magic_buf,"message/rfc822",15);
				retval = M_TXT | M_MESSAGE | M_CLASS_1 ;
				goto out;
			}
		} 
		if(!(strstr(magic_buf,"text/html"))){
			if (deep){
//FIXME
				p_search = searchstr;
				type = searchtype;
				while (*p_search){
					len=0;
					while((*p_search) != 0x20){
						token[len] = ((*p_search)==0x3d)? 0x20 : (*p_search);
						p_search++;
						len++;
					}
					token[len] = 0;
					if (strstr(text,token)){
						len = 0;
						while (type[len] != 0x20){
							magic_buf[len] = type[len];
							len++;
						}
						magic_buf[len] = 0;
						retval = (M_TXT | M_CLASS_1);
						goto out;
					}
					p_search++;
					while (type[0] != 0x20)
						type++;
					if (*type)
						type++;
				}		

				if((count < (current_fs->blocksize -2)) || (!buf[-1])){
					p_search = (char*)buf + 6;
					for (len = 0; len < 20 ; len++){
						if((*p_search ==0x0a) || (*(p_search++) == 0x20))
							break;
					}
					if (len < 20){
						for (len = 6; len < 80 ; len++)
							if( buf[len] == 0x0a)
								break;
						if (len <80){
							retval = M_TXT | M_CLASS_2 ;
							goto out;
						}
					}
				}
			}
			retval |= (M_DATA | M_TXT) ;
			goto out;
		}
	}
	
	if (strstr(magic_buf,"application/vnd.oasis.opendocument")){
		retval |= (M_APPLI | M_BINARY | M_CLASS_1);
		goto out;
	}


	if (strstr(magic_buf,"charset=binary")){
			retval |= M_BINARY ;
	}
	//FIXME test: catch of properties from file-5.04 
        if ((retval & M_TXT) && 
		((retval & M_BINARY) ||// (strstr(magic_buf,"charset=unknown-8bit") && (count > 8)) ||
		 (strstr(text,"very long lines, with no")))){
		
		 retval	|= M_DATA;
	}
	else{
		if ((strstr(magic_buf,"application/octet-stream")) && (!(strncmp(text,"data",4)))){
			if (is_ecryptfs (buf))
				strncpy(text,"ecryptfs ",9);
			else
				retval |= M_DATA;
		}
	}

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
		char	searchstr[] = "7-zip cpio CD-ROM MPEG 9660 Targa Kernel boot SQLite OpenOffice.org VMWare3 VMware4 JPEG ART PCX IFF DIF RIFF ATSC ScreamTracker matroska LZMA Audio=Visual Sample=Vision ISO=Media ext2 ext3 ext4 LUKS python ESRI=Shape ecryptfs ";
		p_search = searchstr;
		while (*p_search){
			len=0;
			while((*p_search) != 0x20){
				token[len] = ((*p_search)==0x3d)? 0x20 : (*p_search);
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
				if((!strstr(magic_buf,"x-archive")) &&((strstr(magic_buf,"x-elc") || strstr(magic_buf,"keyring") || strstr(magic_buf,"x-arc")||strstr(magic_buf,"keystore")||strstr(magic_buf,"x-123")||
				strstr(magic_buf,"fontobject")))){ //FIXME fontobject
					retval = M_DATA;
				}
				else {
					if (strstr(magic_buf,"encrypted") || strstr(magic_buf,"x-tex-tfm") || strstr(magic_buf,"x-compress"))
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
							retval |= ( M_MESSAGE | M_CLASS_1);
							goto out;
						}
						else{
						
							if (strstr(magic_buf,"model/")){
								retval |= ( M_MODEL | M_CLASS_2);
								goto out;
							}
							else{
								if (strstr(magic_buf," V2 Document")){
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



static __u32 check_next_border(struct ext2fs_struct_loc_generic_bitmap *ds_bmap, blk_t  blk, __u32 count){
	__u32 	ret = 0;
	__u32	i = 0;

	while ((i < count) && (! ret) && ((blk + i) <  ds_bmap->end)){
		ret = (ext2fs_test_block_bitmap(d_bmap, blk + i) && (! ext2fs_test_block_bitmap(bmap, blk +i)))? 0 :1 ;
		i++;
	}
	return (ret) ? i-1 : 0 ;
}



static int get_range(blk_t* p_blk ,struct ext2fs_struct_loc_generic_bitmap *ds_bmap, unsigned char* buf, int *flag){
	blk_t  			begin;
	blk_t			end;
	int 			count=1;
	int 			diff = (current_fs->blocksize == 1024)?1:0;
	
	for (begin = *p_blk; begin <= ds_bmap->end ; begin++){
		if((!((begin-diff) & 0x7)) && skip_block(&begin, ds_bmap)){
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
		*flag = 1;
	}else 
		*flag = (ext2fs_test_block_bitmap(d_bmap, end+1) && (! ext2fs_test_block_bitmap( bmap, end+1)))? 0 :1 ;
return count;
}	



static int get_full_range(blk_t* p_blk ,struct ext2fs_struct_loc_generic_bitmap *ds_bmap, unsigned char* buf, blk_t* p_flag){
	blk_t  			begin;
	blk_t			end;
	int 			count=0;
	int			i;
	int 			diff = (current_fs->blocksize == 1024)?1:0;

	for (begin = *p_blk; begin <= ds_bmap->end ; begin++){
		if((!((begin - diff) & 0x7)) && skip_block(&begin, ds_bmap)){
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
		fprintf(stderr,"ERROR: while read block %10lu + %d  %d\n",(long unsigned int)begin,i,count-1);
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
					fprintf(stderr,"ERROR: while read block %10lu + %d\n",(long unsigned int)begin,i);
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
			fprintf(stderr,"ERROR: while read block %10lu + %d  %d\n",(long unsigned int)begin,i,count-1);
			return 0;
		}
	}
	
return count-1;
}	


//FIXME 
//Attention : Changes have a very strong effect of the magic-fuction result
static int check_plain_passing(unsigned char * buf){
	int 			ret = 0;
	unsigned char 		*last = buf-1;
	unsigned char		*next = buf;
	int 			i,j, count1,count2;
	if ((!(*last)) || (!(buf[current_fs->blocksize-1]))) 
		return 0;  //NULL at end
	last = buf -32;
	count1 = count2 = 0;
	for (i = 0; i< 32; i++){
		for (j = 0; j<32; j++){
			if (last[i] == next[j]){
				count1++;
				break;
			}
		}
		for (j = 0; j<32; j++){
			if (next[i] == last[j]){
				count2++;
				break;
			}

		}
	}
	if ((count1 > 7 ) && (count2 > 7 ))
		ret = 1; //many duplicate


	count1 = count2 = 0;
	for (i = 0; i< 31; i++){
		if( last[i] == last[i+1]) count1++;
		if( next[i] == next[i+1]) count2++;
	}	
	if ((count1 > 10) && (count2 > 10))
		ret += 2; //many repetitions


	count1 = count2 = 0;
	for (i = 0; i< 31; i++){
		for (j = i+1; j<32; j++){
			if (last[i] == last[j]) count1++;
			if (next[i] == next[j]) count2++;
		}
	}
	if ((count1 < 5) && (count2 < 5)) 
		ret += 4; //very different

   return ret;
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
//FIXME  Disabled (not tested and can't work by  version)
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
int						blocksize, ds_retval,count,i,ret,dummy, size;
__u32						scan,follow;


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
memset(magic_buf,0,100);
ds_retval = init_block_bitmap_list(&d_bmap, t_after);
while (ds_retval){
	ds_retval = next_block_bitmap(d_bmap);
	if (ds_retval == 2 ) 
		continue;

	if (d_bmap && ds_retval ){
		ds_bmap = (struct ext2fs_struct_loc_generic_bitmap *) d_bmap;
	
	count = 0;
	blk[0] = 1;	
	count = get_range(blk ,ds_bmap, buf, &dummy);

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
				file_data = new_file_data(blk[0]+i,scan,magic_buf,buf+(i*blocksize),&follow, count - i);
				for(j=blk[0]+i; j<(blk[0]+i+12);j++)
					 add_file_data(file_data, j, scan ,&follow);
				scan = magic_check_block(buf+((i+12)*blocksize), cookie, cookie_f , magic_buf , blocksize ,blk[0]+i+12, 0);	
				if ((scan & M_EXT3_META) && (check_indirect_meta3((char*)buf+((i+12)*blocksize)))){
					if (add_ext3_file_meta_data(file_data, (char*)buf+((i+12)*blocksize), j)){
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
		count = get_range(blk ,ds_bmap,buf, &dummy);
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
				if ((!fragment_flag) && (scan & M_EXT3_META) && (check_indirect_meta3((char*)buf+(i*blocksize)))){

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
				file_data = new_file_data(flag[i],scan,magic_buf,buf+(i*blocksize),&follow,count - i);
				for(j=i; j<(12+i);j++)
					add_file_data(file_data, flag[j], scan ,&follow);
				scan = magic_check_block(buf+((i+12)*blocksize), cookie, cookie_f , magic_buf , blocksize ,flag[i+12],0);	
				if (scan & M_EXT3_META){
					if (add_ext3_file_meta_data(file_data, (char*)buf+((i+12)*blocksize), flag[j])){
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
							file_data = new_file_data(flag[i],scan,magic_buf,buf+(i*blocksize),&follow,count - i);
							add_file_data(file_data, flag[i], scan ,&follow);
						}
						else{
							scan = magic_check_block(buf+(j*blocksize), cookie, cookie_f , magic_buf , blocksize ,flag[j] , 0);
							if ( check_file_data_possible(file_data, scan ,buf+(j*blocksize))){
								add_file_data(file_data, flag[j], scan ,&follow);
							}
							else{	
								j--;
								file_data = soft_border(des_dir,buf+(j*blocksize), file_data, &follow, flag[j], &last_rec,0x7); 
								 if ((!fragment_flag) && (scan & M_EXT3_META) && (check_indirect_meta3((char*)buf+((j+1)*blocksize)))){
									blk[1] = block_backward(flag[j] , 12);
									if (blk[1] && (blk[1] < last_rec) && (blk[1] < flag[j]-12)){
										blk[0] = blk[1];
										fragment_flag = flag[j+1];
#ifdef  DEBUG_MAGIC_SCAN
										printf("Try fragment recover for metablock: %lu at  blk: %lu\n",flag[j+1], blk[0]);
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
		if (fragment_flag && (blk[0] > fragment_flag))
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
return 0;
} //end



static int check_extent_len(struct ext3_extent_header *header, struct extent_area* ea, blk_t blocknr ){
	void 				*buf = NULL;
	int 				ret = 0;
	struct ext3_extent_idx		*idx;
	struct ext3_extent 		*extent;
	int				entry,i;
	
	ret = ext2fs_extent_header_verify((void*) header, current_fs->blocksize);
	if(!ret){
		ea->b_count++;
		for (entry =1; ((!ret) && (entry <= ext2fs_le16_to_cpu(header->eh_entries))) ; entry++){
 			if (ext2fs_le16_to_cpu(header->eh_depth)){
				idx = (struct ext3_extent_idx*) (header + entry);
				buf=malloc(current_fs->blocksize);
				if (!buf)
					return 1;
				if(io_channel_read_blk ( current_fs->io,ext2fs_le32_to_cpu(idx->ei_leaf), 1, buf )){
					fprintf(stderr,"Error read block %lu\n",(long unsigned int)ext2fs_le32_to_cpu(idx->ei_leaf));
					ret = 2;
				}
				else{
					//Recursion
					ret = check_extent_len((struct ext3_extent_header*)buf, ea , 0);
				}
				if (buf){ free(buf); buf = NULL ;}
			}
			else{
				extent  = (struct ext3_extent*)(header + entry);
				if ((ext2fs_le16_to_cpu(extent->ee_len)) & 0x8000){
					continue;
				}
				for (i = 0; ((! ret) && (i< ext2fs_le16_to_cpu(extent->ee_len))); i++){
					if (ext2fs_test_block_bitmap(bmap,ext2fs_le32_to_cpu(extent->ee_start)+i) &&
					    (!ext2fs_test_block_bitmap(d_bmap,ext2fs_le32_to_cpu(extent->ee_start)+i)))
						ret = 4;
				}
				if (!ret){
					if ((entry == 1) && (!ea->start_b)){
						ea->start_b = ext2fs_le32_to_cpu(extent->ee_start);
						ea->l_start = ext2fs_le32_to_cpu(extent->ee_block);
					}
//					if (entry == ext2fs_le16_to_cpu(header->eh_entries)){
						ea->end_b = ext2fs_le32_to_cpu(extent->ee_start) + ext2fs_le16_to_cpu(extent->ee_len)-1;
						ea->l_end = ext2fs_le32_to_cpu(extent->ee_block) + ext2fs_le16_to_cpu(extent->ee_len)-1;
//					}
					ea->b_count += ext2fs_le16_to_cpu(extent->ee_len);
					ea->size = ((unsigned long long) ((ext2fs_le32_to_cpu(extent->ee_block)) + 
							 (ext2fs_le16_to_cpu(extent->ee_len)))) * current_fs->blocksize;
				}
			}
		}
		if ((blocknr) && (!ret)){
			ea->blocknr = blocknr;
			ea->depth = ext2fs_le16_to_cpu(header->eh_depth);
		}
	}
return ret;
}



int recover_db(struct extent_db_t *db , struct extent_area *ea_group){
	int 			flag = 0;
	int			count = 0;
	__u32			start;
	struct extent_area	*ea = ea_group ;
	memset (ea_group, 0 ,sizeof(struct extent_area) *4);

	start = 0;
	while ((!flag) && (count < 4)) {	
		start = extentd_db_find(db, (start) ? start+1 : start , ea); 
		if (start){
			count++;
//			printf(" --> %d ; start:  %lu    end:  %lu  depth:  %lu \n",count, ea->l_start, ea->l_end, ea->depth);
			ea++;
		}
		else flag++;
	}

return count;
}


//FIXME   NEW ---------------------------------------------------------------------------------------------------------
//main of the magic_scan_engine for ext4
int magic_block_scan4(char* des_dir, __u32 t_after){
magic_t 					cookie = 0;
magic_t						cookie_f = 0;
struct 	ext2fs_struct_loc_generic_bitmap 	*ds_bmap = NULL;
ext2fs_block_bitmap				c_bmap = NULL;
struct extent_db_t				*db = NULL;
struct found_data_t				*file_data = NULL;
struct found_data_t				*tmp_file_data = NULL;
struct extent_area				*ea = NULL;
struct extent_area				ea_group[4];
blk_t						blk[2] ;
blk_t						first_b;
blk_t						last_rec;
unsigned char					*v_buf = NULL;
unsigned char 					*buf ;
char						*magic_buf = NULL;
unsigned char					*tmp_buf = NULL;
int						blocksize, size, ds_retval,count,i,j,ret,dummy, border;
__u32						scan,follow;
int						tes ; //typical extent size


//only for test
//int zahler = 0;
//int zahler1 =0;


printf("MAGIC-3 : start ext4-magic-scan search\n");
blocksize = current_fs->blocksize ;
tes = 1048576 / blocksize;
if (ext2fs_copy_bitmap(current_fs->block_map, &c_bmap)){
		fprintf(stderr,"Error: while duplicate bitmap\n");
		return 0;
}
ext2fs_clear_block_bitmap(c_bmap);

scan = 0;
count = 0;
blk[0] = 1;
cookie = magic_open(MAGIC_MIME | MAGIC_NO_CHECK_COMPRESS | MAGIC_NO_CHECK_ELF | MAGIC_CONTINUE);
cookie_f = magic_open(MAGIC_NO_CHECK_COMPRESS | MAGIC_NO_CHECK_ELF | MAGIC_RAW );

if ((! cookie) ||  magic_load(cookie, NULL) || (! cookie_f) || magic_load(cookie_f, NULL)){
	fprintf(stderr,"ERROR: can't find libmagic\n");
	goto errout;
}
v_buf = malloc(blocksize * (MAX_RANGE+1));
buf = v_buf + blocksize ; 
tmp_buf = malloc(blocksize * 9);
magic_buf = malloc(100);
if ((! v_buf) || (! magic_buf)  || (! tmp_buf)){
	fprintf(stderr,"ERROR: can't allocate memory\n");
	goto errout;
}

ds_retval = init_block_bitmap_list(&d_bmap, t_after);
while (ds_retval){
	ds_retval = next_block_bitmap(d_bmap);
	if (ds_retval == 2 ) 
		continue;
	db = extent_db_init(d_bmap);
	if (d_bmap && ds_retval && db ){
		ds_bmap = (struct ext2fs_struct_loc_generic_bitmap *) d_bmap;
	
	last_rec = 0;
	count = 0;
	blk[0] = 1;
	follow = 0;	
	count = get_range(blk ,ds_bmap, buf, &dummy);

	while (count){
#ifdef  DEBUG_MAGIC_SCAN
		printf(" %lu    %lu    %d\n", blk[0],blk[1],count);
#endif
		for (i=0 ; i< count; i++){
//			printf(">>>>> %lu \n",blk[0]+i);	
		
			if ((check_dir_block(buf +(i*blocksize), blk[0]+i,1)) ||
				(check_acl_block(buf+(i*blocksize), blk[0]+i,1))){
				ext2fs_mark_generic_bitmap(bmap, blk[0]+i);
//				printf ("<<> %lu  DIR-BLOCK \n",blk[0]+i);
				continue;
			}
			if (check_plain_passing((buf +(i*blocksize))) && ((blk[0]+i-((blocksize==1024)?1:0)) % tes ) )
				ext2fs_mark_generic_bitmap(c_bmap, blk[0]+i);
//zahler++;
			if (check_meta4_block(buf+(i*blocksize), blk[0]+i,1)){
//				printf ("<<> %lu  IDX  \n",blk[0]+i);
				ea = new_extent_area();	
				if ( ea ){
					if (check_extent_len((struct ext3_extent_header*) (buf+(i*blocksize)) , ea, blk[0]+i)){
						printf("extent %lu range allocated or damage\n",(long unsigned int)blk[0]+i);
					}
					else{
//						printf(" --> start: %lu    end:  %lu  depth: %lu \n",ea->l_start, ea->l_end, ea->depth);
						if (!ea->l_start){
							first_b = ea->start_b;
							if(( ext2fs_test_block_bitmap( bmap,first_b)) || 
								(io_channel_read_blk (current_fs->io,first_b - 1,9,tmp_buf ))){
								fprintf(stderr,"Warning: block %10lu can not read or is allocated\n",(long unsigned int)first_b);
//FIXME ERROR goto ?
							}
							else{
								scan = magic_check_block(tmp_buf+blocksize, cookie, cookie_f , magic_buf , blocksize*8 ,first_b, 1);
								file_data = new_file_data(first_b,scan,magic_buf,tmp_buf+blocksize,&follow, 8);
								if ((file_data) && 
									(add_ext4_extent_idx_data(file_data, ea))){
									file_data->scantype |= DATA_METABLOCK;
									io_channel_read_blk (current_fs->io, file_data->last,  1, tmp_buf);
									file_data = soft_border(des_dir, tmp_buf, file_data, &follow, ea->blocknr ,&last_rec, 0x7);
							
									if(last_rec == blk[0]+i){
										extent_db_add (db, ea,1);
										extent_db_del(db,blk[0]+i);
										ea=NULL;
									}
									else{
									extent_db_add (db, ea,1);
										ea = NULL;
									}
								}
							}
						}
						else{
						extent_db_add (db, ea,1);
						ea = NULL;
						}
					}
				blk[0] = blk[0] - count + i;
				i = count;
				if(ea) {free(ea); ea=NULL;}	
				}	
			} //ext4meta
		} //for i
		blk[0] += (i)?i:1;
		count = get_range(blk ,ds_bmap,buf, &dummy);
	} //count
//	} //ds_bmap
	
	while(1){
		count = recover_db(db, ea_group);
		if(count){
			ea = ea_group;	
			if (( ea->blocknr) &&(! ea->l_start)){
				first_b = ea->start_b;
				if(( ext2fs_test_block_bitmap( bmap,first_b)) || 
					(io_channel_read_blk (current_fs->io,first_b-1 ,9,tmp_buf ))){
					fprintf(stderr,"Warning: block %10lu can not read or is allocated\n",(long unsigned int)first_b);
//FIXME ERROR goto ?
				}
				else{
					scan = magic_check_block(tmp_buf+blocksize, cookie, cookie_f , magic_buf , blocksize*8,first_b, 1);
					file_data = new_file_data(first_b,scan,magic_buf,tmp_buf+blocksize,&follow,8);
					if ((file_data) && (add_ext4_extent_idx_data(file_data, ea))){
						for (i = 1; i<count; i++){
							ea++;
							add_ext4_extent_idx_data(file_data, ea);
						}
						io_channel_read_blk (current_fs->io, file_data->last,  1, tmp_buf);
						file_data = soft_border(des_dir, tmp_buf, file_data, &follow, ea_group[0].blocknr,
								&last_rec, 0x7 | FORCE_RECOVER);

						if(last_rec != ea_group[0].blocknr)
						printf("error: block %lu -> file not written\n",(long unsigned int)ea_group[0].blocknr);
						for (i=0; i<count; i++){
							extent_db_del(db,ea_group[i].blocknr);
						}
					}
					else{
						printf("error : while create new file_data\n");
//						goto errout;
					}
				}
			}
			else{
				printf("error : no first blocknumber or not begin of a file\n");
//				goto errout;
			}
		ea = NULL;
		}
		else{
//			printf("ExtentDB ready for now\n");
			break; 
		}
	} //while
//--------------------------
	last_rec = 0;
	count = 0;
	blk[0] = 1;
	follow = 0;	
	count = get_range(blk ,ds_bmap, buf, &border);
	ea = NULL;

	while (count){
newloop:
		for (i = 0; i< count ;i++){
			if ((follow % tes) && (ext2fs_test_block_bitmap(c_bmap,blk[0]+i))){
				if (ea)
					(ea->len)++;
				if (file_data)
					add_file_data(file_data, blk[0]+i, scan ,&follow);
				else
					follow++;
				continue;
			}
				
			if (!follow){
				scan = magic_check_block(buf+(i*blocksize), cookie, cookie_f , magic_buf , (count-i)*blocksize ,blk[0]+i, 1);
//printf("SCAN %lu : %09x : %s\n",blk[0]+i,scan,magic_buf);
				ea = new_extent_area();	
				if ( ea ){
					ea->start_b = blk[0]+i;
					(ea->len)++;
					if ((scan & M_IS_FILE) && (!(scan & M_DATA))){
						file_data = new_file_data(blk[0]+i,scan,magic_buf,buf+(i*blocksize),&follow,count - i);
						if (file_data){
//----------------------<<<
//File can recover by follow the datastream 
				if (file_data->scantype & DATA_CARVING){
					(ea->len)--;
					for (j=0;j<file_data->buf_length;j++){
							add_file_data(file_data, blk[0]+i+j, scan ,&follow);
							(ea->len)++;
					}
					blk[0] += i+j; 
					count = get_range(blk ,ds_bmap,buf, &border);
					ret = 1;
					while (ret && (!(file_data->scantype & (DATA_READY | DATA_BREAK)))){	
						file_data->buf_length = (!follow) ? (count+1) : count;
						ret = file_data->func(buf, &size ,scan ,3 , file_data);
						if (ret){
							if (((!file_data->buf_length)&& (!(file_data->scantype & (DATA_READY))))|| ((ret == 3) && (file_data->scantype & H_F_Carving))){ 
								// break follow > scan now
								goto newloop;
							}

							if (file_data->buf_length > count){
								i = check_next_border(ds_bmap, blk[0],file_data->buf_length);
								if (i){
									ea->len = file_data->last_match_blk;
//		printf("%lu : BREAK-1 : %s after %ld blocks\n",blk[0]+i,file_data->name, ea->len);
									blk[0] = ea->start_b + ((ea->len)?ea->len:1);
									extent_db_add (db, ea, 0);
									ea = NULL;
									file_data = forget_file_data(file_data, &follow);
									count = get_range(blk ,ds_bmap,buf, &border);
									goto newloop;
								}
							}									

							for (j=0;j<file_data->buf_length;j++){
								add_file_data(file_data, blk[0]+j, scan ,&follow);
								(ea->len)++;
							}
							
							if ((border && (file_data->last == blk[1]))&& (!(file_data->scantype & (DATA_READY)))){
								if ((!file_data->next_offset)&&(file_data->scantype & DATA_NO_FOOT)){
									file_data->scantype |= DATA_READY;
//		printf("possible EOF, recover this\n");
								}
								else {
//		printf("%lu : BREAK-2 : %s after %ld blocks\n",blk[0]+i,file_data->name, file_data->last - file_data->first);
									extent_db_add (db, ea, 0);
									ea = NULL;
									blk[0] = file_data->last + 1;
									file_data = forget_file_data(file_data, &follow);
									count = get_range(blk ,ds_bmap,buf, &border);
									goto newloop;
								}
							}
							blk[0] += j;
							count = get_range(blk ,ds_bmap,buf, &border);
						}
						else{
//		printf("%lu : BREAK-3 : %s after %ld blocks\n",blk[0]+file_data->buf_length,file_data->name, file_data->last - file_data->first);
							if (!file_data->last_match_blk)
								(file_data->last_match_blk)++;
							ea->len = file_data->last_match_blk;
							extent_db_add (db, ea, 0);
							ea = NULL;
							blk[0] = file_data->first + file_data->last_match_blk ;
							file_data = forget_file_data(file_data, &follow);
							count = get_range(blk ,ds_bmap,buf, &border);
							goto newloop;
						}
					}
					if (file_data->scantype & (DATA_READY)){
						io_channel_read_blk (current_fs->io, file_data->last,  1, tmp_buf);
						file_data = soft_border(des_dir, tmp_buf, file_data, &follow, ea->start_b ,&last_rec, 0x7);
						if (ea->start_b != last_rec){
// 		printf("%lu force recover, but not written\n",ea->blocknr);
							extent_db_add (db, ea, 0);
							ea = NULL;
						}
					}
					else{
//						printf("%lu : BREAK put into extent-cache\n", blk[0]);
						if (!file_data->last_match_blk)
							(file_data->last_match_blk)++;
						ea->len = file_data->last_match_blk;
						extent_db_add (db, ea, 0);
						ea = NULL;
						blk[0] = file_data->first + file_data->last_match_blk;
						file_data = forget_file_data(file_data, &follow);
						count = get_range(blk ,ds_bmap,buf, &border);
					}
					if (ea){free(ea);ea = NULL;}
					follow = 0;
					goto newloop;
				}
//End follow datastream
				else{
					add_file_data(file_data, blk[0]+i, scan ,&follow); //add the first block 
				}
				if (file_data->scantype & DATA_LENGTH){
//Start of size carving
					j = check_next_border(ds_bmap, blk[0]+i,((file_data->size-1)/blocksize)+1);	
					if ((!j) && (! io_channel_read_blk (current_fs->io, blk[0]+ i + ((file_data->size-1)/blocksize), 1, tmp_buf))){
						size = 	get_block_len((char*)tmp_buf);
						if ((!(file_data->scantype & DATA_MINIMUM)) && (size <= ((file_data->size % blocksize)?(file_data->size % blocksize):blocksize))){
							tmp_file_data = copy_file_data(file_data);
							if (tmp_file_data){
								for (j = 1; j< (((file_data->size-1)/blocksize)+1); j++){
									add_file_data(tmp_file_data, blk[0]+i+j, scan ,&follow);
								}
								tmp_file_data = soft_border(des_dir, tmp_buf, tmp_file_data, &follow, blk[0]+i ,&last_rec, 0x3);
								if (blk[0]+i == last_rec){
									file_data = forget_file_data(file_data, &follow);
									if (ea){free(ea);ea = NULL;}
									blk[0]+= j + i;
									count = get_range(blk ,ds_bmap,buf, &border);
									goto newloop;
								}
							}
						}
								
					}
//and only MIN_SIZE 
					if ((!j) && (file_data->scantype & DATA_MINIMUM)){
						for (j = 1; j< (((file_data->size-1)/blocksize)+1); j++){
							add_file_data(file_data, blk[0]+i+j, scan ,&follow);
							(ea->len)++;
						}
						file_data->scantype = H_F_Carving;
						blk[0]+=(i+j-2);
						count = get_range(blk ,ds_bmap,buf, &border);
						i = 1;
//						size = get_block_len(buf+(i*blocksize));
						goto followloop;
					}
					else{ 
						file_data->scantype = H_F_Carving;
						follow = ea->len;
					}
				}
//End size carving
//------------------------------>>>
							
						}else { // no file_data
//		printf("%lu : no file_data\n",blk[0]+i);
							if(ea){
								(ea->l_start)--;
								if(!ea->len)
									(ea->len)++;
								extent_db_add (db, ea, 0);
								ea = NULL;
							}
						}
					}else {
//	 	printf("%lu : no file begin\n",blk[0]+i);
						(ea->l_start)--;
						follow++;
					}
				}else 
					printf("fail malloc ea\n");
			}
			else{ 
				scan = magic_check_block(buf+(i*blocksize), cookie, cookie_f , magic_buf , blocksize ,blk[0]+i, 0);
//				zahler1++;
//		printf("SCAN %lu : %09x : %s\n",blk[0]+i,scan,magic_buf);
				if (file_data){
					if ( check_file_data_possible(file_data, scan ,buf+(i*blocksize))){
						add_file_data(file_data, blk[0]+i, scan ,&follow);
						(ea->len)++;
					}
					else{
						file_data = soft_border(des_dir,buf+((i-1)*blocksize), file_data, &follow, blk[0]+i-1, &last_rec,0x3); 
						if (last_rec != blk[0]+i-1){
//		printf("%lu : clear, put into extent-cache\n", blk[0]+i-1 );
							extent_db_add (db, ea, 0);
							ea = NULL;
						}
						if (ea){free(ea);ea = NULL;}
						i--;
					}
				}
				else{
					if ((scan & M_IS_FILE) && (!(scan & M_DATA))){ 
//		printf("%lu : end of fragment, put into extent-cache\n",blk[0]+i);
						extent_db_add (db, ea, 0);
						ea = NULL;
						follow = 0;
						i--;
					}
					else {
						(ea->len)++;
						follow++;
						if ((scan & M_SIZE) < (blocksize - 32)){
//		printf("%lu : possible end of fragment, add extent-cache\n",blk[0]+i);
							extent_db_add (db, ea, 0);
							ea = NULL;
							follow = 0;
						}
					}
				}
			}
			size = (scan & M_SIZE ); //get_block_len(buf+(i*blocksize));
followloop:
			if (follow && file_data){
				ret = file_data->func(buf+(i*blocksize), &size ,scan ,0 , file_data);
					if (ret == 1){
						 if (file_data_correct_size(file_data,size)){
							file_data = recover_file_data(des_dir, file_data, &follow);
							last_rec = blk[0]+i;
						}
						else{ 
							file_data = forget_file_data(file_data, &follow);
							extent_db_add (db, ea, 0);
							ea = NULL;

//		printf("%lu : Don't recover this file\n",blk[0]+i);
						}
						if (ea){free(ea);ea = NULL;}
					}
					else 
					     if(ea && (file_data->func == file_none))
						(ea->l_start)--;
			} 
		}
		if(border){ 
//		printf("%lu : border warning\n", blk[0]+i);
			if (file_data) {
				file_data = soft_border(des_dir,buf+((i-1)*blocksize), file_data, &follow, blk[0]+i-1, &last_rec,0x3); 
				if (last_rec != blk[0]+i-1){
//		printf("%lu : clear and put into extent-cache\n", blk[0]+i-1 );
					extent_db_add (db, ea, 0);
					ea = NULL;
				}
				else {
					free(ea);ea = NULL;
				}
			}
			if (follow){
//		printf("%lu : end of fragment\n", blk[0]+i);
				follow = 0;
			}
			if (ea){
				extent_db_add (db, ea, 0);
				ea = NULL;
			}
		}
		blk[0] += (i)?i:1;
		count = get_range(blk ,ds_bmap,buf, &border);
	}
	
	} 
//		printf("ExtentDB : %lu entries\n",db->count);
	if (db){ extent_db_clear(db) ; db=NULL;}
}//ds_retval

errout:
clear_block_bitmap_list(d_bmap);
ext2fs_free_block_bitmap(c_bmap);
if (ea) free (ea);
if (db) extent_db_clear(db);
if (v_buf) free(v_buf);
if (tmp_buf) free(tmp_buf);
if (magic_buf) free(magic_buf);
if (cookie) magic_close(cookie); 
if (cookie_f) magic_close(cookie_f);
//printf("Count-1 = %lu   Count-2 = %lu \n",zahler, zahler1);
return 0;
}//funcion
//--------END----------
