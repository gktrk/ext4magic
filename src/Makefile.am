sbin_PROGRAMS = ext4magic
ext4magic_SOURCES = block.c dir_list.c ext4magic.c extent_db.c file_type.c \
	hard_link_stack.c imap_search.c inode.c journal.c lookup_local.c magic_block_scan.c \
	recover.c ring_buf.c util.c

# set the include path found by configure
INCLUDES= $(all_includes)

man_MANS = ext4magic.8

# the library search path.
ext4magic_CFLAGS = -O2 -g -D_FILE_OFFSET_BITS=64
ext4magic_LDFLAGS = -lext2fs -le2p -luuid -lblkid -lmagic -lz -lbz2
noinst_HEADERS = block.h dir_list.h ext2fsP.h ext4magic.8 ext4magic.h \
	extent_db.h hard_link_stack.h inode.h jfs_compat.h jfs_user.h journal.h \
	kernel-jbd.h kernel-list.h magic.h ring_buf.h util.h
