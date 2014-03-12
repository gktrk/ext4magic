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
//header  util.h

#include <stdio.h>
#include <string.h>
#if HAVE_UNISTD_H  
#include <unistd.h>
#endif

#include <ext2fs/ext2fs.h>
//#include <ext2fs/ext2_io.h>
#include "ext2fsP.h"
#include <ctype.h>
#include <time.h> 
#include "dir_list.h"
#include "util.h"
#include "inode.h"
#include "block.h"

#define DIRENT_MIN_LENGTH 12
extern ext2_filsys     current_fs ;

static const char *monstr[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};



// local sub function for printing the dir entry
static int list_dir_proc(ext2_ino_t dir EXT2FS_ATTR((unused)),
                         int    entry,
                         struct ext2_dir_entry *dirent,
                         int    offset EXT2FS_ATTR((unused)),
                         int    blocksize EXT2FS_ATTR((unused)),
                         char   *buf EXT2FS_ATTR((unused)),
                         void   *private)
{
        struct ext2_inode       inode;
        ext2_ino_t              ino;
        struct tm               *tm_p;
        time_t                  modtime;
        char                    name[EXT2_NAME_LEN + 1];
        char                    tmp[EXT2_NAME_LEN + 16];
        char                    datestr[80];
        char                    lbr, rbr;
        int                     thislen,retval = 0;
	int 			col = 0;
        struct priv_dir_iterate_struct *ls = (struct priv_dir_iterate_struct *) private;

        thislen = ((dirent->name_len & 0xFF) < EXT2_NAME_LEN) ?
                (dirent->name_len & 0xFF) : EXT2_NAME_LEN;
        strncpy(name, dirent->name, thislen);
        name[thislen] = '\0';
        ino = dirent->inode;

//FIXME: 
  //      if ((entry == DIRENT_DELETED_FILE) || (ls->options & DELETED_DIR)) {
	if (entry == DIRENT_DELETED_FILE) {
                lbr = '<';
                rbr = '>';
        } else {
                lbr = rbr = ' ';
        }
        if (ls->options & PARSE_OPT) {
                if (ino && intern_read_inode(ino, &inode)) return 0;
                fprintf(stdout,"/%u/%06o/%d/%d/%s/",ino,inode.i_mode,inode.i_uid, inode.i_gid,name);
                if (LINUX_S_ISDIR(inode.i_mode))
                        fprintf(stdout, "/");
                else
                        fprintf(stdout, "%12lld/", inode.i_size | ((__u64)inode.i_size_high << 32));
                fprintf(stdout, "\n");
        }
        else if (ls->options & LONG_OPT) {
                if ((ino && (ino <= current_fs->super->s_inodes_count))) {
			if (ls->options & ONLY_JOURNAL)
//				retval = read_journal_inode( ino, &inode, ls->transaction->start);
				retval = read_time_match_inode( ino, &inode, ls->time_stamp);
			if (retval || !(ls->options & ONLY_JOURNAL))	
                        	if (intern_read_inode(ino, &inode))
                                	return 0;
                        modtime = inode.i_mtime;
                        tm_p = localtime(&modtime);
                        sprintf(datestr, "%2d-%s-%4d %02d:%02d",
                                tm_p->tm_mday, monstr[tm_p->tm_mon],
                                1900 + tm_p->tm_year, tm_p->tm_hour,
                                tm_p->tm_min);
                } else {
                        strcpy(datestr, "                 ");
                        memset(&inode, 0, sizeof(struct ext2_inode));
                }
                fprintf(stdout, "%c%8u%c %c %4o (%d)  %5d  %5d   ", lbr, ino, rbr, get_inode_mode_type(inode.i_mode),inode.i_mode & 07777 , dirent->name_len >> 8, inode_uid(inode), inode_gid(inode));
                if (LINUX_S_ISDIR(inode.i_mode))
                        fprintf(stdout, "%12d", inode.i_size);
                else
                        fprintf(stdout, "%12llu", inode.i_size |
                                ((unsigned long long) inode.i_size_high << 32));
                fprintf (stdout, " %s %s\n", datestr, name);
        } else {
                sprintf(tmp, "%c%u%c (%d) %s   ", lbr, dirent->inode, rbr,
                        dirent->rec_len, name);
                thislen = strlen(tmp);

                if (col + thislen > 80) {
                        fprintf(stdout, "\n");
                        col = 0;
                }
                fprintf(stdout, "%s", tmp);
                col += thislen;
        }
        return 0;
}


