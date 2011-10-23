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
 *   C Implementation: file_type                                           *
 ***************************************************************************/
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "util.h"
#include "inode.h"

#ifdef WORDS_BIGENDIAN
#define ext4magic_be64_to_cpu(x) ((__u64)(x))
#else
#define ext4magic_be64_to_cpu(x) ext2fs_swab64((x))
#endif

//#define DEBUG_QUICK_TIME
//#define DEBUG_MAGIC_MP3_STREAM
//#define DEBUG_OGG_STREAM

extern ext2_filsys     current_fs ;

// index of the files corresponding magic result strings
int ident_file(struct found_data_t *new, __u32 *scan, char *magic_buf, char *buf){

//Please do not modify the following lines.
//they are used for indices to the filestypes
	char	typestr[] ="application/ audio/ image/ message/ model/ text/ video/ ";
	char	imagestr[] ="gif jp2 jpeg png svg+xml tiff vnd.adobe.photoshop vnd.djvu x-coreldraw x-cpi x-ico x-ms-bmp x-niff x-portable-bitmap x-portable-greymap x-portable-pixmap x-psion-sketch x-quicktime x-unknown x-xcursor x-xpmi x-tga ";
	char	videostr[] ="3gpp h264 mp2p mp2t mp4 mp4v-es mpeg mpv quicktime x-flc x-fli x-flv x-jng x-mng x-msvideo x-sgi-movie x-unknown x-ms-asf x-matroska ";
	char	audiostr[] ="basic midi mp4 mpeg x-adpcm x-aiff x-dec-basic x-flac x-hx-aac-adif x-hx-aac-adts x-mod x-mp4a-latm x-pn-realaudio x-unknown x-wav ";
	char	messagestr[] ="news rfc822 ";
	char	modelstr[] ="vrml x3d ";
	char	applistr[] ="dicom mac-binhex40 msword octet-stream ogg pdf pgp pgp-encrypted pgp-keys pgp-signature postscript unknown+zip vnd.google-earth.kml+xml vnd.google-earth.kmz vnd.lotus-wordpro vnd.ms-cab-compressed vnd.ms-excel vnd.ms-tnef vnd.oasis.opendocument. vnd.rn-realmedia vnd.symbian.install x-123 x-adrift x-archive x-arc x-arj x-bittorrent x-bzip2 x-compress x-coredump x-cpio x-dbf x-dbm x-debian-package x-dosexec x-dvi x-eet x-elc x-executable x-gdbm x-gnucash x-gnumeric x-gnupg-keyring x-gzip x-hdf x-hwp x-ichitaro4 x-ichitaro5 x-ichitaro6 x-iso9660-image x-java-applet x-java-jce-keystore x-java-keystore x-java-pack200 x-kdelnk x-lha x-lharc x-lzip x-mif xml xml-sitemap x-msaccess x-ms-reader x-object x-pgp-keyring x-quark-xpress-3 x-quicktime-player x-rar x-rpm x-sc x-setupscript x-sharedlib x-shockwave-flash x-stuffit x-svr4-package x-tar x-tex-tfm x-tokyocabinet-btree x-tokyocabinet-fixed x-tokyocabinet-hash x-tokyocabinet-table x-xz x-zoo zip x-font-ttf ";
	char	textstr[] = "html PGP rtf texmacs troff vnd.graphviz x-awk x-diff x-fortran x-gawk x-info x-lisp x-lua x-msdos-batch x-nawk x-perl x-php x-shellscript x-texinfo x-tex x-vcard x-xmcd plain x-pascal x-c++ x-c x-mail x-makefile x-asm text ";
//Files not found as mime-type
	char	undefstr[] ="MPEG Targa 7-zip cpio CD-ROM DVD 9660 Kernel boot Linux filesystem x86 Image CDF SQLite OpenOffice.org Microsoft VMWare3 VMware4 JPEG ART PCX RIFF DIF IFF ATSC ScreamTracker ";
//-----------------------------------------------------------------------------------
	char* 		p_search;
	char		token[30];
	int		len, flag;
	int		major = 0;
	int		minor = 0;
		flag = 0;
	p_search = typestr;
	while (*p_search){
		len=0;
		while((*p_search) != 0x20){
			token[len] = *p_search;
			p_search++;
			len++;
		}
		token[len] = 0;
		major++;
		if (!strncmp(magic_buf,token,strlen(token))){
			flag = 1;
			switch (major){
				case 1:
					p_search = applistr;
					break;
				case 2:
					p_search = audiostr;
					break;
				case 3:	
					p_search = imagestr;
					break;
				case 4:
					p_search = messagestr;
					break;
				case 5:
					p_search = modelstr;
					break;
				case 6:
					p_search = textstr;
					break;
				case 7:
					p_search = videostr;
					break;
				default:
					p_search = undefstr;
					major = 8;
			}
			break;
		}
		p_search++;
	}
	if (!flag){
		p_search = undefstr;
		major = 8;
	}
	flag = 0;
	minor = 0;
	while (*p_search){
		len=0;
		while((*p_search) != 0x20){
			token[len] = *p_search;
			p_search++;
			len++;
		}
		token[len] = 0;
		minor++;
		if (strstr(magic_buf,token)){
			flag = 1;	
			break;
		}
		p_search++;
	}
	if(!flag) minor=255;
#ifdef DEBUG_MAGIC_SCAN
	printf("major : %d   minor : %d\n", major,minor);
#endif
	*scan = (major <<8)| minor; 
return 1;
}



static __u32 test_id3_tag(unsigned char *buf){
	__u32 	offset = 0;

	if(buf[0]=='I' && buf[1]=='D' && buf[2]=='3' && (buf[3]==2 || buf[3]==3 || buf[3]==4) && buf[4]==0){

		if(buf[3]==4 && (buf[5]&0x10)==0x10) 
			offset = 10 ;

		offset += ((buf[6]&0x7f)<<21) + ((buf[7]&0x7f)<<14) + ((buf[8]&0x7f)<<7) + (buf[9]&0x7f)+ 10;
//		while (!(buf[offset]) && i--)
//			frame_offset++;

//		fprintf (stderr,"ID3-TAG found : size  %lu \n", offset);	
	}
	return offset;
} 



/*
// skeleton function (Version:  0.2.0)
// The following functions control the recover of the individual file types. All this functions have the same
// call parameters and run on a common function-pointer.
// This is a statement of values and variables for your own extensions.
// For each file type, such a specific function can be defined. If no function is assigned, is used automatically "file_default()".
// In accordance with the scan result one of these functions is selected by "get_file_property()" 
// and invoked during the program run, with different parameters for different tasks.
// The variables have also different meanings. 

int file_dummy(unsigned char *buf, int *size, __u32 scan , int flag, struct found_data_t* f_data){

switch (flag){ 	
// the function is controlled by the variable "flag"
  case 0  : // Called to test whether the file is encountered.
	// unsigned char *buf   	==  pointer to the data of the current last known block 
	//				    (allowed access buf[0] to buf[blocksize-1] )
	// int *size            	==  pointer to the "size" of this block. (means: The last character which is not NULL.)
	//			   	    this can be changed here to set the correct file length
	// __u32 scan			==  undefined
	// struct found_data_t* f_data  ==  pointer to the data structure of its own file, including the generated inode.
					    allowed full access to all fields.
	return	
		// 0 == this is not a file end (do not recover now)
		// 1 == file end is here ; File length is adjusted according to the value in "size". (The length of the last block)
		//	recover this file.
		// 2 == The file could end here if the next block is a new file. (Or the next data block can not be attached)
		//	File length is adjusted accordingly "size". recover if possible
		// 4 == As at "return 2" ; but not if the length of the file currently has 12 blocks.


  case  1 : // Test if a single block can be added to the file or has a wrong content for this file.(ext3 only small files <= 12*blocksize)
	// unsigned char *buf   	==  undefined
	// int *size            	==  undefined
	// __u32 scan			==  the scan result of the next data block, Use the Macro in src/util.h
	// struct found_data_t* f_data  ==  undefined
	
	return
		// 0 == not attach this block
		// 1 == attach this block


  case 2:  // Called during initialization, can test the beginning of the file and the initial course of the file 
	// unsigned char *buf   	==  pointer to data blocks for a possible new file of this type
	//				    (allowed access buf[-blocksize] to buf[(blocksize * 12]) -1 )
	//				    The data of the file can be checked up to 12 data block. 
	//				    For many file types can be determined in the final file length.
	// int *size            	==  undefined
	// __u32 scan			==  undefined
	// struct found_data_t* f_data  ==  pointer to the new data structure for the new file.
	//				    allowed full access to all fields, except inode, (these data are not produced at this time).
	//				    possible change of the name for the file, or change the file_function() for this filetype,
	//				    often a sign created for the file length in the variable "f_data->size", which will
	//				    be used later for file length determination. (see "case 0:" )

	return 
		// the return value is currently not evaluated.
  }
}
*/


//default
int file_default(unsigned char *buf, int *size, __u32 scan , int flag, struct found_data_t* f_data){
	int ret = 0;
	switch (flag){
		case 0 :
			if(f_data->inode->i_block[12]){   //FIXME for ext4
				ret = 1;
				break;
			}
			else{
				if (*size < (current_fs->blocksize -4))
					ret = 4;

				if (*size < (current_fs->blocksize -64))
					ret = 2;

				if (*size < (current_fs->blocksize -256))
					ret = 1;
			}

			break;
		case 1 :
			return (scan & (M_IS_META | M_CLASS_1)) ? 0 :1 ;
			break;
	}
	return ret;
}



//none   #do not recover this
int file_none(unsigned char *buf, int *size, __u32 scan , int flag, struct found_data_t* f_data){
	return 0;
}



//Textfiles 
int file_txt(unsigned char *buf, int *size, __u32 scan , int flag, struct found_data_t* f_data){
	switch (flag){
		case 0 :
			if (*size < current_fs->blocksize){
				if (buf[(*size)-1] == (unsigned char)0x0a) 
					return 1;
			}
			else {
				if (buf[(*size)-1] == (unsigned char)0x0a)
					return (f_data->inode->i_size == (current_fs->blocksize *12)) ? 4 : 2;
			}
			break;
		case 1 :
			return (scan & M_TXT);
			break;
	}
	return 0;
}



//Textfiles binary
int file_bin_txt(unsigned char *buf, int *size, __u32 scan , int flag, struct found_data_t* f_data){
	switch (flag){
		case 0 :
			if (*size < current_fs->blocksize){
				return 1;
			}
			else {
				if (buf[(*size)-1] == (unsigned char)0x0a)
					return (f_data->inode->i_size == (current_fs->blocksize *12)) ? 4 : 2;
			}
			break;
		case 1 :
			return (scan & M_TXT);
			break;
	}
	return 0;
}


//gzip
int file_gzip(unsigned char *buf, int *size, __u32 scan , int flag, struct found_data_t* f_data){
	int		ret = 0;

	switch (flag){
		case 0 :
			if(*size < (current_fs->blocksize -8)) 
				ret = 1 ;
			else {
				if(*size < (current_fs->blocksize -4))
					ret = 2 ;
			}
			*size += 4;
			break;
		case 1 :
			return (scan & (M_IS_META | M_CLASS_1 | M_BLANK | M_TXT)) ? 0 :1 ;
			break;
	}
	return ret;
}


//zip
int file_zip(unsigned char *buf, int *size, __u32 scan , int flag, struct found_data_t* f_data){
	int		ret = 0;
	int		j,i;
	unsigned char		token[5];
	
	sprintf(token,"%c%c%c%c",0x50,0x4b,0x05,0x06);

	switch (flag){
		case 0 :
			if((*size) > 22){
				j = strlen(token) -1;
				i = (*size -12);
				while((i > ((*size)-22)) && (buf[i] != token[j])){
					i--;
				}
				while ((i> ((*size)-22)) && (j >= 0) && (buf[i] == token[j])){
					j--;
					i--;
				}
				if ( j<0){
					*size = i+22+1;
					ret =1;
				} 	
			}
			else
				ret = 2;
			break;
		case 1 :
			return (scan & (M_IS_META | M_CLASS_1 | M_BLANK)) ? 0 :1 ;
			break;
	}
	return ret;
}


//ttf
int file_ttf(unsigned char *buf, int *size, __u32 scan , int flag, struct found_data_t* f_data){
	int		ret = 0;

	switch (flag){
		case 0 :
			if((*size) < (current_fs->blocksize - 16)){
				*size = ((*size ) + 3) & ~3 ;
				ret =1;
			}
			else {
				*size = ((*size ) + 3) & ~3 ;
				ret = 4;
			}
			break;
		case 1 :return (scan & (M_IS_META | M_CLASS_1 | M_BLANK)) ? 0 :1 ;
			break;
	}
	return ret;
}
			


