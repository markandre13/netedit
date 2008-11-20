/*
 * NetEdit -- A network management tool
 * Copyright (C) 2003-2006 by Mark-André Hopf <mhopf@mark13.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or   
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the  
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __NETEDIT_COMMON_HH
#define __NETEDIT_COMMON_HH

enum {
  CMD_LOGIN = 1,

  CMD_GET_MAPLIST,
  CMD_OPEN_MAP,
  CMD_CLOSE_MAP,
  CMD_ADD_MAP,
  CMD_RENAME_MAP,
  CMD_DELETE_MAP,

  CMD_ADD_SYMBOL,
  CMD_RENAME_SYMBOL,
  CMD_DELETE_SYMBOL,
  CMD_TRANSLATE_SYMBOL,

  CMD_ADD_CONNECTION,
  CMD_RENAME_CONNECTION,
  CMD_DELETE_CONNECTION,
  CMD_EDIT_CONNECTION,

  CMD_ADD_NODE,
  CMD_DELETE_NODE,
  CMD_OPEN_NODE,
  CMD_CLOSE_NODE,
  CMD_SET_NODE,
  CMD_UPDATE_NODE,
  CMD_LOCK_NODE,
  CMD_UNLOCK_NODE
};

enum {
  NODE_UNLOCKED,
  NODE_LOCKED_LOCAL,
  NODE_LOCKED_REMOTE,
  NODE_IS_NOT
};

enum ELock {
  UNLOCKED,
  LOCKED_LOCAL,
  LOCKED_REMOTE
};

#endif