// list dir by using real fs inode and real fs dir blocks an real dir-entry-inodes
void list_dir(ext2_ino_t inode)
{
        int             retval;
        int             flags; 
        struct priv_dir_iterate_struct ls;

        ls.options = 0;
        if (!inode)  return;

        ls.options |= LONG_OPT;
        ls.options |= DELETED_OPT;

        flags = DIRENT_FLAG_INCLUDE_EMPTY;
        if (ls.options & DELETED_OPT) flags |= DIRENT_FLAG_INCLUDE_REMOVED;

        retval = ext2fs_dir_iterate2(current_fs, inode, flags,0, list_dir_proc, &ls);
        fprintf(stdout, "\n");
        if (retval)
                fprintf(stderr,"Error: %d directory data error\n", retval);

        return;
}





//---------------------------------------------------------------
/*
 * This function checks to see whether or not a potential deleted
 * directory entry looks valid.  What we do is check the deleted entry
 * and each successive entry to make sure that they all look valid and
 * that the last deleted entry ends at the beginning of the next
 * undeleted entry.  Returns 1 if the deleted entry looks valid, zero
 * if not valid.
 *
 * copy of a not public function from e2fsprogs ext2fs_validate_entry()
 * use only from local_process_dir_block()
 */
static int local_validate_entry(ext2_filsys fs, char *buf,
                                 unsigned int offset,
                                 unsigned int final_offset)
{
        struct ext2_dir_entry *dirent;
        unsigned int rec_len = 0;
	

        while ((offset < final_offset) &&
               (offset <= fs->blocksize - DIRENT_MIN_LENGTH)) {
                dirent = (struct ext2_dir_entry *)(buf + offset);
                if (ext2fs_get_rec_len(fs, dirent, &rec_len))
                        return 0;

                offset += rec_len;
                if ((rec_len < 8) ||
                    ((rec_len % 4) != 0) ||
                    ((((unsigned) dirent->name_len & 0xFF)+8) > rec_len))
                        return 0;
        }
		
        return (offset == final_offset);
}



#ifdef WORDS_BIGENDIAN
//---------------------------------------------------------------
/*
 * This function checks to see whether or not a potential deleted
 * directory entry looks valid. Returns 1 if the deleted entry looks valid,
 * zero if not valid.
 *
 * only from convert_dir_block() on bigendian
 */
static int local_validate_entry2(char *p , char *end, unsigned int offset, unsigned int final_offset)
{
        struct ext2_dir_entry_2 *dirent;
        __u16   rec_len = 0; 

        while ((offset < final_offset) &&
               (offset <= (unsigned int) (end - p - DIRENT_MIN_LENGTH))) {
                dirent = (struct ext2_dir_entry_2 *)(p + offset);
			rec_len = ext2fs_swab16(dirent->rec_len);
			if ((rec_len == 0 ) || (rec_len == 65535 )) 
				rec_len = current_fs->blocksize ;

                	offset += rec_len;
                	if ((rec_len < 8) || ((rec_len % 4) != 0) || (((unsigned) dirent->name_len +8) > rec_len))
                        	return 0;
        }
        return (offset == final_offset);
}


