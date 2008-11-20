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

#include "server.hh"
#include "mapmodel.hh"
#include "symbol.hh"
#include "nodeeditor.hh"
#include "../lib/common.hh"
#include "../lib/binary.hh"

#include <errno.h>
#include <sys/socket.h> 
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <iostream>

using namespace netedit;

typedef map<int, TNodeModel*> nodemap_t;
nodemap_t nodemap;

TNodeModel::TNodeModel()
{
  TCLOSURE1(lock.sigChanged,
           obj, this,
    obj->readwrite(obj->lock.get()==LOCKED_LOCAL);
  )
  refcount = 0;
  readwrite(lock.get()==LOCKED_LOCAL);
}

TNodeModel::~TNodeModel()
{
  assert(refcount==0);
  for(TInterfaces::iterator p = interfaces.begin();
      p != interfaces.end();
      ++p)
  {
    delete *p;
  }
}

/**
 * fetch node data from server message
 */
void
TNodeModel::fetch(const string &buffer, unsigned *p)
{
  sysObjectID = getString(buffer, p);
  sysName     = getString(buffer, p);
  sysContact  = getString(buffer, p);
  sysLocation = getString(buffer, p);
  sysDescr    = getString(buffer, p);
  ipforwarding= getByte(buffer, p);
  mgmtaddr    = getString(buffer, p);
  mgmtflags   = getDWord(buffer, p);
  topoflags   = getDWord(buffer, p);
  
  int ifCount = getDWord(buffer, p);
  for(int i=0; i<ifCount; ++i) {
    TInterface *in = new TInterface;
    in->interface_id = getDWord(buffer, p);
    in->status       = getDWord(buffer, p);
    in->flags        = getDWord(buffer, p);
    in->ipaddress    = getString(buffer, p);
    in->ifIndex      = getDWord(buffer, p);
    in->ifDescr      = getString(buffer, p);
    in->ifType       = getDWord(buffer, p);
    in->ifPhysAddress= getString(buffer, p);
    interfaces.push_back(in);
  }
}

void
TNodeModel::readwrite(bool rw)
{
//  cout << "TNodeModel " << this << " readwrite := " << rw << endl;
  sysObjectID.setEnabled(rw);
  sysName.setEnabled(rw);
  sysContact.setEnabled(rw);
  sysLocation.setEnabled(rw);
  sysDescr.setEnabled(rw);
  mgmtaddr.setEnabled(rw);
}

TServer::TServer(const string &hostname, unsigned port)
{
  sock = -1;

  sockaddr_in name;
  in_addr ia;
  if (inet_aton(hostname.c_str(), &ia)!=0) {
    name.sin_addr.s_addr = ia.s_addr;
  } else {
    struct hostent *hostinfo;
    hostinfo = gethostbyname(hostname.c_str());
    if (hostinfo==0) {
      cerr << "couldn't resolve hostname '" << hostname << "'" << endl;
      return;
    }
    name.sin_addr = *(struct in_addr *) hostinfo->h_addr;
  }
  name.sin_family = AF_INET;
  name.sin_port   = htons(port);
  
  sock = socket (AF_INET, SOCK_STREAM, 0);
  if (sock==-1) {
    perror("failed to create socket");
    return;
  }
  
  int yes = 1;
  if (setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &yes, sizeof(int))<0) {
    perror("failed to set TCP_NODELAY");
  }
  
  if (connect(sock, (sockaddr*) &name, sizeof(sockaddr_in)) < 0) {
    cerr << "couldn't connect to client" << endl;
    return;
  }

  sndGetMapList();
  
  setFD(sock);
}

