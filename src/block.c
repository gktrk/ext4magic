/*
 * This file was modified from e2fsprogs 1.41.4
 * Use this file when compiling against a newer version of ext2fs headers.
 * block.c --- iterate over all blocks in an inode
 * extent.c --- work with extents
 *
 * Copyright (C) 1993, 1994, 1995, 1996 Theodore Ts'o.
 *
 * %Begin-Header%
 * This file may be redistributed under the terms of the GNU Public
 * License.
 * %End-Header%
 */

/*
 This is a workaround to allow compilation, but the one line that uses
 this constant will never run because we open the fs read-only.
*/ 
#define EXT4INO  0

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#ifndef EXT2_FLAT_INCLUDES
#define EXT2_FLAT_INCLUDES 0
#endif


#include "ext2fsP.h"
#include "block.h"
#include "journal.h"
#include "util.h"

extern ext2fs_block_bitmap 	  	bmap ; 

struct block_context {
	ext2_filsys	fs;
	int (*func)(ext2_filsys	fs,
		    blk64_t	*blocknr,
		    e2_blkcnt_t	bcount,
		    blk64_t	ref_blk,
		    int		ref_offset,
		    void	*priv_data);
	e2_blkcnt_t	bcount;
	int		bsize;
	int		flags;
	errcode_t	errcode;
	char	*ind_buf;
	char	*dind_buf;
	char	*tind_buf;
	void	*priv_data;
};


//#ifdef BLOCK_FLAG_READ_ONLY

//#include <ext2fs/ext3_extents.h>
struct extent_path {
        char            *buf;
        int             entries;
        int             max_entries;
        int             left;
        int             visit_num;
        int             flags;
        blk64_t         end_blk;
        void            *curr;
};


struct ext2_extent_handle {
        errcode_t               magic;
        ext2_filsys             fs;
        ext2_ino_t              ino;
        struct ext2_inode       *inode;
#ifdef EXT2_FLAG_64BITS
	struct ext2_inode       inodebuf;
#endif
        int                     type;
        int                     level;
        int                     max_depth;
        struct extent_path      *path;
};



// Leave the inode intact 
void local_ext2fs_extent_free(ext2_extent_handle_t handle)
{
        int                     i;

        if (!handle)
                return;

//        if (handle->inode)
//                ext2fs_free_mem(&handle->inode);
        if (handle->path) {
                for (i=1; i <= handle->max_depth; i++) {
                        if (handle->path[i].buf)
                                ext2fs_free_mem(&handle->path[i].buf);
                }
                ext2fs_free_mem(&handle->path);
        }
        ext2fs_free_mem(&handle);
}


// ------------------------------------------------------------------------
// This function inserts the data from block blocknr[0] into the input buffer
// 'buf'.  The data from the block is inserted into
// the input buffer beginning at location 'blockcnt'.
// NOTE:  The output returned by this command should be corrected to the proper
//    endianness for the host cpu when reading multi-byte structures from disk.
int read_block ( ext2_filsys fs, blk_t *blocknr, void *buf )
{
        errcode_t retval = io_channel_read_blk ( fs->io,  *blocknr,  1,  buf  );
        if (retval)
                fprintf(stderr,"Error %d while read block\n", (int)retval);
        return retval;
} 
#ifdef EXT2_FLAG_64BITS
int read_block64 ( ext2_filsys fs, blk64_t *blocknr, void *buf )
{
        errcode_t retval = io_channel_read_blk64 ( fs->io,  *blocknr,  1,  buf  );
        if (retval)
                fprintf(stderr,"Error %d while read block\n", (int)retval);
        return retval;
}
#endif