// convert dir_block for bigendian inclusive deleted entry
static int convert_dir_block(char *buf, int flags){
       	errcode_t       retval;
        char            *p, *end;
	char		real_len;
        struct ext2_dir_entry *dirent;
        unsigned int    name_len, rec_len;

        p = (char *) buf;
        end = (char *) buf + current_fs->blocksize;
        while (p < end-8) {
                dirent = (struct ext2_dir_entry *) p;

                dirent->inode = ext2fs_swab32(dirent->inode);
                dirent->rec_len = ext2fs_swab16(dirent->rec_len);
                dirent->name_len = ext2fs_swab16(dirent->name_len);

                name_len = dirent->name_len;

                if (flags & EXT2_DIRBLOCK_V2_STRUCT)
                        dirent->name_len = ext2fs_swab16(dirent->name_len);

                if ((retval = ext2fs_get_rec_len(current_fs, dirent, &rec_len)) != 0)
                        return retval;
                if ((rec_len < 8) || (rec_len % 4)) {
                        rec_len = 8;
                        retval = EXT2_ET_DIR_CORRUPTED;
                } else if (((name_len & 0xFF) + 8) > rec_len)
                        retval = EXT2_ET_DIR_CORRUPTED;
		real_len = ((name_len & 0xFF) + 11) & ~3;
		if ((rec_len > real_len) && ((real_len + DIRENT_MIN_LENGTH) <= rec_len)){
 		//	p += ((name_len & 0xFF) + 11) & ~3;
//FIXME
                        unsigned int offset = real_len;

		        while (offset < rec_len &&  !local_validate_entry2(p , end , offset, rec_len))
				offset += 4;
			p += offset;
		}
		else
              		 p += rec_len;
        }
        return retval;
}
#endif 



/*
 * Helper function which is private to this module.  Used by
 * local_dir_iterate3() (modi of ext2fs_process_dir_block
 */
 static int local_process_dir_block(ext2_filsys fs,
                             blk64_t      *blocknr,
                             e2_blkcnt_t blockcnt,
                             blk64_t      ref_block EXT2FS_ATTR((unused)),
                             int        ref_offset EXT2FS_ATTR((unused)),
                             void       *priv_data)
{
        struct dir_context *ctx = (struct dir_context *) priv_data;
        unsigned int    offset = 0;
        unsigned int    next_real_entry = 0;
        int             ret = 0;
        int             changed = 0;
        int             do_abort = 0;
        unsigned int    rec_len, size;
        int             entry, i;
        struct ext2_dir_entry *dirent;
 	trans_range_t *transaction = NULL;

        if (blockcnt < 0)
                return 0;
 	entry = blockcnt ? DIRENT_OTHER_FILE : DIRENT_DOT_FILE;

	transaction = (trans_range_t*) ((struct priv_dir_iterate_struct*)ctx->priv_data)->transaction;
	if (get_last_block(ctx->buf, blocknr, transaction->start, transaction->end)){
		if (ctx->flags & ONLY_JOURNAL ) return BLOCK_ABORT;
//FIXME:  if not found dir-block in journal, read from real filesystem and hope is ok ??????
		ctx->errcode = io_channel_read_blk(fs->io, *blocknr, 1, ctx->buf);
	}
#ifdef WORDS_BIGENDIAN
	if(!ctx->errcode)
		ctx->errcode = convert_dir_block(ctx->buf,0); 
#endif
	if (ctx->errcode)
                return BLOCK_ABORT;
	if (bmap)
		ext2fs_mark_generic_bitmap(bmap, *blocknr);
	
	i=0;
	while  ((!(ctx->buf[i++])) && (i<8)) ;
	if (i==8)
		return 0;  //Interior nodes of an htree which the full length of a data block
		 

	while (offset < fs->blocksize) {
                dirent = (struct ext2_dir_entry *) (ctx->buf + offset);
                if (ext2fs_get_rec_len(fs, dirent, &rec_len))
                        return BLOCK_ABORT;
                if (((offset + rec_len) > fs->blocksize) ||
                    (rec_len < 8) ||
                    ((rec_len % 4) != 0) ||
                    ((((unsigned) dirent->name_len & 0xFF)+8) > rec_len)) {

                        ctx->errcode = EXT2_ET_DIR_CORRUPTED;
                        return BLOCK_ABORT;
                }
                if (!dirent->inode && 
                    !(ctx->flags & DIRENT_FLAG_INCLUDE_EMPTY))
                        goto next;

                ret = (ctx->func)(ctx->dir,
                                  (next_real_entry > offset) ?
                                  DIRENT_DELETED_FILE : entry,
                                  dirent, offset, 
                                  fs->blocksize, ctx->buf,
                                  ctx->priv_data);
                if (entry < DIRENT_OTHER_FILE)
                        entry++;

                if (ret & DIRENT_CHANGED) {
                        if (ext2fs_get_rec_len(fs, dirent, &rec_len))
                                return BLOCK_ABORT;
                        changed++;
                }
                if (ret & DIRENT_ABORT) {
                        do_abort++;
                        break;
                }
next:
                if (next_real_entry == offset)
                        next_real_entry += rec_len;

                if (ctx->flags & DIRENT_FLAG_INCLUDE_REMOVED) {
                        size = ((dirent->name_len & 0xFF) + 11) & ~3;
			if (ctx->flags & SKIP_HTREE){
				if ((! *(__u32*)(ctx->buf + (offset+size))) && 
					(*(ctx->buf + (offset+size+5)) == 0x08) && ( !(*(ctx->buf + (offset+size+7))))){
					ctx->flags &= ~SKIP_HTREE;
					break;
				}
			}

                        if (rec_len != size)  {
                                unsigned int final_offset = 0;

                                final_offset = offset + rec_len;
                                offset += size;
                                while (offset < final_offset &&
                                       !local_validate_entry(fs, ctx->buf,
                                                              offset,
                                                              final_offset))
                                        offset += 4;
                                continue;
                        }
                }
                offset += rec_len;
        }

        if (changed) {
                ctx->errcode = ext2fs_write_dir_block(fs, *blocknr, ctx->buf);
                if (ctx->errcode)
                        return BLOCK_ABORT;
        }
        if (do_abort)
                return BLOCK_ABORT;
        return 0;
}




