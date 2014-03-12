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

/* ext3/4 libraries */
#include <ext2fs/ext2fs.h>
#include <e2p/e2p.h>

#include "util.h"
#include "ext2fsP.h"
#include "block.h"
#include "inode.h"

// default maxcount for histrogram 
#define HIST_COUNT 10


extern ext2_filsys     current_fs;
static struct inode_nr_collect	*collect ;

// privat struct for histogram
struct time_counter{
	__u32 time;
	__u32 c_count;
	__u32 d_count;
	__u32 cr_count;
}; 


//Sub function for print header for histogram
static void dump_hist_header(char* type, __u32 after){
	fprintf(stdout,"|-----------%s-----------------  after  --------------------  %s",type,time_to_string(after));
	return;
}


//Sub function for print the histogram
static void dump_hist(struct time_counter* p_hist, int cm, __u32 after, __u32 befor , __u16 crt_found ){
 int i,j ;	
 float x, step = 10.1 ;

// FIXME: this function can optimize

// c_time
dump_hist_header("c_time  Histogram",after);
for (i=1;i <= cm ;i++) if (step < (p_hist+i)->c_count) step = (p_hist+i)->c_count;
step = step / 50;
for (i=1;i <= cm ;i++){
	fprintf(stdout,"%10lu : %8lu |", (long unsigned int)(p_hist+i)->time , (long unsigned int)(p_hist+i)->c_count);
	for (j = 0,x = (p_hist+i)->c_count/50 ; j < 50 ; j++, x+=step )
	fprintf(stdout,"%c",(x<(p_hist+i)->c_count) ? '*' : ' '); 
	fprintf(stdout,"|   %s", time_to_string((__u32)(p_hist+i)->time));
}
// d_time
fprintf(stdout,"\n\n");
dump_hist_header("d_time  Histogram",after);
step = 10.1 ;
for (i=1;i <= cm ;i++) if (step < (p_hist+i)->d_count) step = (p_hist+i)->d_count;
step = step / 50;
for (i=1;i <= cm ;i++){
	fprintf(stdout,"%10lu : %8lu |", (long unsigned int)(p_hist+i)->time , (long unsigned int)(p_hist+i)->d_count);
	for (j = 0,x = (p_hist+i)->d_count/50 ; j < 50 ; j++, x+=step )
	fprintf(stdout,"%c",(x<(p_hist+i)->d_count) ? '*' : ' '); 
	fprintf(stdout,"|   %s", time_to_string((__u32)(p_hist+i)->time));
}
// cr_time
if (crt_found){
	fprintf(stdout,"\n\n");
	dump_hist_header("cr_time Histogram",after);
	step = 10.1 ;
	for (i=1;i <= cm ;i++) if (step < (p_hist+i)->cr_count) step = (p_hist+i)->cr_count;
	step = step / 50;
	for (i=1;i <= cm ;i++){
		fprintf(stdout,"%10lu : %8lu |", (long unsigned int)(p_hist+i)->time , (long unsigned int)(p_hist+i)->cr_count);
		for (j = 0,x = (p_hist+i)->cr_count/50 ; j < 50 ; j++, x+=step )
		fprintf(stdout,"%c",(x<(p_hist+i)->cr_count) ? '*' : ' '); 
		fprintf(stdout,"|   %s", time_to_string((__u32)(p_hist+i)->time));
	}
}
return;
}