//iso9660 CD-ROM
int file_iso9660(unsigned char *buf, int *size, __u32 scan , int flag , struct found_data_t* f_data){
	__u64		lsize;
	__u32		*p_32;
	__u16		*p_16;
	int		ssize;
	int		ret = 0;
	unsigned char		cd_str[] = "CD001";

	switch (flag){
		case 0 :
			if (f_data->size || f_data->h_size) {
				if (f_data->h_size != f_data->inode->i_size_high)
					ret = 0;
				else{
					if (f_data->inode->i_size < f_data->size)
						ret = 0;
					else{
						ssize = (f_data->size % current_fs->blocksize);
						*size = (ssize)? ssize : current_fs->blocksize;
						ret = 1;
					}
				}
			}
			else {
				ret =0;
			}
			break; 
		case 1 :
			return 1;
			break;
		case 2 :
			if (current_fs->blocksize > 2048){
				p_32 = (__u32*)(buf + 0x8050);
				p_16 = (__u16*)(buf + 0x8080);
				if ((!strncmp((buf+0x8001),cd_str,5)) && (ext2fs_le32_to_cpu(*(p_32 +1)) == ext2fs_be32_to_cpu(*p_32))){
					lsize = (__u64)(ext2fs_le32_to_cpu(*p_32)) * ext2fs_le16_to_cpu(*p_16);
					f_data->size = lsize & 0xFFFFFFFF;
					f_data->h_size = lsize >> 32;
					ret =1;
				}
			}
			break;
	}
	return ret;
}


// 7-zip
int file_7z(unsigned char *buf, int *size, __u32 scan , int flag , struct found_data_t* f_data){ 
__u64		lsize,*p;
__u32		ssize;
int		ret = 0;
	switch (flag){
		case 0 : 
			if (f_data->size || f_data->h_size) {
				if (f_data->h_size != f_data->inode->i_size_high)
					ret = 0;
				else{
					if (f_data->inode->i_size < f_data->size)
						ret = 0;
					else{
						ssize = (f_data->size % current_fs->blocksize);
						 if (*size < ssize) 
							*size = ssize;
						ret = 1;
					}
				}
			}
			else {
				ret =0x10; //not recover for now
				*size += 7;
			}
			break;

		case 1 :
			return ( scan & (M_IS_META | M_CLASS_1 | M_BLANK)) ? 0 :1 ;
			break;
		case 2 :
			p = (__u64*)(buf+12);
			lsize = ext2fs_le64_to_cpu(*p) + (__u64)32;
			p++;
			lsize = lsize + (ext2fs_le64_to_cpu(*p));
			f_data->size = lsize & 0xFFFFFFFF ;
			f_data->h_size = lsize >> 32;
			ret = 1;
			break;
	}
	return ret;
}


//rar
int file_rar(unsigned char *buf, int *size, __u32 scan , int flag, struct found_data_t* f_data){
	int 	i,j;
	int 	ret = 0;
	unsigned char token[7]={0xc4, 0x3d, 0x7b, 0x00, 0x40, 0x07, 0x00 };

	switch (flag){
		case 0 :
			j = 5;
			i = (*size) -1;
			while ((i >= 0) && (j >= 0) && (buf[i] == token[j])){
				i--;
				j--;
			}
			if ((i == -1) || (j == -1)){
				*size = (*size) + 1;
				ret = 1;
			}
			break;
		case 1 :	
			return (scan & (M_IS_META | M_CLASS_1 | M_BLANK)) ? 0 :1 ;
			break;
	}
	return ret;
}	 




//cpio
int file_cpio(unsigned char *buf, int *size, __u32 scan , int flag, struct found_data_t* f_data){
	int 	i,j;
	int 	ret = 0;
	unsigned char	token[]="TRAILER!!!";

	switch (flag){
		case 0 :
			j = strlen(token) -1;
			i = (*size) -1;
			while ((i >= 0) && (j >= 0) && (buf[i] == token[j])){
				i--;
				j--;
			}
			if ((i == -1) || (j == -1)){
				*size = ((*size) + 0x01FF) & 0xfe00 ;
				ret=1;
			}	
			break;
		case 1 :	
			return (scan & (M_IS_META | M_CLASS_1)) ? 0 :1 ;
			break;
	}
	return ret;
}	 



//pdf
int file_pdf(unsigned char *buf, int *size, __u32 scan , int flag, struct found_data_t* f_data){
	int 	i,j;
	int 	ret = 0;
	unsigned char	token[6];
	sprintf(token,"%c%c%c%c%c",0x25,0x45,0x4f,0x46,0x0a);

	switch (flag){
		case 0 :
			if ((*size) < 3)
				ret =2;
				else{ 
				j = strlen(token) -2;
				i = (*size) -2;
				if(buf[i] != (char)0x46)
					i--;
	
				while ((i >= 0) && (j >= 0) && (buf[i] == token[j])){
					i--;
					j--;
				}
				if ((i == -1) || (j == -1)){
					ret=1;
				}
			}
			break;
		case 1 :
			return (scan & (M_IS_META | M_CLASS_1)) ? 0 :1 ;
			break;
		}
	return ret;
}


//ps   switch only to pdf or txt
int file_ps(unsigned char *buf, int *size, __u32 scan , int flag, struct found_data_t* f_data){
	unsigned char *c;
	int 	ret = 1;
	unsigned char	token[9] = "PS-Adobe";

	switch (flag){
		case 0 :
			break;
		case 1 :
			break;
		case 2 :
			c = buf+2 ;
			if (! strncmp(c,token,7))
				f_data->func = file_pdf ;
			else 
				f_data->func = file_txt ;
			break;
		}
	return ret;
}



//arj
int file_arj(unsigned char *buf, int *size, __u32 scan , int flag, struct found_data_t* f_data){
	int 	ret = 0;

	switch (flag){
		case 0 :
			if ((*size>1) && (*size < current_fs->blocksize) && (buf[(*size)-1] == (unsigned char)0xea) && 
				(buf[(*size)-2] == (unsigned char)0x60)){
					*size +=2;
					ret = 1;
			}
			break;
			
		case 1 :
			return (scan & (M_IS_META | M_CLASS_1)| M_BLANK | M_TXT) ? 0 :1 ;
			break;
	}
	return ret;
}



//xz
int file_xz(unsigned char *buf, int *size, __u32 scan , int flag, struct found_data_t* f_data){
	int 	ret = 0;

	switch (flag){
		case 0 :
			if ((*size>1) && (*size < current_fs->blocksize) && (buf[(*size)-1] == (unsigned char)0x5a) && 
				(buf[(*size)-2] == (unsigned char)0x59))
					ret = 1;
			break;
			
		case 1 :
			return (scan & (M_IS_META | M_CLASS_1)| M_BLANK | M_TXT) ? 0 :1 ;
			break;
	}
	return ret;
}



//tar
int file_tar(unsigned char *buf, int *size, __u32 scan , int flag, struct found_data_t* f_data){
	int 	i,ret = 0;
	__u64	offset;
	__u64	len;
	__u8	*o_str;

	switch (flag){
		case 0 :
			if ((((__u64)f_data->inode->i_size |((__u64)f_data->inode->i_size_high<<32)) >= (__u64)((f_data->size + 0xfff) & ~0xfff)) &&  
				((*size) < (current_fs->blocksize - 0xff))){
				if (!(*size))
					*size = current_fs->blocksize;
				else
					*size = ((*size) + 1023) & ~1023 ;
	
				if ((f_data->inode->i_block[12]) || (f_data->size < f_data->inode->i_size))  //FIXME for ext4
					ret = 1;
			}
			break;
		case 1 :
			if (scan & M_TAR)
				ret = 1;
			 else 
				ret = (scan & M_IS_META | M_CLASS_1 ) ? 0 :1 ;
			break;
		case 2 :
			offset = 0x0;
			while (offset < ((12 * current_fs->blocksize)-27)){
				if ((buf[offset + 0x101] == 0x75) && (buf[offset + 0x102] == 0x73) && (buf[offset + 0x103] == 0x74)&&
					(buf[offset + 0x104] == 0x61) && (buf[offset + 0x105] == 0x72)){
					len = 0;
					o_str = (__u8*)(buf + offset + 0x7c);
					for (i=0;i<10;i++){
						len += (*o_str & 0x7) ;
						len <<= 3;
						o_str++;
					}
					len += (*o_str & 0x7);
					len = ((len + 0x1ff) & ~0x1ff);
					offset += len + 0x200 ;
//					printf("TAR : block %lu : offset %8lu \n",f_data->first, offset);
					continue;
				}
				else
					break;
			}
			f_data->size = offset;
			ret = 1 ;		  
	}
	return ret;
}	 


//binary
int file_binary(unsigned char *buf, int *size, __u32 scan , int flag, struct found_data_t* f_data){
	int 	ret = 0;

	switch (flag){
		case 0 :
		if (*size && ((*size) < current_fs->blocksize - 7)&&(buf[(*size)-1] == 0x01)){
			*size +=7;
			ret = 1;
		}
		else {
			if ((*size < current_fs->blocksize) && ((*size) > current_fs->blocksize - 7) && (buf[(*size)-1] == 0x01)){
				*size =  current_fs->blocksize;
				ret = 2;
			}
			else{
				if (!(*size)){
					*size = 7;
					ret = 1;
				}
				else{
					if ((buf[(*size)-1] != 0x01)&&(*size < current_fs->blocksize - 128)){
						*size +=1;
						ret = 2;
					}
				}	
			}
		}
			break;
		case 1 :
			return (scan & (M_IS_META | M_CLASS_1)) ? 0 :1 ;
			break;
	}
	return ret;
}			 

	
//object
int file_object(unsigned char *buf, int *size, __u32 scan , int flag, struct found_data_t* f_data){
	__u32		len;
	__u32		*p_help;
	int		ret = 0;

	switch (flag){
		case 0 :
			if ((buf[(*size)-1] == 0x0a)){
				ret=1;
			}
			else{
				len = ((*size) +3) & ~3;
				p_help = (int*) (buf +len -4);
				if ((*p_help < 0x400) && (len < current_fs->blocksize )){
					ret = 1;
					*size = len;
				}
			}
			break;
		case 1 :
			return (scan & (M_IS_META | M_CLASS_1 )) ? 0 :1 ;
			break;
	}
	return ret;
}




int file_qt(unsigned char*, int*, __u32, int, struct found_data_t*);

//jpeg
int file_jpeg(unsigned char *buf, int *size, __u32 scan , int flag, struct found_data_t* f_data){
	int 	ret = 0;

	switch (flag){
		case 0 :
			if ((*size>1) && (buf[(*size)-1] == (unsigned char)0xd9) && (buf[(*size)-2] == (unsigned char)0xff))
					ret = 1;
			else
				if ((*size == 1) && (buf[(*size)-1] == (unsigned char)0xd9))
					ret = 1;
			break;
			
		case 1 :
			return (scan & (M_IS_META | M_CLASS_1)) ? 0 :1 ;
			break;
	}
	return ret;
}




//art
int file_art(unsigned char *buf, int *size, __u32 scan , int flag, struct found_data_t* f_data){
	int 	ret = 0;
	static const unsigned char header[8]   = {0x4a,0x47,0x04,0x0e,0x00,0x00,0x00,0x00};
	switch (flag){
		case 0 :
			if ((*size >1) && (*size < current_fs->blocksize) && (buf[(*size)-1] == 0xcb) && (buf[(*size)-2] == 0xcf))
					ret = 1;
			break;
		case 1 :
			return (scan & (M_IS_META | M_CLASS_1 | M_TXT | M_BLANK )) ? 0 :1 ;
			break;
		case 2 :
			if(memcmp(buf,header,8)) 
				f_data->func = file_none;
			else 
				ret = 1 ;
			break;
	}
	return ret;
}



//gif
int file_gif(unsigned char *buf, int *size, __u32 scan , int flag, struct found_data_t* f_data){
	int 	ret = 0;

	switch (flag){
		case 0 :
			if ((*size) && (*size < current_fs->blocksize) && (buf[(*size)-1] == 0x3b))
					ret = 1;
			break;
		case 1 :
			return (scan & (M_IS_META | M_CLASS_1)) ? 0 :1 ;
			break;
	}
	return ret;
}
			


