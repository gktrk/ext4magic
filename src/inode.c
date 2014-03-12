/***************************************************************************
 *   Copyright (C) 2010 by Roberto Maar                                    *
 *   robi6@users.sf.net                                                    *
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
 *   along with this program; if not, see <http://www.gnu.org/licenses/>.  *
 ***************************************************************************/
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <time.h>

#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

#include "inode.h"
#include "ring_buf.h"
#include "extent_db.h"
#include "block.h"

extern ext2_filsys	current_fs;
extern time_t		now_time ;



// read real fs inode 128++
int intern_read_inode_full(ext2_ino_t ino, struct ext2_inode * inode, int bufsize)
{
        int retval;

        retval = ext2fs_read_inode_full(current_fs, ino, inode, bufsize);
        if (retval) {
                fprintf(stderr,"Error %d while reading inode %u\n",retval, ino);
                return 1;
        }
        return 0;
}


// read real fs inode 128
int intern_read_inode(ext2_ino_t ino, struct ext2_inode * inode)
{
        int retval;

        retval = ext2fs_read_inode(current_fs, ino, inode);
        if (retval) {
                fprintf(stderr,"Error %d while reading inode %u\n",retval, ino);
                return 1;
        }
        return 0;
}


#ifdef WORDS_BIGENDIAN
// On my current version of libext2 the extra time fields ar not bigendian corrected
// We want this solved temporarily here with this function
 static void le_to_cpu_swap_extra_time(struct ext2_inode_large *inode, char *inode_buf){
	//inode->i_pad1 = ext2fs_le16_to_cpu(((struct ext2_inode_large *))inode_buf->i_pad1);
	inode->i_ctime_extra = ext2fs_le32_to_cpu(((struct ext2_inode_large *)inode_buf)->i_ctime_extra);
	inode->i_mtime_extra = ext2fs_le32_to_cpu(((struct ext2_inode_large *)inode_buf)->i_mtime_extra );
	inode->i_atime_extra = ext2fs_le32_to_cpu(((struct ext2_inode_large *)inode_buf)->i_atime_extra );
	inode->i_crtime  = ext2fs_le32_to_cpu(((struct ext2_inode_large *)inode_buf)->i_crtime );
	inode->i_crtime_extra  = ext2fs_le32_to_cpu(((struct ext2_inode_large *)inode_buf)->i_crtime_extra );
	//inode->i_version_hi  = ext2fs_le32_to_cpu(((struct ext2_inode_large *)inode_buf)->i_version_hi );
}
#endif

//subfunction for dump_inode_extra
static void dump_xattr_string(FILE *out, const char *str, int len)
{
        int printable = 0;
        int i;

        // check: is string "printable enough?" 
        for (i = 0; i < len; i++)
                if (isprint(str[i]))
                        printable++;

        if (printable <= len*7/8)
                printable = 0;

        for (i = 0; i < len; i++)
                if (printable)
                        fprintf(out, isprint(str[i]) ? "%c" : "\\%03o",
                                (unsigned char)str[i]);
                else
                        fprintf(out, "%02x ", (unsigned char)str[i]);
}


//print Blocks of inode (ext4)
static void local_dump_extents(FILE *f, const char *prefix, struct ext2_inode * inode,
                         int flags, int logical_width, int physical_width)
{
        ext2_extent_handle_t    handle;
        struct ext2fs_extent    extent;
        struct ext2_extent_info info;
        int                     op = EXT2_EXTENT_ROOT;
        unsigned int            printed = 0;
        errcode_t               errcode;

	
        errcode = local_ext2fs_extent_open(current_fs, *inode, &handle);
        if (errcode)
                return;

        if (flags & DUMP_EXTENT_TABLE)
                fprintf(f, "Level Entries %*s %*s Length Flags\n",
                        (logical_width*2)+3, "Logical",
                        (physical_width*2)+3, "Physical");
        else
                fprintf(f, "%sEXTENTS:\n%s", prefix, prefix);

        while (1) {
                errcode = ext2fs_extent_get(handle, op, &extent);

                if (errcode)
                        break;

                op = EXT2_EXTENT_NEXT;

                if (extent.e_flags & EXT2_EXTENT_FLAGS_SECOND_VISIT)
                        continue;

                if (extent.e_flags & EXT2_EXTENT_FLAGS_LEAF) {
                        if ((flags & DUMP_LEAF_EXTENTS) == 0)
                                continue;
                } 

                errcode = ext2fs_extent_get_info(handle, &info);
                if (errcode)
                        continue;

                if (!(extent.e_flags & EXT2_EXTENT_FLAGS_LEAF)) {
                        if (extent.e_flags & EXT2_EXTENT_FLAGS_SECOND_VISIT)
                                continue;

                        if (flags & DUMP_EXTENT_TABLE) {
                                fprintf(f, "%2d/%2d %3d/%3d %11llu - %11llu "
                                        "%11llu%14s %6u\n",
                                        info.curr_level, info.max_depth,
                                        info.curr_entry, info.num_entries,
                                        extent.e_lblk,
                                        extent.e_lblk + (extent.e_len - 1),
                                        extent.e_pblk,
					"", extent.e_len);
                                continue;
                        }

                        fprintf(f, "%s(NODE #%d, %lld-%lld, blk %lld)",
                                printed ? ", " : "",
                                info.curr_entry,
                                extent.e_lblk,
                                extent.e_lblk + (extent.e_len - 1),
                                extent.e_pblk);
                        printed = 1;
                        continue;
                }

                if (flags & DUMP_EXTENT_TABLE) {
                        fprintf(f, "%2d/%2d %3d/%3d %11llu - %11llu %11llu - %11llu %6u %s\n",
                                info.curr_level, info.max_depth,
                                info.curr_entry, info.num_entries,
                                extent.e_lblk,
                                extent.e_lblk + (extent.e_len - 1),
                                extent.e_pblk,
                                extent.e_pblk + (extent.e_len - 1),
                                extent.e_len,
                                extent.e_flags & EXT2_EXTENT_FLAGS_UNINIT ?
                                        "Uninit" : "");
                        continue;
               }

                if (extent.e_len == 0)
                        continue;
                else if (extent.e_len == 1)
                        fprintf(f,
                                "%s(%lld%s): %lld",
                                printed ? ", " : "",
                                extent.e_lblk,
                                extent.e_flags & EXT2_EXTENT_FLAGS_UNINIT ?
                                " [uninit]" : "",
                                extent.e_pblk);
                else
                        fprintf(f,
                                "%s(%lld-%lld%s): %lld-%lld",
                                printed ? ", " : "",
                                extent.e_lblk,
                                extent.e_lblk + (extent.e_len - 1),
                                extent.e_flags & EXT2_EXTENT_FLAGS_UNINIT ?
                                        " [uninit]" : "",
                                extent.e_pblk,
                                extent.e_pblk + (extent.e_len - 1));
                printed = 1;
        }
	if (printed)
                fprintf(f, "\n\n");
	 local_ext2fs_extent_free(handle);
	
	
}