// analyse and print time stamp of all fs inode (histogram function)
void read_all_inode_time(ext2_filsys fs, __u32 t_after, __u32 t_before, int flag)
{
struct ext2_group_desc *gdp;
char 	*buf= NULL;
int 	x, j, cm, zero_flag, retval;
__u32 	blocksize , inodesize , inode_max , inode_per_group, block_count;
__u32 	inode_per_block , inode_block_group, group;
blk_t 	block_nr;
__u32 	i, c_time, d_time, cr_time;
struct 	ext2_inode_large *inode;
__u16	crt_found = 0;

cm = (flag) ? 2*HIST_COUNT : HIST_COUNT ;

struct time_counter hist[(HIST_COUNT * 2)+1];
for (i = 0 ; i<= cm; i++){
	hist[i].c_count = 0;
	hist[i].d_count = 0;
	hist[i].cr_count = 0;
	hist[i].time = ((t_before - t_after)/ cm * i) + t_after;
}

blocksize = fs->blocksize;
inodesize = fs->super->s_inode_size;
inode_max = fs->super->s_inodes_count;
inode_per_group = fs->super->s_inodes_per_group;
buf = malloc(blocksize);

inode_per_block = blocksize / inodesize;
inode_block_group = inode_per_group / inode_per_block;

for (group = 0 ; group < fs->group_desc_count ; group++){
#ifdef EXT2_FLAG_64BITS
        gdp = ext2fs_group_desc(current_fs, current_fs->group_desc, group);
#else
        gdp = &current_fs->group_desc[group];
#endif
	zero_flag = 0;

	// NEXT GROUP IF INODE NOT INIT
	if (gdp->bg_flags & (EXT2_BG_INODE_UNINIT)) continue;

	// SET ZERO-FLAG IF FREE INODES == INODE/GROUP for fast ext3 
	if (gdp->bg_free_inodes_count == inode_per_group) zero_flag = 1;

//FIXME for struct ext4_group_desc 48/64BIT	
	for (block_nr = gdp->bg_inode_table , block_count = 0 ;
			 block_nr < (gdp->bg_inode_table + inode_block_group); block_nr++, block_count++) {

		// break if the first block only zero inode
		if ((block_count ==1) && (zero_flag == (inode_per_block + 1))) break;
	
		retval = read_block ( fs , &block_nr , buf);
		if (retval) return;

		for (i = (group * inode_per_group)+(block_count * inode_per_block) + 1 ,x = 0;
				 x < inode_per_block ; i++ , x++){

			if (i >= inode_max) break;	
			inode = (struct ext2_inode_large*) (buf + (x*inodesize));
			c_time = ext2fs_le32_to_cpu(inode->i_ctime);
			if (! c_time) {
				if(zero_flag) zero_flag++ ;
			 continue;
			}

			d_time = ext2fs_le32_to_cpu(inode->i_dtime);
//FIXME for bigendian
			cr_time = ((inodesize > EXT2_GOOD_OLD_INODE_SIZE) && (ext2fs_le16_to_cpu(inode->i_extra_isize) >= 24)) ?
					ext2fs_le32_to_cpu(inode->i_crtime) : 0 ;

			for (j=1;j <= cm ; j++){
				if ((d_time <= hist[j].time) && (d_time > hist[j-1].time)){
			 		hist[j].d_count++; 
			 break;
				}

				if (cr_time){
					if ((cr_time <= hist[j].time) && (cr_time > hist[j-1].time)){
							hist[j].cr_count++;
							crt_found = 1;
							cr_time = 0 ;
					}
				}

				if ((c_time <= hist[j].time) && (c_time > hist[j-1].time)){
		 			hist[j].c_count++; 
			 break;
				}
			}
		}
	}
}

	dump_hist(hist, cm, t_after, t_before, crt_found);

	if(buf) {
		free(buf);
		buf = NULL;
	}
return;
} 



// print the history of collectlist
void print_coll_list(__u32 t_after, __u32 t_before, int flag){

	int 	j, cm;

	__u32 		 c_time, d_time, cr_time;
	ext2_ino_t	i;
	ext2_ino_t 	*pointer;
	char 		inode_buf[256];
	struct 		ext2_inode_large *inode = (struct ext2_inode_large *)inode_buf;
	__u16		crt_found = 0;
	int		inode_size = EXT2_INODE_SIZE(current_fs->super);

	if (collect && collect->count){
		cm = (flag) ? 2*HIST_COUNT : HIST_COUNT ;

		struct time_counter hist[(HIST_COUNT * 2)+1];
		for (i = 0 ; i<= cm; i++){
			hist[i].c_count = 0;
			hist[i].d_count = 0;
			hist[i].cr_count = 0;
			hist[i].time = ((t_before - t_after)/ cm * i) + t_after;
		}
		pointer = collect->list;
		for (i = 0; i < collect->count; i++ ,pointer++){
			intern_read_inode_full(*pointer, (struct ext2_inode*)inode_buf , (inode_size > 256) ? 256 : inode_size );
			c_time = inode->i_ctime;
			d_time = inode->i_dtime;
			cr_time = ((inode_size > EXT2_GOOD_OLD_INODE_SIZE) && (inode->i_extra_isize >= 24)) ? inode->i_crtime : 0 ;
			
			for (j=1;j <= cm ; j++){
				if ((d_time < hist[j].time) && (d_time > hist[j-1].time)){
			 		hist[j].d_count++; 
			 break;
				}
				if (cr_time){
					if ((cr_time < hist[j].time) && (cr_time > hist[j-1].time)){
						hist[j].cr_count++;
						crt_found = 1;
						cr_time = 0 ;
					}
				}
				if ((c_time < hist[j].time) && (c_time > hist[j-1].time)){
					hist[j].c_count++; 
			 break;
				}
			}
		}
	
		dump_hist(hist, cm, t_after, t_before, crt_found);
		if (collect->list) free(collect->list);
		if (collect){
			free(collect);
			collect = NULL;
		}
	}
return;
} 