//------------------------------------------------------
// Sub function for search the path, use from get_dir3()
static int find_dir_proc(ext2_ino_t dir EXT2FS_ATTR((unused)),
                         int    entry,
                         struct ext2_dir_entry *dirent,
                         int    offset EXT2FS_ATTR((unused)),
                         int    blocksize EXT2FS_ATTR((unused)),
                         char   *buf EXT2FS_ATTR((unused)),
                         void   *private)
{
        ext2_ino_t              ino;
        char                    name[EXT2_NAME_LEN + 1];
        int                     thislen;
        struct priv_dir_iterate_struct *fl = (struct priv_dir_iterate_struct *) private;

        thislen = ((dirent->name_len & 0xFF) < EXT2_NAME_LEN) ?
                (dirent->name_len & 0xFF) : EXT2_NAME_LEN;
	if (thislen){
        	strncpy(name, dirent->name, thislen);
        	name[thislen] = '\0';
        	ino = dirent->inode;

		if ((ino) && (ino <= current_fs->super->s_inodes_count))
			add_list_item(fl->dir_list,ino,name,(char) entry);
	}
        return 0;
}




//-------------------------------------------------------
// local directory iterate for use inode  
static errcode_t local_dir_iterate3(ext2_filsys fs,
                              ext2_ino_t dir,
			      struct ext2_inode *inode,
                              int flags,
                              char *block_buf,
                              int (*func)(ext2_ino_t    dir,
                                          int           entry,
                                          struct ext2_dir_entry *dirent,
                                          int   offset,
                                          int   blocksize,
                                          char  *buf,
                                          void  *priv_data),
                              void *priv_data)
{
        struct          dir_context     ctx;
        errcode_t       retval;

        EXT2_CHECK_MAGIC(fs, EXT2_ET_MAGIC_EXT2FS_FILSYS);

        ctx.dir = dir;
        ctx.flags = flags;
        if (block_buf)
                ctx.buf = block_buf;
        else {
                retval = ext2fs_get_mem(fs->blocksize, &ctx.buf);
                if (retval)
                        return retval;
        }
        ctx.func = func;
        ctx.priv_data = priv_data;
        ctx.errcode = 0;
        retval = local_block_iterate3(fs, *inode, BLOCK_FLAG_READ_ONLY, 0,
                                       local_process_dir_block, &ctx);
        if (!block_buf)
                ext2fs_free_mem(&ctx.buf);
        if (retval)
                return retval;
        return ctx.errcode;
}