void
TServer::canRead()
{
  while(true) {
    char cbuffer[4096];
    ssize_t n = read(sock, cbuffer, sizeof(cbuffer));
    if (n==0) {
      cout << "lost connection to server" << endl;
      sleep(1);
      break;
    }
    if (n<0) {
      if (errno==EINTR)
        continue;
      if (errno==EAGAIN)
        break;
      perror("error while reading from server");
      return;
    }
cout << "got " << n << " bytes from server" << endl;
    buffer.append(cbuffer, n);
  }
  
  while(buffer.size()>=8) {
    unsigned p = 0;
    size_t n = getDWord(buffer, &p);
    if (buffer.size() < n)
      break;
    if (n==0) {
      cout << "fatal error: received command of size 0" << endl;
      exit(0);
    }
    unsigned cmd = getDWord(buffer, &p);
    switch(cmd) {
      case CMD_GET_MAPLIST: { // received map list
        map.clear();
        maplist.clear();
        unsigned n = getDWord(buffer, &p);
//        cout << "got " << n << " map names" << endl;
        for(unsigned i=0; i<n; ++i) {
          int id = getSDWord(buffer, &p);
          string name = getString(buffer, &p);
//          cout << "got map " << id << ", '" << name << "'" << endl;
          maplist.push_back(MapListEntry(id, name));
        }
        reason = MAPLIST_AVAILABLE;
        sigChanged();
      } break;
      
      case CMD_OPEN_MAP: { // received map
        TMapModel *m = new TMapModel(0, getDWord(buffer, &p));
        unsigned n;
        n = getDWord(buffer, &p);
//        cout << "got " << n << " symbols" << endl;
        for(unsigned i=0; i<n; ++i) {
          TSymbol *nd = new TSymbol;
          nd->id = getSDWord(buffer, &p);
          nd->objid     = getSDWord(buffer, &p);
          nd->x         = getSDWord(buffer, &p);
          nd->y         = getSDWord(buffer, &p);
          nd->sysName   = getString(buffer, &p);
          nd->type      = getString(buffer, &p);
          m->push_back(nd);
          // maplist.push_back(MapListEntry(id, name));
        }
        n = getDWord(buffer, &p);
//        cout << "got " << n << " connections" << endl;
        for(unsigned i=0; i<n; ++i) {
          int conn_id = getSDWord(buffer, &p);
          int id0     = getSDWord(buffer, &p);
          int id1     = getSDWord(buffer, &p);
//          cout << "  connect " << id0 << " <-> " << id1 << endl;
          m->connectDevice(conn_id, m->deviceByID(id0), m->deviceByID(id1));
        }
        m->server = this;
        netmodel = m;
        reason = NETMODEL_CHANGED;
        sigChanged();
      } break;
      
      case CMD_ADD_SYMBOL:
        if (buffer.size()>=24) {
          int map    = getSDWord(buffer, &p);
          int symbol = getSDWord(buffer, &p);
          int x      = getSDWord(buffer, &p);
          int y      = getSDWord(buffer, &p);
          if (map!=netmodel->id) {
            cout << "received add symbol for foreign map" << endl;
          } else {
            netmodel->addSymbol(symbol, x, y);
          }
        }
        break;
        
      case CMD_RENAME_SYMBOL:
        if (buffer.size()>=20) {
          int map   = getSDWord(buffer, &p);
          int oldid = getSDWord(buffer, &p);
          int newid = getSDWord(buffer, &p);
          if (map!=netmodel->id) {
            cout << "received rename symbol for foreign map" << endl;
          } else {
            netmodel->renameSymbol(oldid, newid);
            sndRenameSymbol(map,oldid, newid);
          }
        }
        break;
        
      case CMD_DELETE_SYMBOL:
        if (buffer.size()>=16) {
          int map    = getSDWord(buffer, &p);
          int symbol = getSDWord(buffer, &p);
          if (map!=netmodel->id) {
            cout << "received delete symbol for foreign map" << endl;
          } else {
            netmodel->deleteSymbol(symbol);
          }
        }
        break;
      
      case CMD_TRANSLATE_SYMBOL: {
        if (buffer.size()>=24) {
          int map    = getSDWord(buffer, &p);
          int symbol = getSDWord(buffer, &p);
          int x      = getSDWord(buffer, &p);
          int y      = getSDWord(buffer, &p);
          if (map!=netmodel->id) {
            cout << "received translate symbol for foreign map" << endl;
          } else {
            netmodel->translateSymbol(symbol, x, y);
          }
        }
      } break;
      
      case CMD_ADD_CONNECTION:
        if (buffer.size()>=24) {
          int map    = getSDWord(buffer, &p);
          int conn   = getSDWord(buffer, &p);
          int sym0   = getSDWord(buffer, &p);
          int sym1   = getSDWord(buffer, &p);
          if (map!=netmodel->id) {
            cout << "received add connection for foreign map" << endl;
          } else {
            netmodel->addConnection(conn, sym0, sym1);
          }
        }
        break;

      case CMD_RENAME_CONNECTION:
        if (buffer.size()>=20) {
          int map   = getSDWord(buffer, &p);
          int oldid = getSDWord(buffer, &p);
          int newid = getSDWord(buffer, &p);
          if (map!=netmodel->id) {
            cout << "received rename symbol for foreign map" << endl;
          } else {
            netmodel->renameConnection(oldid, newid);
            sndRenameConnection(map,oldid, newid);
          }
        }
        break;
        
      case CMD_DELETE_CONNECTION:
        if (buffer.size()>=16) {
          int map  = getSDWord(buffer, &p);
          int conn = getSDWord(buffer, &p);
          if (map!=netmodel->id) {
            cout << "received delete connection for foreign map" << endl;
          } else {
            netmodel->deleteConnection(conn);
          }
        }
        break;
      
      case CMD_OPEN_NODE: {
        int node   = getSDWord(buffer, &p);
        if (nodemap.find(node)!=nodemap.end()) {
          cout << "error: node " << node << " is already open" << endl;
          break;
        }
        unsigned result = getDWord(buffer, &p);
        if (result==NODE_IS_NOT) {
          cout << "error: can't open node " << node << endl;
          break;
        }
        if (result==NODE_LOCKED_LOCAL) {
          cout << "error: node " << node << " is reported as locally locked" << endl;
          break;
        }
        TNodeModel *nm = new TNodeModel;
        if (result==NODE_LOCKED_REMOTE) {
          nm->lock.login    = getString(buffer, &p);
          nm->lock.hostname = getString(buffer, &p);
          nm->lock.since    = getDWord(buffer,  &p);
          nm->lock.set(LOCKED_REMOTE);
        } else {
          nm->lock.set(UNLOCKED);
        }
        nm->node_id = node;
        nm->fetch(buffer, &p);
        nodemap[nm->node_id] = nm;
        ++nm->refcount;
        TNodeEditor *ne = new TNodeEditor(0, "NetEdit - Node Editor", nm, this);
        ne->createWindow();
      } break;

      case CMD_UPDATE_NODE: {
        int node_id   = getSDWord(buffer, &p);
        nodemap_t::iterator q = nodemap.find(node_id);
        if (q==nodemap.end()) {
          cout << "error: update for non-local node" << endl;
          break;
        }
        if (q->second->lock.get() == LOCKED_LOCAL) {
          cout << "error: server tried to update locally owned node" << endl;
          break;
        }
        q->second->fetch(buffer, &p);
      } break;
      
      case CMD_LOCK_NODE: {
cout << "received lock node" << endl;
        int node_id   = getSDWord(buffer, &p);
        nodemap_t::iterator q = nodemap.find(node_id);
        if (q==nodemap.end()) {
          cout << "error: lock for non-local node" << endl;
          break;
        }
        int state = getByte(buffer, &p);
        TNodeModel *nm = q->second;
        nm->lock.login    = getString(buffer, &p);
        nm->lock.hostname = getString(buffer, &p);
        nm->lock.since    = getDWord(buffer, &p);
        nm->lock.set(state == NODE_LOCKED_LOCAL ? LOCKED_LOCAL : LOCKED_REMOTE);
      } break;

      case CMD_UNLOCK_NODE: {
cout << "received unlock node" << endl;
        int node_id   = getSDWord(buffer, &p);
        nodemap_t::iterator q = nodemap.find(node_id);
        if (q==nodemap.end()) {
          cout << "error: unlock for non-local node" << endl;
          break;
        }
        TNodeModel *nm = q->second;
        nm->lock.set(UNLOCKED);
      } break;
      
      default:
        cout << "received unknown command " << cmd << endl;

    }
    buffer.erase(0, n);
  }
}