// search for a time before the last big delete job
__u32 get_last_delete_time(ext2_filsys fs)
{
struct ext2_group_desc		*gdp;
char 				*buf= NULL;
int 				zero_flag, x;
__u32 				blocksize , inodesize , inode_max , inode_per_group, block_count;
__u32 				inode_per_block , inode_block_group, group;
blk_t 				block_nr;
__u32 				i, c_time, d_time;
__u32				last = 0;
__u32				first = 0;
int				flag;

struct 	ext2_inode_large *inode;

blocksize = fs->blocksize;
inodesize = fs->super->s_inode_size;
inode_max = fs->super->s_inodes_count;
inode_per_group = fs->super->s_inodes_per_group;
buf = malloc(blocksize);

inode_per_block = blocksize / inodesize;
inode_block_group = inode_per_group / inode_per_block;

for (flag=0;flag<2;flag++){
	for (group = 0 ; group < fs->group_desc_count ; group++){
#ifdef EXT2_FLAG_64BITS
        	gdp = ext2fs_group_desc(current_fs, current_fs->group_desc, group);
#else
	        gdp = &current_fs->group_desc[group];
#endif
		zero_flag = 0;
	
		// NEXT GROUP IF INODE NOT INIT
		if (gdp->bg_flags & (EXT2_BG_INODE_UNINIT)) continue;
	
		// SET ZERO-FLAG IF FREE INODES == INODE/GROUP for fast ext3 
		if (gdp->bg_free_inodes_count == inode_per_group) zero_flag = 1;
	
		for (block_nr = gdp->bg_inode_table , block_count = 0 ;
				block_nr < (gdp->bg_inode_table + inode_block_group); block_nr++, block_count++) {
	
			// break if the first block only zero inode
			if ((block_count ==1) && (zero_flag == (inode_per_block + 1))) break;
		
			if ( read_block ( fs , &block_nr , buf))
				return 0;
	
			for (i = (group * inode_per_group)+(block_count * inode_per_block) + 1 ,x = 0;
					x < inode_per_block ; i++ , x++){
	
				if (i >= inode_max) break;	
				inode = (struct ext2_inode_large*) (buf + (x*inodesize));
				c_time = ext2fs_le32_to_cpu(inode->i_ctime);
				if (! c_time) {
					if(zero_flag) zero_flag++ ;
				continue;
				}
				d_time = ext2fs_le32_to_cpu(inode->i_dtime);
				if (flag){
					if ((d_time > (last-300)) && (d_time < first))
						first = d_time;
				}
				else{
					if (d_time > last){
						last = d_time;
					}
				}		
			}
		}
	}
if(!flag) first = last;
}

	if(buf) {
		free(buf);
		buf = NULL;
	}
	if (! first) fprintf(stderr,"Warning: No time window found, because no found a deleted inode\n");
return first - 1 ;
} 





// add inodenumber in a collectlist
 void add_coll_list(ext2_ino_t ino_nr ){
	ext2_ino_t *pointer;
	ext2_ino_t i;

	if (!collect){
		collect = malloc(sizeof(struct inode_nr_collect));
		collect->list = malloc(ALLOC_SIZE * sizeof(ext2_ino_t));
		if (!collect->list){
			free(collect);
			fprintf(stderr,"ERROR : can not allocate memory\n");
			return ;
		}
		collect->count = 0;
	}
	pointer = collect->list;
	for ( i = 0 ; i < collect->count; i++, pointer++ ){
		if (*pointer == ino_nr) 
			return ;
	}
	if ( i && (! (i % ALLOC_SIZE ))) {
// we must allocate more memory now	
		pointer = malloc((collect->count + ALLOC_SIZE +1) *sizeof(ext2_ino_t));
		if (!pointer){
			fprintf(stderr,"ERROR : can not allocate memory\n");
			return;
		}
		memcpy(pointer,collect->list,(collect->count *sizeof(ext2_ino_t)));
		free(collect->list);
		collect->list = pointer;
	}
	
	*((collect->list) + i) = ino_nr;
	collect->count++ ;
return;
}



