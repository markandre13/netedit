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

/**
 * NetEdit Server
 */

#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>  
#include <netdb.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/select.h>

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <set>

#include "../lib/common.hh"
#include "../lib/binary.hh"

#include "map.hh"

EXEC SQL INCLUDE SQLCA;

int verbose = 0;

void
throw_sql()
{
  printf("SQL Error: #%ld:%s\n",sqlca.sqlcode,sqlca.sqlerrm.sqlerrmc);
  exit(1);
}

using namespace std;
using namespace netedit;

static int createSocket();

void
sig_term(int)
{
  cout << "Bye" << endl;
  EXEC SQL DISCONNECT;
  exit(0);
}

/**
 * A list of all known clients.
 */
typedef vector<TClient*> TClientList;
TClientList clientlist;

/**
 * A list of all maps as read from the DBMS.
 */
int
main(int argc, char **argv)
{
  cout << "NetEdit Server" << endl;

  for(int i=1; i<argc; ++i) {
    if (strcmp(argv[i], "--verbose")==0) {
      ++verbose;
    } else {
      fprintf(stderr, "unknown argument '%s'\n", argv[i]);
      exit(EXIT_FAILURE);
    }
  }

  signal(SIGTERM, sig_term);
  signal(SIGINT,  sig_term);

  int sock = createSocket();

  EXEC SQL WHENEVER SQLERROR SQLPRINT;
  /* EXEC SQL WHENEVER SQLERROR DO throw_sql(); */
  EXEC SQL CONNECT TO netedit;
  if (sqlca.sqlcode<0) {
    exit(EXIT_FAILURE);
  }

  cout << "ready" << endl;
  fd_set rd0;
  FD_ZERO(&rd0);
  FD_SET(sock, &rd0);
  int max = sock+1;
  
  while(true) {
    fd_set rd = rd0;
    select(max, &rd, NULL, NULL, NULL);
    if (FD_ISSET(sock, &rd)) {
      sockaddr_in cname;
      int clen = sizeof(cname);
      int client = accept(sock, (sockaddr*)&cname, &clen);
      if (client>=0) {
        fcntl(client, F_SETFL, O_NONBLOCK);
        clientlist.push_back(new TClient(client));
        if (client>=max)
          max = client+1;
        FD_SET(client, &rd0);
      } else {
        perror("accept");
      }
    }
    for(TClientList::iterator p = clientlist.begin();
        p != clientlist.end(); ++p)
    {
      if (FD_ISSET((*p)->fd, &rd)) {
        if (!(*p)->handle()) {
          FD_CLR((*p)->fd, &rd0);
          delete *p;
          p = clientlist.erase(p);
          --p;
        }
      }
    }
  }

  EXEC SQL DISCONNECT;  
}

/**
 * Create TCP Server Socket
 */
int
createSocket()
{
  int sock;
  
  sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock==-1) {
    perror("while creating tcp socket");
    exit(EXIT_FAILURE);
  }

  int yes = 1;
  if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int))<0) {
    perror("failed to set SO_REUSEADDR");
  }
  
  if (setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &yes, sizeof(int))<0) {
    perror("failed to set TCP_NODELAY");
  }
  
  sockaddr_in name;
  name.sin_family = AF_INET;
  name.sin_addr.s_addr   = INADDR_ANY;
  name.sin_port   = htons(15001);
  if (bind(sock, (sockaddr*) &name, sizeof(sockaddr_in)) < 0) {
    perror("while binding to tcp socket");
    exit(EXIT_FAILURE);
  }
  
  if (listen(sock, 20)==-1) {
    perror("while starting to listen on tcp socket");
    exit(EXIT_FAILURE);
  }

  //accept(sock, );
  return sock;
}

bool
TClient::handle()
{
  while(true) {
    char cbuffer[4096];
    ssize_t n = read(fd, cbuffer, sizeof(cbuffer));
    if (n<0) {
      if (errno==EAGAIN) {
//        cout << "EAGAIN" << endl;
        return true;
      }
      if (errno==EINTR)
        continue;
      perror("error when reading from client");
      close(fd);
      return false;
    }
    if (n==0) {
      cout << "close connection" << endl;
      close(fd);
      return false;
    }
    buffer.append(cbuffer, n);
    execute();
  }
  return true;
}