// list dir over journal inode use real fs dir blocks an real dir-entry-inodes
// function current not used
void list_dir2(ext2_ino_t ino, struct ext2_inode *inode)
{
        int             retval;
        int             flags; 
        struct priv_dir_iterate_struct ls;

        ls.options = 0;
        if (!ino || (! LINUX_S_ISDIR(inode->i_mode)))  return;

        ls.options |= LONG_OPT;
        ls.options |= DELETED_OPT;
	if (! ext2fs_test_inode_bitmap ( current_fs->inode_map, ino )) ls.options |= DELETED_DIR ;

        flags = DIRENT_FLAG_INCLUDE_EMPTY;
        if (ls.options & DELETED_OPT) flags |= DIRENT_FLAG_INCLUDE_REMOVED;
// not testet        flags = (inode->i_flags & EXT2_INDEX_FL) ? SKIP_HTREE : 0 ;

        retval = local_dir_iterate3(current_fs,ino, inode, flags,0, list_dir_proc, &ls);
        fprintf(stdout, "\n");
        if (retval)
                fprintf(stderr,"Error: %d inode or directory data error\n", retval);

        return;
}


//--------------------------------------------------
// local sub function 
// load dir-index in a dir_list 
 struct dir_list_head_t* get_dir3(struct ext2_inode* d_inode,ext2_ino_t path_ino, ext2_ino_t ino,
                                  char* path, char* name, 
				  __u32 t_after , __u32 t_before,
				   int options )
{
        int             		retval;
        int             		flags; 
	struct ext2_inode* 		inode;
	struct ring_buf 		*i_list = NULL;
	r_item 				*item = NULL;
	struct priv_dir_iterate_struct 	fl;
	struct dir_list_head_t * 	l_dir = NULL;
		
		if(d_inode) memset(d_inode, 0 , current_fs->super->s_inode_size);
		i_list = (struct ring_buf*) get_j_inode_list(current_fs->super, ino);
		if (! i_list) return NULL;
//FIXME
		if (imap){
			ext2fs_mark_generic_bitmap(imap,ino);
//			printf("mark inode %10u\n",ino);
		}
		item = get_undel_inode(i_list , t_after , t_before);
		if ( item && item->inode ) {
			inode = (struct ext2_inode*)item->inode;
			if(d_inode) 
				memcpy(d_inode,inode,current_fs->super->s_inode_size);	
			if (!ino || (! LINUX_S_ISDIR(inode->i_mode)))
				goto errout;
			
//get dir
			l_dir = new_dir_list ( path_ino,ino, path, name);
			if (l_dir)
				{
				fl.options = options;
				fl.dir_list = l_dir;
				fl.transaction = &item->transaction;

//        			flags = DIRENT_FLAG_INCLUDE_EMPTY;
				flags = 0;
				flags = (inode->i_flags & EXT2_INDEX_FL) ? SKIP_HTREE : 0 ; 
				if (options & DELETED_OPT ) flags |= DIRENT_FLAG_INCLUDE_REMOVED;
				retval = local_dir_iterate3(current_fs,ino, inode, flags,0, find_dir_proc, &fl);
			 	if (retval )
               				fprintf(stderr,"Warning: error-NR %d can not found file: %s/%s\n", retval, path, name);
			}
		}
 		
errout:
	if (i_list)
		ring_del(i_list);
        return (l_dir) ? clean_up_dir_list(l_dir) : NULL ;
}



