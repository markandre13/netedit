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

// snmpwalk -v 2c -c public 10.0.0.2  .

#include "../snmpd/asn1.hh"
#include "snmp.hh"
#include <strstream>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <string>
#include <iostream>

#include <toad/toad.hh>
#include <toad/dialog.hh>
#include <toad/textfield.hh>
#include <toad/textarea.hh>
#include <toad/table.hh>
#include <toad/pushbutton.hh>

using namespace toad;
using namespace netedit;

TSNMPQuery::TSNMPQuery()
{
  init();
}

void
TSNMPQuery::init()
{
  tree = 0;
  sock = -1;
  walk = true;
  running = false;
  maxretries = 3;
  retrydelay = 5;
  requestid = 0;
}

bool
TSNMPQuery::open()
{
  if (sock>=0) {
    return true;
  }

  sock = socket(AF_INET, SOCK_DGRAM, 0);
  if (sock<0) {
    perror("socket");
    sock = -1;
    return false;
  }
  
  int yes = 1;
  setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
  
  name.sin_family = AF_INET;
  name.sin_addr.s_addr = htonl(INADDR_ANY);
  name.sin_port        = 0;

  if (bind(sock, (sockaddr*) &name, sizeof(sockaddr_in)) < 0) {
    perror("bind");
    ::close(sock);
    sock = -1;
    return false;
  }
  
  struct hostent *hostinfo;
  hostinfo = gethostbyname(hostname.c_str());
  if (hostinfo==0) {
    cerr << "couldn't resolve hostname '" << hostname << "'\n";
    ::close(sock);
    sock = -1;
    return false;
  }
  name.sin_addr = *(struct in_addr *) hostinfo->h_addr;
  name.sin_port = htons(161);
  setFD(sock);
  return true;
}

void
TSNMPQuery::close()
{
  if (sock>=0) {
    ::close(sock);
    sock = -1;
    setFD(sock);
  }
}


/**
 * \todo
 *    \li
 *      UDP is an unrealiable transport and we also want to handle it
 *      assynchronously
 */
void
TSNMPQuery::snmpQuery(const char *oid, bool next)
{
  this->oid = oid;
  this->next = next;
  sendQuery();
}

void
TSNMPQuery::snmpWalk()
{
  currentOID = "";
  walk = true;
  retries = 0;
  snmpQuery("1.3.6.1.2.1.1.1.0", false);
//  snmpQuery(oid.c_str(), false);
}

void
TSNMPQuery::queryHost(const string &oid)
{
  currentOID = "";
  walk = false;
  retries = 0;
  snmpQuery(oid.c_str(), false);
}

void
TSNMPQuery::queryNextHost(const string &oid)
{
  currentOID = "";
  walk = false;
  retries = 0;
  snmpQuery(oid.c_str(), true);
}

//unsigned counter=0;

void
TSNMPQuery::sendQuery()
{
//cerr << oid << " " << counter << " get " << endl;
//counter++;
  running = true;
  // the request id will be used to identify the answer
  ++requestid;
  
  // build the SNMP command
  TASN1Encoder out;
  out.downSequence();
    out.encodeInteger(0); // version
    out.encodeOctetString(community);
    out.downContext(next?1:0);
      out.encodeInteger(requestid);
      out.encodeInteger(0); // error-status
      out.encodeInteger(0); // error-index
      out.downSequence();   // variables-bindings
        out.downSequence();
          out.encodeOID(oid);
          out.encodeNull();
        out.up();
      out.up();
    out.up();
  out.up();

  if (sendto(sock, 
             out.data.c_str(), out.data.size(),
             0, 
             (sockaddr*)&name, sizeof(name)) != (int)out.data.size())
  {
    perror("sendto");
  }

  startTimer(retrydelay, 0, true);
}

void
TSNMPQuery::tick()
{
//  cerr << __PRETTY_FUNCTION__ << endl;
  retries++;
  if (retries<maxretries) {
    --requestid;
    sendQuery();
    result->insert(result->getValue().size(), "*** retry ***\n"/*, false*/);
  } else {
    stopTimer();
    result->insert(result->getValue().size(), "*** timed out ***\n"/*, false*/);
  }
}

