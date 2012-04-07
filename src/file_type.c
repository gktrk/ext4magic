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
//#define DEBUG_MAGIC_MP3_STREAM

extern ext2_filsys     current_fs ;

// index of the files corresponding magic result strings
int ident_file(struct found_data_t *new, __u32 *scan, char *magic_buf, void *buf){

//Please do not modify the following lines.
//they are used for indices to the filestypes
	char	typestr[] ="application/ audio/ image/ message/ model/ text/ video/ ";
	char	imagestr[] ="gif jp2 jpeg png svg+xml tiff vnd.adobe.photoshop vnd.djvu x-coreldraw x-cpi x-ico x-ms-bmp x-niff x-portable-bitmap x-portable-greymap x-portable-pixmap x-psion-sketch x-quicktime x-unknown x-xcursor x-xpmi x-tga x-xcf ";
	char	videostr[] ="3gpp h264 mp2p mp2t mp4 mp4v-es mpeg mpv quicktime x-flc x-fli x-flv x-jng x-mng x-msvideo x-sgi-movie x-unknown x-ms-asf x-matroska webm ";
	char	audiostr[] ="basic midi mp4 mpeg x-adpcm x-aiff x-dec-basic x-flac x-hx-aac-adif x-hx-aac-adts x-mod x-mp4a-latm x-pn-realaudio x-unknown x-wav ";
	char	messagestr[] ="news rfc822 ";
	char	modelstr[] ="vrml x3d ";
	char	applistr[] ="dicom mac-binhex40 msword octet-stream ogg pdf pgp pgp-encrypted pgp-keys pgp-signature postscript unknown+zip vnd.google-earth.kml+xml vnd.google-earth.kmz vnd.lotus-wordpro vnd.ms-cab-compressed vnd.ms-excel vnd.ms-tnef vnd.oasis.opendocument. vnd.rn-realmedia vnd.symbian.install x-123 x-adrift x-archive x-arc x-arj x-bittorrent x-bzip2 x-compress x-coredump x-cpio x-dbf x-dbm x-debian-package x-dosexec x-dvi x-eet x-elc x-executable x-gdbm x-gnucash x-gnumeric x-gnupg-keyring x-gzip x-hdf x-hwp x-ichitaro4 x-ichitaro5 x-ichitaro6 x-iso9660-image x-java-applet x-java-jce-keystore x-java-keystore x-java-pack200 x-kdelnk x-lha x-lharc x-lzip x-mif xml xml-sitemap x-msaccess x-ms-reader x-object x-pgp-keyring x-quark-xpress-3 x-quicktime-player x-rar x-rpm x-sc x-setupscript x-sharedlib x-shockwave-flash x-stuffit x-svr4-package x-tar x-tex-tfm x-tokyocabinet-btree x-tokyocabinet-fixed x-tokyocabinet-hash x-tokyocabinet-table x-xz x-zoo zip x-font-ttf x-7z-compressed ";
	char	textstr[] = "html PGP rtf texmacs troff vnd.graphviz x-awk x-diff x-fortran x-gawk x-info x-lisp x-lua x-msdos-batch x-nawk x-perl x-php x-shellscript x-texinfo x-tex x-vcard x-xmcd plain x-pascal x-c++ x-c x-mail x-makefile x-asm x-python x-java PEM SGML libtool M3U tcl POD PPD configure ruby sed expect ssh text ";
//Files not found as mime-type
	char	undefstr[] ="MPEG Targa 7-zip cpio CD-ROM DVD 9660 Kernel boot ext2 ext3 ext4 Image Composite SQLite OpenOffice.org Microsoft VMWare3 VMware4 JPEG ART PCX RIFF DIF IFF ATSC ScreamTracker EBML LZMA Audio=Visual Sample=Vision ISO=Media Linux filesystem x86 LUKS python ESRI=Shape CDF ecryptfs ";
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
			token[len] = ((*p_search)==0x3d)? 0x20 : (*p_search);
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




static int analysis_ret1(char* typename, struct found_data_t* f_data, int ret , int b_count, __u32 offset, __u32 last_match){
	if (last_match)
		f_data->last_match_blk = (last_match/current_fs->blocksize) +1 ;
	if (ret){
		f_data->scantype = DATA_CARVING;
		f_data->size = offset;
		if (offset > (b_count * current_fs->blocksize))
			f_data->buf_length = b_count;
		else
			f_data->buf_length = offset / current_fs->blocksize;
		f_data->next_offset = offset - (f_data->buf_length * current_fs->blocksize);
	}
	switch (ret){
		case 0 :
//			fprintf(stderr,"%s-CHECK: Block %lu : Break \n",typename,f_data->first);
			f_data->scantype |= DATA_BREAK;
			f_data->first = 0;
			return 0;
		break;
		case 1:

		break;
		case 2 :
			f_data->scantype |= DATA_READY;
			if(offset % current_fs->blocksize)(f_data->buf_length)++;
		break;
		case 3 :
			f_data->scantype = 0;
//			printf("%s  %lu can not recover over data carving\n",typename,f_data->first);
		break;
	}
	return ret;
}



static int analysis_ret2(char* typename, struct found_data_t* f_data, int ret, __u32 offset, __u32 last_match){
	if (last_match)
		f_data->last_match_blk = ((f_data->size - f_data->next_offset + last_match)/current_fs->blocksize) +1 ;
	if (ret){
		f_data->size += offset - f_data->next_offset;
		f_data->buf_length = offset / current_fs->blocksize;
		f_data->next_offset = offset - (f_data->buf_length * current_fs->blocksize);
	}
	switch (ret){
		case 0 :
//			fprintf(stderr,"%s-CHECK: data error block %lu , recover not\n",typename, f_data->last+(offset / current_fs->blocksize));
			f_data->scantype |= DATA_BREAK;	
		break;
		case 1 :
//			fprintf(stderr,"%s-CHECK: data next found\n", typename);
		break;
		case 2 :
//			fprintf(stderr,"%s-CHECK: data end found, recover\n", typename);
			f_data->scantype |= DATA_READY;
			if(offset % current_fs->blocksize) (f_data->buf_length)++;
		break;
		case 3 :
//			printf("%s-CHECK: %lu : break data following -> block scan now\n",typename,f_data->first);
		break;
	}
	return ret;
}




//FLAC__crc16_table
/* CRC-16, poly = x^16 + x^15 + x^2 + x^0, init = 0 */
 __u16 FLAC__crc16_table[256] = {
        0x0000,  0x8005,  0x800f,  0x000a,  0x801b,  0x001e,  0x0014,  0x8011,
        0x8033,  0x0036,  0x003c,  0x8039,  0x0028,  0x802d,  0x8027,  0x0022,
        0x8063,  0x0066,  0x006c,  0x8069,  0x0078,  0x807d,  0x8077,  0x0072,
        0x0050,  0x8055,  0x805f,  0x005a,  0x804b,  0x004e,  0x0044,  0x8041,
        0x80c3,  0x00c6,  0x00cc,  0x80c9,  0x00d8,  0x80dd,  0x80d7,  0x00d2,
        0x00f0,  0x80f5,  0x80ff,  0x00fa,  0x80eb,  0x00ee,  0x00e4,  0x80e1,
        0x00a0,  0x80a5,  0x80af,  0x00aa,  0x80bb,  0x00be,  0x00b4,  0x80b1,
        0x8093,  0x0096,  0x009c,  0x8099,  0x0088,  0x808d,  0x8087,  0x0082,
        0x8183,  0x0186,  0x018c,  0x8189,  0x0198,  0x819d,  0x8197,  0x0192,
        0x01b0,  0x81b5,  0x81bf,  0x01ba,  0x81ab,  0x01ae,  0x01a4,  0x81a1,
        0x01e0,  0x81e5,  0x81ef,  0x01ea,  0x81fb,  0x01fe,  0x01f4,  0x81f1,
        0x81d3,  0x01d6,  0x01dc,  0x81d9,  0x01c8,  0x81cd,  0x81c7,  0x01c2,
        0x0140,  0x8145,  0x814f,  0x014a,  0x815b,  0x015e,  0x0154,  0x8151,
        0x8173,  0x0176,  0x017c,  0x8179,  0x0168,  0x816d,  0x8167,  0x0162,
        0x8123,  0x0126,  0x012c,  0x8129,  0x0138,  0x813d,  0x8137,  0x0132,
        0x0110,  0x8115,  0x811f,  0x011a,  0x810b,  0x010e,  0x0104,  0x8101,
        0x8303,  0x0306,  0x030c,  0x8309,  0x0318,  0x831d,  0x8317,  0x0312,
        0x0330,  0x8335,  0x833f,  0x033a,  0x832b,  0x032e,  0x0324,  0x8321,
        0x0360,  0x8365,  0x836f,  0x036a,  0x837b,  0x037e,  0x0374,  0x8371,
        0x8353,  0x0356,  0x035c,  0x8359,  0x0348,  0x834d,  0x8347,  0x0342,
        0x03c0,  0x83c5,  0x83cf,  0x03ca,  0x83db,  0x03de,  0x03d4,  0x83d1,
        0x83f3,  0x03f6,  0x03fc,  0x83f9,  0x03e8,  0x83ed,  0x83e7,  0x03e2,
        0x83a3,  0x03a6,  0x03ac,  0x83a9,  0x03b8,  0x83bd,  0x83b7,  0x03b2,
        0x0390,  0x8395,  0x839f,  0x039a,  0x838b,  0x038e,  0x0384,  0x8381,
        0x0280,  0x8285,  0x828f,  0x028a,  0x829b,  0x029e,  0x0294,  0x8291,
        0x82b3,  0x02b6,  0x02bc,  0x82b9,  0x02a8,  0x82ad,  0x82a7,  0x02a2,
        0x82e3,  0x02e6,  0x02ec,  0x82e9,  0x02f8,  0x82fd,  0x82f7,  0x02f2,
        0x02d0,  0x82d5,  0x82df,  0x02da,  0x82cb,  0x02ce,  0x02c4,  0x82c1,
        0x8243,  0x0246,  0x024c,  0x8249,  0x0258,  0x825d,  0x8257,  0x0252,
        0x0270,  0x8275,  0x827f,  0x027a,  0x826b,  0x026e,  0x0264,  0x8261,
        0x0220,  0x8225,  0x822f,  0x022a,  0x823b,  0x023e,  0x0234,  0x8231,
        0x8213,  0x0216,  0x021c,  0x8219,  0x0208,  0x820d,  0x8207,  0x0202};

__u16 FLAC__crc16(const unsigned char *data, unsigned len, __u16 crc_last){
        __u32 crc = (crc_last) ? crc_last : 0 ;

        while(len--)
                crc = ((crc<<8) ^ FLAC__crc16_table[(crc>>8) ^ *data++]) & 0xffff;
        return crc;
}



static __u32 test_id3_tag(unsigned char *buf){
	__u32 	offset = 0;

	if(buf[0]=='I' && buf[1]=='D' && buf[2]=='3' && (buf[3]==2 || buf[3]==3 || buf[3]==4) && buf[4]==0){
		if(buf[3]==4 && (buf[5]&0x10)==0x10) 
			offset = 10 ;
		offset += ((buf[6]&0x7f)<<21) + ((buf[7]&0x7f)<<14) + ((buf[8]&0x7f)<<7) + (buf[9]&0x7f)+ 10;
//		fprintf (stderr,"ID3-TAG found : size  %lu \n", offset);	
	}
	return offset;
} 



static int read_ebml(__u64* p_con , void* ebml){
	int 		i,j;
	unsigned char 	c, *p;

	p = (unsigned char*) ebml;
	c = 0x80;
	for (i=8;i>0;i--){
		if ( *p & c)
			break;
		c = c >>1 ;
	} 
	if (!i) 
		return 0;
	*p_con = (__u64) (*p & ~c);
	p++;
	for (j=8 ; j > i; j--){
		*p_con = (*p_con <<8);
		*p_con += (__u64)*p;
		p++;
	}
	return 9 - i;
}



/*
// skeleton function (Version:  0.3.0)
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
		// 4 == As at "return 2" ; but not if the length of the file currently has 12 blocks. //FIXME


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
	//				    f_data->func = NULL or f_data->first = 0 prevent the recover of the file

	return 
		// the return value is currently not evaluated .

   case 3:  // Called if set "f_data->scantype = DATA_CARVING" to check all the blocks of a file.
	    // All properties from "case 3" are also available below "case 2" but there only maximum of the first 12 blocks
	// unsigned char *buf   	==  pointer to data blocks for a possible append to file  (max 16 blocks)
	// int *size            	==  undefined
	// __u32 scan			==  undefined
	// struct found_data_t* f_data  ==  pointer to the new data structure for the new file.
	
	// There a any flags and variable to control this function, these have to be evaluated and/or re-set in this function.
	// 			    f_data->scantype  (control method flag ; break or ready see also util.h)
	//			    f_data->buf_length (how many blocks, input blocks availabe (max 16); output how many blocks add to file
							corresponding with f_data->next_offset)
	//			    f_data->last_match_blk (output the last matching logical block)
	//			    f_data->next_offset (input and output the offset relative to the first block in buf)
	//			    f_data->size ( output the current total size of the file to f_data->next_offset)
	//			    the pointer f_data->priv can hold private data, (allocated)
	
	return  
		// 0 == break;
		// 1 == countinue
		// 2 == ready
		// control f_data->scantype
		// must set f_data->next_offset; f_data->buf_length ; optional f_data->last_match_blk and f_data->size
	// 	
    break;


    case 4 :
		// If private memory has been alloc to f_data->priv must free here. 
		// In this case not use f_data->func = NULL at "case 2", that would lead to a memory error here.
		// return value is currently not evaluated .
    break;
  }
}
*/




//default
int file_default(unsigned char *buf, int *size, __u32 scan , int flag, struct found_data_t* f_data){
	int ret = 0;
	switch (flag){
		case 0 :
			if(f_data->scantype & DATA_METABLOCK){
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
					return (f_data->scantype & DATA_METABLOCK) ? 2 : 4 ;
			}
			break;
		case 1 :
			return (scan & M_TXT);
			break;
	}
	return 0;
}


//html 
int file_html(unsigned char *buf, int *size, __u32 scan , int flag, struct found_data_t* f_data){
	switch (flag){
		case 0 :
			if (*size < current_fs->blocksize){
				if ((buf[(*size)-1] == (unsigned char)0x0a) ||  (buf[(*size)-1] == (unsigned char)0x3e))
					return 1;
			}
			else {
				if (buf[(*size)-1] == (unsigned char)0x0a)
					return (f_data->scantype & DATA_METABLOCK) ? 2 : 4 ;
			}
			break;
		case 1 :
			return (scan & M_TXT);
			break;
	}
	return 0;
}


//troff  just check > forward to file_txt
int chk_troff(unsigned char *buf, int *size, __u32 scan , int flag, struct found_data_t* f_data){
	char c[8][2] = {{0x2e,0x22},{0x0a,0x2e},{0x0a,0x5c},{0x2e,0x53},{0x2e,0x54},{0x2e,0x50},{0x5c,0x2d},{0x2e,0x42}};
	int ret=0;
	int i,j;
	
	if (flag ==2){
		i=0;
		while ((!ret)&&( i++ < 24)){
			if (buf[i] == 0x20)
				ret++;
		}
		i=0;
		while ((ret == 1)&&( i++ < 80)){
			if (buf[i] == 0x0a)
				ret++;
			if (!buf[i]) 
				ret = 0;
		}
		i=0;
		while ((ret == 2)&&( i++ < 256)){
			for (j=0; j<8;j++){
				if ((buf[i] == c[j][0]) && (buf[i+1] == c[j][1])){
					ret++;
					break;
				}
			}
		}
		if (ret == 3)
			f_data->func =file_txt;
		else
			f_data->func = NULL;
	}
return 1;
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
					return (f_data->scantype & DATA_METABLOCK) ? 2 : 4 ;
			}
			break;
		case 1 :
			return (scan & M_TXT);
			break;
	}
	return 0;
}


struct zip_priv_t{
	int	 	flag;
	__u32		count;};

static int follow_zip(unsigned char *buf, __u16 blockcount, __u32 *offset, __u32 *last_match,  struct zip_priv_t * priv){
#define ZIP_CENTRAL_DIR         0x02014B50
#define ZIP_FILE_ENTRY          0x04034B50
#define ZIP_SIGNATURE           0x05054B50
#define ZIP_END_CENTRAL_DIR     0x06054B50
#define ZIP_CENTRAL_DIR64       0x06064B50
#define ZIP_END_CENTRAL_DIR64   0x07064B50
#define ZIP_EXTRA_DATA		0x08064B50
#define ZIP_DATA_DESCRIPTOR     0x08074B50

	__u32		i=*offset;
	int 		ret = 1;
	__u32		header; 
	__u32		end = (blockcount * current_fs->blocksize)-30;
	__u16		*p_16;
	__u32		*p_32;
	
	if (i == end)
		end++;
	while ((ret == 1 )&& ( i < end )){
		if (priv->flag){
			while ((i < end) && (!((buf[i] == 0x50) && (buf[i+1] ==0x4B) && (buf[i+2] ==0x07)&& (buf[i+3] ==0x08)))){
				(priv->count)++;
				i++;
				if (priv->count > 4194304){ // only 4MB then we give up
					ret = 3;
					break;
				}
			}
			if ((i == end)||(ret == 3))
				break;
			p_32 = 	(__u32*)(buf+i+8);
			if (ext2fs_le32_to_cpu(*p_32) == priv->count){
				priv->flag = 0;
				*last_match = i+1;
			}
			else{
				ret = 0;
				break;
			}	
		}
		else{
			header = ext2fs_le32_to_cpu(*(__u32*)(buf+i));
			switch (header){
			case ZIP_CENTRAL_DIR		:
				*last_match = i +1;
				p_16 = (__u16*)(buf+i+28);
				i += ext2fs_le16_to_cpu(*p_16);
				p_16++;
				i += ext2fs_le16_to_cpu(*p_16) + 46;
				p_16++;
				i += ext2fs_le16_to_cpu(*p_16);	
			break;
			case ZIP_FILE_ENTRY		:
				*last_match = i+1;
				p_32 = (__u32*)(buf+i+18);
				if (! ext2fs_le32_to_cpu(*p_32)){
					if ( (buf[i+6] & 0x08) ){
						priv->flag = 1;
						priv->count = 0;
					}
				}
				p_16 = (__u16*)(buf+i+26);
				i += ext2fs_le32_to_cpu(*p_32) + ext2fs_le16_to_cpu(*p_16);
				p_16++;
				i += ext2fs_le16_to_cpu(*p_16) + 30;	
			break;
			case ZIP_SIGNATURE		:
				*last_match = i +1;
				p_16 = (__u16*)(buf+i+4);
				i += ext2fs_le16_to_cpu(*p_16) + 6;
			break;
			case ZIP_END_CENTRAL_DIR	:
				*last_match = i +1;
				p_16 = (__u16*)(buf+i+20);
				i += ext2fs_le16_to_cpu(*p_16) + 22;
				ret = 2;
			break;
			
			//I think the follow is never needed	
			case ZIP_EXTRA_DATA		:
			//	p_32 = (__u32*)(buf+i+4);
			//	i += ext2fs_le32_to_cpu(*p_16) + 8;
			//break;	
			case ZIP_CENTRAL_DIR64		:
			//	i + 12 ;
			//	ret = 2 ;
			//break;
			case ZIP_END_CENTRAL_DIR64	:
			case ZIP_DATA_DESCRIPTOR	:
				*last_match = i +1;
				i+=16;
				if (buf[i] != 0x50)	
					ret = 3;
			break;
			default				:
				ret = 0 ; 
			break;
			}	
		}
	}
	*offset = i;
return ret;
}



//-------------------------------
#include <zlib.h>
#include <bzlib.h>
#define OUTLEN 		8000

struct priv_zlib_t {
	__u32 		flag;
	__u32		crc;
	z_stream	z_strm;
	unsigned char	out_buf[OUTLEN];
};

struct priv_bzlib_t {
	__u32 		flag;
	bz_stream	b_strm;
	unsigned char	out_buf[OUTLEN];
};

static int init_priv_zlib(struct found_data_t* f_data){
	struct priv_zlib_t *private = NULL;

	private = malloc(sizeof(struct priv_zlib_t));
	if (!private){
		fprintf(stderr,"ERROR: can't allocate memory\n");
		return 1;
	}
	private->flag = 0;
	private->crc = 0;
	memset(&(private->z_strm),0,sizeof(z_stream));
	f_data->priv = private;
return 0;
}


static int init_priv_bzlib(struct found_data_t* f_data){
	struct priv_bzlib_t *private = NULL;

	private = malloc(sizeof(struct priv_bzlib_t));
	if (!private){
		fprintf(stderr,"ERROR: can't allocate memory\n");
		return 1;
	}
	private->flag = 0;
	memset(&(private->b_strm),0,sizeof(bz_stream));
	f_data->priv = private;
return 0;
}



static int follow_zlib(unsigned char *buf, __u16 blockcount, __u32 *offset, __u32 *last_match,  struct priv_zlib_t *priv){
	int  	ret = 1;		//priv->flag: Bit0=initialize2 ; Bit1=initialize ; Bit2=check maxout; Bit3=crc32 ; Bit15=Init 
	int  	z_ret = 0;		//0x0b = initialize for gzip , or use 0x06 for z  
	int  	end = blockcount * current_fs->blocksize;
	__u32	p_offset ;
	__u32	total ;
	unsigned char *out = &(priv->out_buf[0]);

//----------------- Initialize
	if ((priv->flag) & 0x2){ 
		if ((priv->flag) & 0x8)
			priv->crc = 0;
		if ((priv->flag) & 0x4)
			priv->crc = *offset;   //compr. size 

		priv->z_strm.zalloc = Z_NULL;
		priv->z_strm.zfree = Z_NULL;
		priv->z_strm.opaque = Z_NULL;
		priv->z_strm.avail_in = 0;
		priv->z_strm.next_in = Z_NULL;
		if ((priv->flag) & 0x1)
			z_ret = inflateInit2(&(priv->z_strm), -MAX_WBITS);
		else 
			z_ret = inflateInit(&(priv->z_strm));
		if ((z_ret != Z_OK) || (!out)){
			printf("Error: init zlib\n");
		}
		priv->flag &= ~0x3;
		priv->flag |= 0x80;  //flag = if necessary use inflateEnd
		return z_ret;
	}
//----------------------  Decompress
	p_offset = *offset;
	total = priv->z_strm.total_in;
	priv->z_strm.next_in = buf + p_offset ;
	priv->z_strm.avail_in = end - p_offset;
	priv->z_strm.next_out = out;
	priv->z_strm.avail_out = OUTLEN;

	while ((ret == 1) && (!z_ret) && (priv->z_strm.avail_in)){
		z_ret = inflate(&(priv->z_strm), Z_NO_FLUSH);
		p_offset = priv->z_strm.total_in - total + *offset ;
		if (!(priv->z_strm.avail_out)){
			if ((priv->flag) & 0x8)
				priv->crc = crc32(priv->crc, out, OUTLEN - priv->z_strm.avail_out);
			priv->z_strm.next_out = out;
			priv->z_strm.avail_out = OUTLEN;
			*last_match = p_offset ;
		}
		if (((priv->flag) & 0x4) && (priv->crc < priv->z_strm.total_out)) 
			ret = 0;
	}
	if (((priv->flag) & 0x8)&&(OUTLEN - priv->z_strm.avail_out))
		priv->crc = crc32(priv->crc, out, OUTLEN - priv->z_strm.avail_out);
	
	if (z_ret == Z_STREAM_END){
		if ((priv->flag) & 0x8){
			if ((priv->z_strm.avail_in > 3) && ((ext2fs_le32_to_cpu (*(__u32*)(buf + p_offset))) == priv->crc )){
				p_offset += 8;
				ret = 2;
			}
			else 
				ret = 0;
		}
		else
			ret =2;
		
		inflateEnd(&(priv->z_strm));
		priv->flag = 0;	
	}
	else
		if ((!ret) || (z_ret && (z_ret != Z_STREAM_END))){
			inflateEnd(&(priv->z_strm));	
			priv->flag = 0;
			ret = 0;
		}
	*offset = p_offset;
	return ret;
}

	
static int follow_bz2lib(unsigned char *buf, __u16 blockcount, __u32 *offset, __u32 *last_match, struct priv_bzlib_t *priv){
	int  	ret = 1;		//flag: Bit1=initialize
	int  	z_ret = 0;
	int  	end = blockcount * current_fs->blocksize;
	__u32	p_offset ;
	__u32	total ;
	unsigned char *out = &(priv->out_buf[0]);

	if ((priv->flag) & 0x2){ //DecompressInit
		priv->b_strm.bzalloc = Z_NULL;
		priv->b_strm.bzfree = Z_NULL;
		priv->b_strm.opaque = Z_NULL;
		priv->b_strm.avail_in = 0;
		priv->b_strm.avail_out = 0;
		priv->b_strm.next_in = Z_NULL;

		z_ret = BZ2_bzDecompressInit ( &(priv->b_strm), 0, 1 );
		if (z_ret != BZ_OK){
			BZ2_bzDecompressEnd( &(priv->b_strm) );
		}
		priv->flag = 0x80;
		return z_ret;
	}
//----------------------  Decompress
	p_offset = *offset;
	total = priv->b_strm.total_in_lo32;
	priv->b_strm.next_in = (char*)buf + p_offset ;
	priv->b_strm.avail_in = end - p_offset;
	priv->b_strm.next_out = (char*)out;
	priv->b_strm.avail_out = OUTLEN;

	while ((z_ret >=0) && (z_ret != BZ_STREAM_END) && (priv->b_strm.avail_in)){
		z_ret = BZ2_bzDecompress ( &(priv->b_strm) );
		p_offset = priv->b_strm.total_in_lo32 - total + *offset ;
		if ((z_ret == BZ_OUTBUFF_FULL) || (!priv->b_strm.avail_out)){
			priv->b_strm.next_out = (char*)out;
			priv->b_strm.avail_out = OUTLEN;
			*last_match = p_offset ;
			z_ret = 0;
		}
	}
	
	if (z_ret){
		ret = (z_ret == BZ_STREAM_END) ? 2 : 0 ;
		BZ2_bzDecompressEnd( &(priv->b_strm) );
		priv->flag = 0;		
	}
	*offset = p_offset;
	return ret;
}


//gzip
int file_gzip(unsigned char *buf, int *size, __u32 scan , int flag, struct found_data_t* f_data){
#define GZ_FTEXT        1
#define GZ_FHCRC        2
#define GZ_FEXTRA       4
#define GZ_FNAME        8
#define GZ_FCOMMENT     0x10

	int			z_flags, b_count, ret = 0;
	__u32			offset;
	__u32			last_match = 0;


	switch (flag){
		case 0 :
			if(f_data->scantype & DATA_READY){
				*size = (f_data->next_offset)?f_data->next_offset : *size;
				ret = 1;
			}
			else{
				if(*size < (current_fs->blocksize -8)) 
					ret = 1 ;
				else {
					if(*size < (current_fs->blocksize -4))
						ret = 2 ;
				}
				*size += 4;
			}
			break;
		case 1 :
			return ((scan & (M_IS_META | M_CLASS_1 | M_BLANK | M_TXT))||(f_data->scantype & DATA_READY)) ? 0 :1 ;
			break;
		case 2 :
			if(init_priv_zlib(f_data))
					return 0;
			((struct priv_zlib_t*)(f_data->priv))->flag = 0xb;
			
			if ((buf[0]==0x1F)&&(buf[1]==0x8B)&&(buf[2]==0x08)&&((buf[3]&0xe0)==0)&&
				(!follow_zlib(NULL,0,NULL,NULL,(struct priv_zlib_t *)f_data->priv))){
				offset = 10;
				z_flags=buf[3];
				if((z_flags&GZ_FEXTRA)!=0){
					offset += 2;
					offset += (buf[10]|(buf[11]<<8));
				}
				if((z_flags&GZ_FNAME)!=0){
					while( (offset < 256) && (buf[offset++]!='\0')){
					}
				}
				if((z_flags&GZ_FCOMMENT)!=0){
					while((offset<1024) && (buf[offset++]!='\0')){
					}
				}
				if((z_flags&GZ_FHCRC)!=0){
					offset+=2;
				}
				
				b_count = (f_data->buf_length > 12) ? 12 : f_data->buf_length ;
				ret = follow_zlib(buf, b_count, &offset, &last_match,(struct priv_zlib_t *)f_data->priv);
				ret = analysis_ret1("GZIP", f_data, ret , b_count, offset, last_match);
			}
			else
				f_data->first = 0;
		break;	
		case 3 :
			offset = f_data->next_offset ;
			ret = follow_zlib(buf, f_data->buf_length, &offset, &last_match,(struct priv_zlib_t *)f_data->priv);
			ret = analysis_ret2("GZIP", f_data, ret, offset, last_match);
		break;
		case 4:
			if (f_data->priv){
				if (((struct priv_zlib_t*)(f_data->priv))->flag)
					inflateEnd( &((struct priv_zlib_t*)(f_data->priv))->z_strm);
				free(f_data->priv);
			}
		break;
	}
	return ret;
}


//bzip2
int file_bzip2(unsigned char *buf, int *size, __u32 scan , int flag, struct found_data_t* f_data){
	int		 b_count, ret = 0;
	__u32		offset;
	__u32		last_match = 0;
	

	switch (flag){
		case 0 :
			if(f_data->scantype & DATA_READY){
				*size = (f_data->next_offset)?f_data->next_offset : *size;
				ret = 1;
			}
			else{
				if(*size < (current_fs->blocksize -8)) 
					ret = 1 ;
				else {
					if(*size < (current_fs->blocksize -4))
						ret = 2 ;
				}
				*size += 4;
			}
			break;
		case 1 :
			return ((scan & (M_IS_META | M_CLASS_1 | M_BLANK | M_TXT))||(f_data->scantype & DATA_READY)) ? 0 :1 ;
			break;
		case 2 :
				offset = 0;
				if(init_priv_bzlib(f_data))
					return 0;
				((struct priv_bzlib_t*)(f_data->priv))->flag = 0x2;
				b_count = (f_data->buf_length > 12) ? 12 : f_data->buf_length ;
				ret = follow_bz2lib(NULL, 0, NULL, NULL, (struct priv_bzlib_t *)f_data->priv);
				if (! ret)
                                        ret = follow_bz2lib(buf, b_count, &offset, &last_match,(struct priv_bzlib_t *)f_data->priv);
				else 
					return 0;
				ret = analysis_ret1("BZIP2", f_data, ret , b_count, offset, last_match);
		break;	
		case 3 :
			offset = f_data->next_offset ;
			ret = follow_bz2lib(buf, f_data->buf_length, &offset, &last_match, (struct priv_bzlib_t *)f_data->priv);
			ret = analysis_ret2("BZIP2", f_data, ret, offset, last_match);
		break;
		case 4:
			if (f_data->priv){
				if (((struct priv_bzlib_t*)(f_data->priv))->flag)
					BZ2_bzDecompressEnd( &((struct priv_bzlib_t*)(f_data->priv))->b_strm);
				free(f_data->priv);
			}
		break;
	}
	return ret;
}