// main worker function for recover and list
// lookup_local() : search recursiv through the filesystem 
// use inode and dirblocks from journal
// flag = 0 only undelete, flag = 2 process all
void lookup_local(char* des_dir, struct dir_list_head_t * dir, __u32 t_after , __u32 t_before, int flag){
 	struct dir_list_head_t *d_list = NULL;
	struct dir_list_t 	*lp;
	struct ext2_inode* 		inode = NULL;
	struct ring_buf 		*i_list = NULL;
	r_item 				*item = NULL;
	char			c = ' ';
	int allocated;
	int recursion = 0;
		
	if (! dir) {
		d_list = get_dir3(NULL, EXT2_ROOT_INO , EXT2_ROOT_INO , "","", t_after,t_before, flag);
		if (!d_list) return;
//recursion
		lookup_local(des_dir,d_list, t_after, t_before, flag);
	}
	else {	
#ifdef DEBUG
		printf(">>%s>>\n",dir->pathname);
#endif
		if (flag & DOUPLE_QUOTES_LIST)
			c = '"';
		lp = GET_FIRST(dir);
		inode = malloc(current_fs->super->s_inode_size);
		while (lp){
			if (! strcmp(lp->filename,"..")) {
				if ((dir->path_inode != lp->inode_nr) && (dir->path_inode != 0)) break;
				lp = GET_NEXT(dir,lp);
				continue;
			}
			if (! strcmp(lp->filename,".")) 
			{
// prefunction for directory
					switch (flag & REC_FILTER){
						case LIST_ALL :
							if (dir->dir_inode != lp->inode_nr) break;
							printf("DIR	%lu	%c%s%c\n",(long unsigned int)lp->inode_nr,c,dir->pathname,c);
						break;
						case LIST_STATUS :
						break;
						case RECOV_DEL	:
						break;
						case RECOV_ALL	:
							//create_all_dir(des_dir,dir->pathname, lp->inode_nr);
						break;
						case HIST_DIR :
							add_coll_list(lp->inode_nr);
						break;
					}
				
			}
			else{
				d_list = get_dir3(inode, dir->dir_inode,lp->inode_nr,
					dir->pathname,lp->filename,t_after,t_before, flag);

				if (d_list){
//FIXME search for lost dir
					recursion = 1;
					if (flag & LOST_DIR_SEARCH) 
						 recursion = check_find_dir(des_dir,lp->inode_nr,dir->pathname,lp->filename);

//recursion for directory
					if(recursion) {
						lookup_local(des_dir, d_list, t_after, t_before, flag);
#ifdef DEBUG
						printf("<<%s<<\n",dir->pathname);
#endif
					}
				}
				else{
//function for all files apart from dir
					switch (flag & REC_FILTER){
						case LIST_ALL :
							printf("---	%lu	%c%s%s%s%c\n",(long unsigned int)lp->inode_nr,c,dir->pathname,
								((strlen(dir->pathname) > 0) && strcmp(dir->pathname,"/")) ? "/" : "",
								lp->filename,c);
							break;
						case LIST_STATUS :
							allocated = check_file_recover(inode);
							if (allocated)
							printf("%5u%%	%c%s%s%s%c\n",allocated,c,dir->pathname,
								((strlen(dir->pathname) > 0) && strcmp(dir->pathname,"/")) ? "/" : "",
								lp->filename,c);	
						break;
						case RECOV_DEL	:
							if (inode->i_links_count)
								recover_file(des_dir,dir->pathname, lp->filename, inode, lp->inode_nr,1);
						break;
						case RECOV_ALL	:
							if (inode->i_links_count)
								recover_file(des_dir,dir->pathname, lp->filename, inode, lp->inode_nr,0);
						break;
						case HIST_DIR :
							add_coll_list(lp->inode_nr);
						break;
					}

				}
				if (d_list) clear_dir_list(d_list);
				d_list = NULL;
			}
			lp = GET_NEXT(dir,lp);
		}

		//if (inode) 
		free(inode);

		
// postfunction for directory
		switch (flag & REC_FILTER){
			case LIST_ALL :
			break;
			case LIST_STATUS :
			break;
			case RECOV_DEL	:
			break;
			case RECOV_ALL	:
				i_list = get_j_inode_list(current_fs->super, dir->dir_inode);
				if (! i_list) break;
				item = get_undel_inode(i_list , t_after , t_before);
				if (item) 
					set_dir_attributes(des_dir, dir->pathname,(struct ext2_inode*)item->inode);
				if (i_list) ring_del(i_list);
			break;
			case HIST_DIR :
			break;
		}

	}
if(d_list) 
	clear_dir_list(d_list);
}



