/***************************************************************************
 *   Copyright (C) 2010 by Roberto Maar   *
 *   robi@users.berlios.de  *
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
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <time.h>

#ifndef HAVE_ERRNO_H
#include <errno.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <utime.h>

#include <blkid/blkid.h>
#include <ext2fs/ext2fs.h>
#include "jfs_user.h"
#include <uuid/uuid.h>

#include "ext4magic.h"
#include "util.h"
#include "journal.h"
#include "inode.h"

enum journal_location {JOURNAL_IS_INTERNAL, JOURNAL_IS_EXTERNAL, JOURNAL_IS_DUMMY};

//flags for journaldescriptor control
#define ANY_BLOCK ((blk_t) -1)
#define WRAP_UNDEF 0
#define WRAP_ON    1
#define WRAP_OFF   2
#define WRAP_READY 3


struct journal_source
{
	enum journal_location where;
	int fd;
	ext2_file_t file;
	char* j_dummy;
}; 


//static variable for work with journal
struct journal_source 			journal_source;
extern ext2_filsys     			current_fs ;
extern time_t				now_time ;
struct ext2_super_block 		*jsb_pointer;   //pointer for find & open journal
char					jsb_buffer[1024];  //buffer for journal-superblock (be_to_CPU)
void*					pt_buff;  /*pointer of the privat descriptor Table */
journal_descriptor_tag_t		*pt;	/*pointer for descriptor Table*/
__u32					*ptl;   /*reverse pointer for lost inodeblocks*/
__u32					pt_count; /*counter for privat descriptor Table */
__u32					ptl_count; /*counter for lost inodeblocks*/
struct j_bitmap_list_t			jbbm  ;  /* for Block Bitmaps of Journal */


//print journal superblock
void dump_journal_superblock( void)
{
  int i;
  __u32 nr_users;
  char buffer[40];
  uuid_t *uu;
  journal_superblock_t *jsb = (journal_superblock_t*)jsb_buffer;


  fprintf(stdout,"\nJournal Super Block:\n");
  fprintf(stdout," Signature: 0x%08x \n",jsb->s_header.h_magic);
  fprintf(stdout," Blocktype : %s \n",type_to_name(jsb->s_header.h_blocktype));

  fprintf(stdout," Journal block size: %lu \n",(long unsigned int)jsb->s_blocksize);
  fprintf(stdout," Number of journal blocks: %lu \n",(long unsigned int)jsb->s_maxlen);
  fprintf(stdout," Journal block where the journal actually starts: %lu\n",(long unsigned int)jsb->s_first);
  fprintf(stdout," Sequence number of first transaction: %lu\n", (long unsigned int)jsb->s_sequence);
  fprintf(stdout," Journal block of first transaction: %lu\n", (long unsigned int)jsb->s_start);
  fprintf(stdout," Error number: %ld\n",(long int)jsb->s_errno);
  if ((jsb->s_header.h_blocktype) != JFS_SUPERBLOCK_V2)
    return ;
// Remaining fields are only valid in a version-2 superblock

//FIXME: Strings of Features 
  fprintf(stdout," Compatible Features: %lu\n",(long unsigned int)jsb->s_feature_compat);
  fprintf(stdout," Incompatible features: %lu\n",(long unsigned int)jsb->s_feature_incompat);
  fprintf(stdout," Read only compatible features: %lu\n",(long unsigned int)jsb->s_feature_ro_compat);

  uuid_unparse(jsb->s_uuid, buffer);
  fprintf(stdout," Journal UUID: %s \n",buffer);
  
  nr_users = jsb->s_nr_users;
  fprintf(stdout," Number of file systems using journal: %lu\n", (long unsigned int)jsb->s_nr_users);
  
  fprintf(stdout," Location of superblock copy: %lu\n",(long unsigned int)jsb->s_dynsuper);
  fprintf(stdout," Max journal blocks per transaction: %lu\n",(long unsigned int)jsb->s_max_transaction);
  fprintf(stdout," Max file system blocks per transaction: %lu\n",(long unsigned int)jsb->s_max_trans_data);
 
  if (nr_users && (nr_users < 48)) {
	fprintf(stdout," IDs of all file systems using the journal:\n");
	for (i = 0; i < nr_users; i++){
		uu=(void*) &jsb->s_users[i*16];
		uuid_unparse(*uu, buffer);
		fprintf(stdout,"%02d. : %s\n",i+1,buffer);
	}
  }
   fprintf(stdout," \n\n");  
  return;
}


// local subfunction
static void journal_superblock_to_cpu ( __u32 *jsb )
{
 int i;
 for (i=0;i<12;i++)
	*(jsb+i) = ext2fs_be32_to_cpu(*(jsb+i));
// UUIDs are endian-independent, so don't swap those bytes
 for (i=16;i<20;i++)
	*(jsb+i) = ext2fs_be32_to_cpu(*(jsb+i));
 // User IDs are endian-independent
}


