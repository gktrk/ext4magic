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

//#define DEBUG_MAGIC_SCAN


extern ext2_filsys     current_fs ;

// index of the files corresponding magic result strings
int ident_file(struct found_data_t *new, __u32 *scan, char *magic_buf, char *buf){

//Please do not modify the following lines.
//they are used for indices to the filestypes
	char	typestr[] ="application/ audio/ image/ message/ model/ text/ video/ ";
	char	imagestr[] ="gif jp2 jpeg png svg+xml tiff vnd.adobe.photoshop vnd.djvu x-coreldraw x-cpi x-ico x-ms-bmp x-niff x-portable-bitmap x-portable-greymap x-portable-pixmap x-psion-sketch x-quicktime x-unknown x-xcursor x-xpmi x-tga ";
	char	videostr[] ="3gpp h264 mp2p mp2t mp4 mp4v-es mpeg mpv quicktime x-flc x-fli x-flv x-jng x-mng x-msvideo x-sgi-movie x-unknown ";
	char	audiostr[] ="basic midi mp4 mpeg x-adpcm x-aiff x-dec-basic x-flac x-hx-aac-adif x-hx-aac-adts x-mod x-mp4a-latm x-pn-realaudio x-unknown x-wav ";
	char	messagestr[] ="news rfc822 ";
	char	modelstr[] ="vrml x3d ";
	char	applistr[] ="dicom mac-binhex40 msword octet-stream ogg pdf pgp pgp-encrypted pgp-keys pgp-signature postscript unknown+zip vnd.google-earth.kml+xml vnd.google-earth.kmz vnd.lotus-wordpro vnd.ms-cab-compressed vnd.ms-excel vnd.ms-tnef vnd.oasis.opendocument. vnd.rn-realmedia vnd.symbian.install x-123 x-adrift x-archive x-arc x-arj x-bittorrent x-bzip2 x-compress x-coredump x-cpio x-dbf x-dbm x-debian-package x-dosexec x-dvi x-eet x-elc x-executable x-gdbm x-gnucash x-gnumeric x-gnupg-keyring x-gzip x-hdf x-hwp x-ichitaro4 x-ichitaro5 x-ichitaro6 x-iso9660-image x-java-applet x-java-jce-keystore x-java-keystore x-java-pack200 x-kdelnk x-lha x-lharc x-lzip x-mif xml xml-sitemap x-msaccess x-ms-reader x-object x-pgp-keyring x-quark-xpress-3 x-quicktime-player x-rar x-rpm x-sc x-setupscript x-sharedlib x-shockwave-flash x-stuffit x-svr4-package x-tar x-tex-tfm x-tokyocabinet-btree x-tokyocabinet-fixed x-tokyocabinet-hash x-tokyocabinet-table x-xz x-zoo zip ";
	char	textstr[] = "html PGP rtf texmacs troff vnd.graphviz x-awk x-diff x-fortran x-gawk x-info x-lisp x-lua x-msdos-batch x-nawk x-perl x-php x-shellscript x-texinfo x-tex x-vcard x-xmcd plain x-pascal x-c++ x-c x-mail x-makefile x-asm text ";
	char	undefstr[] ="MPEG Targa 7-zip cpio CD-ROM DVD 9660 Kernel boot Linux filesystem x86 Image CDF SQLite OpenOffice.org Microsoft";
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
					ret = 3;
				else 
					break;
				if (*size < (current_fs->blocksize -64))
						ret = 2;
				else
					break;
				if (*size < (current_fs->blocksize -256))
					ret=1;
			}

			break;
		case 1 :
			return (scan & (M_IS_META | M_IS_FILE)) ? 0 :1 ;
			break;
	}
	return ret;
}



//Textfiles 
int file_txt(unsigned char *buf, int *size, __u32 scan , int flag, struct found_data_t* f_data){
	switch (flag){
		case 0 :
			if (*size < current_fs->blocksize)
				return (buf[(*size)-1] == (char)0x0a);
			else {
				if (buf[(*size)-1] == (char)0x0a)
					return 2;
			}
			break;
		case 1 :
			return (scan & M_TXT);
			break;
	}
}



//Textfiles binary
int file_bin_txt(unsigned char *buf, int *size, __u32 scan , int flag, struct found_data_t* f_data){
	switch (flag){
		case 0 :
			return (*size < current_fs->blocksize);
			break;
		case 1 :
			return (scan & M_TXT);
			break;
	}
}