//zip
int file_zip(unsigned char *buf, int *size, __u32 scan , int flag, struct found_data_t* f_data){
	int			ret = 0;
	int			j,i, b_count;
	char			token[5];
	__u32			last_match = 0;
	struct zip_priv_t	*z_priv = NULL;
	__u32			offset;
	
	sprintf(token,"%c%c%c%c",0x50,0x4b,0x05,0x06);

	switch (flag){
		case 0 :if(f_data->scantype & DATA_READY){
				*size = (f_data->next_offset)?f_data->next_offset : current_fs->blocksize ;
				ret = 1;
			}
			else{
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
				else{	
					*size += 4;
					ret = 2;
				}
			}
			break;
		case 1 :
			return ((scan & (M_IS_META | M_CLASS_1 | M_BLANK))||(f_data->scantype & DATA_READY)) ? 0 :1 ;
			break;
		case 2 :
			offset = 0;
			z_priv = malloc(sizeof(struct zip_priv_t));
			if (!z_priv){
				f_data->first = 0;
				return 0;
			}
			z_priv->flag = 0;
			z_priv->count = 0;
			f_data->priv = z_priv;
			b_count = (f_data->buf_length > 12) ? 12 : f_data->buf_length ;
			ret = follow_zip(buf, b_count, &offset, &last_match,f_data->priv);
			ret = analysis_ret1("ZIP", f_data, ret , b_count, offset, last_match);
			break;
		case 3 :
			offset = f_data->next_offset ;
			ret = follow_zip(buf, f_data->buf_length, &offset, &last_match,f_data->priv);
			ret = analysis_ret2("ZIP", f_data, ret, offset, last_match);
		break;
		case 4 :
			if (f_data->priv)
				free(f_data->priv);
		break;
	}
	return ret;
}


//x-compress
int file_lzw(unsigned char *buf, int *size, __u32 scan , int flag, struct found_data_t* f_data){
	int		ret = 0;
	switch (flag){
		case 0 :
			if(f_data->scantype & DATA_METABLOCK){
				ret = 1;
				break;
			}
			else{
				if (*size < (current_fs->blocksize -3)){
					*size += 2;
					ret = 2;
				}
				if (*size < (current_fs->blocksize -6)){
					*size += 2;
					ret = 1;
				}
			}
			break;
		case 1 :
			return (scan & (M_IS_META | M_CLASS_1)) ? 0 :1 ;
			break;
		case 2 :
			if ((buf[2] & 0x60) || ((buf[2] & 0x1F)<12) || ((buf[2] & 0x1F)>16))
				f_data->first = 0;
			break;
	}
	return ret;
}

//rpm
int file_rpm(unsigned char *buf, int *size, __u32 scan , int flag, struct found_data_t* f_data){
#define GZ_FTEXT        1
#define GZ_FHCRC        2
#define GZ_FEXTRA       4
#define GZ_FNAME        8
#define GZ_FCOMMENT     0x10

	int			i, z_flags, b_count, ret = 0;
	__u32			offset;
	__u32			last_match = 0;
	__u32			h_count,h_size;
	__u32			p_size = 0;
	

	switch (flag){
		case 0 :
			if(f_data->scantype & DATA_READY){
				*size = (f_data->next_offset)?f_data->next_offset : *size;
				ret = 1;
			}
			else{
				if (f_data->scantype & DATA_LENGTH)  {
					if (f_data->inode->i_size < f_data->size)
						ret = 0;
					else{
						p_size = ((f_data->size-1) % current_fs->blocksize)+1;
						 if (*size < p_size) 
							*size = p_size;
						ret = 1;
					}
				}
				else{
					if(*size < (current_fs->blocksize -8)) 
						ret = 1 ;
					else {
						if(*size < (current_fs->blocksize -4))
							ret = 2 ;
					}
					*size += 2;
				}
			}
			break;
		case 1 :
			return ((scan & (M_IS_META | M_CLASS_1 | M_BLANK | M_TXT))||(f_data->scantype & DATA_READY)) ? 0 :1 ;
			break;
		case 2 :
			if (!((buf[0]==0xed)&&(buf[1]==0xab)&&(buf[2]==0xee)&&(buf[3]==0xdb)&&(buf[4]==0x03))){
				f_data->func = file_default ;
				return 0;
			}
			if (buf[7] == 1){
				f_data->name[strlen(f_data->name)-3] = 0 ;
				strncat(f_data->name,"srpm",6);
			}
			offset=96;
			if ((buf[offset]== 0x8e)&&(buf[offset+1]== 0xad)&&(buf[offset+2]== 0xe8)) {
				h_count=((buf[offset+8]<<24)|(buf[offset+9]<<16)|(buf[offset+10]<<8)|(buf[offset+11]));
				h_size=((buf[offset+12]<<24)|(buf[offset+13]<<16)|(buf[offset+14]<<8)|(buf[offset+15]));
				for (i=0;i<h_count;i++){
					if((buf[offset+18+(i*16)]==0x03) && (buf[offset+19+(i*16)]==0xe8) && (buf[offset+23+(i*16)]==0x04)){
						p_size =((buf[offset+24+(i*16)]<<24)|(buf[offset+25+(i*16)]<<16)|
							(buf[offset+26+(i*16)]<<8)|(buf[offset+27+(i*16)]))+1;
						break;
					}
				}
				offset += (++h_count * 16);
				if (p_size)
					p_size = (buf[offset+p_size-1]<<24)|(buf[offset+p_size]<<16)|
						 (buf[offset+p_size+1]<<8)|(buf[offset+p_size+2]);
				offset += ((h_size + 7) & ~7);
				if (p_size)
					p_size += offset;

				if ((offset < 1024) && (buf[offset]== 0x8e)&&(buf[offset+1]== 0xad)&&(buf[offset+2]== 0xe8)) {
					h_count=((buf[offset+8]<<24)|(buf[offset+9]<<16)|(buf[offset+10]<<8)|(buf[offset+11]));
					h_size=((buf[offset+12]<<24)|(buf[offset+13]<<16)|(buf[offset+14]<<8)|(buf[offset+15]));
					offset += ((++h_count * 16) + h_size);
				}
			}
			b_count = (f_data->buf_length > 12) ? 12 : f_data->buf_length ;
			if (((offset+12) > (b_count *current_fs->blocksize)) || (buf[offset]!=0x1F)||(buf[offset+1]!=0x8B)||
			    (buf[offset+2]!=0x08)||((buf[offset+3]&0xe0)!=0) || (init_priv_zlib(f_data))){
				f_data->size = p_size ;
				f_data->scantype = DATA_LENGTH ;
				ret = 1;
				break;
			}
			else {
				((struct priv_zlib_t*)(f_data->priv))->flag = 0xb;

			 	if (!follow_zlib(NULL,0,NULL,NULL,(struct priv_zlib_t *)f_data->priv)){
					z_flags=buf[offset+3];
					offset += 10;
					if((z_flags&GZ_FEXTRA)!=0){
						offset += (buf[offset]|(buf[offset+1]<<8));
						offset += 2;	
					}
					if((z_flags&GZ_FNAME)!=0){
						while( (offset < 256) && (buf[offset++]!='\0')){
						}
					}
					if((z_flags&GZ_FCOMMENT)!=0){
						while((offset<1024) && (buf[offset++]!='\0')){
						}
					}
					if((z_flags&GZ_FHCRC)!=0){
						offset+=2;
					}
					ret = follow_zlib(buf, b_count, &offset, &last_match,(struct priv_zlib_t *)f_data->priv);
					ret = analysis_ret1("RPM", f_data, ret , b_count, offset, last_match);
				}
				else
					f_data->first = 0;
			}
		break;	
		case 3 :
			offset = f_data->next_offset ;
			ret = follow_zlib(buf, f_data->buf_length, &offset, &last_match,(struct priv_zlib_t *)f_data->priv);
			ret = analysis_ret2("RPM", f_data, ret, offset, last_match);
		break;
		case 4:
			if (f_data->priv){
				if (((struct priv_zlib_t*)(f_data->priv))->flag)
					inflateEnd( &((struct priv_zlib_t*)(f_data->priv))->z_strm);
				free(f_data->priv);
			}
		break;
	}
	return ret;
}


//ttf
int file_ttf(unsigned char *buf, int *size, __u32 scan , int flag, struct found_data_t* f_data){
	__u32		table_id[9] = {0x70616d63,0x66796c67,0x64616568,0x61656868,0x78746d68,0x61636f6c,0x7078616d,0x656d616e,0x74736f70};
	int		i,j,id_count,ret = 0;
	__u16		tables ; //s_range, shift;
	__u32		*p, id, tmp;
	unsigned char	*c;

	switch (flag){
		case 0 :
			if(((*size) < (current_fs->blocksize - 16))|| (f_data->inode->i_size > f_data->size)){
				*size = ((*size ) + 3) & ~3 ;
				ret =1;
			}
			else {
				*size = ((*size ) + 3) & ~3 ;
				ret = 2;
			}
			break;
		case 1 :return (scan & (M_IS_META | M_CLASS_1 | M_BLANK)) ? 0 :1 ;
			break;
		case 2 :
			id_count = 0;
			tables = (buf[4]<<8) + buf[5];
			c = (unsigned char*) buf + 12;
			for (i = 0; i< tables; i++){
				p = (__u32*)c;
				id = ext2fs_le32_to_cpu(*p);
				for (j=0;j<9;j++){
					if (id == table_id[j]){
						id_count++;
						break;
					}
				} 
				for (j=0;j<4;j++){
					if (( *c < 32) || ((*c > 126) && (*c != 0xa9))){
						f_data->func = NULL;
						return 0;
					}
					c++;
				}
				p = (__u32*)(c + 4);
				tmp = ext2fs_be32_to_cpu(*p);
				p++;
				tmp += ext2fs_be32_to_cpu(*p);
				if (f_data->size < tmp) 
					f_data->size = tmp;
				c+=12;
			}
			if(id_count < 5){
				f_data->func = NULL;
				ret = 0;
			}
			else {
				f_data->size = (f_data->size +3) & ~3 ;
				f_data->scantype = DATA_LENGTH ;
				ret = 1;
			}
		break;
	}
	return ret;
}
			