// open an extract the blocklist from journal 
extern int journal_open(  char *journal_file_name, int journal_backup_flag )
{
	char		*journal_dev_name = NULL;
	int		journal_fd = 0;
        ext2_ino_t	journal_inum;
	struct ext2_inode journal_inode;
	int		retval;
	ext2_file_t 	journal_file;
#ifdef EXPERT_MODE
	char		*dummy_journal = NULL;
#endif
	
	pt_buff = NULL;
	ptl = NULL;
	if (current_fs) jsb_pointer = current_fs->super;
	else {
         fprintf(stderr,"No filesystem open, this must be a bug\n");
         return(JOURNAL_ERROR);
        }

	if (journal_file_name) {
		/* Set up to read journal from a regular file somewhere */
		journal_fd = open(journal_file_name, O_RDONLY, 0);
		if (journal_fd < 0) {
			fprintf(stderr,"Error %d while opening %s for read external journal\n",errno,journal_file_name);
			goto errout;
		}

		journal_source.where = JOURNAL_IS_EXTERNAL;
		journal_source.fd = journal_fd;
		journal_source.file = NULL;
		journal_source.j_dummy = NULL;
		printf("Using external Journal at File %s \n",journal_file_name);

	} else if ((journal_inum = jsb_pointer->s_journal_inum)) {
		if (journal_backup_flag) {
			if (jsb_pointer->s_jnl_backup_type != EXT3_JNL_BACKUP_BLOCKS) {
				fprintf(stderr,"no journal backup in super block\n");
				goto errout;
			}
			memset(&journal_inode, 0, sizeof(struct ext2_inode));
			memcpy(&journal_inode.i_block[0], jsb_pointer->s_jnl_blocks, EXT2_N_BLOCKS*4);
			journal_inode.i_size = jsb_pointer->s_jnl_blocks[16];
			journal_inode.i_links_count = 1;
			journal_inode.i_mode = LINUX_S_IFREG | 0600;	
			if (jsb_pointer->s_feature_incompat & EXT3_FEATURE_INCOMPAT_EXTENTS)
				journal_inode.i_flags |= EXT4_EXTENTS_FL;
		} else {
			if (intern_read_inode(journal_inum, &journal_inode))
				goto errout;
		}

		retval = ext2fs_file_open2(current_fs, journal_inum,
					   &journal_inode, 0, &journal_file);
#ifdef EXPERT_MODE
		if (journal_backup_flag){
			//check the Journal Magic Number
			char*			jsb_buffer =  NULL;
			unsigned int 		got;
			journal_superblock_t*	jsb;	

			jsb_buffer = malloc(1024);
			journal_source.where = JOURNAL_IS_INTERNAL;
			journal_source.file = journal_file;
			if ((jsb_buffer) && (! read_journal_block(0,jsb_buffer, 1024, &got))){
				jsb = (journal_superblock_t *) jsb_buffer;
				if ((ext2fs_be32_to_cpu(jsb->s_header.h_magic) != JFS_MAGIC_NUMBER) ||
					 (current_fs->blocksize != ext2fs_be32_to_cpu(jsb->s_blocksize))) {
					retval =  1;
				}
				free(jsb_buffer);
			}
			else return JOURNAL_ERROR ;
		}
#endif
		if (retval) {
			fprintf(stderr," Error %d while opening journal\n",retval);
#ifdef EXPERT_MODE
			if (journal_backup_flag) {
				journal_superblock_t    *jsb;

//the journal is defect, create a insubstantial blank journal
				dummy_journal = (char*) malloc(current_fs->blocksize * 2);
				if (dummy_journal){
					memset (dummy_journal,0,current_fs->blocksize *2);
					jsb = (journal_superblock_t*) dummy_journal;
 					jsb->s_header.h_magic = ext2fs_cpu_to_be32(JFS_MAGIC_NUMBER);
			                jsb->s_header.h_blocktype = ext2fs_cpu_to_be32(JFS_SUPERBLOCK_V2);
        				jsb->s_blocksize = ext2fs_cpu_to_be32(current_fs->blocksize);
        				jsb->s_maxlen = ext2fs_cpu_to_be32(2);
        				jsb->s_nr_users = ext2fs_cpu_to_be32(1);
        				jsb->s_first = ext2fs_cpu_to_be32(1);
        				jsb->s_sequence = ext2fs_cpu_to_be32(1);
        				memcpy(jsb->s_uuid, current_fs->super->s_uuid, sizeof(current_fs->super->s_uuid));
					journal_source.where = JOURNAL_IS_DUMMY;
					journal_source.file = NULL;
					journal_source.fd = -1 ;
					journal_source.j_dummy = dummy_journal;
					printf("Journal defect, using a dummy Journal\n");
				}else 
					goto errout;
			}
			else
#endif
			goto errout;
		}
		else {
			journal_source.where = JOURNAL_IS_INTERNAL;
			journal_source.file = journal_file;
			journal_source.fd = -1 ;
			journal_source.j_dummy = NULL;
			printf("Using %s internal Journal at Inode %d\n",(journal_backup_flag)?"a recovered":"",journal_inum);
		}

	} else {
		char uuid[37];

		uuid_unparse(jsb_pointer->s_journal_uuid, uuid);
		journal_dev_name = blkid_get_devname(NULL, "UUID", uuid);
		if (!journal_dev_name)
				journal_dev_name = blkid_devno_to_devname(jsb_pointer->s_journal_dev);
		if (!journal_dev_name) {
			fprintf(stderr,"filesystem has no journal\n");
			goto errout;
		}
		journal_fd = open(journal_dev_name, O_RDONLY, 0);
		if (journal_fd < 0) {
			fprintf(stderr,"%d while opening %s for external journal\n",errno,journal_dev_name);
			free(journal_dev_name);
			goto errout;
		}
		printf("Using external journal found at %s\n", journal_dev_name);
		free(journal_dev_name);
		journal_source.where = JOURNAL_IS_EXTERNAL;
		journal_source.fd = journal_fd;
		journal_source.file = NULL;
	}
	return init_journal();

errout:
	fprintf(stderr,"Error: while open journal\n" );
        jsb_pointer = NULL;
	return (JOURNAL_CLOSE);
}