//print extended attribute of Inode 
static void dump_inode_extra(FILE *out,
                                      const char *prefix EXT2FS_ATTR((unused)),
                                      ext2_ino_t inode_num EXT2FS_ATTR((unused)),
                                      struct ext2_inode_large *inode)
{
        struct ext2_ext_attr_entry *entry;
        __u32 *magic;
        char *start, *end;
        unsigned int storage_size;

        fprintf(out, "Size of extra inode fields: %u\n", inode->i_extra_isize);
        if (inode->i_extra_isize > EXT2_INODE_SIZE(current_fs->super) -
                        EXT2_GOOD_OLD_INODE_SIZE) {
                fprintf(stderr, "invalid inode->i_extra_isize (%u)\n",
                                inode->i_extra_isize);
                return;
        }
        storage_size = EXT2_INODE_SIZE(current_fs->super) -
                        EXT2_GOOD_OLD_INODE_SIZE -
                        inode->i_extra_isize;
        magic = (__u32 *)((char *)inode + EXT2_GOOD_OLD_INODE_SIZE +
                        inode->i_extra_isize);
        if (*magic == EXT2_EXT_ATTR_MAGIC) {
                fprintf(out, "Extended attributes stored in inode body: \n");
                end = (char *) inode + EXT2_INODE_SIZE(current_fs->super);
                start = (char *) magic + sizeof(__u32);
                entry = (struct ext2_ext_attr_entry *) start;
                while (!EXT2_EXT_IS_LAST_ENTRY(entry)) {
                        struct ext2_ext_attr_entry *next =
                                EXT2_EXT_ATTR_NEXT(entry);
                        if (entry->e_value_size > storage_size ||
                                        (char *) next >= end) {
                                fprintf(out, "invalid EA entry in inode\n");
                                return;
                        }
                        fprintf(out, "  ");
                        dump_xattr_string(out, EXT2_EXT_ATTR_NAME(entry),
                                          entry->e_name_len);
                        fprintf(out, " = \"");
                        dump_xattr_string(out, start + entry->e_value_offs,
                                                entry->e_value_size);
                        fprintf(out, "\" (%u)\n", entry->e_value_size);
                        entry = next;
                }
        }
}


//subfunction for dump_blocks
static void finish_range(struct list_blocks_struct *lb)
{
        if (lb->first_block == 0)
                return;
        if (lb->first)
                lb->first = 0;
        else
                fprintf(lb->f, ", ");
        if (lb->first_block == lb->last_block)
                fprintf(lb->f, "(%lld):%u",
                        (long long)lb->first_bcnt, lb->first_block);
        else
                fprintf(lb->f, "(%lld-%lld):%u-%u",
                        (long long)lb->first_bcnt, (long long)lb->last_bcnt,
                        lb->first_block, lb->last_block);
        lb->first_block = 0;
}


//subfunction for dump_blocks
static int list_blocks_proc(ext2_filsys fs EXT2FS_ATTR((unused)),
                            blk64_t *block64nr, e2_blkcnt_t blockcnt,
                            blk64_t ref_block EXT2FS_ATTR((unused)),
                            int ref_offset EXT2FS_ATTR((unused)),
                            void *private)
{
	blk_t blocknr = (blk_t) *block64nr;
        struct list_blocks_struct *lb = (struct list_blocks_struct *) private;

        lb->total++;
        if (blockcnt >= 0) {
//* See if we can add on to the existing range (if it exists)
                if (lb->first_block &&
                    (lb->last_block+1 == blocknr) &&
                    (lb->last_bcnt+1 == blockcnt)) {
                        lb->last_block = blocknr;
                        lb->last_bcnt = blockcnt;
                        return 0;
                }
//* Start a new range.
                finish_range(lb);
                lb->first_block = lb->last_block = blocknr;
                lb->first_bcnt = lb->last_bcnt = blockcnt;
                return 0;
        }
//* Not a normal block.  Always force a new range.
        finish_range(lb);
        if (lb->first)
                lb->first = 0;
        else
                fprintf(lb->f, ", ");
        if (blockcnt == -1)
                fprintf(lb->f, "(IND):%u", blocknr);
        else if (blockcnt == -2)
                fprintf(lb->f, "(DIND):%u", blocknr);
        else if (blockcnt == -3)
                fprintf(lb->f, "(TIND):%u", blocknr);
        return 0;
}


