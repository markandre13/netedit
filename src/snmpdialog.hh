/*
 * NetEdit -- A network management tool
 * Copyright (C) 2003-2005 by Mark-André Hopf <mhopf@mark13.org>
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
#ifndef _NETEDIT_SNMPDIALOG_HH
#define _NETEDIT_SNMPDIALOG_HH

#include <toad/dialog.hh>
#include <toad/textmodel.hh>
#include "snmp.hh"

namespace netedit {

using namespace toad;

class TSNMPQueryModel;

class TSNMPDialog:
  public TDialog
{
  public:
    typedef TDialog super;
    
    TSNMPDialog(TWindow*, const string&);
    ~TSNMPDialog();
    
    TTextModel hostname;
    TTextModel community;
    TTextModel oid;
    
    TSNMPQuery *query;
    TSNMPQueryModel *tree;
    TTextModel result;

    TTextModel description;
    TTextModel syntax;
    TTextModel value;
    TTextModel name;

  protected:
    void snmpWalk();
    void stop();
};

} // namespace netedit

#endif