//iso9660 CD-ROM
int file_iso9660(unsigned char *buf, int *size, __u32 scan , int flag , struct found_data_t* f_data){
	__u64		lsize = 0;
	__u32		*p_32;
	__u16		*p_16;
	int		ssize;
	int		ret = 0;
	char		cd_str[] = "CD001";

	switch (flag){
		case 0 :
			if (f_data->size || f_data->h_size) {
				if (f_data->h_size != f_data->inode->i_size_high)
					ret = 0;
				else{
					if (f_data->inode->i_size < f_data->size)
						ret = 0;
					else{
						ssize = ((f_data->size-1) % current_fs->blocksize)+1;
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
				if ((!strncmp(((char*)buf+0x8001),cd_str,5)) && (ext2fs_le32_to_cpu(*(p_32 +1)) == ext2fs_be32_to_cpu(*p_32))){
					lsize = (__u64)(ext2fs_le32_to_cpu(*p_32)) * ext2fs_le16_to_cpu(*p_16);
					f_data->size = lsize & 0xFFFFFFFF;
					f_data->h_size = lsize >> 32;
					ret =1;
				}
			}
			if (!lsize)
				f_data->first = 0;
			break;
	}
	return ret;
}


//gnumeric    only switch to gzip or xml
int file_gnumeric(unsigned char *buf, int *size, __u32 scan , int flag , struct found_data_t* f_data){ 
	if ((buf[0]==0x1F)&&(buf[1]==0x8B)&&(buf[2]==0x08)&&((buf[3]&0xe0)==0))
		f_data->func = file_gzip;
	else	
		f_data->func = file_txt;
	return f_data->func(buf, size,scan,flag,f_data);
}



//dvi 
int file_dvi(unsigned char *buf, int *size, __u32 scan , int flag , struct found_data_t* f_data){ 
	int ret = 0;
	int i,j;
	switch (flag){
		case 0 :
			j = 3;
			i = (*size) -1;
			while ((i >= 0) && (j >= 0) && (buf[i] == 0xdf)){
				i--;
				j--;
			}
			if ((i == -1) || (j == -1)){
				ret = 1;
			}
			break;
			
		case 1 :
			return (scan & (M_IS_META | M_CLASS_1| M_BLANK | M_TXT)) ? 0 :1 ;
			break;
		case 2 :
			if(buf[14]){  //test for preamble comment
				for (i=0; i<buf[14]; i++){
					if (!(buf[15+i] && (isprint(buf[15+i]) || isspace(buf[15+i])))){
						f_data->func = NULL;
						return 0;
					}
				}
			}
			else 
				f_data->func = NULL;
			break;
	}
	return ret;
}



//lzma
int file_lzma(unsigned char *buf, int *size, __u32 scan , int flag , struct found_data_t* f_data){ 
	int ret = 0;
	switch (flag){
		case 0 :
			if ((*size< (current_fs->blocksize - 3))) 
					ret = 1;
			break;
			
		case 1 :
			return (scan & (M_IS_META | M_CLASS_1| M_BLANK | M_TXT)) ? 0 :1 ;
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
						ssize = ((f_data->size-1) % current_fs->blocksize)+1;
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
			f_data->scantype = DATA_LENGTH ;
			ret = 1;
			break;
	}
	return ret;
}


static int follow_rar(unsigned char *buf, __u16 blockcount, __u32 *offset, __u32 *last_match,  int flag){
	int	ret = 1 ;
	__u32	tmp ;
	__u32	f_offset = *offset;
	
	while ((ret ==1) && (f_offset < ((blockcount * current_fs->blocksize)-10))){
		switch (buf[f_offset +2]){
			case 0x7b:
				if(zero_space(buf,f_offset+7))
					ret = 2;
			case 0x72:
				*last_match = f_offset;
				f_offset += ((buf[f_offset +6]<<8) + (buf[f_offset +5]));
				break;
			case 0x73:
			case 0x74:
			case 0x75:
			case 0x76:
			case 0x77:
			case 0x78:
				*last_match = f_offset;
				tmp = ((buf[f_offset +6]<<8) + (buf[f_offset +5]));
				tmp += ((buf[f_offset +10]<<24) + (buf[f_offset +9]<<16) + (buf[f_offset +8]<<8) + (buf[f_offset +7]));
				f_offset += tmp;
				break;
			default:
				ret = 0;
		}
	}
	*offset = f_offset;
	return ret;
}


//rar
int file_rar(unsigned char *buf, int *size, __u32 scan , int flag, struct found_data_t* f_data){
	int 	i,j, b_count;
	int 	ret = 0;
	__u32	offset;
	__u32	last_match = 0;
	unsigned char token[7]={0xc4, 0x3d, 0x7b, 0x00, 0x40, 0x07, 0x00 };

	switch (flag){
		case 0 :
			if (f_data->scantype & DATA_READY){
				*size = f_data->size % current_fs->blocksize;
				ret = 1;
				break;
			}

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
			return ((scan & (M_IS_META | M_CLASS_1 | M_BLANK))||(f_data->scantype & DATA_READY)) ? 0 :1 ;
			break;
		case 2 :
			offset = 0;
			b_count = (f_data->buf_length > 12) ? 12 : f_data->buf_length ;
			ret = follow_rar(buf, b_count, &offset, &last_match,0);
			ret = analysis_ret1("RAR", f_data, ret , b_count, offset, last_match);
	  	break;

		case 3 :
			offset = f_data->next_offset ;
			ret = follow_rar(buf, f_data->buf_length, &offset, &last_match,0);
			ret = analysis_ret2("RAR", f_data, ret, offset, last_match);
		break;
	}
	return ret;
}	 


static int follow_cpio(unsigned char *buf, __u16 blockcount, __u32 *offset, __u32 *last_match,  int flag){
	struct header_old_cpio {
		   unsigned short   c_magic;
		   unsigned short   c_dev;
		   unsigned short   c_ino;
		   unsigned short   c_mode;
		   unsigned short   c_uid;
		   unsigned short   c_gid;
		   unsigned short   c_nlink;
		   unsigned short   c_rdev;
		   unsigned short   c_mtime[2];
		   unsigned short   c_namesize;
		   unsigned short   c_filesize[2];
	   };
	struct cpio_odc_header {
		   char    c_magic[6];
		   char    c_dev[6];
		   char    c_ino[6];
		   char    c_mode[6];
		   char    c_uid[6];
		   char    c_gid[6];
		   char    c_nlink[6];
		   char    c_rdev[6];
		   char    c_mtime[11];
		   char    c_namesize[6];
		   char    c_filesize[11];
	   };
	struct cpio_newc_header {
		   char    c_magic[6];
		   char    c_ino[8];
		   char    c_mode[8];
		   char    c_uid[8];
		   char    c_gid[8];
		   char    c_nlink[8];
		   char    c_mtime[8];
		   char    c_filesize[8];
		   char    c_devmajor[8];
		   char    c_devminor[8];
		   char    c_rdevmajor[8];
		   char    c_rdevminor[8];
		   char    c_namesize[8];
		   char    c_check[8];
	   };
	char				footer[]="TRAILER!!!";
	int				i, ret = 1; //j;
	__u64 				f_offset = (__u64) *offset;
	__u32				n_len ;
	__u64				f_len ;
	__u8				*o_str;
	struct header_old_cpio 		*old_cpio;
	struct cpio_odc_header		*odc_header;
	struct cpio_newc_header		*newc_header;
	char				help[9];

	while ((ret ==1) && (f_offset < ((blockcount * current_fs->blocksize)-122))){
		n_len = 0;
		f_len = 0;
		old_cpio = (struct header_old_cpio*) (buf+f_offset) ;
		if ((old_cpio->c_magic == 0xc771)||(old_cpio->c_magic == 0x71c7)){
			if (strstr((char*)buf+f_offset+sizeof(struct header_old_cpio),footer))
				ret=2;

			if (old_cpio->c_magic == 0xc771)
				f_offset +=((ext2fs_be16_to_cpu(old_cpio->c_filesize[0])<<16) +
					((ext2fs_be16_to_cpu(old_cpio->c_filesize[1])+1) & ~1) +
					((ext2fs_be16_to_cpu(old_cpio->c_namesize) +1) & ~1));
			
			else
				f_offset +=((ext2fs_le16_to_cpu(old_cpio->c_filesize[0])<<16) +
					((ext2fs_le16_to_cpu(old_cpio->c_filesize[1])+1) & ~1) +
					((ext2fs_le16_to_cpu(old_cpio->c_namesize) +1) & ~1));
			*last_match = f_offset+1;
			f_offset += sizeof(struct header_old_cpio);
			continue;
		}
		else{
			if( (buf[f_offset] == 0x30)&&(buf[f_offset+1] == 0x37)&&(buf[f_offset+2] == 0x30)&&(buf[f_offset+3] == 0x37)&&(buf[f_offset+4] == 0x30)){
				if (buf[f_offset+5] == 0x37){
					if (strstr((char*)buf+f_offset+sizeof(struct cpio_odc_header),footer))
						ret=2;
					odc_header = (struct cpio_odc_header*) old_cpio;
					o_str = (__u8*) &(odc_header->c_namesize);
					for (i=0;i<6;i++){
						n_len <<= 3;
						n_len += (*o_str & 0x7) ;
						o_str++;
					}
					o_str = (__u8*) &(odc_header->c_filesize);
					for (i=0;i<11;i++){
						f_len <<= 3;
						f_len += (*o_str & 0x7) ;
						o_str++;
					}
					if(n_len || f_len){
						*last_match = f_offset+1;
						f_offset += n_len + f_len + sizeof(struct cpio_odc_header);
						continue;
					}
				}
				else {
					if (strstr((char*)buf+f_offset+sizeof(struct cpio_newc_header),footer))
						ret=2;
					newc_header = (struct cpio_newc_header*) old_cpio;
					memcpy(help,newc_header->c_namesize,8);
					help[8] = 0;
					n_len = strtoul(help,NULL,16);
					memcpy(help,newc_header->c_filesize,8);
					f_len = strtoul(help,NULL,16);
					if(n_len || n_len){
						n_len = (n_len + sizeof(struct cpio_newc_header)+ 3) & ~3;
						*last_match = f_offset+1;
						f_offset += n_len + f_len ;
						f_offset = ((f_offset +3) & ~3);
						continue;
					}
					
				}	
			}
		}
		ret = 0;
	}
	*offset = f_offset;
	return ret;
}


//cpio
int file_cpio(unsigned char *buf, int *size, __u32 scan , int flag, struct found_data_t* f_data){
	int 	i,j,b_count;
	int 	ret = 0;
	__u32	offset;
	__u32	last_match = 0;
	char	token[]="TRAILER!!!";

	switch (flag){
		case 0 :
			if ((((__u64)f_data->inode->i_size |((__u64)f_data->inode->i_size_high<<32)) >= (__u64)f_data->size) &&  
				((*size) < (current_fs->blocksize - 0xff))){
				if (!(*size))
					*size = current_fs->blocksize;
				else
					*size = ((*size) + 0x01ff) & 0xfe00 ;
				ret = 1;
			}else{

				if (*size){
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
				}
			}	
			break;

		case 1 :	
			return ((scan & (M_IS_META))||(f_data->scantype & DATA_READY)) ? 0 :1 ;   //FIXME
			break;

		case 2 :
			offset = 0;
			b_count = (f_data->buf_length > 12) ? 12 : f_data->buf_length ;
			ret = follow_cpio(buf, b_count, &offset, &last_match,0);
			ret = analysis_ret1("CPIO", f_data, ret , b_count, offset, last_match);
	  	break;

		case 3 :
			offset = f_data->next_offset ;
			ret = follow_cpio(buf, f_data->buf_length, &offset, &last_match,0);
			ret = analysis_ret2("CPIO", f_data, ret, offset, last_match);
		break;
	}
	return ret;
}	 

//function currently not used 
/*static int a2u(unsigned char *buf, __u32 *value){
	int		count=0;
	__u32		tmp=0;
	
	while ((buf[0+count]==0x20) ||(buf[0+count]==0x0a) ||(buf[0+count]==0x0d)){
		count++;
	}
	if ((buf[0+count] & 0xc0) != 0x30)
		return 0; 
	while ((buf[0+count] > 0x2f) && (buf[0+count] < 0x3a)){
		tmp *= 10;
		tmp += (buf[0+count] & 0x0f);
		count++;
	}
	while (buf[0+count]==0x20){
		count++;
	}
	if (count > 1){
		*value = tmp;
		return count;
	}
	return 0;
}*/



static int follow_archive(unsigned char *buf, __u16 blockcount, __u32 *offset, __u32 *last_match,  int flag){
	int ret = 1;
	__u32	tmp ;
	__u32	end = blockcount * current_fs->blocksize ;
	__u32	f_offset = *offset;
	__u8		*o_str;
	
	if ((blockcount ==1) && (f_offset>=(end-59)) && (zero_space(buf,f_offset)))
		ret = 2;
	while ((ret ==1) && (f_offset < (end-59))){
		if (buf[f_offset] == 0x0a){
			f_offset++;
			continue;
		}
		if ((!buf[f_offset]) && (zero_space(buf,f_offset)))
				ret = 2;
		else{
			if ((buf[58+f_offset]==0x60)&&(buf[59+f_offset]==0x0a)){
				tmp = 0;
				o_str = (buf + 48 + f_offset);
				while ((*o_str > 0x2f) && (*o_str < 0x3a) && (o_str < (buf + 58 + f_offset))){
					tmp *= 10;
					tmp += (*o_str & 0x0f);
					o_str++;
				}
				if (!tmp)
					ret = 0;
				*last_match = f_offset;
				f_offset += (tmp + 60);
			}
			else 
				ret = 0;
		}
	}
	*offset = f_offset;
	return ret;
}


//x-archive
int file_archive(unsigned char *buf, int *size, __u32 scan , int flag, struct found_data_t* f_data){
	int		ret = 0;
	__u32		offset,b_count,last_match = 0;

	switch (flag){
		case 0 :
			if(f_data->scantype & DATA_READY){
				*size = (f_data->next_offset)?f_data->next_offset : *size;
				ret = 1;
			}else{
				if((*size) < (current_fs->blocksize - 16)){
					*size = ((*size ) + 3) & ~3 ;
					ret =1;
				}
				else {
					*size = ((*size ) + 3) & ~3 ;
					ret = 4;
				}
			}
			break;
		case 1 :return ((scan & (M_IS_META | M_CLASS_1 | M_BLANK))||(f_data->scantype & DATA_READY)) ? 0 :1 ;
			break;
		case 2 :
			if((buf[0]==0x21)&&(buf[1]==0x3c)&&(buf[2]==0x61)&&(buf[3]==0x72)&&(buf[4]==0x63)&&(buf[5]==0x68)&&(buf[6]==0x3e))
				offset = 8;
			else{
				f_data->func = NULL;
				return 0;
			}
			b_count = (f_data->buf_length > 12) ? 12 : f_data->buf_length ;
			ret = follow_archive(buf, b_count, &offset, &last_match,0);
			ret = analysis_ret1("AR", f_data, ret , b_count, offset, last_match);
	  	break;
		case 3 :
			offset = f_data->next_offset ;
			ret = follow_archive(buf, f_data->buf_length, &offset, &last_match,0);
			ret = analysis_ret2("AR", f_data, ret, offset, last_match);
		break;
	}
	return ret;
}

struct priv_pdf_t {
	__u32		flag;
	__u32		o_nr;
	__u32		xref_0;
	__u32		xref_1;
	__u32		len;
};

int pdf_is_obj(unsigned char *buf,__u32 offset){
	int i,j;

	if(isdigit(buf[offset])){
		i=1;
		while (isdigit(buf[offset + i]) && (i<12))
			i++;
		if (buf[offset + i] == 0x20){
			i++;
			j=0;
			while (isdigit(buf[offset + i + j]) && (j<4))
			j++;
			i += j;
			if ((buf[offset + i] == 0x20) && (buf[offset + i + 1] == 0x6f) &&
				(buf[offset + i + 2] == 0x62) &&  (buf[offset + i + 3] == 0x6a)){
					return (i+4);
			}
		}
	}
	return 0;
}


static int follow_pdf(unsigned char *buf, __u16 blockcount, __u32 *offset, __u32 *last_match,  struct priv_pdf_t *priv){
	int 		nr,tmp,i,ret = 1;
	__u32		f_offset = *offset;
	__u32		end = (blockcount * current_fs->blocksize)-20;
	__u32		o_nr;

	while ((ret ==1) && (f_offset < end)){
		switch (priv->flag & 0x07){
			case 0:
				while ((f_offset < end) && (!((buf[f_offset] == 0x0d) || (buf[f_offset] == 0x0a))))
					f_offset++;
				if (f_offset == end)
					break;
				else 
					(priv->flag)++;
			case 1:
				while ((f_offset < end) && ((buf[f_offset] == 0x0d) || (buf[f_offset] == 0x0a) || (buf[f_offset] == 0x20)))
					f_offset++;
				if (f_offset == end){
					break;
				}
				else
					(priv->flag)++;
			case 2:
//				if (f_offset > end)
//					break;
				switch (priv->flag & 0x70){
					case 0: //obj
						tmp = pdf_is_obj(buf,f_offset);
						if (tmp){
							*last_match = f_offset;
							o_nr=0;
							i=0;
							while (isdigit(buf[f_offset +i])){
								o_nr *= 10;
								o_nr += (buf[f_offset +i] - 0x30);
								i++;
							}
							if (buf[f_offset+i] != 0x20) 
								ret = 0;
							if (o_nr > priv->o_nr)
								priv->o_nr = o_nr; 
							f_offset += tmp;
							priv->flag = 0x10;
//		printf("obj   %08x\n",priv->len - *offset +f_offset);
							break;
						}
						//xref
						if((buf[f_offset] ==0x78)&& (buf[f_offset+1] ==0x72) && (buf[f_offset+2] ==0x65) &&
							 (buf[f_offset+3] ==0x66)){
								*last_match = f_offset;
								priv->xref_1 = priv->xref_0;
								priv->xref_0 = priv->len - *offset + f_offset;
								f_offset += 4;
								priv->flag = 0x40;
//		printf("xref   %08x\n",priv->len - *offset +f_offset);
								break;
						}	
						 //startxref whithout xref
						if((buf[f_offset] ==0x73)&& (buf[f_offset+1] ==0x74) && (buf[f_offset+2] ==0x61) &&
							 (buf[f_offset+3] ==0x72) && (buf[f_offset+4] ==0x74) && (buf[f_offset+5] ==0x78)&&
							 (buf[f_offset+6] ==0x72)&&(buf[f_offset+7] ==0x65) && (buf[f_offset+8] ==0x66)){
							priv->flag = 0x30;
							*last_match = f_offset;
							f_offset += 9;
//		printf("startxref   %08x\n",priv->len - *offset +f_offset);	
							break;				
						}
						
					break;

						 break;
					case 0x10 ://endobj
						if((buf[f_offset] ==0x65)&& (buf[f_offset+1] ==0x6e) && (buf[f_offset+2] ==0x64) &&
							 (buf[f_offset+3] ==0x6f) && (buf[f_offset+4] ==0x62) && (buf[f_offset+5] ==0x6a)){
								*last_match = f_offset;
								f_offset += 6;
								priv->flag = 0x00;
//		printf("endobj   %08x\n",priv->len - *offset +f_offset);
								break;
						}
						//xref (if no linebreak before the last endobj
						if((buf[f_offset] ==0x78)&& (buf[f_offset+1] ==0x72) && (buf[f_offset+2] ==0x65) &&
							 (buf[f_offset+3] ==0x66)){
								*last_match = f_offset;
								priv->xref_1 = priv->xref_0;
								priv->xref_0 = priv->len - *offset + f_offset;
								f_offset += 4;
								priv->flag = 0x40;
//		printf("xref   %08x\n",priv->len - *offset +f_offset);
								break;
						}	


					break;
					case 0x30 : //check offset of the xref table
						if (!isdigit(buf[f_offset]))
							ret = 0;
						else {	
							i=0;
							nr = 0;
							while (isdigit(buf[f_offset +i])){
								nr *= 10;
								nr += (buf[f_offset +i] - 0x30);
								i++;
							}
							if(nr && priv->xref_0 && (nr != priv->xref_0)&&(nr != priv->xref_1)){
//								printf("PDF : xref table check failed\n");
								ret = 0;
							}
							else {
								priv->flag = 0x60;
								f_offset += i;
							}
						}
						break;
					case 0x40 : //trailer
						if((buf[f_offset] ==0x74)&& (buf[f_offset+1] ==0x72) && (buf[f_offset+2] ==0x61) &&
							 (buf[f_offset+3] ==0x69) && (buf[f_offset+4] ==0x6c) && (buf[f_offset+5] ==0x65)&&
							 (buf[f_offset+6] ==0x72)){
							priv->flag = 0x50;
							*last_match = f_offset;
							f_offset += 7;
//		printf("trailer   %08x\n",priv->len - *offset +f_offset);
						}
						else{	while (isdigit(buf[f_offset]))
								f_offset++;
							while (buf[f_offset] == 0x20)
								f_offset++;
							if(! isdigit(buf[f_offset])){
								ret= 0;
								break;
							}
							i=0;
							nr = 0;
							while (isdigit(buf[f_offset +i])){
								nr *= 10;
								nr += (buf[f_offset +i] - 0x30);
								i++;
							}
							f_offset += ((nr * 20)+i);
						}
					break;
					case 0x50 : //startxref
						if((buf[f_offset] ==0x73)&& (buf[f_offset+1] ==0x74) && (buf[f_offset+2] ==0x61) &&
							 (buf[f_offset+3] ==0x72) && (buf[f_offset+4] ==0x74) && (buf[f_offset+5] ==0x78)&&
							 (buf[f_offset+6] ==0x72)&&(buf[f_offset+7] ==0x65) && (buf[f_offset+8] ==0x66)){
							priv->flag = 0x30;
							*last_match = f_offset;
							f_offset += 9;
//		printf("startxref   %08x\n",priv->len - *offset +f_offset);					
						}
						//else
						//	break;
					break;
					case 0x60 ://EOF
						if((buf[f_offset] ==0x25)&& (buf[f_offset+1] ==0x25) && (buf[f_offset+2] ==0x45) &&
							 (buf[f_offset+3] ==0x4f) && (buf[f_offset+4] ==0x46)){
							*last_match = f_offset;
							f_offset += 5;
//		printf("EOF   %08x\n",priv->len - *offset +f_offset);
							while ((buf[f_offset] == 0x0d) || (buf[f_offset] == 0x0a))
								f_offset++;
							if (zero_space(buf,f_offset)){
								ret = 2;
							}
							else {
								f_offset--;
								priv->o_nr = 0;
								priv->flag = 0;
							}
						}
						else
							ret = 0;
					break;
				}//while 0x70
				priv->flag &= (~0x07);	
		}//while 0x07
	}
	*offset = f_offset;
	return ret;
}

//pdf
int file_pdf(unsigned char *buf, int *size, __u32 scan , int flag, struct found_data_t* f_data){
	int 	i,j;
	int 	ret = 0;
	__u32	offset;
	__u32	last_match = 0;
	int	b_count;
	struct priv_pdf_t *priv = NULL;
	char	token[6];
	sprintf(token,"%c%c%c%c%c",0x25,0x45,0x4f,0x46,0x0a);

	switch (flag){
		case 0 :
			if(f_data->scantype & DATA_READY){
				*size = (f_data->next_offset)?f_data->next_offset : *size;
				ret = 1;
			}else{
				if ((*size) < 3)
					ret =2;
					else{ 
					j = strlen(token) -2;
					i = (*size) -2;
					if(buf[i] != 0x46)
						i--;
		
					while ((i >= 0) && (j >= 0) && (buf[i] == token[j])){
						i--;
						j--;
					}
					if ((i == -1) || (j == -1)){
						ret=1;
					}
				}
			}
			break;
		case 1 :
			return ((scan & (M_IS_META | M_CLASS_1))||(f_data->scantype & DATA_READY)) ? 0 :1 ;
			break;
		case 2 :
			offset = 0;
			priv = malloc(sizeof(struct priv_pdf_t));
			priv->o_nr = 0;
			priv->flag = 0;
			priv->xref_0 = 0;
			priv->xref_1 = 0;
			priv->len  = 0;
			f_data->priv = priv;
			b_count = (f_data->buf_length > 12) ? 12 : f_data->buf_length ;
			ret = follow_pdf(buf, b_count, &offset, &last_match,f_data->priv);
			ret = analysis_ret1("PDF", f_data, ret , b_count, offset, last_match);
	  	break;

		case 3 :
			offset = f_data->next_offset ;
			((struct priv_pdf_t*)(f_data->priv))->len = f_data->size;
			ret = follow_pdf(buf, f_data->buf_length, &offset, &last_match,f_data->priv);
			ret = analysis_ret2("PDF", f_data, ret, offset, last_match);
		break;
		case 4 :
			if(f_data->priv)
				free (f_data->priv);
		break;
		}
	return ret;
}


#define MAX_SMTP_CONTENT 16
struct priv_smtp_t {
	__u16		flag;
	__u16		len;
	struct {
		__u16		c_flag;
		__u16		crc;
		__u16		crc_len;
		__u16		c_code;
	} slot[MAX_SMTP_CONTENT];
};

static __u16 cull;

static int read_smtp_len(unsigned char *buf,__u32 f_offset, __u32 end, __u16 flag){
__u32	 i = f_offset;
int	 len = 0;
int 	 exception = 0;
__u16	save_flag = flag;

	if ((buf[i]==0x0a) ||((buf[i]==0x0d)&&(buf[i+1]==0x0a)))
		return 1;
	if (!(buf[i]))
		return -2; 
	for (;(i<end); i++,len++){
		switch (flag & 0xff00){
			case 0x400:
				if (!( (buf[i]==0x0a) | (buf[i]==0x0d) | (buf[i]==61) | (buf[i]==43) 
				    | ((buf[i]<58) && (buf[i]>46))
				    | ((buf[i]<91) && (buf[i]>64)) 
				    | ((buf[i]<123) && (buf[i]>96)))){
					if ((buf[i]==0x20) && ((buf[i+1]==0x0a)||(buf[i+1]==0x0a))){
					//Warning, there is a blanc
					}
					else 
						return 0;
				}
			case 0x200:
				if ((buf[i]<0x07) || ((buf[i]<0x20) && (buf[i]>0x0d)))
				return 0;
			break;
			case 0x800:
				if (((i-f_offset)==8) && (!(strncmp("Subject:", (char*)buf+f_offset, 8)))){
					flag = 0x200; 
					exception = 2;
				}
				if ((buf[i] == '=') && (buf[i+1] == '?')){
					flag = 0x200;
					 exception =1;
				}

			case 0x000:
				if (buf[i] & 0x80)
					cull++;
				if ((buf[i]<0x09) || ((buf[i]<0x20) && (buf[i]>0x0d)))
					return 0;
				if ((buf[i] == 0x3c) && (buf[i+3] == 0x3e) && ((buf[i+1] & 0x20) == 0x42) && ((buf[i+1] & 0x20) == 0x52)){
					cull++;
					len = 0;
				}				
			break;
			case 0x100:
				if ((buf[i]<0x09) || (buf[i] & 0x80) || ((buf[i]<0x20) && (buf[i]>0x0d)))
					return 0;
			break;
			
		}
		if (buf[i]==0x0a){
			if (len > 1000)
				return 0;
			
			if (((flag & 0x800) | exception) && ((buf[i+1]==0x20)||(buf[i+1]==0x09))){
				i++;
				len = -1;
				flag = save_flag;
				if (exception == 1)
					exception = 0;
			}
			else {
				return ( i - f_offset + 1);
			}
		}
	}
	return (len < 1000)? -1 : 0;
}

static int read_smtp_codestr(unsigned char *buf,__u32 f_offset, int len){
	char 		codestr[] ="7bit 7Bit quoted-printable Quoted-printable 8bit 8Bit binary Binary base64 Base64 ";
	char* 		p_search;
	char		token[20];
	int		i;
	int		flag = 0;
	int 		type = -1;
	
	if (len < 32)
		return 0;
	p_search = codestr;
	while (*p_search){
		i=0;
		while((*p_search) != 0x20){
			token[i] = *p_search;
			p_search++;
			i++;
		}
		token[i] = 0;
		type++;
		if (!strncmp((char*)buf + 27 + f_offset,token,strlen(token))){
			flag = 1;	
			break;
		}
		p_search++;
	}
	if(!flag) type=0;
	return (type >>1);
}


static int smtp_decode_content_type(unsigned char * buf, __u32 offset, int len, struct priv_smtp_t *priv){
	int i,j,slot;	
		
	if (!strncmp("Content-Type:",(char*)buf+offset,12)) {
		i = 13;
		while ((i<(len-6)) && (buf[offset +i] != ';')) i++;
		while ((i<(len-6)) && (!((buf[offset +i] == 'd') && // dary=
			(buf[offset +i+1] == 'a') &&
			(buf[offset +i+2] == 'r') &&
			(buf[offset +i+3] == 'y') &&
			(buf[offset +i+4] == '=')))) i++;
		if (i < (len-6)){
			i += 5;
			//while ((i<(len-6)) && (buf[offset +i] != '"')) i++;
			if (buf[offset +i] == '"') i++;
			j = i +1;
			while ((buf[offset +j] != '"') && (buf[offset +j] != 0x0a) && (buf[offset +j] != 0x0d)) j++;
			if (--j < len){
				for(slot=0;slot<MAX_SMTP_CONTENT;slot++)
					if (!priv->slot[slot].c_flag)
						break;
				if (slot==MAX_SMTP_CONTENT){
					printf("SMTP no free slot found\n");
					return  0;
				}
				else{
					priv->slot[slot].crc = FLAC__crc16(buf+offset+i,j-i+1,0);
					priv->slot[slot].crc_len = j-i+1;
					priv->slot[slot].c_flag = 1;
				}
			}
			else{
				return 0;
			}
		}
	}
	return 1;
}

static int follow_smtp(unsigned char *buf, __u16 blockcount, __u32 *offset, __u32 *last_match,  struct priv_smtp_t *priv){
	int 		i = 0;
	int		ret = 1;
	__u32		f_offset = *offset;
	__u32		end = (blockcount * current_fs->blocksize)-20;
	int		len = 0;
	int		tmp_slot,slot = 0;
	__u16		tmp_flag = 0; //crc,crc_d ;
		
	if (end == f_offset)
		end++;
	while ((ret ==1) && (f_offset < end)){
		switch (priv->flag & 0x07){
			case 0 :  //header
				len = read_smtp_len(buf,f_offset,end,0x800);
				switch (len){
					case 0 :
						ret = 0;
					break;
					case -1 :
						*offset = f_offset;
						return 1;
					break;
					case -2 :
						for(i=0;i<MAX_SMTP_CONTENT;i++)
							if (priv->slot[i].c_flag)
								return 0;
						*offset = f_offset;
						return 2;
					break;
					case 1 :
						if (buf[f_offset]== 0x0d)
							f_offset++;
						f_offset++;
						*last_match = f_offset;
						cull = 0;
						priv->flag = (priv->flag & ~0x0f) | 1;
					break;
					default :
						if (!strncmp("From: ", (char*)buf+f_offset, 6)
						 || !strncmp("From ", (char*)buf+f_offset, 5)
						 || !strncmp("To: ", (char*)buf+f_offset, 4)
						 || !strncmp("Reply-", (char*)buf+f_offset, 6)
						 || !strncmp("Cc: ", (char*)buf+f_offset, 4)
						 || !strncmp("Bcc: ", (char*)buf+f_offset, 5)
						 || !strncmp("Content-", (char*)buf+f_offset, 8)
						 || !strncmp("MIME-Version: ", (char*)buf+f_offset, 14)){
							 if(buf[f_offset + 8] == 'T') {
								if (!strncmp("Content-Transfer-Encoding:",(char*)buf+f_offset,26)){ 
									priv->flag = ((priv->flag & 0xff)|(read_smtp_codestr(buf,f_offset,len)<<8));
//FIXME									if (priv->flag & 0xff00
								}
								else
									ret  = smtp_decode_content_type(buf, f_offset, len, priv);
							}	
							if (ret) {
								*last_match = f_offset;
								f_offset += len;
							}
						}
						else{
							for(i=0;((i<len) && (!(buf[f_offset +i]== 0x3a)));i++){
								if(buf[f_offset +i]== 0x20){
									ret = 0;  //Error in Header
									break;
								}
							}
							if (ret){
								f_offset += len;
							}
						}
					break;
				}
			break;
			case 1 :  //space
				while ((buf[f_offset]== 0x0a) || (buf[f_offset]== 0x0d))
					f_offset++;
				if (!buf[f_offset]){
					for(i=0;i<MAX_SMTP_CONTENT;i++)
						if (priv->slot[i].c_flag)
								break;
					if (i==MAX_SMTP_CONTENT){
						*offset = f_offset;
						return 2;
					}
					else
						return 0;	
					
				}
				if (!strncmp("--", (char*)buf+f_offset, 2)){
					tmp_flag = (priv->flag & ~0x0f) | 3;
					priv->flag = (priv->flag & ~0x0f) | 2;
					break;
				}
				if (!strncmp("From ", (char*)buf+f_offset, 5)){
					for(i=0;i<MAX_SMTP_CONTENT;i++)
						if (priv->slot[i].c_flag)
								break;
					if (i==MAX_SMTP_CONTENT){
						priv->flag = 0;
						cull = 0;
						break;
					}
					else {
//						printf("SMTP Warning not all slots empty\n"); //FIXME
					}
				}
				priv->flag = (priv->flag & ~0x0f) | 3;
			case 3 : //MIME format
				len = read_smtp_len(buf,f_offset,end,priv->flag);
				switch (len){
					case 0 :
						ret = 0;
					break;
					case -1 :
						*offset = f_offset;
						return 1;
					break;
					case -2 : 
						for(i=0;i<MAX_SMTP_CONTENT;i++)
							if (priv->slot[i].c_flag)
								return 0;
						*offset = f_offset;
						return 2;
					break;
					case 1 :
						if (buf[f_offset]== 0x0d)
							f_offset++;
						f_offset++;
					break;
					default:
						if ((len > 30) && (!strncmp("This ", (char*)buf+f_offset, 5))){
							for(i=6;((i<len) && (!(buf[f_offset +i] == 'M')));i++);
							if ((i<len) && (!strncmp("MIME format", (char*)buf+f_offset+i, 11))){
								*last_match = f_offset;
								priv->flag = (priv->flag & ~0x0f) | 1;
							}
						}else
							priv->flag = (priv->flag & ~0x0f) | 4;
						f_offset += len;
					break;
				}
			break;
			case 4 : //text
				len = read_smtp_len(buf,f_offset,end,priv->flag);
				switch (len){
					case -2 :
						if (zero_space(buf,f_offset)){
							*offset = f_offset;
							return  2;
						}
					case 0 :
						ret = 0;
					break;
					case -1 :
						*offset = f_offset;
						return 1;
					break;
					case 1 :
						if (buf[f_offset]== 0x0d)
							f_offset++;
						f_offset++;
						priv->flag = (priv->flag & ~0x0f) | 1;
						*last_match = f_offset;
					break;
					default :
						f_offset += len;
					break;
				}
			break;
			case 2 :
				if ((end - 80)<f_offset){
					*offset = f_offset;
					return 1;
				}
				tmp_slot = slot;
				for (i=2;((i<100) && (buf[f_offset+i]!= 0x0a) && (buf[f_offset+i]!= 0x0d));i++);
				
				for (slot=0;slot<MAX_SMTP_CONTENT;slot++){
//printf("%d       %d\n",priv->slot[slot].crc, FLAC__crc16(buf+f_offset+2,priv->slot[slot].crc_len,0));
					if((priv->slot[slot].c_flag) &&  (priv->slot[slot].crc_len < i)
						&& (priv->slot[slot].crc == FLAC__crc16(buf+f_offset+2,priv->slot[slot].crc_len,0)))
							break;
				}
				if (slot == MAX_SMTP_CONTENT){
//					printf("SMTP can not find crc\n");
					priv->flag = tmp_flag;
					slot = tmp_slot;
					while (buf[f_offset] == '-')
						f_offset++;
					while ((buf[f_offset]== 0x0a) || (buf[f_offset]== 0x0d))
						f_offset++;
					break;
				}

				 if ((buf[f_offset+priv->slot[slot].crc_len+2]=='-')&&(buf[f_offset+priv->slot[slot].crc_len+3]=='-')){
					priv->slot[slot].crc = 0;
					priv->slot[slot].c_flag = 0;
					priv->slot[slot].crc_len = 0;
					priv->slot[slot].c_code = 0;
					priv->flag = (priv->flag & ~0x0f) | 1;
					
				}
				else {
					priv->flag = (priv->flag & ~0x0f) | 5;	
				}
				*last_match = f_offset;
				f_offset += i;
				while ((buf[f_offset]== 0x0a) || (buf[f_offset]== 0x0d))
					f_offset++;
			break;
			case 5:
				len = read_smtp_len(buf,f_offset,end,0x800);
				switch (len){
					case 0 :
						ret = 0;
					break;
					case -1 :
						*offset = f_offset;
						return 1;
					break;
					case -2 : 
						for(i=0;i<MAX_SMTP_CONTENT;i++)
							if (priv->slot[i].c_flag)
								return 0;
						*offset = f_offset;
						return 2;
					break;
					case 1 :
						while ((buf[f_offset]== 0x0d) || (buf[f_offset]== 0x0a))
							f_offset++;
						priv->len = 0;
						if (priv->slot[slot].c_flag & 0x02)
							priv->flag = (priv->flag & ~0x0f) | 6;
						else
							priv->flag = (priv->flag & ~0x0f) | 1;
					break;
					default:
						if(buf[f_offset + 8] == 'T'){
							if(!strncmp("Content-Transfer-Encoding:",(char*)buf+f_offset,26)){ 
								priv->slot[slot].c_flag = 3;
								priv->slot[slot].c_code = (read_smtp_codestr(buf,f_offset,len)<<8);
							}
							else 
								ret  = smtp_decode_content_type(buf, f_offset, len, priv);
						}
						if (ret) {
							*last_match = f_offset;
							f_offset += len;
						}
					break;
				}
				break;
			case 6 :
				if((priv->slot[slot].c_code == 0x400) && ((buf[f_offset] == '-') &&(buf[f_offset +1] == '-')))
					priv->slot[slot].c_flag |= 4;
				len = read_smtp_len(buf,f_offset,end,
					(((priv->slot[slot].c_code != 0x200) && ((priv->slot[slot].c_flag & 4) &&(buf[f_offset] == '-') &&(buf[f_offset +1] == '-'))) ? 0 : priv->slot[slot].c_code ));
				switch (len){
					case 0 :
					case -2 :
						ret = 0;
					break;
					case -1 :
						*offset = f_offset;
						return 1;
					break;
					case 1 :
						if (buf[f_offset]== 0x0d)
							f_offset++;
						f_offset++;
						priv->slot[slot].c_flag |= 4;
						if (priv->slot[slot].c_code & 0x400){
							tmp_flag = priv->flag;
							priv->flag = (priv->flag & ~0x0f) | 2;
						}
					break;	
					default :
						if (!priv->len)
							priv->len = len;
						
						if ((len > 7)&&(!strncmp("--", (char*)buf+f_offset, 2))){
							tmp_flag = priv->flag;
							priv->flag = (priv->flag & ~0x0f) | 2;
							break;
						}
						if(priv->slot[slot].c_flag & 4)
							priv->slot[slot].c_flag &= ~4;
						if ((priv->slot[slot].c_code & 0x400) && (priv->len != len) ){
							if(!((len < priv->len) && ((buf[f_offset +len] ==0x0d) 
								|| (buf[f_offset +len] ==0x0a)
								|| (buf[f_offset +len] == '-'))))
								return 0;
						}
						f_offset += len;
					break;
				}
			break;
		}//switch flag
	}//while
	*offset = f_offset;
	return ret;
}


//rfc822
int file_smtp(unsigned char *buf, int *size, __u32 scan , int flag, struct found_data_t* f_data){
	int 	ret = 0;
	__u32	offset;
	__u32	last_match = 0;
	int	b_count;
	struct priv_smtp_t *priv = NULL;

	switch (flag){
		case 0 :
			if (*size < current_fs->blocksize){
				if (buf[(*size)-1] == (unsigned char)0x0a) 
					ret = 1;
			}
			else {
				if (buf[(*size)-1] == (unsigned char)0x0a)
					ret = (f_data->scantype & DATA_METABLOCK) ? 2 : 4 ;
			}
			break;
		case 1 :
			return (scan & M_TXT);
			break;
		case 2 :
			offset = 0;
			priv = malloc(sizeof(struct priv_smtp_t));
			memset(priv,0,sizeof(struct priv_smtp_t));
			f_data->priv = priv;
			b_count = (f_data->buf_length > 12) ? 12 : f_data->buf_length ;
			ret = follow_smtp(buf, b_count, &offset, &last_match,f_data->priv);
			ret = analysis_ret1("SMTP", f_data, ret , b_count, offset, last_match);
	  	break;
		case 3 :
			offset = f_data->next_offset ;
			ret = follow_smtp(buf, f_data->buf_length, &offset, &last_match,f_data->priv);
			ret = analysis_ret2("SMTP", f_data, ret, offset, last_match);
		break;
		case 4 :
			if(f_data->priv)
				free (f_data->priv);
		break;
	}
	return ret;
}
	
	



//ps   switch only to pdf or txt
int file_ps(unsigned char *buf, int *size, __u32 scan , int flag, struct found_data_t* f_data){
	int 	ret = 1;
//	unsigned char *c;
//	unsigned char	token[9] = "PS-Adobe";

	switch (flag){
		case 0 :
			break;
		case 1 :
			break;
		case 2 :
			//c = buf+2 ;
			//if (! strncmp(c,token,7))
			//	f_data->func = file_pdf ;
			//else 
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
			return (scan & ((M_IS_META | M_CLASS_1)| M_BLANK | M_TXT)) ? 0 :1 ;
			break;
	}
	return ret;
}



//xz
int file_xz(unsigned char *buf, int *size, __u32 scan , int flag, struct found_data_t* f_data){
	int 	ret = 0;

	switch (flag){
		case 0 :
			if ((*size>3) && (buf[(*size)-1] == (unsigned char)0x5a) && 
				(buf[(*size)-2] == (unsigned char)0x59))
					ret = 1;
			break;
			
		case 1 :
			return ((scan & (M_IS_META | M_CLASS_1 | M_TXT | M_BLANK )&&(!(scan & M_DATA)))) ? 0 :1 ;
			break;
	}
	return ret;
}


static int follow_tar(unsigned char *buf, __u16 blockcount, __u32 *offset, __u32 *last_match,  int flag){
	int		i, j,ret = 1;
	__u64 		f_offset = (__u64) *offset;
	__u64		f_len = 0;
	__u8		*o_str;

	while ((ret ==1) && (f_offset < ((blockcount * current_fs->blocksize)-27))){
		if (((buf[f_offset + 0x101] == 0x75) && (buf[f_offset + 0x102] == 0x73) && (buf[f_offset + 0x103] == 0x74)&&
			(buf[f_offset + 0x104] == 0x61) && (buf[f_offset + 0x105] == 0x72))||(
			(!buf[f_offset + 0x101])&&(!buf[f_offset + 0x102])&&(!buf[f_offset + 0x103])&&(!buf[f_offset + 0x104])&&(!buf[f_offset + 0x105])&& buf[f_offset])){
			f_len = 0;
			*last_match = (__u32) f_offset +1;
			o_str = (__u8*)(buf + f_offset + 0x7c);
			for (i=0;i<10;i++){
				f_len += (*o_str & 0x7) ;
				f_len <<= 3;
				o_str++;
			}
			f_len += (*o_str & 0x7);
			f_len = ((f_len + 0x1ff) & ~0x1ff);
			f_offset += f_len + 0x200 ;
//			printf("TAR follow : offset %8lu \n", f_offset);
			continue;
		}
		else{
			for (j=0, i=f_offset; ((ret==1)&&(j<30)&&(i<(blockcount * current_fs->blocksize))); i++,j++){
				if (buf[i]){
//					printf("TAR break %lu \n",i);
					 ret=0;
				}
			}
			if (ret) {
//				printf("TAR complete %lu \n",f_offset);
				ret = 2;
			}
			break;
		}
	}
	*offset = f_offset & 0xFFFFFFFF;
	return ret;
}


//tar
int file_tar(unsigned char *buf, int *size, __u32 scan , int flag, struct found_data_t* f_data){
	int 	ret = 0;
	int	b_count;
	__u32	offset;
	__u32	last_match = 0;

	switch (flag){
		case 0 :
			if ((((__u64)f_data->inode->i_size |((__u64)f_data->inode->i_size_high<<32)) >= (__u64)f_data->size) &&  
				((*size) < (current_fs->blocksize - 0xff))){
				if (!(*size))
					*size = current_fs->blocksize;
				else
					*size = ((*size) + 1023) & ~1023 ;
	
				if (((!(f_data->inode->i_flags & EXT4_EXTENTS_FL)) &&(f_data->inode->i_block[12])) || (f_data->size < f_data->inode->i_size)
				 || (f_data->scantype & DATA_READY))  //FIXME 
					ret = 1;
			}
			break;
		case 1 :
			if (scan & M_TAR)
				ret = 1;
			 else 
				ret = ((scan & (M_IS_META | M_CLASS_1))||(f_data->scantype & DATA_READY)) ? 0 :1 ;
			break;
		case 2 :
			offset = 0;
			b_count = (f_data->buf_length > 12) ? 12 : f_data->buf_length ;
			ret = follow_tar(buf,b_count, &offset, &last_match,0);
			ret = analysis_ret1("TAR", f_data, ret , b_count, offset, last_match);
	  	break;
		case 3 :
			offset = f_data->next_offset ;
			ret = follow_tar(buf, f_data->buf_length, &offset, &last_match,0);
			ret = analysis_ret2("TAR", f_data, ret, offset, last_match);
		break;
	}
	return ret;
}	 


static int follow_elf(unsigned char *buf, __u16 blockcount, __u32 *offset, __u32 *table,  int flag){
	int		i;
	__u16		count = (flag >>3);
	int		ret = 1;
	__u32 		f_offset = *offset;
	int		size;
	__u32		end = blockcount * current_fs->blocksize;
	__u32		max;
	__u32		entry;
	
	if((!f_offset) && (flag & 0x1) && (!(*table))){ // HEAD
		if (flag & 0x4){ //64 bit
			if (flag & 0x2){ // BIG Endian
				f_offset = (buf[44]<<24)+(buf[45]<<16)+(buf[46]<<8)+(buf[47]);
				count = (buf[60]<<8)+(buf[61]);
			}
			else{ //Little Endian
				f_offset = (buf[40])+(buf[41]<<8)+(buf[42]<<16)+(buf[43]<<24);
				count = (buf[60])+(buf[61]<<8);
			}
		}
		else{ //x32 bit
			if (flag & 0x2){ // BIG Endian
				f_offset = (buf[32]<<24)+(buf[33]<<16)+(buf[34]<<8)+(buf[35]);
				count = (buf[48]<<8)+(buf[49]);
			}
			else{ //Little Endian
				f_offset = (buf[32])+(buf[33]<<8)+(buf[34]<<16)+(buf[35]<<24);
				count = (buf[48])+(buf[49]<<8);
			}
		}
		*offset = f_offset;
		*table = f_offset;
		return count;
	}
//----------------------
	if((flag & 0x1) && (*table)){  // Section Header 
		size = (flag & 0x4) ? 64 :40 ;
				
		if ((f_offset + (count *size))<= end){
			for (i=0;i<(size+4);i++)
				if (buf[f_offset +i])
					break;
			if((i<size) || (i == (size +4))){
//				printf("ELF : no section header found\n");
				ret = 0;
			}
	
			max = (*table) + (size * count);
			for ( i=0; i< count; i++){
				if (flag & 0x4){ //64 bit
					if (flag & 0x2){ // BIG Endian
						entry = (buf[f_offset+31])+(buf[f_offset+30]<<8)+(buf[f_offset+29]<<16)+(buf[f_offset+28]<<24);
						if (!((buf[f_offset+7]==0x08)&&(!(buf[f_offset+6]))&&(!(buf[f_offset+5]))&&(!(buf[f_offset+4]))))
							entry +=
							(buf[f_offset+39])+(buf[f_offset+38]<<8)+(buf[f_offset+37]<<16)+(buf[f_offset+36]<<24);
					}
					else{ //Little Endian
						entry = (buf[f_offset+24])+(buf[f_offset+25]<<8)+(buf[f_offset+26]<<16)+(buf[f_offset+27]<<24);
						if (!((buf[f_offset+4]==0x08)&&(!(buf[f_offset+5]))&&(!(buf[f_offset+6]))&&(!(buf[f_offset+7]))))
							entry +=
							(buf[f_offset+32])+(buf[f_offset+33]<<8)+(buf[f_offset+34]<<16)+(buf[f_offset+35]<<24);
					}
				}
				else{ //32 bit
					if (flag & 0x2){ // BIG Endian
						entry = (buf[f_offset+19])+(buf[f_offset+18]<<8)+(buf[f_offset+17]<<16)+(buf[f_offset+16]<<24);
						if (!((buf[f_offset+7]==0x08)&&(!(buf[f_offset+6]))&&(!(buf[f_offset+5]))&&(!(buf[f_offset+4]))))
							entry += (buf[f_offset+23])+(buf[f_offset+22]<<8)+(buf[f_offset+21]<<16)+(buf[f_offset+20]<<24);
					}
					else{ //Little Endian
						entry = (buf[f_offset+16])+(buf[f_offset+17]<<8)+(buf[f_offset+18]<<16)+(buf[f_offset+19]<<24);
						if (!((buf[f_offset+4]==0x08)&&(!(buf[f_offset+5]))&&(!(buf[f_offset+6]))&&(!(buf[f_offset+7]))))
							entry +=
							(buf[f_offset+20])+(buf[f_offset+21]<<8)+(buf[f_offset+22]<<16)+(buf[f_offset+23]<<24);
					}
				}
				
				f_offset += size;
				if (entry > max)
					max = entry;
			}
			*offset += (max - *table);
			ret = 3; 
		}
		if (*offset < end){
			f_offset = *offset;
			flag &= (~1);
		}
		else 
			return ret;
	}
//----------------------
	if(!(flag & 0x1)){
		if ((f_offset < end) && (zero_space(buf,f_offset)))
			ret = 2;
		else 
			ret = 0;
	}
	else {
		ret = 0;
	}

	return ret;
}





//binary
int file_binary(unsigned char *buf, int *size, __u32 scan , int flag, struct found_data_t* f_data){
	int 	ret = 0;
	__u32	offset = 0;
	__u32	tmp;
	__u32	table = 0;
	int	b_count = 0;

	switch (flag){
		case 0 :
			if (f_data->scantype & DATA_READY){
				*size = f_data->next_offset;
				ret =1;
			}
			else {
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
			}
			break;
		case 1 :
			return ((scan & (M_IS_META | M_CLASS_1))||(f_data->scantype & DATA_READY)) ? 0 :1 ;
			break;
		case 2 :
			offset = 0;
			if (!((buf[0] == 0x7f) &&  (buf[1] == 0x45) && (buf[2] == 0x4c) && (buf[3] == 0x46))){
				f_data->func = NULL;
				return 0;
			}
			f_data->scantype |= (buf[4] == 0x2) ? DATA_X64 :0 ;
			f_data->scantype |= (buf[5] == 0x2) ? DATA_BIG_END :0 ;
			f_data->scantype |= DATA_FLAG ;
			offset = 0;
			b_count = (f_data->buf_length > 12) ? 12 : f_data->buf_length ;
			ret = follow_elf(buf,b_count, &offset, &table,f_data->scantype >>13);
			if (!ret){
				f_data->func = NULL;
				return 0;
			}
			f_data->last_match_blk=1;
			f_data->h_size = ret;
			f_data->scantype |= DATA_CARVING;
			f_data->size = table;

			if (( offset + (((f_data->scantype & DATA_X64)?64:40) * ret)) >= (b_count * current_fs->blocksize)){
				if (offset > (b_count * current_fs->blocksize))
					f_data->buf_length = b_count;
				else
					f_data->buf_length = offset / current_fs->blocksize;
				f_data->next_offset = offset - (f_data->buf_length * current_fs->blocksize);
				return 1;
			}
			
		case 3 :
			if (!offset)
				offset = f_data->next_offset ;
			if (!table)
				table = f_data->size ;
			if (!b_count)
				b_count = f_data->buf_length;
			tmp = offset;
			ret = follow_elf(buf,b_count, &offset, &table,((f_data->h_size)<<3) + (f_data->scantype >>13));			
			if (ret){
				f_data->buf_length = offset / current_fs->blocksize;
				f_data->next_offset = offset - (f_data->buf_length * current_fs->blocksize);
			}
			switch (ret){
				case 0 :
//					fprintf(stderr,"ELF-CHECK: error, recover not \n");
					f_data->scantype |= DATA_BREAK;	
				break;
				case 1 :
//					fprintf(stderr,"ELF-CHECK: data next found\n");
				break;
				case 2 :
//					fprintf(stderr,"ELF-CHECK: data end found, recover \n");
					f_data->scantype |= DATA_READY;
					if(offset % current_fs->blocksize) (f_data->buf_length)++;
				break;
				case 3 :f_data->last_match_blk = f_data->last -f_data->first +1;
//					fprintf(stderr,"ELF-CHECK: file size found\n");
					f_data->scantype &= (~DATA_FLAG);
					f_data->size += (offset - tmp);
					f_data->h_size = 0;
					ret = 1;
				break;
					
			}
	}
	return ret;
}			 



//bin-raw
int file_bin_raw(unsigned char *buf, int *size, __u32 scan , int flag, struct found_data_t* f_data){
	int ret = 0;
	switch (flag){
		case 0 :
			if(f_data->scantype & DATA_METABLOCK){
				ret = 1;
				break;
			}
			else{
				if (*size < (current_fs->blocksize -8))
					ret = 2;

				if (*size < (current_fs->blocksize -16))
					ret = 1;
			}
			break;
		case 1 :
			return (scan & (M_IS_META | M_CLASS_1)) ? 0 :1 ;
			break;
	}
	return ret;
}	



int file_tfm(unsigned char *buf, int *size, __u32 scan , int flag, struct found_data_t* f_data){
	int 	i,ret = 0;
	__u16	*p_header;
	__u16	header[12];

	switch (flag){
		case 0 :
			if ((f_data->size) && (f_data->inode->i_size == f_data->size))
					return 1;
			if ((f_data->size) && (f_data->inode->i_size < f_data->size))
					return 0;
			else{
				if (*size <= f_data->size % current_fs->blocksize ){
					*size = f_data->size % current_fs->blocksize;
					ret = 1;
				}
			}
			break;
			
		case 1 :
			return (scan & (M_IS_META | M_CLASS_1 | M_BLANK | M_TXT)) ? 0 :1 ;
			break;
		case 2 :
			p_header = (__u16*) buf;
			for (i=0; i< 12; i++){
				header[i] = ext2fs_be16_to_cpu( *p_header);
				p_header++;
			}
			if (header[0] != (6+header[1]+(header[3]-header[2]+1)+header[4]+header[5]+header[6]+header[7]+header[8]+header[9]+header[10]+header[11]))
				f_data->func = NULL;
			else{
				f_data->size = header[0] << 2;
				f_data->scantype = DATA_LENGTH ;
				ret =1;
			}
	}
	return ret;
}




int file_qt(unsigned char*, int*, __u32, int, struct found_data_t*);

static int follow_jp2000(unsigned char *buf, __u16 blockcount, __u32 *offset, __u32 *last_match,  int flag){
	int	 ret = 1;
	int 	 end = blockcount * current_fs->blocksize;
	__u32	 f_offset = *offset;
	__u32	 size;

	while ((ret == 1)&&( f_offset < (end - 3))){
		if (buf[f_offset] == 0xff){
			switch (buf[f_offset+1]){
				case 0x4f : //SOC
					f_offset += 2;
					break;
				case 0xd9 : //EOC
					*last_match = f_offset;
					f_offset += 2;
					if (zero_space(buf,f_offset))
						ret = 2;
					else 
						ret = 0;
					break;
				case 0x90 : //SOT
					*last_match = f_offset;
					size = ((buf[f_offset+6])<<24)+((buf[f_offset+7])<<16)+((buf[f_offset+8])<<8) + buf[f_offset+9];
					f_offset += size;
					break;
				case 0x51 : //SIZ
				case 0x52 : //COD
				case 0x53 : //COC
				case 0x5e : //RGN
				case 0x5c : //QCD
				case 0x5d : //QCC
				case 0x5f : //POC
				case 0x55 : //TLM
				case 0x57 : //PLM
				case 0x58 : //PLT
				case 0x60 : //PPM
				case 0x61 : //PPT
				case 0x91 : //SOP
				case 0x63 : //CRG
				case 0x64 : //COM
				case 0x68 : //EPC
				case 0x66 : //EPB
				case 0x67 : //ESD
				case 0x69 : //RED
				case 0x65 : //SEC
				case 0x94 : //INSEC
						*last_match = f_offset;
						f_offset += 2;
						size = ((buf[f_offset])<<8) + buf[f_offset+1];
						if (size < 2){
							ret = 0;
							break;
						}
						f_offset += size;
					break;
				case 0x93 : //SOD
					f_offset += 2;
					ret = 0;
//					printf("ERROR JP2000 SOD-Marker\n");
					break;
				default :
//					printf("Error JP2000 Unknown marker %02x \n",buf[f_offset+1]);
					ret = 0;
					break;
			}
		}
		else{
			ret = 0;
		}
	}
	*offset = f_offset;
	return ret;
}	


//jp2
int file_jp2(unsigned char *buf, int *size, __u32 scan , int flag, struct found_data_t* f_data){
	int 	b_count,ret = 0;
	__u32 	offset;
	__u32	last_match = 0;
	

	switch (flag){
		case 0 :
			if ((*size>1) && (buf[(*size)-1] == (unsigned char)0xd9) && (buf[(*size)-2] == (unsigned char)0xff))
					ret = 1;
			else
				if ((*size == 1) && (buf[(*size)-1] == (unsigned char)0xd9))
					ret = 1;
			break;
			
		case 1 :
			return ((scan & (M_IS_META | M_CLASS_1))||(f_data->scantype & DATA_READY)) ? 0 :1 ;
			break;
		case 2 :
			offset = f_data->next_offset;  //take the offset from QuickTime
			b_count = (f_data->buf_length > 12) ? 12 : f_data->buf_length ;
			ret = follow_jp2000(buf, b_count, &offset, &last_match,0);
			ret = analysis_ret1("JP2000", f_data, ret , b_count, offset, last_match);
			break;
		case 3 :
			offset = f_data->next_offset ;
			ret = follow_jp2000(buf, f_data->buf_length, &offset, &last_match,0);
			ret = analysis_ret2("JP2000", f_data, ret , offset, last_match);
		break;
	}
	return ret;
}


//switch only to QT or jp2
int file_jp2000(unsigned char *buf, int *size, __u32 scan , int flag, struct found_data_t* f_data){
	if ((buf[0]==0xff) && (buf[1]==0x4f))
			f_data->func=file_jp2;
	else
			f_data->func=file_qt;
return f_data->func(buf,size,scan,flag,f_data);
}


static int follow_jpeg(unsigned char *buf, __u16 blockcount, __u32 *offset, __u32 *last_match,  int* flag){
	int	 ret = 1;
	int 	 end = blockcount * current_fs->blocksize;
	__u32	 f_offset = *offset;
	__u16	 size;

	while ((ret == 1)&&( f_offset < (end - 3))){
		if ((!(*flag))&&(buf[f_offset] == 0xff)){
			switch (buf[f_offset+1]){
				case 0xd8 : //Start Of Image (beginning of datastream)
					f_offset += 2;
					break;
				case 0xd9 : //End Of Image (end of datastream) 
					*last_match = f_offset;
					f_offset += 2;
					if (zero_space(buf,f_offset))
						ret = 2;
					else 
						ret = 0;
					break;
				case 0xc0 : //Baseline
				case 0xc1 : //Extended sequential
				case 0xc2 : //Progressive
				case 0xc3 : //Lossless
				case 0xc5 : //Differential sequential
				case 0xc6 : //Differential progressive
				case 0xc7 : //Differential lossless
				case 0xc9 : //Extended sequential, arithmetic coding
				case 0xca : //Progressive, arithmetic coding
				case 0xcb : //Lossless, arithmetic coding
				case 0xcd : //Differential sequential, arithmetic coding
				case 0xce : //Differential progressive, arithmetic coding
				case 0xcf : //Differential lossless, arithmetic coding

				case 0xc4 : //Huffman-table
				case 0xc8 : //reserv JPEG extension
				case 0xcc : //arithmetic Code

				case 0xdb : //DQT
				case 0xdd :
				//Application-specific marker; all 16 APPn's
				case 0xe0 :  //JFIF
				case 0xe1 :  //EXIF
				case 0xe2 :
				case 0xe3 :
				case 0xe4 :
				case 0xe5 :
				case 0xe6 :
				case 0xe7 :
				case 0xe8 :
				case 0xe9 :
				case 0xea :
				case 0xeb :
				case 0xec :
				case 0xed :
				case 0xee : //often Copyright
				case 0xef :
				case 0xfe : //COMment
						*last_match = f_offset;
						f_offset += 2;
						size = ((buf[f_offset])<<8) + buf[f_offset+1];
						if (size < 2){
							ret = 0;
							break;
						}
						f_offset += size;
					break;
				case 0xda : //Start Of Scan (begins compressed data) 
					*last_match = f_offset;
					f_offset += 2;
					*flag = 1;
					break;
				default :
//					printf("Error JPEG %02x \n",buf[f_offset+1]);
					ret = 0;
					break;
			}
		}
		else{
			if (*flag){
loop:
				while ((f_offset < (end-2)) && ((buf[f_offset]) != 0xff))	
					f_offset++;
				if (((buf[f_offset])==0xff) && ((!buf[f_offset+1]) || ((buf[f_offset+1] >= 0xd0)&&(buf[f_offset+1] <= 0xd7)))){
					f_offset++;
					goto loop;
				}
				else {	
					if ((buf[f_offset]) == 0xff){
						if (buf[f_offset+1] == 0xd8){
							ret = 0; // this is begin of a new jpeg
							f_offset--;
						}
						else 
							*flag = 0;
					}
				}
			}
			else{
				ret = 0;
			}
		}
	}
	*offset = f_offset;
	return ret;
}	
	




//jpeg
int file_jpeg(unsigned char *buf, int *size, __u32 scan , int flag, struct found_data_t* f_data){
	int 	b_count,ret = 0;
	__u32 	offset;
	__u32	last_match = 0;
	int	*priv_flag = NULL;
	

	switch (flag){
		case 0 :
			if ((*size>1) && (buf[(*size)-1] == (unsigned char)0xd9) && (buf[(*size)-2] == (unsigned char)0xff))
					ret = 1;
			else
				if ((*size == 1) && (buf[(*size)-1] == (unsigned char)0xd9))
					ret = 1;
			break;
			
		case 1 :
			return ((scan & (M_IS_META | M_CLASS_1))||(f_data->scantype & DATA_READY)) ? 0 :1 ;
			break;
		case 2 :
			offset = 0;
			priv_flag = malloc(4);
			*priv_flag = 0;
			f_data->priv = priv_flag;
			b_count = (f_data->buf_length > 12) ? 12 : f_data->buf_length ;
			ret = follow_jpeg(buf, b_count, &offset, &last_match,(int*)f_data->priv);
			ret = analysis_ret1("JPEG", f_data, ret , b_count, offset, last_match);
			break;
		case 3 :
			offset = f_data->next_offset ;
			ret = follow_jpeg(buf, f_data->buf_length, &offset, &last_match,(int*)f_data->priv);
			ret = analysis_ret2("JPEG", f_data, ret, offset, last_match);
		break;
		case 4 :
			if (f_data->priv)
				free(f_data->priv);
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
			return ((scan & (M_IS_META | M_CLASS_1 | M_TXT | M_BLANK )&&(!(scan & M_DATA)))) ? 0 :1 ;
			break;
		case 2 :
			if(memcmp(buf,header,8)) 
				f_data->func = NULL;
			else 
				ret = 1 ;
			break;
	}
	return ret;
}




#define GIF_SUB_BLOCK	0x40000000
#define GIF_IMAGE_DATA  0x80000000
#define GIF_DATA        0xC0000000
static int follow_gif(unsigned char *buf, __u16 blockcount, __u32 *offset, __u32 *last_match,  int flag, __u32 *gif_data){
	int 		ret = 1;
	__u32 		i  = *offset ;
	__u32		data_flag = *gif_data; 

	//File begin
	if ((!flag) && (!i)){ //Header
		if ((buf[i] == 0x47) && (buf[i+1] == 0x49) && (buf[i+2] ==0x46 ) &&
		   (buf[i+3] & 0x38) && (buf[i+4] & 0x30) && (buf[i+5] > 0x60) && (buf[i+5] < 0x7b)){
			i+=6;
		   //Logical Screen Descriptor
			if(buf[i+4] & 0x80)
		   //Global Color Table 
				i += ((0x02 << (buf[i+4] & 0x07))*3) + 7 ;
			else 
				i += 7 ;	
		}	
		else ret = 0;
	}
	while ((ret == 1)&&( i < ((blockcount * current_fs->blocksize)-12))){
		switch (buf[i]){
			case  0x00 :
						//Block Terminator
				if (data_flag & GIF_DATA){
					i++;
					data_flag = 0;
				}else
					ret = 0;
				break;
			case  0x21 :		//Extension Introducer
				switch (buf[i+1]){
					case 0x01 :
						//Plain Text Label
						*last_match = i;
						i += (16 + buf[i+15]);
						data_flag = GIF_SUB_BLOCK ;
						break;
					case 0xf9 :
						//Graphic Control Extension
						*last_match = i;
						i += 7;
						data_flag = GIF_SUB_BLOCK ;
						break;
					case 0xfe :
						//Comment Label
						*last_match = i;
						i += (3 + buf[i+2]);
						data_flag = GIF_SUB_BLOCK ;
						break;
					case 0xff :
						//Application Extension Label
						*last_match = i;
						i += (15 + buf[i+14]);
						data_flag = GIF_SUB_BLOCK ;
						break;
					default:
						ret = 0;
					break;
			}
			break;
			case 0x3B  :		//Trailer
				i++;
				ret = 2;
				break;
			case 0x2C  :		//Image Separator
				if(buf[i+9] & 0x80)
		   				//Local Color Table 
					i += ((0x02 << (buf[i+9] & 0x07))*3) + 10 ;
				else 
					i += 10 ;
				//---------------------------------------
						//Table Based Image Data.
				i++; 		//LZW Minimum Code Size 
				data_flag = GIF_IMAGE_DATA;
forward:
				while ((i < ((blockcount * current_fs->blocksize)-12))&& (buf[i])){
					i += (buf[i]+1);
				}
			break;
			default :
				if (data_flag & GIF_IMAGE_DATA){
					goto forward;
				}else
					ret = 0;
			break;
		}//switch

	}//while
	*offset = i ;
	*gif_data = data_flag ;
return ret;
}



//gif
int file_gif(unsigned char *buf, int *size, __u32 scan , int flag, struct found_data_t* f_data){
	int 	b_count,ret = 0;
	__u32	last_match = 0;
	__u32	offset;
	__u32	gif_data = 0;

	switch (flag){
		case 0 :
			if ((f_data->size) && (f_data->size <= f_data->inode->i_size)){
				if(f_data->scantype & DATA_READY){
					*size = (f_data->next_offset)?f_data->next_offset : *size;
					ret = 1 ;
					break;
				}
			}
			if ((*size) && (*size < current_fs->blocksize) && (buf[(*size)-1] == 0x3b))
				ret = 1;
			break;
		case 1 :
			return ((scan & (M_IS_META | M_CLASS_1))||(f_data->scantype & DATA_READY)) ? 0 :1 ;
			break;
		case 2 :
			offset = 0;
			b_count = (f_data->buf_length > 12) ? 12 : f_data->buf_length ;
			ret = follow_gif(buf, b_count, &offset, &last_match,0,&gif_data);
			if (last_match)
				f_data->last_match_blk = (last_match/current_fs->blocksize) +1 ;
			if (ret){
				f_data->scantype = DATA_CARVING;
				if(gif_data) 
					f_data->scantype |= gif_data;
				f_data->size = offset;
				if (offset > (b_count * current_fs->blocksize))
					f_data->buf_length = b_count;
				else
					f_data->buf_length = offset / current_fs->blocksize;
				f_data->next_offset = offset - (f_data->buf_length * current_fs->blocksize);
			}
			switch (ret){
				case 0 :
//					fprintf(stderr,"GIF-CHECK: Block %lu : Break \n",f_data->first);
					f_data->scantype |= DATA_BREAK;
					f_data->func = NULL;
					return 0;
				break;
				case 1:

				break;
				case 2 :
					f_data->scantype |= DATA_READY;
					if(offset % current_fs->blocksize)(f_data->buf_length)++;
				break;
			}
	
			break;
		case 3 :
			offset = f_data->next_offset ;
			gif_data =  f_data->scantype & GIF_DATA ;
			ret = follow_gif(buf, f_data->buf_length, &offset, &last_match,1,&gif_data);
	
			if (last_match)
				f_data->last_match_blk = ((f_data->size - f_data->next_offset + last_match)/current_fs->blocksize) +1 ;
			if (ret){
				if(gif_data) 
					f_data->scantype |= gif_data;
				else 
					f_data->scantype &= 0x3FFFFFFF ;
				f_data->size += offset - f_data->next_offset;
				f_data->buf_length = offset / current_fs->blocksize;
				f_data->next_offset = offset - (f_data->buf_length * current_fs->blocksize);
			}
			switch (ret){
				case 0 :
//					fprintf(stderr,"GIF-CHECK: data error, recover not \n");
					f_data->scantype |= DATA_BREAK;	
				break;
				case 1 :
//					fprintf(stderr,"GIF-CHECK: data next found\n");
				break;
				case 2 :
//					fprintf(stderr,"GIF-CHECK: data end found, recover \n");
					f_data->scantype |= DATA_READY;
					if(offset % current_fs->blocksize) (f_data->buf_length)++;
				break;
			}
	
		break;
	}
	return ret;
}
	



//bmp
int file_bmp(unsigned char *buf, int *size, __u32 scan , int flag, struct found_data_t* f_data){
	__u32		wi,hi,offset,ssize;
	int 		ret = 0;
	__u16	    	colors;

/*	struct bmp_header {
	__u16  	bfType;
	__u32  	bfSize;
	__u16  	bfReserved1;
	__u16  	bfReserved2;
	__u32  	bfOffBits;
	} *p_bmp_header;
	
	struct bmp_info {
	__u32  	biSize;
	__u32  	biWidth;
	__u32  	biHeight;
	__u16  	biPlanes;
	__u16  	biBitCount;
	__u32  	biCompression;
	__u32  	biSizeImage;
	__u32  	biXPelsPerMeter;
	__u32  	biYPelsPerMeter;
	__u32  	biClrUsed;
	__u32  	biClrImportant;
	} *p_bmp_info;
	
	struct bmp_info_s {
	__u32  	biSize;
	__u16  	biWidth;
	__u16  	biHeight;
	__u16  	biPlanes;
	__u16  	biBitCount;
	} *p_bmp_info_s;
*/

	switch (flag){
		case 0 : 
			if (f_data->size && (f_data->size <= f_data->inode->i_size)) {
				ssize = ((f_data->size-1) % current_fs->blocksize) + 1;
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
			offset = ((__u32)*(buf+10)) | ((__u32)*(buf+11))<<8 | ((__u32)*(buf+12))<<16 | ((__u32)*(buf+13))<<24 ;
			
			if ( *(buf+14) == 0x0c){
				wi = ((__u32)*(buf+18)) | ((__u32)*(buf+19))<<8 ;
				colors = ((__u16)*(buf+24)) | ((__u16)*(buf+25))<<8 ;
				switch (colors){
					case 1:
						wi = (((((wi+0x7) & ~0x7) / 8) + 1) & ~1);
						break;
					case 4: 
						wi = (((wi+0x7) & ~0x07) / 2);
						break;
					case 8:
						wi = ((wi +0x03) & ~0x03);
						break;
					case 16:
						wi *=2;
						break;
					case 24:	
						wi *=3;
						break;
					case 32:
						wi *=4;
						break;
				}
				hi = ((__u32)*(buf+20))  | ((__u32)(*(buf+21) & 0x7f))<<8 ;
				ssize = (wi * hi) + offset ;
			}
			else{
				ssize = ((__u32)*(buf+34)) | ((__u32)*(buf+35))<<8 | ((__u32)*(buf+36))<<16 | ((__u32)*(buf+37))<<24 ;
				if(ssize){
					ssize += offset;
				}
				else{
					wi = ((__u32)*(buf+18)) | ((__u32)*(buf+19))<<8 | ((__u32)*(buf+20))<<16 | ((__u32)*(buf+21))<<24 ;
					colors = ((__u16)*(buf+28)) | ((__u16)*(buf+29))<<8 ;
					switch (colors){
					case 1:
						wi = (((((wi+0x7) & ~0x7) / 8) + 1) & ~1);
						break;
					case 4: 
						wi = (((wi+0x7) & ~0x07) / 2);
						break;
					case 8:
						wi = ((wi +0x03) & ~0x03);
						break;
					case 16:
						wi *=2;
						break;
					case 24:	
						wi *=3;
						break;
					case 32:
						wi *=4;
						break;
					}
					hi = ((__u32)*(buf+22)) | ((__u32)*(buf+23))<<8 | ((__u32)*(buf+24))<<16 | ((__u32)(*(buf+25) & 0x7f))<<24 ;
					ssize = (wi * hi) + offset ;
				}
			}
			f_data->size = ssize;
			f_data->scantype = DATA_LENGTH ;
			ret =1;
		break;	
	}
	return ret;
}


static int follow_png(unsigned char *buf, __u16 blockcount, __u32 *offset, __u32 *last_match,  int flag){
	char	searchstr[] = "IDAT IEND MHDR MEND LOOP IHDR JHDR JDAT JDAA JSEP TERM BACK SAVE SEEK sBIT gAMA cHRM PLTE hIST bKGD tRNS pHYS oFFs pCAL sCAL tIME tEXt zTXt gIFg gIFx eXPI pHYg iTXt iCCP sRGB pHYs DEFI ENDL FILT MAGN BASI sPLT fRAc vpAg dSIG gIFt sTER tXMP ";
	char		chunk[5];
	char		endchunk[]="IEND";
	int 		ret = 1;
	__u32 		i  = *offset;
	int		j;
	
	if (flag) endchunk[0]=0x4d;
	while ( i < (blockcount * current_fs->blocksize)-12){
		for (j=i+4;j<(i+8);j++){
			if (((buf[j] & 0xdf) < 65) || ((buf[j] & 0xdf) > 90)){
				ret = 0 ;
				break;
			}
		}
		if (buf[i] & 0x8){
			ret = 0 ;
			break;
		}
		memcpy(chunk, buf+i+4, 4);
		chunk[4] = 0;
		if(!strstr(searchstr,chunk)){
			ret = 0;
			break;
		}	
		if (strstr(chunk,endchunk)){ 
			i+=12;
			ret = 2;
			break;
		}
		*last_match = i;
		i += (buf[i]<<24)+(buf[i+1]<<16)+(buf[i+2]<<8)+buf[i+3] + 12;
	}	
	*offset = i;
	return ret;
}

//png
int file_png(unsigned char *buf, int *size, __u32 scan , int flag, struct found_data_t* f_data){
	int 	b_count,ret = 0;
	__u32	last_match = 0;
	__u32	offset;

	switch (flag){
		case 0 :
			if ((f_data->size) && (f_data->size <= f_data->inode->i_size)){
				if(f_data->scantype & DATA_READY){
					*size = (f_data->next_offset)?f_data->next_offset : *size;
					ret = 1;
					break;
				}
/*				if((!(f_data->inode->i_flags & EXT4_EXTENTS_FL))&&(f_data->size < (12 * current_fs->blocksize))){
					f_data->inode->i_size = (f_data->size + current_fs->blocksize -1) & ~(current_fs->blocksize-1);
					*size = f_data->size % current_fs->blocksize;
				}*/
				else{
//				if((!(f_data->inode->i_flags & EXT4_EXTENTS_FL))){
					if (*size > 8){
						if (strstr((char*)buf + (*size) -8,"END"))	
							ret=1;
					}
					else{
						if (*size >= 5){
							if (strtok((char*)buf,"D" ))
							ret=1;
						}
						else
							if (*size < 5)
								ret = 2; 
					}
				}
			}	
			break;
		case 1 : 
			return ((scan & (M_IS_META | M_CLASS_1))||(f_data->scantype & DATA_READY)) ? 0 :1 ;
			break;

		case 2 :
			offset = 8;
			b_count = (f_data->buf_length > 12) ? 12 : f_data->buf_length ;
			ret = follow_png(buf, b_count, &offset, &last_match,(f_data->type == 0x070e)?1:0);
			ret = analysis_ret1("PNG", f_data, ret , b_count, offset, last_match);
			break;
		case 3 :
			offset = f_data->next_offset ;
			ret = follow_png(buf, f_data->buf_length, &offset, &last_match,(f_data->type == 0x070e)?1:0);
			ret = analysis_ret2("PNG", f_data, ret, offset, last_match);
		break;
	}
	return ret;
}


//mng
/*int file_mng(unsigned char *buf, int *size, __u32 scan , int flag, struct found_data_t* f_data){
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
}*/


//iff
int file_iff(unsigned char *buf, int *size, __u32 scan , int flag, struct found_data_t* f_data){
	int 		i,ret = 0;
	__u32		ssize;
	char 		token[5];	
	char type[] = "AIFF AIFC 8SVX 16SV SAMP MAUD SMUS CMUS ILBM RGBN RGB8 DEEP DR2D TDDD LWOB LWO2 LWLO REAL MC4D ANIM YAFA SSA ACBM FAXX FTXT CTLG PREF DTYP PTCH AMFF WZRD DOC ";

	switch (flag){
		case 0 : 
			if ((f_data->size) && (f_data->inode->i_size >= f_data->size)) {
				ssize = ((f_data->size-1) % current_fs->blocksize) +1;
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
				f_data->scantype = DATA_LENGTH ;
				ret =1;
			}
		}
		break;
	}
return ret;
}

//DjVu
int file_djvu(unsigned char *buf, int *size, __u32 scan , int flag, struct found_data_t* f_data){
	int 		i,ret = 0;
	__u32		ssize;
	char 		token[5];	
	char type[] = "DJVU DJVM DJVI THUM ";

	switch (flag){
		case 0 : 
			if ((f_data->size) && (f_data->inode->i_size >= f_data->size)) {
				ssize = ((f_data->size-1) % current_fs->blocksize) +1;
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
		if ((buf[0]==0x41) && (buf[1]==0x54) && (buf[2] ==0x26) && (buf[3]==0x54)
			&& (buf[4]==0x46) && (buf[5]==0x4f) && (buf[6] ==0x52) && (buf[7]==0x4d)){
			for (i=0;i<4;i++)
				token[i]=buf[i+12];
			token[i] = 0;
			if(strstr(type,token)){
				f_data->size = (buf[8]<<24) + (buf[9]<<16) + (buf[10]<<8) + buf[11] + 12;
				f_data->scantype = DATA_LENGTH ;
				ret =1;
			}
		}
		break;
	}
return ret;
}


struct pcx_priv_t {
	__u16 	width;
	__u16	height;
	__u16	planes;
	__u16	w;
	__u16	h;
	__u16	p;
	__u16	RLC;
};

static int follow_pcx(unsigned char *buf, __u16 blockcount, __u32 *offset, __u32 *last_match,  void *priv){
	int			flag = 1;
	int 			ret = 1;
	__u32			f_offset = *offset;
	__u32			end = blockcount * current_fs->blocksize;
	__u16	w,h,p;
	struct pcx_priv_t 	*p_data = (struct pcx_priv_t*) priv;
	w = h = p = 0;

	while ( (ret == 1) && (f_offset < (end-1))){
		for (p = ((flag) ? p_data->p:0) ; p < p_data->planes ; p++){
			for (h = ((flag) ? p_data->h :0) ; h < p_data->height; h++){
				for (w = ((flag) ? p_data->w :0) ; w < p_data->width; w++){
					if ((p_data->RLC) && ((buf[f_offset] & 0xc0) == 0xc0)){
						w += ((buf[f_offset] & 0x3f)-1);
						f_offset+=2;
					}
						else
							f_offset++;
					if (f_offset >=(end-1)){
						goto out;
					}
				}
				if (flag) flag = 0;
				if (w > p_data->width){
					ret = 0;
					goto out;
				}
//				printf("-----< %d    %d   %d   %d  %02x \n",p,h,w,f_offset,buf[f_offset]); 
			}
		}
		if (!zero_space(buf,f_offset)){
			if (buf[f_offset] == 0x0c){
				f_offset += 769;
				ret =2;
			}
			else 
				ret = 0;
		}
		else ret =2;
	}
out:
//	printf("----> %d    %d   %d   %d  %02x \n",p,h,w,f_offset,buf[f_offset]); 
	p_data->w = w+1;
	p_data->h = h;
	p_data->p = p;
	*offset = f_offset;
return ret;
}





//pcx
int file_pcx(unsigned char *buf, int *size, __u32 scan , int flag, struct found_data_t* f_data){
	int 			i,ret = 0;
	struct pcx_priv_t  	*priv;
	__u32			offset;
	__u32			last_match = 0;
	__u32			b_count;

struct pcx_header {
  __u8  	Manufacturer; /* should always be 0Ah                */
  __u8  	Version; 
  __u8  	Encoding;    /* 0: unsed, 1: RLE compressed */
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
			if(f_data->scantype & DATA_READY){
				*size = (f_data->next_offset)?f_data->next_offset : *size;
				ret = 1 ;
			}
			else {
				if (*size < (current_fs->blocksize -3))
					ret = 4;
				if (*size < (current_fs->blocksize - 32))
					ret = 2;
				if (*size < (current_fs->blocksize - 64))
					ret = 1;
			}
			break;
		case 1 :return ((scan & (M_IS_META | M_CLASS_1 | M_BLANK ))||(f_data->scantype & DATA_READY)) ? 0 :1 ;
			break;
		case 2 :
			pcx = (struct pcx_header*) buf;
			for (i=0;i<54;i++)
				if (pcx->Filler[i])
					break;
			if ((pcx->Version<=5 && pcx->Version!=1) && pcx->Encoding <= 1 && 
				(pcx->BitsPerPixel==1 || pcx->BitsPerPixel==4 || pcx->BitsPerPixel==8 || pcx->BitsPerPixel==24) &&
				(pcx->Reserved==0) && (i == 54) && (f_data->priv = malloc(sizeof(struct pcx_priv_t)))){
	
					f_data->priv_len = sizeof(struct pcx_priv_t);
					memset(f_data->priv,0,sizeof(struct pcx_priv_t));
					priv = f_data->priv ;
					f_data->last_match_blk =1;
			}
			else {
				f_data->first = 0;
				return 0;
			}

			if (!((pcx->Encoding)&& (pcx->ColorPlanes < 4))){// && (pcx->BitsPerPixel > 4))){
				return 1;
			}
			priv->RLC = pcx->Encoding;
			priv->width = pcx->BytesPerLine;
			priv->height = pcx->YMax -pcx->YMin + 1;
			priv->planes = pcx->ColorPlanes;
			offset = 128;

			b_count = (f_data->buf_length > 12) ? 12 : f_data->buf_length ;
			ret = follow_pcx(buf,b_count, &offset, &last_match,f_data->priv);
			ret = analysis_ret1("PCX", f_data, ret , b_count, offset, last_match);
	  	break;
		case 3 :
			offset = f_data->next_offset ;
			ret = follow_pcx(buf, f_data->buf_length, &offset, &last_match,f_data->priv);
			ret = analysis_ret2("PCX", f_data, ret, offset, last_match);
		break;
		case 4:
			if (f_data->priv)
				free(f_data->priv);
		break;
	}
return ret;
}


struct xcf_priv_t {
	int 		flag ;
	__u32		width ;
	__u32		height ;
	int		byte_p_pix ;
	int		s_count ;
	int		compress;
	__u32		b_stream ;
	__u32		l_offset ;
};

static int follow_xcf(unsigned char *buf, __u16 blockcount, __u32 *offset, __u32 *last_match,  void *priv){
	int			i;
	int 			ret = 1;
	__u32			f_offset = *offset;
	__u32			*p1, *p2;
	__u32			type, pl, pointer;
	__u32			end = (blockcount * current_fs->blocksize)-30;
	struct xcf_priv_t 	*p_data = (struct xcf_priv_t*) priv;

	while ( (ret == 1) && (f_offset < (end-1))){
		switch (p_data->flag){
			case 0://Master image structure
				if ((!f_offset) &&(! p_data->b_stream)){
					f_offset += 26;
					p1 = (__u32*) (buf+14);
					p_data->width = ext2fs_be32_to_cpu(*p1);
					p1++;
					p_data->height = ext2fs_be32_to_cpu(*p1);
				}
				do{ //Image properties
					p1 = (__u32*) (buf+f_offset);
					p2 = p1 +1;
					type = ext2fs_be32_to_cpu(*p1);
					pl = ext2fs_be32_to_cpu(*p2);
					if ((type == 17) && (pl == 1))
						p_data->compress = buf[f_offset+8];
					if (type)
						f_offset += (pl + 8);
				}while(type && (type <28) && (f_offset < end));
				if((!pl) && (!type)) {
					f_offset += 8;
					p_data->flag = 1;
				}
				if (type > 27)
					ret = 0;
			break;
			case 1 : //Pointer to the layer structure 
				do{
					p1 = (__u32*) (buf+f_offset);
					pointer = ext2fs_be32_to_cpu(*p1);
					if (p_data->l_offset < pointer)
						p_data->l_offset = pointer;
					if((p_data->s_count)++ > 256)
						ret = 0;
					f_offset += 4;
				}while (ret && pointer && (f_offset < end));
				if(ret && (f_offset < end)){	
					p_data->flag = 2; 
					p_data->s_count = 0;
				}
			break;
			case 2 : //Pointer to the channel structure 
				do{
					p1 = (__u32*) (buf+f_offset);
					pointer = ext2fs_be32_to_cpu(*p1);
					if (p_data->l_offset < pointer){
						p_data->l_offset = pointer;
						p_data->flag = 5; //FIXME
					}
					if((p_data->s_count)++ > 64)
						ret = 0;
					f_offset += 4;
				}while (ret && pointer && (f_offset < end));
				if(ret && (f_offset < end)){	
					(p_data->flag)++;   //FIXME
					f_offset = (p_data->l_offset - (p_data->b_stream - *offset));
				} 
			break;
			case 3 : //Layer structure
				p1 = (__u32*) (buf+f_offset);
				p_data->width = ext2fs_be32_to_cpu(*p1);
				p1++;
				p_data->height = ext2fs_be32_to_cpu(*p1);
				p1++;
				switch (ext2fs_be32_to_cpu(*p1)){
					case 0 : p_data->byte_p_pix = 3 ; break;
					case 1 : p_data->byte_p_pix = 4 ; break;
					case 2 :
					case 4 : p_data->byte_p_pix = 1 ; break;
					case 3 :
					case 5 : p_data->byte_p_pix = 2 ; break;
					default : p_data->byte_p_pix = 0;
				}
				if (p_data->width && p_data->height && p_data->byte_p_pix){
					p1++; //Layer name
					pl = ext2fs_be32_to_cpu(*p1);
					if ((f_offset + 20 + pl) <  end){
						f_offset += 16;
						if (pl){
							for  (i=0; i < (pl-1); i++){
								if (! isprint(buf[f_offset+i])){
									if (! is_unicode(buf+f_offset+i))
										ret = 0;
									break;
								}
							}
						}
						if (ret && (!buf[f_offset + pl -1])){
							*last_match = f_offset;
							f_offset += pl;
							p_data->flag = 4;
						}
					}
					else {
						end = f_offset;
						break;
					}
				}
				if (p_data->flag != 4)
					ret = 0;
			break;
			case 7 : //Channel or Mask properties 
			case 4 : //Layer properties
				do{
					p1 = (__u32*) (buf+f_offset);
					p2 = p1 +1;
					type = ext2fs_be32_to_cpu(*p1);
					pl = ext2fs_be32_to_cpu(*p2);
					if (type)
						f_offset += (pl + 8);
				}while(type && (type <28) && (f_offset < end));
				if((!pl) && (!type)) {
					f_offset += 8;
					(p_data->flag)++;
				}
				if (type > 27)
					ret = 0;
			break;
			case 5 : //Pointer to the hierarchy structure 
				p1 = (__u32*) (buf+f_offset);
				pointer = ext2fs_be32_to_cpu(*p1);
				if (p_data->l_offset < pointer){
					p_data->l_offset = pointer;
					p_data->flag = 9;
				}
				p1++; //Pointer to the layer mask (a channel structure)
				pointer = ext2fs_be32_to_cpu(*p1);
				if (pointer &&(p_data->l_offset < pointer)){
					p_data->l_offset = pointer;
					p_data->flag = 6;
				}
				f_offset += 8;
				if (p_data->flag == 5)
					ret = 0;
				else {
					*last_match = f_offset;
					if (p_data->l_offset < ((p_data->b_stream - *offset) + f_offset)){
						ret = 3;
						break;
					}
					else 
						f_offset = (p_data->l_offset - (p_data->b_stream - *offset));
				}
			break;
			case 6 : //Channel structure (layer masks or from the master image structure)
				p1 = (__u32*) (buf+f_offset);
				p2 = p1 +1;
				if ((p_data->width == ext2fs_be32_to_cpu(*p1)) && (p_data->height == ext2fs_be32_to_cpu(*p2))){
					p2++; //Channel or Mask name
					pl = ext2fs_be32_to_cpu(*p2);
					if ((f_offset + 20 + pl) <  end){
						f_offset += 12;
						if (pl){
							for  (i=0; i < (pl-1); i++){
								if (! isprint(buf[f_offset +i])){
									if (! is_unicode(buf+f_offset+i))
										ret = 0;
									break;
								}
							}
						}
						if (ret && (!buf[f_offset + pl -1])){
							*last_match = f_offset;
							f_offset += pl;
							p_data->flag = 7;
						}
					}
					else {
						end = f_offset;
						break;
					}
				}
				if (p_data->flag == 6)
					ret = 0;
				else {
					*last_match = f_offset;
				}
			break;
			case 8 : //Pointer to the hierarchy structure
				p1 = (__u32*) (buf+f_offset);
				pointer = ext2fs_be32_to_cpu(*p1);
				if (p_data->l_offset < pointer){
					p_data->l_offset = pointer;
					p_data->flag = 9;
				}
				f_offset += 4;
				if (p_data->flag == 8)
					ret = 0;
				else {
					*last_match = f_offset;
					if (p_data->l_offset < ((p_data->b_stream - *offset) + f_offset)){
						ret = 3;
						break;
					}
					else 
						f_offset = (p_data->l_offset - (p_data->b_stream - *offset));
				}
			break;
			case 9 : //hierarchy structure
				p1 = (__u32*) (buf+f_offset);
				p2 = p1 +1;
				if ((p_data->width == ext2fs_be32_to_cpu(*p1)) && (p_data->height == ext2fs_be32_to_cpu(*p2))){
					p2 += 3;
					p_data->l_offset = ext2fs_be32_to_cpu(*p2);
					if ((p_data->l_offset)&& (p_data->l_offset > ((p_data->b_stream - *offset) + f_offset))){
						*last_match = f_offset;
						f_offset = (p_data->l_offset - (p_data->b_stream - *offset));
						p_data->flag = 10;
					}
					else{ 
						ret = 3;
						break;
					}
				}
				if (p_data->flag == 9)
					ret = 0;
			break;
			case 10 ://Pointer to unused level structure 
				if (f_offset < (end -120)){
				do {
					p1 = (__u32*) (buf+f_offset);
					p2 = p1 +1;
					p_data->width >>= 1;
					p_data->height >>= 1;
					if (!((p_data->width == ext2fs_be32_to_cpu(*p1)) && (p_data->height == ext2fs_be32_to_cpu(*p2)))){
						ret = 0;
					}
					else {
						f_offset += 12;		
					}
				}while ((p_data->width > 64) || (p_data->height > 64));
				if (ret)
					ret = 2;
				}
				else
					end = f_offset;
			break;
		}
	}
	p_data->b_stream += (f_offset - *offset);
	*offset = f_offset;
	
return ret;
}


//xcf
int file_xcf(unsigned char *buf, int *size, __u32 scan , int flag, struct found_data_t* f_data){
	int 			i,ret = 0;
	__u32			offset;
	__u32			last_match = 0;
	__u32			b_count;


	switch (flag){
		case 0 :
			if(f_data->scantype & DATA_READY){
				*size = (f_data->next_offset)?f_data->next_offset : *size;
				ret = 1 ;
			}
			else {
				__u32	*p1;
				__u32	e[4];
				if (*size > 16){
					p1 = (__u32*) (buf + *size -4);
					for (i = 0; i<4; i++){
						e[i] = ext2fs_be32_to_cpu(*p1);
						p1--;
					}
					if((e[0]<65) && ((!e[1])||(!e[2])) && ((e[3] & ~1) == (e[0]<<1))){
						ret = 1;
						*size += 4;
						break;
					}
				} 
				if (*size < (current_fs->blocksize - 8))
					ret = 2;
				if (*size < (current_fs->blocksize - 12))
					ret = 1;
				*size += 4;
			}
			break;
		case 1 :return ((scan & (M_IS_META | M_CLASS_1 | M_BLANK ))||(f_data->scantype & DATA_READY)) ? 0 :1 ;
			break;
		case 2 :
			if ((buf[8]==0x20) && (!buf[13]) && (!buf[22]) && (!buf[23]) && (!buf[24]) && (buf[25]<4) 
				 && (f_data->priv = malloc(sizeof(struct xcf_priv_t)))){
	
				f_data->priv_len = sizeof(struct xcf_priv_t);
				memset(f_data->priv,0,sizeof(struct xcf_priv_t));
				f_data->last_match_blk =1;
			} else{
				f_data->first = 0;
				return 0;
			}
			offset = 0;
			b_count = (f_data->buf_length > 12) ? 12 : f_data->buf_length ;
			ret = follow_xcf(buf,b_count, &offset, &last_match,f_data->priv);
			ret = analysis_ret1("XCF", f_data, ret , b_count, offset, last_match);
	  	break;
		case 3 :
			offset = f_data->next_offset ;
			ret = follow_xcf(buf, f_data->buf_length, &offset, &last_match,f_data->priv);
			ret = analysis_ret2("XCF", f_data, ret, offset, last_match);
		break;
		case 4:
			if (f_data->priv)
				free(f_data->priv);
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
			if ((f_data->size) && (f_data->inode->i_size >= f_data->size)) {
				counter = ((f_data->size-1) % current_fs->blocksize)+1;
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
			if (counter && (counter <= 256)){
				
				ico_dir = (struct ico_directory*)(buf + 6) ;
				while (counter && offset){
					if ((!ico_dir->reserved) && ico_dir->width && ico_dir->heigth){
						if (ext2fs_le32_to_cpu(ico_dir->bitmap_offset) >= offset){
							offset = ext2fs_le32_to_cpu(ico_dir->bitmap_offset) + ext2fs_le32_to_cpu(ico_dir->bitmap_size);
						}//else 
						//	offset = 0;
					}
					else{
						offset = 0;
						break;
					}		
					counter-- ;
					ico_dir++ ;
				}
			}
			if ((offset > 22) && (! counter)){
				f_data->size = offset;
				f_data->scantype = DATA_LENGTH;
				ret = 1;
			}
			else 
				f_data->func = NULL;
			break;
	}
	return ret;
}


//tga
int file_tga(unsigned char *buf, int *size, __u32 scan , int flag, struct found_data_t* f_data){
	int 	i,j,x,y;
	__u16	*p ;
	__u32	ssize = 0;
	__u32	psize = 0;
	int 	ret = 0;
	char	token[]="-XFILE.";

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
						ssize = ((f_data->size-1) % current_fs->blocksize)+1;
						if (f_data->inode->i_size > (f_data->size - ssize)){
//							*size = ssize;
							ret = 2;
						}
					}
					else{
						if ((*size) < (current_fs->blocksize - 256)){
							ret=1;
						}
						else
							if ((*size) < (current_fs->blocksize - 32)){
								ret=2;
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
					p = (__u16*) (buf + 8);
					i = ext2fs_le16_to_cpu(*p);
					p++;
					j = ext2fs_le16_to_cpu(*p);
					p++;
					x = ext2fs_le16_to_cpu(*p);
					p++;
					y = ext2fs_le16_to_cpu(*p);
					if ((x<32)||(y<32)|| (i>(x/8))||(j>(y/8))){
						f_data->first = 0;
						return 0;
					}
					ssize = x * y;
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
					if (ssize){
						ssize += (psize + 18);
						f_data->size = ssize;
						f_data->scantype = DATA_MIN_LENGTH;
						ret = 1;
					}
				}
				else //runlength encoding
					f_data->scantype = H_F_Carving;
			}
			else{
				f_data->func = NULL;
				ret = 0;
			}
			break;
	}
	return ret;
}	

//matroska
int file_mkv(unsigned char *buf, int *size, __u32 scan , int flag, struct found_data_t* f_data){
	int 		i, ret = 0;
	unsigned char 	header[]  ={0x1a,0x45,0xDF,0xA3};
	unsigned char 	segment[] ={0x18,0x53,0x80,0x67};
	unsigned char 	*p;
	__u64		offset = 0;
	__u64		result;

	switch (flag){
		case 0 :

			if (f_data->size || f_data->h_size) {
				if (f_data->h_size != f_data->inode->i_size_high)
					ret = 0;
				else{
					if (f_data->inode->i_size < f_data->size)
						ret = 0;
					else{
						if (f_data->scantype & DATA_LENGTH){
							*size =  ((f_data->size-1) % current_fs->blocksize)+1 ;
							ret = 1;
						}
						else{
							if (*size < (current_fs->blocksize -16))
								ret = 4;

							if (*size < (current_fs->blocksize -64))
								ret = 2;

							if (*size < (current_fs->blocksize -256))
								ret = 1;
						}
					}
				}
			}
			break;
		case 1 :
			return (scan & (M_IS_META | M_CLASS_1)) ? 0 :1 ;
			break;
		case 2 :
			p = buf;
			if (strncmp((char*)p,(char*)header,4))
				break;
			p +=4;
			i = read_ebml(&result,(void*)p);
			if (!i)
				break;
			offset = result + 4 + i;
			p += (__u32) result + i;
			 
			if (strncmp((char*)p,(char*)segment,4))
				break;	
			p +=4;
			i = read_ebml(&result,(void*)p);
			if (!i)
				break;
			offset += result + 4 + i;
			f_data->size = offset & 0xFFFFFFFF;
			f_data->h_size = offset >> 32;
			f_data->scantype = DATA_LENGTH;
			ret = 1;
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
	__u32		b_count;
	__u32		offset = 0;
	__u32		last_match = 0;

	switch (flag){
		case 0 :
			if (f_data->size){
				ssize = ((f_data->size-1) % current_fs->blocksize)+1;
				if ((f_data->inode->i_size >= f_data->size) && (f_data->inode->i_size > (f_data->size - ssize))){
					*size = ssize;
					ret =1;
				}
			}
			else{
				if ((*size) < (current_fs->blocksize - 3)){
					ret=1;
				}
			}
			break;
		case 1:
			return ((scan & (M_IS_META | M_CLASS_1 | M_BLANK ))||(f_data->scantype & DATA_READY)) ? 0 :1 ;
			break;
		
		case 2:
			p_32 = (__u32*)(buf+4);
			ssize = ext2fs_le32_to_cpu(*p_32);
			if (buf[0] == 'F'){
				f_data->size = ssize;
				f_data->scantype = DATA_LENGTH;
				ret = 1;
			}
			else {  
				ssize -= 8;

				if(init_priv_zlib(f_data))
					return 0;
				((struct priv_zlib_t*)(f_data->priv))->flag = 0x6;
				if ((buf[0] == 'C') && (!follow_zlib(NULL,0,&ssize,NULL,(struct priv_zlib_t *)f_data->priv))){
					offset = 8;
					b_count = (f_data->buf_length > 12) ? 12 : f_data->buf_length ;
					ret = follow_zlib(buf, b_count, &offset, &last_match,(struct priv_zlib_t *)f_data->priv);
					ret = analysis_ret1("CWF", f_data, ret , b_count, offset, last_match);
				}
			}
		break;
		case 3 :
			offset = f_data->next_offset ;
			ret = follow_zlib(buf, f_data->buf_length, &offset, &last_match,(struct priv_zlib_t *)f_data->priv);
			ret = analysis_ret2("CWF", f_data, ret, offset, last_match);
		break;
		case 4:
			if (f_data->priv){
				if (((struct priv_zlib_t*)(f_data->priv))->flag)
					inflateEnd( &((struct priv_zlib_t*)(f_data->priv))->z_strm);
				free(f_data->priv);
			}
		break;
	}
	return ret;
}

			
/*
//aiff
int file_aiff(unsigned char *buf, int *size, __u32 scan , int flag, struct found_data_t* f_data){
	int 		ret = 0;
	__u32		*p_32;
	__u32		ssize;

	switch (flag){
		case 0 :
			if ((f_data->size) && (f_data->inode->i_size >= f_data->size)) {
				ssize = ((f_data->size-1) % current_fs->blocksize)+1;
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
				f_data->scantype = DATA_LENGTH;
				ret = 1;
			}
		break;
	}
	return ret;
}
*/

static int follow_asf(unsigned char *buf, __u16 blockcount, __u32 *offset, __u32 *last_match,  int flag){
static const unsigned char top_header[16]   = { 0x30,0x26,0xb2,0x75,0x8e,0x66,0xcf,0x11,0xa6,0xd9,0x00,0xaa,0x00,0x62,0xce,0x6c };
static const unsigned char index[6][8] = {
{0x36,0x26,0xb2,0x75,0x8e,0x66,0xcf,0x11},
{0x90,0x08,0x00,0x33,0xb1,0xe5,0xcf,0x11},
{0xd3,0x29,0xe2,0xd6,0xda,0x35,0xd1,0x11},
{0xf8,0x03,0xb1,0xfe,0xad,0x12,0x64,0x4c},
{0xd0,0x3f,0xb7,0x3c,0x4a,0x0c,0x03,0x48},
{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}};
	int 		ret = 1;
	unsigned char	*p_offset;
	__u64		f_offset;
	__u64		obj_size;
	int 		j,i=0;

	f_offset = (__u64) *offset;

	while ( (ret == 1) && (f_offset < (blockcount * current_fs->blocksize)-57)){
		obj_size = 0;
		p_offset = buf + (f_offset & 0xffffffff);
		if (flag && (!f_offset)){
			if( memcmp(buf,top_header,16)) {
				i = 7;
			}
		}
		else {
			for (i=0 ; i<6 ; i++){
				for (j=0;j<8;j++){
					if (! (p_offset[j] == index[i][j])) 
						break;
				}
				if(j < 8)
					continue;
				break;
			}
		}
		if (i<6){
			obj_size = (__u64) p_offset[16] | (p_offset[17]<<8) | (p_offset[18]<<16) |  (p_offset[19]<<24) | 
				   ((__u64)p_offset[20]<<32) | ((__u64)p_offset[21]<<40) | ((__u64)p_offset[22]<<48) | ((__u64)p_offset[23]<<56) ;
		}
		else 
			ret = 0;
		
		if (ret && (obj_size < 30)){
			if ((!obj_size) && (i==5) && (zero_space(buf, f_offset & 0xffffffff)))
				ret =2;       //FIXME if this is zero block 
			else  
				ret = 0;
		}
		
		if (ret){
			*last_match = (__u32)(f_offset & 0xffffffff);
			f_offset += obj_size;
		}	
	}
	*offset = (__u32) f_offset & 0xFFFFFFFF ;
	return ret;	
}

//asf
int file_asf(unsigned char *buf, int *size, __u32 scan , int flag, struct found_data_t* f_data){
	int 		ret = 0;
	__u32		last_match = 0;
	__u32		offset;
	int		b_count;

	switch (flag){
		case 0 :
			if (f_data->size || f_data->h_size) {
				if (f_data->h_size != f_data->inode->i_size_high)
					ret = 0;
				else{
					if (f_data->inode->i_size < f_data->size)
						ret = 0;
					else{
						if (f_data->scantype & DATA_READY){
							*size =  ((f_data->size-1) % current_fs->blocksize)+1 ;
							ret = 1;
						}
						else{
							*size += 4;
							ret = 2;
						}
					}
				}
			}
			break;
		case 1:
			return ((scan & (M_IS_META | M_CLASS_1 | M_BLANK ))||(f_data->scantype & DATA_READY)) ? 0 :1 ;
			break;
		case 2 :
			offset = 0;
			b_count = (f_data->buf_length > 12) ? 12 : f_data->buf_length ;
			ret = follow_asf(buf, b_count, &offset, &last_match,1);
			ret = analysis_ret1("ASF", f_data, ret , b_count, offset, last_match);
			break;
		case 3 :
			offset = f_data->next_offset ;
			ret = follow_asf(buf, f_data->buf_length, &offset, &last_match,0);
			ret = analysis_ret2("ASF", f_data, ret, offset, last_match);
		break;
	}
	return ret;
}

struct flac_priv_t {
	__u8 	b_head[4];
	__u32 	begin;
	__u32	max_blocks;
	__u16	crc;
	__u16	flag;} ;


static int follow_flac(unsigned char *buf, __u16 blockcount, __u32 *offset, __u32 *last_match,  void *priv){
	int			size,ret = 1;
	struct flac_priv_t 	*p_data = (struct flac_priv_t*) priv;
	__u32			end = blockcount * current_fs->blocksize;
	__u32			blocksize;
	__u32			bits_per_sample;
	__u32			channel;
//	__u64			total_samples;
	__u32			f_offset = *offset;


	p_data->begin = f_offset;
	while ( (ret == 1) && (f_offset < (end-21))){
		switch (p_data->flag){
		case 0: //search metadata
			if ((buf[f_offset] == 0xff) && ((buf[f_offset+1] & 0xfd) == 0xf8)){	// first Blockheader
				(p_data->flag)++;
				memcpy(p_data->b_head,buf+f_offset,4);
				*last_match = f_offset;
				p_data->flag = 1;
	//			p_data->begin = f_offset;
				break;
			}
			if ((buf[f_offset] & 0x7f) > 126 )	// invalid break
				ret = 0;
			else{  //meta data
				size = (buf[f_offset+1]<<16) + (buf[f_offset+2]<<8) + buf[f_offset+3] +4;
				if(!(buf[f_offset] & 0x7f)){ //stream info
					blocksize = (buf[f_offset+4]<<8) + buf[f_offset+5];
					bits_per_sample = ((buf[f_offset+16] & 0x01) << 4) + ((buf[f_offset+17] & 0xf0) >> 4) +1 ;
					channel = ((buf[f_offset+16] & 0x0e) >>1) +1;
//					total_samples = (((__u64)(buf[f_offset+17] & 0x0f0))<<32)+(buf[f_offset+18] <<24)+(buf[f_offset+19] <<16)+
//						(buf[f_offset+20] <<8)+(buf[f_offset+21]);	
					p_data->max_blocks = blocksize * 2 * channel;
				}
				f_offset += size;
			}
		break;
		case 1: // first FRAME_HEADER
			if ((p_data->b_head[0] == buf[f_offset]) && (p_data->b_head[1] == buf[f_offset+1]) && 
				(p_data->b_head[2] == buf[f_offset+2]) && ((p_data->b_head[3]&0x0f) == (buf[f_offset+3]&0x0f))){
				p_data->begin = f_offset;
				p_data->flag = 3;
				p_data->crc = 0;
				f_offset += 4;
			}
		case 2:
			f_offset += 4;
			p_data->flag = 3;
		case 3:  
			while (( f_offset < (end -4)) && ((f_offset - p_data->begin)< p_data->max_blocks) &&
					(!((buf[f_offset] == 0xff) && ((buf[f_offset+1] & 0xfd) == 0xf8) &&
					(p_data->b_head[2] == buf[f_offset+2]) && ((p_data->b_head[3]&0x0f) == (buf[f_offset+3]&0x0f))))){
					f_offset ++;
					if ((!(buf[f_offset])) && (!(buf[f_offset+1]))&& (!(buf[f_offset+2]))&&(zero_space(buf,f_offset))){
						p_data->crc = FLAC__crc16(buf+p_data->begin,f_offset - p_data->begin-2,p_data->crc);
						p_data->begin = f_offset - 2;
						if ((((p_data->crc) & 0x00ff) == buf[f_offset-1]) &&  ((((p_data->crc) & 0xff00)>>8) == buf[f_offset-2])){
							*offset = f_offset;
							return 2;  // file complete
						}
					}
			}
			if (f_offset == (end -4)){
				f_offset--;
				p_data->crc = FLAC__crc16(buf+p_data->begin,f_offset - p_data->begin,p_data->crc);
			}
			else {
				if ((f_offset - p_data->begin)== p_data->max_blocks)
					ret = 0;
				else {
					p_data->crc = FLAC__crc16(buf+p_data->begin,f_offset - p_data->begin-2,p_data->crc);
					if (!((((p_data->crc) & 0x00ff) == buf[f_offset-1]) &&  ((((p_data->crc) & 0xff00)>>8) == buf[f_offset-2])))
						ret = 0;
					else {
						p_data->begin = f_offset;
						*last_match = f_offset;
						p_data->crc = 0;
						p_data->flag = 2;
					}	
				}
			}
		break;		
		}	
	}
	*offset = f_offset;
return ret;
}


static int follow_creative_voice(unsigned char *buf, __u16 blockcount, __u32 *offset, __u32 *last_match,  int flag){
	int			size,ret = 1;
	__u32			end = blockcount * current_fs->blocksize;
	__u32			f_offset = *offset;

	while ( (ret == 1) && (f_offset < (end-4))){
		size = buf[f_offset+1] + ((buf[f_offset+2])<<8) + ((buf[f_offset+3])<<16);
		switch (buf[f_offset]){
			case 0 :
				if (zero_space(buf, f_offset)){
					f_offset++;
					ret= 2;
				}
				else
					ret = 0;
			break;
			case 1 :
			case 2 :
			case 3 :
			case 4 :
			case 5 :
			case 6 :
			case 7 :
			case 8 :
				*last_match = f_offset;
				f_offset += (size +4) ;
			break;
			case 9 :
				*last_match = f_offset;
				f_offset += (size + 12) ;
			break;
			default:
				ret = 0;
			break;
		}
	}
	*offset = f_offset;
	return ret;
}				
				
				


//Creative Voice
int file_creative_voice(unsigned char *buf, int *size, __u32 scan , int flag, struct found_data_t* f_data){
	unsigned char cv_header[20]  = "Creative Voice File" ; 
	cv_header[19] = 0x1a;
	__u32 		offset;
	__u32		last_match = 0;
	__u32		b_count;
	__s16		tmp;
	int 		ret = 0;

switch (flag){
		case 0 :
			if(f_data->scantype & DATA_READY){
				*size = (f_data->next_offset)?f_data->next_offset : *size;
				ret = 1;
			}
			else{
				if (*size < (current_fs->blocksize -8))
					ret = 4;

				if (*size < (current_fs->blocksize -64))
					ret = 2;

				if (*size < (current_fs->blocksize -128))
					ret = 1;
				(*size)++;
			}
			break;

		case 1 :
			return ((scan & (M_IS_META | M_CLASS_1 | M_BLANK))||(f_data->scantype & DATA_READY)) ? 0 :1 ;
			break;

		case 2 :	
			tmp = ~((buf[23]<<8)+buf[22]);
			if ((memcmp(buf,cv_header,20)) || ((__s16)(((buf[25])<<8) + buf[24] -0x1234) != tmp)){
				f_data->func = NULL;
				return 0;
			}
				
			offset = ((buf[21])<<8) + buf[20];
			b_count = (f_data->buf_length > 12) ? 12 : f_data->buf_length ;
			ret = follow_creative_voice(buf, b_count, &offset, &last_match,0);
			ret = analysis_ret1("CREATIVE-VOICE", f_data, ret , b_count, offset, last_match);
			break;
		case 3 :
			offset = f_data->next_offset ;
			ret = follow_creative_voice(buf, f_data->buf_length, &offset, &last_match,0);
			ret = analysis_ret2("CREATIVE-VOICE", f_data, ret, offset, last_match);
		break;
		}
	return ret;
}




//flac
int file_flac(unsigned char *buf, int *size, __u32 scan , int flag, struct found_data_t* f_data){
	__u32 	offset;
	__u32	last_match = 0;
	__u32	b_count;
	int 	ret = 0;
	
//static const unsigned char 	token[4]= {0x66,0x4c,0x61,0x43};

switch (flag){
		case 0 :
			if(f_data->scantype & DATA_READY){
				*size = (f_data->next_offset)?f_data->next_offset : *size;
				ret = 1;
			}
			else{
				if ((f_data->size) && (f_data->size < f_data->inode->i_size)){
					if (*size < current_fs->blocksize-7)
						ret = 1;
				}
				else {
					if (!f_data->size){
						if (*size < current_fs->blocksize-64)
							ret = 2;
						if (*size < current_fs->blocksize-128)
							ret = 1;
					}
				}
			}
			break;
		case 1 :
			if (f_data->size > f_data->inode->i_size)
				ret = (scan & (M_IS_META | M_CLASS_1 )) ? 0 :1 ;
			else
				ret = ((scan & (M_IS_META | M_CLASS_1 | M_BLANK | M_TXT ))||(f_data->scantype & DATA_READY)) ? 0 :1 ;
			break;
		case 2:
			f_data->priv = malloc(sizeof(struct flac_priv_t));
			if(f_data->priv){
				f_data->priv_len = sizeof(struct flac_priv_t);
				memset(f_data->priv,0,sizeof(struct flac_priv_t));
			}
			else {
				f_data->first = 0;
				return 0;
			}
				
			offset = test_id3_tag(buf) +4 ;
			b_count = (f_data->buf_length > 12) ? 12 : f_data->buf_length ;
			ret = follow_flac(buf, b_count, &offset, &last_match,f_data->priv);
			ret = analysis_ret1("FLAC", f_data, ret , b_count, offset, last_match);
			break;
		case 3 :
			offset = f_data->next_offset ;
			ret = follow_flac(buf, f_data->buf_length, &offset, &last_match,f_data->priv);
			ret = analysis_ret2("FLAC", f_data, ret, offset, last_match);
		break;
		case 4 :
				if (f_data->priv)
					free(f_data->priv);
				ret = 1;
		break;
	}
	return ret;
}



static int follow_mpeg(unsigned char *buf, __u16 blockcount, __u32 *offset, __u32 *last_match,  int flag){
	int 		ret =1;
    	__u32	 	end;
	__u32		frame_offset = *offset;
    	__u32	 	tmp = 0;
	__u32		i ;
	int		tolerance = 64; //for incorrect zero-byte padding
	
	end = blockcount * current_fs->blocksize ;
	while ((ret == 1) && ((frame_offset+14) <= end)){
		if (buf[frame_offset] || buf[frame_offset+1] || (buf[frame_offset+2] != 0x01)) {
			if (tolerance){
				if ((!buf[frame_offset]) && (!buf[frame_offset+1]) && (!buf[frame_offset+2])){
					frame_offset++;
					tolerance --;
					continue;
				}
			}
			if (zero_space(buf,frame_offset))
				ret = 3;
			else 
				ret = 0;
			break;
		}
		if (!flag){
			switch (buf[frame_offset+3]) {//Multiplexed Stream
				case 0xb9:      /* program end code */
					ret = 2;
					frame_offset += 4;
					break;
				case 0xba:      /* pack header */
					if ((buf[frame_offset+4] & 0xc0) == 0x40)             /* mpeg2 */
						tmp = 14 + (buf[frame_offset+13] & 7);
					else 	if ((buf[frame_offset+4] & 0xf0) == 0x20)     /* mpeg1 */
							tmp = 12;
					*last_match = frame_offset;
					frame_offset += tmp;
					break;
				//audio
				case 0xc0:
				case 0xc1:
				case 0xc2:
				case 0xc3:
				case 0xc4:
				case 0xc5:
				case 0xc6:
				case 0xc7:
				case 0xc8:
				case 0xc9:
				case 0xca:
				case 0xcb:
				case 0xcc:
				case 0xcd:
				case 0xce:
				case 0xcf:
				case 0xd0:
				case 0xd1:
				case 0xd2:
				case 0xd3:
				case 0xd4:
				case 0xd5:
				case 0xd6:
				case 0xd7:
				case 0xd8:
				case 0xd9:
				case 0xda:
				case 0xdb:
				case 0xdc:
				case 0xdd:
				case 0xde:
				case 0xdf:
				//video
				case 0xe0:
				case 0xe1:
				case 0xe2:
				case 0xe3:
				case 0xe4:
				case 0xe5:
				case 0xe6:
				case 0xe7:
				case 0xe8:
				case 0xe9:
				case 0xea:
				case 0xeb:
				case 0xec:
				case 0xed:
				case 0xee:
				case 0xef:
				//privat stream / Padding stream /System Header/Program Stream Map
				case 0xbb:	
				case 0xbc:
				case 0xbd:
				case 0xbe: 
				case 0xbf:	
					tmp = 6 + (buf[frame_offset+4] << 8) + buf[frame_offset+5];
					*last_match = frame_offset;
					frame_offset += tmp;
					break;
				default:
					if (buf[frame_offset+3] < 0xb9) {
//						fprintf(stderr, "broken stream - skipping data\n");
						tmp = 6 + (buf[frame_offset+4] << 8) + buf[frame_offset+5];
	//					*last_match = frame_offset;
						frame_offset += tmp;
						break;
					}
					else
						ret = 3;  //0xf0->0xff
			}
		}
		else{ //Elementar Stream
			switch (buf[frame_offset+3]) {
				case 0x00: //Picture
					tmp = 8;
					if (buf[frame_offset+5] & 0x10) //backward_vector	
						tmp += 1;
					//FIXME (extra_bit_picture)
					*last_match = frame_offset;
					frame_offset += tmp ; 
					break;
	
				case 0xb3: //Sequence Header
					*last_match = frame_offset;
					if (frame_offset < (end -75)){
						tmp = (buf[frame_offset+11] & 0x2)? 76 : 12 ;  //Intra Matrix
						tmp += (buf[frame_offset + tmp -1] & 0x01) ? 64 : 0; //Non Intra Matrix
						frame_offset += tmp ;
					}
						else ret = 8; 
	
					break;
				case 0xb5: //extension header
					*last_match = frame_offset;
					if (frame_offset < (end -196)){
						switch (buf[frame_offset+4] & 0xf0){
							case 0x10: //Sequence Extension
								tmp = 10; break; 
							case 0x20: //Sequence Display Extension
								tmp = (buf[frame_offset+4] & 0x01) ? 12 : 9; 
								break;
							case 0x30: //Quant Matrix Extension
								tmp = (buf[frame_offset+4] & 0x8) ?  69 : 5 ; //Intra Quantiser Matrix
								tmp += (buf[frame_offset+tmp-1] & 0x4) ? 64 : 0 ; //Non Intra Quantiser Matrix
								tmp += (buf[frame_offset+tmp-1] & 0x2) ? 64 : 0 ; //Chroma Intra Quantiser Matrix
								tmp += (buf[frame_offset+tmp-1] & 0x1) ? 64 : 0 ; //Chroma Non Intra Quantiser Matrix
								break;
							case 0x40: //Copyright Extension
								tmp = 15;
								break;
							case 0x50: //Sequence Scalable Extension
		//						tmp = (buf[frame_offset+4] & 0x0c) ??? 6 ??? 7 ??? 12 ;
		//FIXME
								tmp = 6;
								while ((tmp <= 12) && (buf[frame_offset + tmp] || buf[frame_offset + tmp +1] || (buf[frame_offset+ tmp +2] != 0x01)))
									tmp++;
								if (tmp > 12)
									ret = 0;
								break;
							case 0x70: //Picture Display Extension
								tmp = 9;
								break;
							case 0x80: //Picture Coding Extension 
								tmp = (buf[frame_offset+8] & 0x40) ? 11 : 9;
								break;
						}
					frame_offset += tmp;
					}
					else 
						ret = 8;
					break;
				case 0xb7: //Sequence End
					tmp = 4 ; 
					frame_offset += tmp ;
					if ((!buf[frame_offset]) && (!buf[frame_offset+1]) && (buf[frame_offset+2] == 1))
						break;
					else
						ret = 2;
					break;
				case 0xb8: //Group of Picture
					tmp = 8;
					*last_match = frame_offset;
					frame_offset += tmp;
					break;
				default:
					if ((buf[frame_offset+3] < 0xb0) || (buf[frame_offset+3] == 0xb2)){ // slice or user_data
						*last_match = frame_offset;
						tmp = 6;
						while (((frame_offset + tmp + 4) < end) && (buf[frame_offset + tmp] || buf[frame_offset + tmp +1] || (buf[frame_offset+ tmp +2] != 0x01)))
							tmp++;
						if ((frame_offset + tmp + 4) >= end)
							ret = 8;
						else{
							if (((buf[frame_offset + tmp +3]) < 0xb9) && (tmp < 9500))  //max slice size ???????
								frame_offset += tmp;
							else {
								i=6;
								while ((i<(tmp-1))&&((buf[frame_offset+i])||(buf[frame_offset+i+1])))
									i++;
								if ((i < tmp) && zero_space(buf,frame_offset+i)){
									frame_offset += i;
									ret = 2;
								}
								else 
									ret = 0;
							}
						}
						break;
					}
					else{
//						fprintf(stderr, "broken mpeg elementar stream - skipping data\n");
						ret = 0;
					}
			}
		} //if
	}//while
	*offset = frame_offset;
return ret;
}


//mpeg
int file_mpeg(unsigned char *buf, int *size, __u32 scan , int flag, struct found_data_t* f_data){
	int 	i,j;
	int 	ret = 0;
	__u32	offset;
	__u32	b_count;
	__u32	last_match = 0;
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
					if ((f_data->scantype & (DATA_READY))||((f_data->last_match_blk) && 
						(f_data->last_match_blk <= (f_data->inode->i_size/current_fs->blocksize)) &&
					 	(*size < (current_fs->blocksize - 2)))){
						ret=1;
						break;
					}
						
					if(*size < (current_fs->blocksize - 48))
						ret=4;
					else{   //FIXME
						if (!(current_fs->super->s_feature_incompat & EXT3_FEATURE_INCOMPAT_EXTENTS) && (*size > (current_fs->blocksize - 2)) && (f_data->inode->i_size > (12 * current_fs->blocksize)) &&
						((f_data->inode->i_blocks - (13 * (current_fs->blocksize / 512))) % 256)  ){
							*size = current_fs->blocksize;
							 ret = 4;
						}
					}
				}
					
			break;
		case 1 :	
			return ((scan & (M_IS_META | M_CLASS_1 ))||(f_data->scantype & DATA_READY)) ? 0 :1 ;
			break;
		case 2 :
			offset = 0;
			if (buf[3] < 0xb9)
				f_data->scantype = DATA_ELEMENT | DATA_NO_FOOT ;
			b_count = (f_data->buf_length > 12) ? 12 : f_data->buf_length ;
			ret = follow_mpeg(buf, b_count, &offset, &last_match,(f_data->scantype & DATA_ELEMENT));
			if (last_match)
				f_data->last_match_blk = (last_match/current_fs->blocksize) +1 ;
			if (ret){
				f_data->scantype |= DATA_CARVING;
				f_data->size = offset;
				if (offset > (b_count * current_fs->blocksize))
					f_data->buf_length = b_count;
				else
					f_data->buf_length = offset / current_fs->blocksize;
				f_data->next_offset = offset - (f_data->buf_length * current_fs->blocksize);
			}
			switch (ret){
				case 0 :
//					fprintf(stderr,"MPEG-CHECK: Block %lu : Break\n",f_data->first);
					f_data->scantype |= DATA_BREAK;
					f_data->func = NULL;
					return 0;
				break;
				case 1:

				break;
				case 2 :
					f_data->scantype |= DATA_READY;
					if(offset % current_fs->blocksize)(f_data->buf_length)++;
				break;
				case 3:
//					fprintf(stderr,"MPEG-CHECK: mpeg-sequence found, H_F-Carving on\n");
					f_data->scantype = H_F_Carving;
				break;
				case 8 :
					 ret = 1;
				break;
			}
			break;
		case 3 :
			offset = f_data->next_offset ;
			ret = follow_mpeg(buf, f_data->buf_length, &offset, &last_match,(f_data->scantype & DATA_ELEMENT));
			if (last_match)
				f_data->last_match_blk = ((f_data->size - f_data->next_offset + last_match)/current_fs->blocksize) +1 ;
			if (ret){
				f_data->size += offset - f_data->next_offset;
				f_data->buf_length = offset / current_fs->blocksize;
				f_data->next_offset = offset - (f_data->buf_length * current_fs->blocksize);
			}
			switch (ret){
				case 0 :
//					fprintf(stderr,"MPEG-CHECK: mpeg-data error, recover not\n");
					f_data->scantype |= DATA_BREAK;	
				break;
				case 8 :
					 ret = 1;
			//	NO break;
				case 1 :
//					fprintf(stderr,"MPEG-CHECK: mpeg-data next found\n");
				break;
				case 2 :
//					fprintf(stderr,"MPEG-CHECK: mpeg-data end found, recover\n");
					f_data->scantype |= DATA_READY;
					if(offset % current_fs->blocksize) (f_data->buf_length)++;
				break;
				case 3:
//					fprintf(stderr,"MPEG-CHECK: mpeg-sequence or end of stream found, H_F-Carving on\n");
					if (!f_data->next_offset){
						f_data->scantype |= DATA_READY;
						ret = 2;
					}
					else 
						f_data->scantype = H_F_Carving;
				break;
			}
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
				ssize = ((f_data->size-1) % current_fs->blocksize)+1;
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
			f_data->size = offset ? (offset + ssize) : ssize ;
			if (f_data->size > current_fs->blocksize)
				f_data->scantype = DATA_LENGTH;
			else
				f_data->scantype = H_F_Carving;
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
	char	*c, *txt;
	__u32		ssize = 0;
	int 		ret = 0;

switch (flag){
case 0 :
	if (f_data->size ) {
		ssize = ((f_data->size-1) % current_fs->blocksize)+1;
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
	c = (char*)buf;
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
					ssize += (__u32)(txt - (char*)buf);
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
						ssize += (__u32)(txt - (char*)buf);
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
						ssize += (__u32)(txt - (char*)buf);
					}
					break;
			}
			if (ssize){
				f_data->size = ssize;
				if (f_data->size > current_fs->blocksize)
					f_data->scantype = DATA_LENGTH;
				else
					f_data->scantype = H_F_Carving;
				ret = 1;
			}	
		}
	}
	break;
}
	return ret;
}