void
TClient::execute()
{
  while(buffer.size()>=8) {
    unsigned p = 0;
    size_t n = getDWord(buffer, &p);
    if (buffer.size() < n)
      break;
    unsigned cmd = getDWord(buffer, &p);
    switch(cmd) {
      case CMD_LOGIN: { // client side login request
        login = getString(buffer, &p);
        string passwd = getString(buffer, &p);
        cout << "login by " << login << endl;
      } break;

      case CMD_GET_MAPLIST: // retrieve map list
        sendMapList();
        break;
      case CMD_OPEN_MAP: // retrieve map
        if (buffer.size()>=12)
          sendMap(getSDWord(buffer, &p));
        else
          cout << "error: CMD_OPEN_MAP command is too small" << endl;
        break;
      case CMD_CLOSE_MAP: // close map
        if (buffer.size()>=12)
          dropMap(getSDWord(buffer, &p));
        break;
        
      case CMD_ADD_SYMBOL:
        if (buffer.size()>=24) {
          int map = getDWord(buffer, &p);
          int symid = getSDWord(buffer, &p);
          if (symid>=0) {
            cout << "warning: attempt ignored to add symbol with non-temporary id" << endl;
          } else {
            int x  = getSDWord(buffer, &p);
            int y  = getSDWord(buffer, &p);
            int newsymid = TMap::addSymbol(fd, map, symid, x, y);
            if (symid<0 && newsymid>=0)
              symmapping.insert(map, symid, newsymid);
          }
        }
        break;
      case CMD_RENAME_SYMBOL:
        if (buffer.size()>=20) {
          int map     = getSDWord(buffer, &p);
          int old_sym = getSDWord(buffer, &p);
          int new_sym = getSDWord(buffer, &p);
// cout << "renamed in map "<<map<<" symbol " << old_sym << " into " << new_sym << endl;
          symmapping.erase(map, old_sym, new_sym);
        }
        break;
      case CMD_DELETE_SYMBOL:
        if (buffer.size()>=16) {
          int map     = getSDWord(buffer, &p);
          int sym     = getSDWord(buffer, &p);
          if (sym<0) {
            int s = symmapping.map(map, sym);
            symmapping.erase(map, sym, s);
          }
          TMap::deleteSymbol(fd, map, sym);
        }
        break;
      case CMD_TRANSLATE_SYMBOL: 
        if (buffer.size()>=24) {
          int map = getSDWord(buffer, &p);
          int sym = symmapping.map(map, getSDWord(buffer, &p));
          if (map>=0 && sym>=0) {
            int x  = getSDWord(buffer, &p);
            int y  = getSDWord(buffer, &p);
            TMap::translateSymbol(fd, map, sym, x, y);
          }
        }
        break;
        
      case CMD_ADD_CONNECTION:
        if (buffer.size()>=24) {
          int map = getSDWord(buffer, &p);
          int conn_id = getSDWord(buffer, &p);
          int sym0 = symmapping.map(map, getSDWord(buffer, &p));
          int sym1 = symmapping.map(map, getSDWord(buffer, &p));
cout << "CMD_ADD_CONNECTION: map="<<map<<", conn="<<conn_id<<", sym0="<<sym0<<", sym1="<<sym1<<endl;
          int newconnid = TMap::addConnection(fd, map, conn_id, sym0, sym1);
          if (conn_id<0 && newconnid>=0)
            connmapping.insert(map, conn_id, newconnid);
        }
        break;
      case CMD_RENAME_CONNECTION:
        if (buffer.size()>=20) {
          int map = getSDWord(buffer, &p);
          int old_conn = getSDWord(buffer, &p);
          int new_conn = getSDWord(buffer, &p);
          connmapping.erase(map, old_conn, new_conn);
        }
        break;
      case CMD_DELETE_CONNECTION:
        if (buffer.size()>=16) {
          int map = getSDWord(buffer, &p);
          int conn = getSDWord(buffer, &p);
          if (conn<0) {
            int c = connmapping.map(map, conn);
            connmapping.erase(map, conn, c);
          }
          TMap::deleteConnection(fd, map, conn);
        }
        break;
        
      case CMD_OPEN_NODE:
        if (buffer.size()>=12) {
          int node_id = getDWord(buffer, &p);
          openNode(node_id);
        }
        break;
      case CMD_SET_NODE:
        setNode(buffer, &p);
        break;
      case CMD_CLOSE_NODE:
        if (buffer.size()>=12) {
          int node_id = getDWord(buffer, &p);
          closeNode(node_id);
        }
        break;
      case CMD_LOCK_NODE:
        if (buffer.size()>=12) {
          int node_id = getDWord(buffer, &p);
          lockNode(node_id);
        }
        break;
      case CMD_UNLOCK_NODE:
        if (buffer.size()>=12) {
          int node_id = getDWord(buffer, &p);
          unlockNode(node_id);
        }
        break;
      default:
        cout << "received unknown command " << cmd << endl;
        break;
    }
    buffer.erase(0, n);
  }
}

