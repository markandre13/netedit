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

#include "map.hh"
#include "../lib/common.hh"
#include "../lib/binary.hh"

using namespace netedit;

extern int verbose;

TMap::TMapMap TMap::mapmap;

TMap::~TMap()
{
  for(vector<TSymbol*>::iterator p = symbols.begin();
      p != symbols.end();
      ++p)
  {
    delete *p;
  }

  for(vector<TConnection*>::iterator p = connections.begin();
      p != connections.end();
      ++p)
  {
    delete *p;
  }
}

/**
 * Retrieve a map (map, symbols and connections) from the server
 */
TMap*
TMap::load(int mapid)
{
  EXEC SQL BEGIN DECLARE SECTION;
  int map_id = mapid;
  int symbol_id, conn_id, objid, s0, s1, x, y;
  char name[256];
  char type[256];
  EXEC SQL END DECLARE SECTION;

  TMap *map = 0;

  TMapMap::iterator mp = mapmap.find(map_id);
  if (mp==mapmap.end()) {
    map = new TMap;
    map->id = map_id;

    // symbols for node
    EXEC SQL DECLARE cur_sym_node CURSOR FOR
      SELECT
        symbol.symbol_id, symbol.id, symbol.xpos, symbol.ypos,
        node.sysName, icon.name
      FROM symbol, node, icon 
      WHERE
        symbol.map_id = :map_id AND
        symbol.id = node.node_id AND
        node.sysObjectID = icon.sysObjectID;

    EXEC SQL OPEN cur_sym_node;
    EXEC SQL WHENEVER NOT FOUND DO break;
    while(true) {
      *name = 0;
      *type = 0;
      EXEC SQL FETCH FROM cur_sym_node 
                     INTO :symbol_id, :objid, :x, :y, :name, :type;
      if (verbose>1)
        cout << symbol_id << ", "
             << objid << ", "
             << x << ", "
             << y << ", "
             << name << ", "
             << type << endl;
      map->addSymbol(symbol_id, objid, x, y, name, type);
    }
    EXEC SQL WHENEVER NOT FOUND SQLPRINT;
    EXEC SQL CLOSE cur_sym_node;

    // symbols for maps
    EXEC SQL DECLARE cur_sym_map CURSOR FOR
      SELECT
        symbol.symbol_id, symbol.id, symbol.xpos, symbol.ypos, map.name
      FROM symbol, map
      WHERE
        symbol.map_id = :map_id AND
        symbol.id = map.map_id;

    EXEC SQL OPEN cur_sym_map;
    EXEC SQL WHENEVER NOT FOUND DO break;
    while(true) {
      *name = 0;
      *type = 0;
      EXEC SQL FETCH FROM cur_sym_map 
                     INTO :symbol_id, :objid, :x, :y, :name;
      if (verbose>1)
        cout << "symbol: "
             << symbol_id << ", "
             << objid << ", "
             << x << ", "
             << y << ", "
             << name << endl;
      map->addSymbol(symbol_id, objid, x, y, name, "Map:Submap");
    }
    EXEC SQL WHENEVER NOT FOUND SQLPRINT;
    EXEC SQL CLOSE cur_sym_map;

    // connections
    EXEC SQL DECLARE cur_conn CURSOR FOR
      SELECT DISTINCT conn_id, id0, id1 FROM conn WHERE map_id = :map_id;

    EXEC SQL OPEN cur_conn;
    EXEC SQL WHENEVER NOT FOUND DO break;
    while(true) {
      EXEC SQL FETCH FROM cur_conn INTO :conn_id, :s0, :s1;
      if (verbose>1) {
        cout << "connection: "
             << conn_id << ", "
             << s0 << ", "
             << s1 << endl;
      }
      map->addConnection(conn_id, s0, s1);
    }
    EXEC SQL WHENEVER NOT FOUND SQLPRINT;
    EXEC SQL CLOSE cur_conn;

    mapmap[map_id] = map;
  } else {
    map = mp->second;
  }
  
  return map;
}

