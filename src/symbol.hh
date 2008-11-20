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

#ifndef __NETEDIT_NETDEVICE_HH
#define __NETEDIT_NETDEVICE_HH

#include <toad/figure.hh>

namespace netedit {

using namespace toad;

class TSymbol;

class TConnection:
  public TFigure 
{
  public:
    typedef TFigure super;
    TConnection() {
      id0 = id1 = 0;
      nd0 = nd1 = 0;
    }
    TConnection(int conn_id, TSymbol *nd0, TSymbol *nd1) {
      this->conn_id = conn_id;
      id0 = id1 = 0;
      this->nd0 = nd0;
      this->nd1 = nd1;
    }
    int conn_id;
    int id0, id1;
    TSymbol *nd0, *nd1;

    bool editEvent(TFigureEditEvent &ee);
    void paint(toad::TPenBase&, toad::TFigure::EPaintType);
    void getShape(toad::TRectangle*);
    void translate(int, int) {}
    double distance(int, int);
    SERIALIZABLE_INTERFACE(netedit::, TConnection);    
};

class TSymbol:
  public TFigure, public TRectangle
{
    typedef TFigure super;
  public:
    TSymbol();
    
    int id; // symbol id
    int objid; // object id (map, node, ...)
    string sysName, sysContact, sysLocation, comment, label;
    
    string type;
    unsigned submap;
    
    unsigned port;
    string protocol;

    bool editEvent(TFigureEditEvent &ee);

    unsigned mouseLDown(TFigureEditor *editor, int mx, int my, unsigned);
    unsigned mouseRDown(TFigureEditor *editor, int mx, int my, unsigned);

    void paint(TPenBase&, EPaintType);
    void paintSelection(TPenBase&, int);
    double distance(int, int); 
    void getShape(TRectangle*);
    void translate(int dx, int dy);

    TCloneable* clone() const { return new TSymbol(*this); }
    const char * getClassName() const { return "netedit::TSymbol"; }
    void store(TOutObjectStream &out) const;
    bool restore(TInObjectStream &in);
};

struct imgtxt_t {
  const char *name;
  const char *file;
  TBitmap *bmp;
};
extern imgtxt_t imgtxt[];

void loadimages();

} // namespaace netedit

#endif
