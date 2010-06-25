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

// control flags for recover- and listmodus
#define DOUPLE_QUOTES_LIST	0x0100
#define LIST_ALL	0x1000
#define LIST_STATUS	0x2000
#define RECOV_DEL	0x4000
#define RECOV_ALL	0x8000
#define HIST_DIR	0x0800
#define REC_FILTER	0xF800
#define REC_DIR_NEEDED   RECOV_ALL | RECOV_DEL


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
#define ALLOC_SIZE 1024

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
int check_file_recover(struct ext2_inode*); // return percentage of not allocated blocks
void set_dir_attributes(char* ,char* ,struct ext2_inode*); //set owner,file mode bits an timestamps for directory

#endif