void
TClient::sendMapList()
{
  EXEC SQL BEGIN DECLARE SECTION;
  char *name = 0;
  int id;
  EXEC SQL END DECLARE SECTION;
  string msg;
  addDWord(&msg, 0);
  addDWord(&msg, CMD_GET_MAPLIST);
  addDWord(&msg, 0);
  unsigned count = 0;

  EXEC SQL DECLARE cur_maplist CURSOR FOR
    SELECT map_id, name FROM map ORDER BY map_id;

  EXEC SQL OPEN cur_maplist;
  EXEC SQL WHENEVER NOT FOUND DO break;
  while(true) {
    EXEC SQL FETCH FROM cur_maplist INTO :id, :name;
    ++count;
//    printf("got %u '%s'\n", id, name);
    addSDWord(&msg, id);
    size_t n = strlen(name);
    addDWord(&msg, n);
    msg.append(name, n);
  }
  EXEC SQL WHENEVER NOT FOUND SQLPRINT;
  EXEC SQL CLOSE cur_maplist;

  if (verbose>0)
    cout << "sending map list with " << count << " entries" << endl;
  setDWord(&msg, 8, count);
  setDWord(&msg, 0, msg.size());
  write(fd, msg.c_str(), msg.size());
}

void
TClient::sendMap(int map_id)
{
  if (map_id==0) {
    cout << "warning: ignoring map_id==0, not sending it" << endl;
    return;
  }

  if (verbose)
    cout << "send map " << map_id << endl;

  TMap *map = TMap::load(map_id);
  map->send(fd);
  map->clients.insert(this);
}

void
TClient::dropMap(int map_id)
{
  TMap::dropMap(this, map_id);
}

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

class TNode
{
  public:
    TNode() {
      lock = 0;
    }
    ~TNode() {
      for(TInterfaces::iterator p = interfaces.begin();
          p != interfaces.end();
          ++p)
      {
        delete *p;
      }
    }

    int node_id;
    string sysObjectID;
    string sysName;
    string sysContact;
    string sysLocation;
    string sysDescr;
    int ipforwarding;
    string mgmtaddr;
    unsigned mgmtflags;
    unsigned topoflags;
    
    typedef vector<TInterface*> TInterfaces;
    TInterfaces interfaces;

    TClient *lock;            // client who owns the lock or NULL
    time_t locktime;          // lock creation time
    
    set<TClient*> clients;    // clients referencing this node
};

class TNodeCache
{
  private:
    typedef map<int, TNode*> TStorage;
    TStorage storage;
  public:
    TNode *get(TClient *client, int node_id);
    TNode *getCached(int node_id);
    void drop(TClient *client, int node_id);
    void closeClient(TClient *client);
};

TNodeCache nodecache;

TNode*
TNodeCache::getCached(int node_id)
{
  TStorage::iterator p = storage.find(node_id);
  return p!=storage.end() ? p->second : 0;
}

