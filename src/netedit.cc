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
#include "symbol.hh"

#include "nodeeditor.hh"

#include "browser.hh"
#include "server.hh"
#include "snmp.hh"

#include "snmpdialog.hh"

#include <toad/toad.hh>
#include <toad/figureeditor.hh>
#include <toad/undomanager.hh>

#include <toad/action.hh>
#include <toad/form.hh>
#include <toad/menubar.hh>
#include <toad/fatradiobutton.hh>
#include <toad/combobox.hh>

#include <fstream>
#include <toad/filedialog.hh>

#include <vector>

using namespace netedit;

string programname ("NetEdit II (alpha)");
static string login, password;

PServer server;
PFigureAttributes figureattributes;

namespace netedit {

string resource(const string &filename)
{
  return "resource/" + filename;
}

} // namespace netedit

class TMapListAdapter:
  public GTableAdapter<TServer::MapList>
{
    typedef GTableAdapter<TServer::MapList> super;
    int w, h;
    
  public:
    TMapListAdapter(TServer::MapList *model)
      //:super(model) {}
    {
      setModel(model);
    }
    void modelChanged(bool newmodel) {
      if (model) {
        TFont &font(TOADBase::getDefaultFont());
        h = font.getHeight();
        w = 0;
        for(TServer::MapList::iterator p = model->begin();
            p != model->end();
            ++p)
        {
          w = max(w, font.getTextWidth(p->name));
        }
      }
      TTableAdapter::modelChanged(newmodel);
    }
    size_t getCols() { return 1; }
    size_t getRows() { return model ? model->size() : 0; }
    TServer::MapList *getModel() const { return model; }
    
    void tableEvent(TTableEvent &te) {
      switch(te.type) {
        case TTableEvent::GET_COL_SIZE:
          te.w = w;
          break;
        case TTableEvent::GET_ROW_SIZE:
          te.h = h+2;
          break;
        case TTableEvent::PAINT:
          renderBackground(te);
          te.pen->drawString(1, 1, (*model)[te.row].name);
          renderCursor(te);
          break;
        default:
          ;
      }
    }
};

class TMainWindow:
  public TForm
{

  public:
    TMainWindow(TWindow *parent, const string &title);
    static PFigureAttributes fe;
    
    void menuOpen();
    void menuQuit();
    void menuNewView();
    void editNodes();
};

TEditorWindow::TEditorWindow(TWindow *parent, const string &title):
  TWindow(parent, title)
{
  setSize(640,480);
  CONNECT(server->sigChanged, this, serverChanged);

  ne = new TNetEditor(this, "neteditor");
  ne->setAttributes(figureattributes);
  TComboBox *cb = new TComboBox(this, "maplist");
  
  // cb->setSelectionModel(&server->map);
  cb->setAdapter(new TMapListAdapter(&server->maplist));
  TCLOSURE2(cb->getSelectionModel()->sigChanged, 
    s, cb->getSelectionModel(),
    w, this,
    w->gotoMapByRow(s->getRow());
  )
  gotoMapByRow(0);
  
  loadLayout(resource("TEditorWindow.atv"));
}

void
TEditorWindow::gotoMapByRow(unsigned i)
{
  nextmap = server->getMapIDByRow(i);
cout << "retrieve map " << nextmap << endl;
  server->sndGetMapModelByRow(i);
}

void
TEditorWindow::gotoMapByID(unsigned id)
{
  nextmap = id;
cout << "retrieve map " << nextmap << endl;
  server->sndGetMapModel(id);
}

void
TEditorWindow::serverChanged()
{
  switch(server->reason) {
    case TServer::MAPLIST_AVAILABLE:
      gotoMapByRow(0);
      break;
    case TServer::NETMODEL_CHANGED:
      if (server->netmodel->id == nextmap) {
        cout << "new netmodel " << nextmap << endl;
        ne->setModel(server->netmodel);
      }
      break;
    default:
      cout << "unknown server notification reason" << endl;
  }
}



TNetEditor::TNetEditor(TWindow *parent, const string &title):
  TFigureEditor(parent, title)
{
  setModel(0);
  new TDropSiteObjectType(this);
}

