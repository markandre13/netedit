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

#ifndef __NETEDITD_MAP_HH

#include "client.hh"
#include <map>
#include <set>
#include <vector>
#include <string>

namespace netedit {

using namespace std;

class TMap
{
  typedef map<int, TMap*> TMapMap;
  static TMapMap mapmap;

  public:
    ~TMap();
  
    int id;
    static TMap* load(int map_id);
    void send(int fd);
    
    struct TSymbol {
      int symbol_id;
      int objid;
      int x, y;
      string sysName;
      string type;
    };
    vector<TSymbol*> symbols;
    
                   
    struct TConnection {
      int conn_id;
      int id0, id1;
    };
    vector<TConnection*> connections;

    // list of clients using this map
    // (used to distribute changes to all clients and to copy the map
    // back to the DBMS when no client is using it anymore)
    set<TClient*> clients;
    
    static int addSymbol(int clientfd, int map, int sym, int x, int y);
    int addSymbol(int clientfd, int sym, int x, int y);

    static void deleteSymbol(int clientfd, int map, int sym);
    void deleteSymbol(int clientfd, int sym);
    
    static void translateSymbol(int clientfd, int map, int sym, int dx, int dy);
    void translateSymbol(int clientfd, int sym, int dx, int dy);

    static int addConnection(int clientfd, int map, int conn_id, int sym0, int sym1);
    int addConnection(int map, int conn_id, int sym0, int sym1);

    static void deleteConnection(int clientfd, int map, int conn);
    void deleteConnection(int clientfd, int conn);

    static void dropMap(TClient*, int map);

  private:
    // utility methods
    void addSymbol(int symbol_id, int objid, int x, int y, const string &name, const string &type);
    void addConnection(int conn_id, int id0, int id1);
};

} // namespace netedit

#endif
