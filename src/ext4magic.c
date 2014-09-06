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


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <time.h>
#include <errno.h> 

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#else
extern int optind;
extern char *optarg;
#endif

#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

/* ext3/4 libraries */
#include <ext2fs/ext2fs.h>
#include <e2p/e2p.h>

//local header files
#include "util.h"
#include "ext4magic.h"
#include "journal.h"
#include "inode.h"
#include "hard_link_stack.h"
#include "block.h"


char*                           magicfile = NULL;
ext2_filsys     		current_fs = NULL;
ext2_ino_t      		root, cwd;
ext2fs_inode_bitmap 		imap = NULL ;
ext2fs_block_bitmap 	  	bmap = NULL ; 
time_t				now_time ;

//print Versions an CPU-endian-type
void print_version ( void )
{
        printf("ext4magic  version : %s\n",(char*) &VERSION);

        const char *libver;
        ext2fs_get_library_version ( &libver, NULL );
        printf("libext2fs version : %s\n",libver);

        int n = 1 ;
	if ( * ( char * ) &n == 1 )  // True if the cpu is little endian.
                printf("CPU is little endian.\n");
        else
                printf("CPU is big endian.\n");
#ifdef EXPERT_MODE
	printf("Expert Mode is activ\n");
#endif
}


//subfunction for show_super_stats()
#ifdef EXT2_FLAG_64BITS
static void print_bg_opts(ext2_filsys fs, dgrp_t group, int mask,
                          const char *str, int *first, FILE *f)
{
        if (ext2fs_bg_flags_test(fs, group, mask)) {
                if (*first) {
                        fputs("           [", f);
                        *first = 0;
                } else
                        fputs(", ", f);
                fputs(str, f);
        }
}
#else
static void print_bg_opts(struct ext2_group_desc *gdp, int mask,
                          const char *str, int *first, FILE *f)
{
        if (gdp->bg_flags & mask) {
                if (*first) {
                        fputs("           [", f);
                        *first = 0;
                } else
                        fputs(", ", f);
                fputs(str, f);
        }
}
#endif


//print superblock
void show_super_stats(int header_only)
{
#ifdef EXT2_FLAG_64BITS
	const char *units ="block";
#endif
        dgrp_t  i;
        FILE    *out;
        struct ext2_group_desc *gdp;
        int     numdirs = 0, first, gdt_csum;
        out=stdout;

        list_super2(current_fs->super, out);
	if (! header_only) {
                return;
        }

        for (i=0; i < current_fs->group_desc_count; i++)
#ifdef EXT2_FLAG_64BITS 
		numdirs += ext2fs_bg_used_dirs_count(current_fs, i);
#else
                numdirs += current_fs->group_desc[i].bg_used_dirs_count;
#endif
        fprintf(out, "Directories:              %d\n", numdirs);


        gdt_csum = EXT2_HAS_RO_COMPAT_FEATURE(current_fs->super,
                                              EXT4_FEATURE_RO_COMPAT_GDT_CSUM);
#ifdef EXT2_FLAG_64BITS
	for (i = 0; i < current_fs->group_desc_count; i++) {
                fprintf(out, " Group %2d: block bitmap at %llu, "
                        "inode bitmap at %llu, "
                        "inode table at %llu\n"
                        "           %u free %s%s, "
                        "%u free %s, "
                        "%u used %s%s",
                        i, ext2fs_block_bitmap_loc(current_fs, i),
                        ext2fs_inode_bitmap_loc(current_fs, i),
                        ext2fs_inode_table_loc(current_fs, i),
                        ext2fs_bg_free_blocks_count(current_fs, i), units,
                        ext2fs_bg_free_blocks_count(current_fs, i) != 1 ?
                        "s" : "",
                        ext2fs_bg_free_inodes_count(current_fs, i),
                        ext2fs_bg_free_inodes_count(current_fs, i) != 1 ?
                        "inodes" : "inode",
                        ext2fs_bg_used_dirs_count(current_fs, i),
                        ext2fs_bg_used_dirs_count(current_fs, i) != 1 ? "directories"
                                : "directory", gdt_csum ? ", " : "\n");
                if (gdt_csum)
                        fprintf(out, "%u unused %s\n",
                                ext2fs_bg_itable_unused(current_fs, i),
                                ext2fs_bg_itable_unused(current_fs, i) != 1 ?
                                "inodes" : "inode");
                first = 1;
                print_bg_opts(current_fs, i, EXT2_BG_INODE_UNINIT, "Inode not init",
                              &first, out);
                print_bg_opts(current_fs, i, EXT2_BG_BLOCK_UNINIT, "Block not init",
                              &first, out);
                if (gdt_csum) {
                        fprintf(out, "%sChecksum 0x%04x",
                                first ? "           [":", ", ext2fs_bg_checksum(current_fs, i));
                        first = 0;
                }
                if (!first)
                        fputs("]\n", out);
        }
#else
        gdp = &current_fs->group_desc[0];
        for (i = 0; i < current_fs->group_desc_count; i++, gdp++) {
                fprintf(out, " Group %2d: block bitmap at %u, "
                        "inode bitmap at %u, "
                        "inode table at %u\n"
                        "           %d free %s, "
                        "%d free %s, "
                        "%d used %s%s",
                        i, gdp->bg_block_bitmap,
                        gdp->bg_inode_bitmap, gdp->bg_inode_table,
                        gdp->bg_free_blocks_count,
                        gdp->bg_free_blocks_count != 1 ? "blocks" : "block",
                        gdp->bg_free_inodes_count,
                        gdp->bg_free_inodes_count != 1 ? "inodes" : "inode",
                        gdp->bg_used_dirs_count,
                        gdp->bg_used_dirs_count != 1 ? "directories"
                                : "directory", gdt_csum ? ", " : "\n");
                if (gdt_csum)
                        fprintf(out, "%d unused %s\n",
                                gdp->bg_itable_unused,
                                gdp->bg_itable_unused != 1 ? "inodes":"inode");
                first = 1;
                print_bg_opts(gdp, EXT2_BG_INODE_UNINIT, "Inode not init",
                              &first, out);
                print_bg_opts(gdp, EXT2_BG_BLOCK_UNINIT, "Block not init",
                              &first, out);
                if (gdt_csum) {
                        fprintf(out, "%sChecksum 0x%04x",
                                first ? "           [":", ", gdp->bg_checksum);
                        first = 0;
                }
                if (!first)
                        fputs("]\n", out);
        }
#endif
        return;
}