void
TNetEditor::mouseEvent(TMouseEvent &me)
{
  if (getOperation()!=OP_CONNECT) {
    super::mouseEvent(me);
    return;
  }
  
  if (me.type!=TMouseEvent::LDOWN)
    return;
  
  // bookkeeping for LDOWN (should be provided by a TFigureEditor
  // helper method)  
  if (!window)
    return;
  setFocus();
  if (preferences)
    preferences->setCurrent(this);
  int x, y;
  mouse2sheet(me.x, me.y, &x, &y);
  sheet2grid(x, y, &x, &y);
  
  TFigure *f = findFigureAt(x, y);
  if (!f)
    return;

  TSymbol *nd = dynamic_cast<TSymbol*>(f);
  if (!nd)
    return;
    
  switch(state) {
    case 0:
      cout << "found first device" << endl;
      nd0 = nd;
      state = 1;
      break;
    case 1:
      cout << "found second device" << endl;
      state = 0;
      // see also: TSymbol::editEvent
      int id = model->uniqueConnID();
      model->connectDevice(id, nd0, nd);
      model->server->sndConnectSymbol(model->id, id, nd0->id, nd->id);

      invalidateWindow(visible);
      break;
  }
}

void
TNetEditor::invalidateFigure(TFigure *f)
{
  if (!window)
    return;

  TSymbol *nd = dynamic_cast<TSymbol*>(f);
  if (f) {
    for(TMapModel::TConnections::iterator p = model->connections.begin();
        p != model->connections.end();
        ++p)
    {
      if ((*p)->nd0 == nd || (*p)->nd1 == nd) {
        TRectangle r((*p)->nd0->x, (*p)->nd0->y,
                     (*p)->nd1->x - (*p)->nd0->x, (*p)->nd1->y - (*p)->nd0->y);
        r.x+=window->getOriginX() + visible.x;
        r.y+=window->getOriginY() + visible.y;
        
        r.x -= 2;
        r.y -= 2;
        r.w += 5;
        r.h += 5;
        
        invalidateWindow(r);
      }
    }
  }
  super::invalidateFigure(f);
}

TMainWindow::TMainWindow(TWindow *parent, const string &title):
  TForm(parent, title)
{
/*
  server = new TServerModel();
  if (!server->open("../old/a.dmap")) {
    cout << "open failed" << endl;
    return;
  }
*/

  TMenuBar *mb = new TMenuBar(this, "menubar");
  mb->loadLayout(resource("menubar.atv"));

  TAction *a;
  a = new TAction(this, "file|new");

  a = new TAction(this, "file|open");
  CONNECT(a->sigClicked, this, menuOpen);
  a = new TAction(this, "file|save_as");
  a = new TAction(this, "file|close");  
  a = new TAction(this, "file|quit");   
  CONNECT(a->sigClicked, this, menuQuit);

  a = new TAction(this, "topology|networks");
  a = new TAction(this, "topology|nodes");
  CONNECT(a->sigClicked, this, editNodes);
  
  a = new TAction(this, "view|new_view");
  CONNECT(a->sigClicked, this, menuNewView);
  a = new TAction(this, "view|maps");
  
  a = new TAction(this, "rights|users");
  a = new TAction(this, "rights|groups");

  a = new TAction(this, "help|about");
  TCLOSURE1(
    a->sigClicked,
    wnd, this,
    TBitmap bmp;
    bmp.load(resource("netedit.png"));
    messageBox(
      wnd,
      "About",
      "NetEdit II\n\n"
      "Copyright 1998-2006 Mark-André Hopf <mhopf@mark13.org>",
      TMessageBox::ICON_INFORMATION|TMessageBox::OK,
      &bmp);
  )

  setBackground(192,192,192);
  setSize(320,64);

  static TFLine gline;     
  static TFText gtext;     
  static TSymbol gnetdevice;

  TRadioStateModel *state = new TRadioStateModel();
  TWindow *prev = 0;
  TWindow *wnd;
  TButtonBase *rb;
  TFigureAttributes *fe = figureattributes;
  for(unsigned i=0; i<=20; i++) {
    wnd = NULL;
    switch(i) {
      case 0:  
        wnd = rb = new TFatRadioButton(this, "select", state);
        rb->loadBitmap(resource("tool_select.png"));
        rb->setToolTip("Edit, move and edit nodes.");
        CONNECT(rb->sigClicked, fe, setOperation, TFigureEditor::OP_SELECT);
        rb->setDown();
        break;
      case 1:
        wnd = rb = new TFatRadioButton(this, "text", state);
        rb->loadBitmap(resource("tool_text.png"));
        CONNECT(rb->sigClicked, fe, setCreate, &gtext);
        break;
      case 3: 
        wnd = rb = new TFatRadioButton(this, "line", state);
        rb->loadBitmap(resource("tool_connect.png"));
        CONNECT(rb->sigClicked, fe, setOperation, TNetEditor::OP_CONNECT);
        break;
      // backbone/network
      case 5: 
        wnd = rb = new TFatRadioButton(this, "netdevice", state);
         rb->loadBitmap(resource("tool_device.png"));
        // rb->loadBitmap("memory://resource/tool_text.png");
        CONNECT(rb->sigClicked, fe, setCreate, &gnetdevice);
        break;
    }
    if (wnd) {
      wnd->setSize(34,34);
      attach(wnd, TOP, mb);
      attach(wnd, LEFT, prev);
      distance(wnd, 3);
      prev = wnd;
    }
  }  
  attach(mb, TOP|LEFT|RIGHT);
#if 0
  attach(fe, TOP, mb);
  attach(fe, RIGHT);
  attach(fe, LEFT, prev);
  attach(fe, BOTTOM, cb);  
  attach(cb, BOTTOM);
  attach(cb, LEFT, prev);
  distance(cb, 2);
#endif
}