//bmp
int file_bmp(unsigned char *buf, int *size, __u32 scan , int flag, struct found_data_t* f_data){
	__u32	ssize;
	int 	ret = 0;

	switch (flag){
		case 0 : 
			if (f_data->size ) {
				ssize = f_data->size % current_fs->blocksize;
				if (f_data->inode->i_size > (f_data->size - ssize)){
					*size = ssize;
					ret =1;
				}
			}
			
			break;
		case 1 :
			return (scan & (M_IS_META | M_CLASS_1 )) ? 0 :1 ;
			break;

		case 2 :
	//		p_32 = (__u32*)(buf+2);
        //			f_data->size =  ext2fs_le32_to_cpu(*p_32);
			f_data->size = ((__u32)*(buf+2)) | ((__u32)*(buf+3))<<8 | ((__u32)*(buf+4))<<16 | ((__u32)*(buf+5))<<24 ;
			ret =1;
	}
	return ret;
}


//png
int file_png(unsigned char *buf, int *size, __u32 scan , int flag, struct found_data_t* f_data){
	int 	ret = 0;

	switch (flag){
		case 0 :
			if (*size > 8){
				 if (strstr(buf + (*size) -8,"END"))	
					ret=1;
			}
			else{
				if (*size >= 5){
				 	if (strtok(buf,"D" ))
					ret=1;
				}
				else
					if (*size < 5)
						ret = 2; 
			}	
			break;
		case 1 : 
			return (scan & (M_IS_META | M_CLASS_1)) ? 0 :1 ;
			break;
	}
	return ret;
}


//mng
int file_mng(unsigned char *buf, int *size, __u32 scan , int flag, struct found_data_t* f_data){
	int 	ret = 0;

	switch (flag){
		case 0 :
			if ((*size>1) && (buf[(*size)-1] == (unsigned char)0xd5) && (buf[(*size)-2] == (unsigned char)0xf7))
				ret = 1;
			else
				if ((*size == 1) && (buf[(*size)-1] == (unsigned char)0xd5))
					ret = 1;
			break;
		case 1 :
			return (scan & (M_IS_META | M_CLASS_1)) ? 0 :1 ;
			break;
	}
	return ret;
}


//iff
int file_iff(unsigned char *buf, int *size, __u32 scan , int flag, struct found_data_t* f_data){
	int 		i,ret = 0;
	__u32		ssize;
	char 		token[5];	
	char type[] = "AIFF AIFC 8SVX 16SV SAMP MAUD SMUS CMUS ILBM RGBN RGB8 DEEP DR2D TDDD LWOB LWO2 LWLO REAL MC4D ANIM YAFA SSA ACBM FAXX FTXT CTLG PREF DTYP PTCH AMFF WZRD DOC ";

	switch (flag){
		case 0 : 
			if (f_data->size ) {
				ssize = f_data->size % current_fs->blocksize;
				if (f_data->inode->i_size > (f_data->size - ssize)){
					*size = ssize;
					ret =1;
				}
			}
			break;
		case 1 :
			return (scan & (M_IS_META | M_CLASS_1 )) ? 0 :1 ;
			break;

		case 2 :

		if ((buf[0]==0x46) && (buf[1]==0x4f) && (buf[2] ==0x52) && (buf[3]==0x4d)){
			for (i=0;i<4;i++)
				token[i]=buf[i+8];
			token[i] = 0;
			if(strstr(type,token)){
				f_data->size = (buf[4]<<24) + (buf[5]<<16) + (buf[6]<<8) + buf[7] + 8;
				ret =1;
			}
		}
		break;
	}
return ret;
}



//pcx
int file_pcx(unsigned char *buf, int *size, __u32 scan , int flag, struct found_data_t* f_data){
	int 	ret = 0;
struct pcx_header {
  __u8  	Manufacturer; /* should always be 0Ah                */
  __u8  	Version; 
  __u8  	Encoding;    /* 0: uncompressed, 1: RLE compressed */
  __u8  	BitsPerPixel;
  __u16		XMin;        /* image width = XMax-XMin+1      */
  __u16		YMin;        /* image height = YMax-YMin+1    */
  __u16		XMax;
  __u16		YMax;
  __u16 	H_DPI;
  __u16		V_DPI;
  __u8  	Palette[48];
  __u8  	Reserved;
  __u8  	ColorPlanes;   /* 4 -- 16 colors ; * 3 -- 24 bit color (16.7 million colors) */
  __u16 	BytesPerLine;
  __u16 	PaletteType;
  __u16 	HScrSize;    /* only supported by            */
  __u16 	VScrSize;    /* PC Paintbrush IV or higher   */
  __u8  	Filler[54];
};
struct pcx_header	*pcx;

	switch (flag){
		case 0 :
			if (*size < (current_fs->blocksize -3))
				ret = 4;
			if (*size < (current_fs->blocksize - 48))
				ret = 2;
			if (*size < (current_fs->blocksize - 128))
					ret = 1;
			break;
		case 1 :return (scan & (M_IS_META | M_CLASS_1 | M_BLANK | M_TXT)) ? 0 :1 ;
			break;
		case 2 :
		pcx = (struct pcx_header*) buf;

		if ((pcx->Version<=5 && pcx->Version!=1) && pcx->Encoding <=1 && 
      			(pcx->BitsPerPixel==1 || pcx->BitsPerPixel==4 || pcx->BitsPerPixel==8 || pcx->BitsPerPixel==24) &&
      			(pcx->Reserved==0) && (! pcx->Filler[1])  && (! pcx->Filler[53]))

			ret = 1;
		else
			f_data->func = file_none;
	break;	
	}
return ret;
}



//ico
int file_ico(unsigned char *buf, int *size, __u32 scan , int flag, struct found_data_t* f_data){

struct ico_directory
{
  __u8       width;
  __u8       heigth;
  __u8       color_count;
  __u8       reserved;
  __u16      color_planes;
  __u16      bits_per_pixel;
  __u32      bitmap_size;
  __u32      bitmap_offset;
} *ico_dir;

	__u16	*p_count;
	int	counter;
	int 	ret = 0;
	__u32	offset = 22 ;

	switch (flag){
		case 0 :
			if (f_data->size ) {
				counter = f_data->size % current_fs->blocksize;
				if (f_data->inode->i_size > (f_data->size - counter)){
					*size = counter;
					ret =1;
				}
			}
			else{
				if ((*size) < (current_fs->blocksize - 128)){
					ret=1;
				}
				else
					if ((*size) < (current_fs->blocksize - 16)){
					ret=2;
				}
			}
			break;
		case 1 :
			return (scan & (M_IS_META | M_CLASS_1)) ? 0 :1 ;
			break;
		case 2 :
			p_count = (__u16*) (buf + 4) ;
			counter = (int) (ext2fs_le16_to_cpu(*p_count));
			if (counter > 256)
				break; 
			ico_dir = (struct ico_directory*)(buf + 6) ;
			while (counter && offset){
				if (ico_dir->reserved == 0){
					if (ext2fs_le32_to_cpu(ico_dir->bitmap_offset) >= offset){
						offset = ext2fs_le32_to_cpu(ico_dir->bitmap_offset) + ext2fs_le32_to_cpu(ico_dir->bitmap_size);
					}else 
						offset = 0;
				}
				else
					offset = 0;		
				counter-- ;
				ico_dir++ ;
			}
			if ((offset > 22) && (! counter)){
				f_data->size = offset;
				ret = 1;
			}
			break;
	}
	return ret;
}






//tga
int file_tga(unsigned char *buf, int *size, __u32 scan , int flag, struct found_data_t* f_data){
	int 	i,j;
	__u16	*p ;
	__u32	ssize = 0;
	__u32	psize = 0;
	int 	ret = 0;
	unsigned char	token[]="-XFILE.";

	switch (flag){
		case 0 :	
				if ((f_data->size) && (f_data->inode->i_size < f_data->size))
					return 0;
				

				j = strlen(token) -1;
				i = (*size) -1;
				while ((i >= 0) && (j >= 0) && (buf[i] == token[j])){
					i--;
					j--;
				}
				if ((i == -1) || (j == -1)){
					*size = (*size) + 1 ;
					ret=1;
				}
				else{
					if (f_data->size){
						ssize = f_data->size % current_fs->blocksize;
						if (f_data->inode->i_size > (f_data->size - ssize)){
							*size = ssize;
							ret =1;
						}
					}
				}	
			break;
		case 1 :	
			return (scan & (M_IS_META | M_CLASS_1)) ? 0 :1 ;
			break;
		case 2 :
			if ((buf[16]==1) || (buf[16]==8) || (buf[16]==15) || (buf[16]==16) || (buf[16]==24) || (buf[16]==32)){
				if (buf[2] < 4){
					p = (__u16*) (buf + 12);
					ssize = ext2fs_le16_to_cpu(*p);
					p++;
					ssize *= ext2fs_le16_to_cpu(*p);
					switch (buf[16]){
						case 1  : ssize  = 0; break;
						case 15 : ssize *= 2; break;
						default : ssize *= (buf[16] / 8); break;
					}

					if (ssize && (buf[1] == 1)) {
						p = (__u16*) (buf + 5);
						psize = ext2fs_le16_to_cpu(*p) ;
						psize *= ((buf[7]+1) / 8);
						p--;
						psize += ext2fs_le16_to_cpu(*p);
					}
					if (ssize)
						ssize += (psize + 18);
					f_data->size = ssize;
					ret = 1;
				}
			}
			else{
				f_data->func = file_none;
				ret = 0;
			}
			break;
	}
	return ret;
}	



//midi
int file_midi(unsigned char *buf, int *size, __u32 scan , int flag, struct found_data_t* f_data){
	int 	ret = 0;

	switch (flag){
		case 0 :
			if ((*size>1) && (buf[(*size)-1] == (unsigned char)0x2f) && (buf[(*size)-2] == (unsigned char)0xff)){
					*size +=1;
					ret = 1;
			}
			else
				if ((*size == 1) && (buf[(*size)-1] == (unsigned char)0x2f)){
					*size +=1 ;
					ret = 1;
			}
			break;
		case 1 :return (scan & (M_IS_META | M_CLASS_1 | M_BLANK)) ? 0 :1 ;
			break;
	}
	return ret;
}


//swf
int file_swf(unsigned char *buf, int *size, __u32 scan , int flag, struct found_data_t* f_data){
	int 		ret = 0;
	__u32		*p_32;
	__u32		ssize;

	switch (flag){
		case 0 :
			if (f_data->size ) {
				ssize = f_data->size % current_fs->blocksize;
				if (f_data->inode->i_size > (f_data->size - ssize)){
					*size = ssize;
					ret =1;
				}
			}
			else{
				if ((*size) < (current_fs->blocksize - 2)){
					ret=1;
				}
			}
			break;
		case 1:
			return (scan & (M_IS_META | M_CLASS_1 | M_BLANK )) ? 0 :1 ;
			break;
		
		case 2:
			if (buf[0] == 'F'){
				p_32 = (__u32*)(buf+4);
				ssize = ext2fs_le32_to_cpu(*p_32);
				f_data->size = ssize;
				ret = 1;
			}
		break;
	}
	return ret;
}

			

//aiff
int file_aiff(unsigned char *buf, int *size, __u32 scan , int flag, struct found_data_t* f_data){
	int 		ret = 0;
	__u32		*p_32;
	__u32		ssize;

	switch (flag){
		case 0 :
			if (f_data->size ) {
				ssize = f_data->size % current_fs->blocksize;
				if (f_data->inode->i_size > (f_data->size - ssize)){
					*size = ssize;
					ret =1;
				}
			}
			break;
		case 1:
			return (scan & (M_IS_META | M_CLASS_1 | M_BLANK )) ? 0 :1 ;
			break;
		
		case 2:
			if (buf[8] == 'A'){
				p_32 = (__u32*)(buf+4);
				ssize = ext2fs_be32_to_cpu(*p_32) +8;
				f_data->size = ssize;
				ret = 1;
			}
		break;
	}
	return ret;
}


