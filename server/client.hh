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

#ifndef __NETEDITD_CLIENT_HH

#include "idmapping.hh"
#include <string>

namespace netedit {

using namespace std;

/**
 * Each client is handled by an object of type TClient.
 */
class TClient
{
    string buffer;
    TIDMapping symmapping;
    TIDMapping connmapping;

  public:
    string login;    // login name of the user as in DBMS
    string hostname; // hostname or IP (+port) from which the user connected
    int fd;

    TClient(int fd) {
      this->fd = fd;
    }
    ~TClient();
    bool handle();
    void execute();
    
    void sendMapList();
    void sendMap(int map_id);
    void dropMap(int map_id);

    void openNode(int node_id);
    void setNode(const string &msg, unsigned *p);
    void closeNode(int node_id);
    void lockNode(int node_id);
    void unlockNode(int node_id);
};

} // namespace netedit

#endif