//gzip
int file_gzip(unsigned char *buf, int *size, __u32 scan , int flag, struct found_data_t* f_data){
	int		ret = 0;

	switch (flag){
		case 0 :
			if(*size < (current_fs->blocksize -4)){
				ret = 2 ;
				*size += 4;
			}
			break;
		case 1 :
			return (scan & (M_IS_META | M_IS_FILE | M_TXT | M_BLANK)) ? 0 :1 ;
			break;
	}
	return ret;
}


//zip    ???????????
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
			return (scan & (M_IS_META | M_IS_FILE | M_TXT | M_BLANK)) ? 0 :1 ;
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
				ret =3;
				*size += 7;
			}
			break;


/*			if ((f_data->size) && (f_data->size <= (__u64)f_data->inode->i_size | ((__u64)f_data->inode->i_size_high<<32))){
				ssize = (f_data->size % current_fs->blocksize);
				if (*size < ssize)
					*size = ssize;
				ret = 1;
			}
			else{
			//	*size +=7;
				ret = 0;
			}
			break;*/
		case 1 :
			return ( scan & (M_IS_META | M_IS_FILE | M_TXT | M_BLANK)) ? 0 :1 ;
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
			
			

//cpio
int file_cpio(unsigned char *buf, int *size, __u32 scan , int flag, struct found_data_t* f_data){
	int 	i,j;
	int 	ret = 0;
	unsigned char	token[]="TRAILER!!!";

	switch (flag){
		case 0 :
	/*		if (*size>10) {
				if (strstr(buf+(*size)-10,token)){
					*size = ((*size) + 0x01FF) & 0xfe00 ;
					ret=1;
				}
			}
			else{*/
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
			//}	
			break;
		case 1 :	
			return (scan & (M_IS_META | M_IS_FILE)) ? 0 :1 ;
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
			return (scan & (M_IS_META)) ? 0 :1 ;
			break;
		}
	return ret;
}



//tar
int file_tar(unsigned char *buf, int *size, __u32 scan , int flag, struct found_data_t* f_data){
	int 	ret = 0;

	switch (flag){
		case 0 :
			if ((((__u64)f_data->inode->i_size |((__u64)f_data->inode->i_size_high<<32)) >= (__u64)0x2800 ) &&
				((*size) < (current_fs->blocksize - 0x4ff))){
				if (!(*size))
					*size = current_fs->blocksize;
				else
					*size = ((*size) + 1023) & ~1023 ;
	
				if (f_data->inode->i_block[12])   //FIXME for ext4
					ret = 1;
				else
					ret = 2;
			}
			break;
		case 1 :
			if (scan & M_TAR)
				ret = 1;
			 else 
				ret = (scan & (M_IS_META | (M_IS_FILE & (~ M_TXT)))) ? 0 :1 ;
			break;
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
			return (scan & (M_IS_META | M_IS_FILE | M_TXT)) ? 0 :1 ;
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
			return (scan & (M_IS_META | M_IS_FILE | M_TXT)) ? 0 :1 ;
			break;
	}
	return ret;
}


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
			return (scan & (M_IS_META | M_IS_FILE | M_TXT)) ? 0 :1 ;
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
			return (scan & (M_IS_META | M_IS_FILE | M_TXT)) ? 0 :1 ;
			break;
	}
	return ret;
}
			


//bmp
int file_bmp(unsigned char *buf, int *size, __u32 scan , int flag, struct found_data_t* f_data){
	//__u32	*p_32;
	int 	ret = 0;

	switch (flag){
		case 0 : 
			if (f_data->inode->i_size >= f_data->size){
				*size = f_data->size % current_fs->blocksize;
				ret =1;
			}
			break;
		case 1 :
			return (scan & M_IS_META) ? 0 :1 ;
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
			if ((*size > 8) && (strstr(buf + (*size) -8,"END")))	
					ret=1;
			else{
				if ((*size >= 5) && (strtok(buf,"D" )))
					ret=1;
				else
					if (*size < 5)
						ret = 2; 
			}	
			break;
		case 1 : 
			return (scan & (M_IS_META | M_IS_FILE | M_TXT)) ? 0 :1 ;
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
			return (scan & (M_IS_META | M_IS_FILE | M_TXT)) ? 0 :1 ;
			break;
	}
	return ret;
}


