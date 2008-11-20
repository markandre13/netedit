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

#ifndef __NETEDITD_IDMAPPING_HH

#include <map>
#include <iostream>

namespace netedit {

using namespace std;

/**
 * This class manages a clients temporary object ids.
 */
class TIDMapping
{
    typedef map<int,int> data_t;
    data_t data;
  public:
    /**
     * Add a new mapping. 
     * \param map
     *    obsolete?
     * \param old_id
     *    the old id (<0) as known to the client
     * \param new_id
     *    the new id (>=0) as send to the client
     */
    void insert(int map, int old_id, int new_id) {
      if (old_id>=0) {
        cout << "warning: attempt to map non-temporary id" << endl;
        return;
      }
      if (data.find(old_id)!=data.end()) {
        cout << "warning: attempt to override id " << old_id << endl;
        return;
      }
      data[old_id] = new_id;
    }
    
    /**
     * Delete mapping.
     */
    void erase(int map, int old_id, int new_id) {
      data_t::iterator p;
      p = data.find(old_id);
      if (p==data.end()) {
        cout << "warning: attempt to erase non-existent id " << old_id << endl;
        return;
      }
      if (p->second != new_id) {
        cout << "warning: attempt to erase wrong mapping for id " << old_id << endl;
        return;
      }
      data.erase(p);
    }
    
    /**
     * Lookup mapping
     */
    int map(int map, int old_id) {
      if (old_id>=0)
        return old_id;
      data_t::iterator p = data.find(old_id);
      if (p==data.end()) {
        cout << "warning: attempt to locate non-existent id " << old_id << endl;
        return old_id;
      }
      return p->second;
    }
};

} // namespace netedit

#endif