void
TMap::addSymbol(int symbol_id, int objid,
                int x, int y, const string &name,
                const string &type)
{
  TSymbol *s = new TSymbol;
  s->symbol_id = symbol_id;
  s->objid     = objid;
  s->x         = x;
  s->y         = y;
  s->sysName   = name;
  s->type      = type;
  symbols.push_back(s);
}

void
TMap::addConnection(int conn_id, int id0, int id1)
{
  TConnection *c = new TConnection;
  c->conn_id = conn_id;
  c->id0 = id0;
  c->id1 = id1;
  connections.push_back(c);
}

/**
 * Store map on DBMS and delete local copy.
 */
void
TMap::dropMap(TClient *client, int aMap)
{
  EXEC SQL BEGIN DECLARE SECTION;
  int map;
  EXEC SQL END DECLARE SECTION;
  
  map = aMap;
  
  TMapMap::iterator p = mapmap.find(map);
  if (p==mapmap.end()) {
    cout << "TMap::dropMap: map " << map << " isn't active" << endl;
    return;
  }
  TMap *m = p->second;
  set<TClient*>::iterator c = m->clients.find(client);
  if (c==m->clients.end()) {
    cout << "TMap::dropMap: client hasn't opened map" << endl;
    return;
  }
  
  m->clients.erase(c);
  if (m->clients.empty()) {
    cout << "store and free map " << map << endl;
    
//    EXEC SQL START TRANSACTION;

    // erase old map
    EXEC SQL DELETE FROM conn WHERE map_id = :map;
    EXEC SQL DELETE FROM symbol WHERE map_id = :map;

    // insert new map
    for(vector<TSymbol*>::iterator q = m->symbols.begin();
        q != m->symbols.end();
        ++q)
    {
      EXEC SQL BEGIN DECLARE SECTION;
      int id    = (*q)->symbol_id;
      int objid = (*q)->objid;
      int x     = (*q)->x;
      int y     = (*q)->y;
      EXEC SQL END DECLARE SECTION;
      EXEC SQL INSERT INTO symbol(map_id, symbol_id, id, xpos, ypos)
        VALUES (:map, :id, :objid, :x, :y);
    }
    
    for(vector<TConnection*>::iterator q = m->connections.begin();
        q != m->connections.end();
        ++q)
    {
      EXEC SQL BEGIN DECLARE SECTION;
      int id  = (*q)->conn_id;
      int id0 = (*q)->id0;
      int id1 = (*q)->id1; 
      EXEC SQL END DECLARE SECTION;
cout << "connection " << id0 << " and " << id1 << endl;
      EXEC SQL INSERT INTO conn(map_id, conn_id, id0, id1)
        VALUES (:map, :id, :id0, :id1);
    }
    
    EXEC SQL COMMIT;

    mapmap.erase(p);
  }
}

void
TMap::send(int fd)
{
  if (verbose>1)
    cout << "sending map: " << symbols.size() << " symbols, "
                            << connections.size() << " connections"
                            << endl;

  string msg;
  addDWord(&msg, 0);
  addDWord(&msg, CMD_OPEN_MAP);
  addSDWord(&msg, id);
  
  addDWord(&msg, symbols.size());
  for(vector<TSymbol*>::iterator p = symbols.begin();
      p != symbols.end();
      ++p)
  {
     addSDWord(&msg, (*p)->symbol_id);
     addSDWord(&msg, (*p)->objid);
     addSDWord(&msg, (*p)->x);
     addSDWord(&msg, (*p)->y);
     addString(&msg, (*p)->sysName);
     addString(&msg, (*p)->type);
  }

  addDWord(&msg, connections.size());
  for(vector<TConnection*>::iterator p = connections.begin();
      p != connections.end();
      ++p)
  {
    addSDWord(&msg, (*p)->conn_id);
    addSDWord(&msg, (*p)->id0);
    addSDWord(&msg, (*p)->id1);
  }
  
//  cout << "send map " << id << endl;

  setDWord(&msg, 0, msg.size());
  write(fd, msg.c_str(), msg.size());
}