//asf
int file_asf(unsigned char *buf, int *size, __u32 scan , int flag, struct found_data_t* f_data){
static const unsigned char top_header[16]   = { 0x30,0x26,0xb2,0x75,0x8e,0x66,0xcf,0x11,0xa6,0xd9,0x00,0xaa,0x00,0x62,0xce,0x6c };
static const unsigned char data_header[16]  = { 0x36,0x26,0xb2,0x75,0x8e,0x66,0xcf,0x11,0xa6,0xd9,0x00,0xaa,0x00,0x62,0xce,0x6c };
static const unsigned char index[4][4] = {{0x90,0x08,0x00,0x33},{0xd3,0x29,0xe2,0xd6},{0xf8,0x03,0xb1,0xfe},{0xd0,0x3f,0xb7,0x3c}};
	int 		ret = 0;
	unsigned char	*p_offset;
	__u64		offset;
	__u64		obj_size;
	int 		i,j;

	switch (flag){
		case 0 :
			if (f_data->size || f_data->h_size) {
				if (f_data->h_size != f_data->inode->i_size_high)
					ret = 0;
				else{
					if (f_data->inode->i_size < f_data->size)
						ret = 0;
					else{
						*size += 4;
						ret = 1;
					}
				}
			}
			break;
		case 1:
			return (scan & (M_IS_META | M_CLASS_1 | M_BLANK )) ? 0 :1 ;
			break;
		
		case 2:
			offset = 0;
			obj_size = 0;
			if(! memcmp(buf,top_header,16)) {
				obj_size = ext2fs_le64_to_cpu(*(__u64*)(buf+16)); 

				if (obj_size < 30)  return 0;
				offset += obj_size;
				if (offset < (__u64)((12 * current_fs->blocksize) - 23) && (! memcmp(buf +(__u32)offset,data_header,16))){
					p_offset = buf + (offset & 0xffffffff);	
					obj_size = (__u64) p_offset[16] | (p_offset[17]<<8) | (p_offset[18]<<16) |  (p_offset[19]<<24) | 
						   ((__u64)p_offset[20]<<32) | ((__u64)p_offset[21]<<40) | ((__u64)p_offset[22]<<48) | ((__u64)p_offset[23]<<56) ;  
					//obj_size =  ext2fs_le64_to_cpu(*(__u64*)(p_offset+16)); 
					if ( obj_size < 50 ) return 0;
				
				offset += obj_size;
				}	
				while (offset < ((__u64)((12 * current_fs->blocksize) - 23))){
					p_offset = buf + (offset & 0xffffffff);	
					for (i=0 ; i<4 ; i++){
						for (j=0;j<4;j++){
							if (! (p_offset[j] == index[i][j])) 
								break;
						}
						if(j < 4)
							continue;
						break;
					}
					if (i<4){
						obj_size = (__u64) p_offset[16] | (p_offset[17]<<8) | (p_offset[18]<<16) |  (p_offset[19]<<24) | 
						   ((__u64)p_offset[20]<<32) | ((__u64)p_offset[21]<<40) | ((__u64)p_offset[22]<<48) | ((__u64)p_offset[23]<<56) ;  
						//obj_size =  ext2fs_le64_to_cpu(*(__u64*)(p_offset+16));
						offset += obj_size;
					}
					else
						break;
				}
			f_data->size = offset & 0xFFFFFFFF ;
			f_data->h_size = offset >> 32;
			ret = 1;
			break;	
			}	
	}
	return ret;
}



//flac
int file_flac(unsigned char *buf, int *size, __u32 scan , int flag, struct found_data_t* f_data){
	int 	meta_size;
	int 	ret = 0;
	__u32	offset;
static const unsigned char 	token[4]= {0x66,0x4c,0x61,0x43};

switch (flag){
		case 0 :
			if ((f_data->size) && (f_data->size < f_data->inode->i_size)){
				if (*size < current_fs->blocksize-7)
					ret = 1;
			}
			else {
				ret =0;
			}
			break;
		case 1 :
			if (f_data->size > f_data->inode->i_size)
				ret = (scan & (M_IS_META | M_CLASS_1 )) ? 0 :1 ;
			else
				ret = (scan & (M_IS_META | M_CLASS_1 | M_BLANK | M_TXT )) ? 0 :1 ;
			break;
		case 2:
			offset = test_id3_tag(buf);
			if(! memcmp((void*)(buf + offset),token,4)) {
				offset += 4;
				while ((buf[offset] != 0xff) && (offset < ((12*current_fs->blocksize)-4))){
					meta_size = (buf[offset+1]<<16) + (buf[offset+2]<<8) + buf[offset+3] + 4;
					offset += meta_size;
				}
				if ((offset < ((12*current_fs->blocksize)-4)) && (buf[offset] == 0xff)&& ((buf[offset+1] & 0xfc)==0xf8)){
					f_data->size = offset+2;
					ret = 1;
				}
				else 
					f_data->func = file_none;
			}
			break;
	}
	return ret;
}


//mpeg
int file_mpeg(unsigned char *buf, int *size, __u32 scan , int flag, struct found_data_t* f_data){
	int 	i,j;
	int 	ret = 0;
static const unsigned char 	token0[4]= {0x00,0x00,0x01,0xb9};
static const unsigned char	token1[4]= {0x00,0x00,0x01,0xb7};

	switch (flag){
		case 0 :
				j = 3 ; 
				i = (*size) -1;
				while ((i >= 0) && (j >= 0) && (buf[i] == token0[j])){
					i--;
					j--;
				}
				if ((i != -1 ) && (j != -1)){
					j = 3 ; 
					i = (*size) -1;
					while ((i >= 0) && (j >= 0) && (buf[i] == token1[j])){
						i--;
						j--;
					}
				}
				if ((i == -1) || (j == -1)){
					ret=1;
				}
				else{
					if(*size < (current_fs->blocksize - 48))
						ret=4;
					else{
					//possible ????? a stream cat at block-size ; I seen this in test files ????
						if (!(current_fs->super->s_feature_incompat & EXT3_FEATURE_INCOMPAT_EXTENTS) && (*size > (current_fs->blocksize - 2)) && (f_data->inode->i_size > (12 * current_fs->blocksize)) &&
						((f_data->inode->i_blocks - (13 * (current_fs->blocksize / 512))) % 256)  ){
							*size = current_fs->blocksize;
							 ret = 4;
						}
					}
				}
					
			break;
		case 1 :	
			return (scan & (M_IS_META | M_CLASS_1 )) ? 0 :1 ;
			break;
	}
	return ret;
}	


	
//riff
int file_riff(unsigned char *buf, int *size, __u32 scan , int flag, struct found_data_t* f_data){
	__u32		*p_32  ;
	__u32		ssize , offset ;
	int 		ret = 0;

	switch (flag){
		case 0 :
			if ((f_data->size) && (f_data->size <= f_data->inode->i_size)){
				ssize = (f_data->size % current_fs->blocksize);
				*size = (ssize >= *size)? ssize : (*size) + 2;
				ret = 1;
			}
			else { 
				if ( (!f_data->size) && (*size>4) && (buf[(*size)-2] == 'D') && (buf[(*size)-3] == 'N') &&(buf[(*size)-4] == 'E'))
					ret = 1;
				else
					ret =0;
			}
			break;
		case 1 :
			return (scan & (M_IS_META | M_CLASS_1 )) ? 0 :1 ;
			break;
		case 2 :
			offset = test_id3_tag(buf);
			p_32 = (__u32*)(buf + offset);
			p_32++;
			switch (buf[offset + 3]){
				case 'F' :
					ssize =  ext2fs_le32_to_cpu(*p_32) + 8 ;
					break;
				case 'X' :
					ssize =  ext2fs_be32_to_cpu(*p_32) + 8 ;
					break;
				default :
					ssize = 0;
			}
			f_data->size = ssize ? (offset + ssize) : ssize ;
			ret=1;
		break;
	}
	return ret;
}


//psd
int file_psd(unsigned char *buf, int *size, __u32 scan , int flag, struct found_data_t* f_data){
	int 		ret = 0;

	switch (flag){
		case 0 :
			if (*size < (current_fs->blocksize - 16)){
				if ((!buf[(*size)-2]) && (!buf[(*size)-4]))
					(*size)++;
				ret = 1;
			}
			else {
				if (*size < (current_fs->blocksize - 2))
					ret = 2;
			}
			break;
		case 1 :
			return (scan & (M_IS_META | M_CLASS_1)) ? 0 :1 ;
			break;
	}
	return ret;
}


//pnm
int file_pnm(unsigned char *buf, int *size, __u32 scan , int flag, struct found_data_t* f_data){
	int		x,y,d,s ;
	unsigned char	*c, *txt;
	__u32		ssize = 0;
	int 		ret = 0;

switch (flag){
case 0 :
	if (f_data->size ) {
		ssize = f_data->size % current_fs->blocksize;
		if (f_data->inode->i_size > (f_data->size - ssize)){
			*size = ssize;
			ret =1;
		}
	}
	else{
		if ((*size) < (current_fs->blocksize - 16)){
			ret = 2;
		}
	}
	break;
case 1:
	return (scan & (M_IS_META | M_CLASS_1 )) ? 0 :1 ;
	break;

case 2:
	c = buf;
	if (*c =='P'){
		c++;
		txt = c + 1;
		while (isspace(*txt)) txt++;
		while (*txt == '#'){
			s = 0;
			while ((*txt != 0x0a) && (*txt != 0x0d) && (s < 70)){
			 	txt++;
			 	s++;
			}
			if (s == 70) {
				f_data->func = file_none;
				return 0;
			}
			while (isspace(*txt)) txt++;
		}
		x=atoi(txt);
		while (isdigit(*txt)) txt++;
	        txt++;
		y=atoi(txt);
		while (isdigit(*txt)) txt++;
	        txt++;
		if (x && y){
			switch (*c){
				case 49 :
				case 50 :
				case 51 :
					 f_data->func = file_bin_txt;
					 break;
				case 52 :
					ssize = ((x*y)+7) / 8 ;
					ssize += (__u32)(txt -buf);
					break;
				case 53 :
					d = atoi(txt);
					while (isdigit(*txt)) txt++;
	        			txt++;
					if (d && d< 256)
						ssize = 1;
					else
						if(d)
							ssize = 2;
					if (ssize){
						ssize *= (x*y);
						ssize += (__u32)(txt - buf);
					}
					break;
				case 54 :
					d = atoi(txt);
					while (isdigit(*txt)) txt++;
	        			txt++;
					if (d && d< 256)
						ssize = 3;
					else
						if(d)
							ssize = 6;
					if (ssize){
						ssize *= (x*y);
						ssize += (__u32)(txt - buf);
					}
					break;
			}
			if (ssize){
				f_data->size = ssize;
				ret = 1;
			}	
		}
	}
	break;
}
	return ret;
}


// tiff
int file_tiff(unsigned char *buf, int *size, __u32 scan , int flag, struct found_data_t* f_data){
	__u32		*p_32  ;
	__u32		ssize = 0;
	int 		ret = 0;

	switch (flag){
		case 0 :
			if (f_data->size ){
				if ((f_data->inode->i_size > f_data->size) && (*size < (current_fs->blocksize -8))){  //FIXME
					ret = 4;
					if (*size < (current_fs->blocksize - 256))
						ret = 1;
					*size += 8;
				}
			}
			else{
				if (f_data->inode->i_block[12]){   //FIXME for ext4
					*size += 8;
					ret = 2;
				}
			}				
			break;
		case 1 : 
			return (scan & (M_IS_META | M_CLASS_1)) ? 0 :1 ;
			break;
		case 2 :
			p_32 = (__u32*)buf;
			p_32++;
	//precise calculations are very complex, we will see later if we need it for ext4
	//for the moment we take only the offset for the first IDF
			if ((*buf == 0x49) && (*(buf+1) == 0x49))
				ssize = ext2fs_le32_to_cpu(*p_32);
			if ((*buf == 0x4d) && (*(buf+1) == 0x4d))
				ssize = ext2fs_be32_to_cpu(*p_32);
			
			f_data->size = ssize + 120;
			ret = 1;
			
		break;
	}
	return ret;
}
						


//mod
int file_mod(unsigned char *buf, int *size, __u32 scan , int flag, struct found_data_t* f_data){
	int 		ret = 0;

	switch (flag){
		case 0 :
			if (*size < (current_fs->blocksize -4)){
				*size = ((*size) +3) & 0xfffc ;
				ret = 1;
			}
			break;
		case 1 :
			return (scan & (M_IS_META | M_CLASS_1)) ? 0 :1 ;
			break;
	}
	return ret;
}


//CDF    FIXME ????????????????
int file_CDF(unsigned char *buf, int *size, __u32 scan , int flag, struct found_data_t* f_data){
	int 		ret = 0;

	switch (flag){
		case 0 :
			if (*size < (current_fs->blocksize -7) && ((*size ) && buf[(*size)-1] == (unsigned char)0xff )){
			*size = ((*size) +8) ;
			ret = 2;
		}
			break;
		case 1:
			return (scan & (M_IS_META | M_CLASS_1)) ? 0 :1 ;
			break;
	}
	return ret;
}


//vmware    FIXME ????????????????
int file_vmware(unsigned char *buf, int *size, __u32 scan , int flag, struct found_data_t* f_data){
	int 		ret = 0;

	switch (flag){
		case 0 :
			if (f_data->inode->i_block[12]){   //FIXME for ext4
					*size = current_fs->blocksize;
					ret = 1;
			}
			break;
		case 1:
			return (f_data->inode->i_size < (12 * current_fs->blocksize)) ;
			break;
	}
	return ret;
}