TNode*
TNodeCache::get(TClient *client, int node_id)
{
  TNode *node;
  TStorage::iterator p = storage.find(node_id);
  if (p!=storage.end()) {
    node = p->second;
  } else {
    EXEC SQL BEGIN DECLARE SECTION;
    varchar sysObjectID[252];
    varchar sysName[252];
    varchar sysContact[252];
    varchar sysLocation[252];
    varchar sysDescr[252];
    int ipforwarding;
    varchar mgmtaddr[40];
    unsigned mgmtflags;
    unsigned topoflags;
    int id = node_id;
    int flag;
    
    int interface_id, status, flags;
    varchar ipaddress[40];
    int ifIndex;
    varchar ifDescr[255];
    int ifType;
    varchar ifPhysAddress[36];
    EXEC SQL END DECLARE SECTION;

    sysObjectID.len = 0;
    sysName.len = 0;
    sysContact.len = 0;
    sysLocation.len = 0;
    sysDescr.len = 0;
    ipforwarding = 0;
    mgmtaddr.len = 0;
    mgmtflags = 0;
    topoflags = 0;

    EXEC SQL
      SELECT sysObjectID, 
             sysName, 
             sysContact, 
             sysLocation, 
             sysDescr,
             ipforwarding,
             mgmtaddr,
             mgmtflags,
             topoflags
      INTO   :sysObjectID :flag,
             :sysName :flag, 
             :sysContact :flag, 
             :sysLocation :flag, 
             :sysDescr :flag,
             :ipforwarding :flag,
             :mgmtaddr :flag,
             :mgmtflags :flag,
             :topoflags :flag
      FROM   node
      WHERE  node_id = :id;
      
    node = new TNode;
    node->node_id = node_id;
    node->sysObjectID.append(sysObjectID.arr, sysObjectID.len);
    node->sysName.append(sysName.arr, sysName.len);
    node->sysContact.append(sysContact.arr, sysContact.len);
    node->sysLocation.append(sysLocation.arr, sysLocation.len);
    node->sysDescr.append(sysDescr.arr, sysDescr.len);
    node->ipforwarding = ipforwarding;
    node->mgmtaddr.append(mgmtaddr.arr, mgmtaddr.len);
    node->mgmtflags = mgmtflags;
    node->topoflags = topoflags;
    storage[node_id] = node;
    
    EXEC SQL DECLARE cur_interfaces CURSOR FOR
      SELECT interface_id, status, flags, ipaddress, ifIndex, ifDescr, ifType,
             ifPhysAddress
      FROM interface ORDER BY ifIndex;
      
    EXEC SQL OPEN cur_interfaces;
    EXEC SQL WHENEVER NOT FOUND DO break;
    while(true) {
      interface_id = 0;
      status = 0;
      flags = 0;
      ipaddress.len = 0;
      ifIndex = 0;
      ifDescr.len = 0;
      ifType = 0;
      ifPhysAddress.len = 0;

      EXEC SQL FETCH FROM cur_interfaces
               INTO :interface_id :flag,
                    :status :flag,
                    :flags :flag,
                    :ipaddress :flag,
                    :ifIndex :flag,
                    :ifDescr :flag,
                    :ifType :flag,
                    :ifPhysAddress :flag;
      TInterface *in = new TInterface;
      in->interface_id = interface_id;
      in->status       = status;
      in->flags        = flags;
      in->ipaddress.append(ipaddress.arr, ipaddress.len);
      in->ifIndex      = ifIndex;
      in->ifDescr.append(ifDescr.arr, ifDescr.len);
      in->ifType       = ifType;
      in->ifPhysAddress.append(ifPhysAddress.arr, ifPhysAddress.len);
      node->interfaces.push_back(in);
    }
    EXEC SQL WHENEVER NOT FOUND SQLPRINT;
    EXEC SQL CLOSE cur_interfaces;
  }
  return node;
}

