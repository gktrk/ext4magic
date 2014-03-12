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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifndef DIR_LIST_H
#define DIR_LIST_H
#include <ext2fs/ext2fs.h>

//Dir-List item
struct dir_list_t	{
	struct dir_list_t 	*next;
	char			*filename;
	ext2_ino_t		inode_nr;
	char			entry;
};

//Dir-List header
struct dir_list_head_t  {
	struct dir_list_t 	*next;
	struct dir_list_t	*last;
	char 			*dirname;
	char 			*pathname;
	ext2_ino_t		path_inode;
	ext2_ino_t		dir_inode;
	int			count;
};

//function
struct dir_list_t* add_list_item (struct dir_list_head_t*, ext2_ino_t, char* ,char);
int clear_dir_list(struct dir_list_head_t*);
struct  dir_list_head_t* new_dir_list ( ext2_ino_t, ext2_ino_t, char*, char*);
struct  dir_list_head_t* clean_up_dir_list(struct dir_list_head_t* );

#define GET_FIRST(h)  ((h)->count) ? (h)->next : NULL
#define GET_NEXT(h,i) ((i)->next != (struct dir_list_t*) (h)) ? (i)->next : NULL  


#endif