struct private_tiff_t {
	int 	flag ; // Bit0 search IFD; Bit1 process IFD; Bit2 test EOF; Bit7 BIGENDIAN
	__u32	count; 
	__u32	last_id;
	__u32	ifd;	//offset of last IFD
	__u32	max ;  // currently found maximum offset 
};


static int follow_tiff(unsigned char *buf, __u16 blockcount, __u32 *offset, __u32 *last_match, struct private_tiff_t*  priv){
	char b_count[13]= {0,1,1,2,4,8,1,1,2,4,8,4,8}; //size in Byte 
	int		i,j;
	int		ret = 1;
	__u32 		f_offset = *offset;
	__u16		count, id;
	__u16		type = 0;
	__u16		needful = 4 ;
	__u32		end = blockcount * current_fs->blocksize;
	__u32		max = priv->max;
	__u32		entry_offset = 0;
	__u32		entry_count = 0;
	__u32		entry;
	__u32		last_strip_offset = 0;
	__u32		last_strip_byte_count = 0;
	
	if ((priv->count) && (priv->flag & 0x2))
		needful = (priv->count * 12)+4 ;
	while ((ret == 1) && (f_offset < (end - needful))){
		switch ((priv->flag) & 0x7){
			case 1 :
				if ((priv->flag)&0x8)
					count = ((buf[f_offset])<<8) + buf[f_offset+1];
				else 
					count = ((buf[f_offset+1])<<8) + buf[f_offset];
				f_offset += 2;
				if ((count) && (count < 256)){ 
					priv->count = count;
					priv->last_id = 0;
					priv->flag += 1;
					needful = (count*12)+4;
				}
				else 
					ret = 0;
			break;
			case 2 :
				if (priv->count){
					for (i = 0; i < priv->count; i++){
						entry = 0;
						if ((priv->flag)&0x8)
							id = ((buf[f_offset])<<8) + buf[f_offset+1];
						else 
							id = ((buf[f_offset+1])<<8) + buf[f_offset];
						if (priv->last_id > id){
							ret = 0;
							break;
						}
						priv->last_id = id;
						if ((priv->flag)&0x8)
							type = ((buf[f_offset+2])<<8) + buf[f_offset+3];
						else 
							type = ((buf[f_offset+3])<<8) + buf[f_offset+2];
						if(!type || (type > 12)){
							ret = 0;
							break;
						}
						type = b_count[type];
						if ((priv->flag)&0x8){
							entry_count =  ((buf[f_offset+4])<<24)+((buf[f_offset+5])<<16)+((buf[f_offset+6])<<8)+buf[f_offset+7];
							entry_offset =  (((buf[f_offset+8])<<24)+((buf[f_offset+9])<<16)+((buf[f_offset+10])<<8)+buf[f_offset+11]);
							if ((entry_count * type)>4)
							entry =(entry_count * type ) + entry_offset;	
						}
						else{ 
							entry_count = ((buf[f_offset+7])<<24)+((buf[f_offset+6])<<16)+((buf[f_offset+5])<<8)+buf[f_offset+4];
							entry_offset = (((buf[f_offset+11])<<24)+((buf[f_offset+10])<<16)+((buf[f_offset+9])<<8)+buf[f_offset+8]);
							if ((entry_count *type)>4)
							entry =(entry_count * type) + entry_offset;	
						}
						if (!entry_count){
							ret=0;
							break;
						}

						if ((id == 273) && (entry) && (entry < (priv->ifd - *offset + end))&& ((priv->ifd - *offset) < entry)){//StripOffsets
							j = (priv->ifd)? (entry - priv->ifd + *offset) : entry;
							switch (type){
								case 2:
									if ((priv->flag)&0x8)
										last_strip_offset = ((buf[j-2])<<8) + buf[j-1] ;
									else
										last_strip_offset = ((buf[j-1])<<8) + buf[j-2] ;
								break;
								case 4:
									if ((priv->flag)&0x8)
										last_strip_offset = ((buf[j-4])<<24) + ((buf[j-3])<<16)+
										 ((buf[j-2])<<8) + buf[j-1] ;
									else
										last_strip_offset = ((buf[j-1])<<24) + ((buf[j-2])<<16)+
										 ((buf[j-3])<<8) + buf[j-4] ;
								break;
								default:
									ret = 0;
							}
							
						}
						if ((id == 279) && (entry) && (entry < (priv->ifd - *offset + end)) && ((priv->ifd - *offset) < entry)){//StripByteCounts
							j = (priv->ifd)? (entry - priv->ifd + *offset) : entry;
							switch (type){
								case 2:
									if ((priv->flag)&0x8)
										last_strip_byte_count = ((buf[j-2])<<8) + buf[j-1] ;
									else
										last_strip_byte_count = ((buf[j-1])<<8) + buf[j-2] ;
								break;
								case 4:
									if ((priv->flag)&0x8)
										last_strip_byte_count = ((buf[j-4])<<24)+((buf[j-3])<<16)+
										 ((buf[j-2])<<8) + buf[j-1] ;
									else
										last_strip_byte_count = ((buf[j-1])<<24)+((buf[j-2])<<16)+
										 ((buf[j-3])<<8) + buf[j-4] ;
								break;
								default:
									ret = 0;
							}
						}
						if (!ret)
							break;

						if (max < entry)
							max = entry;
						f_offset += 12;
					}
					if (last_strip_offset && last_strip_byte_count){
						entry = last_strip_offset + last_strip_byte_count;
						if (max < entry)
							max = entry;
					}
					priv->max = max;
					if ((!buf[f_offset])&&(!buf[f_offset+1])&&(!buf[f_offset+2])&&(!buf[f_offset+3])){
						*last_match = f_offset;
						f_offset += 4;
						if ((max > (priv->ifd+12*priv->count))&&((max - priv->ifd + *offset) > f_offset))
							f_offset = max - priv->ifd + *offset;
						priv->flag += 2;
						needful = 1;
					}
					else{
						if ((priv->flag)&0x8)
							entry = (((buf[f_offset])<<24)+((buf[f_offset+1])<<16)+((buf[f_offset+2])<<8)+ buf[f_offset+3]);
						else 
							entry = (((buf[f_offset+3])<<24)+((buf[f_offset+2])<<16)+((buf[f_offset+1])<<8)+ buf[f_offset]);
						f_offset = entry - priv->ifd + *offset;
						priv->ifd = entry;
						priv->flag -= 1;
						needful = 4;
					}
				}
				else 
					ret = 0;
			break;
			case 4 :	
					if ((!buf[f_offset])&& (zero_space(buf,f_offset)))
						ret = 2;
					else 
						ret = 3;
			break;
			default : ret = 0;
		}
	}
	*offset = f_offset;
return ret;
}



