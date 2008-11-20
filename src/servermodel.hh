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
#if 0

#ifndef __NETEDIT_SERVERMODEL_HH
#define __NETEDIT_SERVERMODEL_HH

#include <string>
#include <vector>

namespace netedit {

using namespace std;

class TMapModel;

class TServerModel {
  public:
    bool open(const string &path);
    bool insertFile(const string &file);
    unsigned getMapCount() const { return maps.size(); }
    const string& getMapName(unsigned map) const {
      return maps[map].name;
    }
    TMapModel* getNetModel(unsigned map);
  protected:
    struct Object {
      string id;
      int x, y;
      string sysName;
      string type;
      string label;
      string submap;
      string sysLocation;
    };
    struct Connection {
      string id0;
      string if0;
      string id1;
      string if1;
      string label;
    };
    struct Map {
      string name;
      string comment;
      vector<Object> objects;
      vector<Connection> connections;
    };
    vector<Map> maps;
};

} // namespace netedit

#endif

#endif