//hexdump of block or data
void blockhex (FILE *out_file, void *buf, int flag, int blocksize)
{
        int i,j;
        int *intp;
        char *charp_0 , *charp_1 ;
        unsigned char c;

        intp = (int *) buf;
        charp_0 = (char *) buf;
        charp_1 = (char *) buf;

        for (i=0; i<blocksize; i+=16) {
                fprintf(out_file, "    %04x:  ", i);
               if (flag)  for (j=0; j<16; j+=4) fprintf(out_file, "%08x ", *intp++);
               else
                for (j=0; j<16; j++)  fprintf(out_file, "%02x ", 0xff & *charp_0++);
                        fprintf(out_file, "   ");
                for (j=0; j<16; j++) {
                        c = *charp_1++;
                        if (c < ' ' || c >= 127)
                                c = '.';
                        fprintf(out_file, "%c", c);
                }
                fprintf(out_file, "\n");
        }
}


//get inode type
char get_inode_mode_type( __u16 i_mode)
{
 char type;
 if (LINUX_S_ISDIR(i_mode)) type = 'd';
    else if (LINUX_S_ISREG(i_mode)) type = '_';
    else if (LINUX_S_ISLNK(i_mode)) type = 'l';
    else if (LINUX_S_ISBLK(i_mode)) type = 'b';
    else if (LINUX_S_ISCHR(i_mode)) type = 'c';
    else if (LINUX_S_ISFIFO(i_mode)) type = 'f';
    else if (LINUX_S_ISSOCK(i_mode)) type = 's';
    else type = ' ';
  return type;
}





/*
 * This function takes a __u32 time value and converts it to a string,
 * using ctime
 */
char *time_to_string(__u32 cl)
{
        static int      do_gmt = -1;
        time_t          t = (time_t) cl;
        const char      *tz;

        if (do_gmt == -1) {
                /* The diet libc doesn't respect the TZ environemnt variable */
                tz = getenv("TZ");
                if (!tz)
                        tz = "";
                do_gmt = !strcmp(tz, "GMT");
        }

        return asctime((do_gmt) ? gmtime(&t) : localtime(&t));
}



/*
 * This routine returns 1 if the filesystem is not open, and prints an
 * error message to that effect.
 */
int check_fs_open(char *name)
{
        if (!current_fs) {
                fprintf(stderr, "Filesystem not open\n");
                return 1;
        }
        return 0;
}


/*
 * This function resets the libc getopt() function, which keeps
 * internal state.  Bad design!  Stupid libc API designers!  No
 * biscuit!
 *
 * BSD-derived getopt() functions require that optind be reset to 1 in
 * order to reset getopt() state.  This used to be generally accepted
 * way of resetting getopt().  However, glibc's getopt()
 * has additional getopt() state beyond optind, and requires that
 * optind be set zero to reset its state.  So the unfortunate state of
 * affairs is that BSD-derived versions of getopt() misbehave if
 * optind is set to 0 in order to reset getopt(), and glibc's getopt()
 * will core dump if optind is set 1 in order to reset getopt().
 *
 * More modern versions of BSD require that optreset be set to 1 in
 * order to reset getopt().   Sigh.  Standards, anyone?
 *
 * We hide the hair here.
 */
void reset_getopt(void)
{
#if defined(__GLIBC__) || defined(__linux__)
        optind = 0;
#else
        optind = 1;
#endif
#ifdef HAVE_OPTRESET
        optreset = 1;           /* Makes BSD getopt happy */
#endif
}


//function for check argument by optarg
unsigned long parse_ulong(const char *str, const char *cmd,
                          const char *descr, int *err)
{
        char            *tmp;
        unsigned long   ret;

        ret = strtoul(str, &tmp, 0);
        if (*tmp == 0) {
                if (err)
                        *err = 0;
                return ret;
        }
        fprintf(stderr, "Bad %s - %s\n", descr, str);
        if (err)
                *err = 1;
        else
                exit(1);
        return 0;
}



extern ext2fs_block_bitmap 	  	d_bmap;


void get_nblock_len(char *buf, blk_t *blk, __u64 *p_len){
	(*blk)++;
	*p_len += current_fs->blocksize ;
return;
}


int get_ind_block_len(char *buf, blk_t *blk, blk_t *last ,blk_t *next, __u64 *p_len){
	int			i = (current_fs->blocksize >> 2)- 1;
	char			*priv_buf = NULL;
	blk_t			block = 0;
	blk_t			*p_block;
	int 			flag = 0;

	priv_buf = malloc(current_fs->blocksize);
	if (! priv_buf){
		fprintf(stderr,"can not allocate memory\n");
		return 0;
	}
	p_block = (blk_t*)buf;
	p_block += i;
	while ( i >=0 ){
		block = ext2fs_le32_to_cpu(*p_block);
		if (! block){
			i--;
			p_block-- ;
			flag++;
		}
		else break;
	}
	if ((block >= current_fs->super->s_blocks_count) ||
		      (!ext2fs_test_block_bitmap(d_bmap,block)) || (io_channel_read_blk ( current_fs->io,  block,  1,  priv_buf ))){
//		fprintf(stderr,"ERROR: while read block %10u\n",block);
		return 0;
	}
	*next = (flag) ? 0 : block+1 ;
	*last = block; 
	get_nblock_len(priv_buf, blk, p_len);
	*p_len += (i * (current_fs->blocksize)) ;
	*blk += (i + 1) ;
	free(priv_buf);
return 1;
}