// print the  Datablocks from Inode (ext3)
static void dump_blocks(FILE *f, const char *prefix, struct ext2_inode *inode)
{
        struct list_blocks_struct lb;

        fprintf(f, "%sBLOCKS:\n%s", prefix, prefix);
        lb.total = 0;
        lb.first_block = 0;
        lb.f = f;
        lb.first = 1;
       // ext2fs_block_iterate2(current_fs, inode, BLOCK_FLAG_READ_ONLY, NULL,
	local_block_iterate3(current_fs, *inode, BLOCK_FLAG_READ_ONLY, NULL,
                             list_blocks_proc, (void *)&lb);
        finish_range(&lb);
        if (lb.total)
                fprintf(f, "\n%sTOTAL: %lld\n", prefix, (long long)lb.total);
        fprintf(f,"\n");
}


//print the contents of inode
void dump_inode(FILE *out, const char *prefix,
                         ext2_ino_t inode_num, struct ext2_inode *inode,
                         int do_dump_blocks)
{
        const char *i_type;
        char frag, fsize;
        int os = current_fs->super->s_creator_os;
        struct ext2_inode_large *large_inode;
        int is_large_inode = 0;

        if (EXT2_INODE_SIZE(current_fs->super) > EXT2_GOOD_OLD_INODE_SIZE)
                is_large_inode = 1;
        large_inode = (struct ext2_inode_large *) inode;
//	blockhex(stdout,large_inode,0,256);

        if (LINUX_S_ISDIR(inode->i_mode)) i_type = "directory";
        else if (LINUX_S_ISREG(inode->i_mode)) i_type = "regular";
        else if (LINUX_S_ISLNK(inode->i_mode)) i_type = "symlink";
        else if (LINUX_S_ISBLK(inode->i_mode)) i_type = "block special";
        else if (LINUX_S_ISCHR(inode->i_mode)) i_type = "character special";
        else if (LINUX_S_ISFIFO(inode->i_mode)) i_type = "FIFO";
        else if (LINUX_S_ISSOCK(inode->i_mode)) i_type = "socket";
        else i_type = "bad type";
        fprintf(out, "%sInode: %u   Type: %s    ", prefix, inode_num, i_type);
        fprintf(out, "%sMode:  %04o   Flags: 0x%x ",
                prefix, inode->i_mode & 0777, inode->i_flags);
#ifdef FILE_ATTR
#include <e2p/e2p.h>
	if (do_dump_blocks && inode->i_flags) {
		fprintf(out,"[");
		print_flags(out, inode->i_flags, 0);
		fprintf(out,"]\n");
	}
	else
#endif
	fprintf(out,"\n");

        if (is_large_inode && large_inode->i_extra_isize >= 24) {
                fprintf(out, "%sGeneration: %u    Version: 0x%08x:%08x\n",
                        prefix, inode->i_generation, large_inode->i_version_hi,
                        inode->osd1.linux1.l_i_version);
        } else {
                fprintf(out, "%sGeneration: %u    Version: 0x%08x\n", prefix,
                        inode->i_generation, inode->osd1.linux1.l_i_version);
        }
        fprintf(out, "%sUser: %5d   Group: %5d   Size: ",
                prefix, inode_uid(*inode), inode_gid(*inode));
        if (LINUX_S_ISREG(inode->i_mode)) {
                unsigned long long i_size = (inode->i_size |
                                    ((unsigned long long)inode->i_size_high << 32));

                fprintf(out, "%llu\n", i_size);
        } else
                fprintf(out, "%d\n", inode->i_size);
        if (os == EXT2_OS_HURD)
        fprintf(out,
                        "%sFile ACL: %d    Directory ACL: %d Translator: %d\n",
                        prefix,
                        inode->i_file_acl, LINUX_S_ISDIR(inode->i_mode) ? inode->i_dir_acl : 0,
                        inode->osd1.hurd1.h_i_translator);
        else
                fprintf(out, "%sFile ACL: %llu    Directory ACL: %d\n",
                        prefix,
                        inode->i_file_acl | ((long long)
                                (inode->osd2.linux2.l_i_file_acl_high) << 32),
                        LINUX_S_ISDIR(inode->i_mode) ? inode->i_dir_acl : 0);
        if (os == EXT2_OS_LINUX)
                fprintf(out, "%sLinks: %d   Blockcount: %llu\n",
                        prefix, inode->i_links_count,
                        (((unsigned long long)
                          inode->osd2.linux2.l_i_blocks_hi << 32)) +
                        inode->i_blocks);
        else
                fprintf(out, "%sLinks: %d   Blockcount: %u\n",
                        prefix, inode->i_links_count, inode->i_blocks);
        switch (os) {
            case EXT2_OS_HURD:
                frag = inode->osd2.hurd2.h_i_frag;
                fsize = inode->osd2.hurd2.h_i_fsize;
                break;
            default:
                frag = fsize = 0;
        }
        fprintf(out, "%sFragment:  Address: %d    Number: %d    Size: %d\n",
                prefix, inode->i_faddr, frag, fsize);
        if (is_large_inode && large_inode->i_extra_isize >= 24) {
		fprintf(out, "%s ctime: %10lu:%010lu -- %s", prefix,
                        (long unsigned int)inode->i_ctime, (long unsigned int)large_inode->i_ctime_extra,
                        time_to_string(inode->i_ctime));
                fprintf(out, "%s atime: %10lu:%010lu -- %s", prefix,
                        (long unsigned int)inode->i_atime, (long unsigned int)large_inode->i_atime_extra,
                        time_to_string(inode->i_atime));
                fprintf(out, "%s mtime: %10lu:%010lu -- %s", prefix,
                        (long unsigned int)inode->i_mtime, (long unsigned int)large_inode->i_mtime_extra,
                        time_to_string(inode->i_mtime));
                fprintf(out, "%scrtime: %10lu:%010lu -- %s", prefix,
                        (long unsigned int)large_inode->i_crtime, (long unsigned int)large_inode->i_crtime_extra,
                        time_to_string(large_inode->i_crtime));
        } else {
           fprintf(out, "%sctime: %10lu -- %s", prefix, (long unsigned int)inode->i_ctime,
                        time_to_string(inode->i_ctime));
                fprintf(out, "%satime: %10lu -- %s", prefix, (long unsigned int)inode->i_atime,
                        time_to_string(inode->i_atime));
                fprintf(out, "%smtime: %10lu -- %s", prefix, (long unsigned int)inode->i_mtime,
                        time_to_string(inode->i_mtime));
        }
        if (inode->i_dtime)
          fprintf(out, "%sdtime: %10lu -- %s", prefix, (long unsigned int)inode->i_dtime,
                  time_to_string(inode->i_dtime));
        if (EXT2_INODE_SIZE(current_fs->super) > EXT2_GOOD_OLD_INODE_SIZE)
                dump_inode_extra(out, prefix, inode_num,
                                          (struct ext2_inode_large *) inode);
        if (LINUX_S_ISLNK(inode->i_mode) && ext2fs_inode_data_blocks(current_fs,inode) == 0)
                fprintf(out, "%sFast_link_dest: %.*s\n\n", prefix,
                        (int) inode->i_size, (char *)inode->i_block);
        else if (LINUX_S_ISBLK(inode->i_mode) || LINUX_S_ISCHR(inode->i_mode)) {
                int major, minor;
                const char *devnote;

                if (inode->i_block[0]) {
                        major = (inode->i_block[0] >> 8) & 255;
                        minor = inode->i_block[0] & 255;
                        devnote = "";
                } else {
                        major = (inode->i_block[1] & 0xfff00) >> 8;
                        minor = ((inode->i_block[1] & 0xff) |
                                 ((inode->i_block[1] >> 12) & 0xfff00));
                        devnote = "(New-style) ";
                }
                fprintf(out, "%sDevice major/minor number: %02d:%02d (hex %02x:%02x)\n\n",
                        devnote, major, minor, major, minor);
        } else if (do_dump_blocks && !(inode->i_dtime)) {
                if (inode->i_flags & EXT4_EXTENTS_FL)
                        local_dump_extents(out, prefix, inode,
                                     DUMP_LEAF_EXTENTS | DUMP_EXTENT_TABLE, 11, 11);
                else
                        dump_blocks(out, prefix, inode);
        }
}