void
TServer::sndLogin(const string &login, const string &passwd)
{
  string msg;

  addDWord(&msg, 0);
  addDWord(&msg, CMD_LOGIN);
  addString(&msg, login);
  addString(&msg, passwd);

  setDWord(&msg, 0, msg.size());
  write(sock, msg.c_str(), msg.size());
}

void
TServer::sndGetMapList()
{
  string cmd;
  addDWord(&cmd, 8);
  addDWord(&cmd, CMD_GET_MAPLIST); // get map list
  write(sock, cmd.c_str(), cmd.size());
}

void
TServer::sndGetMapModelByRow(unsigned maplistrow)
{
  if (maplistrow >= maplist.size()) {
    cout << "warning: TServer::getMapModelByRow: index out of range" << endl;
    return;
  }
  sndGetMapModel(maplist[maplistrow].map_id);
}

unsigned
TServer::getMapIDByRow(unsigned maplistrow)
{
  if (maplistrow >= maplist.size()) {
    cout << "warning: TServer::getMapIDByRow: index out of range" << endl;
    return 0;
  }
  return maplist[maplistrow].map_id;
}

void
TServer::sndGetMapModel(unsigned map_id)
{
#warning "should cache map models"
  string cmd;
  addDWord(&cmd, 12);
  addDWord(&cmd, CMD_OPEN_MAP);
  addSDWord(&cmd, map_id);
  write(sock, cmd.c_str(), cmd.size());
}