int
TMap::addSymbol(int client, int map, int sym, int dx, int dy)
{
  TMapMap::iterator p = mapmap.find(map);
  if (p==mapmap.end()) {
    cout << "TMap::addSymbol: map " << map << " isn't active" << endl;
    return sym;
  }
  return p->second->addSymbol(client, sym, dx, dy);
}

int
TMap::addSymbol(int clientfd, int symbol_id, int x, int y)
{
  // check id
  if (symbol_id>=0) {
    cout << "TMap::addSymbol: device id isn't negative" << endl;
    return id;
  }

//cout << "TMap::addSymbol("<<id<<","<<x<<","<<y<<")\n";

  // allocate new id
  int new_id = 1;
  for(vector<TSymbol*>::iterator p = symbols.begin();
      p != symbols.end();
      ++p)
  {
    if ((*p)->symbol_id == new_id) {
      new_id = (*p)->symbol_id + 1;
      p = symbols.begin() - 1;
    }
  }

  // inform the client about the new id
  string cmd;
  addDWord(&cmd, 20);
  addDWord(&cmd, CMD_RENAME_SYMBOL);
  addSDWord(&cmd, this->id);  // map id
  addSDWord(&cmd, symbol_id); // old symbol id
  addSDWord(&cmd, new_id);    // new symbol id
  write(clientfd, cmd.c_str(), cmd.size());
cout << "send rename symbol " << id << " into " << new_id << endl;
  // store the new symbol
  addSymbol(new_id, 0, x, y, "unnamed", "unknown");
  
  // inform other clients about the new symbol
  cmd.clear();
  addDWord(&cmd, 24);   
  addDWord(&cmd, CMD_ADD_SYMBOL);
  addSDWord(&cmd, id);        // map id
  addSDWord(&cmd, new_id);    // new symbol id
  addSDWord(&cmd, x);
  addSDWord(&cmd, y);
  for(set<TClient*>::iterator p = clients.begin();
      p != clients.end();
      ++p)
  {
    if (clientfd == (*p)->fd)
      continue;
    write((*p)->fd, cmd.c_str(), cmd.size());
  }
  
  return new_id;
}

void
TMap::deleteSymbol(int client, int map, int sym)
{
  TMapMap::iterator p = mapmap.find(map);
  if (p==mapmap.end()) {
    cout << "TMap::deleteSymbol: map " << map << " isn't active" << endl;
  }
  p->second->deleteSymbol(client, sym);
}

#warning "id problem from add is repeated in delete code but unhandled"

void
TMap::deleteSymbol(int clientfd, int id)
{
cout << "TMap::deleteSymbol("<<id<<")\n";

  for(vector<TSymbol*>::iterator p = symbols.begin();
      p != symbols.end();
      ++p)
  {
    if ((*p)->symbol_id == id) {
      delete *p;
      symbols.erase(p);
      break;
    }
  }

  string cmd;
  addDWord(&cmd, 16);
  addDWord(&cmd, CMD_DELETE_SYMBOL);
  addSDWord(&cmd, this->id);  // map id
  addSDWord(&cmd, id);        // symbol id

  for(set<TClient*>::iterator p = clients.begin();
      p != clients.end();
      ++p)
  {
    if (clientfd == (*p)->fd)
      continue;
    write((*p)->fd, cmd.c_str(), cmd.size());
  }
}

void
TMap::translateSymbol(int client, int map, int sym, int dx, int dy)
{
  TMapMap::iterator p = mapmap.find(map);
  if (p==mapmap.end()) {
    cout << "TMap::translateSymbol: map " << map << " isn't active" << endl;
    return;
  }
  p->second->translateSymbol(client, sym, dx, dy);
}