//SQLite   FIXME
int file_SQLite(unsigned char *buf, int *size, __u32 scan , int flag, struct found_data_t* f_data){
	int 		ret = 0;
	__u16		*p_16;
	__u32		ssize;

	switch (flag){
		case 0 :
			if (f_data->size ) {
				ssize = f_data->size;
				if (ssize < current_fs->blocksize){
					*size += ssize -1;
					*size &= (~(ssize-1));
					ret = 2;
				}
				else{
					*size = current_fs->blocksize;
					ret = 2;
				}
			}
			break;
		case 1:
			return (scan & (M_IS_META | M_CLASS_1)) ? 0 :1 ;
			break;
		
		case 2:
			p_16 = (__u16*)(buf+16);
			ssize = (__u32)ext2fs_be16_to_cpu(*p_16);
			f_data->size = ssize;
			ret = 1;
		break;
	}
	return ret;
}


//au
int file_au(unsigned char *buf, int *size, __u32 scan , int flag, struct found_data_t* f_data){
static const unsigned char au_header[4]= {'.','s','n','d'};
struct au_header_t
{
	__u32 	magic;
	__u32 	offset;
	__u32 	size;
	__u32 	encoding;
	__u32 	sample_rate;
	__u32 	channels;
};
	int 			ret = 0;
	__u32			ssize;
	struct au_header_t	*p_header;

	switch (flag){
		case 0 :
			if (f_data->size ) {
				ssize = f_data->size % current_fs->blocksize;
				if (f_data->inode->i_size > (f_data->size - ssize)){
					*size = ssize;
					ret =1;
				}
			}
			else{
				if (*size < (current_fs->blocksize -4)){
					*size += 4;
					ret = 4;
				}
			}
			break;
		case 1:
			return (scan & (M_IS_META | M_CLASS_1 | M_TXT)) ? 0 :1 ;
			break;
		
		case 2:
			p_header = (struct au_header_t*) buf;
			if(memcmp(buf,p_header,sizeof(struct au_header_t)) == 0 &&
    			    ext2fs_be32_to_cpu(p_header->encoding)<=27 && ext2fs_be32_to_cpu(p_header->channels)<=256){
				if(ext2fs_be32_to_cpu(p_header->size)!=0xffffffff){
					f_data->size = ext2fs_be32_to_cpu(p_header->offset) + ext2fs_be32_to_cpu(p_header->size);
				}
				ret = 1;
			}
			break;
	}
	return ret;
}



//ra
int file_ra(unsigned char *buf, int *size, __u32 scan , int flag, struct found_data_t* f_data){
static const unsigned char ra_header[4]= { '.', 'r', 'a', 0xfd};
static const unsigned char rm_header[9]  = { '.', 'R', 'M', 'F', 0x00, 0x00, 0x00, 0x12, 0x00};
	int 			ret = 0;
	__u32			ssize;

	switch (flag){
		case 0 :
			if (f_data->size ) {
				ssize = f_data->size % current_fs->blocksize;
				if (f_data->inode->i_size > (f_data->size - ssize)){
					*size = ssize;
					ret =1;
				}
			}
			else{
				if (*size < (current_fs->blocksize -4)){
					*size += 4;
					ret = 4;
				}
			}
			break;
		case 1:
			return (scan & (M_IS_META | M_CLASS_1 | M_TXT)) ? 0 :1 ;
			break;
		
		case 2:
			if(! memcmp(buf,ra_header,4)) {
				if(buf[4]==0x00 && buf[5]==0x03)
					return 1;  //V3
				if(buf[4]==0x00 && buf[5]==0x04 && buf[9]=='r' && buf[10]=='a' && buf[11]=='4'){  //V4
					f_data->size = (buf[12]<<24) + (buf[13]<<16) + (buf[14]<<8) + buf[15] + 40;
					return 1;
				}
			}
			else{
				if(! memcmp(buf,rm_header,9))
					return 1 ;
			}
			break;
	}
	return ret;
}



//QuickTime  
int file_qt(unsigned char *buf, int *size, __u32 scan , int flag, struct found_data_t* f_data){
	int 			i, j, ret = 0;
	__u32			atom_size;
	__u32			offset;
static unsigned char 	basic[18][4]={
{'f','t','y','p'},
{'m','d','a','t'},
{'m','o','o','v'},
{'c','m','o','v'},
{'c','m','v','d'},
{'p','n','o','t'},
{'d','c','o','m'},
{'w','i','d','e'},
{'f','r','e','e'},
{'s','k','i','p'},
{'j','p','2','h'},
{'i','d','s','c'},
{'i','d','a','t'},
{'m','d','i','a'},
{'p','c','k','g'},
{'s','t','b','l'},
{'t','r','a','k'},
{'j','P',' ',' '}};


static unsigned char ftype[10][3]={
{'i','s','o'},
{'m','p','4'},
{'m','m','p'},
{'M','4','A'},
{'M','4','B'},
{'M','4','C'},
{'3','g','p'},
{'3','g','2'},
{'j','p','2'},
{'q','t',' '}};

static unsigned char atom[21][4]={
{'c','m','o','v'},
{'c','m','v','d'},
{'d','c','o','m'},
{'f','r','e','e'},
{'f','t','y','p'},
{'j','p','2','h'},
{'j','p','2','c'},
{'m','d','a','t'},
{'m','d','i','a'},
{'m','o','o','v'},
{'P','I','C','T'},
{'p','n','o','t'},
{'s','k','i','p'},
{'s','t','b','l'},
{'r','m','r','a'},
{'m','e','t','a'},
{'i','d','s','c'},
{'i','d','a','t'},
{'t','r','a','k'},
{'u','u','i','d'},
{'w','i','d','e'}};

switch (flag){
		case 0 :
			if (f_data->size || f_data->h_size) {
				if ((f_data->h_size == 0xff )&& (!f_data->size)){
					if (*size < (current_fs->blocksize - 4)){
						ret = 4;
						break;
					}
				}
				if (f_data->h_size != f_data->inode->i_size_high)
					ret = 0;
				else{
					if (f_data->inode->i_size < f_data->size)
						ret = 0;
					else{
						ret = 1;
					}
				}
			}
			else {
				ret =0;
			}
			break; 
		case 1 :
			return (scan & (M_IS_META | M_CLASS_1 | M_BLANK | M_TXT)) ? 0 :1 ;
			break;
		case 2 :

offset = 0;
atom_size=(buf[offset+0]<<24)+(buf[offset+1]<<16)+(buf[offset+2]<<8)+buf[offset+3];
for (i=0;i<18;i++){
	if(memcmp((void*)(buf + offset +4), basic[i], 4))
		continue;
	if((!atom_size) && (i == 1)){ //FIXME
		f_data->h_size = 0xff;
#ifdef DEBUG_QUICK_TIME
		fprintf(stderr,"QuickTime : found a \"mdat\" atom at begin , block %lu\n",f_data->first);
#endif
		return 1;
	}
	if(atom_size == 1){
		f_data->size   = (buf[offset+12]<<24)+(buf[offset+13]<<16)+(buf[offset+14]<<8)+buf[offset+15];
		f_data->h_size = (buf[offset+8]<<24)+(buf[offset+9]<<16)+(buf[offset+10]<<8)+buf[offset+11];
#ifdef DEBUG_QUICK_TIME
		fprintf(stderr,"QuickTime : found a large atom, block %lu ; offset size :%llu\n",f_data->first,
				 ((__u64)f_data->h_size<<32) + f_data->size + offset);
#endif
		return 1;
	}
	if(!i){
		for (j=0;j<10;j++){
			if(memcmp((void*)(buf + offset +8), ftype[j], 3))
			continue;
#ifdef DEBUG_QUICK_TIME
			fprintf(stderr,"QuickTime : Type \"%c%c%c\"\n",ftype[j][0],ftype[j][1],ftype[j][2]);
#endif
		//FIXME
		}
		if (i == 10){
#ifdef DEBUG_QUICK_TIME
			fprintf(stderr,"QuickTime : Type : \"%c%c%c%c\" is unknown\n",buf[offset +4],buf[offset +5],buf[offset+6],buf[offset+7]);
#endif
			//return 0;
		}
	}
	offset += atom_size;
	break;
}
if (i == 18){
#ifdef DEBUG_QUICK_TIME
	fprintf(stderr,"QuickTime : first Container atom unknown ; block %lu ; size %lu\n",f_data->first, atom_size);
#endif
	return 0;
}

while ((offset < ((12 * current_fs->blocksize)-8)) && (buf[offset+4])){
	atom_size=(buf[offset+0]<<24)+(buf[offset+1]<<16)+(buf[offset+2]<<8)+buf[offset+3];
	for (i=0;i<21;i++){
		if(memcmp((void*)(buf + offset +4), atom[i], 4))
			continue;
		if ((!atom_size ) && (i == 6)){
			f_data->func = file_jpeg;
#ifdef DEBUG_QUICK_TIME
			fprintf(stderr,"QuickTime : found JP-2000, block %lu ; offset size :%llu\n",f_data->first,
				 ((__u64)f_data->h_size<<32) + f_data->size + offset);
#endif
			return 1;
		}
		if (atom_size == 1){
			f_data->size   = (buf[offset+12]<<24)+(buf[offset+13]<<16)+(buf[offset+14]<<8)+buf[offset+15];
			f_data->h_size = (buf[offset+8]<<24)+(buf[offset+9]<<16)+(buf[offset+10]<<8)+buf[offset+11];
#ifdef DEBUG_QUICK_TIME
			fprintf(stderr,"QuickTime : found a large atom, block %lu ; offset size :%llu\n",f_data->first,
				 ((__u64)f_data->h_size<<32) + f_data->size + offset);
#endif
			return 1;
		}
		offset += atom_size;
#ifdef DEBUG_QUICK_TIME
			fprintf(stderr,"QuickTime : atom \"%c%c%c%c\" ; size %lu\n", atom[i][0],atom[i][1],atom[i][2],atom[i][3],offset);
#endif
		break;
	}
	if (i == 21){
#ifdef DEBUG_QUICK_TIME
		fprintf(stderr,"QuickTime : atom \"%c%c%c%c\" unknown ; block  %lu ; offset %lu , size %lu\n", 
			atom[i][0],atom[i][1],atom[i][2],atom[i][3], f_data->first, offset, atom_size);
#endif
		return 0;
	}	
}
#ifdef DEBUG_QUICK_TIME
	fprintf(stderr,"QuickTime : found ;  block  %lu ; offset %lu\n", f_data->first, offset);
#endif
	f_data->size = offset;
	ret = 1;
	break;
}
return ret;
}




//fli
int file_fli(unsigned char *buf, int *size, __u32 scan , int flag, struct found_data_t* f_data){
	int 			ret = 0;
	__u32			ssize;

	switch (flag){
		case 0 :
			if (f_data->size ) {
				ssize = f_data->size % current_fs->blocksize;
				if (f_data->inode->i_size > (f_data->size - ssize)){
					*size = ssize;
					ret =1;
				}
			}
			
			break;
		case 1:
			return (scan & (M_IS_META | M_CLASS_1 | M_TXT)) ? 0 :1 ;
			break;
		
		case 2:
			f_data->size = (buf[3]<<24) + (buf[2]<<16) + (buf[1]<<8) + buf[0] ;
			ret =  1;
			break;
	}
	return ret;
}