// close the journal (last function in main() if the journal open)
extern int journal_close(void)
{
	jsb_pointer = NULL;
	pt_count = 0;
	if ( pt_buff ) { 
		free(pt_buff);
		pt_buff = NULL;
	}
	switch (journal_source.where){
	case JOURNAL_IS_DUMMY : 
		if (journal_source.j_dummy){
			free(journal_source.j_dummy);
			journal_source.j_dummy = NULL;
		}
	break;
	case JOURNAL_IS_INTERNAL :
		if (journal_source.file){ 	
			ext2fs_file_close(journal_source.file);
        		journal_source.file = NULL;
          	}
	break;
	case JOURNAL_IS_EXTERNAL :
		if (journal_source.fd > 0) {
			close(journal_source.fd);
			journal_source.fd = -1;
		}
	}
        return (JOURNAL_CLOSE);
}


//print hexdump of a journalblock
int dump_journal_block( __u32 block_nr , int flag){
	char 		*buf = NULL;
	unsigned int 	got;
	int 		retval = 0;
	int 		blocksize = current_fs->blocksize;

	buf = (char*) malloc(blocksize);
	if(! buf) return 1 ;

	retval = read_journal_block(block_nr * blocksize ,buf,blocksize,&got);
	if (retval || got != blocksize)
		return retval;

	blockhex (stdout, buf, flag, blocksize);
	if (buf) 
		free(buf);			
	return 0;
}


//read a journal block
int read_journal_block(off_t offset, char *buf, int size, unsigned int *got)
{
	int retval = 1;
	switch (journal_source.where) {
	case JOURNAL_IS_EXTERNAL :
		if (lseek(journal_source.fd, offset, SEEK_SET) < 0) {
			retval = errno;
			fprintf(stderr,"Error %d while seeking in reading journal",retval);
			return retval;
		}
		retval = read(journal_source.fd, buf, size);
		if (retval >= 0) { 
			*got = retval;
			retval = 0;
		} else 	retval = errno;
		break;
	case JOURNAL_IS_INTERNAL :
		retval = ext2fs_file_lseek(journal_source.file, offset, EXT2_SEEK_SET, NULL);
		if (retval) { 
                         fprintf(stderr,"Error %d while seeking in reading journal",retval);
			return retval;
		}
		retval = ext2fs_file_read(journal_source.file, buf, size, got);
		break;
	case JOURNAL_IS_DUMMY :
			if((offset + size) > (2 *current_fs->blocksize)) retval = -1 ;
			memcpy(buf,journal_source.j_dummy + offset,size);
			*got = (unsigned int) size;
			retval = 0;
		break;
	}		

	if (retval)
                fprintf(stderr,"Error %d while reading journal",retval);
	else if (*got != (unsigned int) size) {
		fprintf(stderr,"short read (read %d, expected %d) while reading journal", *got, size);
		retval = -1;
	}
	return retval;
}



const char *type_to_name(int btype)
{
	switch (btype) {
	case JFS_DESCRIPTOR_BLOCK:
		return "descriptor block";
	case JFS_COMMIT_BLOCK:
		return "commit block";
	case JFS_SUPERBLOCK_V1:
		return "V1 superblock";
	case JFS_SUPERBLOCK_V2:
		return "V2 superblock";
	case JFS_REVOKE_BLOCK:
		return "revoke table";
	}
	return "unrecognised type";
}


//check if journal block is a lost inodeblock local function
static int jb_is_inodetable(char *buf){
	struct ext2_inode 	*inode;
	__u16			mode;
	__u32   		atime;
        __u32   		ctime;
        __u32   		mtime;
        __u32   		dtime;
	int 			i;
	__u32			min = 315601200; // 315601200 = "1980-01-01 20:00:00"
	__u32			max = (__u32) now_time;
	int 			i_size = EXT2_INODE_SIZE(current_fs->super);
	int 			inodes_per_block = (current_fs->blocksize / i_size );
	int			flag = 0;
	

	for (i=0; i<inodes_per_block;i++){
		inode = (struct ext2_inode*) (buf + (i_size * i));
		mode = ext2fs_le16_to_cpu(inode->i_mode);
		atime = ext2fs_le32_to_cpu(inode->i_atime);
		ctime = ext2fs_le32_to_cpu(inode->i_ctime);
		mtime = ext2fs_le32_to_cpu(inode->i_mtime);
		dtime = ext2fs_le32_to_cpu(inode->i_dtime);
		if ((!mode) && (!atime) && (!ctime) && (!mtime) && (!dtime) && (!i))
			return 0;
		if ((!dtime) && (ctime > min) && (ctime < max) && (atime > min) && (atime < max) 
			&& (mtime > min) && (mtime < max) && inode->i_size && inode->i_blocks 
			&& LINUX_S_ISREG(mode) && inode->i_block[0] )
			flag++;
		if (ctime > max)
			return 0;
	}
	return flag;
}