errcode_t local_ext2fs_extent_open(ext2_filsys fs, struct ext2_inode inode,
                          ext2_extent_handle_t *ret_handle) {

        struct ext2_extent_handle       *handle;
        struct ext3_extent_header       *eh;
        int 				i;
        errcode_t                       retval;
        retval = ext2fs_get_mem(sizeof(struct ext2_extent_handle), &handle);
        if (retval)
                return retval;
        memset(handle, 0, sizeof(struct ext2_extent_handle));

        handle->ino = 0;
        handle->fs = fs;
        handle->inode = &inode;

        eh = (struct ext3_extent_header *) &handle->inode->i_block[0];
        for (i=0; i < EXT2_N_BLOCKS; i++)
                if (handle->inode->i_block[i])
                        break;
        if (i >= EXT2_N_BLOCKS) {
                eh->eh_magic = ext2fs_cpu_to_le16(EXT3_EXT_MAGIC);
                eh->eh_depth = 0;
                eh->eh_entries = 0;
                i = (sizeof(handle->inode->i_block) - sizeof(*eh)) /
                        sizeof(struct ext3_extent);
                eh->eh_max = ext2fs_cpu_to_le16(i);
                handle->inode->i_flags |= EXT4_EXTENTS_FL;
        }


        handle->max_depth = ext2fs_le16_to_cpu(eh->eh_depth);
        handle->type = ext2fs_le16_to_cpu(eh->eh_magic);

        retval = ext2fs_get_mem(((handle->max_depth+1) *
                                 sizeof(struct extent_path)),
                                &handle->path);
        memset(handle->path, 0,
               (handle->max_depth+1) * sizeof(struct extent_path));
        handle->path[0].buf = (char *) handle->inode->i_block;

        handle->path[0].left = handle->path[0].entries =
                ext2fs_le16_to_cpu(eh->eh_entries);
        handle->path[0].max_entries = ext2fs_le16_to_cpu(eh->eh_max);
        handle->path[0].curr = 0;
        handle->path[0].end_blk =
                ((((__u64) handle->inode->i_size_high << 32) +
                  handle->inode->i_size + (fs->blocksize - 1))
                 >> EXT2_BLOCK_SIZE_BITS(fs->super));
        handle->path[0].visit_num = 1;
        handle->level = 0;
        handle->magic = EXT2_ET_MAGIC_EXTENT_HANDLE;

        *ret_handle = handle;
	//free(handle);

        return 0;
}


static int mark_extent_block(ext2_filsys fs, char *extent_block ){
	struct ext3_extent_header 	*eh;
	struct ext3_extent_idx		*idx;
	int 				i, ret = 0 ;
#ifdef EXT2_FLAG_64BITS
	blk64_t index_bl;
#else
	blk_t index_bl;
#endif
	char *buf = NULL;

	eh = (struct ext3_extent_header*) extent_block;
	if (eh->eh_magic != ext2fs_cpu_to_le16(EXT3_EXT_MAGIC))
				return 1; 
	if (ext2fs_le16_to_cpu(eh->eh_depth)) {
		for (i = 1; ((i <= ext2fs_le16_to_cpu(eh->eh_entries)) && (i <= ext2fs_le16_to_cpu(eh->eh_max))); i++){
			idx = (struct ext3_extent_idx*) &(extent_block[12 * i]);
#ifdef EXT2_FLAG_64BITS
			index_bl = ext2fs_le32_to_cpu(idx->ei_leaf) +
	                       ((__u64) ext2fs_le16_to_cpu(idx->ei_leaf_hi) << 32);
#else
			index_bl = ext2fs_le32_to_cpu(idx->ei_leaf);
#endif
			if (index_bl && index_bl <= fs->super->s_blocks_count ){ 
				if (bmap){
#ifdef EXT2_FLAG_64BITS
					ext2fs_mark_generic_bmap(bmap, index_bl);
#else
					ext2fs_mark_generic_bitmap(bmap, index_bl);
#endif
					buf = malloc(fs->blocksize);
					if (buf){
#ifdef EXT2_FLAG_64BITS
						ret = read_block64 (fs, &index_bl, buf );
#else
						ret = read_block (fs, &index_bl, buf );
#endif
						if (!ret)
							ret = mark_extent_block(fs,buf);
						free(buf);
					}
				}			
			}
		} 
	}
return ret;
}



