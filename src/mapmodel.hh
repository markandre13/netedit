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
#ifndef __NETEDIT_NETMODEL_HH
#define __NETEDIT_NETMODEL_HH

#include <toad/figuremodel.hh>
#include "server.hh"

namespace netedit {

using namespace toad;

class TMapModel:
  public TFigureModel
{
    typedef TFigureModel super;
  public:
    bool itsme;
  
    PServer server;
    int id;
    TMapModel(TServer *server, int id);
    ~TMapModel();

    // connections must be also figures so we can select and delete them?
    typedef vector<TConnection*> TConnections;
    TConnections connections;

    int uniqueDeviceID(TSymbol *device) const;
    TSymbol* deviceByID(int id) const;
    
    int uniqueConnID() const;
    TConnection* connByID(int id) const;
    
    void connectDevice(int conn_id, TSymbol *nd0, TSymbol *nd1);

    void addSymbol(int id, int  x, int y);    
    void renameSymbol(int old_id, int new_id);
    void deleteSymbol(int id);
    void translateSymbol(int id, int dx, int dy);

    void addConnection(int id, int sym0, int sym1);
    void renameConnection(int old_id, int new_id);
    void deleteConnection(int id);

    void erase(TFigureSet&);
    SERIALIZABLE_INTERFACE(netedit::, TMapModel);    
};
typedef GSmartPointer<TMapModel> PNetModel;

} // namespace netedit

#endif