//check if block is a inodeblock local function
static int block_is_inodetable(blk64_t block){
	int group;
	struct ext2_group_desc *gdp;
	blk64_t last;

	last = current_fs->super->s_inodes_count / (current_fs->blocksize / current_fs->super->s_inode_size ) ;
	last -= ((current_fs->group_desc_count - 1) * current_fs->inode_blocks_per_group );
	
//FIXME for struct ext4_group_desc 48/64BIT
	for (group = 0; group < current_fs->group_desc_count; group++){
#ifdef EXT2_FLAG_64BITS
	        gdp = ext2fs_group_desc(current_fs, current_fs->group_desc, group);
#else
        	gdp = &current_fs->group_desc[group];
#endif
		if (block >= (gdp->bg_inode_table + current_fs->inode_blocks_per_group)) 
			continue;
		if (block >= gdp->bg_inode_table){
			if ( group == (current_fs->group_desc_count - 1 )){
				if (block >= gdp->bg_inode_table + last)
					break;
			}
			return 1;
		}
		else
			break;
	}
return 0;
}


// get last timestamp of all inode in this journalblock
static __u32 get_transaction_time( __u32 j_block){
	char *buf;
	struct ext2_inode *inode;
	__u32 		t_time = 0;
	int 		ino, retval = 0;
	unsigned int	got;
	int 		inode_size = EXT2_INODE_SIZE(current_fs->super);
	int 		blocksize = current_fs->blocksize;

	buf = (char*) malloc(blocksize);
	if(! buf) return 0 ;

	retval = read_journal_block(j_block * blocksize ,buf,blocksize,&got);
	if (retval || got != blocksize){
		fprintf(stderr,"Error while read journal block %u\n",(unsigned int)j_block);
		t_time = 0;
		goto errout;
	}
	for (ino = 0; ino < EXT2_INODES_PER_BLOCK(current_fs->super);ino++){
		inode = (struct ext2_inode*) (buf + (inode_size * ino));
		if (t_time < ext2fs_le32_to_cpu(inode->i_ctime))
  			t_time = ext2fs_le32_to_cpu(inode->i_ctime);
		if (t_time < ext2fs_le32_to_cpu(inode->i_atime))
  			t_time = ext2fs_le32_to_cpu(inode->i_atime);
		
	}
	
errout:
if (buf) free(buf);
return t_time;
}



//get the transactiontime of a transactionnumber
__u32 get_trans_time( __u32 transaction){
	journal_descriptor_tag_t *pointer;
	__u32	counter;

	pointer = pt_buff;
	for (counter=0 ; counter<pt_count ; counter++, pointer++) {
		if (pointer->transaction != transaction) continue;

		if (block_is_inodetable(pointer->f_blocknr)) {
			return get_transaction_time(pointer->j_blocknr);
		}
		else continue;
	}
	return 0;
}


//print all usable copy of filesystem blocks are found in journal
void print_block_list(int flag){
	journal_descriptor_tag_t *pointer;
	__u32	counter;
	__u32	transaction_time = 0;
		
	pointer = pt_buff;
	printf("\nFound %d copy of Filesystemblock in Journal\n",pt_count); 
	printf("FS-Block	 Journal	Transact	%s\n",(flag) ? "Time in sec	Time of Transaction" :"" );
	
	for (counter=0 ; counter<pt_count ; counter++) {
		if (flag){
			if (block_is_inodetable (pointer->f_blocknr)){
				transaction_time = get_transaction_time(pointer->j_blocknr);	
			}
			else transaction_time = 0;
		}
		if (transaction_time)
			fprintf(stdout,"%12llu	%8lu	%8lu 	%8lu	%s",(__u64) pointer->f_blocknr , (long unsigned int)pointer->j_blocknr ,
			 (long unsigned int)pointer->transaction, (long unsigned int)transaction_time, time_to_string(transaction_time) );
		else
			fprintf(stdout,"%12llu	%8lu	%8lu\n",(__u64) pointer->f_blocknr , (long unsigned int)pointer->j_blocknr ,
			 (long unsigned int)pointer->transaction);
		pointer++ ;
	}
}


//print all transactions for a fs-block
void print_block_transaction(blk64_t block_nr, int flag){
	journal_descriptor_tag_t *pointer;
	__u32	counter;
	int is_inode_block = 0;
	__u32	transaction_time = 0;

	if (flag) is_inode_block = block_is_inodetable(block_nr);
		
	pointer = pt_buff;
	printf("\nTransactions of Filesystemblock %llu in Journal\n",block_nr); 
	printf("FS-Block	 Journal	Transact	%s\n",(is_inode_block) ? "Time in sec	Time of Transaction" :"");
	for (counter=0 ; counter<pt_count ; counter++) {
		if (pointer->f_blocknr == block_nr){
			if (is_inode_block){
				transaction_time = get_transaction_time(pointer->j_blocknr);
				fprintf(stdout,"%12llu	%8lu	%8lu 	%8lu	%s",(__u64) pointer->f_blocknr ,
					 (long unsigned int)pointer->j_blocknr, (long unsigned int)pointer->transaction,
					 (long unsigned int)transaction_time, time_to_string(transaction_time));	
			}else
			fprintf(stdout,"%12llu	%8lu	%8lu\n",(__u64) pointer->f_blocknr ,
				(long unsigned int)pointer->j_blocknr , (long unsigned int)pointer->transaction);
		}
		pointer++ ;
	}
}