//tga
int file_tga(unsigned char *buf, int *size, __u32 scan , int flag, struct found_data_t* f_data){
	int 	i,j;
	int 	ret = 0;
	unsigned char	token[]="-XFILE.";

	switch (flag){
		case 0 :
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
			break;
		case 1 :	
			return (scan & (M_IS_META | M_IS_FILE | M_TXT )) ? 0 :1 ;
			break;
	}
	return ret;
}	


//mpeg
int file_mpeg(unsigned char *buf, int *size, __u32 scan , int flag, struct found_data_t* f_data){
	int 	i,j;
	int 	ret = 0;
	unsigned char	token[5];
	sprintf(token,"%c%c%c%c",0x00,0x00,0x01,0xb9);
	switch (flag){
		case 0 :
				j = 3 ; //strlen(token) -1;
				i = (*size) -1;
				while ((i >= 0) && (j >= 0) && (buf[i] == token[j])){
					i--;
					j--;
				}
				if ((i == -1) || (j == -1)){
					ret=1;
				}	
			break;
		case 1 :	
			return (scan & (M_IS_META | M_TXT )) ? 0 :1 ;
			break;
	}
	return ret;
}	


	
//riff
int file_riff(unsigned char *buf, int *size, __u32 scan , int flag, struct found_data_t* f_data){
	__u32		*p_32  ;
	__u32		ssize ;
	int 		ret = 0;

	switch (flag){
		case 0 :
			if ((f_data->size) && (f_data->size <= f_data->inode->i_size)){
				ssize = (f_data->size % current_fs->blocksize);
				*size = (ssize >= *size)? ssize : (*size) + 2;
				ret = 1;
			}
			else {
				ret =0;
			}
			break;
		case 1 :
			return (scan & (M_IS_META | M_IS_FILE | M_TXT)) ? 0 :1 ;
			break;
		case 2 :
			p_32 = (__u32*)buf;
			p_32++;
			switch (buf[3]){
				case 'F' :
					ssize =  ext2fs_le32_to_cpu(*p_32) + 8 ;
					break;
				case 'X' :
					ssize =  ext2fs_be32_to_cpu(*p_32) + 8 ;
					break;
				default :
					ssize = 0;
			}
			f_data->size = ssize;
			ret=1;
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
				if (f_data->inode->i_size > f_data->size){  //FIXME
					*size +=1;
					ret =1;
				}
			}
			else{
				if (f_data->inode->i_block[12]){   //FIXME for ext4
					ret = 2;
				}
			}				
			break;
		case 1 : 
			return (scan & (M_IS_META | M_IS_FILE | M_TXT)) ? 0 :1 ;
			break;
		case 2 :
			p_32 = (__u32*)buf;
			p_32++;
			if ((*buf == 0x49) && (*(buf+1) == 0x49))
				ssize = ext2fs_le32_to_cpu(*p_32);
			if ((*buf == 0x4d) && (*(buf+1) == 0x4d))
				ssize = ext2fs_be32_to_cpu(*p_32);
			
			f_data->size = ssize;
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
			if (*size < (current_fs->blocksize -2)){
				*size = ((*size) +3) & 0xfffc ;
				ret = 1;
			}
			break;
		case 1 :
			return (scan & (M_IS_META | M_IS_FILE | M_TXT)) ? 0 :1 ;
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
			ret = 3;
		}
			break;
		case 1:
			return (scan & (M_IS_META | M_IS_FILE | M_TXT)) ? 0 :1 ;
			break;
	}
	return ret;
}



//SQLite   FIXME ????????????????
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
					ret = 3;
				}
			}
			break;
		case 1:
			return (scan & (M_IS_META | M_IS_FILE | M_TXT)) ? 0 :1 ;
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


