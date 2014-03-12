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

// A special linked list for collect directory entry
// Used for manage old directory blocks in journal
 
#include <stdio.h>
#include <stdlib.h>
#include "dir_list.h"


// add new Dir-List item
struct dir_list_t* add_list_item (struct dir_list_head_t *head , ext2_ino_t nr, char* name ,char entry){
	struct dir_list_t *this;
	if( !nr || (! strcmp(name,"")))
		return head->last;
	
	this = malloc(sizeof(struct dir_list_t));
	if (! this)
		 goto errout;
    	this->filename = malloc(strlen(name) + 1);
	if (! this->filename) 
		 goto errout;

	head->last->next = this;
	head->last = this;
	this->next = (struct dir_list_t*) head;
	head->count++;
	this->entry = entry;
	this->inode_nr = nr;
	strcpy(this->filename,name);
	return (this);

errout:
	fprintf(stderr,"no free memory\n");
	if (this->filename) free(this->filename);
	if (this) free(this);
	return NULL;
}	


// destroy Dir-List
int clear_dir_list(struct dir_list_head_t *head)
{	
	struct dir_list_t	 *next;
	struct dir_list_t	 *this;

	next = head->next;
	while (next != (struct dir_list_t*) head){
		this = next;
		if (next->filename) 
			free(next->filename);
		next = this->next;
		free(this);
	}
	free(head->pathname);
	free(head);
return 0;
} 
		
		
// create a new Dir-List
struct  dir_list_head_t* new_dir_list (ext2_ino_t path_inode, ext2_ino_t dir_inode, char *path, char *name){
	struct dir_list_head_t 	*this; 
	int 			len_n;

	this = malloc(sizeof(struct dir_list_head_t));
	if (! this) return NULL;
	this->next = (struct dir_list_t*) this;
	this->last = (struct dir_list_t*) this;
	this->path_inode = path_inode;
	this->dir_inode = dir_inode;
	this->count = 0;	

	len_n = strlen(path) + strlen(name) +2;
	this->pathname = malloc(len_n);

	if (this->pathname) {
		strcpy(this->pathname, path);
		if ((strlen(path) > 0) && strcmp(path,"/"))
			strcat(this->pathname,"/");
		this->dirname = strchr(this->pathname,0);
		strcat(this->pathname , name);
	}
	else
	{
		if (this->pathname) free(this->pathname);
		free(this);
		return NULL;
	}
	return(this);
}


//local helper func ;check for duplicate values
static char d_find_entry(struct dir_list_head_t* dir , ext2_ino_t ino , char *filename){
	char ret = 0;
	struct dir_list_t *pointer = dir->next;
	while (pointer != (struct dir_list_t*) dir){
		if (strcmp(pointer->filename,filename))
			pointer = pointer->next;
		else {
			if (pointer->inode_nr == ino)
				return 1; 
			pointer = pointer->next;
		}
	}
return ret;
}	


// sort out invalid values by data copy
struct  dir_list_head_t* clean_up_dir_list(struct dir_list_head_t* o_dir ){
	struct dir_list_head_t* n_dir;
	int i;
	char *p;
	struct dir_list_t *pointer;


	n_dir = new_dir_list(o_dir->path_inode,o_dir->dir_inode," "," ");
	if (!n_dir) 
		return o_dir;

//change the allocatet names
	p = n_dir->pathname;
	n_dir->pathname = o_dir->pathname;
	o_dir->pathname = p;
	
	p = n_dir->dirname;
	n_dir->dirname = o_dir->dirname;
	o_dir->dirname = p;

//copy all practicable values
	pointer = o_dir->next;
	for (i = o_dir->count ; i>0 ; i-- , pointer = pointer->next ){
		if ( (!pointer->inode_nr) || (pointer->filename[0] == 0)  )
			continue;
		
		switch (pointer->entry){
		  case 	DIRENT_DOT_FILE:
//			 break;
		  case 	DIRENT_DOT_DOT_FILE:
			 break;
		  case	DIRENT_OTHER_FILE:
//			if (d_find_entry(n_dir , pointer->inode_nr , pointer->filename))
//			 	continue;
//			break;					
		  case	DIRENT_DELETED_FILE:
			if (d_find_entry(n_dir , pointer->inode_nr , pointer->filename)||
				(pointer->inode_nr < EXT2_GOOD_OLD_FIRST_INO))
			 	continue;
			break;
		  default:
			continue;
		}
		
		if(! add_list_item (n_dir, pointer->inode_nr , pointer->filename,0))
			fprintf(stderr, "ERROR by copy of dirlist");	
		
	}
clear_dir_list(o_dir);
return n_dir;
}	
		



	