#define check_for_ro_violation_return(ctx, ret)				\
	do {								\
		if (((ctx)->flags & BLOCK_FLAG_READ_ONLY) &&		\
		    ((ret) & BLOCK_CHANGED)) {				\
			(ctx)->errcode = EXT2_ET_RO_BLOCK_ITERATE;	\
			ret |= BLOCK_ABORT | BLOCK_ERROR;		\
			return ret;					\
		}							\
	} while (0)

#define check_for_ro_violation_goto(ctx, ret, label)			\
	do {								\
		if (((ctx)->flags & BLOCK_FLAG_READ_ONLY) &&		\
		    ((ret) & BLOCK_CHANGED)) {				\
			(ctx)->errcode = EXT2_ET_RO_BLOCK_ITERATE;	\
			ret |= BLOCK_ABORT | BLOCK_ERROR;		\
			goto label;					\
		}							\
	} while (0)

static int block_iterate_ind(blk_t *ind_block, blk_t ref_block,
			     int ref_offset, struct block_context *ctx)
{
	int	ret = 0, changed = 0;
	int	i, flags, limit, offset;
	blk_t	*block_nr;
	blk64_t	blk64;

	limit = ctx->fs->blocksize >> 2;
	if (!(ctx->flags & BLOCK_FLAG_DEPTH_TRAVERSE) &&
	    !(ctx->flags & BLOCK_FLAG_DATA_ONLY)){
		blk64 = *ind_block;
		ret = (*ctx->func)(ctx->fs, &blk64,
				   BLOCK_COUNT_IND, ref_block,
				   ref_offset, ctx->priv_data);
		*ind_block = blk64;
	}
	check_for_ro_violation_return(ctx, ret);
	if (!*ind_block || (ret & BLOCK_ABORT)) {
		ctx->bcount += limit;
		return ret;
	}
	if (*ind_block >= ctx->fs->super->s_blocks_count ||
	    *ind_block < ctx->fs->super->s_first_data_block) {
		ctx->errcode = EXT2_ET_BAD_IND_BLOCK;
		ret |= BLOCK_ERROR;
		return ret;
	}
	ctx->errcode = ext2fs_read_ind_block(ctx->fs, *ind_block,
					     ctx->ind_buf);
	if (ctx->errcode) {
		ret |= BLOCK_ERROR;
		return ret;
	}
	if (bmap)
		ext2fs_mark_generic_bitmap(bmap, *ind_block);
	
	block_nr = (blk_t *) ctx->ind_buf;
	offset = 0;
	if (ctx->flags & BLOCK_FLAG_APPEND) {
		for (i = 0; i < limit; i++, ctx->bcount++, block_nr++) {
			blk64 = *block_nr;
			flags = (*ctx->func)(ctx->fs, &blk64 , ctx->bcount,
					     *ind_block, offset,
					     ctx->priv_data);
			*block_nr = blk64;
			changed	|= flags;
			if (flags & BLOCK_ABORT) {
				ret |= BLOCK_ABORT;
				break;
			}
			offset += sizeof(blk_t);
		}
	} else {
		for (i = 0; i < limit; i++, ctx->bcount++, block_nr++) {
			if (*block_nr == 0)
				continue;
			blk64 = *block_nr;
			flags = (*ctx->func)(ctx->fs, &blk64, ctx->bcount,
					     *ind_block, offset,
					     ctx->priv_data);
			*block_nr = blk64;
			changed	|= flags;
			if (flags & BLOCK_ABORT) {
				ret |= BLOCK_ABORT;
				break;
			}
			offset += sizeof(blk_t);
		}
	}
	check_for_ro_violation_return(ctx, changed);
	if (changed & BLOCK_CHANGED) {
		ctx->errcode = ext2fs_write_ind_block(ctx->fs, *ind_block,
						      ctx->ind_buf);
		if (ctx->errcode)
			ret |= BLOCK_ERROR | BLOCK_ABORT;
	}
	if ((ctx->flags & BLOCK_FLAG_DEPTH_TRAVERSE) &&
	    !(ctx->flags & BLOCK_FLAG_DATA_ONLY) &&
	    !(ret & BLOCK_ABORT)){
		blk64 = *ind_block;
		ret |= (*ctx->func)(ctx->fs, &blk64 ,
				    BLOCK_COUNT_IND, ref_block,
				    ref_offset, ctx->priv_data);
		*ind_block = blk64;
	}
	check_for_ro_violation_return(ctx, ret);
	return ret;
}



