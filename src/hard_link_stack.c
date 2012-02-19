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

//construct for global collect of hardlinks

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include "hard_link_stack.h"

static struct link_stack_head head;

void init_link_stack(){
	head.count = 0;
	head.begin = NULL;
	head.pointer = NULL;
}


void add_link_stack(ext2_ino_t inode_nr, __u32 link_count, char* name, __u32 generation){
	struct link_entry* this;
	
	this = malloc (sizeof(struct link_entry));
	if (!this) 
		goto errout;
	this->inode_nr = inode_nr;
	this->link_count = link_count -1 ;
	this->name = malloc(strlen(name) +1);
	this->generation = generation;
	if (!this->name) 
		goto errout;
	strcpy(this->name,name);
	this->next = head.begin;
	head.begin = this;
	head.count++;	
return;

errout:
	if(this->name)
		free(this->name);
	if(this)
		free(this);
}


// subfunction for check_find_dir() use in stage 2 of magical recover 
int rename_hardlink_path(char *old, char *neu){
	char *newname;
	char *endname;
	int size = strlen(old);
	
	head.pointer = head.begin;
	while (head.pointer) {
		if (! strcmp(old,head.pointer->name)){
			newname=malloc(strlen(neu) + strlen(head.pointer->name) - size +1);
			if (! newname)
				return 1;				 
			strcpy(newname,neu);
			endname = head.pointer->name + size;
			strcat(newname,endname);
			free(head.pointer->name);
			head.pointer->name = newname;
//#ifdef DEBUG
			fprintf(stderr,"HL-DB change %s -> %s\n",old,neu);
//#endif
		}
		head.pointer = head.pointer->next;
	}
	return 0;
}




		
char* check_link_stack(ext2_ino_t inode_nr, __u32 generation){

	head.pointer = head.begin;
	while (head.pointer) {
		 if((head.pointer->inode_nr == inode_nr) && (head.pointer->generation == generation))
			break;
		head.pointer = head.pointer->next;
	}

#ifdef DEBUG
	if (head.pointer)
		printf("HARD_LINK found -> %s\n",head.pointer->name);
#endif
	return (head.pointer) ? head.pointer->name : NULL ;
}


// not used ; gcc warning okay
static void del_link_stack(struct link_entry* entry){
	if(entry->name)	
		free(entry->name);
	head.pointer = head.begin;
	if (head.begin->next){
		while ((head.pointer->next)  && (head.pointer != entry) && (head.pointer->next != entry))
			head.pointer = head.pointer->next;
		if(head.begin == entry)
			head.begin = entry->next;
		else
			head.pointer->next = entry->next;
	}
	else 
		head.begin = NULL;
	free(entry);
	head.count--;		
}



int  match_link_stack(ext2_ino_t inode_nr, __u32 generation){
	int retval = 1;
	if ((head.pointer->inode_nr == inode_nr) && (head.pointer->generation == generation)){
//		if (! --(head.pointer->link_count))
//			del_link_stack(head.pointer);
		(head.pointer->link_count)-- ;
		retval = 0;
	}
return retval;
}



void clear_link_stack(){
	int d_count = 0 ;

	fflush(stdout);
	if (head.count){
		fprintf(stderr,"Hardlink Database : %lu entries\n", (long unsigned int)head.count);
	
		head.pointer = head.begin;
		while (head.pointer){
			if(head.pointer->link_count){
				fprintf(stderr,"%10d\t%s\n",head.pointer->link_count,head.pointer->name);
				d_count++ ;
			}
			if(head.pointer->name)	
				free(head.pointer->name);
			head.begin = head.pointer->next;
			free(head.pointer);
			head.pointer = head.begin;
		}
		if (! d_count) 
			fprintf(stderr,"all Hardlinks be resolved\n");
	}
}

	
	