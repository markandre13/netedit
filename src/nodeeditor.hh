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

#ifndef _NETEDIT_NODEEDITOR_HH
#define _NETEDIT_NODEEDITOR_HH

#include <toad/dialog.hh>
#include <toad/textmodel.hh>
#include <toad/boolmodel.hh>
#include <toad/stl/vector.hh>
#include "../lib/common.hh"
#include "server.hh"

namespace netedit {

using namespace toad;

class TLockModel:
  public TModel
{
    ELock lock;
  public:
    TLockModel() {
      lock = UNLOCKED;
    }
    ELock get() const { return lock; }
    void set(ELock lock) { 
      if (this->lock==lock)
        return;
      this->lock = lock;
      sigChanged(); 
    }
    string login;
    string hostname;
    time_t since;
};

class TInterface { 
  public:
    int interface_id;  
    int status;
    int flags;
    string ipaddress;
    int ifIndex;  
    string ifDescr;
    int ifType;
    string ifPhysAddress;
};

class TNodeModel
{
  public:
    TNodeModel();
    ~TNodeModel();
  
    int refcount;
    int node_id;
    TTextModel sysObjectID;
    TTextModel sysName;
    TTextModel sysContact;
    TTextModel sysLocation;
    TTextModel sysDescr;
    TBoolModel ipforwarding;
    TTextModel mgmtaddr;
    unsigned mgmtflags;
    unsigned topoflags;

    typedef GVector<TInterface*> TInterfaces;
    TInterfaces interfaces;
    
    TLockModel lock;

    void fetch(const string &buffer, unsigned *p);
    void readwrite(bool rw);
};

class TNodeEditor:
  public TDialog
{
  public:
    typedef TDialog super;
    
    TNodeEditor(TWindow*, const string&, TNodeModel *model, TServer *server);
    virtual ~TNodeEditor();
    
    PServer server;
    TNodeModel *model;

    void closeRequest();
    
    enum EButton {
      BTN_RESET, BTN_APPLY, BTN_CANCEL, BTN_OK
    };
    TPushButton *reset, *apply, *cancel, *ok;
    
    void button(int);
    void update();
};

} // namespace netedit

#endif