//open and init the Filesystem, use in main()
static void open_filesystem(char *device, int open_flags, blk_t superblock,
                            blk_t blocksize, int magicscan,
                            char *data_filename)
{
        int     retval;
        io_channel data_io = 0;

        if (superblock != 0 && blocksize == 0) {
                fprintf(stderr,"if you specify the superblock, you must also specify the block size\n");
                current_fs = NULL;
                return;
        }

        if (data_filename) {
                if ((open_flags & EXT2_FLAG_IMAGE_FILE) == 0) {
                        fprintf(stderr,"The -d option is only valid when reading an e2image file\n");
                        current_fs = NULL;
                        return;
                }
                retval = unix_io_manager->open(data_filename, 0, &data_io);
                if (retval) {
                        fprintf(stderr,"%s while opening data source\n" ,data_filename);
                        current_fs = NULL;
                        return;
                }
        }

        if (open_flags & EXT2_FLAG_RW) {
                open_flags &= ~EXT2_FLAG_RW;
        }
//	open_flags |= EXT2_FLAG_64BITS;
        retval = ext2fs_open(device, open_flags, superblock, blocksize,
                             unix_io_manager, &current_fs);
        if (retval) {
                fprintf(stderr,"%s Error %d while opening filesystem \n", device, retval);
                current_fs = NULL;
                return;
        }


        retval = ext2fs_read_inode_bitmap(current_fs);
        if (retval) {
                fprintf(stderr,"%s Error %d while reading inode bitmap\n", device, retval);
                goto errout;
        }
        retval = ext2fs_read_block_bitmap(current_fs);
        if (retval) {
        fprintf(stderr,"%s Error %d while reading block bitmap\n",device, retval);
                goto errout;
        }

	if (magicscan){	
//		switch on magic scan function 
#ifdef EXT2_FLAG_64BITS
		if( ext2fs_copy_generic_bmap(current_fs->inode_map, &imap) || ext2fs_copy_generic_bmap(current_fs->block_map, &bmap)){
#else
		if( ext2fs_copy_bitmap(current_fs->inode_map, &imap) || ext2fs_copy_bitmap(current_fs->block_map, &bmap)){
#endif
			fprintf(stderr,"%s Error while copy bitmap\n",device );
			imap = NULL;
			bmap = NULL;
		}else{
			int i;
#ifdef EXT2_FLAG_64BITS
			ext2fs_clear_generic_bmap(imap);
			for (i = 1; i < current_fs->super->s_first_ino; i++)//mark inode 1-8
				ext2fs_mark_generic_bmap(imap,(blk64_t) i);
			ext2fs_clear_generic_bmap(bmap);
#else
			ext2fs_clear_inode_bitmap(imap);
			for (i = 1; i < current_fs->super->s_first_ino; i++)//mark inode 1-8
					ext2fs_mark_generic_bitmap(imap,i); 
			ext2fs_clear_block_bitmap(bmap);
#endif
		}
	}

        if (data_io) {
                retval = ext2fs_set_data_io(current_fs, data_io);
                if (retval) {
                        fprintf(stderr,"%s Error %d while setting data source\n", device, retval);
                        goto errout;
                }
        }

        root = cwd = EXT2_ROOT_INO;
        return;

errout:
#ifdef DEBUG
	printf("Errout : open ext2fs\n");
#endif
        retval = ext2fs_close(current_fs);
        if (retval)
                fprintf(stderr, "%s Error %d while trying to close filesystem\n", device, retval);
        current_fs = NULL;
}


//subfunction for main
void print_modus_error(){
	char message0[] = "Invalide parameter : only input of one modus allowed [ -M | -m | -R | -r | -L | -l | -H | D ]\n";
	fprintf(stderr,"%s",message0);
}


//-------------------------------------------------------------------------------------------------------------
// the main() function 
int main(int argc, char *argv[]){
char* progname = argv[0];
int             retval, exitval;
char		defaultdir[] = "RECOVERDIR" ;
//FIXME : usage is not correct
const char      *usage = "\next4magic -M [-j <journal_file>] [-d <target_dir>] <filesystem> \n\
ext4magic -m [-j <journal_file>] [-d <target_dir>] <filesystem> \n\
ext4magic [-S|-J|-H|-V|-T] [-x] [-j <journal_file>] [-B n|-I n|-f <file_name>|-i <input_list>] [-t n|[[-a n][-b n]]] [-d <target_dir>] [-R|-r|-L|-l] [-Q] <filesystem>";
int             l,c ;
int             open_flags = EXT2_FLAG_SOFTSUPP_FEATURES;
int		recovermodus = 0 ;
int 		disaster = 0;
int 		recoverquality = DELETED_OPT; // default use also delete dir entry
char            *j_file_name = NULL;
char		*pathname = NULL;
char            *des_dir = NULL;
char 		*input_filename = NULL;
blk_t           superblock = 0;
blk_t           blocksize = 0;
__u32		transaction_nr = 0;
int             magicscan = 0;
char            *data_filename = 0;
int             mode = 0;
ext2_ino_t      inode_nr = EXT2_ROOT_INO ;
blk_t           block_nr = 0;
int             format = 0;
int             journal_flag = JOURNAL_CLOSE ;
int 		journal_backup = 0;
__u32		t_before = 0;
__u32		t_after = 0;
struct stat  filestat;


 // Sanity checks on the user. 
if ((argc == 1) || ((argc == 2) && (!(strstr(argv[1],"-V")))))    // allow '-V' as single argument
   { 
     printf("%s : Error: Missing device name and options.\n", progname);
     printf("%s \n", usage);
     return  EXIT_FAILURE;
   }


// set default Time from "now -1 day" to "now"
	
time( &now_time ); 
t_before = (__u32) now_time;
t_after = t_before - 86400 ;
 

// decode arguments
#ifdef EXPERT_MODE
while ((c = getopt (argc, argv, "TJRMLlmrQSxi:t:j:f:Vd:B:b:a:I:Hs:n:cD")) != EOF) { 
#else
while ((c = getopt (argc, argv, "TJRMLlmrSxi:t:j:f:Vd:B:b:a:I:H")) != EOF) { 
#endif
                switch (c) {
		case 'M':
			//not active, still in development
			if(mode & RECOVER_INODE){
				print_modus_error();
				exitval = EXIT_FAILURE ; 
                              	goto errout;
			}
			mode |= RECOVER_INODE;
			mode |= READ_JOURNAL;
			recovermodus = RECOV_ALL ;
			magicscan = 1;
			break;

		case 'm':
			//not active, still in development
			if(mode & RECOVER_INODE){
				print_modus_error();
				exitval = EXIT_FAILURE ; 
                              	goto errout;
			}
			mode |= RECOVER_INODE;
			mode |= READ_JOURNAL;
			recovermodus = RECOV_DEL ;
			magicscan = 1;
			break;
		
                case 'S':
                        mode |= PRINT_SUPERBLOCK ;
                        break; 

		case 'H':
			if(mode & RECOVER_INODE){
				print_modus_error();
				exitval = EXIT_FAILURE ; 
                              	goto errout;
			}
			mode |= RECOVER_INODE;
			mode |= PRINT_HISTOGRAM ;
			break;

		case 'R':
			if(mode & RECOVER_INODE){
				print_modus_error();
				exitval = EXIT_FAILURE ; 
                              	goto errout;
			}
			mode |= RECOVER_INODE;
			mode |= READ_JOURNAL;
			recovermodus = RECOV_ALL ;
			break;

		case 'r':
			if(mode & RECOVER_INODE){
				print_modus_error();
				exitval = EXIT_FAILURE ; 
                              	goto errout;
			}
			mode |= RECOVER_INODE;
			mode |= READ_JOURNAL;
			recovermodus = RECOV_DEL ;
			break;

		case 'L':
			if(mode & RECOVER_INODE){
				print_modus_error();
				exitval = EXIT_FAILURE ; 
                              	goto errout;
			}
			mode |= RECOVER_INODE;
			mode |= READ_JOURNAL;
			recovermodus = LIST_ALL ;
			break;

		case 'l':
			if(mode & RECOVER_INODE){
				print_modus_error();
				exitval = EXIT_FAILURE ; 
                              	goto errout;
			}
			mode |= RECOVER_INODE;
			mode |= READ_JOURNAL;
			recovermodus = LIST_STATUS ;
			break;

		case 'd':
                        des_dir = optarg;
			//we check later
                        break;

		case 'i':
			mode |= RECOVER_LIST;
			mode |= READ_JOURNAL;
                        input_filename = optarg;
			retval = stat (input_filename, &filestat);
			if (retval){ 
				fprintf(stderr,"Error: Invalid parameter: -i  %s\n", optarg); 
				fprintf(stderr,"filestat %s returns error %d\n", input_filename, retval);
                              	exitval = EXIT_FAILURE ; 
                              	goto errout;
			}
			else{
				if  (S_ISREG(filestat.st_mode) && (! access(input_filename,R_OK))){
					printf("\"%s\"  accept for inputfile \n",input_filename);
			 	}
				else {
					fprintf(stderr,"ERROR: can not use \"%s\" for inputfile\n",input_filename);
					exitval = EXIT_FAILURE ; 
              				goto errout;
				}
			}
			break;

                case 'I': 
			if (mode & COMMAND_INODE){
				fprintf(stderr,"Warning: only input of one inodeNR or filename allowed\n");
				break;
			}
                        mode |= COMMAND_INODE ;
                        errno = 0;
                        inode_nr = strtoul ( optarg, NULL, 10 );
                        if ( errno )
                            {
                              fprintf(stderr,"Error: Invalid parameter: -I  %s \n", optarg );
			      exitval = EXIT_FAILURE ; 
                              goto errout;
                            }
                        if ( inode_nr < 1 )
                            {
                              fprintf(stderr,"Error: %s -I: inodeNR \n", progname);
                              fprintf(stderr,"%lu is out of range\n", (long unsigned int)inode_nr);
			      exitval = EXIT_FAILURE ; 
                              goto errout;
                            }
                        break;

                  case 'B':
                        mode |= COMMAND_BLOCK ;
                        errno = 0;
                        block_nr = strtoul ( optarg, NULL, 10 );
                        if ( errno )
                            {
                              fprintf(stderr,"Error: Invalid parameter: -B  %s \n", optarg );
                              exitval = EXIT_FAILURE ; 
                              goto errout;
                            }
                        if ( block_nr < 1 )
                            {
                              fprintf(stderr,"Error: %s -B: blockNR \n", progname);
                              fprintf(stderr,"%lu is out of range\n", (long unsigned int)block_nr);
                              exitval = EXIT_FAILURE ; 
                              goto errout;
                            }
                        break;

		 case 't':
                        mode |= PRINT_TRANSACTION ;
			mode |= READ_JOURNAL;
                        errno = 0;
                        transaction_nr = strtoul ( optarg, NULL, 10 );
                        if ( errno )
                            {
                              fprintf(stderr,"Error: Invalid parameter: -t  %s \n", optarg );
                              exitval = EXIT_FAILURE ; 
                              goto errout;
                            }
                        if ( transaction_nr < 1 )
                            {
                              fprintf(stderr,"Error: %s -t: transactionNR \n", progname);
                              fprintf(stderr,"%lu is out of range\n", (long unsigned int)transaction_nr);
                              exitval = EXIT_FAILURE ; 
                              goto errout;
                            }
                        break;

                case 'x':
                        format = 1;
                        break;

                case 'j':
                        j_file_name = optarg;
			retval = stat (j_file_name, &filestat);
			if (retval){ 
				fprintf(stderr,"Error: Invalid parameter: -j  %s\n", optarg); 
				fprintf(stderr,"filestat %s returns error %d\n", j_file_name, retval);
                              	exitval = EXIT_FAILURE ; 
                              	goto errout;
			}
                        break;

		case 'f':
			if (mode & COMMAND_INODE){
				fprintf(stderr,"Warning: only input of one inodeNR or filename allowed\n");
				break;
			}				
			mode |= COMMAND_INODE ;
			mode |= COMMAND_PATHNAME ;
			l = strlen(optarg);
			pathname = malloc( l+1 );
			strcpy(pathname,optarg);
			break;

#ifdef EXPERT_MODE
		case 'Q':
			mode |= HIGH_QUALITY;
                        break;

		case 'c': // use a backup journal inode 
			journal_backup=1;
			break;

		case 'D': // Disaster recovery 
			if(mode & RECOVER_INODE){
				print_modus_error();
				exitval = EXIT_FAILURE ; 
                              	goto errout;
			}
			mode |= RECOVER_INODE;
			mode |= READ_JOURNAL;
			recovermodus = RECOV_ALL ;
			disaster = 2;
			break;

		case 's':
                        blocksize = parse_ulong(optarg, argv[0],
                                                "block size", 0);
			if ((!blocksize) || (blocksize % 1024)){
				fprintf(stderr,"Error: Invalid parameter: -s  %d\n", blocksize);
				fprintf(stderr,"ERROR: blocksize allowed only 1024 ; 2048 or 4096\n");
				exitval = EXIT_FAILURE ; 
                              	goto errout;
			}
                        break;
                case 'n':
                        superblock = parse_ulong(optarg, argv[0],
                                                 "superblock number", 0);
			if ((!superblock) || (!blocksize) || ((blocksize * 8) > superblock)){
				fprintf(stderr,"Error: Invalid parameter: -s  %d\n", superblock);
				switch (blocksize){
					case 1024: 
						fprintf(stderr, "blocksize 1024 possible 8193, 24577, 40961, 57345 ....\n");
						break;
					case 2048:
						fprintf(stderr, "blocksize 2048 possible 16384, 49152, 81920, 114688 ....\n");
						break;
					case 4096:
						fprintf(stderr, "blocksize 4096 possible 32768, 98304, 163840, 229376 ....\n");
						break;
					default:
					  	fprintf(stderr, "blocksize unknown\n");
				}
				exitval = EXIT_FAILURE ; 
                              	goto errout;
			}
                        break;
#endif
		case 'T':
			mode |= PRINT_BLOCKLIST;
			mode |= READ_JOURNAL;
			break;

		case 'J':
			mode |= PRINT_J_SUPERBLOCK;
			mode |= READ_JOURNAL;
			break;

		case 'b':
                        errno = 0;
                        t_before = strtoul ( optarg, NULL, 10 );
                        if ( errno )
                            {
                              fprintf(stderr,"Error: Invalid parameter: -b  %s \n", optarg );
                              exitval = EXIT_FAILURE ; 
                              goto errout;
                            }
                        if ( t_before < 1 )
                            {
                              fprintf(stderr,"Error: %s -b: time \n", progname);
                              fprintf(stderr,"%lu is out of range\n", (long unsigned int)inode_nr);
                              exitval = EXIT_FAILURE ; 
                              goto errout;
                            }
			mode |= INPUT_TIME; 
                        break;

		case 'a':
                        errno = 0;
                        t_after = strtoul ( optarg, NULL, 10 );
                        if ( errno )
                            {
                              fprintf(stderr,"Error: Invalid parameter: -a  %s \n", optarg );
                              exitval = EXIT_FAILURE ; 
                              goto errout;
                            }
                        if ( t_after < 1 )
                            {
                              fprintf(stderr,"Error: %s -a: time \n", progname);
                              fprintf(stderr,"%d is out of range\n", inode_nr);
                              exitval = EXIT_FAILURE ; 
                              goto errout;
                            }
			mode |= INPUT_TIME;
                        break;


                case 'z':   //experimental not activ at default
                        data_filename = optarg;
                        break;

                case 'y':   //experimental not activ at default
                        open_flags |= EXT2_FLAG_IMAGE_FILE;
                        break;

                case 'V':
			print_version();
                        exit(0);

                default:
                        fprintf(stderr,"Usage: %s %s\n",argv[0], usage);
			exitval = EXIT_FAILURE ; 
                        goto errout; 
                }
        }

	if (getuid()) mode = 0;
        if (optind < argc)
                open_filesystem(argv[optind], open_flags,superblock, blocksize, magicscan, data_filename);

	if (! current_fs) mode = 0;
#ifdef DEBUG
	printf("Operation-mode = %d\n", mode);
#endif

//--------------------------------------------------------------------------------------------
// check any parameter and options
// check time option
if ((mode && magicscan) || disaster){
	printf("Warning: Activate magic-scan or disaster-recovery function, may be some command line options ignored\n");
	inode_nr = EXT2_ROOT_INO;
	t_before = (__u32) now_time;
	if (disaster){
		t_after = t_before - 60;
		printf("Start ext4magic Disaster Recovery\n"); 
	}
	else{
		mode &= MASK_MAGIC_SCAN;
		if ((!(mode & INPUT_TIME)) || (t_after == t_before - 86400)){
			t_after = get_last_delete_time(current_fs);
			if (! (t_after + 1) ){
				exitval = EXIT_FAILURE ; 
                		goto errout;
			}
			mode |= INPUT_TIME;
		}
		magicfile = malloc(64);
		memset(magicfile,0,64);
		strncpy (magicfile,"/usr/share/misc/ext4magic",25); 
		retval = stat (magicfile, &filestat);
		if (retval){ 
			strncpy (magicfile,"/usr/local/share/misc/ext4magic",31); 
			retval = stat (magicfile, &filestat);
		}
		if ((! retval) && (S_ISREG(filestat.st_mode) && (! access(magicfile,R_OK)))) {
			printf("use magic-db on \"%s\"\n",magicfile);
		}
		else {
			free(magicfile);
			magicfile=NULL;
		}
	}
}



if (mode & INPUT_TIME){
	if (! ((t_after > 315601200) && (t_after < t_before))) // 315601200 = "1980-01-01 20:00:00"
		{
		  fprintf(stderr,"Invalide parameter: range \"AFTER <--> BEFORE\"\n");
		  fprintf(stderr,"the automatic default parameter AFTER=\"now -1 day\" ; BEFORE=\"now\"\n");
		  fprintf(stderr,"\"-b before-timestamp\" must greater then \"-a after-timestamp\"\n"); 
		  fprintf(stderr,"Example : %s -H -b $(date +%%s) -a $(date -d \"-1 day\" +%%s) %s\n",progname,current_fs->device_name);
		  exitval = EXIT_FAILURE ; 
                  goto errout;
	}
	if (mode & PRINT_TRANSACTION){
		  fprintf(stderr,"Invalide parameter: use either Transaction-Nr or Timestamps for search in Journal\n");
 		  exitval = EXIT_FAILURE ; 
                  goto errout;
	}
	mode |= READ_JOURNAL;
} 



//check for the recoverdir
if (((mode & RECOVER_INODE) && (recovermodus & REC_DIR_NEEDED)) || mode & RECOVER_LIST || magicscan) {
	struct stat     st_buf;
	dev_t           file_rdev=0;
	
	if(!des_dir)	
		des_dir = defaultdir;

		retval = stat (des_dir, &filestat);
		if (retval){
			retval = mkdir(des_dir , S_IRWXU);
			if (retval && (errno != EEXIST)){
				fprintf(stderr,"ERROR: can not create the recover directory: %s\n", des_dir);
				exitval = EXIT_FAILURE ; 
                       		goto errout;	
			}
			else 
				retval = stat (des_dir, &filestat);
		}
		if (!retval){
			if  (S_ISDIR(filestat.st_mode) && (filestat.st_mode & S_IRWXU) == S_IRWXU){
				if (stat(argv[optind], &st_buf) == 0) {
                			if (S_ISBLK(st_buf.st_mode)) {
#ifndef __GNU__ /* The GNU hurd is broken with respect to stat devices */
                        			file_rdev = st_buf.st_rdev;
#endif  /* __GNU__ */
                			} else {
                        			file_rdev = st_buf.st_dev;
                			}
        			}
				if ((filestat.st_dev == file_rdev) && (S_ISBLK(st_buf.st_mode))){
					fprintf(stderr,"ERROR: can not use \"%s\" for recover directory. It's the same filesystem : \"%s\"\n", des_dir, argv[optind]);
					exitval = EXIT_FAILURE ; 
#ifdef DEBUG
					printf("recover_dir_dev : %d    filesystem_dev : %d\n",(int)filestat.st_dev, (int)file_rdev);
#endif
              				goto errout;
				}	
				printf("\"%s\"  accept for recoverdir\n",des_dir);
			 }
			else {
				fprintf(stderr,"ERROR: can not use \"%s\" for recover directory\n", des_dir);
				exitval = EXIT_FAILURE ; 
              			goto errout;
			}
		}
		else{
			fprintf(stderr,"Error: Invalid parameter: -d  %s \n", des_dir );
			exitval = EXIT_FAILURE ; 
                       	goto errout;
		}
// set quality
	if (mode & HIGH_QUALITY)
		recoverquality = 0 ;	// not use of delete dir entry

}

//mark for douple quotes filename
if ((recovermodus & (LIST_ALL | LIST_STATUS)) && format)
 	recovermodus |= DOUPLE_QUOTES_LIST;


//................................................................................................ 
// begin Operation
  if (current_fs) {
	//  printf( "Open mode: read-%s\n", current_fs->flags & EXT2_FLAG_RW ? "write" : "only");
  	  printf("Filesystem in use: %s\n\n",current_fs ? current_fs->device_name : "--none--");
	  init_link_stack();


// print filesystem superblock
  if ((mode & PRINT_SUPERBLOCK) && (! ( mode & READ_JOURNAL))) show_super_stats(format);


// print histogram over inodeblock
  if (mode & PRINT_HISTOGRAM){
	if (mode & COMMAND_INODE){
			mode |= READ_JOURNAL;
			recovermodus = HIST_DIR ;
	}
	else  {
		mode = 0 ;
   		read_all_inode_time(current_fs,t_after,t_before,format);
	}
  }


// no file or directory is given, use the default root-inode at now and set COMMAND_INODE flag 
  if((mode & RECOVER_INODE) && (! (mode & COMMAND_INODE)))
	mode |= COMMAND_INODE ;



// print filesystem inode
  if ((mode & COMMAND_INODE) && (! (mode & READ_JOURNAL)))
       {
	int allocated;
	retval = 0;
        struct ext2_inode *inode_buf;

        // search for inode if the pathname is given 
	if (mode & COMMAND_PATHNAME){
		if(! strcmp(pathname,"/") || ! strcmp(pathname,"")){
			inode_nr = EXT2_ROOT_INO ;
			*pathname = 0;
		}
		else{
			int l=strlen(pathname) -1;
			if (*(pathname + l)  == '/')
				*(pathname + l)  = 0 ;
			retval = ext2fs_namei(current_fs,EXT2_ROOT_INO ,EXT2_ROOT_INO , pathname, &inode_nr);
		}
        	if (retval) {
                	fprintf(stderr,"Error: Filename \"%s\" not found in Filesystem\n", pathname);
			fprintf(stderr,"if \"%s\" deleted, use InodeNr or try Journaling options\n",pathname);
	                exitval = EXIT_FAILURE ; 
        	          goto errout;
        	}
	}

        inode_buf = malloc(EXT2_INODE_SIZE(current_fs->super));
        if (intern_read_inode_full(inode_nr, inode_buf,EXT2_INODE_SIZE(current_fs->super))){
 		fprintf(stderr,"Fatal Error: can not read InodeNr. %lu \n", (long unsigned int)inode_nr);
                return EXIT_FAILURE;
        }
	
	allocated = ext2fs_test_inode_bitmap ( current_fs->inode_map, inode_nr );

	fprintf(stdout,"\nDump internal Inode %d\nStatus : %s\n\n",inode_nr,(allocated) ? "Inode is Allocated" : "Inode is Unallocated");
        dump_inode(stdout, "",inode_nr, inode_buf, format);
        if (LINUX_S_ISDIR(inode_buf->i_mode)) list_dir(inode_nr);
        free(inode_buf);
       }


// print filesystem block
  if ((mode & COMMAND_BLOCK) && (! (mode & READ_JOURNAL)))
    {
	void *block_buf;
	int allocated;
	block_buf = malloc(EXT2_BLOCK_SIZE(current_fs->super ));
	if(!read_block ( current_fs , &block_nr , block_buf )){
		allocated = ext2fs_test_block_bitmap ( current_fs->block_map, block_nr );
		fprintf(stdout,"Dump Filesystemblock %10lu   Status : %s\n",(long unsigned int)block_nr,
			(allocated) ? "Block is Allocated" : "Block is Unallocated");
		blockhex ( stdout , block_buf , format , EXT2_BLOCK_SIZE(current_fs->super ));
	}
	free(block_buf);
    }



//-----------------------------------------------------
// loading Journal 
if (mode & READ_JOURNAL){
	if (journal_flag) journal_flag = journal_open( j_file_name , journal_backup );
	if (journal_flag != JOURNAL_OPEN){
		 exitval = EXIT_FAILURE ; 
                  goto errout;
	}


//print the after and before Time 
	if (mode & INPUT_TIME){
		printf("Activ Time after  : %s", time_to_string(t_after));
		printf("Activ Time before : %s", time_to_string(t_before));
	}



//recover from inputlist
	if(mode & RECOVER_LIST){
		recover_list(des_dir, input_filename,t_after,t_before,(recovermodus & RECOV_ALL )? 0 : 1);
		mode = 0;
	}


// search inodeNr from pathName use journal
	if (mode & COMMAND_PATHNAME){
		if(! strcmp(pathname,"/") || ! strcmp(pathname,"")){
			inode_nr = EXT2_ROOT_INO ;
			*pathname = 0;
		}
		else{
			int l=strlen(pathname) -1 ;
			if (*(pathname + l)  == '/')
				*(pathname + l)  = 0 ;
			inode_nr = local_namei(NULL,pathname,t_after,t_before,DELETED_OPT);
		}
		if (inode_nr) {
			printf("Inode found \"%s\"   %lu \n", pathname, (long unsigned int)inode_nr);
		}
		else{
	   		fprintf(stderr,"Error: Inode not found for \"%s\"\n",pathname);
			fprintf(stderr,"Check the valid PATHNAME \"%s\" and the BEFORE option \"%s\"\n", pathname,time_to_string(t_before));
			exitval = EXIT_FAILURE ; 
                  	goto journalout;
		}
	}
	else{
		if (mode & COMMAND_INODE){
			if (!pathname) pathname = malloc(20);
			if (!pathname) {
				fprintf(stderr,"ERROR: can not allocate memory\n");
				goto journalout;
			}
			if (inode_nr == EXT2_ROOT_INO)
				*pathname = 0;
			else
				sprintf(pathname,"<%lu>",(long unsigned int)inode_nr);
		}
	}



// recover all mode if possible
// activate magic level 1 and 2 for recover from the root inode
	if ((inode_nr == EXT2_ROOT_INO) && (mode & COMMAND_INODE) && (mode & RECOVER_INODE)
	     && (recoverquality) && (recovermodus & REC_DIR_NEEDED) && (! magicscan)) {
		if( ext2fs_copy_bitmap(current_fs->inode_map, &imap)){
			fprintf(stderr,"%s Error while copy bitmap\n",argv[optind]);
			imap = NULL;
		}else{
			int i;
#ifdef EXT2_FLAG_64BITS
			ext2fs_clear_generic_bmap(imap);
			for (i = 1; i < current_fs->super->s_first_ino; i++)
				ext2fs_mark_generic_bmap(imap,(blk64_t) i); //mark inode 1-8

#else
			ext2fs_clear_inode_bitmap(imap);
			for (i = 1; i < current_fs->super->s_first_ino; i++)//mark inode 1-8
					ext2fs_mark_generic_bitmap(imap,i); 
#endif
		}
	}



// print journal transaction bocklist
	if (mode &  PRINT_BLOCKLIST){
		if (mode & COMMAND_BLOCK)
			 print_block_transaction((blk64_t)block_nr,format);
		if (mode & COMMAND_INODE){
			struct inode_pos_struct pos;
			block_nr = get_inode_pos(current_fs->super, &pos , inode_nr, 1);
			print_block_transaction((blk64_t)block_nr,format);
		}
		if (!(mode & (COMMAND_INODE + COMMAND_BLOCK))) print_block_list( format );
	} 


// print journal superblock
	if (mode &  PRINT_J_SUPERBLOCK) dump_journal_superblock();


//print journal block
	if ((mode & COMMAND_BLOCK) && (mode & PRINT_TRANSACTION )){
		__u32 journal_block;
		journal_block = get_journal_blocknr(block_nr, transaction_nr);
		if ( journal_block ){	
			printf("dump Journalblock %lu  : a copy of Filesystemblock %lu : Transaction %lu\n",
				(long unsigned int)journal_block, (long unsigned int)block_nr, (long unsigned int)transaction_nr);	
		 	dump_journal_block( journal_block, format );
		}
		else
			fprintf(stderr,"Error: Filesystemblock %lu not found in Journaltransaction %lu\n",
				(long unsigned int)block_nr, (long unsigned int)transaction_nr);
	}


//print or recover a single inode from transactionnumber
if ((mode & COMMAND_INODE) && (mode & PRINT_TRANSACTION )){
	struct ext2_inode_large *inode;
	inode = malloc( current_fs->blocksize );
	if (inode){
		if (! get_transaction_inode(inode_nr, transaction_nr, inode))
			print_j_inode(inode , inode_nr , transaction_nr , format);
		if ((mode & RECOVER_INODE) && (recovermodus & (RECOV_ALL | RECOV_DEL)) && (!(LINUX_S_ISDIR(inode->i_mode)))){
			recover_file(des_dir,"", pathname,(struct ext2_inode*)inode, inode_nr ,(recovermodus & RECOV_ALL )? 0 : 1 );
			mode = 0;
		}
		free(inode);
	}
}


// start recursiv over the filesystem, used for recover, list and tree-history 
if ((mode & COMMAND_INODE) && (mode & RECOVER_INODE))
       	{
		struct ring_buf *i_list;
		struct ext2_inode* r_inode;
		r_item *item = NULL;

		if (ext2fs_test_inode_bitmap ( current_fs->inode_map, inode_nr )) {
			fprintf(stdout,"Inode %lu is allocated\n",(long unsigned int)inode_nr);
		}

		i_list = get_j_inode_list(current_fs->super, inode_nr);
		
		if (mode & INPUT_TIME)
			item = get_undel_inode(i_list,t_after,t_before);
		else
			item = get_last_undel_inode(i_list);


		if (item) {
			r_inode = (struct ext2_inode*)item->inode;
			if (LINUX_S_ISDIR(r_inode->i_mode) ) 
			{
				struct dir_list_head_t * dir = NULL;

				dir = get_dir3(NULL,0, inode_nr , "",pathname, t_after,t_before, recoverquality );
				if (dir) {
					lookup_local(des_dir, dir,t_after,t_before, recoverquality | recovermodus );
					if (recovermodus & HIST_DIR )
						print_coll_list(t_after, t_before, format);
//Magic step 1 + 2 +3
					if (imap){
						imap_search(des_dir, t_after, t_before, disaster );
						// we use imap as a flag for the disaster mode
						ext2fs_free_inode_bitmap(imap);
						imap = NULL;
						if (bmap){
							if (!(current_fs->super->s_feature_incompat & EXT3_FEATURE_INCOMPAT_EXTENTS)){ 
								printf("MAGIC function for ext3 not available, use ext4magic 0.2.4 instead\n");
//								magic_block_scan3(des_dir, t_after);
							}
							else{
								//if (bmap) printf("The MAGIC Function is currently only for ext3 filesystems available\n");
								magic_block_scan4(des_dir,t_after);
							}
						}
					}
					clear_dir_list(dir);
				}
				else
					printf("Inode %lu is a directory but not found after %lu and before %lu\n",
						(long unsigned int)inode_nr, (long unsigned int)t_after, (long unsigned int)t_before);
			}
			else {
				if (recovermodus & (RECOV_ALL | RECOV_DEL))
					recover_file(des_dir,"", pathname, r_inode, inode_nr ,(recovermodus & RECOV_ALL )? 0 : 1 );
				else
					printf("Single file can only recovered with option -R or -r\n");
			}
		 }	
		else
			fprintf(stdout,"No undeled inode %u in journal found\n",inode_nr);

#ifdef EXPERT_MODE
		if (disaster && imap){
			// Nothing worked, trying to recover all undeleted files under catastrophic conditions
			printf("Force a disaster recovery\n");
			imap_search(des_dir, t_after, t_before, disaster );
			ext2fs_free_inode_bitmap(imap);
			imap = NULL;
		}
#endif
 
		if (i_list) ring_del(i_list);
	}



//print all journal inode 
	if ((mode & COMMAND_INODE) && !(mode & RECOVER_INODE) && !(mode & PRINT_TRANSACTION))
       	{
		struct ring_buf* i_list;
		r_item *item = NULL;
		__u32 time_stamp;

		i_list = get_j_inode_list(current_fs->super, inode_nr);
		if (i_list){ 
			if (mode & INPUT_TIME){
				item = get_undel_inode(i_list,t_after,t_before);	
				if(item){
					dump_inode(stdout, "",inode_nr, (struct ext2_inode *)item->inode, format);
					if ( LINUX_S_ISDIR(item->inode->i_mode)){
						time_stamp = (item->transaction.start) ? get_trans_time(item->transaction.start) : 0 ;
#ifdef DEBUG
						printf ("Transaction-time for search %ld : %s\n", time_stamp, time_to_string(time_stamp));
#endif
						list_dir3(inode_nr, (struct ext2_inode*) item->inode, &(item->transaction) , time_stamp);
					}
				}
				else
					printf("No entry with this time found\n");
			}
			else{
				dump_inode_list(i_list, format);
			}
		if (i_list) ring_del(i_list);
		}
	}
journalout:
   if(!journal_flag) journal_flag = journal_close();
   }// end open Journal
}  // end Operation 

exitval = EXIT_SUCCESS;
#ifdef DEBUG
   printf("EXIT_SUCCESS : %d, close and clear\n",exitval);
#endif


errout: 
  if (current_fs)    {
/*//FIXME in development	
	if (bmap){
		struct ext2fs_struct_loc_generic_bitmap *d_bmap = bmap ;
		blockhex(stdout,(void*)d_bmap->bitmap,0,current_fs->super->s_blocks_count >> 3);
	}*/
	if (imap) ext2fs_free_inode_bitmap(imap);
	if (bmap) ext2fs_free_inode_bitmap(bmap);
	imap = NULL;
	bmap = NULL;
	if (magicfile) free(magicfile);
	magicfile = NULL;
        retval = ext2fs_close(current_fs);
        if (retval) {
           fprintf(stderr, "ext2fs_close\n");
           current_fs = NULL;
          }
        }
  if (pathname) free(pathname);
  clear_link_stack();
  if (! exitval)
       printf("ext4magic : EXIT_SUCCESS\n");
  return exitval;
}
//-------------------------------------------------------------------------------------------------------------------------------