//calculate the position of inode in FS
blk_t get_inode_pos(struct ext2_super_block *es ,struct inode_pos_struct *pos, ext2_ino_t inode_nr,int flag){
//FIXME blk64_t
	__u32 inode_group, group_offset, inodes_per_block, inode_offset;
	blk_t inode_block;
		
		pos->size = EXT2_INODE_SIZE(current_fs->super);
		inode_group = ((inode_nr - 1) / es->s_inodes_per_group);
		group_offset = ((inode_nr - 1) % es->s_inodes_per_group);
		inodes_per_block = (current_fs->blocksize / pos->size );
#ifdef EXT2_FLAG_64BITS
		inode_block = (blk_t) ext2fs_inode_table_loc(current_fs,inode_group) + (group_offset / inodes_per_block);
#else
		inode_block = current_fs->group_desc[inode_group].bg_inode_table + (group_offset / inodes_per_block);
#endif
		inode_offset = ((group_offset % inodes_per_block) * pos->size );
		if (flag)
			printf("Inode %u is at group %u, block %u, offset %u\n", inode_nr, inode_group, inode_block, inode_offset);

		pos->block = inode_block;
		pos->offset = inode_offset;

	return inode_block;
};


// get journalinode from transactionnumber 
int get_transaction_inode(ext2_ino_t inode_nr, __u32 transaction_nr, struct ext2_inode_large *inode){
	struct inode_pos_struct 	pos;
	__u32 				journal_block;
	blk_t				block_nr;
	struct ext2_inode_large 	*inode_buf;
	char 				*buf = NULL;
	int 				retval = 0;
	unsigned int			got;
	int 				blocksize = current_fs->blocksize;

	block_nr = get_inode_pos(current_fs->super, &pos , inode_nr, 0);
	journal_block = get_journal_blocknr(block_nr, transaction_nr);
	if (! journal_block){
		fprintf(stdout,"No journalblock found for inode %lu by transaction %lu\n",
			(long unsigned int)inode_nr,(long unsigned int)transaction_nr);
		retval = -1;
	}
	else {
		buf =(char*) malloc(blocksize);
		if(buf){

			retval = read_journal_block(journal_block * blocksize ,buf,blocksize,&got);
			if ((! retval) && (got == blocksize)){
				inode_buf = (struct ext2_inode_large *)(buf + pos.offset);
#ifdef WORDS_BIGENDIAN
                		memset(inode, 0, pos.size);
                		ext2fs_swap_inode_full(current_fs, inode, inode_buf, 0, pos.size);

				if ((pos.size > EXT2_GOOD_OLD_INODE_SIZE) && (inode->i_extra_isize >= 24)
					&& (ext2fs_le32_to_cpu(inode_buf->i_crtime ) != inode->i_crtime)){
//FIXME: On my current version of libext2 the extra time fields ar not bigendian corrected
	// We solved this temporarily here with this function
					le_to_cpu_swap_extra_time(inode,(char*)inode_buf);
				}
#else
                		memcpy(inode, inode_buf, pos.size);
#endif 			
			}
		free(buf);
		}
	}
return retval;
}


