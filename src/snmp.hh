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

#ifndef __FRIENDLY_SNMP_HH
#define __FRIENDLY_SNMP_HH

#include <arpa/inet.h>
#include <netinet/in.h>
#include <string>

#include <smi.h>

#include <toad/ioobserver.hh>
#include <toad/simpletimer.hh>
#include <toad/textmodel.hh>
#include "oidnode.hh"

namespace netedit {

bool initialize();

/**
 * This class holds the result of an SNMP query.
 */
class TSNMPTree
{
    static const int maxtail = 50;
  public:
    struct node_t {
      node_t() {
        id = 0;
        sminode = 0;
        next = down = 0;
        taillen = 0;
      }
      node_t(unsigned id) {
        this->id = id;
        sminode = 0;
        next = down = 0;
        taillen = 0;
      }
      unsigned id;
      SmiNode *sminode;
      
      SmiSubid tail[maxtail];
      unsigned taillen;
      
      std::string raw;
      node_t *next, *down;
    };
    node_t *root;

    TSNMPTree();
    ~TSNMPTree();
    virtual bool insert(SmiSubid *oid, size_t oidlen, SmiNode *sminode, const std::string &raw);
    void print();

  protected:
    void _print(node_t *node, unsigned depth);
    node_t* _find(SmiSubid *oid, size_t oidlen, SmiNode *sminode, bool justnew);
};

class TSNMPQuery:
  public toad::TIOObserver, public toad::TSimpleTimer
{
  public:
    TSNMPQuery();
    
    bool open();
    void close();
    
    void snmpWalk();
    void stop();
    
    void queryHost(const std::string &oid);
    void queryNextHost(const std::string &oid);
    
  protected:
    void init();
  public:
    TSNMPTree *tree;
    toad::TTextModel *result;
    std::string hostname, community;
    bool walk;
    
    std::string oid;
    int requestid;
    bool next;

    bool running;
    unsigned retries;
    unsigned maxretries;
    unsigned retrydelay;
    
  protected:
    sockaddr_in name;
    std::string currentOID;
    
    void snmpQuery(const char *oid, bool next);
    void sendQuery();

    void canRead();
    
/*
    void canWrite();
    void gotException();
*/
    int sock;
    
    void tick();
};


} // namespace netedit

#endif