//get the journalblocknumber for a transactionnumber
__u32 get_journal_blocknr(blk64_t block_nr, __u32 trans_nr){
	__u32 journal_nr = 0;
	journal_descriptor_tag_t *pointer;
	int i;
	pointer = pt_buff;
	for (i=0; i<pt_count;i++ , pointer++){
		if ( pointer->transaction != trans_nr) continue;
		if ( pointer->f_blocknr == block_nr ) journal_nr = pointer->j_blocknr ;
	}
return journal_nr;
}


// get the last dir-block for transaction from journal or if not, found the real block
//FIXME: blk64_t ????
int get_last_block(char *buf,  blk64_t *block, __u32 t_start, __u32 t_end){
	int		retval = 0;
	int 		i , count ;
	unsigned int 	got;
	char 		*journal_tag_buf = NULL;
	journal_descriptor_tag_t *block_list;
	blk_t 		j_block = 0;
	int 		blksize = current_fs->blocksize;

	if ((!t_start)  && (!t_end)){
//there is no transaction, it is not from a journal Inode	
#ifdef DEBUG
		printf("DIR_BLOCK (F) %lld\n", (blk64_t)*block);
#endif 
		return io_channel_read_blk(current_fs->io, *block, 1, buf);
	}

        count = get_block_list_count((blk64_t)*block) ;
	
	journal_tag_buf = (void *)malloc((sizeof(journal_descriptor_tag_t) * count));
	if (!journal_tag_buf) {
//		fprintf(stderr,"Error: while allocate Memory for blocklist\n");
		retval = BLOCK_ABORT;
		goto errout;
	}
	block_list = (journal_descriptor_tag_t *) journal_tag_buf;
	count = get_block_list(block_list, *block, count);

	for (i = 0 ; i < count ; i++ , block_list++ ) {
		if  (block_list->transaction < t_start )
			continue;
		if  (block_list->transaction <= t_end){
			j_block = block_list->j_blocknr;
			 if (i != (count -1)) 
				continue;
		}	
		
		retval = read_journal_block((j_block) ? (j_block * blksize) : (block_list->j_blocknr * blksize), 
				 			buf , current_fs->blocksize , &got);
#ifdef DEBUG
		printf("DIR_BLOCK (J) %lld (%ld <-> %ld) JB=%ld : %d\n", (blk64_t)*block,t_start,t_end, j_block, retval );
#endif 
		if (retval) {
			retval = BLOCK_ERROR ;	
		}
		goto errout;		
	}
	
// if not in journal found, use real block
#ifdef DEBUG
		printf("DIR_BLOCK (N) %lld\n", (blk64_t)*block);
#endif 
		retval = io_channel_read_blk(current_fs->io, *block, 1, buf);

	
errout:
	if (journal_tag_buf) free(journal_tag_buf);
	
return retval;
}


 //get count of journal blocklist 
int get_block_list_count(blk64_t block){
	int counter = 0;
	int i;
	journal_descriptor_tag_t *list_pointer = pt_buff ;

	for (i=0;i<pt_count;i++,list_pointer++)
		if (list_pointer->f_blocknr == block) counter++ ;
return counter;
} 


//local sortfunction for journalblocks
static int sort_block_list(journal_descriptor_tag_t *pointer, int counter ){
	journal_descriptor_tag_t p1;
	int c1,c2,flag=1;
	c1 = 0;
//FIXME : for faster sort
	while (flag) {
		flag = 0;
		for (c2 = 0;c2 < (counter -c1 -1);c2++){
			if ((pointer+c2)->transaction > (pointer+c2+1)->transaction ){
				p1=*(pointer+c2);
				*(pointer+c2)=*(pointer+c2+1);
				*(pointer+c2+1)=p1;
				flag = 1 ;
			}
		}
		c1++ ;
	}
return counter;
}


//get a sorted list of all copys of a filesystemblock
int get_block_list(journal_descriptor_tag_t *pointer, blk64_t block , int counter ){
	int i,j = 0;
	journal_descriptor_tag_t *list_pointer = pt_buff ;
	journal_descriptor_tag_t *fill_pointer = pointer ;

	if (counter) {
		for (i=0,j=0;(i<pt_count && j<counter);i++,list_pointer++) 
			if (list_pointer->f_blocknr == block) {
				fill_pointer->f_blocknr = block;
				fill_pointer->j_blocknr = list_pointer->j_blocknr;
				fill_pointer->transaction = list_pointer->transaction;
				fill_pointer++ ;
				j++;
			}
		sort_block_list(pointer,counter);
	
		}
	return j;
}


