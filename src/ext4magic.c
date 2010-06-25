/***************************************************************************
 *   Copyright (C) 2010 by Roberto Maar   *
 *   robi@users.berlios.de *
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

//local header files
#include "util.h"
#include "ext4magic.h"
#include "journal.h"
#include "inode.h"
#include "hard_link_stack.h"



ext2_filsys     current_fs = NULL;
ext2_ino_t      root, cwd;



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
}


//subfunction for show_super_stats()
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

//print superblock
void show_super_stats(int header_only)
{
        dgrp_t  i;
        FILE    *out;
        struct ext2_group_desc *gdp;
        int     c;
        int     numdirs = 0, first, gdt_csum;

        out=stdout;

        list_super2(current_fs->super, out);
	if (! header_only) {
                return;
        }

        for (i=0; i < current_fs->group_desc_count; i++)
                numdirs += current_fs->group_desc[i].bg_used_dirs_count;
        fprintf(out, "Directories:              %d\n", numdirs);


        gdt_csum = EXT2_HAS_RO_COMPAT_FEATURE(current_fs->super,
                                              EXT4_FEATURE_RO_COMPAT_GDT_CSUM);
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
        return;
}



//open and init the Filesystem, use in main()
static void open_filesystem(char *device, int open_flags, blk_t superblock,
                            blk_t blocksize, int catastrophic,
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

        if (catastrophic && (open_flags & EXT2_FLAG_RW)) {
                fprintf(stderr,"opening read-only because of catastrophic mode\n");
                open_flags &= ~EXT2_FLAG_RW;
        }

        retval = ext2fs_open(device, open_flags, superblock, blocksize,
                             unix_io_manager, &current_fs);
        if (retval) {
                fprintf(stderr,"%s %d while opening filesystem \n", device, retval);
                current_fs = NULL;
                return;
        }


        if (catastrophic)
                fprintf(stderr,"%s catastrophic mode - not reading inode or group bitmaps\n", device);
        else {
                retval = ext2fs_read_inode_bitmap(current_fs);
                if (retval) {
                        fprintf(stderr,"%s %d while reading inode bitmap\n", device, retval);
                        goto errout;
                }
                retval = ext2fs_read_block_bitmap(current_fs);
                if (retval) {
                        fprintf(stderr,"%s %d while reading block bitmap\n",device, retval);
                        goto errout;
                }
        }

        if (data_io) {
                retval = ext2fs_set_data_io(current_fs, data_io);
                if (retval) {
                        fprintf(stderr,"%s %d while setting data source\n", device, retval);
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
                fprintf(stderr, "%s %d while trying to close filesystem\n", device, retval);
        current_fs = NULL;
}


//subfunction for main
void print_modus_error(){
	char message0[] = "Warning: only input of one modus allowed [ -R | -r | -L | -l | -H ]\n";
	fprintf(stderr,"%s",message0);
}


//-------------------------------------------------------------------------------------------------------------
// the main() function 
int main(int argc, char *argv[]){
char* progname = argv[0];
int             retval, exitval;
char		defaultdir[] = "RECOVERDIR" ;
//FIXME : usage is not correct
const char      *usage = "ext4magic [-S|-J|-H|-V|-T] [-x] [-j <journal_file>] [-B n|-I n|-f <file_name>|-i <input_list>] [-t n|[[-a n][-b n]]] [-d <target_dir>] [-R|-r|-L|-l] [-Q] <filesystem>";
int             c;
int             open_flags = EXT2_FLAG_SOFTSUPP_FEATURES;
int             exit_status = 0 ;
int		recovermodus = 0 ;
int 		recoverquality = DELETED_OPT; // default use also delete dir entry
char            *j_file_name = NULL;
char		*pathname = NULL;
char            *des_dir = NULL;
char 		*input_filename = NULL;
blk_t           superblock = 0;
blk_t           blocksize = 0;
int		transaction_nr = 0;
//int             catastrophic = 0;
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
if ( argc < 3 )  
   { 
     printf("%s : Error: Missing device name and options.\n", progname);
     printf("%s \n", usage);
     return  EXIT_FAILURE;
   }

   {
// set default Time from "now -1 day" to "now"
	time_t  help_time;
	time( &help_time ); 
	t_before = (__u32) help_time;
	t_after = t_before - 86400 ;
   }

// decode arguments
while ((c = getopt (argc, argv, "TJRLlrQSxi:t:j:f:Vd:B:b:a:I:H")) != EOF) { 
                switch (c) {
                case 'S':
                        mode |= PRINT_SUPERBLOCK ;
                        break; 

		case 'H':
			if(mode & RECOVER_INODE){
				print_modus_error();
			break;
			}
			mode |= RECOVER_INODE;
			mode |= PRINT_HISTOGRAM ;
			break;

		case 'R':
			if(mode & RECOVER_INODE){
				print_modus_error();
				break;
			}
			mode |= RECOVER_INODE;
			mode |= READ_JOURNAL;
			recovermodus = RECOV_ALL ;
			break;

		case 'r':
			if(mode & RECOVER_INODE){
				print_modus_error();
				break;
			}
			mode |= RECOVER_INODE;
			mode |= READ_JOURNAL;
			recovermodus = RECOV_DEL ;
			break;

		case 'L':
			if(mode & RECOVER_INODE){
				print_modus_error();
				break;
			}
			mode |= RECOVER_INODE;
			mode |= READ_JOURNAL;
			recovermodus = LIST_ALL ;
			break;

		case 'l':
			if(mode & RECOVER_INODE){
				print_modus_error();
				break;
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
                        inode_nr = strtol ( optarg, NULL, 10 );
                        if ( errno )
                            {
                              fprintf(stderr,"Error: Invalid parameter: -I  %s \n", optarg );
			      exitval = EXIT_FAILURE ; 
                              goto errout;
                            }
                        if ( inode_nr < 1 )
                            {
                              fprintf(stderr,"Error: %s -I: inodeNR \n", progname);
                              fprintf(stderr,"%d is out of range\n", inode_nr);
			      exitval = EXIT_FAILURE ; 
                              goto errout;
                            }
                        break;

                  case 'B':
                        mode |= COMMAND_BLOCK ;
                        errno = 0;
                        block_nr = strtol ( optarg, NULL, 10 );
                        if ( errno )
                            {
                              fprintf(stderr,"Error: Invalid parameter: -B  %s \n", optarg );
                              exitval = EXIT_FAILURE ; 
                              goto errout;
                            }
                        if ( block_nr < 1 )
                            {
                              fprintf(stderr,"Error: %s -B: blockNR \n", progname);
                              fprintf(stderr,"%d is out of range\n", block_nr);
                              exitval = EXIT_FAILURE ; 
                              goto errout;
                            }
                        break;

		 case 't':
                        mode |= PRINT_TRANSACTION ;
			mode |= READ_JOURNAL;
                        errno = 0;
                        transaction_nr = strtol ( optarg, NULL, 10 );
                        if ( errno )
                            {
                              fprintf(stderr,"Error: Invalid parameter: -t  %s \n", optarg );
                              exitval = EXIT_FAILURE ; 
                              goto errout;
                            }
                        if ( transaction_nr < 1 )
                            {
                              fprintf(stderr,"Error: %s -t: transactionNR \n", progname);
                              fprintf(stderr,"%d is out of range\n", transaction_nr);
                              exitval = EXIT_FAILURE ; 
                              goto errout;
                            }
                        break;

                case 'x':
                        format = 1;
                        break;

		case 'Q':
			mode |= HIGH_QUALITY;
                        break;

		case 'w'://experimental not activ at default
			journal_backup=1;
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
			pathname = malloc(512);
			strcpy(pathname,optarg);
			break;
			
		case 'v'://experimental not activ at default
                        blocksize = parse_ulong(optarg, argv[0],
                                                "block size", 0);
                        break;
                case 's'://experimental not activ at default
                        superblock = parse_ulong(optarg, argv[0],
                                                 "superblock number", 0);
                        break;
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
                        t_before = strtol ( optarg, NULL, 10 );
                        if ( errno )
                            {
                              fprintf(stderr,"Error: Invalid parameter: -b  %s \n", optarg );
                              exitval = EXIT_FAILURE ; 
                              goto errout;
                            }
                        if ( t_before < 1 )
                            {
                              fprintf(stderr,"Error: %s -b: time \n", progname);
                              fprintf(stderr,"%d is out of range\n", inode_nr);
                              exitval = EXIT_FAILURE ; 
                              goto errout;
                            }
			mode |= INPUT_TIME; 
                        break;

		case 'a':
                        errno = 0;
                        t_after = strtol ( optarg, NULL, 10 );
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
                open_filesystem(argv[optind], open_flags,superblock, blocksize, 0, data_filename);
#ifdef DEBUG
	printf("Operation-mode = %d\n", mode);
#endif

//--------------------------------------------------------------------------------------------
// check any parameter an options
// check time option
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
if ((mode & RECOVER_INODE) && (recovermodus &  (REC_DIR_NEEDED)) || mode & RECOVER_LIST) {
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
				if (filestat.st_dev == file_rdev){
					fprintf(stderr,"ERROR: can not use \"%s\" for recover directory. It's the same filesystem : \"%s\"\n", des_dir, argv[optind]);
					exitval = EXIT_FAILURE ; 
#ifdef DEBUG
					printf("recover_dir_dev : %d    filesystem_dev : %d   ",filestat.st_dev, file_rdev);
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
        if (intern_read_inode_full(inode_nr, inode_buf,EXT2_INODE_SIZE(current_fs->super))) return;
	
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
	read_block ( current_fs , &block_nr , block_buf );
	allocated = ext2fs_test_block_bitmap ( current_fs->block_map, block_nr );
	fprintf(stdout,"Dump Filesystemblock %10lu   Status : %s\n",block_nr,(allocated) ? "Block is Allocated" : "Block is Unallocated");
	blockhex ( stdout , block_buf , format , EXT2_BLOCK_SIZE(current_fs->super ));
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
			printf("Inode found \"%s\"   %lu \n", pathname, inode_nr);
		}
		else{
	   		fprintf(stderr,"Error: Inode not found for \"%s\"\n",pathname);
			fprintf(stderr,"Check the valid PATHNAME \"%s\" and the BEFORE option \"%s\"\n", pathname,time_to_string(t_before));
			exitval = EXIT_FAILURE ; 
                  	goto errout;
		}
	}
	else{
		if (mode & COMMAND_INODE){
			pathname = malloc(20);
			if (inode_nr == EXT2_ROOT_INO)
				*pathname = 0;
			else
				sprintf(pathname,"<%lu>",inode_nr);
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
				journal_block, block_nr, transaction_nr);	
		 	dump_journal_block( journal_block, format );
		}
		else
			fprintf(stderr,"Error: Filesystemblock %lu not found in Journaltransaction %lu\n",block_nr, transaction_nr);
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
			fprintf(stdout,"Inode %lu is allocated\n",inode_nr);
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
//FIXME: recovermodus
					lookup_local(des_dir, dir,t_after,t_before, recoverquality | recovermodus );
					if (recovermodus & HIST_DIR )
						print_coll_list(t_after, t_before, format);
				}
				else
					printf("Inode %lu is a directory but not found after %lu and before %lu\n",inode_nr,t_after,t_before);
				
				if (dir) clear_dir_list(dir);
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

   if(!journal_flag) journal_flag = journal_close();
   }// end open Journal
}  // end Operation 

exitval = EXIT_SUCCESS;
#ifdef DEBUG
   printf("EXIT_SUCCESS : %d, close and clear\n",exitval);
#endif


errout: 
  if (current_fs)    {
        retval = ext2fs_close(current_fs);
        if (retval) {
           fprintf(stderr, "ext2fs_close\n");
           current_fs = NULL;
          }
        }
  if (pathname) free(pathname);
  clear_link_stack();
  return exitval;
}
//-------------------------------------------------------------------------------------------------------------------------------