static int block_iterate_dind(blk_t *dind_block, blk_t ref_block,
			      int ref_offset, struct block_context *ctx)
{
	int	ret = 0, changed = 0;
	int	i, flags, limit, offset;
	blk_t	*block_nr;
	blk64_t blk64;

	limit = ctx->fs->blocksize >> 2;
	if (!(ctx->flags & (BLOCK_FLAG_DEPTH_TRAVERSE |
			    BLOCK_FLAG_DATA_ONLY))){
		blk64 = *dind_block;
		ret = (*ctx->func)(ctx->fs, &blk64 ,
				   BLOCK_COUNT_DIND, ref_block,
				   ref_offset, ctx->priv_data);
		*dind_block = blk64;
	}
	check_for_ro_violation_return(ctx, ret);
	if (!*dind_block || (ret & BLOCK_ABORT)) {
		ctx->bcount += limit*limit;
		return ret;
	}
	if (*dind_block >= ctx->fs->super->s_blocks_count ||
	    *dind_block < ctx->fs->super->s_first_data_block) {
		ctx->errcode = EXT2_ET_BAD_DIND_BLOCK;
		ret |= BLOCK_ERROR;
		return ret;
	}
	ctx->errcode = ext2fs_read_ind_block(ctx->fs, *dind_block,
					     ctx->dind_buf);
	if (ctx->errcode) {
		ret |= BLOCK_ERROR;
		return ret;
	}
	if (bmap)
		ext2fs_mark_generic_bitmap(bmap, *dind_block);
	
	block_nr = (blk_t *) ctx->dind_buf;
	offset = 0;
	if (ctx->flags & BLOCK_FLAG_APPEND) {
		for (i = 0; i < limit; i++, block_nr++) {
			flags = block_iterate_ind(block_nr,
						  *dind_block, offset,
						  ctx);
			changed |= flags;
			if (flags & (BLOCK_ABORT | BLOCK_ERROR)) {
				ret |= flags & (BLOCK_ABORT | BLOCK_ERROR);
				break;
			}
			offset += sizeof(blk_t);
		}
	} else {
		for (i = 0; i < limit; i++, block_nr++) {
			if (*block_nr == 0) {
				ctx->bcount += limit;
				continue;
			}
			flags = block_iterate_ind(block_nr,
						  *dind_block, offset,
						  ctx);
			changed |= flags;
			if (flags & (BLOCK_ABORT | BLOCK_ERROR)) {
				ret |= flags & (BLOCK_ABORT | BLOCK_ERROR);
				break;
			}
			offset += sizeof(blk_t);
		}
	}
	check_for_ro_violation_return(ctx, changed);
	if (changed & BLOCK_CHANGED) {
		ctx->errcode = ext2fs_write_ind_block(ctx->fs, *dind_block,
						      ctx->dind_buf);
		if (ctx->errcode)
			ret |= BLOCK_ERROR | BLOCK_ABORT;
	}
	if ((ctx->flags & BLOCK_FLAG_DEPTH_TRAVERSE) &&
	    !(ctx->flags & BLOCK_FLAG_DATA_ONLY) &&
	    !(ret & BLOCK_ABORT)){
		blk64 = *dind_block;
		ret |= (*ctx->func)(ctx->fs, &blk64,
				    BLOCK_COUNT_DIND, ref_block,
				    ref_offset, ctx->priv_data);

		*dind_block = blk64;
	}
	check_for_ro_violation_return(ctx, ret);
	return ret;
}



