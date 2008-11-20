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

#include "mapmodel.hh"

using namespace netedit;

TMapModel::TMapModel(TServer *server, int id) {
//  cout << "new map model for map " << id << endl;
  this->server = server;
  this->id = id;
  itsme = false;
}

TMapModel::~TMapModel() {
//   cout << "deleted map model for map " << id << endl;
   if (server)
     server->sndDropMapModel(id);
}

/**
 * Return an unique temporary ID within the net model for device d.
 * \param d
 *   ignore this symbol during search
 */
int
TMapModel::uniqueDeviceID(TSymbol *d) const
{
  int id = -1;
  bool collision;
  do {
    collision = false;
    for(const_iterator p=begin();
        p != end();
        ++p)
    {
      TSymbol *s = dynamic_cast<TSymbol*>(*p);
      if (!s || s==d)
        continue;
      if (id==s->id) {
        --id;
        collision = true;
      }
    }
  } while(collision);
  return id;
}


TSymbol*
TMapModel::deviceByID(int id) const
{
  for(const_iterator p=begin();
      p != end();
      ++p)
  {
    TSymbol *nd = dynamic_cast<TSymbol*>(*p);
    if ( nd && nd->id == id )
      return nd;
  }
  return NULL;
}

TConnection*
TMapModel::connByID(int id) const
{
  for(TConnections::const_iterator p=connections.begin();
      p != connections.end();
      ++p)
  {
    if ((*p)->conn_id == id )
      return *p;
  }
  return NULL;
}

/**
 * Return an unique temporary ID within the net model for device d.
 */
int
TMapModel::uniqueConnID() const
{
  int id = -1;
  bool collision;
  do {
    collision = false;
    for(TConnections::const_iterator p=connections.begin();
        p != connections.end();
        ++p)
    {
      if (id==(*p)->conn_id) {
        --id;
        collision = true;
      }
    }
  } while(collision);
  return id;
}

void
TMapModel::connectDevice(int conn_id, TSymbol *nd0, TSymbol *nd1)
{
  if (!nd0 || !nd1) {
    cout << "TMapModel::connectDevice: NULL argument" << endl;
    return;
  }
  if (nd0==nd1)
    return;
  TConnection *c = new TConnection(conn_id, nd0, nd1);
  connections.push_back(c);
  insert(begin(), c);
}

void
TMapModel::addSymbol(int id, int x, int y)
{
  TSymbol *f = new TSymbol;
  f->id = id;
  f->x  = x;
  f->y  = y;
  storage.push_back(f);
  
  type = ADD;
  figures.clear();
  figures.insert(f);
  sigChanged();
}

void
TMapModel::renameSymbol(int old_id, int new_id)
{
  TSymbol *d = deviceByID(old_id);
  if (!d) {
    cout << "TMapModel::renameSymbol: unknown symbol " << old_id << endl;
    return;
  }
//cout << "renamed symbol " << old_id << " into " << new_id << endl;
  d->id = new_id;
}

void
TMapModel::deleteSymbol(int id)
{
  TSymbol *d = deviceByID(id);
  if (!d) {
    cout << "TMapModel::deleteSymbol: unknown symbol " << id << endl;
    return;
  }

  for(iterator p=storage.begin();
      p != storage.end();
      ++p)
  {
    TSymbol *nd = dynamic_cast<TSymbol*>(*p);
    if ( nd && nd->id == id ) {
      type = REMOVE;
      figures.clear();
      figures.insert(nd);
      sigChanged();

#warning "not removing connections"

      delete *p;

      storage.erase(p);

      break;
    }
  }
}

/**
 * translate a symbol without notifing the server
 * (used when handling requests from the server)
 */
void
TMapModel::translateSymbol(int id, int dx, int dy)
{
  TSymbol *d = deviceByID(id);
  if (!d) {
    cout << "TMapModel::translateSymbol: unknown symbol " << id << endl;
    return;
  }

//  cout << "from server: translate symbol " << id << " by " << dx << ", " << dy << endl;
  
  figures.clear();
  figures.insert(d);
  type = MODIFY;
  sigChanged();
  
  d->x += dx;
  d->y += dy;
  
  type = MODIFIED;
  sigChanged();
  
  // undo?
}

void
TMapModel::addConnection(int conn_id, int sym0, int sym1)
{
  connectDevice(conn_id,
                deviceByID(sym0),
                deviceByID(sym1));
}

void
TMapModel::renameConnection(int old_id, int new_id)
{
//cout << "rename connection " << old_id << " to " << new_id << endl;
  TConnection *d = connByID(old_id);
  if (!d) {
    cout << "TMapModel::renameConnection: unknown connection " << old_id << endl;
    return;
  }
  d->conn_id = new_id;
}

void
TMapModel::deleteConnection(int id)
{
  TConnection *c = connByID(id);
  if (!c) {
    cout << "TMapModel::deleteConnection: unknown connection " << id << endl;
    return;
  }
cout << "TMapModel::deleteConnection going to erase connection" << endl;
  itsme = true;
  TFigureSet set;
  set.insert(c);
  erase(set);
  itsme = false;
cout << "TMapModel::deleteConnection erased connection" << endl;
}


void
TMapModel::erase(TFigureSet &set)
{
  if (set.empty())
    return;

  // in case one of the connections points to one of the figures
  // in set, add 'em to the set to avoid dangling pointers
  for(TConnections::iterator q = connections.begin();
      q != connections.end();
      ++q)
  {
    if ( set.find((*q)->nd0) != set.end() ||
         set.find((*q)->nd1) != set.end() )
    {
//      cout << "  erase connection" << endl;
      set.insert(*q);
      --q;
    }
  }

  // also remove TConnections from 'connections'
  for(TFigureSet::iterator p=set.begin();
      p!=set.end();
      ++p)
  {
    TConnection *c = dynamic_cast<TConnection*>(*p);
    if (c) {
      for(TConnections::iterator q = connections.begin();
          q != connections.end();
          ++q)
      {
        if ( *q == c) {
          connections.erase(q);
          break;
        }
      }
    }
  }
  
  super::erase(set);
}

bool
TMapModel::restore(TInObjectStream &in)
{
  if (!super::restore(in))
    return false;
  if (!finished(in))
    return true;
  for(TStorage::const_iterator p = storage.begin();
      p != storage.end();
      ++p)
  {
    TConnection *c = dynamic_cast<TConnection*>(*p);
    if (c) {
      connections.push_back(c);
      for(TStorage::const_iterator p = storage.begin();
          p != storage.end();
          ++p)
      {
        TSymbol *nd = dynamic_cast<TSymbol*>(*p);
        if (nd) {
          if (nd->id == c->id0)
            c->nd0 = nd;
          if (nd->id == c->id1)
            c->nd1 = nd;
        }
      }
      
    }
  }
  return true;
}

void
TMapModel::store(TOutObjectStream &out) const
{
  int i = 0;
  for(TStorage::const_iterator p = storage.begin();
      p != storage.end();
      ++p)
  {
    TSymbol *nd = dynamic_cast<TSymbol*>(*p);
    if (nd) {
      nd->id = ++i;
    }
  }
  super::store(out);
}
