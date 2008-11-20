/*
 * NetEdit -- A network management tool
 * Copyright (C) 2003 by Mark-André Hopf <mhopf@mark13.de>
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

#ifndef OIDNODE_HH
#define OIDNODE_HH

#include <stdlib.h>
#include <string>

struct TObjectSyntax
{
  TObjectSyntax() {
    type = UNKNOWN;
    min = 0;
    max = 0;
    optional = false;
  }
  std::string name;
  enum {
    UNKNOWN,
    ANY,
    INTEGER,
    BIT_STRING,
    OCTET_STRING,
    DISPLAY_STRING,
    OBJECT_IDENTIFIER,
    CHOICE_OF,
    SEQUENCE
  } type;
  unsigned min, max;
  bool optional;
};

struct TObjectType
{
  TObjectType() {
    syntax = 0;
  }
  ~TObjectType() {
    if (syntax)
      delete syntax;
  }
  std::string description;
  TObjectSyntax *syntax;
};
  

/**
 * \class TOIDNode
 *
 * iso    : TOIDNode(-1, text)
 * org(3) : TOIDNode( 3, text)
 * 1      : TOIDNode( 1, 0)
 */

struct TOIDNode
{
  TOIDNode(int oid, char *name) {
    this->oid = oid;
    this->name = name;
    next = down = parent = 0;
    type = 0;
  }
  ~TOIDNode() {
    if (name)
      free(name);
    if (next)
      delete next;
    if (down)
      delete down;
    if (type)
      delete type;
  }
  int oid;     // -1 == none
  char *name;  //  0 == none
  TObjectType *type;
  
  std::string description;

  TOIDNode *parent;
  TOIDNode *next;
  TOIDNode *down;
  
  void print();
};

#endif