void
TNodeCache::drop(TClient *client, int node_id)
{
  TStorage::iterator p = storage.find(node_id);
  if (p==storage.end()) {
    cout << "warning: client tried to drop inactive node" << endl;
    return;
  }
  
  TNode* node = p->second;
  set<TClient*>::iterator c = node->clients.find(client);
  if (c==node->clients.end()) {
    cout << "warning: client tried to drop non-existent lease on node" << endl;
    return;
  }
  
  // drop reference to node
  node->clients.erase(c);
  if (node->lock == client)
    node->lock = 0;
  #warning "should inform other clients about dropped lock"
  
  // write node to DBMS when last client is detached
  if (node->clients.empty()) {
    EXEC SQL BEGIN DECLARE SECTION;
    int nid;
    const char *sysObjectID, *sysName, *sysContact, *sysLocation, *sysDescr;
    bool ipforwarding;
    const char *mgmtaddr;
    unsigned mgmtflags;
    unsigned topoflags;
    EXEC SQL END DECLARE SECTION;
    
    nid         = node_id;
    sysObjectID = node->sysObjectID.c_str();
    sysName     = node->sysName.c_str();
    sysContact  = node->sysContact.c_str();
    sysLocation = node->sysLocation.c_str();
    sysDescr    = node->sysDescr.c_str();
    ipforwarding= node->ipforwarding;
    mgmtaddr    = node->mgmtaddr.c_str();
    mgmtflags   = node->mgmtflags;
    topoflags   = node->topoflags;
    
    cout << "node_id = " << node_id << endl;
    cout << "sysObjectID = " << sysObjectID << endl;
    cout << "sysName     = " << sysName << endl;
    cout << "sysContact  = " << sysContact << endl;
    cout << "sysLocation = " << sysLocation << endl;
    cout << "sysDescr    = " << sysDescr << endl;
    cout << "mgmtaddr    = " << mgmtaddr << endl;

    EXEC SQL
      UPDATE node
      SET    sysObjectID = :sysObjectID,
             sysName     = :sysName,
             sysContact  = :sysContact,
             sysLocation = :sysLocation,
             sysDescr    = :sysDescr,
             mgmtaddr    = :mgmtaddr
      WHERE  node_id = :nid;

    EXEC SQL COMMIT;
    delete node;
    storage.erase(p);
  }
}

void
TNodeCache::closeClient(TClient *client)
{
  for(TStorage::iterator p = storage.begin();
      p != storage.end();
      )
  {
    set<TClient*>::iterator q = p->second->clients.find(client);
    if (q!=p->second->clients.end()) {
      #warning "suboptimal code"
      drop(client, p->second->node_id);
      p = storage.begin();
    } else {
      ++p;
    }
  }
}

void
TClient::openNode(int node_id)
{
  if (verbose>1)
    cout << "open node " << node_id << endl;
  TNode *node = nodecache.get(this, node_id);

  if (node->clients.find(this)!=node->clients.end()) {
    cout << "warning: client retrieves node more than once and may be broken" << endl;
  }
  node->clients.insert(this);

  string msg;
  addDWord(&msg, 0);
  addDWord(&msg, CMD_OPEN_NODE);
  addDWord(&msg, node_id);
  if (!node) {
    addDWord(&msg, NODE_IS_NOT);
  } else {
    if (node->lock == 0) {
      addDWord(&msg, NODE_UNLOCKED);
    } else {
      addDWord(&msg, node->lock==this ? NODE_LOCKED_LOCAL : NODE_LOCKED_REMOTE);
      addString(&msg, node->lock->login);
      addString(&msg, node->lock->hostname);
      addDWord(&msg,  node->locktime);
    }
  
    addString(&msg, node->sysObjectID);
    addString(&msg, node->sysName);
    addString(&msg, node->sysContact);
    addString(&msg, node->sysLocation);
    addString(&msg, node->sysDescr);
    addByte  (&msg, node->ipforwarding);
    addString(&msg, node->mgmtaddr);
    addDWord (&msg, node->mgmtflags);
    addDWord (&msg, node->topoflags);
    
    addDWord (&msg, node->interfaces.size());
    for(TNode::TInterfaces::iterator p = node->interfaces.begin();
        p != node->interfaces.end();
        ++p)
    {
      addDWord (&msg, (*p)->interface_id);
      addDWord (&msg, (*p)->status);
      addDWord (&msg, (*p)->flags);
      addString(&msg, (*p)->ipaddress);
      addDWord (&msg, (*p)->ifIndex);
      addString(&msg, (*p)->ifDescr);
      addDWord (&msg, (*p)->ifType);
      addString(&msg, (*p)->ifPhysAddress);
    }
  }
  setDWord(&msg, 0, msg.size());
  write(fd, msg.c_str(), msg.size());
}