//ogg  
int file_ogg(unsigned char *buf, int *size, __u32 scan , int flag, struct found_data_t* f_data){
	
	int 			i, ret = 0;
	unsigned char 		token[7]= {0x80, 't', 'h', 'e', 'o', 'r', 'a'};
	unsigned char		*ogg_h;
	__u8			*t_pointer;	
	__u32			frame_offset = 0;
	__u32			seq_len;

	
	switch (flag){
		case 0 :
			if ((f_data->size) && (f_data->size <= f_data->inode->i_size)){
				if( f_data->size < (12 * current_fs->blocksize)){
					f_data->inode->i_size = (f_data->size + current_fs->blocksize -1) & ~(current_fs->blocksize-1);
					*size = f_data->size % current_fs->blocksize;
				}
				else
					*size += 2;
				ret = 1;
			}
			else {
				ret =0;
			}

			break;
		case 1 : 
			return (scan & (M_IS_META | M_CLASS_1 | M_BLANK | M_TXT)) ? 0 :1 ;
		break;
		
		case 2 :
			frame_offset = test_id3_tag(buf);
			ogg_h = (buf + frame_offset) ;
			if (!(ogg_h[5] & 0x02)){
#ifdef DEBUG_OGG_STREAM
				fprintf(stderr,"OGG : Block %8lu is sequence %lu and not begin of a file\n",f_data->first, 
					(ogg_h[18] | (ogg_h[19]<<8) | (ogg_h[20]<<16) | (ogg_h[21]<<24))); 
#endif
				f_data->func = file_none;
				return 0;
			}
			while (frame_offset < ((12 * current_fs->blocksize)-27)){
				if ( ogg_h[5] & 0x4){
					ret = 1;
				}
				ogg_h = (buf + frame_offset);
				seq_len = 27;
				i = 0;
				if(!(memcmp((void*)(buf + frame_offset +28), token, 7)))
				f_data->name[strlen(f_data->name)-1] = 'm' ;
				t_pointer = (__u8*) (buf + frame_offset + seq_len);
				
				if ((ogg_h[0] == 0x4f)&&(ogg_h[1] == 0x67)&&(ogg_h[2] == 0x67) &&(ogg_h[3] == 0x53)){
					for (i = 0; i < ogg_h[26] ;i++, t_pointer++){
						if ((frame_offset + 27 + i) >= (12 * current_fs->blocksize))
							break;
						seq_len += *t_pointer;
					}				
				}
				else {
					if (ret && ogg_h[0]) 
						ret = 0;
					break;
				}
				
				frame_offset += seq_len + ogg_h[26];
#ifdef DEBUG_OGG_STREAM
				fprintf(stderr,"OGG-STREAM: Block %8lu : serial number %12lu : size %6lu : offset %6lu \n",f_data->first,
				 (ogg_h[14] | (ogg_h[15]<<8) | (ogg_h[16]<<16) | (ogg_h[17]<<24)), seq_len , frame_offset); 
#endif
					
			}
			if( ret || ((frame_offset + 27 + i) >= (12 * current_fs->blocksize))){
				f_data->size = frame_offset;
				ret = 1;
			}
			else 
				f_data->func = file_none;
	}
	return ret;
}



//mp3
int file_mp3(unsigned char *buf, int *size, __u32 scan , int flag, struct found_data_t* f_data){
#define MPEG_V25        0
#define MPEG_V2         2
#define MPEG_V1         3
#define MPEG_L3 	0x01
#define MPEG_L2 	0x02
#define MPEG_L1 	0x03
int	ret = 0;
__u32	frame_offset = 0 ;
int	frame_flag = 0;
int 	mpeg_version;
int 	mpeg_layer;
int 	bit_rate_key;
int 	sampling_rate_key;
int 	padding;
int 	bit_rate;
int 	sample_rate;
__u32 	frameLength;
int 	i=1023;
unsigned char head[5]={0,0,0,0,0};


static const unsigned int sample_rate_table[4][4]={
  {11025, 12000,  8000, 0},     /* MPEG_V25 */
  {    0,     0,     0, 0},
  {22050, 24000, 16000, 0},     /* MPEG_V2 */
  {44100, 48000, 32000, 0}      /* MPEG_V1 */
};

static const unsigned int bit_rate_table[4][4][16]=
{
/* MPEG_V25 */
  {
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    { 0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, 0},
    { 0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, 0},
    { 0, 32, 48, 56, 64, 80, 96, 112, 128, 144, 160, 176, 192, 224, 256, 0}
  },
/* MPEG_INVALID */
  {
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  },
  /* MPEG_V2 */
  {
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    { 0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, 0},
    { 0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, 0},
    { 0, 32, 48, 56, 64, 80, 96, 112, 128, 144, 160, 176, 192, 224, 256, 0}
  },
  /* MPEG_V1 */
  {
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    { 0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 0},
    { 0, 32, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384, 0},
    { 0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 294, 416, 448, 0}
  }
};


switch (flag){
		case 0 :
			if ((f_data->size) && (f_data->size <= f_data->inode->i_size)){
				if( f_data->size < (12 * current_fs->blocksize)){
					f_data->inode->i_size = (f_data->size + current_fs->blocksize -1) & ~(current_fs->blocksize-1);
					*size = f_data->size % current_fs->blocksize;
				}
				ret = 1;
			}
			else {
				ret =0;
			}
			break;
		case 1 :
			return (scan & (M_IS_META | M_CLASS_1 |  M_BLANK )) ? 0 :1 ;
			break;
		case 2 :

	frameLength       = 0;
	frame_offset	= test_id3_tag(buf);
	while (frame_offset < (12 * current_fs->blocksize)){
		if((buf[frame_offset + 0]==0xFF && ((buf[frame_offset + 1]&0xFE)==0xFA || (buf[frame_offset + 1]&0xFE)==0xF2 || (buf[frame_offset + 1]&0xFE)==0xE2))){
			mpeg_version      = (buf[frame_offset + 1]>>3) & 0x03;
			mpeg_layer        = (buf[frame_offset + 1]>>1) & 0x03;
			bit_rate_key      = (buf[frame_offset + 2]>>4) & 0x0F;
			sampling_rate_key = (buf[frame_offset + 2]>>2) & 0x03;
			padding           = (buf[frame_offset + 2]>>1) & 0x01;
			bit_rate          = bit_rate_table[mpeg_version][mpeg_layer][bit_rate_key];
			sample_rate       = sample_rate_table[mpeg_version][sampling_rate_key];
			frameLength       = 0;
	
			if(sample_rate==0 || bit_rate==0 || mpeg_layer==MPEG_L1){
				f_data->func = file_none;
				return 0;
			}
			if(mpeg_layer==MPEG_L3)	{
				if(mpeg_version==MPEG_V1)
					frameLength = 144000 * bit_rate / sample_rate + padding;
				else
					frameLength = 72000 * bit_rate / sample_rate + padding;
			}
			else{
				if(mpeg_layer==MPEG_L2)
					frameLength = 144000 * bit_rate / sample_rate + padding;
				else
					frameLength = (12000 * bit_rate / sample_rate + padding) * 4;
			}
//#ifdef DEBUG_MAGIC_MP3_STREAM
//			fprintf(stderr,"MP3-STREAM %8u-> framesize: %u, layer: %u, bitrate: %u, padding: %u\n", 
//					frame_flag, frameLength, 4-mpeg_layer, bit_rate, padding);
//#endif
			if( ! frameLength ){
				f_data->func = file_none;
				return 0;
			}
			
			if (! frame_flag) {
				head[0] = buf[frame_offset];
				head[1] = buf[frame_offset +1];
				head[2] = buf[frame_offset +2];
				head[3] = (unsigned char)((frameLength-padding) >>8);
				head[4] = (unsigned char)((frameLength-padding) & 0xff);
			}

			frame_offset += frameLength;
			frame_flag++;
		}
		else{
			if( ! ( buf[frame_offset + 0]) && ( ! ( buf[frame_offset + 1]) )){
#ifdef DEBUG_MAGIC_MP3_STREAM
				fprintf (stderr,"\nMP3-END : blk %lu : offset %lu : last_frame %lu\n", f_data->first, frame_offset, frameLength);
#endif
				break;
			}
			if((buf[frame_offset + 0] =='T') && (buf[frame_offset + 1] == 'A') && (buf[frame_offset + 2] == 'G')){
				frame_offset += 128 ;
#ifdef DEBUG_MAGIC_MP3_STREAM
				fprintf (stderr,"\nMP3-TAG-END : blk %lu : offset %lu : last_frame %lu\n", f_data->first, frame_offset, frameLength);
#endif
				break;
			}
			else {
#ifdef DEBUG_MAGIC_MP3_STREAM
				fprintf (stderr,"MP3-STREAM_END : blk %lu : offset %lu : last_frame %lu\n", f_data->first, frame_offset, frameLength);
				blockhex(stderr,(void*)(buf+frame_offset),0,64);
#endif
				return 0;
			}
		}	

	}
	if (frame_flag > 2){
#ifdef DEBUG_MAGIC_MP3_STREAM
		fprintf(stderr,"MP3-STREAM layer: %u, bitrate: %u, testet last offset %u\n", 
					 4-mpeg_layer, bit_rate,frame_offset );
#endif
		f_data->size = frame_offset;
		if (head[0]){
			__u16		 reverse = (__u16)(head[3]<<8) + head[4];
			__u32 		 offset = current_fs->blocksize;
			unsigned char	 *v_buf = buf - offset; 
			if (reverse < (offset -1)){
				if (((v_buf[offset-reverse] == head[0]) && (v_buf[offset-reverse +1 ] == head[1]) && (v_buf[offset-reverse+2] == head[2] & ~0x2)) ||
				((v_buf[offset-reverse-1] == head[0]) && (v_buf[offset-reverse] == (head[1])) && (v_buf[offset-reverse+1] == head[2] | 0x2))){
#ifdef DEBUG_MAGIC_MP3_STREAM
					fprintf(stderr,"MP3-CHECK: Block %lu : is mp3-data but not begin of file\n", f_data->first);
					blockhex(stderr,(void*)(v_buf+offset-reverse-16),0,64);
#endif
					f_data->func = file_none;
				}
			}
		}
		return 1;
	}
	break;	
} //switch

return ret;
}//end mp3