void
TServer::sndDropMapModel(int mapid)
{
  string cmd;
  addDWord(&cmd, 12);
  addDWord(&cmd, CMD_CLOSE_MAP);
  addSDWord(&cmd, mapid);
  write(sock, cmd.c_str(), cmd.size());
}

void
TServer::sndAddSymbol(int map, int sym, int x, int y)
{
  string cmd;
  addDWord(&cmd, 24);
  addDWord(&cmd, CMD_ADD_SYMBOL);
  addSDWord(&cmd, map);
  addSDWord(&cmd, sym);
  addSDWord(&cmd, x);
  addSDWord(&cmd, y);
cout << "sndAddSymbol("<<map<<", "<<sym<<", "<<x<<", "<<y<<")\n";
  write(sock, cmd.c_str(), cmd.size());
}

void
TServer::sndRenameSymbol(int map, int old_id, int new_id)
{
  string cmd;
  addDWord(&cmd, 20);
  addDWord(&cmd, CMD_RENAME_SYMBOL);
  addSDWord(&cmd, map);
  addSDWord(&cmd, old_id);
  addSDWord(&cmd, new_id);
  //cout << "sndRenameSymbol("<<map<<", "<<old_id<<", "<<new_id<<")\n";
  write(sock, cmd.c_str(), cmd.size());
}

void
TServer::sndDeleteSymbol(int map, int sym)
{
  string cmd;
  addDWord(&cmd, 16);
  addDWord(&cmd, CMD_DELETE_SYMBOL);
  addSDWord(&cmd, map);
  addSDWord(&cmd, sym);
  write(sock, cmd.c_str(), cmd.size());
}

void
TServer::sndTranslateSymbol(int map_id, int symbol_id, int x, int y)
{
  string cmd;
  addDWord(&cmd, 24);
  addDWord(&cmd, CMD_TRANSLATE_SYMBOL);
  addSDWord(&cmd, map_id);
  addSDWord(&cmd, symbol_id);
  addSDWord(&cmd, x);
  addSDWord(&cmd, y);
  write(sock, cmd.c_str(), cmd.size());
}