// list dir over journal inode and journal dir blocks an journal dir-entry-inodes
void list_dir3(ext2_ino_t ino, struct ext2_inode *inode, trans_range_t* transaction ,__u32 time_stamp)
{
        int             retval;
        int             flags; 
        struct priv_dir_iterate_struct ls;

        ls.options = 0;
        if (!ino || (! LINUX_S_ISDIR(inode->i_mode)))  return;

        ls.options |= LONG_OPT;
        ls.options |= DELETED_OPT;
	ls.options |= ONLY_JOURNAL;
	ls.time_stamp = time_stamp;
	
	if (! ext2fs_test_inode_bitmap ( current_fs->inode_map, ino )) ls.options |= DELETED_DIR ;

	ls.transaction = transaction;

        flags = DIRENT_FLAG_INCLUDE_EMPTY + ONLY_JOURNAL;

        if (ls.options & DELETED_OPT) flags |= DIRENT_FLAG_INCLUDE_REMOVED;
	flags |= (inode->i_flags & EXT2_INDEX_FL) ? SKIP_HTREE : 0 ; 

        retval = local_dir_iterate3(current_fs,ino, inode, flags,0, list_dir_proc, &ls);
        fprintf(stdout, "\n");
        if (retval)
                fprintf(stderr,"Error %d inode or journal directory data error\n", retval);

        return;
}



// search the Inode for an pathname (recursiv function)
ext2_ino_t local_namei(struct dir_list_head_t * dir, char *path, __u32 t_after , __u32 t_before, int flag){
 	struct dir_list_head_t *d_list = NULL;
	struct dir_list_t 	*lp;
	ext2_ino_t ret_inode = 0;
	char *p_path = NULL;
	char *p1;
	char c;
	char *p2;
	
	if (! dir) {
		d_list = get_dir3(NULL,EXT2_ROOT_INO , EXT2_ROOT_INO , "","", t_after,t_before, flag);
		if (!d_list) {
			fprintf(stderr,"no rootdir at filesystem found\n");
			return 0;
		}
		ret_inode = local_namei(d_list, path, t_after, t_before, flag);
	}
	else {	
// Check end recursion
		if (!(strlen(path))) {
			ret_inode = dir->dir_inode;
			goto out;
		}

		p_path = (char*) malloc(strlen(path)+1);
		p2 = p_path;
		p1 = path + strspn(path,"/");
		c = *p1;
		while ( c != '/' ){
			*p2++ = c;
			if (c == 0) break;
			p1++;
			c = *p1;
		}
		if ( c ) *p2 = 0;
		
		lp = GET_FIRST(dir);
		while (lp){
			if (! strcmp(lp->filename,"..")) {
				if (dir->path_inode != lp->inode_nr) {
					break;
				}
			}
			if (! strcmp(lp->filename,".")) 
			{
				if (dir->dir_inode != lp->inode_nr) {
					break;
				} 
			}
			else{
				if (strcmp( p_path , lp->filename)) {
					lp = GET_NEXT(dir,lp);
					continue; 
				}
				d_list = get_dir3(NULL,dir->dir_inode,lp->inode_nr,dir->pathname,lp->filename,t_after,t_before, flag);
				if (d_list){
					ret_inode = local_namei(d_list, p1, t_after, t_before, flag);
// break if found			
					if (ret_inode) goto out;
				}
				else{
// recursions end (file find)
					ret_inode = lp->inode_nr;
					goto out;
				}
			}
			lp = GET_NEXT(dir,lp);
		}
	}
out:
if(p_path)
	free(p_path);
if(d_list) 
	clear_dir_list(d_list);

return ret_inode;
}
