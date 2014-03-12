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
#ifndef EXT4MAGIC_H
#define EXT4MAGIC_H

// ext4magic mode for navigation main() 
#define PRINT_SUPERBLOCK             0x0001
#define PRINT_J_SUPERBLOCK           0x0002
#define COMMAND_INODE                0x0004
#define COMMAND_BLOCK                0x0008
#define COMMAND_PATHNAME	     0x0010
#define PRINT_TRANSACTION            0x0020	
#define PRINT_BLOCKLIST              0x0040
#define PRINT_HISTOGRAM              0x0080
#define INPUT_TIME                   0x0100
#define HIGH_QUALITY                 0x0200
#define READ_JOURNAL                 0x1000
#define RECOVER_INODE                0x2000
#define RECOVER_LIST		     0x4000

#define MASK_MAGIC_SCAN		     0x3100
// journal status flags
#define JOURNAL_OPEN		     0
#define JOURNAL_CLOSE		     1
#define JOURNAL_ERROR		     2

#endif