//ogg  
int file_ogg(unsigned char *buf, int *size, __u32 scan , int flag, struct found_data_t* f_data){
	int 		ret = 0;
	unsigned char token[7]= {0x80, 't', 'h', 'e', 'o', 'r', 'a'};

	//FIXME for ext4. must follow the bitstream by the page header (variable page size, 4-8 kB, maximum 65307)
	
	switch (flag){
		case 0 :
			if (*size < (current_fs->blocksize -7)){
				*size += 2;
				ret =1;
			}
			else{
				if (*size < (current_fs->blocksize -2)){
				*size += 2;
				ret =2;
				}
			}	
			break;
		case 1 : 
			return (scan & (M_IS_META | M_IS_FILE | M_TXT)) ? 0 :1 ;
		break;
		
		case 2 :
			if(!(memcmp(&buf[28], token, 7))){
				f_data->name[strlen(f_data->name)-1] == 'm' ;
			return 0;
			}
	}
	return ret;
}



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
		strncat(this->name,".unknow",7);
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
	//              this->func = file_postscript ;
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
	//              strncat(this->name,".vnd.ms-tnef",7);
		break;
	
		case 0x0113     :               //vnd.oasis.opendocument.
	             this->func = file_zip ;
			if (strstr(this->scan_result,"text")){
		        	strncat(this->name,".ott",7);
				break;
			}
			if (strstr(this->scan_result,"presentation")){
		        	strncat(this->name,".otp",7);
				break;
			}
			if (strstr(this->scan_result,"spreadsheet")){
		        	strncat(this->name,".ots",7);
				break;
			}
			if (strstr(this->scan_result,"graphics")){
		        	strncat(this->name,".otg",7);
				break;
			}
		break;
	
		case 0x0114     :               //vnd.rn-realmedia
	//              this->func = file_vnd.rn-realmedia ;
	//              strncat(this->name,".vnd.rn-realmedia",7);
		break;
	
		case 0x0115     :               //vnd.symbian.install
	//              this->func = file_vnd.symbian.install ;
	//              strncat(this->name,".vnd.symbian.install",7);
		break;
	
		case 0x0116     :               //x-123
	//              this->func = file_x-123 ;
	//              strncat(this->name,".x-123",7);
		break;
	
		case 0x0117     :               //x-adrift
	//              this->func = file_x-adrift ;
	//              strncat(this->name,".x-adrift",7);
		break;
	
		case 0x0118     :               //x-archive 
			this->func = file_object ;
			strncat(this->name,".a",7);
		break;
	
		case 0x0119     :               //x-arc
	//              this->func = file_x-arc ;
	//              strncat(this->name,".x-arc",7);
		break;
	
		case 0x011a     :               //x-arj
	//              this->func = file_x-arj ;      
	//              strncat(this->name,".x-arj",7);
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
	//              strncat(this->name,".z",7);
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
	//              strncat(this->name,".x-dbf",7);
		break;
	
		case 0x0121     :               //x-dbm
	//              this->func = file_x-dbm ;
	//              strncat(this->name,".x-dbm",7);
		break;
	
		case 0x0122     :               //x-debian-package
	//              this->func = file_x-debian-package ;
	//              strncat(this->name,".x-debian-package",7);
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
	//              strncat(this->name,".x-eet",7);
		break;
	
		case 0x0126     :               //x-elc
	//              this->func = file_x-elc ;
	//              strncat(this->name,".x-elc",7);
		break;
	
		case 0x0127     :               //x-executable
		this->func = file_binary ;
		strncat(this->name,".bin",7);
		break;
	
		case 0x0128     :               //x-gdbm
	//              this->func = file_x-gdbm ;
	//              strncat(this->name,".x-gdbm",7);
		break;
	
		case 0x0129     :               //x-gnucash
	//              this->func = file_x-gnucash ;
	//              strncat(this->name,".x-gnucash",7);
		break;
	
		case 0x012a     :               //x-gnumeric
	//              this->func = file_x-gnumeric ;
	//              strncat(this->name,".x-gnumeric",7);
		break;
	
		case 0x012b     :               //x-gnupg-keyring
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
	//              strncat(this->name,".x-hwp",7);
		break;
	
		case 0x012f     :               //x-ichitaro4
	//              this->func = file_x-ichitaro4 ;
	//              strncat(this->name,".x-ichitaro4",7);
		break;
	
		case 0x0130     :               //x-ichitaro5
	//              this->func = file_x-ichitaro5 ;
	//              strncat(this->name,".x-ichitaro5",7);
		break;
	
		case 0x0131     :               //x-ichitaro6
	//              this->func = file_x-ichitaro6 ;
	//              strncat(this->name,".x-ichitaro6",7);
		break;
	
		case 0x0132     :               //x-iso9660-image
		this->func = file_iso9660 ;
		strncat(this->name,".iso",7);
		break;
	
		case 0x0133     :               //x-java-applet
	//              this->func = file_x-java-applet ;
		strncat(this->name,".java",7);
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
		strncat(this->name,".lha",7);
		break;
	
		case 0x0139     :               //x-lharc
	//              this->func = file_x-lharc ;
	//              strncat(this->name,".x-lharc",7);
		break;
	
		case 0x013a     :               //x-lzip
	//              this->func = file_x-lzip ;
	//              strncat(this->name,".x-lzip",7);
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
	
		case 0x0141     :               //x-pgp-keyring
	//              this->func = file_x-pgp-keyring ;
	//              strncat(this->name,".x-pgp-keyring",7);
		break;
	
		case 0x0142     :               //x-quark-xpress-3
	//              this->func = file_x-quark-xpress-3 ; 
	//              strncat(this->name,".x-quark-xpress-3",7);
		break;
	
		case 0x0143     :               //x-quicktime-player
	//              this->func = file_x-quicktime-player ;
	//              strncat(this->name,".x-quicktime-player",7);
		break;
	
		case 0x0144     :               //x-rar
	//              this->func = file_x-rar ;
		strncat(this->name,".rar",7);
		break;
	
		case 0x0145     :               //x-rpm
	//              this->func = file_x-rpm ;
		strncat(this->name,".rpm",7);
		break;
	
		case 0x0146     :               //x-sc
	//              this->func = file_x-sc ;
	//              strncat(this->name,".x-sc",7);
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
	//              this->func = file_x-shockwave-flash ;
		strncat(this->name,".swf",7);
		break;
	
		case 0x014a     :               //x-stuffit
	//              this->func = file_x-stuffit ;
	//              strncat(this->name,".x-stuffit",7);
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
	//              strncat(this->name,".x-tex-tfm",7);
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
	//              this->func = file_x-xz ;
	//              strncat(this->name,".x-xz",7);
		break;
	
		case 0x0153     :               //x-zoo
	//              this->func = file_x-zoo ;
	//              strncat(this->name,".x-zoo",7);
		break;
	
		case 0x0154     :               //zip
		this->func = file_zip ;
		strncat(this->name,".zip",7);
		break;
	
	
	//----------------------------------------------------------------
	//Audio
		case 0x0201     :               //basic
	//              this->func = file_basic ;
		strncat(this->name,".au",7);
		break;
	
		case 0x0202     :               //midi
	//              this->func = file_midi ;
		strncat(this->name,".mid",7);
		break;
	
		case 0x0203     :               //mp4
	//              this->func = file_mp4 ;
		strncat(this->name,".mp4",7);
		break;
	
		case 0x0204     :               //mpeg
	//              this->func = file_mpeg ;
		strncat(this->name,".mp3",7);
		break;
	
		case 0x0205     :               //x-adpcm
	//              this->func = file_x-adpcm ;
	//              strncat(this->name,".x-adpcm",7);
		break;
	
		case 0x0206     :               //x-aiff
	//              this->func = file_x-aiff ;
		strncat(this->name,".aif",7);
		break;
	
		case 0x0207     :               //x-dec-basic
	//              this->func = file_x-dec-basic ;
	//              strncat(this->name,".x-dec-basic",7);
		break;
	
		case 0x0208     :               //x-flac
	//              this->func = file_x-flac ;
	//              strncat(this->name,".x-flac",7);
		break;
	
		case 0x0209     :               //x-hx-aac-adif
	//              this->func = file_x-hx-aac-adif ;
		strncat(this->name,".aac",7);
		break;
	
		case 0x020a     :               //x-hx-aac-adts
	//              this->func = file_x-hx-aac-adts ;
		strncat(this->name,".aac",7);
		break;
	
		case 0x020b     :               //x-mod
	             this->func = file_mod ;
	              strncat(this->name,".mod",7);
		break;
	
		case 0x020c     :               //x-mp4a-latm
	//              this->func = file_x-mp4a-latm ;
	//              strncat(this->name,".x-mp4a-latm",7);
		break;
	
		case 0x020d     :               //x-pn-realaudio
	//              this->func = file_x-pn-realaudio ;
		strncat(this->name,".ra",7);
		break;
	
		case 0x020e     :               //x-unknown
	//              this->func = file_x-unknown ;
	//              strncat(this->name,".x-unknown",7);
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
	//              this->func = file_jp2 ;
		strncat(this->name,".jp2",7);
		break;
	
		case 0x0303     :               //jpeg
	              this->func = file_jpeg ;
		strncat(this->name,".jpeg",7);
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
		strncat(this->name,".tiff",7);
		break;
	
		case 0x0307     :               //vnd.adobe.photoshop
	//              this->func = file_vnd.adobe.photoshop ;
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
	//              strncat(this->name,".x-cpi",7);
		break;
	
		case 0x030b     :               //x-ico
	//              this->func = file_x-ico ;
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
	//              this->func = file_x-portable-bitmap ;
		strncat(this->name,".pbm",7);
		break;
	
		case 0x030f     :               //x-portable-greymap
	//              this->func = file_x-portable-greymap ;
		strncat(this->name,".pgm",7);
		break;
	
		case 0x0310     :               //x-portable-pixmap
	//              this->func = file_x-portable-pixmap ;
		strncat(this->name,".ppm",7);
		break;
	
		case 0x0311     :               //x-psion-sketch
	//              this->func = file_x-psion-sketch ;
	//              strncat(this->name,".x-psion-sketch",7);
		break;
	
		case 0x0312     :               //x-quicktime
	//              this->func = file_x-quicktime ;
	              strncat(this->name,".qti",7);
		break;
	
		case 0x0313     :               //x-unknown
	//              this->func = file_x-unknown ;
	//              strncat(this->name,".x-unknown",7);
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
	//              strncat(this->name,".PGP",7);
		break;
	
		case 0x0603     :               //rtf
		this->func = file_txt ;
		strncat(this->name,".rtf",7);
		break;
	
		case 0x0604     :               //texmacs
	//              this->func = file_texmacs ;
	//              strncat(this->name,".texmacs",7);
		break;
	
		case 0x0605     :               //troff
		this->func = file_txt ;
		strncat(this->name,".man",7);
		break;
	
		case 0x0606     :               //vnd.graphviz
	//              this->func = file_vnd.graphviz ;
	//              strncat(this->name,".vnd.graphviz",7);
		break;
	
		case 0x0607     :               //x-awk
		this->func = file_txt ;
		strncat(this->name,".awk",7);
		break;
	
		case 0x0608     :               //x-diff
		this->func = file_txt ;
		strncat(this->name,".diff",7);
		break;
	
		case 0x0609     :               //x-fortran  (often c files)
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
	
		case 0x060c     :               //x-lisp   (often c files)
	             this->func = file_txt ;
	              strncat(this->name,".l",7);
		break;
	
		case 0x060d     :               //x-lua
	//              this->func = file_x-lua ;
	//              strncat(this->name,".x-lua",7);
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
	//              strncat(this->name,".txt",7);
		break;
	
	//----------------------------------------------------------------
	//Video
		case 0x0701     :               //3gpp
	//              this->func = file_3gpp ;
		strncat(this->name,".gpp",7);
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
	//              this->func = file_mp4 ;
		strncat(this->name,".mp4",7);
		break;
	
		case 0x0706     :               //mp4v-es
	//              this->func = file_mp4v-es ;
		strncat(this->name,".mp4",7);
		break;
	
		case 0x0707     :               //mpeg
	//              this->func = file_mpeg ;
		strncat(this->name,".mpeg",7);
		break;
	
		case 0x0708     :               //mpv
	//              this->func = file_mpv ;
		strncat(this->name,".mpv",7);
		break;
	
		case 0x0709     :               //quicktime
	//              this->func = file_quicktime ;
		strncat(this->name,".qt",7);
		break;
	
		case 0x070a     :               //x-flc
	//              this->func = file_x-flc ;
		strncat(this->name,".flc",7);
		break;
	
		case 0x070b     :               //x-fli
	//              this->func = file_x-fli ;
		strncat(this->name,".fli",7);
		break;
	
		case 0x070c     :               //x-flv
	//              this->func = file_x-flv ;
		strncat(this->name,".flv",7);
		break;
	
		case 0x070d     :               //x-jng
	//              this->func = file_x-jng ;
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
	//              strncat(this->name,".x-unknown",7);
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
	//              strncat(this->name,".doc",7);
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
	//              strncat(this->name,".doc",7);
		break;

	//----------------------------------------------------------------
		default:
			this->func = file_default ;

	}
	if (!this->func)
		this->func = file_default ;	
}