//function for dump_inode_list
void print_j_inode(struct ext2_inode_large *inode , ext2_ino_t inode_nr , __u32 transaction , int flag){
	fprintf(stdout,"\nDump Inode %d from journal transaction %d\n",inode_nr,transaction);
	dump_inode(stdout, "",inode_nr, (struct ext2_inode *)inode, flag);
return ;
}



//print the contents of all copy of inode in the journal
void dump_inode_list(struct ring_buf* buf, int flag){
	r_item* item;
	int i, count;
	__u32 time_stamp;

	if (!buf ) return;
	item = r_first(buf);

	count = r_get_count(buf);
	for (i = 0; i < count ; i++){
		print_j_inode(item->inode , buf->nr , item->transaction.start, flag);
		if ( LINUX_S_ISDIR(item->inode->i_mode)){
			time_stamp = (item->transaction.start) ? get_trans_time(item->transaction.start) : 0 ;
#ifdef DEBUG
			printf ("Transaction-time for search %ld : %s\n", time_stamp, time_to_string(time_stamp));
#endif
			list_dir3(buf->nr, (struct ext2_inode*)item->inode, &(item->transaction),time_stamp);
		}
		item = r_next(item);	
	}
return;
}


// return the last undeleted inode in journal
r_item* get_last_undel_inode(struct ring_buf* buf){
	r_item* item;
	int i, count;
//	__u32 generation;

	if (!buf ) return NULL;
	item = r_last(buf);

	count = r_get_count(buf);
	for (i = 0; i< count; i++){
//		if ( !i ) generation = item->inode->i_generation;
		if (item->inode->i_dtime) {
//			 buf->del_flag = 1;
			 item = r_prev(item);
		}
		  else {
//			if (item->inode->i_generation != generation) 
//				buf->reuse_flag = 1;
#ifdef DEBUG
		printf("UD-Inode %d\n",item->transaction.start);
#endif
			if((! item->inode->i_links_count)&&(item->inode->i_ctime)
				&&(item->inode->i_mode))
				item->inode->i_links_count = 1 ;
			return item;
		}
	}
	return NULL;
}


// return the last undelete inode in journal after ->  <-before
 r_item* get_undel_inode(struct ring_buf* buf, __u32 after, __u32 before){
	r_item* item;
	int i, count;
//	__u32 generation;

	if (!buf) return NULL;
	item = r_last(buf);

	count = r_get_count(buf);
	for (i = 0; i< count; i++){
//		if ( !i ) generation = item->inode->i_generation;
		if ((item->inode->i_dtime) && (item->inode->i_dtime < after) ) 
			return NULL;
		if ((item->inode->i_ctime >= before ) || (item->inode->i_dtime)) {
//			if (item->inode->i_dtime) 
//				buf->del_flag = 1 ;
			item = r_prev(item);
			continue;
		}
//		if (item->inode->i_generation != generation) 
//			buf->reuse_flag = 1;
		if((! item->inode->i_links_count)&&(item->inode->i_ctime)&&(item->inode->i_mode))
			item->inode->i_links_count = 1 ;
#ifdef DEBUG
		printf("UTD-Inode %d\n",item->transaction.start);
#endif
		return item;
	}
	return NULL;
}


//fill all inode found in the Journal in the inode-ringbuffer
struct ring_buf* get_j_inode_list(struct ext2_super_block *es, ext2_ino_t inode_nr){
	struct inode_pos_struct 	pos;
	blk_t				block;
	char 				*inode_buf = NULL ;
	struct ext2_inode 		*inode_pointer;
	struct ring_buf			*buf = NULL;
	r_item 				*item = NULL;
//	struct ext2_inode_large 	*inode = NULL;
	int 				count, retval = 0;
	unsigned int			got;	
	off_t 				offset;
	char 				*journal_tag_buf = NULL;
	journal_descriptor_tag_t 	*block_list;
	__u32 				ctime = 1;
	__u32 				same_size = 1;
	__u32 				same_block_count = 0;
	__u16 				same_link_count = 0;

	if ((inode_nr > es->s_inodes_count) || (inode_nr == 0))
	{
         printf(" unknown ERROR: bad inode number found  %d \n", inode_nr );
		 return NULL;
	}

	block = get_inode_pos(es , &pos , inode_nr, 0);
	inode_buf = malloc(pos.size);
	if (!inode_buf) {
		fprintf(stderr,"Error: can not allocate memory for buffer\n");
		goto errout;
	}
	count = get_block_list_count(block) ;
#ifdef DEBUG
	fprintf(stdout,"\n\nINODE_RING : %ld/%ld/%ld : ",inode_nr, pos.block, pos.offset);
#endif