//change this only carefully
//Although the scanner is controlled here, but you can not directly configure whether a file is found or not.
//This function has a strong influence on the accuracy of the result.
void get_file_property(struct found_data_t* this){
	switch (this->type){
	//Application
		case 0x0101     :               //dicom
	//              this->func = file_dicom ;
	//              strncat(this->name,".dicom",7);
		break;
	
		case 0x0102     :               //mac-binhex40
	//              this->func = file_mac-binhex40 ;
		strncat(this->name,".hqx",7);
		break;
	
		case 0x0103     :               //msword
	//              this->func = file_msword ;
		strncat(this->name,".doc",7);
		break;
	
		case 0x0104     :               //octet-stream
	//              this->func = file_octet-stream;
		strncat(this->name,".unknown",8);
		break;
	
		case 0x0105     :               //ogg
	              this->func = file_ogg ;
		strncat(this->name,".ogg",7);
		break;
	
		case 0x0106     :               //pdf
	              this->func = file_pdf ;
		strncat(this->name,".pdf",7);
		break;
	
		case 0x0107     :               //pgp
	//              this->func = file_pgp ;
		strncat(this->name,".pgp",7);
		break;
	
		case 0x0108     :               //pgp-encrypted
	//              this->func = file_pgp-encrypted ;
		strncat(this->name,".pgp-encrypted",7);
		break;
	
		case 0x0109     :               //pgp-keys
	//              this->func = file_pgp-keys ;
		strncat(this->name,".key",7);
		break;
	
		case 0x010a     :               //pgp-signature
	//              this->func = file_pgp-signature ;
		strncat(this->name,".pgp-sig",7);
		break;
	
		case 0x010b     :               //postscript
	              this->func = file_ps ;
		strncat(this->name,".ps",7);
		break;
	
		case 0x010c     :               //unknown+zip
		this->func = file_zip ;
		strncat(this->name,".zip",7);
		break;
	
		case 0x010d     :               //vnd.google-earth.kml+xml
	//              this->func = file_vnd.google-earth.kml+xml ;
	//              strncat(this->name,".vnd.google-earth.kml+xml",7);
		break;
	
		case 0x010e     :               //vnd.google-earth.kmz
	//              this->func = file_vnd.google-earth.kmz ;
	//              strncat(this->name,".vnd.google-earth.kmz",7);
		break;
	
		case 0x010f     :               //vnd.lotus-wordpro
	//              this->func = file_vnd.lotus-wordpro ;
	//              strncat(this->name,".vnd.lotus-wordpro",7);
		break;
	
		case 0x0110     :               //vnd.ms-cab-compressed
	//              this->func = file_vnd.ms-cab-compressed ;
		strncat(this->name,".cab",7);
		break;
	
		case 0x0111     :               //vnd.ms-excel
	//              this->func = file_vnd.ms-excel ;
		strncat(this->name,".xls",7);
		break;
	
		case 0x0112     :               //vnd.ms-tnef
	//              this->func = file_vnd.ms-tnef ;
	              strncat(this->name,".dat",7);
		break;
	
		case 0x0113     :               //vnd.oasis.opendocument.
	             this->func = file_zip ;
			if (strstr(this->scan_result,"text")){
		        	strncat(this->name,".odt",7);
				break;
			}
			if (strstr(this->scan_result,"presentation")){
		        	strncat(this->name,".odp",7);
				break;
			}
			if (strstr(this->scan_result,"spreadsheet")){
		        	strncat(this->name,".ods",7);
				break;
			}
			if (strstr(this->scan_result,"graphics")){
		        	strncat(this->name,".odg",7);
				break;
			}
		break;
	
		case 0x0114     :               //vnd.rn-realmedia
	              this->func = file_ra ;
	              strncat(this->name,".ra",7);
		break;
	
		case 0x0115     :               //vnd.symbian.install
	//              this->func = file_vnd.symbian.install ;
	              strncat(this->name,".sis",7);
		break;
	
		case 0x0116     :               //x-123
	//              this->func = file_x-123 ;
	              strncat(this->name,".123",7);
		break;
	
		case 0x0117     :               //x-adrift
	//              this->func = file_x-adrift ;
	//              strncat(this->name,".x-adrift",7);
		break;
	
		case 0x0118     :               //x-archive 
			this->func = file_object ;
			strncat(this->name,".a",7);
		break;
	
		case 0x0119     :               //x-arc   deaktiv
	//              this->func = file_x-arc ;
	              strncat(this->name,".arc",7);
		break;
	
		case 0x011a     :               //x-arj
	              this->func = file_arj ;      
	              strncat(this->name,".arj",7);
		break;
	
		case 0x011b     :               //x-bittorrent
	//              this->func = file_x-bittorrent ;
	//              strncat(this->name,".x-bittorrent",7);
		break;
	
		case 0x011c     :               //x-bzip2
		this->func = file_gzip ;   //FIXME
		strncat(this->name,".bzip2",7);
		break;
	
		case 0x011d     :               //x-compress
	//              this->func = file_x-compress ;
	              strncat(this->name,".Z",7);
		break;
	
		case 0x011e     :               //x-coredump
	//              this->func = file_x-coredump ;
		strncat(this->name,".core",7);
		break;
	
		case 0x011f     :               //x-cpio
		this->func = file_cpio ;
		strncat(this->name,".cpio",7);
		break;
	
		case 0x0120     :               //x-dbf
	//              this->func = file_x-dbf ;
	              strncat(this->name,".dbf",7);
		break;
	
		case 0x0121     :               //x-dbm
	//              this->func = file_x-dbm ;
	              strncat(this->name,".dbm",7);
		break;
	
		case 0x0122     :               //x-debian-package
	//              this->func = file_x-debian-package ;
	              strncat(this->name,".deb",7);
		break;
	
		case 0x0123     :               //x-dosexec
	//              this->func = file_x-dosexec ;
		strncat(this->name,".exe",7);
		break;
	
		case 0x0124     :               //x-dvi
	//              this->func = file_x-dvi ;
		strncat(this->name,".dvi",7);
		break;
	
		case 0x0125     :               //x-eet
	//              this->func = file_x-eet ;
	              strncat(this->name,".eet",7);
		break;
	
		case 0x0126     :               //x-elc
	//              this->func = file_x-elc ;
	              strncat(this->name,".elc",7);
		break;
	
		case 0x0127     :               //x-executable
		this->func = file_binary ;
		strncat(this->name,".bin",7);
		break;
	
		case 0x0128     :               //x-gdbm
	//              this->func = file_x-gdbm ;
	              strncat(this->name,".dbm",7);
		break;
	
		case 0x0129     :               //x-gnucash
	//              this->func = file_x-gnucash
	              strncat(this->name,".xml",7);
		break;
	
		case 0x012a     :               //x-gnumeric
	//              this->func = file_x-gnumeric ;
	              strncat(this->name,".gnm",7);
		break;
	
		case 0x012b     :               //x-gnupg-keyring   deaktiv
	//              this->func = file_x-gnupg-keyring ;
	//              strncat(this->name,".x-gnupg-keyring",7);
		break;
	
		case 0x012c     :               //x-gzip
		this->func = file_gzip ;
		strncat(this->name,".gz",7);
		break;
	
		case 0x012d     :               //x-hdf
	//              this->func = file_x-hdf ;
		strncat(this->name,".hdf",7);
		break;
	
		case 0x012e     :               //x-hwp
	//              this->func = file_x-hwp ;
	              strncat(this->name,".hwp",7);
		break;
	
		case 0x012f     :               //x-ichitaro4
	//              this->func = file_x-ichitaro4 ;
	              strncat(this->name,".JDT",7);
		break;
	
		case 0x0130     :               //x-ichitaro5
	//              this->func = file_x-ichitaro5 ;
	              strncat(this->name,".JDT",7);
		break;
	
		case 0x0131     :               //x-ichitaro6
	//              this->func = file_x-ichitaro6 ;
	              strncat(this->name,".JDT",7);
		break;
	
		case 0x0132     :               //x-iso9660-image
		this->func = file_iso9660 ;
		strncat(this->name,".iso",7);
		break;
	
		case 0x0133     :               //x-java-applet
	//              this->func = file_x-java-applet ;
		strncat(this->name,".jar",7);
		break;
	
		case 0x0134     :               //x-java-jce-keystore
	//              this->func = file_x-java-jce-keystore ;
	//              strncat(this->name,".x-java-jce-keystore",7);
		break;
	
		case 0x0135     :               //x-java-keystore
	//              this->func = file_x-java-keystore ;
	//              strncat(this->name,".x-java-keystore",7);
		break;
	
		case 0x0136     :               //x-java-pack200
	//              this->func = file_x-java-pack200 ;
	//              strncat(this->name,".x-java-pack200",7);
		break;
	
		case 0x0137     :               //x-kdelnk
	//              this->func = file_x-kdelnk ;
	//              strncat(this->name,".x-kdelnk",7);
		break;
	
		case 0x0138     :               //x-lha
	//              this->func = file_x-lha ;
		strncat(this->name,".lzh",7);
		break;
	
		case 0x0139     :               //x-lharc
	//              this->func = file_x-lharc ;
	              strncat(this->name,".lzs",7);
		break;
	
		case 0x013a     :               //x-lzip
	//              this->func = file_x-lzip ;
	              strncat(this->name,".lz",7);
		break;
	
		case 0x013b     :               //x-mif
	//              this->func = file_x-mif ;
		strncat(this->name,".mif",7);
		break;
	
		case 0x013c     :               //xml
		this->func =  file_txt;
		strncat(this->name,".xml",7);
		break;
	
		case 0x013d     :               //xml-sitemap
		this->func = file_txt ;
		strncat(this->name,".xml",7);
		break;
	
		case 0x013e     :               //x-msaccess
	//              this->func = file_x-msaccess ;
		strncat(this->name,".mdb",7);
		break;
	
		case 0x013f     :               //x-ms-reader
	//              this->func = file_x-ms-reader ;
	//              strncat(this->name,".x-ms-reader",7);
		break;
	
		case 0x0140     :               //x-object
		this->func = file_object ;
		strncat(this->name,".o",7);
		break;
	
		case 0x0141     :               //x-pgp-keyring    deaktiv
	//              this->func = file_x-pgp-keyring ;
	//              strncat(this->name,".x-pgp-keyring",7);
		break;
	
		case 0x0142     :               //x-quark-xpress-3
	//              this->func = file_x-quark-xpress-3 ; 
	              strncat(this->name,".qxp",7);
		break;
	
		case 0x0143     :               //x-quicktime-player
	              this->func = file_qt;
	              strncat(this->name,".qt",7);
		break;
	
		case 0x0144     :               //x-rar
	              this->func = file_rar ;
		strncat(this->name,".rar",7);
		break;
	
		case 0x0145     :               //x-rpm
	//              this->func = file_x-rpm ;
		strncat(this->name,".rpm",7);
		break;
	
		case 0x0146     :               //x-sc
	//              this->func = file_x-sc ;
	              strncat(this->name,".sc",7);
		break;
	
		case 0x0147     :               //x-setupscript
	//              this->func = file_x-setupscript ;
	//              strncat(this->name,".x-setupscript",7);
		break;
	
		case 0x0148     :               //x-sharedlib
		this->func = file_binary ;
		strncat(this->name,".so",7);
		break;
	
		case 0x0149     :               //x-shockwave-flash
	              this->func = file_swf ;
		strncat(this->name,".swf",7);
		break;
	
		case 0x014a     :               //x-stuffit
	//              this->func = file_x-stuffit ;
	              strncat(this->name,".sit",7);
		break;
	
		case 0x014b     :               //x-svr4-package
	//              this->func = file_x-svr4-package ;
	//              strncat(this->name,".x-svr4-package",7);
		break;
	
		case 0x014c     :               //x-tar
		this->func = file_tar ;
		strncat(this->name,".tar",7);
		break;
	
		case 0x014d     :               //x-tex-tfm
	//              this->func = file_x-tex-tfm ;
	              strncat(this->name,".tfm",7);
		break;
	
		case 0x014e     :               //x-tokyocabinet-btree
	//              this->func = file_x-tokyocabinet-btree ;
	//              strncat(this->name,".x-tokyocabinet-btree",7);
		break;
	
		case 0x014f     :               //x-tokyocabinet-fixed
	//              this->func = file_x-tokyocabinet-fixed ;
	//              strncat(this->name,".x-tokyocabinet-fixed",7);
		break;
	
		case 0x0150     :               //x-tokyocabinet-hash
	//              this->func = file_x-tokyocabinet-hash ;
	//              strncat(this->name,".x-tokyocabinet-hash",7);
		break;
	
		case 0x0151     :               //x-tokyocabinet-table
	//              this->func = file_x-tokyocabinet-table ;
	//              strncat(this->name,".x-tokyocabinet-table",7);
		break;
	
		case 0x0152     :               //x-xz
	              this->func = file_xz ;
	              strncat(this->name,".xz",7);
		break;
	
		case 0x0153     :               //x-zoo
	//              this->func = file_x-zoo ;
	              strncat(this->name,".zoo",7);
		break;
	
		case 0x0154     :               //zip
		this->func = file_zip ;
		strncat(this->name,".zip",7);
		break;

		case 0x0155     :               //x-font-ttf
	             this->func = file_ttf ;
	             strncat(this->name,".ttf",7);
		break;
	
	//----------------------------------------------------------------
	//Audio
		case 0x0201     :               //basic
	              this->func = file_au ;
		strncat(this->name,".au",7);
		break;
	
		case 0x0202     :               //midi
	             this->func = file_midi ;
		strncat(this->name,".mid",7);
		break;
	
		case 0x0203     :               //mp4
		     this->func = file_qt ;
		strncat(this->name,".mp4",7);
		break;
	
		case 0x0204     :               //mpeg
	              this->func = file_mp3 ;
		strncat(this->name,".mp3",7);
		break;
	
		case 0x0205     :               //x-adpcm
	              this->func = file_au ;
	              strncat(this->name,".au",7);
		break;
	
		case 0x0206     :               //x-aiff
	              this->func = file_aiff ;
		strncat(this->name,".aiff",7);
		break;
	
		case 0x0207     :               //x-dec-basic
	//              this->func = file_x-dec-basic ;
	//              strncat(this->name,".x-dec-basic",7);
		break;
	
		case 0x0208     :               //x-flac
	              this->func = file_flac ;
	              strncat(this->name,".flac",7);
		break;
	
		case 0x0209     :               //x-hx-aac-adif        
	//              this->func = file_x-hx-aac-adif ;
		strncat(this->name,".aac",7);
		break;
	
		case 0x020a     :               //x-hx-aac-adts        deaktiv
	//              this->func = file_x-hx-aac-adts ;
		strncat(this->name,".aac",7);
		break;
	
		case 0x020b     :               //x-mod
	             this->func = file_mod ;
	              strncat(this->name,".mod",7);
		break;
	
		case 0x020c     :               //x-mp4a-latm        deaktiv 
	//              this->func = file_x-mp4a-latm ;
	//              strncat(this->name,".x-mp4a-latm",7);
		break;
	
		case 0x020d     :               //x-pn-realaudio
	              this->func = file_ra;
		strncat(this->name,".ra",7);
		break;
	
		case 0x020e     :               //x-unknown
	//              this->func = file_x-unknown ;
	              strncat(this->name,".voc",7);
		break;
	
		case 0x020f     :               //x-wav
	              this->func = file_riff ;
		strncat(this->name,".wav",7);
		break;
	
	
	//----------------------------------------------------------------
	//Images
		case 0x0301     :               //gif
	              this->func = file_gif ;
		strncat(this->name,".gif",7);
		break;
	
		case 0x0302     :               //jp2
	              this->func = file_jpeg ;
		strncat(this->name,".jp2",7);
		break;
	
		case 0x0303     :               //jpeg
	              this->func = file_jpeg ;
		strncat(this->name,".jpg",7);
		break;
	
		case 0x0304     :               //png
	             this->func = file_png ;
		strncat(this->name,".png",7);
		break;
	
		case 0x0305     :               //svg+xml
	              this->func = file_txt ;
	              strncat(this->name,".svg",7);
		break;
	
		case 0x0306     :               //tiff
	              this->func = file_tiff ;
		strncat(this->name,".tif",7);
		break;
	
		case 0x0307     :               //vnd.adobe.photoshop
	              this->func = file_psd;
	              strncat(this->name,".psd",7);
		break;
	
		case 0x0308     :               //vnd.djvu
	//              this->func = file_vnd.djvu ;
	              strncat(this->name,".djv",7);
		break;
	
		case 0x0309     :               //x-coreldraw
	              this->func = file_riff ;
	              strncat(this->name,".cdr",7);
		break;
	
		case 0x030a     :               //x-cpi
	//              this->func = file_x-cpi ;
	              strncat(this->name,".cpi",7);
		break;
	
		case 0x030b     :               //x-ico
	              this->func = file_ico ;
		strncat(this->name,".ico",7);
		break;
	
		case 0x030c     :               //x-ms-bmp
	              this->func = file_bmp ;
		strncat(this->name,".bmp",7);
		break;
	
		case 0x030d     :               //x-niff
	              this->func = file_riff ;
	              strncat(this->name,".niff",7);
		break;
	
		case 0x030e     :               //x-portable-bitmap
	              this->func = file_pnm ;
		strncat(this->name,".pbm",7);
		break;
	
		case 0x030f     :               //x-portable-greymap
	              this->func = file_pnm ;
		strncat(this->name,".pgm",7);
		break;
	
		case 0x0310     :               //x-portable-pixmap
	              this->func = file_pnm ;
		strncat(this->name,".ppm",7);
		break;
	
		case 0x0311     :               //x-psion-sketch
	//              this->func = file_x-psion-sketch ;
	//              strncat(this->name,".x-psion-sketch",7);
		break;
	
		case 0x0312     :               //x-quicktime
	              this->func = file_qt ;
	              strncat(this->name,".qtif",7);
		break;
	
		case 0x0313     :               //x-unknown
	//              this->func = file_x-unknown ;
	              strncat(this->name,".unknown",8);
		break;
	
		case 0x0314     :               //x-xcursor
	//              this->func = file_x-xcursor ;
	              strncat(this->name,".cur",7);
		break;
	
		case 0x0315     :               //x-xpmi
	//              this->func = file_x-xpmi ;
	              strncat(this->name,".xpm",7);
		break;
	
		case 0x0316     :               //x-tga
	              this->func = file_tga ;
	              strncat(this->name,".tga",7);
		break;
	
	
	//----------------------------------------------------------------
	//Messages
		case 0x0401     :               //news
	              this->func = file_txt ;
	//	strncat(this->name,".news",7);
		break;
	
		case 0x0402     :               //rfc822
	              this->func = file_txt ;
	//              strncat(this->name,".rfc822",7);
		break;
	
	
	//----------------------------------------------------------------
	//Model
		case 0x0501     :               //vrml
	//              this->func = file_vrml ;
	//              strncat(this->name,".vrml",7);
		break;
	
		case 0x0502     :               //x3d
	//              this->func = file_x3d ;
	//              strncat(this->name,".x3d",7);
		break;
	
	
	//----------------------------------------------------------------
	//Text
		case 0x0601     :               //html
		this->func = file_txt ;
		strncat(this->name,".html",7);
		break;
	
		case 0x0602     :               //PGP
		this->func = file_txt ;
	              strncat(this->name,".pgp",7);
		break;
	
		case 0x0603     :               //rtf
		this->func = file_txt ;
		strncat(this->name,".rtf",7);
		break;
	
		case 0x0604     :               //texmacs
	//              this->func = file_texmacs ;
	              strncat(this->name,".tm",7);
		break;
	
		case 0x0605     :               //troff
		this->func = file_txt ;
		strncat(this->name,".man",7);
		break;
	
		case 0x0606     :               //vnd.graphviz
	//              this->func = file_vnd.graphviz ;
	              strncat(this->name,".gv",7);
		break;
	
		case 0x0607     :               //x-awk
		this->func = file_txt ;
		strncat(this->name,".awk",7);
		break;
	
		case 0x0608     :               //x-diff
		this->func = file_txt ;
		strncat(this->name,".diff",7);
		break;
	
		case 0x0609     :               //x-fortran  
		this->func = file_txt ;
	              strncat(this->name,".f",7);
		break;
	
		case 0x060a     :               //x-gawk
		this->func = file_txt ;
		strncat(this->name,".awk",7);
		break;
	
		case 0x060b     :               //x-info
	              this->func = file_txt ;
		strncat(this->name,".info",7);
		break;
	
		case 0x060c     :               //x-lisp  
	             this->func = file_txt ;
	              strncat(this->name,".l",7);
		break;
	
		case 0x060d     :               //x-lua
	//              this->func = file_x-lua ;
	              strncat(this->name,".lua",7);
		break;
	
		case 0x060e     :               //x-msdos-batch
		this->func = file_txt ;
		strncat(this->name,".bat",7);
		break;
	
		case 0x060f     :               //x-nawk
		this->func = file_txt ;
		strncat(this->name,".awk",7);
		break;
	
		case 0x0610     :               //x-perl
		this->func = file_txt ;
		strncat(this->name,".pl",7);
		break;
	
		case 0x0611     :               //x-php
		this->func = file_txt ;
		strncat(this->name,".php",7);
		break;
	
		case 0x0612     :               //x-shellscript
		this->func = file_txt ;
		strncat(this->name,".sh",7);
		break;
	
		case 0x0613     :               //x-texinfo
		this->func = file_txt ;
		strncat(this->name,".texi",7);
		break;
	
		case 0x0614     :               //x-tex
		this->func = file_txt ;
		strncat(this->name,".tex",7);
		break;
	
		case 0x0615     :               //x-vcard
	              this->func = file_txt;
	              strncat(this->name,".vcf",7);
		break;
	
		case 0x0616     :               //x-xmcd
	//              this->func = file_x-xmcd ;
	//              strncat(this->name,".x-xmcd",7);
		break;
	
		case 0x0617     :               //plain
	              this->func = file_bin_txt ;
	              strncat(this->name,".txt",7);
		break;

		case 0x0618     :               //x-pascal (often c files)
	              this->func = file_txt ;
	              strncat(this->name,".c",7);
		break;

		case 0x0619     :               //c++
	              this->func = file_txt ;
	              strncat(this->name,".c++",7);
		break;

		case 0x061a     :               //c
	              this->func = file_txt ;
	              strncat(this->name,".c",7);
		break;

		case 0x061b     :               //x-mail
	              this->func = file_txt ;
	//              strncat(this->name,".x-mail",7);
		break;

		case 0x061c     :               //x-makefile
	              this->func = file_txt ;
	//              strncat(this->name,".x-makefile",7);
		break;
	
		case 0x061d     :               //x-asm
	              this->func = file_txt ;
	              strncat(this->name,".S",7);
		break;

		case 0x061e     :               //text (for all unknown)
	              this->func = file_txt ;
	              strncat(this->name,".txt",7);
		break;
	
	//----------------------------------------------------------------
	//Video
		case 0x0701     :               //3gpp
	              this->func = file_qt ;
		strncat(this->name,".3gp",7);
		break;
	
		case 0x0702     :               //h264
	//              this->func = file_h264 ;
		strncat(this->name,".mp4",7);
		break;
	
		case 0x0703     :               //mp2p
	//              this->func = file_mp2p ;
		strncat(this->name,".ps",7);
		break;
	
		case 0x0704     :               //mp2t
	//              this->func = file_mp2t ;
		strncat(this->name,".ts",7);
		break;
	
		case 0x0705     :               //mp4
	             this->func = file_qt ; 
		strncat(this->name,".mp4",7);
		break;
	
		case 0x0706     :               //mp4v-es
	//              this->func = file_mp4v-es ;
		strncat(this->name,".mp4",7);
		break;
	
		case 0x0707     :               //mpeg
	              this->func = file_mpeg ;
		strncat(this->name,".mpeg",7);
		break;
	
		case 0x0708     :               //mpv
	//              this->func = file_mpv ;
		strncat(this->name,".mpv",7);
		break;
	
		case 0x0709     :               //quicktime
	              this->func = file_qt ;
		strncat(this->name,".qt",7);
		break;
	
		case 0x070a     :               //x-flc
	              this->func = file_fli ;
		strncat(this->name,".flc",7);
		break;
	
		case 0x070b     :               //x-fli
	              this->func = file_fli ;
		strncat(this->name,".fli",7);
		break;
	
		case 0x070c     :               //x-flv
	//              this->func = file_x-flv ;
		strncat(this->name,".flv",7);
		break;
	
		case 0x070d     :               //x-jng
	              this->func = file_mng ;
		strncat(this->name,".jng",7);
		break;
	
		case 0x070e     :               //x-mng
	              this->func = file_mng ;
		strncat(this->name,".mng",7);
		break;
	
		case 0x070f     :               //x-msvideo
	              this->func = file_riff ;
		strncat(this->name,".avi",7);
		break;
	
		case 0x0710     :               //x-sgi-movie
	//              this->func = file_x-sgi-movie ;
		strncat(this->name,".movie",7);
		break;
	
		case 0x0711     :               //x-unknown
	//              this->func = file_x-unknown ;
	              strncat(this->name,".unknown",8);
		break;

		case 0x0712     :               //x-ms-asf
	              this->func = file_asf ;
	              strncat(this->name,".asf",7);
		break;

		case 0x0713     :               //x-matroska
	//              this->func = file_x-matroska ;
	              strncat(this->name,".mkv",7);
		break;
	
	
	//----------------------------------------------------------------
	//Reservoir found in application/octet-stream
		case 0x0801     :               //MPEG
	              this->func = file_mpeg ;
		strncat(this->name,".mpeg",7);
		break;
	
		case 0x0802     :               //Targa
	              this->func = file_tga ;
		strncat(this->name,".tga",7);
		break;
	
		case 0x0803     :               //7-zip
		this->func = file_7z ;
		strncat(this->name,".7z",7);
		break;
	
		case 0x0804     :               //cpio
		this->func = file_cpio ;
		strncat(this->name,".cpio",7);
		break;
	
		case 0x0805     :               //CD-ROM
		this->func = file_iso9660 ;
		strncat(this->name,".iso",7);
		break;
	
		case 0x0806     :               //DVD
	//              this->func = file_DVD ;
		strncat(this->name,".iso",7);
		break;
	
		case 0x0807     :               //9660
		this->func = file_iso9660 ;
		strncat(this->name,".iso",7);
		break;
	
		case 0x0808     :               //Kernel
	//              this->func = file_Kernel ;
	//              strncat(this->name,".Kernel",7);
		break;
	
		case 0x0809     :               //boot
	//              this->func = file_boot ;
		strncat(this->name,".iso",7);
		break;
	
		case 0x080a     :               //Linux
	//              this->func = file_Linux ;
	//              strncat(this->name,".Linux",7);
		break;
	
		case 0x080b     :               //filesystem
	//              this->func = file_filesystem ;
		strncat(this->name,".iso",7);
		break;
	
		case 0x080c     :               //x86
	//              this->func = file_x86 ;
		strncat(this->name,".iso",7);
		break;
	
		case 0x080d     :               //Image
	//              this->func = file_Image ;
	//              strncat(this->name,".Image",7);
		break;
	
		case 0x080e     :               //CDF
	              this->func = file_CDF ;
	              strncat(this->name,".doc",7);
		break;

		case 0x080f     :               //SQLite
	              this->func = file_SQLite ;
	              strncat(this->name,".sql",7);
		break;

		case 0x0810     :               //OpenOffice.org 
	              this->func = file_zip ;
	              strncat(this->name,".sxw",7);
		break;
	
		case 0x0811     :               //Microsoft
	              this->func = file_CDF ;
	              strncat(this->name,".doc",7);
		break;
		
		case 0x0812     :               //VMWare3
	              this->func = file_vmware ;
	              strncat(this->name,".vmdk",7);
		break;
		
		case 0x0813     :               //VMware4
	              this->func = file_vmware ;
	              strncat(this->name,".vmdk",7);
		break;

		case 0x0814     :               //JPEG
	              this->func = file_jpeg ;
	              strncat(this->name,".jp2",7);
		break;

		case 0x0815     :               //ART
	              this->func = file_art ;
	              strncat(this->name,".art",7);
		break;
		case 0x0816     :               //PCX
	              this->func = file_pcx ;
	              strncat(this->name,".pcx",7);
		break;

		case 0x0817     :               //RIFF
	              this->func = file_riff ;
	              strncat(this->name,".amv",7);
		break;

		case 0x0818     :               //DIF
	//              this->func = file_dv ;
	              strncat(this->name,".dv",7);
		break;

		case 0x0819     :               //IFF
	              this->func = file_iff ;
	              strncat(this->name,".iff",7);
		break;

		case 0x081a     :               //ATSC
	//              this->func = file_ac3 ;
	              strncat(this->name,".ac3",7);
		break;
		
		case 0x081b     :              // ScreamTracker III
	              this->func = file_mod ;
	              strncat(this->name,".s3m",7);
		break;

	//----------------------------------------------------------------
		default:
			this->func = file_default ;

	}
	if (!this->func)
		this->func = file_default ;	
}