void
TClient::setNode(const string &msg, unsigned *p)
{
  int node_id = getDWord(msg, p);
  if (verbose>1)
    cout << "set node " << node_id << endl;
  TNode *node = nodecache.get(this, node_id);
  if (!node) {
    cout << "error: can't set node because it wasn't found" << endl;
    return;
  }
  if (node->lock != this) {
    cout << "error: can't set node because client doesn't held lock" << endl;
    return;
  }
  node->sysObjectID = getString(msg, p);
  node->sysName     = getString(msg, p);
  node->sysContact  = getString(msg, p);
  node->sysLocation = getString(msg, p);
  node->sysDescr    = getString(msg, p);
  node->ipforwarding= getByte(msg, p);
  node->mgmtaddr    = getString(msg, p);
  node->mgmtflags   = getDWord(msg, p);
  node->topoflags   = getDWord(msg, p);
  
  string out;
  addDWord(&out, 0);
  addDWord(&out, CMD_UPDATE_NODE);
  addDWord(&out, node_id);
  
  addString(&out, node->sysObjectID);
  addString(&out, node->sysName);
  addString(&out, node->sysContact);
  addString(&out, node->sysLocation);
  addString(&out, node->sysDescr);
  addByte  (&out, node->ipforwarding);
  addString(&out, node->mgmtaddr);
  addDWord (&out, node->mgmtflags);
  addDWord (&out, node->topoflags);
  setDWord (&out, 0, msg.size());

  for(set<TClient*>::iterator p=node->clients.begin();
      p!=node->clients.end();
      ++p)
  {
    if (*p != this) {
      write((*p)->fd, out.c_str(), out.size());
    }
  }
}

void
TClient::closeNode(int node_id)
{
  unlockNode(node_id);
  if (verbose>1)
    cout << "drop node " << node_id << endl;
  nodecache.drop(this, node_id);
}

void
TClient::lockNode(int node_id)
{
  if (verbose>1)
    cout << "lock node " << node_id << endl;
  TNode *node = nodecache.getCached(node_id);
  if (node->lock) {
    if (verbose>1)
      cout << "warning: client tried to lock already locked node" << endl;
    return;
  }

cout << "lock node " << node_id << endl;

  node->lock = this;
  node->locktime = time(NULL);

  string out;
  addDWord(&out, 0);
  addDWord(&out, CMD_LOCK_NODE);
  addDWord(&out, node_id);
  addByte(&out, 0);
  addString(&out, node->lock->login);
  addString(&out, node->lock->hostname);
  addDWord(&out,  node->locktime);
  setDWord (&out, 0, out.size());

  for(set<TClient*>::iterator p=node->clients.begin();
      p!=node->clients.end();
      ++p)
  {
    setByte(&out, 12, node->lock==*p ? NODE_LOCKED_LOCAL : NODE_LOCKED_REMOTE);
    write((*p)->fd, out.c_str(), out.size());
  }
}

void
TClient::unlockNode(int node_id)
{
  if (verbose>1)
    cout << "unlock node " << node_id << endl;
  TNode *node = nodecache.getCached(node_id);
  if (node->lock!=this) {
//    cout << "error: client tried to drop non-existing or foreign lock" << endl;
    return;
  }
  node->lock = 0;
  string out;
  addDWord(&out, 0);
  addDWord(&out, CMD_UNLOCK_NODE);
  addDWord(&out, node_id);
  setDWord (&out, 0, out.size());

  for(set<TClient*>::iterator p=node->clients.begin();
      p!=node->clients.end();
      ++p)
  {
    write((*p)->fd, out.c_str(), out.size());
  }
}

TClient::~TClient()
{
  nodecache.closeClient(this);
  close(fd);
}