//extract the journal in the local intern blocklist
static void extract_descriptor_block(char *buf, journal_superblock_t *jsb,
				  unsigned int *blockp, int blocksize,
				  tid_t transaction, unsigned int *wrapflag )
{
	int			offset, tag_size = JBD_TAG_SIZE32;
	char			*tagp;
	journal_block_tag_t	*tag;
	unsigned int		blocknr;
	__u32			tag_block;
	__u32			tag_flags;

	if (jsb->s_feature_incompat & JFS_FEATURE_INCOMPAT_64BIT)
		tag_size = JBD_TAG_SIZE64;

	offset = sizeof(journal_header_t);
	blocknr = *blockp;
#ifdef DEBUG
	fprintf(stdout, "D-Block %u & %u: ", transaction, blocknr);
#endif
	++blocknr;
	 if (blocknr >= jsb->s_maxlen) {
		if (*wrapflag == WRAP_ON) {
 		blocknr -= (jsb->s_maxlen - jsb->s_first);
		*wrapflag = WRAP_READY;
		}
		else {
			*blockp = blocknr;
			return;
		}
	}

	do {
		/* Work out the location of the current tag, and skip to
		 * the next one... */
		tagp = &buf[offset];
		tag = (journal_block_tag_t *) tagp;
		offset += tag_size;

		/* ... and if we have gone too far, then we've reached the
		   end of this block. */
		if (offset > blocksize) break;

		tag_block = ext2fs_be32_to_cpu(tag->t_blocknr) ;
		tag_flags = ext2fs_be32_to_cpu(tag->t_flags);

		if (!(tag_flags & JFS_FLAG_SAME_UUID))
			offset += 16;

#ifdef DEBUG
//		fprintf(stderr, "  FS block %12lu logged at ", tag_block);
//		fprintf(stderr, "sequence %u, ", transaction);
//		fprintf(stderr, "journal block %u (flags 0x%x)\n", blocknr,tag_flags);
		fprintf(stdout,"*");
#endif
		pt->f_blocknr = tag_block ;
		if (tag_size > JBD_TAG_SIZE32) pt->f_blocknr |= (__u64)ext2fs_be32_to_cpu(tag->t_blocknr_high) << 32;
		pt->j_blocknr = blocknr;
		pt->transaction = transaction;
		pt++;
		pt_count++;

		++blocknr;
		if (blocknr >= jsb->s_maxlen) {
			if (*wrapflag == WRAP_ON) {
 				blocknr -= (jsb->s_maxlen - jsb->s_first);
				*wrapflag = WRAP_READY;
			}
			else {
				*blockp = blocknr;
				return;
			}
		}

	} while (!(tag_flags & JFS_FLAG_LAST_TAG));
	*blockp = (*wrapflag == WRAP_READY) ? jsb->s_maxlen : blocknr ;
}



// init and extract the journal in the local private data
int init_journal(void)
{
	struct ext2_super_block *sb;
	char			buf[8192];
	journal_superblock_t	*jsb;
	unsigned int		blocksize = 1024;
	unsigned int		got;
	int			retval;
	__u32			magic, sequence, blocktype;
	journal_header_t	*header;

	tid_t			transaction;
	unsigned int		blocknr = 0;
	unsigned int		maxlen = 0;
	unsigned int		wrapflag;

	/* First, check to see if there's an ext2 superblock header */
	retval = read_journal_block( 0, buf, 2048, &got);
	if (retval) goto errout;
	
	jsb = (journal_superblock_t *) buf;
	sb = (struct ext2_super_block *) (buf+1024);
#ifdef WORDS_BIGENDIAN
	if (sb->s_magic == ext2fs_swab16(EXT2_SUPER_MAGIC)) ext2fs_swap_super(sb);
#endif

	if ((ext2fs_be32_to_cpu(jsb->s_header.h_magic) != JFS_MAGIC_NUMBER) &&
	    (sb->s_magic == EXT2_SUPER_MAGIC) &&
	    (sb->s_feature_incompat & EXT3_FEATURE_INCOMPAT_JOURNAL_DEV)) {
		blocksize = EXT2_BLOCK_SIZE(sb);
		blocknr = (blocksize == 1024) ? 2 : 1;
		uuid_unparse(sb->s_uuid, jsb_buffer);
#ifdef DEBUG
		fprintf(stdout, "Ext2 superblock header found.");
		fprintf(stdout, "\tuuid=%s", jsb_buffer);
		fprintf(stdout, "\tblocksize=%d", blocksize);
		fprintf(stdout, "\tjournal data size %lu\n",(unsigned long) sb->s_blocks_count);
#endif
		}

	/* Next, read the journal superblock */
	retval = read_journal_block(blocknr*blocksize,jsb_buffer, 1024, &got);
	if (retval) goto errout;

	jsb = (journal_superblock_t *) jsb_buffer;
	if (ext2fs_be32_to_cpu(jsb->s_header.h_magic) != JFS_MAGIC_NUMBER) {
		fprintf(stderr, "Journal superblock magic number invalid!\n");
		return JOURNAL_ERROR ;
	}
	journal_superblock_to_cpu((__u32*)jsb_buffer);

	blocksize = jsb->s_blocksize;
	transaction = jsb->s_sequence;
	maxlen =  jsb->s_maxlen;
        blocknr = jsb->s_first;
	wrapflag = WRAP_UNDEF ;

	
	pt_buff = malloc(maxlen * sizeof(journal_descriptor_tag_t));
	pt = pt_buff ;
	pt_count = 0;
	if (pt_buff == NULL) {
		fprintf(stderr,"Error: can't allocate %u Memory\n",maxlen * sizeof(journal_descriptor_tag_t));
		goto errout;
	}
	ptl = (__u32*)(pt_buff + (maxlen * sizeof(journal_descriptor_tag_t)));
	ptl_count = 0;

#ifdef DEBUG
	fprintf(stdout, "Journal starts at block %u, transaction %u\n", blocknr, transaction);	
#endif
	while ( blocknr < maxlen ){
		retval = read_journal_block(blocknr*blocksize, buf, blocksize, &got);
		if (retval || got != blocksize)
			//return JOURNAL_OPEN;
			goto errout;

		header = (journal_header_t *) buf;

		magic = ext2fs_be32_to_cpu(header->h_magic);
		sequence = ext2fs_be32_to_cpu(header->h_sequence);
		blocktype = ext2fs_be32_to_cpu(header->h_blocktype);

		if (magic != JFS_MAGIC_NUMBER) {
#ifdef DEBUG
//			fprintf (stdout, "No magic number at block %u: skip this block\n", blocknr);
			fprintf(stdout,"-");
#endif
			if (jb_is_inodetable(buf)){
//				printf("lost %d\n", blocknr);
				ptl--;
				*ptl = blocknr;
				ptl_count ++;
			}
			if ( !  wrapflag ) wrapflag = WRAP_ON ;
			blocknr++ ;
                  	continue;
		}
#ifdef DEBUG
		fprintf (stdout, "\n%u (%s) T=%u JB=%u\n", blocktype, type_to_name(blocktype), sequence,  blocknr);
#endif
		if ( !  wrapflag ) wrapflag = WRAP_OFF ;

		switch (blocktype) {
		case JFS_DESCRIPTOR_BLOCK:
			extract_descriptor_block(buf, jsb, &blocknr, blocksize, sequence, &wrapflag);
			continue;

		case JFS_COMMIT_BLOCK:
			blocknr++;
			continue;

		case JFS_REVOKE_BLOCK:
			blocknr++;
			continue;

		default:
			fprintf (stderr, "Unexpected block type %u at block %u.\n", blocktype, blocknr);
			return JOURNAL_OPEN;
		}
	}
#ifdef DEBUG
	fprintf(stdout,"\nJournal init complete:  %ld block copy\n",pt_count);
#endif
return JOURNAL_OPEN ;

errout:
	fprintf(stderr,"Error: can't read journal\n"); 
	if ( pt_buff ) { 
		free(pt_buff);
		pt_buff = NULL;
	}
	return JOURNAL_ERROR ;

}