	if(! count) {
// no inode block found  
// then we will load the oginal inode from the filesystem
// we will hope, the file has not change for a long time
		buf = ring_new(pos.size,inode_nr);
		if (buf)
			item = r_item_add(buf);
		if ( ! item)
		{
			fprintf(stderr,"Error: can not allocate memory for inode\n");
			goto errout;
		}
		if ( ext2fs_read_inode_full(current_fs, inode_nr, (struct ext2_inode*) item->inode, pos.size))
			goto errout;
		item->transaction.start = item->transaction.end = 0;
#ifdef DEBUG
	fprintf(stdout,"*;");
#endif
	}
	else {
	
// read and fill the journal inode 
	journal_tag_buf = (void *)malloc((sizeof(journal_descriptor_tag_t) * count));
	if (!journal_tag_buf) {
		fprintf(stderr,"Error: while allocate Memory for blocklist\n");
		goto errout;
	}
	block_list = (journal_descriptor_tag_t *) journal_tag_buf;

	count = get_block_list(block_list, block, count);
	buf = ring_new(pos.size,inode_nr);
	if ((! buf) || (! count)) goto errout;

	for (;count > 0;count-- , block_list++ ){
		offset = (block_list->j_blocknr * current_fs->blocksize) + pos.offset ;
		retval = read_journal_block( offset  , inode_buf , pos.size , &got);
		if (retval) {
			fprintf(stderr,"Error: while read Inode %d from journal transaction %d\n", inode_nr, block_list->transaction); 
			goto errout;
		}

		inode_pointer = (struct ext2_inode*) inode_buf ;
//FIXME: check more bad Inode 
 		if (get_inode_mode_type(ext2fs_le16_to_cpu(inode_pointer->i_mode)) == ' '){
#ifdef DEBUG
			fprintf(stdout,"!%ld;",block_list->transaction);
#endif
			continue;
		}
//Correct d_time anomalies if directory delete process not completed  BUG: #18730
		if ((LINUX_S_ISDIR(ext2fs_le16_to_cpu(inode_pointer->i_mode))) &&
			ext2fs_le32_to_cpu(inode_pointer->i_dtime) && ext2fs_le32_to_cpu(inode_pointer->i_blocks) &&
			ext2fs_le32_to_cpu(inode_pointer->i_size) && (!ext2fs_le16_to_cpu(inode_pointer->i_links_count)) &&
			ext2fs_le32_to_cpu(inode_pointer->i_ctime) != ext2fs_le32_to_cpu(inode_pointer->i_dtime)){
			if ( buf->count && LINUX_S_ISDIR(item->inode->i_mode) && item->inode->i_blocks){
				continue;
			}
			else{
				inode_pointer->i_dtime = 0;
			}
		}
//inode with the same ctime + the same size and links and <= block_count, skipped
 		if (ext2fs_le32_to_cpu(inode_pointer->i_ctime) == ctime){
			if (item) { 
				if ((ext2fs_le32_to_cpu(inode_pointer->i_size) == same_size) &&
					(ext2fs_le16_to_cpu(inode_pointer->i_links_count) == same_link_count) &&
					(ext2fs_le32_to_cpu(inode_pointer->i_blocks) <= same_block_count)){
					item->transaction.end = block_list->transaction;
#ifdef DEBUG
					fprintf(stdout,"-;");
#endif
					continue;
				}
			}
		}

		item = r_item_add(buf);
		if ( ! item)
		{
			fprintf(stderr,"Error: can not allocate memory for inode\n");
			goto errout;
		}

#ifdef WORDS_BIGENDIAN
                memset(item->inode, 0, pos.size);

                ext2fs_swap_inode_full(current_fs,
                                (struct ext2_inode_large *) item->inode,
                                (struct ext2_inode_large *) inode_buf,
                                0, pos.size); 

		if ((pos.size > EXT2_GOOD_OLD_INODE_SIZE) && (item->inode->i_extra_isize >= 24)
  			&& (ext2fs_le32_to_cpu(((struct ext2_inode_large *)inode_buf)->i_crtime)  != item->inode->i_crtime)){
//FIXME: On my current version of libext2 the extra time fields ar not bigendian corrected
	// We solved this temporarily here with this function
			le_to_cpu_swap_extra_time(item->inode,(char*)inode_buf);
		}
#else
                memcpy(item->inode, inode_buf, pos.size);
#endif
#ifdef DEBUG
		fprintf(stdout,"%ld;",block_list->transaction);
#endif
		item->transaction.start = block_list->transaction;
		item->transaction.end = block_list->transaction;
		ctime = item->inode->i_ctime;
		same_size = item->inode->i_size; 
		same_block_count = item->inode->i_blocks;
		same_link_count = item->inode->i_links_count;
	  }

// check for the real filesystem inode
// if not delete or newer than the last journal copy, we add also this inode 
		
		if ( ext2fs_read_inode_full(current_fs, inode_nr, (struct ext2_inode*)inode_buf, pos.size))
			goto errout;
		inode_pointer = (struct ext2_inode*) inode_buf ;
		if((((! inode_pointer->i_dtime) && inode_pointer->i_ctime) || (inode_pointer->i_ctime > ctime)) &&
			(inode_pointer->i_ctime < (__u32) now_time)){
			item = r_item_add(buf);
			if ( ! item){
				fprintf(stderr,"Error: can not allocate memory for inode\n");
				goto errout;
			}
			memcpy(item->inode, inode_buf, pos.size);
			item->transaction.start = item->transaction.end = 0;
#ifdef DEBUG
			fprintf(stdout,"*;");
#endif
		}

	} // else-tree ? count==0 