int get_dind_block_len(char *buf, blk_t *blk, blk_t *last, blk_t *next, __u64 *p_len){
	int			ret = 0;
	int			i = (current_fs->blocksize >> 2)- 1;
	char			*priv_buf = NULL;
	blk_t			block = 0;
	blk_t			*p_block;

	priv_buf = malloc(current_fs->blocksize);
	if (! priv_buf){
		fprintf(stderr,"can not allocate memory\n");
		return 0;
	}
	p_block = (blk_t*)buf;
	p_block += i;
	while ( i >=0 ){
		block = ext2fs_le32_to_cpu(*p_block);
		if (! block){
			i--;
			p_block-- ;
		}
		else break;
	}
	if ((block >= current_fs->super->s_blocks_count) ||
		         (!ext2fs_test_block_bitmap(d_bmap,block)) || (io_channel_read_blk ( current_fs->io,  block,  1,  priv_buf ))){
//		fprintf(stderr,"ERROR: while read ind-block %10u\n",block);
		return 0;
	}
	
	ret = get_ind_block_len(priv_buf, blk, last, next, p_len);
	if (ret){
		*p_len += (i * current_fs->blocksize * (current_fs->blocksize >>2) ) ;
		*next = ( i == ((current_fs->blocksize >> 2)- 1)) ? *next : 0 ;
		*blk += ((i * ((current_fs->blocksize >>2) + 1)) + 1) ;
	}
	free(priv_buf);
return ret ;
}



int get_tind_block_len(char *buf, blk_t *blk, blk_t *last, blk_t *next, __u64 *p_len){
	int 			ret = 0;
	int			i = (current_fs->blocksize >> 2)- 1;
	char			*priv_buf = NULL;
	blk_t			block = 0;
	blk_t			*p_block;

	priv_buf = malloc(current_fs->blocksize);
	if (! priv_buf){
		fprintf(stderr,"can not allocate memory\n");
		return 0 ;
	}
	p_block = (blk_t*)buf;
	p_block += i;
	while ( i >=0 ){
		block = ext2fs_le32_to_cpu(*p_block);
		if (! block){
			i--;
			p_block-- ;
		}
		else break;
	}
	if ((block >= current_fs->super->s_blocks_count) ||
		            (ext2fs_test_block_bitmap(bmap,block)) ||(io_channel_read_blk ( current_fs->io,  block,  1,  priv_buf ))){
//		fprintf(stderr,"ERROR: while read dind-block %10u\n",block);
		return 0;
	}
	
	ret = get_dind_block_len(priv_buf, blk, last, next,p_len);
	if (ret){
		*p_len += (i * current_fs->blocksize * (current_fs->blocksize >>2) * (current_fs->blocksize >>2) ) ;
		*blk += ((i * ((current_fs->blocksize >>2) + 1) * (current_fs->blocksize >>2)) + 1) ;
		*next = 0;
	}
	free(priv_buf);
return ret;
}


int zero_space(unsigned char *buf, __u32 offset){
	__u32	i=offset;// +1;
	__u32 	end = (i)?((i + (current_fs->blocksize-1)) & ~(current_fs->blocksize-1)) : (i+current_fs->blocksize) ;
	
	while ((i<end) && (!buf[i]))
		i++;
	return (i == end ) ? 1 : 0 ;
}	

int is_unicode( unsigned char* buf){
	int 	ret = 0;
	unsigned char	*p = buf;
	unsigned char	*p1 = buf +1;
	if ((*p > 0xc1) && (*p < 0xf5) && (*p1 > 0x7f) && (*p1 < 0xc0)){
		if (!(*p & 0x20))
			ret = 2;
		else{
			p1++;
			if ((!(*p & 0x10)) && (*p1 > 0x7f) && (*p1 < 0xc0)) 
				ret = 3 ;
			else{
 				p1++;
				if ((!(*p & 0x08)) && (*p1 > 0x7f) && (*p1 < 0xc0)) 
					ret = 4 ;
			}
		}
	}
	return ret;
}