void
TSNMPQuery::stop()
{
  running = false;
  stopTimer();
}

void
TSNMPQuery::canRead()
{
//  cerr << "can read ..." << endl;
  stopTimer();
  if (!running)
    return;

  // fetch data from socket
  sockaddr_in remote;
  socklen_t len = sizeof(remote);
  char buffer[0xFFFF];
  int n = recvfrom(sock, buffer, 0xFFFF, MSG_TRUNC, (sockaddr*)&remote, &len);
  //  printf("got %i bytes from %s\n", n, inet_ntoa(remote.sin_addr) );

  // unmarshall ASN.1
  TASN1Decoder in(buffer, n);
//  in.print();
  
  int version;
  OctetString community;
  unsigned cmd;
  int requestID;
  int errorStatus;
  int errorIndex;
  
  // Message formats
  // RFC1157: v1   Message
  // RFC1901: v2c
  // RFC2572: v3   SNMPv3Message
  if (in.decode() && in.tag==TASN1::SEQUENCE &&
      in.down() && in.decodeInteger(&version))
  {
//    cout << "version: " << version << endl;
    
    enum {
      PDU_GET_REQUEST = 0,
      PDU_GET_NEXT_REQUEST,
      PDU_RESPONSE,
      PDU_SET_REQUEST,
      PDU_TRAP_PDU,           // SNMPv1 Trap PDU, obsoleted by SNMPv2
      PDU_GET_BULK_REQUEST,   // since SNMPv2
      PDU_INFORM_REQUEST,     // since SNMPv2
      PDU_SNMPV2_TRAP_PDU,    // since SNMPv2
      PDU_REPORT              // since SNMPv2
    };
    
    switch(version) {
      case 0: // SNMPv1 Message (RFC 1157)
      case 1: // SNMPv2c Message (RFC 1901)
        if (in.decodeOctetString(&community) &&
            in.decodeContext(&cmd) &&
            in.down())
        {
          if (cmd!=PDU_RESPONSE) {
            cout << "expected Response-PDU" << endl;
            break;
          }
          if (in.decodeInteger(&requestID) &&
              in.decodeInteger(&errorStatus) &&
              in.decodeInteger(&errorIndex) &&
              in.downSequence() )
          {
            while(in.decode()) {
              SmiSubid *oid = 0;
              size_t oidlen;
              if (in.tag == TASN1::SEQUENCE && in.down() &&
                  in.decodeOID(&oid, &oidlen) &&
                  in.decode() )
              {
                SmiNode *node = smiGetNodeByOID(oidlen, oid);
                bool isnew = tree->insert(oid, oidlen, node, string((char*)in.raw, in.len));
//isnew = true;
                if (walk) {
                  if (isnew) {
                    string s;
                    for(size_t i=0; i<oidlen; ++i) {
                      char buffer[64];
                      snprintf(buffer, sizeof(buffer), i ? ".%u" : "%u", oid[i]);
                      s += buffer;
                    }
//                    cout << s << ": " << errorStatus << "," << errorIndex << endl;
                    snmpQuery(s.c_str(), true);
                  } else {
                    cout << "walk finished" << endl;
                    result->insert(result->getValue().size(), "*** walk finished ***\n"/*, false*/);
                  }
                }
//                cout << "got OID" << endl;
                in.up();
              }
              if (oid)
                delete[] oid;
            }
//            cout << "ok: cmd = " << cmd << endl;
          }
        }
        break;
      case 3: // SNMPv3 Message (RFC 2572)
        break;
    }
  }
}

TSNMPTree::TSNMPTree()
{
  root = 0;
}

TSNMPTree::~TSNMPTree()
{
}

bool
TSNMPTree::insert(SmiSubid *oid, size_t oidlen, SmiNode *sminode, const string &raw)
{
  node_t *node = _find(oid, oidlen, sminode, true);
//    oid.substr(0, oid.size()-tail.size())
//  );
  if (node) {
    node->sminode = sminode;
    node->raw     = raw;
  }
  return node!=0;
}