	if (buf->count == 0)
		goto errout;
	
	if ( inode_buf ) { 
		free(inode_buf);
		inode_buf = NULL;
	}
	if ( journal_tag_buf ) {
		free(journal_tag_buf);
		journal_tag_buf = NULL;
	}
	r_begin(buf);
#ifdef DEBUG
	fprintf(stdout,"<OK> %ld\n",buf->count);
#endif
	return buf;

errout: 
	if ( inode_buf ) { 
		free(inode_buf);
		inode_buf = NULL;
	}
	if ( journal_tag_buf ) {
		free(journal_tag_buf);
		journal_tag_buf = NULL;
	}

	if ( buf ) { 
			ring_del(buf);
			buf = NULL;
	}
#ifdef DEBUG
	fprintf(stdout,"<NULL>\n");
#endif
	return NULL;
}


// get the first Journal Inode by time_stamp	
int read_time_match_inode( ext2_ino_t inode_nr, struct ext2_inode* inode_buf, __u32 time_stamp){
	struct ring_buf* i_ring;
	r_item* item;
	int retval = 1;
	
	i_ring = get_j_inode_list(current_fs->super, inode_nr);
	if (i_ring) {
		item = r_last(i_ring);

		while ((item->inode->i_ctime  > time_stamp) && ( item != r_first(i_ring)))
				item = r_prev(item);

//FIXME
		memcpy(inode_buf,item->inode,EXT2_GOOD_OLD_INODE_SIZE);
		ring_del(i_ring);
		retval= 0;
	}
return retval;
}





// get the first Journal Inode by transaction	
int read_journal_inode( ext2_ino_t inode_nr, struct ext2_inode* inode_buf, __u32 transaction){
	struct ring_buf* i_ring;
	r_item* item;
	int retval = 1;
	
	i_ring = get_j_inode_list(current_fs->super, inode_nr);
	if (i_ring) {
		item = r_first(i_ring);
		while ((item->transaction.start < transaction) && ( item != r_last(i_ring)))
				item = r_next(item);
		memcpy(inode_buf,item->inode,EXT2_GOOD_OLD_INODE_SIZE);
		ring_del(i_ring);
		retval= 0;
	}
return retval;
}



//create a new inode
struct ext2_inode_large* new_inode(){
	struct ext2_inode_large			*inode;
	__u32 					a_time;
	time_t  				help_time;
	struct ext3_extent_header		*p_extent_header;
	
	time( &help_time ); 
	a_time = (__u32) help_time;
	inode = malloc(EXT2_INODE_SIZE(current_fs->super));
	if (! inode )
		return NULL;
	
	memset(inode, 0 , EXT2_INODE_SIZE(current_fs->super));
	inode->i_mode = LINUX_S_IFREG | LINUX_S_IRUSR | LINUX_S_IWUSR | LINUX_S_IRGRP | LINUX_S_IROTH ;
	inode->i_ctime = inode->i_mtime = inode->i_atime = a_time;
	inode->i_links_count = 1 ;
	if (current_fs->super->s_feature_incompat & EXT3_FEATURE_INCOMPAT_EXTENTS) {
		inode->i_flags = EXT4_EXTENTS_FL ;
		p_extent_header = (struct ext3_extent_header*) inode->i_block;
		p_extent_header->eh_magic = ext2fs_cpu_to_le16(EXT3_EXT_MAGIC) ;
		p_extent_header->eh_max = ext2fs_cpu_to_le16(4);		
	}
return inode;
}