void
TMap::translateSymbol(int clientfd, int sym, int dx, int dy)
{
  string cmd;
  addDWord(&cmd, 24);   
  addDWord(&cmd, CMD_TRANSLATE_SYMBOL);
  addSDWord(&cmd, id);
  addSDWord(&cmd, sym);
  addSDWord(&cmd, dx);
  addSDWord(&cmd, dy);
  for(set<TClient*>::iterator p = clients.begin();
      p != clients.end();
      ++p)
  {
    if (clientfd == (*p)->fd)
      continue;
    write((*p)->fd, cmd.c_str(), cmd.size());
  }
  
  for(vector<TSymbol*>::iterator p = symbols.begin();
      p != symbols.end();
      ++p)
  {
    if ((*p)->symbol_id == sym) {
      (*p)->x += dx;
      (*p)->y += dy;
      break;
    }
  }
}

int
TMap::addConnection(int client, int map, int conn_id, int sym0, int sym1)
{
  TMapMap::iterator p = mapmap.find(map);
  if (p==mapmap.end()) {
    cout << "TMap::addConnection: map " << map << " isn't active" << endl;
    return conn_id;
  }
  return p->second->addConnection(client, conn_id, sym0, sym1);
}

int
TMap::addConnection(int clientfd, int conn_id, int sym0, int sym1)
{
  // check id
  if (conn_id>=0) {
    cout << "TMap::addConnection: conn_id id isn't negative" << endl;
    return conn_id;
  }

//cout << "TMap::addSymbol("<<id<<","<<x<<","<<y<<")\n";

  // allocate new id
  int new_id = 1;
  for(vector<TConnection*>::iterator p = connections.begin();
      p != connections.end();
      ++p)
  {
    if ((*p)->conn_id == new_id) {
      new_id = (*p)->conn_id + 1;
      p = connections.begin() - 1;
    }
  }

  // inform the client about the new id
  string cmd;
  addDWord(&cmd, 20);
  addDWord(&cmd, CMD_RENAME_CONNECTION);
  addSDWord(&cmd, this->id);  // map id
  addSDWord(&cmd, conn_id);   // old symbol id
  addSDWord(&cmd, new_id);    // new symbol id
  write(clientfd, cmd.c_str(), cmd.size());
cout << "send rename connection " << conn_id << " into " << new_id << endl;
  // store the new symbol
  addConnection(new_id, sym0, sym1);
  
  // inform other clients about the new symbol
  cmd.clear();
  addDWord(&cmd, 24);   
  addDWord(&cmd, CMD_ADD_CONNECTION);
  addSDWord(&cmd, this->id);  // map id
  addSDWord(&cmd, new_id);    // new symbol id
#warning "reverse mapping of IDs may be required..."
  addSDWord(&cmd, sym0);
  addSDWord(&cmd, sym1);
  for(set<TClient*>::iterator p = clients.begin();
      p != clients.end();
      ++p)
  {
    if (clientfd == (*p)->fd)
      continue;
    write((*p)->fd, cmd.c_str(), cmd.size());
  }
  
  return new_id;
}

void
TMap::deleteConnection(int client, int map, int conn)
{
  TMapMap::iterator p = mapmap.find(map);
  if (p==mapmap.end()) {
    cout << "TMap::deleteConnection: map " << map << " isn't active" << endl;
  }
  p->second->deleteConnection(client, conn);
}

#warning "id problem from add is repeated in delete code but unhandled"

void
TMap::deleteConnection(int clientfd, int id)
{
cout << "TMap::deleteConnection("<<id<<")\n";

  for(vector<TSymbol*>::iterator p = symbols.begin();
      p != symbols.end();
      ++p)
  {
    if ((*p)->symbol_id == id) {
      delete *p;
      symbols.erase(p);
      break;
    }
  }

  string cmd;
  addDWord(&cmd, 16);
  addDWord(&cmd, CMD_DELETE_CONNECTION);
  addSDWord(&cmd, this->id);  // map id
  addSDWord(&cmd, id);        // symbol id

  for(set<TClient*>::iterator p = clients.begin();
      p != clients.end();
      ++p)
  {
    if (clientfd == (*p)->fd)
      continue;
    write((*p)->fd, cmd.c_str(), cmd.size());
  }
}