static int block_iterate_tind(blk_t *tind_block, blk_t ref_block,
			      int ref_offset, struct block_context *ctx)
{
	int	ret = 0, changed = 0;
	int	i, flags, limit, offset;
	blk_t	*block_nr;
	blk64_t blk64;

	limit = ctx->fs->blocksize >> 2;
	if (!(ctx->flags & (BLOCK_FLAG_DEPTH_TRAVERSE |
			    BLOCK_FLAG_DATA_ONLY))){
		blk64 = *tind_block;
		ret = (*ctx->func)(ctx->fs, &blk64 ,
				   BLOCK_COUNT_TIND, ref_block,
				   ref_offset, ctx->priv_data);
		*tind_block = blk64;
	}
	check_for_ro_violation_return(ctx, ret);
	if (!*tind_block || (ret & BLOCK_ABORT)) {
		ctx->bcount += limit*limit*limit;
		return ret;
	}
	if (*tind_block >= ctx->fs->super->s_blocks_count ||
	    *tind_block < ctx->fs->super->s_first_data_block) {
		ctx->errcode = EXT2_ET_BAD_TIND_BLOCK;
		ret |= BLOCK_ERROR;
		return ret;
	}
	ctx->errcode = ext2fs_read_ind_block(ctx->fs, *tind_block,
					     ctx->tind_buf);
	if (ctx->errcode) {
		ret |= BLOCK_ERROR;
		return ret;
	}
	if (bmap)
		ext2fs_mark_generic_bitmap(bmap, *tind_block);

	block_nr = (blk_t *) ctx->tind_buf;
	offset = 0;
	if (ctx->flags & BLOCK_FLAG_APPEND) {
		for (i = 0; i < limit; i++, block_nr++) {
			flags = block_iterate_dind(block_nr,
						   *tind_block,
						   offset, ctx);
			changed |= flags;
			if (flags & (BLOCK_ABORT | BLOCK_ERROR)) {
				ret |= flags & (BLOCK_ABORT | BLOCK_ERROR);
				break;
			}
			offset += sizeof(blk_t);
		}
	} else {
		for (i = 0; i < limit; i++, block_nr++) {
			if (*block_nr == 0) {
				ctx->bcount += limit*limit;
				continue;
			}
			flags = block_iterate_dind(block_nr,
						   *tind_block,
						   offset, ctx);
			changed |= flags;
			if (flags & (BLOCK_ABORT | BLOCK_ERROR)) {
				ret |= flags & (BLOCK_ABORT | BLOCK_ERROR);
				break;
			}
			offset += sizeof(blk_t);
		}
	}
	check_for_ro_violation_return(ctx, changed);
	if (changed & BLOCK_CHANGED) {
		ctx->errcode = ext2fs_write_ind_block(ctx->fs, *tind_block,
						      ctx->tind_buf);
		if (ctx->errcode)
			ret |= BLOCK_ERROR | BLOCK_ABORT;
	}
	if ((ctx->flags & BLOCK_FLAG_DEPTH_TRAVERSE) &&
	    !(ctx->flags & BLOCK_FLAG_DATA_ONLY) &&
	    !(ret & BLOCK_ABORT)){
		blk64 = *tind_block;
		ret |= (*ctx->func)(ctx->fs, &blk64,
				    BLOCK_COUNT_TIND, ref_block,
				    ref_offset, ctx->priv_data);
		*tind_block = blk64;
	}
	check_for_ro_violation_return(ctx, ret);
	return ret;
}