// tiff
int file_tiff(unsigned char *buf, int *size, __u32 scan , int flag, struct found_data_t* f_data){
	__u32			*p_32 ;
	__u32			offset = 0;
	__u32			last_match = 0;
	int 			b_count,ret = 0;
	struct private_tiff_t*	priv = NULL;	

	switch (flag){
		case 0 :
			if (f_data->scantype & DATA_READY){
				*size = f_data->next_offset;
				ret =1;
			}
			else {
				if (f_data->size ){
					if ((f_data->inode->i_size > f_data->size) && (*size < (current_fs->blocksize -8))){
						ret = 4;
						if (*size < (current_fs->blocksize - 256))
							ret = 1;
						*size += 8;
					}
				}
				else{
					if (f_data->scantype & DATA_METABLOCK){   //FIXME 
						*size += 8;
						ret = 2;
					}
				}
			}				
			break;
		case 1 : 
			return ((scan & (M_IS_META | M_CLASS_1))||(f_data->scantype & DATA_READY)) ? 0 :1 ;
			break;
		case 2 :
			priv = malloc(sizeof(struct private_tiff_t));
			if (!priv){
				return 0;
			}
			f_data->priv = priv;
			priv->max = 0;
			priv->count = 0;
			priv->flag = 0;
			priv->last_id = 0;	
			p_32 = (__u32*)buf;
			p_32++;
			if ((*buf == 0x49) && (*(buf+1) == 0x49)){
				priv->flag = 0x01;
				offset = ext2fs_le32_to_cpu(*p_32);
			}
			if ((*buf == 0x4d) && (*(buf+1) == 0x4d)){
				priv->flag = 0x09;
				offset = ext2fs_be32_to_cpu(*p_32);
			}
			if ((!offset) || (!priv->flag)){
				f_data->first = 0;
				return 0;
			}
			f_data->last_match_blk = 1;
			priv->ifd = offset;
			b_count = (f_data->buf_length > 12) ? 12 : f_data->buf_length ;
			ret = follow_tiff(buf,b_count, &offset, &last_match,f_data->priv);
			ret = analysis_ret1("TIFF", f_data, ret , b_count, offset, last_match);
			break;
		case 3 :
			offset = f_data->next_offset ;
			ret = follow_tiff(buf, f_data->buf_length, &offset, &last_match,f_data->priv);
			ret = analysis_ret2("TIFF", f_data, ret, offset, last_match);
		break;
		case 4 :
				if (f_data->priv)
					free(f_data->priv);
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

//---CDF-------------------------------------------------------
struct OLE_HDR{
	char		magic[8];		// 0
	char		clsid[16];		// 8
	__u16		uMinorVersion;		// 24
	__u16		uDllVersion;		// 26
	__u16		uByteOrder;		// 28
	__u16		uSectorShift;		// 30
	__u16		uMiniSectorShift;	// 32
	__u16		reserved;		// 34
	__u32		reserved1;		// 36
	__u32		reserved2;		// 40
	__u32		num_FAT_blocks;		// 44
	__u32		root_start_block;	// 48
	__u32		dfsignature;		// 52
	__u32		miniSectorCutoff;	// 56
	__u32		dir_flag;		// 60 first sec in the mini fat chain
	__u32		csectMiniFat;		// 64 number of sectors in the  minifat
	__u32		FAT_next_block;		// 68
	__u32		num_extra_FAT_blocks;	// 72
        // FAT block list starts here !! first 109 entries
};

struct OLE_DIR {
	char		name[64];       // 0
	__u16		namsize;         // 64
	char		type;           // 66
	char		bflags;         // 67: 0 or 1
	__u32		prev_dirent;    // 68
	__u32		next_dirent;    // 72
	__u32		sidChild;       // 76
	char		clsid[16];      // 80
	__u32		userFlags;      // 96
	int		secs1;          // 100
	int		days1;          // 104
	int		secs2;          // 108
	int		days2;          // 112
	__u32		start_block;    // 116 starting SECT of stream
	__u32		size;           // 120
	__u16		reserved;       // 124 must be 0
	__u16		padding;        // 126 must be 0
};


struct priv_cdf_t{
	__u32		flag;
	__u32		shift;
	__u32		FAT_blocks;
	__u32		s_offset;
	__u32		extra_FAT_blocks;
	__u32		FAT_next_block;
	__u32		root_start_block;
	__u32		max;
	__u32		d0;
	__u32		d1;
	__u32		type;
	__u32		p[5];
};


static int read_dirname(char *buf, int len){
	int ret = 0;
	if (len > 63) 
		return ret;
	if(!(memcmp(buf, "R\0o\0o\0t\0 \0E\0n\0t\0r\0y\0",20)))
		return 0;
	if(!(memcmp(buf, "W\0o\0r\0d\0D\0o\0c\0u\0m\0e\0n\0t\0",24)))
		return 1; // doc
	if(!(memcmp(buf, "W\0o\0r\0k\0b\0o\0o\0k\0",16)))
        	return 2; // xls
	if(!(memcmp(buf,"S\0f\0x\0D\0o\0c\0u\0m\0e\0n\0t\0",22)))
		return 3; // sdw
	if(!(memcmp(buf,"S\0t\0a\0r\0D\0r\0a\0w\0",16)))
		return 4; // sda
	if(!(memcmp(buf,"S\0t\0a\0r\0C\0a\0l\0c\0",16)))
		return 5; // sdc
	if(!(memcmp(buf,"W\0k\0s\0S\0S\0W\0o\0r\0k\0B\0o\0o\0k\0",26)))
		return 6; // xlr
	if(!(memcmp(buf,"I\0m\0a\0g\0e\0s\0S\0t\0o\0r\0e\0",22)))
		return 7; // albm
	if(!(memcmp(buf,"J\0N\0B\0V\0e\0r\0s\0i\0o\0n\0", 20)))
		return 8; // jnb
	if(!(memcmp(buf,"P\0o\0w\0e\0r\0P\0o\0i\0n\0t\0",20)))
		return 9; // ppt
	if(!(memcmp(buf,"C\0O\0N\0T\0E\0N\0T\0S\0",16))) 
		return 10; //wps
	if(!(memcmp(buf,"_\0_\0n\0a\0m\0e\0i\0d\0_\0v\0e\0r\0s\0i\0o\0n\0001\0.\0000\0",38)))
		return 11; // msg
	if(!(memcmp(buf,"L\0i\0c\0o\0m\0",10))) 
		return 12; //amb
	if(!(memcmp(buf,"V\0i\0s\0i\0o\0D\0o\0c\0u\0m\0e\0n\0t\0",26)))
		return 13; // vsd
	if(!(memcmp(buf,"s\0w\0X\0m\0l\0C\0o\0n\0t\0e\0n\0t\0s\0",26)))
		return 14; // sld
	if(!(memcmp(buf,"N\0a\0t\0i\0v\0e\0C\0o\0n\0t\0e\0n\0t\0_\0M\0A\0I\0N\0", 36)))
		return 15; // qpw
	if(!(memcmp(buf,"D\0g\0n", 6)))
		return 16; // dgn
	if(!(memcmp(buf,"P\0i\0c\0t\0u\0r\0e\0s\0",16)))
		return 17; // pps
	if(!(memcmp(buf,"p\0e\0r\0s\0i\0s\0t\0 \0e\0l\0e\0m\0e\0n\0t\0s\0",32)))
		return 50; // ??? sdw
	if(!(memcmp(buf,"1\0\0\0", 4)))
		return 100; // db
	if(!(memcmp(buf,"1\0\0\0", 4)))
		return 100; // db
	return ret;
}


static int follow_cdf(unsigned char *buf, __u16 blockcount, __u32 *offset, __u32 *last_match, struct priv_cdf_t*  priv){
	int		i,j,flag;
	int		ret = 1;
	__u32 		f_offset = *offset;
	__u32		len = (1<<priv->shift);
	__u32		count = len / 4;
	__u32		end = (blockcount * current_fs->blocksize) - ((priv->flag & 0x80)? 0 : len);
	__u32		*p_32;
	struct OLE_DIR	*o_dir;
	
	while ((ret == 1) && (f_offset <= ((priv->flag & 0x10)?(blockcount * current_fs->blocksize): end))){
		switch (priv->flag & 0x1f){
			case 0 :
				if (priv->root_start_block < priv->max){
					priv->p[0] = priv->root_start_block;
					priv->flag |= 0x100;
				}
				else{
					ret = 0;
					break;
				}
				*last_match = f_offset;
				if ((f_offset == 76) && (priv->FAT_blocks < 110)){
					f_offset += (priv->FAT_blocks -1)*4;
					p_32 = (__u32*) (buf+f_offset);
					priv->p[3] = ext2fs_le32_to_cpu(*p_32);
					priv->flag |= 0x800;
					if (priv->FAT_blocks >1){
						p_32--;
						if (ext2fs_le32_to_cpu(*p_32) < priv->max){
							priv->p[2] = ext2fs_le32_to_cpu(*p_32);
							priv->flag |= 0x400;
						}
					}
				}
				else {
					if (f_offset == 76){
						priv->p[1] = priv->FAT_next_block;
						priv->flag |= 0x200;
						
					}else 
						ret = 0;
				}
			break;
			case 0x1 :
				o_dir = (struct OLE_DIR*) (buf+f_offset);
				for (i=0;((i< 4) && (o_dir->type));i++){
					if (/*(o_dir->reserved) || o_dir->padding ||*/ (o_dir->bflags >1) || (o_dir->type > 5) || (!o_dir->namsize) || (o_dir->namsize > 63) || (o_dir->name[1] && o_dir->name[2])){
						ret = 0;
						break;
					}
#ifdef DEBUG_MAGIC_SCAN
					for (j=0;j<o_dir->namsize;j++){
						if(o_dir->name[j])
							printf("%c",o_dir->name[j]);
					}
					printf("\t\t\t type : %u \t sector %u (%u bytes)\n", o_dir->type, o_dir->start_block, o_dir->size);
#endif
				
					if ((!priv->type) || (priv->type <50)){
						flag = read_dirname(o_dir->name,o_dir->namsize);
						if (flag)
							priv->type = flag;
					}
					o_dir++;
				}
				*last_match = f_offset;		
				priv->flag &= (~0x100);
			break;
			case 0x2 :
				*last_match = f_offset+1;
				if (priv->extra_FAT_blocks >1){
					p_32 = (__u32*) (buf+f_offset + len -4);
					if ((ext2fs_le32_to_cpu(*p_32) > priv->p[1]) && (ext2fs_le32_to_cpu(*p_32) < priv->max)){
						priv->p[1] = ext2fs_le32_to_cpu(*p_32);
						(priv->extra_FAT_blocks)--;
					}
					else{
//						printf("ERROR CDF Extra FAT:  %lu \n",priv-> s_offset);
						ret = 0;
					}
					break;
				}
				i = (priv->FAT_blocks -109) % (count-1);
				p_32 = (__u32*) (buf+f_offset);
				p_32 += (i-1);
				priv->flag &= (~0x200);
				if ((ext2fs_le32_to_cpu(*p_32) > priv->p[1]) && (ext2fs_le32_to_cpu(*p_32) < priv->max)){
					priv->p[3] = ext2fs_le32_to_cpu(*p_32);
					priv->flag |= 0x800;
					if (i > 1){
						p_32--;
						if ((ext2fs_le32_to_cpu(*p_32) > priv->p[1]) && (ext2fs_le32_to_cpu(*p_32) < priv->max)){
							priv->p[2] = ext2fs_le32_to_cpu(*p_32);
							priv->flag |= 0x400;
						}
					}
				}
				else {
//					printf("ERROR CDF Extra FAT:  %lu \n",priv-> s_offset);
					ret = 0;
				}
			break;
			case 0x4 :
			case 0x8 :
				j=0;
				flag = 0;
				p_32 = (__u32*) (buf+f_offset + (len-4));
				for (i=0;i < count; i++, p_32--){
					if((!flag) && (*p_32 == 0xffffffff)){
						j++;
						continue;
					}
					if ((ext2fs_le32_to_cpu(*p_32) > priv->max) && (ext2fs_le32_to_cpu(*p_32) < 0xfffffffd)){
						ret = 0;
						break;
					}
					else 
						flag++;
				}
				*last_match = f_offset+1;
				if ((priv->flag & 0x1f) == 0x4){
					priv->d0 = j;
					priv->flag &= (~0x400);
				}
				else{
					priv->d1 = j;
					priv->flag &= (~0x800);
					if (j < count)
						priv->flag &= (~0x400);
				}
				if(!(priv->flag & 0xc00)){
					priv->p[4] = priv->max - ((priv->d1 < count) ? priv->d1 : (priv->d1 + priv->d0));
					priv->flag |= 0x1000;
				}
			break;
			case 0x10 :
				if (zero_space(buf, f_offset)){
					ret = 2;
				}
				else{
					if (!(f_offset % current_fs->blocksize)){
						ret = 2;
					}
					else{
//						printf("ERROR CDF EOF:  %lu \n",priv-> s_offset);
						ret = 0;
					}
				}
			break;
		}
		if (ret == 1){
			priv->flag &= 0x1f00;
			flag = 0;
			for (i=0;i<5;i++){
				if( priv->flag & (1<<(i+8))){
					flag = i+1;
					for(j=i+1;j<5;j++){
						if(( priv->flag & (1<<(j+8))) && (priv->p[i] > priv->p[j])){
							flag = j+1;
							break;
						}
					}
					if(j==5)
						break;
				}
			}
			if (flag){
				flag--;
				f_offset = ((priv->p[flag] << priv->shift) + 512) - (priv->s_offset - *offset) ;
				priv->flag |= (1<<(flag));
			}
			else {
					ret = 0;
			}
		}
	}
	priv->s_offset = (priv->s_offset - *offset) + f_offset;
	*offset = f_offset;
	return ret;
}


//CDF
int file_CDF(unsigned char *buf, int *size, __u32 scan , int flag, struct found_data_t* f_data){
	char cdf_ext[19][4]={{'d','o','c','\0'},
	{'d','o','c','\0'},{'x','l','s','\0'},{'s','d','w','\0'},{'s','d','a','\0'},{'s','d','c','\0'},{'x','l','r','\0'},
	{'a','l','b','\0'},{'j','n','b','\0'},{'p','p','t','\0'},{'w','p','s','\0'},{'m','s','g','\0'},{'a','m','b','\0'},
	{'v','s','d','\0'},{'s','l','d','\0'},{'q','p','w','\0'},{'d','g','n','\0'},{'p','p','s','\0'},{'d','b','\0','\0'}};

	int 			b_count, ret = 0;
	struct OLE_HDR		*head;
	struct priv_cdf_t	*priv = NULL;
	__u32			FAT_blocks,extra_FAT_blocks;
	__u32			last_match = 0;
	__u16			shift;
	__u32			offset; //,last_match_blk = 0;

	switch (flag){
		case 0 :
			if (f_data->scantype & DATA_READY){
				if (f_data->priv){
					priv = f_data->priv;
					if (priv->type == 50)
						priv->type = 3;
					if (priv->type == 100)
						priv->type = 18;
					if (priv->type > 18)
						priv->type = 0;
					f_data->name[strlen(f_data->name)-3] = 0 ;
					strncat(f_data->name,cdf_ext[priv->type],4);
				}
				*size = (f_data->next_offset)?f_data->next_offset : current_fs->blocksize;
				ret =1;
			}
			else {
				if ((f_data->scantype & DATA_METABLOCK) || (*size < (current_fs->blocksize - 64))){
				*size = (((*size) +127) & ~127);
				ret = 1;
			}
		}
			break;
		case 1 :
			return (scan & (M_IS_META | M_CLASS_1)) ? 0 :1 ;
			break;
		case 2 :
			head =(struct OLE_HDR*)buf;
			FAT_blocks = ext2fs_le32_to_cpu(head->num_FAT_blocks);
			extra_FAT_blocks = ext2fs_le32_to_cpu(head->num_extra_FAT_blocks);
			shift = ext2fs_le16_to_cpu(head->uSectorShift);
			priv = malloc(sizeof(struct priv_cdf_t));
			if ((!((buf[0]==0xd0)&&(buf[1]==0xcf)&&(buf[2]==0x11)&&(buf[3]==0xe0)&&(buf[4]==0xa1)
				&&(buf[5]==0xb1)&&(buf[6]==0x1a)&&(buf[7]==0xe1) && (buf[28]==0xfe))) 
				|| (!FAT_blocks) || (extra_FAT_blocks > 50) || (FAT_blocks > ((extra_FAT_blocks * (((1<<shift)/4)-1)) +109))
				|| (shift < 6) || (shift > 11) || (! priv))   {
				f_data->first = 0;
				if( priv)
					free(priv);
				break;
			}
			offset = 76;
			memset(priv,0,sizeof(struct priv_cdf_t));
			priv->s_offset = offset;
			priv->FAT_blocks = FAT_blocks;
			priv->extra_FAT_blocks = extra_FAT_blocks;
			priv->shift = shift;
			priv->root_start_block = ext2fs_le32_to_cpu(head->root_start_block);
			priv->FAT_next_block = ext2fs_le32_to_cpu(head->FAT_next_block);
			f_data->priv = priv;
			f_data->last_match_blk = 1;
			priv->max = FAT_blocks * ((1<<shift)/4);
			b_count = (f_data->buf_length > 12) ? 12 : f_data->buf_length ;
			ret = follow_cdf(buf,b_count, &offset, &last_match,f_data->priv);
			ret = analysis_ret1("CDF", f_data, ret , b_count, offset, last_match);
			f_data->scantype |= DATA_NO_FOOT;
			break;
		case 3 :
			offset = f_data->next_offset ;
			ret = follow_cdf(buf, f_data->buf_length, &offset, &last_match,f_data->priv);
			ret = analysis_ret2("CDF", f_data, ret, offset, last_match);
		break;
		case 4 :
				if (f_data->priv)
					free(f_data->priv);
				ret = 1;
		break;
			
	}
	return ret;
}
//----END CDF--------------------------------------------------------------------



//vmware    FIXME ????????????????
int file_vmware(unsigned char *buf, int *size, __u32 scan , int flag, struct found_data_t* f_data){
	int 		ret = 0;

	switch (flag){
		case 0 :
			if (f_data->scantype & DATA_METABLOCK){   //FIXME 
					*size = current_fs->blocksize;
					ret = 1;
			}
			break;
		case 1:
			return (f_data->scantype & DATA_METABLOCK) ? 0 : 1 ;
			break;
	}
	return ret;
}

//ext2fs
int file_ext2fs(unsigned char *buf, int *size, __u32 scan , int flag, struct found_data_t* f_data){
	int 		ret = 0;
	__u32		tmp,i;
	__u64		lsize,ssize;
	int		blksize = 1024;
	int		blkcount;
	

	switch (flag){
		case 0: 
			if (f_data->size || f_data->h_size) {
				if (f_data->scantype & DATA_METABLOCK){
					f_data->inode->i_size_high = f_data->h_size;
					f_data->inode->i_size = f_data->size;
					*size = current_fs->blocksize;
					ret = 1;
				}
				else { 
					lsize = (__u64)f_data->h_size << 32;
					lsize += f_data->size;
					ssize = (__u64)f_data->inode->i_size_high << 32;
					ssize += f_data->inode->i_size;
					if (lsize > ssize)
						ret = 0;
					else{
						f_data->inode->i_size = f_data->size;
						*size = current_fs->blocksize;
						ret = 1;
					}
				}
			}
		break;
		case 1:
			return (f_data->scantype & DATA_METABLOCK) ? 0 : 1 ;
		break;
		case 2:
			tmp = (buf[1051]<<24)+(buf[1050]<<16)+(buf[1049]<<8)+buf[1048];
			if (((f_data->buf_length * current_fs->blocksize) < 2048) || (buf[1080] != 0x53) || (buf[1081] != 0xEF) || (tmp > 2)){
				f_data->func = NULL;
			}
			else{
				for ( i=0 ;i < tmp; i++)
					blksize *= 2;
				blkcount = (buf[1031]<<24)+(buf[1030]<<16)+(buf[1029]<<8)+buf[1028];
				lsize =(__u64) blkcount * blksize;
				f_data->size = lsize & 0xFFFFFFFF;
				f_data->h_size = lsize >> 32;
				f_data->scantype = DATA_LENGTH;
				ret =1;
			}
		break;
	}
	return ret;
}


//LUKS   FIXME ???????????????
int file_luks(unsigned char *buf, int *size, __u32 scan , int flag, struct found_data_t* f_data){
#define LUKSMAGIC 		6
#define LUKSCIPHERNAME	 	32
#define LUKSCIPHERMODEL 	32
#define LUKSDIGESTSIZE		20
#define LUKSSALTSIZE		32
#define LUKSHASHSPEC		32
#define UUIDSTRING		40
#define LUKSNUMKEYS		8
	struct luks_phdr {
		char magic [LUKSMAGIC] ;
		__u16		 version ;
		char		 cipherName[LUKSCIPHERNAME] ;
		char		 cipherMode [LUKSCIPHERMODEL] ;
		char		 hashSpec [LUKSHASHSPEC] ;
		__u32		 payloadOffset ;
		__u32		 keyBytes ;
		char		 mkDigest [LUKSDIGESTSIZE] ;
		char		 mkDigestSalt [LUKSSALTSIZE] ;
		__u32		 mkDigestIterations ;
		char		 uuid [UUIDSTRING] ;
		struct {
			__u32		 active ;
			__u32		 passwordIterations ;
			char		 passwordSalt [LUKSSALTSIZE] ;
			__u32		 keyMaterialOffset ;
			__u32		 stripes ;
		} keyblock [LUKSNUMKEYS] ;
	};
	unsigned char luksmagic[] = {'L','U','K','S',0xBA,0xBE};
	struct luks_phdr 	*phdr = (struct luks_phdr*)buf;
	int 			ret = 0;	
	switch (flag){
		case 0 :
			if(f_data->scantype & DATA_METABLOCK){
				ret = 1;
			}
			else{
				if ((f_data->inode->i_size_high) || (f_data->inode->i_size > f_data->size))
				ret = 8; // ext4 recover nothing 
					// There is no size and no footer for find the end of file
			}
			*size = current_fs->blocksize;
			break;
		case 1 :return (scan & (M_IS_META | M_CLASS_1 )) ? 0 :1 ;
			break;
		case 2 :
			if (!(strncmp((char*)buf,(char*)luksmagic,6))){
				f_data->size = ext2fs_be32_to_cpu(phdr->payloadOffset) * 512;	
				f_data->scantype = DATA_MIN_LENGTH ;
				ret = 1;
			}
			else{
				f_data->func = NULL;
			}
		}
	return ret;
}


//dbf
int file_dbf(unsigned char *buf, int *size, __u32 scan , int flag, struct found_data_t* f_data){
	int 			ret = 0;
	__u32			ssize;
	__u32			b_size;
	__u16			head,count;
	__u32			*p32;
	__u16			*p16;

	switch (flag){
		case 0 :
			if ((f_data->size ) && (f_data->inode->i_size >= f_data->size)){
				ssize = ((f_data->size-1) % current_fs->blocksize)+1;
				if (f_data->inode->i_size > (f_data->size - ssize)){
					*size = ssize;
					ret =1;
				}
			}
			else{
				if (*size < (current_fs->blocksize -128)){
					*size += 4;
					ret = 4;
				}
			}
			break;
		case 1:
			return (scan & (M_IS_META | M_CLASS_1 | M_TXT)) ? 0 :1 ;
			break;
		
		case 2:
			p32 = (__u32*) (buf +4);
			p16 = (__u16*) (buf +8);
			b_size = ext2fs_le32_to_cpu(*p32);
			head = ext2fs_le16_to_cpu(*p16);
			p16++;
			count = ext2fs_le16_to_cpu(*p16);
			//FIXME for DB7 http://www.dbase.com/KnowledgeBase/int/db7_file_fmt.htm
			if( (buf[1]<100)|| (buf[1]>130)|| (!buf[2]) || (!buf[3]) || (buf[2]>12) || (buf[3] >31) || (!b_size) ||
			   (head < 65) || (head & 0x1e) || (!count) || (b_size > 0x8000) || buf[12] || buf[13] || buf[30] || buf[31]){
				f_data->first = 0;
				break;
			} 
			else{
				f_data->size = (count * b_size) + head + 1;
				f_data->scantype = DATA_LENGTH;
				ret = 1;
			}
			break;
	}
	return ret;
}


//ESRI
int file_esri(unsigned char *buf, int *size, __u32 scan , int flag, struct found_data_t* f_data){
	int 			ret = 0;
	__u32			ssize;
	__u32			head;
	int			i;
	__u32			*p32;

	switch (flag){
		case 0 :
			if ((f_data->size ) && (f_data->inode->i_size >= f_data->size)){
				ssize = ((f_data->size-1) % current_fs->blocksize)+1;
				if (f_data->inode->i_size > (f_data->size - ssize)){
					*size = ssize;
					ret =1;
				}
			}
			else{
				if (*size < (current_fs->blocksize -128)){
					*size += 4;
					ret = 4;
				}
			}
			break;
		case 1:
			return (scan & (M_IS_META | M_CLASS_1 | M_TXT)) ? 0 :1 ;
			break;
		
		case 2:
			p32 = (__u32*) buf;
			head = ext2fs_be32_to_cpu(*p32);
			for (i=4;i<24;i++)
				if (buf[i]) break;
			if( (i<24) || (head != 9994)){
				f_data->first = 0;
				break;
			} 
			else{	
				p32 = (__u32*) (buf +24);
				f_data->size = 2 * ext2fs_be32_to_cpu(*p32);
				f_data->scantype = DATA_LENGTH;
				ret = 1;
			}
			break;
	}
	return ret;
}


//SQLite
int file_SQLite(unsigned char *buf, int *size, __u32 scan , int flag, struct found_data_t* f_data){
	int 		ret = 0;
	__u16		*p_16;
	__u16		b_size;
	__u32		count;
	__u64		ssize;

	switch (flag){
		case 0 :
			if ((f_data->size ) || (f_data->h_size)) {
				if (f_data->h_size != f_data->inode->i_size_high)
					ret = 0;
				else{
					if (f_data->inode->i_size < f_data->size)
						ret = 0;
					else{
						count = ((f_data->size-1) % current_fs->blocksize)+1;
						*size = (count)? count : current_fs->blocksize;
						ret = 1;
					}
				}
			}
			break;
		case 1:
			return (scan & (M_IS_META | M_CLASS_1)) ? 0 :1 ;
			break;
		
		case 2:
			p_16 = (__u16*)(buf+16);
			b_size = (__u32)ext2fs_be16_to_cpu(*p_16);
			if (b_size ==1)
				b_size = 1;
			else {
				if((b_size <512)||(b_size > 32768) || (b_size &1))
					b_size = 0;
			}
			count=(buf[28]<<24)+(buf[29]<<16)+(buf[30]<<8)+(buf[31]);
			if ((count) && (b_size) && (buf[23] == 32)){
				ssize = ((__u64)count) * ((b_size == 1)?65536:b_size);
				f_data->size = ssize & 0xffffffff;
				f_data->h_size	= ssize >>32;
				f_data->scantype = DATA_LENGTH ;
				ret = 1;
			}
			else
				f_data->first = 0;
		break;
	}
	return ret;
}


//smp    (Sample Vision Format)
int file_smp(unsigned char *buf, int *size, __u32 scan , int flag, struct found_data_t* f_data){
static const unsigned char smp_header[18]= "SOUND SAMPLE DATA ";
	int 			ret = 0;
	__u32			ssize;

	switch (flag){
		case 0 :
			if (f_data->size ) {
				ssize = ((f_data->size-1) % current_fs->blocksize)+1;
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
			if(memcmp(buf,smp_header,18)==0){
				ssize = buf[112]+(buf[113]<<8)+(buf[114]<<16)+(buf[115]<<24);
				ssize *= 2;
				ssize += 331; // 116+2+(8*11)+(8*14)+1+12
				f_data->size = ssize;
				f_data->scantype = DATA_LENGTH;
				ret =1;
			}
			else
				f_data->first = 0;
		break;
	}
	return ret;
}



//avr (Audio Visual Research)
int file_avr(unsigned char *buf, int *size, __u32 scan , int flag, struct found_data_t* f_data){
static const unsigned char avr_header[4]= {'2','B','I','T'};
	int 			ret = 0;
	__u32			ssize;
	__u64			wsize;
	__u16			rez = (buf[14]<<8) + buf[15];
	
	switch (flag){
		case 0 :
			if (f_data->size || f_data->h_size) {
				if (f_data->h_size != f_data->inode->i_size_high)
					ret = 0;
				else{
					if (f_data->inode->i_size < f_data->size)
						ret = 0;
					else{
						ssize = ((f_data->size-1) % current_fs->blocksize)+1;
						 if (*size < ssize) 
							*size = ssize;
						ret = 1;
					}
				}
			}
		break;
		case 1:
			return (scan & (M_IS_META | M_CLASS_1 | M_TXT)) ? 0 :1 ;
		break;
		case 2:
			if((memcmp(buf,avr_header,4)==0) && ((rez == 8)||(rez ==16))){
				wsize = (((__u64)buf[30])<<56)+(((__u64)buf[31])<<48)+(((__u64)buf[32])<<40)+
					(((__u64)buf[33])<<32)+(((__u64)buf[34])<<24)+(((__u64)buf[35])<<16)+
					(((__u64)buf[36])<<8)+buf[37];
				wsize = wsize * (rez/8);
				f_data->size = wsize & 0xFFFFFFFF;
				f_data->h_size = wsize >> 32;
				f_data->scantype = DATA_LENGTH;
				ret =1;
			}
			else
				f_data->first = 0;
		break;
	}
	return ret;
}


//au (NeXT)
int file_au(unsigned char *buf, int *size, __u32 scan , int flag, struct found_data_t* f_data){
//static const unsigned char au_header[4]= {'.','s','n','d'};
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
				ssize = ((f_data->size-1) % current_fs->blocksize)+1;
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
					f_data->scantype = DATA_LENGTH;
				}
				ret = 1;
			}
			else
				f_data->first = 0;
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
				ssize = ((f_data->size-1) % current_fs->blocksize)+1;
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
					f_data->scantype = DATA_LENGTH;
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


static int follow_qt(unsigned char *buf, __u16 blockcount, __u32 *offset, __u32 *last_match, struct found_data_t* f_data){
	static unsigned char atom[][4]={
	{'c','m','o','v'},	{'c','m','v','d'},	{'d','c','o','m'},	{'f','r','e','e'},
	{'f','t','y','p'},	{'j','p','2','h'},	{'j','p','2','c'},	{'m','d','a','t'},
	{'m','d','i','a'},	{'m','o','o','v'},	{'P','I','C','T'},	{'p','n','o','t'},
	{'s','k','i','p'},	{'s','t','b','l'},	{'r','m','r','a'},	{'m','e','t','a'},
	{'i','d','s','c'},	{'i','d','a','t'},	{'t','r','a','k'},	{'u','u','i','d'},
	{'w','i','d','e'},
//MP4 atom 
	{'I','D','3','2'},	{'a','l','b','m'},	{'a','u','t','h'},	{'b','p','c','c'},
	{'b','u','f','f'},	{'b','x','m','l'},	{'c','c','i','d'},	{'c','d','e','f'},
	{'c','l','s','f'},	{'c','m','a','p'},	{'c','o','6','4'},	{'c','o','l','r'},
	{'c','p','r','t'},	{'c','r','h','d'},	{'c','s','l','g'},	{'c','t','t','s'},
	{'c','v','r','u'},	{'d','c','f','D'},	{'d','i','n','f'},	{'d','r','e','f'},
	{'d','s','c','p'},	{'d','s','g','d'},	{'d','s','t','g'},	{'e','d','t','s'},
	{'e','l','s','t'},	{'f','e','c','i'},	{'f','e','c','r'},	{'f','i','i','n'},
	{'f','i','r','e'},	{'f','p','a','r'},	{'f','r','m','a'},	{'g','i','t','n'},
	{'g','n','r','e'},	{'g','r','p','i'},	{'h','d','l','r'},	{'h','m','h','d'},
	{'h','p','i','x'},	{'i','c','n','u'},	{'i','h','d','r'},	{'i','i','n','f'},
	{'i','l','o','c'},	{'i','m','i','f'},	{'i','n','f','u'},	{'i','o','d','s'},
	{'i','p','h','d'},	{'i','p','m','c'},	{'i','p','r','o'},	{'i','r','e','f'},
	{'j','P',' ',' '},	{'j','p','2','i'},	{'k','y','w','d'},	{'l','o','c','i'},
	{'l','r','c','u'},	{'m','7','h','d'},	{'m','d','h','d'},	{'m','d','r','i'},
	{'m','e','c','o'},	{'m','e','h','d'},	{'m','e','r','e'},	{'m','f','h','d'},
	{'m','f','r','a'},	{'m','f','r','o'},	{'m','i','n','f'},	{'m','j','h','d'},
	{'m','o','o','f'},	{'m','v','c','g'},	{'m','v','c','i'},	{'m','v','e','x'},
	{'m','v','h','d'},	{'m','v','r','a'},	{'n','m','h','d'},	{'o','c','h','d'},
	{'o','d','a','f'},	{'o','d','d','a'},	{'o','d','h','d'},	{'o','d','h','e'},
	{'o','d','r','b'},	{'o','d','r','m'},	{'o','d','t','t'},	{'o','h','d','r'},
	{'p','a','d','b'},	{'p','a','e','n'},	{'p','c','l','r'},	{'p','d','i','n'},
	{'p','e','r','f'},	{'p','i','t','m'},	{'r','e','s',' '},	{'r','e','s','c'},
	{'r','e','s','d'},	{'r','t','n','g'},	{'s','b','g','p'},	{'s','c','h','i'},
	{'s','c','h','m'},	{'s','d','e','p'},	{'s','d','h','d'},	{'s','d','t','p'},
	{'s','d','v','p'},	{'s','e','g','r'},	{'s','g','p','d'},	{'s','i','d','x'},
	{'s','i','n','f'},	{'s','m','h','d'},	{'s','r','m','b'},	{'s','r','m','c'},
	{'s','r','p','p'},	{'s','t','c','o'},	{'s','t','d','p'},	{'s','t','s','c'},
	{'s','t','s','d'},	{'s','t','s','h'},	{'s','t','s','s'},	{'s','t','s','z'},
	{'s','t','t','s'},	{'s','t','y','p'},	{'s','t','z','2'},	{'s','u','b','s'},
	{'s','w','t','c'},	{'t','f','a','d'},	{'t','f','h','d'},	{'t','f','m','a'},
	{'t','f','r','a'},	{'t','i','b','r'},	{'t','i','r','i'},	{'t','i','t','l'},
	{'t','k','h','d'},	{'t','r','a','f'},	{'t','r','e','f'},	{'t','r','e','x'},
	{'t','r','g','r'},	{'t','r','u','n'},	{'t','s','e','l'},	{'u','d','t','a'},
	{'u','i','n','f'},	{'U','I','T','S'},	{'u','l','s','t'},	{'u','r','l',' '},
	{'v','m','h','d'},	{'v','w','d','i'},	{'x','m','l',' '},	{'y','r','r','c'}};


	int 		atom_db_len = sizeof(atom) / 4;
	int 		i,ret = 1 ;
	__u32		qt_offset = *offset;
	__u32		atom_size;

while ((ret ==1) && (qt_offset < ((blockcount * current_fs->blocksize)-8))){
	atom_size=(buf[qt_offset+0]<<24)+(buf[qt_offset+1]<<16)+(buf[qt_offset+2]<<8)+buf[qt_offset+3];
	for (i=0;i<atom_db_len;i++){
		if(memcmp((void*)(buf + qt_offset +4), atom[i], 4))
			continue;
//		if ((!atom_size ) && (i == 6)){
		if (i == 6){ 
			f_data->func = file_jp2;
#ifdef DEBUG_QUICK_TIME
			fprintf(stderr,"QuickTime : found JP-2000, block %lu ; offset size :%llu\n",f_data->first,
				 ((__u64)f_data->h_size<<32) + f_data->size + qt_offset + 8);
#endif
			*offset = qt_offset +8;
			return 1;
		}
		if (atom_size == 1){
			f_data->size   = (buf[qt_offset+12]<<24)+(buf[qt_offset+13]<<16)+(buf[qt_offset+14]<<8)+buf[qt_offset+15] + qt_offset;
			f_data->h_size = (buf[qt_offset+8]<<24)+(buf[qt_offset+9]<<16)+(buf[qt_offset+10]<<8)+buf[qt_offset+11];
#ifdef DEBUG_QUICK_TIME
			fprintf(stderr,"QuickTime : found a large atom, block %lu ; offset size :%llu\n",f_data->first,
				 ((__u64)f_data->h_size<<32) + f_data->size + qt_offset);
#endif
			f_data->scantype = DATA_MIN_LENGTH ;
			return 1;
		}
		*last_match = qt_offset;
		qt_offset += atom_size;
#ifdef DEBUG_QUICK_TIME
			fprintf(stderr,"QuickTime : atom \"%c%c%c%c\" ; size %lu\n", atom[i][0],atom[i][1],atom[i][2],atom[i][3],qt_offset);
#endif
		break;
	}
	if (i == atom_db_len){
			if (((!atom_size) && (!buf[qt_offset+4])) || (zero_space(buf, qt_offset)))
				ret = 2;
			else{
#ifdef DEBUG_QUICK_TIME
			
				fprintf(stderr,"QuickTime : atom \"%c%c%c%c\" unknown ; block  %lu ; offset %lu , size %lu\n", 
					buf[qt_offset+4],buf[qt_offset+5],buf[qt_offset+6],buf[qt_offset+7], f_data->first, qt_offset, atom_size);
#endif
				ret = 0;
			}
	}	
}
#ifdef DEBUG_QUICK_TIME
	fprintf(stderr,"QuickTime : found ;  block  %lu ; offset %lu\n", f_data->first, qt_offset);
#endif
	*offset = qt_offset;
	return ret;
}

//--------------------------------


//QuickTime  
int file_qt(unsigned char *buf, int *size, __u32 scan , int flag, struct found_data_t* f_data){
	int 			i, j, ret = 0;
	__u32			atom_size;
	__u32			offset;
	__u32			last_match = 0;
	__u16			b_count;

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



static unsigned char ftype[12][3]={
{'i','s','o'},
{'m','p','4'},
{'m','m','p'},
{'M','4','A'},
{'M','4','B'},
{'M','4','C'},
{'3','g','p'},
{'3','g','2'},
{'j','p','2'},
{'m','j','p'},
{'q','t',' '},
{'a','v','s'}};

switch (flag){
		case 0 :
			if (f_data->size || f_data->h_size) {
				if ((f_data->h_size == 0xffff )&& (!f_data->size)){
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
			return ((scan & (M_IS_META | M_CLASS_1 | M_BLANK | M_TXT))||(f_data->scantype & DATA_READY)) ? 0 :1 ;
			break;
		case 2 :

offset = 0;
atom_size=(buf[offset+0]<<24)+(buf[offset+1]<<16)+(buf[offset+2]<<8)+buf[offset+3];
for (i=0;i<18;i++){
	if(memcmp((void*)(buf + offset +4), basic[i], 4))
		continue;
	f_data->last_match_blk = 1;
	if((!atom_size) && (i == 1)){ //FIXME
		f_data->h_size = 0xffff;
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
		f_data->scantype = DATA_MIN_LENGTH ;
		return 1;
	}
	if(!i){
		for (j=0;j<12;j++){
			if(memcmp((void*)(buf + offset +8), ftype[j], 3))
			continue;
#ifdef DEBUG_QUICK_TIME
			fprintf(stderr,"QuickTime : Type \"%c%c%c\"\n",ftype[j][0],ftype[j][1],ftype[j][2]);
#endif
			break;
		//FIXME
		}
		if (j == 12){
#ifdef DEBUG_QUICK_TIME
			fprintf(stderr,"QuickTime : Type : \"%c%c%c%c\" is unknown\n",buf[offset +4],buf[offset +5],buf[offset+6],buf[offset+7]);
#endif
			f_data->func = file_none;
			return 0;
		}
	}	
	offset += atom_size;
	break;
}
for (j=0;(j<12)&&(isprint(buf[j]));j++){};
if ((i == 18)||(j==12)||((i==7)&&(atom_size != 8)) || ((i==7)&&(atom_size != 8)) || ((!i)&&(atom_size % 4)))  {
#ifdef DEBUG_QUICK_TIME
	fprintf(stderr,"QuickTime : first Container atom unknown ; block %lu ; size %lu\n",f_data->first, atom_size);
#endif
	f_data->func = NULL;
	return 0;
}
		b_count = (f_data->buf_length > 12) ? 12 : f_data->buf_length ;
		ret = follow_qt(buf, b_count, &offset, &last_match,f_data);
		ret = analysis_ret1("QT", f_data, ret , b_count, offset, last_match);
	break;
	case 3 :
			offset = f_data->next_offset ;
			ret = follow_qt(buf, f_data->buf_length, &offset, &last_match,f_data);
			ret = analysis_ret2("QT", f_data, ret, offset, last_match);
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
				ssize = ((f_data->size-1) % current_fs->blocksize)+1;
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
			if ((!(buf[4] & 0xec)) &&(buf[5]== 0xaf)){
				f_data->size = (buf[3]<<24) + (buf[2]<<16) + (buf[1]<<8) + buf[0] ;
				f_data->scantype = DATA_LENGTH ;
				ret =  1;
			}
			else 
				f_data->func = file_none;
			break;
	}
	return ret;
}

//file_ecryptfs
int file_ecryptfs(unsigned char *buf, int *size, __u32 scan , int flag, struct found_data_t* f_data){
	int 		ret = 0;
	__u32		blz,count;
	__u64		lsize,ssize;
	

	switch (flag){
		case 0: 
			if (f_data->size || f_data->h_size) {
				if (f_data->scantype & DATA_METABLOCK){
					f_data->inode->i_size_high = f_data->h_size;
					f_data->inode->i_size = f_data->size;
					*size = current_fs->blocksize;
					ret = 1;
				}
				else { 
					lsize = (__u64)f_data->h_size << 32;
					lsize += f_data->size;
					ssize = (__u64)f_data->inode->i_size_high << 32;
					ssize += f_data->inode->i_size;
					if (lsize > ssize)
						ret = 0;
					else{
						f_data->inode->i_size = f_data->size;
						*size = current_fs->blocksize;
						ret = 1;
					}
				}
			}
		break;
		case 1:
			return (scan & (M_IS_META | M_CLASS_1 | M_BLANK | M_TXT)) ? 0 : 1 ;
		break;
		case 2:
			lsize = ((__u64)buf[0]<<56)+((__u64)buf[1]<<48)+((__u64)buf[2]<<40)+((__u64)buf[3]<<32)+
			        (buf[4]<<24)+(buf[5]<<16)+(buf[6]<<8)+buf[7];
			blz = (buf[20]<<24)+(buf[21]<<16)+(buf[22]<<8)+buf[23];
			count = (buf[24]<<8)+buf[25];
			if (!(blz & 0x3ff)){
		//		if (!lsize) lsize++;
				lsize += (blz *count);

				f_data->size = lsize & 0xFFFFFFFF;
				f_data->size = (f_data->size + (blz-1)) & (~(blz-1)); 
				f_data->h_size = lsize >> 32;
				f_data->scantype = DATA_LENGTH;
				ret =1;
			}
		break;
	}
	return ret;
}





static int follow_ac3(unsigned char *buf, __u16 blockcount, __u32 *offset, __u32 *last_match,  int flag){
	static const __u16	tab[38][3] = {
	{ 64 , 69 , 96 },      { 64 , 70 , 96 },      { 80 , 87 , 120 },     { 80 , 88 , 120 },
	{ 96 , 104 , 144 },    { 96 , 105 , 144 },    { 112 , 121 , 168 },   { 112 , 122 , 168 },
	{ 128 , 139 , 192 },   { 128 , 140 , 192 },   { 160 , 174 , 240 },   { 160 , 175 , 240 },
	{ 192 , 208 , 288 },   { 192 , 209 , 288 },   { 224 , 243 , 336 },   { 224 , 244 , 336 },
	{ 256 , 278 , 384 },   { 256 , 279 , 384 },   { 320 , 348 , 480 },   { 320 , 349 , 480 },
	{ 384 , 417 , 576 },   { 384 , 418 , 576 },   { 448 , 487 , 672 },   { 448 , 488 , 672 },
	{ 512 , 557 , 768 },   { 512 , 558 , 768 },   { 640 , 696 , 960 },   { 640 , 697 , 960 },
	{ 768 , 835 , 1152 },  { 768 , 836 , 1152 },  { 896 , 975 , 1344 },  { 896 , 976 , 1344 },
	{ 1024 , 1114 , 1536 },{ 1024 , 1115 , 1536 },{ 1152 , 1253 , 1728 },{ 1152 , 1254 , 1728 },
	{ 1280 , 1393 , 1920 },{ 1280 , 1394 , 1920 }};

	int 			ret = 1;
	unsigned char		*ac3_h;
	__u16			fscod, frmsizecod ;
	__u32			frame_offset = *offset ;
	__u32			frame_size ;
	while ((ret == 1) && (frame_offset < (blockcount * current_fs->blocksize)-5)){
		ac3_h = (buf + frame_offset);
		if (((!ac3_h[0]) && (!ac3_h[1]) && (!ac3_h[2]) && (!ac3_h[3]) && (!ac3_h[4]) && ac3_h[-1]) && (zero_space(buf, frame_offset))){
			ret = 2;
			continue;
		}		
		if ((ac3_h[0] == 0x0b) && (ac3_h[1] == 0x77)){
			fscod = (ac3_h[4] & 0xc0 ) >> 6 ;
			frmsizecod = ac3_h[4] & 0x3f ;
			if (((fscod > 2) || (frmsizecod > 0x25)) || ((flag) && (flag != ac3_h[4]))) { 
				ret = 0;
				break;
			}
			frame_size = tab[frmsizecod][fscod] * 2;
			*last_match = frame_offset;
			frame_offset += frame_size;
		}
		else {
			if (frame_offset % (current_fs->blocksize))
				ret = 0;
			else 
				ret = 2;
		}		
	}
	*offset = frame_offset;
	return ret;
}

//ac3  
int file_ac3(unsigned char *buf, int *size, __u32 scan , int flag, struct found_data_t* f_data){
	
	int 			ret = 0;
	__u32			b_count;	
	__u32			offset = 0;
	__u32			last_match = 0;

	
	switch (flag){
		case 0 :
			if ((f_data->size) && (f_data->size <= f_data->inode->i_size)){
				if(f_data->scantype & DATA_READY){
					*size = (f_data->next_offset)?f_data->next_offset : *size;
					ret = 1;
					break;
				} //FIXME
				if((!(f_data->inode->i_flags & EXT4_EXTENTS_FL))&&(f_data->size < (12 * current_fs->blocksize))){
					f_data->inode->i_size = (f_data->size + current_fs->blocksize -1) & ~(current_fs->blocksize-1);
					*size = f_data->size % current_fs->blocksize;
					ret = 1;
				}
			}
			else {
				ret =0;
			}

			break;
		case 1 : 
			return ((scan & (M_IS_META | M_CLASS_1 | M_BLANK | M_TXT))||(f_data->scantype & DATA_READY)) ? 0 :1 ;
		break;
		
		case 2 :
			offset = test_id3_tag(buf);
			b_count = (f_data->buf_length > 12) ? 12 : f_data->buf_length ;
			ret = follow_ac3(buf, b_count, &offset, &last_match,0);
			ret = analysis_ret1("AC3", f_data, ret , b_count, offset, last_match);
			if (ret == 1){
				f_data->scantype |= DATA_NO_FOOT ;
				f_data->type = buf[4];
			}
			break;
		case 3 :
			offset = f_data->next_offset ;
			ret = follow_ac3(buf, f_data->buf_length, &offset, &last_match, f_data->type);
			ret = analysis_ret2("AC3", f_data, ret, offset, last_match);
		break;
	}
	return ret;
}



static int follow_ogg(unsigned char *buf, __u16 blockcount, __u32 *offset, __u32 *last_match,  int flag){
	int 			i, ret = 1;
	int			last =  0;
	unsigned char		*ogg_h;
	__u8			*t_pointer;	
	__u32			frame_offset = *offset;
	__u32			seq_len;

	while ((ret == 1) && (frame_offset < (blockcount * current_fs->blocksize)-27)){
		ogg_h = (buf + frame_offset);
		
		seq_len = 27;
		i = 0;
		t_pointer = (__u8*) (buf + frame_offset + seq_len);
			
		if ((ogg_h[0] == 0x4f)&&(ogg_h[1] == 0x67)&&(ogg_h[2] == 0x67) &&(ogg_h[3] == 0x53)){
			if ( ogg_h[5] & 0x4)
				last = 1;
			for (i = 0; i < ogg_h[26] ;i++, t_pointer++){
				if ((frame_offset + 27 + i) >= (blockcount * current_fs->blocksize))
					break;
				seq_len += *t_pointer;
			}				
		}
		else {
			if (last)
				ret = 2;
			else {			
				if (ret && (ogg_h[0] || ogg_h[1] ||ogg_h[2] ||ogg_h[3])) 
					ret = 0;
				else {
					if (zero_space(buf, frame_offset))
						ret = 2;
				}
			}
			break;
		}
		*last_match = frame_offset;	
		frame_offset += seq_len + ogg_h[26];
		if (last && (!frame_offset % current_fs->blocksize))
			ret = 2 ;
#ifdef DEBUG_OGG_STREAM
		fprintf(stderr,"OGG-STREAM:  serial number %12lu : size %6lu : offset %6lu \n",
		 (ogg_h[14] | (ogg_h[15]<<8) | (ogg_h[16]<<16) | (ogg_h[17]<<24)), seq_len , frame_offset); 
#endif	
	}
	*offset = frame_offset;
	return ret;
}


//ogg  
int file_ogg(unsigned char *buf, int *size, __u32 scan , int flag, struct found_data_t* f_data){
	
	int 			ret = 0;
	unsigned char 		token[2][7]= {{0x80, 't', 'h', 'e', 'o', 'r', 'a'},
					      {0x01, 'v', 'i', 'd' ,'e', 'o', 0x00}};
	unsigned char		*ogg_h;
	__u32			b_count;	
	__u32			offset = 0;
	__u32			last_match = 0;

	
	switch (flag){
		case 0 :
			if ((f_data->size) && (f_data->size <= f_data->inode->i_size)){
				if(f_data->scantype & DATA_READY){
					*size = (f_data->next_offset)?f_data->next_offset : *size;
					ret = 1;
					break;
				}//FIXME
				if((!(f_data->inode->i_flags & EXT4_EXTENTS_FL))&&(f_data->size < (12 * current_fs->blocksize))){
					f_data->inode->i_size = (f_data->size + current_fs->blocksize -1) & ~(current_fs->blocksize-1);
					*size = f_data->size % current_fs->blocksize;
					ret = 1;
				}
				else {
					*size +=2;
					ret = 2;
				}
			}
			else {
				ret =0;
			}
		break;
		case 1 : 
			return ((scan & (M_IS_META | M_CLASS_1 | M_BLANK | M_TXT))||(f_data->scantype & DATA_READY)) ? 0 :1 ;
		break;
		
		case 2 :
			offset = test_id3_tag(buf);
			ogg_h = (buf + offset) ;
			if (!(ogg_h[5] & 0x02)){
#ifdef DEBUG
				fprintf(stderr,"OGG : Block %8lu is sequence %lu and not begin of a file\n",f_data->first, 
					(ogg_h[18] | (ogg_h[19]<<8) | (ogg_h[20]<<16) | (ogg_h[21]<<24))); 
#endif
				f_data->func = file_none;
				return 0;
			}
			if((!(memcmp((void*)(buf + offset +28), token[0], 7))) ||
				(!(memcmp((void*)(buf + offset +28), token[1], 7))))
				f_data->name[strlen(f_data->name)-1] = 'm' ;

			b_count = (f_data->buf_length > 12) ? 12 : f_data->buf_length ;
			ret = follow_ogg(buf, b_count, &offset, &last_match,0);
			ret = analysis_ret1("OGG", f_data, ret , b_count, offset, last_match);
			break;
		case 3 :
			offset = f_data->next_offset ;
			ret = follow_ogg(buf, f_data->buf_length, &offset, &last_match,0);
			ret = analysis_ret2("OGG", f_data, ret, offset, last_match);
		break;
	}
	return ret;
}


static int follow_mp3(unsigned char *buf, __u16 blockcount, __u32 *offset, int *flag, __u32 *last_match, unsigned char* head){
	#define MPEG_V25        0
	#define MPEG_V2         2
	#define MPEG_V1         3
	#define MPEG_L3 	0x01
	#define MPEG_L2 	0x02
	#define MPEG_L1 	0x03
	int	ret = 1;
	__u32	frame_offset = *offset ;
	int	frame_flag = *flag;
	int 	mpeg_version;
	int 	mpeg_layer;
	int 	bit_rate_key;
	int 	sampling_rate_key;
	int 	padding;
	int 	bit_rate;
	int 	sample_rate;
	__u32 	frameLength = 0;	
	
	
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
	
	while (frame_offset < (blockcount * current_fs->blocksize)-2){
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
#ifdef DEBUG_MAGIC_MP3_STREAM
			fprintf(stderr,"MP3-STREAM %8u-> framesize: %u, layer: %u, bitrate: %u, padding: %u\n", 
					frame_flag, frameLength, 4-mpeg_layer, bit_rate, padding);
#endif
			if( ! frameLength ){
				return 0;
			}
			
			if (head && (! frame_flag)  && (!frame_offset)) {
				head[0] = buf[frame_offset];
				head[1] = buf[frame_offset +1];
				head[2] = buf[frame_offset +2];
				head[3] = (unsigned char)((frameLength-padding) >>8);
				head[4] = (unsigned char)((frameLength-padding) & 0xff);
			}
			*last_match = frame_offset;
			frame_offset += frameLength;
			frame_flag++;
		}
		else{
			if((buf[frame_offset + 0] =='T') && (buf[frame_offset + 1] == 'A') && (buf[frame_offset + 2] == 'G')){
				frame_offset += 128 ;
#ifdef DEBUG_MAGIC_MP3_STREAM
				fprintf (stderr,"\nMP3-TAG-END : offset %lu : last_frame %lu\n", frame_offset, frameLength);
#endif
				*last_match = frame_offset;
				ret= 2;
				break;
			}
			if((!(frame_offset % current_fs->blocksize)) || ( ! ( buf[frame_offset + 0]) && ( ! ( buf[frame_offset + 1]) )&& (zero_space(buf, frame_offset)))){
#ifdef DEBUG_MAGIC_MP3_STREAM
				fprintf (stderr,"\nMP3-END : offset %lu : last_frame %lu\n", frame_offset, frameLength);
#endif
				ret = 2;
				break;
			}
			else {
#ifdef DEBUG_MAGIC_MP3_STREAM
				fprintf (stderr,"MP3-STREAM_END : offset %lu : last_frame %lu\n", frame_offset, frameLength);
				blockhex(stderr,(void*)(buf+frame_offset),0,64);
#endif
				ret = 0;
				break;
			}
		}	
	}
	if (frame_offset >= (blockcount * current_fs->blocksize)-2) 
		ret = 1;
	else{
		if ((ret != 2) && (frame_flag) && (frame_offset % current_fs->blocksize))
			ret = 3;
	}
	*offset = frame_offset;
	*flag   = frame_flag;
return ret;
}


//mp3
int file_mp3(unsigned char *buf, int *size, __u32 scan , int flag, struct found_data_t* f_data){
int	ret = 0;
__u32	offset = 0 ;
__u32	last_match = 0;
int	frame_flag = 0;
int 	b_count, tag_flag = 0;
unsigned char head[5]={0,0,0,0,0};

switch (flag){
		case 0 :
			if ((f_data->size) && (f_data->size <= f_data->inode->i_size)){
				if(f_data->scantype & DATA_READY){
					*size = (f_data->next_offset)?f_data->next_offset : *size;
					ret = 1;
					break;
				}//FIXME
				if((!(f_data->inode->i_flags & EXT4_EXTENTS_FL))&&(f_data->size < (12 * current_fs->blocksize))){
					f_data->inode->i_size = (f_data->size + current_fs->blocksize -1) & ~(current_fs->blocksize-1);
					*size = f_data->size % current_fs->blocksize;
					ret = 1;
				}
			}
			else {
				ret =0;
			}
			break;
		case 1 :
			return ((scan & (M_IS_META | M_CLASS_1 |  M_BLANK ))||(f_data->scantype & DATA_READY)) ? 0 :1 ;
			break;
		case 2 :

	offset	= test_id3_tag(buf);
	if (offset) {
		tag_flag++;
	}
	b_count = (f_data->buf_length > 12) ? 12 : f_data->buf_length ;
	ret = follow_mp3(buf, b_count, &offset, &frame_flag, &last_match, head);
	if (last_match)
		f_data->last_match_blk = (last_match/current_fs->blocksize) +1 ;
	if (ret){
		f_data->scantype = DATA_CARVING + DATA_NO_FOOT;
		f_data->size = offset;
		if (offset > (b_count * current_fs->blocksize))
			f_data->buf_length = b_count;
		else
			f_data->buf_length = offset / current_fs->blocksize;
		f_data->next_offset = offset - (f_data->buf_length * current_fs->blocksize);
		switch (ret){
		case 3 :
#ifdef DEBUG_MAGIC_MP3_STREAM
			fprintf(stderr,"MP3-CHECK: Block %lu : mp3-data block found, to short for recover\n",f_data->first);
#endif	 
			f_data->scantype |= DATA_BREAK;
			f_data->func = NULL;
			return 0;
		break;
		
		case 1 :
		break;
		case 2 :
			if (offset){
				f_data->scantype |= DATA_READY;
				if(offset % current_fs->blocksize)(f_data->buf_length)++;
			}
			else {
				f_data->func = NULL;
				return 0;
			}
		break;
		}
		if ((frame_flag > 2) &&( ! tag_flag)){
			if (head[0]){
				__u16		 reverse = (__u16)(head[3]<<8) + head[4];
				__u32 		 b_size = current_fs->blocksize;
				unsigned char	 *v_buf = buf - b_size; 
				if (reverse < (b_size -1)){
					if (((v_buf[b_size-reverse] == head[0]) && (v_buf[b_size-reverse +1 ] == head[1]) && (v_buf[b_size-reverse+2] == (head[2] & ~0x2))) ||
					((v_buf[b_size-reverse-1] == head[0]) && (v_buf[b_size-reverse] == (head[1])) && (v_buf[b_size-reverse+1] == (head[2] | 0x2)))){
#ifdef DEBUG_MAGIC_MP3_STREAM
						fprintf(stderr,"MP3-CHECK: Block %lu : is mp3-data but not begin of file\n", f_data->first);
						blockhex(stderr,(void*)(v_buf+b_size-reverse-16),0,64);
#endif
						f_data->func = NULL;
					}
				}
			}
			ret = 1;
		}
	} 
	  else{
		f_data->func = NULL;
	}	
	break;	
	
	case 3:
		offset = f_data->next_offset ;
		ret = follow_mp3(buf, f_data->buf_length , &offset, &frame_flag, &last_match, NULL);
		if (last_match)
			f_data->last_match_blk = ((f_data->size - f_data->next_offset + last_match)/current_fs->blocksize) +1 ;
		if (ret){
			f_data->size += offset - f_data->next_offset;
			f_data->buf_length = offset / current_fs->blocksize;
			f_data->next_offset = offset - (f_data->buf_length * current_fs->blocksize);
			switch (ret){
			case 3 :
#ifdef DEBUG_MAGIC_MP3_STREAM
				fprintf(stderr,"MP3-CHECK: mp3-data found, recover not all\n");
#endif
				f_data->scantype |= DATA_BREAK;	
			break;

			case 1 :
#ifdef DEBUG_MAGIC_MP3_STREAM
//				fprintf(stderr,"MP3-CHECK: mp3-data next found\n");
#endif				
			break;

			case 2 :
#ifdef DEBUG_MAGIC_MP3_STREAM
				fprintf(stderr,"MP3-CHECK: mp3-data end found, recover \n");
#endif
				f_data->scantype |= DATA_READY;
				if(offset % current_fs->blocksize) (f_data->buf_length)++;
			break;
			}
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
	              this->func = file_CDF ;
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
		strncat(this->name,".pgp",7);
		break;
	
		case 0x0109     :               //pgp-keys
	//              this->func = file_pgp-keys ;
		strncat(this->name,".key",7);
		break;
	
		case 0x010a     :               //pgp-signature
	//              this->func = file_pgp-signature ;
		strncat(this->name,".pgp",7);
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
	              this->func = file_none ;
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
			this->func = file_archive ;
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
		this->func = file_bzip2 ;
		strncat(this->name,".bzip2",7);
		break;
	
		case 0x011d     :               //x-compress
	              this->func = file_lzw ;
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
	              this->func = file_dbf ;
	              strncat(this->name,".dbf",7);
		break;
	
		case 0x0121     :               //x-dbm
	//              this->func = file_x-dbm ;
	              strncat(this->name,".dbm",7);
		break;
	
		case 0x0122     :               //x-debian-package
	              this->func =  file_archive;
	              strncat(this->name,".deb",7);
		break;
	
		case 0x0123     :               //x-dosexec
	//              this->func = file_x-dosexec ;
		strncat(this->name,".exe",7);
		break;
	
		case 0x0124     :               //x-dvi
	             this->func = file_dvi ;
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
	              this->func = file_txt ;
	              strncat(this->name,".xml",7);
		break;
	
		case 0x012a     :               //x-gnumeric
	              this->func = file_gnumeric ;
	              strncat(this->name,".gnm",7);
		break;
	
		case 0x012b     :               //x-gnupg-keyring   deaktiv
	//              this->func = file_x-gnupg-keyring ;
	              strncat(this->name,".key",7);
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
		this->func = file_binary ;
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
	              this->func = file_rpm ;
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
	             this->func = file_tfm ;
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

		case 0x0156     : //x-7z-compressed
	              this->func = file_7z ;
	              strncat(this->name,".7z",7);
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
	              this->func = file_iff ;
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
	              this->func = file_creative_voice;
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
		      this->func = file_jp2000;
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
	              this->func = file_djvu ;
	              strncat(this->name,".djvu",7);
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

		case 0x0317     :               //x-xcf
	              this->func = file_xcf;
	              strncat(this->name,".xcf",7);
		break;

	
	//----------------------------------------------------------------
	//Messages
		case 0x0401     :               //news
	              this->func = file_txt ;
	//	strncat(this->name,".news",7);
		break;
	
		case 0x0402     :               //rfc822
	              this->func = file_smtp ;
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
		this->func = file_html ;
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
		this->func = chk_troff;
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
		this->func =  file_bin_txt;
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
	              strncat(this->name,".pas",7);
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

		case 0x061e     :               //x-python
	              this->func = file_txt ;
	              strncat(this->name,".py",7);
		break;
		
		case 0x061f     :               //x-java
	              this->func = file_txt ;
	              strncat(this->name,".jar",7);
		break;

		case 0x0620     :               //PEM
	              this->func = file_txt ;
	              strncat(this->name,".pem",7);
		break;

		case 0x0621     :               //SGML
	              this->func = file_txt ;
	              strncat(this->name,".rc",7);
		break;

		case 0x0622     :               //libtool
	              this->func = file_txt ;
	              strncat(this->name,".la",7);
		break;

		case 0x0623     :               //M3U
	              this->func = file_txt ;
	              strncat(this->name,".m3u",7);
		break;

		case 0x0624     :               //tcl
	              this->func = file_txt ;
	              strncat(this->name,".tcl",7);
		break;

		case 0x0625     :               //POD
	              this->func = file_txt ;
	              strncat(this->name,".pod",7);
		break;

		case 0x0626     :               //PPD
	              this->func = file_txt ;
	              strncat(this->name,".ppd",7);
		break;

		case 0x0627     :               //configure
	              this->func = file_txt ;
	              strncat(this->name,".conf",7);
		break;

		case 0x0628     :               //ruby
	              this->func = file_txt ;
	              strncat(this->name,".rb",7);
		break;

		case 0x0629     :               //sed
	              this->func = file_txt ;
	              strncat(this->name,".sed",7);
		break;

		case 0x062a     :               //expect
	              this->func = file_txt ;
//	              strncat(this->name,".expect",8);
		break;

		case 0x062b     :               //ssh
	              this->func = file_txt ;
	              strncat(this->name,".key",7);
		break;

		case 0x062c     :               //text (for all unknown)
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
	              this->func = file_png ;
		strncat(this->name,".jng",7);
		break;
	
		case 0x070e     :               //x-mng
	              this->func = file_png ;
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
	              this->func = file_mkv ;
	              strncat(this->name,".mkv",7);
		break;
	
		case 0x0714     :               //webm
	              this->func = file_mkv ;
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
	
		case 0x080a	:		//ext2
		    this->func = file_ext2fs ;
		    strncat(this->name,".ext2",7);
		break;

		case 0x080b	:		//ext3
		    this->func = file_ext2fs ;
		    strncat(this->name,".ext3",7);
		break;

		case 0x080c	:		//ext4
		    this->func = file_ext2fs ;
		    strncat(this->name,".ext4",7);
		break;

		case 0x080d     :               //Image
	//              this->func = file_Image ;
	//              strncat(this->name,".Image",7);
		break;
	
		case 0x080e     :               //Composite
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
			this->func = file_jp2000;
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
	              this->func = file_ac3 ;
	              strncat(this->name,".ac3",7);
		break;
		
		case 0x081b     :              // ScreamTracker III
	              this->func = file_mod ;
	              strncat(this->name,".s3m",7);
		break;
		
		case 0x081c     :              // EBML matroska 
	             this->func = file_mkv ;
	              strncat(this->name,".mkv",7);
		break;
		
		case 0x081d     :              // LZMA 
	             this->func = file_lzma ;
	              strncat(this->name,".lzma",7);
		break;

		case 0x081e	:		//Audio Visual Research
		    this->func = file_avr ;
		    strncat(this->name,".avr",7);
		break;
		
		case 0x081f	:		//Sample Vision Format
		    this->func = file_smp ;
		    strncat(this->name,".smp",7);
		break;
		
		case 0x0820	:		//ISO Media (mp4)
		    this->func = file_qt ;
		    strncat(this->name,".mp4",7);
		break;

		case 0x0821     :               //Linux
	//              this->func = file_Linux ;
	//              strncat(this->name,".Linux",7);
		break;
	
		case 0x0822     :               //filesystem
	//              this->func = file_filesystem ;
		strncat(this->name,".iso",7);
		break;
	
		case 0x0823     :               //x86
	//              this->func = file_x86 ;
		strncat(this->name,".iso",7);
		break;

		case 0x0824     :               //LUKS
	              this->func = file_luks ;
		strncat(this->name,".luks",7);
		break;

		case 0x0825     :               //python (binary)
	              this->func = file_bin_raw ;
		strncat(this->name,".pyc",7);
		break;

		case 0x0826     :               //ESRI Shapefile
	              this->func = file_esri ;
		strncat(this->name,".shp",7);
		break;

		case 0x0827     :               //CDF
	              this->func = file_CDF ;
	              strncat(this->name,".doc",7);
		break;

		case 0x0828     :               //ecryptfs
	              this->func = file_ecryptfs ;
	              strncat(this->name,".ecrypt",8);
		break;
	//----------------------------------------------------------------
		default:
			this->func = file_default ;

	}
	if (!this->func)
		this->func = file_default ;	
}





