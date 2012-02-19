#ifndef BLOCK_H
#define BLOCK_H
 
#include <ext2fs/ext2fs.h>



errcode_t local_block_iterate3(ext2_filsys fs,
				struct ext2_inode inode,
				int	flags,
				char *block_buf,
				int (*func)(ext2_filsys fs,
					    blk64_t	*blocknr,
					    e2_blkcnt_t	blockcnt,
					    blk64_t	ref_blk,
					    int		ref_offset,
					    void	*priv_data),
				void *priv_data);




int read_block ( ext2_filsys, blk_t*, void* ); //read filesystem block
#ifdef EXT2_FLAG_64BITS
int read_block64 ( ext2_filsys, blk64_t*, void* ); //read filesystem block64
#endif
errcode_t local_ext2fs_extent_open(ext2_filsys, struct ext2_inode, ext2_extent_handle_t*);
void local_ext2fs_extent_free(ext2_extent_handle_t);

#endif //BLOCK_H