//add extent to inode
int inode_add_extent(struct ext2_inode_large* inode , struct extent_area* ea, __u32* last, int flag ){
	int 				ret = 0;
	struct ext3_extent_idx		*idx;
	struct ext3_extent_header	*header;
	struct ext3_extent 		*extent;
	struct ext3_extent 		*extent_new;
	__u64				i_size;
	
	if ((!ea ) || (!ea->blocknr))
		return 0;
	header = (struct ext3_extent_header*) inode->i_block;
	if (flag){
// flag == 1 ; add extent index
		if (ext2fs_le16_to_cpu(header->eh_entries) >= ext2fs_le16_to_cpu(header->eh_max)){
			fprintf(stderr," Error: can not add a extent to inode\n");
			return 0;
		}
		idx = (struct ext3_extent_idx*) (header + (ext2fs_le16_to_cpu(header->eh_entries) + 1));

		if (! header->eh_entries)
			header->eh_depth = ext2fs_cpu_to_le16(ea->depth) +1;
		
		idx->ei_leaf = ext2fs_cpu_to_le32(ea->blocknr);
		idx->ei_leaf_hi = 0;
		idx->ei_unused = 0;
		idx->ei_block = ext2fs_cpu_to_le32(ea-> l_start);
		header->eh_entries = ext2fs_cpu_to_le16(ext2fs_le16_to_cpu(header->eh_entries) + 1 );
		ret = 1;
	}
	 else{
// flag == 0 : add or attach a extent entry		
		if (! ext2fs_le16_to_cpu(header->eh_entries)) 
			header->eh_entries = ext2fs_cpu_to_le16(1);
			
		extent = (struct ext3_extent*) (header + (ext2fs_le16_to_cpu(header->eh_entries)+1));
//		new		
		if(!(ext2fs_le32_to_cpu(extent->ee_start))){
			extent->ee_start = ext2fs_cpu_to_le32(ea->start_b);
			extent->ee_len = ext2fs_cpu_to_le16(ea->len);
			ret = 1 ;
		} 
		else{
//			attach
			if (ea->start_b == (ext2fs_le32_to_cpu(extent->ee_start) + ext2fs_le16_to_cpu(extent->ee_len))){
				extent->ee_len = ext2fs_cpu_to_le16(ext2fs_le16_to_cpu(extent->ee_len)+ ea->len);
				ret = 1;
			}
		}
		if ((! ret) && (ext2fs_le16_to_cpu(header->eh_entries) < ext2fs_le16_to_cpu(header->eh_max))){
//			new entry
			header->eh_entries = ext2fs_cpu_to_le16(ext2fs_le16_to_cpu(header->eh_entries) + 1 );
			extent_new = (struct ext3_extent*) (header + (ext2fs_le16_to_cpu(header->eh_entries)+1));
//			extent->ee_start_hi
			extent_new->ee_start = ext2fs_cpu_to_le32(ea->start_b);
			extent_new->ee_len = ext2fs_cpu_to_le16(ea->len);
			extent_new->ee_block = ext2fs_cpu_to_le32(ext2fs_le32_to_cpu(extent->ee_block)+ext2fs_le16_to_cpu(extent->ee_len));
			ret = 1 ;
		}
	}
	if (ret){
		i_size = (unsigned long long)(inode->i_size | ((unsigned long long)inode->i_size_high << 32));
		i_size += ea->size;
		inode->i_size = i_size & 0xffffffff ;
		inode->i_size_high = i_size >> 32 ;
		inode->i_blocks += (ea->b_count * (current_fs->blocksize / 512)) ;	
		*last = (ea->end_b) ? ea->end_b : 0 ;
//		blockhex (stdout, (void*) inode, 0, 128);
	}
	else
		fprintf(stderr," Error: can not add a extent to inode\n");
return ret;
}



//add a block to inode, (ext3 only the first 12 blocks)
int inode_add_block(struct ext2_inode_large* inode , blk_t blk ){
	int				i = 0 ; 
	unsigned long long		i_size;
	struct ext3_extent 		*extent;
	struct ext3_extent_header	*header;
	
	if (! (inode->i_flags & EXT4_EXTENTS_FL)){
	//ext3
		while ((i < EXT2_N_BLOCKS) && inode->i_block[i] )
			i++;
		if ( i >= EXT2_NDIR_BLOCKS){
//			printf("faulty Block %u  as i_block %d \n", i,blk); 
			//indirect blocks
			return 0;
		}

		inode->i_block[i] = blk;
	}
	else{
	//ext4
		header = (struct ext3_extent_header*) inode->i_block;
		if (! ext2fs_le16_to_cpu(header->eh_entries)) {
			header->eh_entries = ext2fs_cpu_to_le16(1);
			extent = (struct ext3_extent*) (header + (ext2fs_le16_to_cpu(header->eh_entries)));
			extent->ee_start = ext2fs_cpu_to_le32(blk);
		}
		else 
			extent = (struct ext3_extent*) (header + (ext2fs_le16_to_cpu(header->eh_entries)));
		extent->ee_len = ext2fs_cpu_to_le16(ext2fs_le16_to_cpu(extent->ee_len) +1);
	}
	i_size = (unsigned long long)(inode->i_size | ((unsigned long long)inode->i_size_high << 32));
	i_size += current_fs->blocksize;
	inode->i_size = i_size & 0xffffffff ;
	inode->i_size_high = i_size >> 32 ;
	inode->i_blocks += (current_fs->blocksize / 512);

return 1;
}


//add the ext3  indirect Blocks to the inode
int inode_add_meta_block(struct ext2_inode_large* inode , blk_t blk, blk_t *last, blk_t  *next, char *buf ){
	blk_t 				block_count = 0;
	int 				i = 0;
	__u64				i_size = 0;
	int 				ret = 0;

	if (! (inode->i_flags & EXT4_EXTENTS_FL)){
		
		while ((i < EXT2_N_BLOCKS) && inode->i_block[i] )
			i++;

		switch (i){
			case EXT2_IND_BLOCK :
				ret = get_ind_block_len(buf, &block_count, last, next, &i_size);
				break;
			case EXT2_DIND_BLOCK :
				ret = get_dind_block_len(buf, &block_count, last,  next, &i_size);
				break;
			case EXT2_TIND_BLOCK :
				ret = get_tind_block_len(buf, &block_count, last, next, &i_size);
				break;
			default:
//				printf("faulty Block %u as indirekter_block %d \n", i,blk); 
				return 0;
				break;
		}

		if (ret   &&  i_size){
			i_size += (((__u64)inode->i_size_high << 32)| inode->i_size);
			inode->i_size = i_size & 0xffffffff ;
			inode->i_size_high = i_size >> 32 ;
			inode->i_blocks += (block_count * (current_fs->blocksize / 512));
			inode->i_block[ i ] = blk;
		}
		else 
			*next = 0;
					
		return ret;
	}
	else{
//		printf("ERROR: ext3 indirect block %u ; but is a ext4_inode\n", blk);
	//FIXME ext4
	}
 return 0;
}