void
TServer::sndConnectSymbol(int map_id, int conn_id, int symbol_id0, int symbol_id1)
{
  string msg;
  addDWord(&msg, 0);
  addDWord(&msg, CMD_ADD_CONNECTION);
  addSDWord(&msg, map_id);
  addSDWord(&msg, conn_id);
  addSDWord(&msg, symbol_id0);
  addSDWord(&msg, symbol_id1);
  
  setDWord(&msg, 0, msg.size());
  write(sock, msg.c_str(), msg.size());
}

void
TServer::sndRenameConnection(int map, int old_id, int new_id)
{
  string cmd;
  addDWord(&cmd, 20);
  addDWord(&cmd, CMD_RENAME_CONNECTION);
  addSDWord(&cmd, map);
  addSDWord(&cmd, old_id);
  addSDWord(&cmd, new_id);
  //cout << "sndRenameConnection("<<map<<", "<<old_id<<", "<<new_id<<")\n";
  write(sock, cmd.c_str(), cmd.size());
}

void
TServer::sndDeleteConnection(int map, int sym)
{
  string cmd;
  addDWord(&cmd, 16);
  addDWord(&cmd, CMD_DELETE_CONNECTION);
  addSDWord(&cmd, map);
  addSDWord(&cmd, sym);
  write(sock, cmd.c_str(), cmd.size());
}

void
TServer::sndOpenNode(int node_id)
{
cout << "send open node" << endl;
  nodemap_t::iterator p = nodemap.find(node_id);
  if (p!=nodemap.end()) {
    ++p->second->refcount;
    TNodeEditor *ne = new TNodeEditor(0, "NetEdit - Node Editor", p->second, this);
    ne->server = this;
    ne->createWindow();
    return;
  }
  string cmd;
  addDWord(&cmd, 12);
  addDWord(&cmd, CMD_OPEN_NODE);
  addDWord(&cmd, node_id);
  write(sock, cmd.c_str(), cmd.size());
}

void
TServer::sndSetNode(TNodeEditor *ne)
{
  string msg;
  addDWord(&msg, 0);
  addDWord(&msg, CMD_SET_NODE);
  TNodeModel *nm = ne->model;
  addSDWord(&msg, nm->node_id);
  addString(&msg, nm->sysObjectID);
  addString(&msg, nm->sysName);
  addString(&msg, nm->sysContact);
  addString(&msg, nm->sysLocation);
  addString(&msg, nm->sysDescr);
  addByte(&msg, nm->ipforwarding);
  addString(&msg, nm->mgmtaddr);
  addDWord(&msg, nm->mgmtflags);
  addDWord(&msg, nm->topoflags);

  setDWord(&msg, 0, msg.size());
  write(sock, msg.c_str(), msg.size());
}


void
TServer::sndCloseNode(int node_id)
{
  nodemap_t::iterator p = nodemap.find(node_id);
  if (p==nodemap.end()) {
    cout << "error: can't close unknown node" << endl;
    return;
  }
  --p->second->refcount;
  if (p->second->refcount>0)
    return;
  delete p->second;
  nodemap.erase(p);
  string cmd;
  addDWord(&cmd, 0);
  addDWord(&cmd, CMD_CLOSE_NODE);
  addDWord(&cmd, node_id);
  setDWord(&cmd, 0, cmd.size());
  write(sock, cmd.c_str(), cmd.size());
}

void
TServer::sndLockNode(int node_id)
{
  string cmd;
  addDWord(&cmd, 0);
  addDWord(&cmd, CMD_LOCK_NODE);
  addDWord(&cmd, node_id);
  setDWord(&cmd, 0, cmd.size());
  write(sock, cmd.c_str(), cmd.size());
}

void
TServer::sndUnlockNode(int node_id)
{
  string cmd;
  addDWord(&cmd, 0);
  addDWord(&cmd, CMD_UNLOCK_NODE);
  addDWord(&cmd, node_id);
  setDWord(&cmd, 0, cmd.size());
  write(sock, cmd.c_str(), cmd.size());
}
