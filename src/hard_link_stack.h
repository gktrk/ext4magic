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

//construct for global collect of hardlinks

#ifndef HARD_LINKSTACK_H
#define HARD_LINKSTACK_H
#include <ext2fs/ext2fs.h>

struct link_entry{
	ext2_ino_t 	inode_nr;
	__u32		generation;	
	int 		link_count;
	char*		name;
	struct link_entry* next;
};

struct link_stack_head{
	__u32 count;
	struct link_entry* begin;
	struct link_entry* pointer;
};


void init_link_stack();
void add_link_stack(ext2_ino_t , __u32, char*, __u32 );
char* check_link_stack(ext2_ino_t, __u32);
int  match_link_stack(ext2_ino_t, __u32 );
void clear_link_stack();
int rename_hardlink_path(char*, char*);

#endif