//get a list of all copy of blockbitmap from journal
int get_block_bitmap_list( journal_bitmap_tag_t **bitmap_list){
	int 				count, sum, i, j;
	char*				tag_buf = NULL;
	journal_bitmap_tag_t 		*fill_pointer ;
	journal_descriptor_tag_t 	*list_pointer = pt_buff ;
	
	count = sum = 0;
	for (i = 0; i < pt_count ; i++, list_pointer++ )
		for (j = 0 ; j < current_fs->group_desc_count; j++ )
#ifdef EXT2_FLAG_64BITS
			if( list_pointer->f_blocknr == (blk64_t) ext2fs_block_bitmap_loc(current_fs, j))
#else
			if (list_pointer->f_blocknr == (blk64_t) current_fs->group_desc[j].bg_block_bitmap)
#endif
				 count++ ;

	if (count && (tag_buf = malloc((sizeof(journal_bitmap_tag_t) * count)))){
		fill_pointer = (journal_bitmap_tag_t*)tag_buf ;
		list_pointer = pt_buff ;
		for ( i = 0 ; (i < pt_count && sum < count); i++ , list_pointer++ ) 
			for ( j = 0; j < current_fs->group_desc_count; j++ )
#ifdef EXT2_FLAG_64BITS
                        	if( list_pointer->f_blocknr == (blk64_t) ext2fs_block_bitmap_loc(current_fs, j)){
#else
                        	if (list_pointer->f_blocknr == (blk64_t) current_fs->group_desc[j].bg_block_bitmap){
#endif
					fill_pointer->blockgroup = (blk64_t) j;
					fill_pointer->j_blocknr = list_pointer->j_blocknr;
					fill_pointer->transaction = list_pointer->transaction;
					fill_pointer++ ;
					sum++;
				}

	list_pointer = (journal_descriptor_tag_t*) tag_buf;
	sort_block_list( list_pointer,sum );	
	*bitmap_list = 	(journal_bitmap_tag_t*) tag_buf;
	}
return sum;
}



//create and init the journal block bitmap
//return the count of all blockbitmap copies or 0
 int init_block_bitmap_list(ext2fs_block_bitmap *d_bmap, __u32 t_after){
	__u32	t;  
	jbbm.list = NULL;
	jbbm.block_buf = NULL;

	if (ext2fs_copy_bitmap(current_fs->block_map, d_bmap)){
		fprintf(stderr,"Error: while duplicate bitmap\n");
		return 0;
	}
	ext2fs_clear_block_bitmap(*d_bmap);

	jbbm.count = get_block_bitmap_list( &jbbm.list);
	jbbm.block_buf = malloc(current_fs->blocksize * 2);
	if (! jbbm.block_buf){
		fprintf(stderr,"Error: can't allocate Memory\n");
		return 0;
	}
	if (jbbm.count){
		jbbm.pointer = jbbm.list + jbbm.count - 1;
//		jbbm.last_trans = jbbm.pointer->transaction;
		jbbm.last_trans = 0;
		jbbm.groups = current_fs->group_desc_count;	
	
		while ((jbbm.pointer > jbbm.list) && (get_trans_time(jbbm.pointer->transaction) >= t_after)) {
			t = jbbm.pointer->transaction;
			while (jbbm.pointer->transaction == t) 
				jbbm.pointer-- ;
		}
		jbbm.pointer++;
		jbbm.first_trans = jbbm.pointer->transaction ;
		jbbm.pointer = jbbm.list + jbbm.count - 1;
		jbbm.blocksize = current_fs->blocksize;
		jbbm.blocklen = current_fs->super->s_blocks_per_group >> 3 ;
		jbbm.last_blocklen = (current_fs->super->s_blocks_count >> 3) % jbbm.blocklen;
		if (!jbbm.last_blocklen)
			jbbm.last_blocklen = jbbm.blocklen;
		else
			jbbm.last_blocklen += (current_fs->super->s_blocks_count % 8) ? 1 : 0 ;
	}
return jbbm.count;
}



//destroy the journal block bitmap 
void clear_block_bitmap_list(ext2fs_block_bitmap d_bmap){
	if (jbbm.list) 
		free (jbbm.list);
	if (jbbm.block_buf)
		free (jbbm.block_buf);
	jbbm.list = NULL;
	jbbm.block_buf = NULL;
	ext2fs_free_block_bitmap(d_bmap);
}



//produces a differential block bitmap for a transaction from the Journal
//return 1 if bitmap is differenzial ; an 2 if bitmap is empty
int next_block_bitmap(ext2fs_block_bitmap d_bmap){
	struct ext2fs_struct_loc_generic_bitmap 	*fs_bitmap, *df_bitmap;
	journal_bitmap_tag_t 				*p1;
	journal_bitmap_tag_t 				*p2;
	__u32						blockg, skip,i,len;
	unsigned int					got;
	int						retval;
	char						*diff_buf;

	if (jbbm.pointer->transaction < jbbm.first_trans)
		return 0;

	fs_bitmap = (struct ext2fs_struct_loc_generic_bitmap*) current_fs->block_map;
	df_bitmap = (struct ext2fs_struct_loc_generic_bitmap*) d_bmap;
	ext2fs_clear_block_bitmap(d_bmap);

	if (jbbm.last_trans == 0 ){
// last transaction compared with fs blockbitmap 
		jbbm.last_trans = jbbm.pointer->transaction;
		p2 = jbbm.pointer;
		while ((p2 >= jbbm.list) && (p2->transaction == jbbm.pointer->transaction)){
			blockg = (__u32)p2->blockgroup;
			skip = (jbbm.blocklen * blockg);
			len = ((blockg + 1) == jbbm.groups) ? jbbm.last_blocklen : jbbm.blocklen;

			retval = read_journal_block((__u32)p2->j_blocknr * jbbm.blocksize, jbbm.block_buf, jbbm.blocksize, &got);
			if (retval || got != jbbm.blocksize){
				fprintf(stderr,"Error: while reading journal\n");
				goto errout;
			}

			for (i = 0 ; i < len ; i++){
				*((df_bitmap->bitmap)+skip+i) = *(jbbm.block_buf + i) ^ *((fs_bitmap->bitmap)+skip+i) ;
			}
		p2--;
		}
	}
	else{

//all other transactions compared with previous block copy
		p1 = jbbm.pointer;
		diff_buf = jbbm.block_buf + jbbm.blocksize ;

		while ((p1->transaction >= jbbm.first_trans) && (p1->transaction == jbbm.pointer->transaction)){
			p2 = p1 -1;
			blockg = (__u32)p1->blockgroup;
			skip = (jbbm.blocklen * blockg);
			len = ((blockg + 1) == jbbm.groups) ? jbbm.last_blocklen : jbbm.blocklen;
			while ((p2 > jbbm.list) && ((__u32)p2->blockgroup != blockg))
				p2-- ;
			retval = read_journal_block((__u32)p1->j_blocknr * jbbm.blocksize, jbbm.block_buf, jbbm.blocksize, &got);
			if (retval || got != jbbm.blocksize){
				fprintf(stderr,"Error: while reading journal\n");
				goto errout;
			}
			if ((p2 == jbbm.list) && ((__u32)p2->blockgroup != blockg)){
				//no previous block copy found, create difference to the entire block group
				for (i = 0 ; i < len ; i++){
					*((df_bitmap->bitmap)+skip+i) = *(jbbm.block_buf + i) ^ 0xFF ;
				}
			}
			else{
				retval = read_journal_block((__u32)p2->j_blocknr * jbbm.blocksize, diff_buf, jbbm.blocksize, &got);
				if (retval || got != jbbm.blocksize){
					fprintf(stderr,"Error: while reading journal\n");
					goto errout;
				}
				//previous block copy is found, create difference to this copy
				for (i = 0 ; i < len ; i++){
					*((df_bitmap->bitmap)+skip+i) = *(jbbm.block_buf + i) ^ *(diff_buf + i ) ;
				}
			}
			p1--;	
		}
		jbbm.pointer = p1;
	}

i = 0;
len = (fs_bitmap->end +1) >> 3;
while ((i < len) && (!(*(df_bitmap->bitmap +i))))
		 i++;
return  ( i == len) ? 2 : 1 ;

errout:
	return 0;
}



//read the next journal-lost inode block
int get_pool_block(char *buf){
	int 		retval = 0;
	int		ret  = 0;
	unsigned int 	got ;
	int		blocksize = current_fs->blocksize;

	if (ptl_count){
		retval = read_journal_block(*ptl * blocksize ,(char*)buf,blocksize,&got);
		if ((! retval) && (got == blocksize)){
			ret = 1;
		}
		ptl_count--;
		ptl++;
	}
return ret;
}


