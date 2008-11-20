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

#ifndef __NETEDIT_BROWSER_HH
#define __NETEDIT_BROWSER_HH

#include <toad/figureeditor.hh>
#include <toad/dragndrop.hh>

#include <toad/undomanager.hh>
#include "symbol.hh"
#include "mapmodel.hh"

namespace netedit {

using namespace toad;

class TBrowser:
  public TFigureEditor
{
  public:
    TBrowser(TWindow *parent, const string &title);
    
    void arrangeFigures();
};

class TNetEditor:
  public TFigureEditor
{
    typedef TFigureEditor super;
    TSymbol *nd0;
    PNetModel model;
    
  public:
    TNetEditor(TWindow *parent, const string &title);
    void setModel(TMapModel *model) {
      this->model = model;
      super::setModel(model);
      // we're running a client-server scenario with multiple clients
      // and undo/redo handling can't cope with that yet...
      TUndoManager::unregisterModel(model);
    }
    
    static const unsigned OP_CONNECT = 255;
    void mouseEvent(TMouseEvent &me);
    void invalidateFigure(TFigure*);
};

class TEditorWindow:
  public TWindow
{
    TNetEditor *ne;
    int nextmap;

  public:
    TEditorWindow(TWindow *parent, const string &title);
    void gotoMapByRow(unsigned);
    void gotoMapByID(unsigned);

  protected:
    void serverChanged();
};


class TDnDObjectType:
  public TDnDObject
{
    TDnDObjectType() {}
    TDnDObjectType(const TDnDObjectType &){}
  public:
    string objecttype;
    
    TDnDObjectType(const string &);
    
    // function to flatten/serialize the object when the drop is taking place
    void flatten();
    
    // utility function which can be used in TDropSite::dropRequest to
    // check the MIME type
    static bool select(TDnDObject&);
    
    static string unflatten(TDnDObject &drop);
};

class TDropSiteObjectType:
  public TDropSite
{
    typedef TDropSite super;
  public:
    TDropSiteObjectType(TNetEditor *p):super(p) {
      editor = p;
    }
    TDropSiteObjectType(TNetEditor *p, const TRectangle &r):super(p,r) {
      editor = p;
    }
    
    TNetEditor *editor;
    
  protected:
    // function to check whether the drop site would accept the object
    void dropRequest(TDnDObject&);
    // function to actually receive the drop
    void drop(TDnDObject&);
};

} // namespace toad

#endif
