/***************************************************************************
 *   Copyright (C) 2010 by Roberto Maar   *
 *   robi@users.berlios.de   *
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
#ifndef UTIL_H
#define UTIL_H


//status for control flag list-functions
#define LONG_OPT        0x0001
#define DELETED_OPT     0x0002
#define PARSE_OPT       0x0004
#define ONLY_JOURNAL	0x0008
#define DELETED_DIR	0x0010
#define SKIP_HTREE	0x0020

// control flags for recover- and listmodus
#define DOUPLE_QUOTES_LIST	0x0100
#define LOST_DIR_SEARCH	0x0400
#define LIST_ALL	0x1000
#define LIST_STATUS	0x2000
#define RECOV_DEL	0x4000
#define RECOV_ALL	0x8000
#define HIST_DIR	0x0800
#define REC_FILTER	0xF800
#define REC_DIR_NEEDED  (RECOV_ALL | RECOV_DEL)

//Definitions to magic scan functions
#define  M_SIZE		0x00003FFF
#define  M_BLANK	0x00010000
#define  M_DATA		0x00020000
#define  M_TXT		0x00040000
#define	 M_IMAGE	0x00080000
#define  M_AUDIO	0x00100000
#define  M_VIDEO	0x00200000
#define  M_APPLI	0x00400000
#define  M_MESSAGE	0x00800000
#define  M_MODEL	0x00004000
#define  M_TAR		0x00008000
#define  M_IS_FILE	0x00FC4000
#define  M_IS_DATA	0x00030000

#define  M_BINARY	0x01000000

#define	 M_CLASS_1	0x02000000
#define	 M_CLASS_2	0x04000000
#define  M_IS_CLASS	0x06000000

#define  M_ARCHIV	0x08000000

#define  M_ACL		0x80000000
#define  M_EXT4_META	0x40000000
#define  M_EXT3_META	0x20000000
#define  M_DIR		0x10000000
#define  M_IS_META	0xF0000000


/* Definitions to allow ext4magic compilation with old e2fsprogs */
#ifndef EXT4_EXTENTS_FL
#define EXT4_EXTENTS_FL                 0x00080000 /* Inode uses extents */
#endif


#include "ring_buf.h"
#include "dir_list.h"

//struct for iterate directory
struct priv_dir_iterate_struct {
	// char *name;
	//	char *path;
	__u32 time_stamp;
	//	 ext2_ino_t *found_inode;
     	//	 int     col;
        int     		options;
	struct dir_list_head_t 	*dir_list;
//FIXME;
	trans_range_t 		*transaction;
};

//struct for inode position 
struct inode_pos_struct {
	blk_t  block;
	int	offset;
	int	size;
};

//struct for a simple inodenumberlist
struct inode_nr_collect{
	 ext2_ino_t count;
	 ext2_ino_t *list;
};


//struct for generic bitmap
/* 
struct ext2fs_struct_loc64_generic_bitmap {
        errcode_t               magic;
        ext2_filsys             fs;
        struct ext2_bitmap_ops  *bitmap_ops;
        int                     flags;
        __u64                   start, end;
        __u64                   real_end;
        int                     cluster_bits;
        char                    *description;
        void                    *private;
        errcode_t               base_error_code;
};*/

struct ext2fs_struct_loc_generic_bitmap {
        errcode_t       magic;
        ext2_filsys     fs;
        __u32           start, end;
        __u32           real_end;
        char    *       description;
        unsigned char   *bitmap;
        errcode_t       base_error_code;
        __u32           reserved[7];
};



//struct for collect data for magic_scan recover
struct found_data_t{
blk_t				first;
blk_t				last;
blk_t				leng;
__u32				size;
__u32				h_size;
int (*func)(unsigned char*, int*, __u32, int, struct found_data_t*);
__u32				scan;
__u32				type;
char				*scan_result;
char				*name;
struct ext2_inode_large		*inode;
};



#define ALLOC_SIZE 1024
extern ext2fs_inode_bitmap 		imap ;
extern ext2fs_block_bitmap 	  	bmap ; 

// public functions util.c
void read_all_inode_time(ext2_filsys , __u32 , __u32 , int ); //analyse an print histogram
void print_coll_list(__u32, __u32, int); //print the history of collectlist
void add_coll_list(ext2_ino_t );// add inodenumber in a collectlist
void blockhex (FILE* , void*, int , int); //hexdump
char get_inode_mode_type( __u16); //get filetype of inode
//public helper functions util.c
char *time_to_string(__u32);
int check_fs_open(char*);
void reset_getopt(void);
unsigned long parse_ulong(const char* , const char* , const char* , int* );


// public functions lookup_local.c
void list_dir(ext2_ino_t inode); //list dir (using normal functions from ext2fs)
void list_dir2(ext2_ino_t, struct ext2_inode*); //list dir (search in journal ; not automatical use the real inode from fs)
void list_dir3(ext2_ino_t, struct ext2_inode*, trans_range_t* ,__u32 ); //list (search over journal; both inode as well journaldirblocks) 
struct dir_list_head_t* get_dir3(struct ext2_inode*,ext2_ino_t, ext2_ino_t,char*, char*,__u32,__u32,int ); //directory finder
void lookup_local(char*, struct dir_list_head_t*, __u32 , __u32 , int ); // main worker function for recover and list
ext2_ino_t local_namei(struct dir_list_head_t*, char* , __u32, __u32, int);// search the Inode for an pathname (use journal data)


//public functions recover.c
void recover_list(char*, char*,__u32, __u32, int); // recover files from a "double quotes" listfile
int recover_file( char* ,char* , char* , struct ext2_inode* , ext2_ino_t, int); //recover all filetypes
int check_file_stat(struct ext2_inode*); //check inode; return true if blocks not allocated and not recovered
int check_file_recover(struct ext2_inode*); // return percentage of not allocated blocks
void set_dir_attributes(char* ,char* ,struct ext2_inode*); //set owner,file mode bits an timestamps for directory
int check_dir(char*);//check if the target directory existent


//public functions imap_search.c
void imap_search(char* , __u32, __u32 ,int ); // search inode by imap (step1 + step2)
int check_find_dir(char*, ext2_ino_t, char*, char*); //check if the directory always recovert; then move



//public function file_type.c
int ident_file(struct found_data_t*, __u32*, char*, char*); // index of the files corresponding magic result strings
void get_file_property(struct found_data_t*); //set the file properties and the extension



//public functions magic_block_scan.c
struct found_data_t* free_file_data(struct found_data_t*); //clear + free found_data
int magic_block_scan3(char*, __u32);//main of the magic_scan_engine for ext3

//functions in develop
int magic_block_scan4(char*, __u32);//main of the magic_scan_engine for ext4

#endif
