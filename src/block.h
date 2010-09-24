#ifndef BLOCK_H
#define BLOCK_H
 
#include <ext2fs/ext2fs.h>



errcode_t local_block_iterate3(ext2_filsys fs,
				struct ext2_inode inode,
				int	flags,
				char *block_buf,
				int (*func)(ext2_filsys fs,
					    blk_t	*blocknr,
					    e2_blkcnt_t	blockcnt,
					    blk_t	ref_blk,
					    int		ref_offset,
					    void	*priv_data),
				void *priv_data);




int read_block ( ext2_filsys, blk_t*, void* ); //read filesystem block
#endif //BLOCK_H