void
TMainWindow::menuOpen()
{
}

void
TMainWindow::menuQuit()
{
  toad::postQuitMessage(0);
}

void
TMainWindow::menuNewView()
{
  TWindow *w = new TEditorWindow(0, "NetEdit - View");
  w->createWindow();
}

void
TMainWindow::editNodes()
{
/*
  TWindow *w = new TNodeEditor(this, "NetEdit - Node Editor");
  w->createWindow();
*/
}

int 
main(int argc, char **argv, char **envv)
{
  string hostname = "localhost";
  string port     = "15001";

  string query;
  int i;
  for(i=1; i<argc; ++i) {
    if (strcmp(argv[i], "-h")==0 ||
        strcmp(argv[i], "--host")==0)
    {
      if (i+1>=argc)
        break;
      hostname = argv[++i];
    } else
    if (strcmp(argv[i], "-p")==0 ||
        strcmp(argv[i], "--port")==0)
    {
      if (i+1>=argc)
        break;
      port = argv[++i];
    } else
    if (strcmp(argv[i], "-l")==0 ||
        strcmp(argv[i], "--login")==0)
    {
      if (i+1>=argc)
        break;
      login = argv[++i];
    } else
    if (strcmp(argv[i], "-q")==0 ||
        strcmp(argv[i], "--query")==0)
    {
      if (i+1>=argc)
        break;
      query = argv[++i];
    } else {
      fprintf(stderr, "error: unknown option '%s'\n", argv[i]);
      exit(EXIT_FAILURE);
    }
  }
  
  if (i<argc) {
    fprintf(stderr, "error: not enough arguments for '%s'\n", argv[i]);
    exit(EXIT_FAILURE);
  }

  if (login.empty())
    login = getlogin();

  if (!netedit::initialize())
    return EXIT_FAILURE;

  toad::initialize(1, argv, envv);
  try {
    // createMemoryFiles();
  
    TObjectStore& store(toad::getDefaultStore());
    store.registerObject(new TSymbol);
    store.registerObject(new TConnection);
    store.registerObject(new TMapModel(0, 0));

    figureattributes = new TFigureAttributes;
    
    if (!query.empty()) {
      TSNMPDialog *dlg = new TSNMPDialog(0, "NetEdit: " + query);
      dlg->hostname = query;
    } else {
      server = new TServer(hostname, atoi(port.c_str()));
      server->sndLogin(login, password);

      //    TBrowser wnd0(NULL, "NetEdit: Object Types");    
      new TEditorWindow(0, "NetEdit - View");
      new TMainWindow(0, "NetEdit");
    }
    toad::mainLoop();
  }
  catch(exception e) {
    cerr << "caught exception" << endl;
  }
  toad::terminate();
  return 0;
}
