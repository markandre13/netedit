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
#ifndef __NETEDIT_SERVER_HH
#define __NETEDIT_SERVER_HH 1

#include <toad/model.hh>
#include <toad/ioobserver.hh>
#include <toad/stl/vector.hh>
#include <toad/table.hh>
#include <string>

#include "symbol.hh"

namespace netedit {

class TMapModel;
class TNodeEditor;

using namespace std;
using namespace toad;

class TServer:
  public TModel, TIOObserver
{
    int sock;
    string buffer;
    
    struct MapListEntry {
      MapListEntry(int map_id, const string &name) {
        this->map_id = map_id;
        this->name   = name;
      }
      MapListEntry(const MapListEntry &obj) {
        map_id = obj.map_id;
        name   = obj.name;
      }
      int map_id;
      string name;
    };

  public:    
    typedef GVector<MapListEntry> MapList;
    MapList maplist;
    TSingleSelectionModel map;
    TMapModel *netmodel;

    TServer(const string &hostname, unsigned port);
    
    enum EReason {
      MAPLIST_AVAILABLE,
      NETMODEL_CHANGED
    } reason;

    void sndLogin(const string &login, const string &passwd);
    
    void sndGetMapList();
    unsigned getMapIDByRow(unsigned);
    void sndGetMapModelByRow(unsigned maplistrow);
    void sndGetMapModel(unsigned map_id);
    void sndDropMapModel(int map);

    void sndAddSymbol(int map, int sym, int x, int y);
    void sndRenameSymbol(int map, int old_id, int new_id);
    void sndDeleteSymbol(int map, int sym);
    void sndTranslateSymbol(int map, int sym, int x, int y);

    void sndConnectSymbol(int map, int conn, int sym0, int sym1);
    void sndRenameConnection(int map, int old_id, int new_id);
    void sndDeleteConnection(int map, int conn);

    void sndOpenNode(int id);
    void sndCloseNode(int id);
    void sndSetNode(TNodeEditor*);
    
    void sndLockNode(int id);
    void sndUnlockNode(int id);

  protected:
    void canRead();
};

typedef GSmartPointer<TServer> PServer;

} // namespace

#endif