#define DBM(CMD)
TSNMPTree::node_t*
TSNMPTree::_find(SmiSubid *oid, size_t oidlen, SmiNode *sminode, bool justnew)
{
/*
cout << "full: ";
for(size_t i=0; i<oidlen; ++i) {
  if (i!=0)
    cout << ".";
  cout << oid[i];
}
cout << endl;
*/
  bool isnew;
  node_t *pptr=0, *ptr = root, *node;

  for(unsigned i=0; i<oidlen; ++i) {
    SmiSubid id = oid[i];
    isnew = false;

    // insert at empty head, pptr points to parent
    if (!ptr) {
      node = new node_t(id);
      if (!pptr) {
        root = node;
      } else {
        pptr->down = node;
      }
      ptr = node;
      isnew = true;
    } else
    
    // insert at head, pptr points to parent
    if (id < ptr->id) {
      node = new node_t(id);
      node->next = ptr;
      pptr->down = node;
      ptr = node;
      isnew = true;
      break;
    } else {
      
      while(true) {
        // check for match
        if (ptr->id==id)
          break;
    
        // insert after current node
        if (!ptr->next || (ptr->id < id && id < ptr->next->id)) {
          node = new node_t(id);
          node->next = ptr->next;
          ptr->next = node;
          ptr = node;
          isnew = true;
          break;
        }
        ptr = ptr->next;
      }
    }

    // begin: compress row index
    if (i==sminode->oidlen) {
      size_t taillen = oidlen - i;
      if (taillen>=maxtail) {
        cerr << "maxtail execed by tail of " << taillen << " bytes" << endl;
        taillen = maxtail - 1;
      }
      if (!isnew) {
        size_t j;
        for(j=i; j<oidlen; ++j) {
          while(ptr->next && ptr->tail[j-i] < oid[j])
            ptr=ptr->next;
        }
        if (justnew) {
          for(j=i; j<oidlen; ++j) {
            if (ptr->tail[j-i] != oid[j])
              break;
          }
          if (j==oidlen)
            return 0;
        }
        node = new node_t(id);
        node->next = ptr->next;
        ptr->next = node;
        ptr = node;
      }
      ptr->sminode = smiGetNodeByOID(i, oid);
      ptr->taillen = taillen;
//cout << ptr->sminode->name;
      for(size_t j=i, k=0; k<ptr->taillen; ++j ,++k) {
//cout << "." << oid[j];
        ptr->tail[k] = oid[j];
      }
//cout << endl;
      return ptr;
    }
    // end compress row index

    if (!ptr->sminode && i+1<oidlen) {
      ptr->sminode = smiGetNodeByOID(i+1, oid);
    }
    pptr = ptr;
    ptr = ptr->down;
    
    // begin: compress singular value .0
    if (i+1==sminode->oidlen &&
        oidlen == sminode->oidlen+1 &&
        oid[i+1] == 0)
    {
      pptr->taillen = 1;
      pptr->tail[0] = 0;
      return pptr;
    }
    // end: compress singular value .0
  }
  if (justnew && !isnew)
    pptr = 0;
  return pptr;
}

void
TSNMPTree::print()
{
  _print(root, 0);
}

void
TSNMPTree::_print(node_t *node, unsigned depth)
{
  while(node) {

    for(unsigned i=0; i<depth; ++i) {
      printf("  ");
    }
    printf("%d\n", node->id);

    if (node->down)
      _print(node->down, depth+1);
    
    node = node->next;
  }
}

bool
netedit::initialize()
{
  smiInit(NULL);
  smiSetErrorLevel(9);

  string mibdir = "/usr/share/snmp/mibs";
  DIR *d = opendir(mibdir.c_str());
  dirent *de;
  mibdir+="/";
  while( (de=readdir(d)) ) {
    if (de->d_name[0]=='.')
      continue;
    string name = mibdir + de->d_name;
    if (!smiLoadModule(name.c_str())) {
      cout << "warning: failed to load MIB file '" << name << "'\n";
    }
  }
  
  return true;
}