errcode_t local_block_iterate3(ext2_filsys fs,
				struct ext2_inode inode, // ext2_ino_t ino,
				int	flags,
				char *block_buf,
				int (*func)(ext2_filsys fs,
					    blk64_t	*blocknr,
					    e2_blkcnt_t	blockcnt,
					    blk64_t	ref_blk,
					    int		ref_offset,
					    void	*priv_data),
				void *priv_data)
{
	int	i;
	int	r, ret = 0;
	// struct ext2_inode inode;
	errcode_t	retval;
	struct block_context ctx;
	int	limit;
	blk64_t blk64;

	EXT2_CHECK_MAGIC(fs, EXT2_ET_MAGIC_EXT2FS_FILSYS);
        ctx.errcode = 0;
	/*ctx.errcode = ext2fs_read_inode(fs, ino, &inode);
	if (ctx.errcode)
		return ctx.errcode;
	*/

	/*
	 * Check to see if we need to limit large files
	 */
	if (flags & BLOCK_FLAG_NO_LARGE) {
		if (!LINUX_S_ISDIR(inode.i_mode) &&
		    (inode.i_size_high != 0))
			return EXT2_ET_FILE_TOO_BIG;
	}

	limit = fs->blocksize >> 2;

	ctx.fs = fs;
	ctx.func = func;
	ctx.priv_data = priv_data;
	ctx.flags = flags;
	ctx.bcount = 0;
	if (block_buf) {
		ctx.ind_buf = block_buf;
	} else {
		retval = ext2fs_get_array(3, fs->blocksize, &ctx.ind_buf);
		if (retval)
			return retval;
	}
	ctx.dind_buf = ctx.ind_buf + fs->blocksize;
	ctx.tind_buf = ctx.dind_buf + fs->blocksize;

	/*
	 * Iterate over the HURD translator block (if present)
	 */
	if ((fs->super->s_creator_os == EXT2_OS_HURD) &&
	    !(flags & BLOCK_FLAG_DATA_ONLY)) {
		if (inode.osd1.hurd1.h_i_translator) {
			blk64 = inode.osd1.hurd1.h_i_translator;
			ret |= (*ctx.func)(fs, &blk64,
					   BLOCK_COUNT_TRANSLATOR,
					   0, 0, priv_data);
			inode.osd1.hurd1.h_i_translator = (blk_t) blk64;
			if (ret & BLOCK_ABORT)
				goto abort_exit;
			check_for_ro_violation_goto(&ctx, ret, abort_exit);
		}
	}

	if (inode.i_flags & EXT4_EXTENTS_FL) {
		ext2_extent_handle_t	handle = NULL;
		struct ext2fs_extent	extent;
		e2_blkcnt_t		blockcnt = 0;
		blk64_t			blk, new_blk;
		int			op = EXT2_EXTENT_ROOT;
		int			uninit;
		unsigned int		j;

		ctx.errcode = local_ext2fs_extent_open(fs, inode, &handle);
		if (ctx.errcode)
			goto abort_exit;

		while (1) {
			ctx.errcode = ext2fs_extent_get(handle, op, &extent);
			if (ctx.errcode) {
				if (ctx.errcode != EXT2_ET_EXTENT_NO_NEXT)
					break;
				ctx.errcode = 0;
				if (!(flags & BLOCK_FLAG_APPEND))
					break;
				blk = 0;
				r = (*ctx.func)(fs, &blk, blockcnt,
						0, 0, priv_data);
				ret |= r;
				check_for_ro_violation_goto(&ctx, ret,
							    extent_errout);
				if (r & BLOCK_CHANGED) {
					ctx.errcode =
						ext2fs_extent_set_bmap(handle,
						       (blk64_t) blockcnt++,
						       (blk64_t) blk, 0);
					if (ctx.errcode || (ret & BLOCK_ABORT))
						break;
					continue;
				}
				break;
			}

			op = EXT2_EXTENT_NEXT;
			blk = extent.e_pblk;
			if (!(extent.e_flags & EXT2_EXTENT_FLAGS_LEAF)) {
				if (ctx.flags & BLOCK_FLAG_DATA_ONLY)
					continue;
				if ((!(extent.e_flags &
				       EXT2_EXTENT_FLAGS_SECOND_VISIT) &&
				     !(ctx.flags & BLOCK_FLAG_DEPTH_TRAVERSE)) ||
				    ((extent.e_flags &
				      EXT2_EXTENT_FLAGS_SECOND_VISIT) &&
				     (ctx.flags & BLOCK_FLAG_DEPTH_TRAVERSE))) {
					ret |= (*ctx.func)(fs, &blk,
							   -1, 0, 0, priv_data);
					if (ret & BLOCK_CHANGED) {
						extent.e_pblk = blk;
						ctx.errcode =
				ext2fs_extent_replace(handle, 0, &extent);
						if (ctx.errcode)
							break;
					}
				}
				continue;
			}
			uninit = 0;
			if (extent.e_flags & EXT2_EXTENT_FLAGS_UNINIT)
				uninit = EXT2_EXTENT_SET_BMAP_UNINIT;
			for (blockcnt = extent.e_lblk, j = 0;
			     j < extent.e_len;
			     blk++, blockcnt++, j++) {
				new_blk = blk;
				r = (*ctx.func)(fs, &new_blk, blockcnt,
						0, 0, priv_data);
				ret |= r;
				check_for_ro_violation_goto(&ctx, ret,
							    extent_errout);
				if (r & BLOCK_CHANGED) {
					ctx.errcode =
						ext2fs_extent_set_bmap(handle,
						       (blk64_t) blockcnt,
						       (blk64_t) new_blk,
						       uninit);
					if (ctx.errcode)
						goto extent_errout;
				}
				if (ret & BLOCK_ABORT)
					break;
			}
		}
	if (bmap)
		mark_extent_block(fs, (char*) inode.i_block);

	extent_errout:
		local_ext2fs_extent_free(handle);
		ret |= BLOCK_ERROR | BLOCK_ABORT;
		goto errout;
	}

	/*
	 * Iterate over normal data blocks
	 */
	for (i = 0; i < EXT2_NDIR_BLOCKS ; i++, ctx.bcount++) {
		if (inode.i_block[i] || (flags & BLOCK_FLAG_APPEND)) {
			blk64 = inode.i_block[i];
			ret |= (*ctx.func)(fs, &blk64 ,
					    ctx.bcount, 0, i, priv_data);
			inode.i_block[i] = (blk_t) blk64;
			if (ret & BLOCK_ABORT)
				goto abort_exit;
		}
	}
	check_for_ro_violation_goto(&ctx, ret, abort_exit);
	if (inode.i_block[EXT2_IND_BLOCK] || (flags & BLOCK_FLAG_APPEND)) {
		ret |= block_iterate_ind(&inode.i_block[EXT2_IND_BLOCK],
					 0, EXT2_IND_BLOCK, &ctx);
		if (ret & BLOCK_ABORT)
			goto abort_exit;
	} else
		ctx.bcount += limit;
	if (inode.i_block[EXT2_DIND_BLOCK] || (flags & BLOCK_FLAG_APPEND)) {
		ret |= block_iterate_dind(&inode.i_block[EXT2_DIND_BLOCK],
					  0, EXT2_DIND_BLOCK, &ctx);
		if (ret & BLOCK_ABORT)
			goto abort_exit;
	} else
		ctx.bcount += limit * limit;
	if (inode.i_block[EXT2_TIND_BLOCK] || (flags & BLOCK_FLAG_APPEND)) {
		ret |= block_iterate_tind(&inode.i_block[EXT2_TIND_BLOCK],
					  0, EXT2_TIND_BLOCK, &ctx);
		if (ret & BLOCK_ABORT){
			goto abort_exit;
		}
	}

abort_exit:
	if (ret & BLOCK_CHANGED) {
		retval = ext2fs_write_inode(fs, EXT4INO, &inode);
		if (retval) {
			if (!block_buf)
				ext2fs_free_mem(&ctx.ind_buf);
			return retval;
		}
	}
errout:
	if (!block_buf)
		ext2fs_free_mem(&ctx.ind_buf);

	return (ret & BLOCK_ERROR) ? ctx.errcode : 0;
}
//___________________________________________________________________________________________________


