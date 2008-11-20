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
#include <toad/figureeditor.hh>
#include <toad/textfield.hh>
#include <toad/pushbutton.hh>
#include <toad/dialog.hh>

#include <toad/action.hh>
#include <toad/popupmenu.hh>

#include <fstream>
#include <map>

#include "netedit.hh"
#include "mapmodel.hh"
#include "symbol.hh"
#include "snmpdialog.hh"
#include "server.hh"

#include "browser.hh"

using namespace netedit;
using namespace toad;

#if 0
const double toad::TFigure::OUT_OF_RANGE = HUGE_VAL;
const double toad::TFigure::RANGE = 5.0;
static const double toad::TFigure::INSIDE = -1.0;
#endif

imgtxt_t netedit::imgtxt[] =
{
  { "Connector:BayNetworks-Router",	"BayNetworksRouter.png" },
  { "Generic Hub"                 , "Generic_Hub.png" },
  { "SNPX HUB 5000",			"SnpxHub5000.png" },
  { "SNPX HUB 5000 FDDI",		"SnpxHub5000.png" },
  { "SNPX HUB 5000 TR",		"SnpxHub5000.png" },
  { "SNPX HUB 5000 ENET",		"SnpxHub5000.png" },
  { "SNPX 281X Enet Hub",		"SNPX_281X_Enet_Hub.png" },
  { "SNPX 810 Hub",			"SNPX_810_Hub.png" },
  { "SNPX D5000 Hub",		"SNPX_D5000_Hub.png" },
  { "SNPX 3000 Hub",			"SnpxHub5000.png" },
  { "BayStack FastEN. Hub",		"BayStack_FastEN_Hub.png" },
  { "Bridge",			"Bridge.png" },
  { "Local Bridge",			"Bridge.png" },
  { "SNMP",				"GenericSNMP.png" },
  { "Router",			"Router.png" },
  { "28XXX Switch",			"28XXX_Switch.png" },
  { "PC Agent",			"PC_Agent.png" },
  { "Personal Computer",		"Personal_Computer.png" },
  { "Small Computer",		"Small_Computer.png" },
  { "Large Computer",		"Large_Computer.png" },
  { "Submap",			"Submap.png" },
  { "Document Server",		"Document_Server.png" },
  { "Database Server",		"Database_Server.png" },
  { "BayNet Stack Probe",		"BayNet_Stack_Probe.png" },
  { "Text",				"SmallText.png" },
  { 0, 0 }
};

map<string, TFigure*> figtxt;

void
netedit::loadimages()
{
  static bool loaded = false;
  if (loaded)
    return;
  loaded = true;
  
  imgtxt_t *p = imgtxt;
  while(p->name) {
    p->bmp = new TBitmap;
    string file = "icons/";
    file += p->file;
    if (!p->bmp->load(file)) {
      cout << "failed to load icon " << p->file << endl;
      delete p->bmp;
      p->bmp = 0;
    } else {
      p->bmp->_toad_ref_cntr++;
    }
    ++p;
  };

  // new variant vector icons
  ifstream fin("netedit-icons.fish");
  
  string iconname;
  TFGroup *g = 0;
  
  if (fin) {
    // TATVParser in(&fin);
    TInObjectStream in(&fin);
    in.setInterpreter(0);
    while(in.parse()) {
#if 0
      switch(in.what) {
        case ATV_START:
          cout << "ATV_START";
          break;
        case ATV_VALUE:
          cout << "ATV_VALUE";
          break;
        case ATV_GROUP:
          cout << "ATV_GROUP";
          break;
        case ATV_FINISHED:
          cout << "ATV_FINISHED";
          break;
      }
      cout << ": ('" << in.attribute 
           << "', '" << in.type
           << "', '" << in.value
           << "'), position=" << in.getPosition()
           << ", depth=" << in.getDepth() << endl;
#endif
      bool ok = false;
      switch(in.getDepth()) {
        case 0:
          switch(in.what) {
            case ATV_FINISHED:
              ok = true;
              break;
          }
          break;
        case 1:
          switch(in.what) {
            case ATV_GROUP:
              if (in.type=="fischland::TDocument")
                ok=true;
              break;
            case ATV_VALUE:
            case ATV_FINISHED:
              ok = true;
              break;
          }
          break;
        case 2:
          switch(in.what) {
            case ATV_GROUP:
              if (in.type=="fischland::TSlide") {
                if (!g) {
                  g = new TFGroup;
                  g->mat = new TMatrix2D();
                  g->mat->scale(1.0/96.0, 1.0/96.0);
                  g->mat->translate(48.0, 48.0);
                }
                ok = true;
            } break;
            case ATV_VALUE:
              if (in.attribute == "name") {
                iconname = in.value;
cout << "load icon '" << iconname << "'" << endl;
              }
              ok = true;
              break;
            case ATV_FINISHED: {
              if (g) {
                if (figtxt.find(iconname)==figtxt.end()) {
                  g->calcSize();
                  figtxt[iconname] = g;
                  g = 0;
                } else {
                  cout << "duplicate icon '" << iconname << "'\n";
                }
              }
              ok = true;
            } break;
          }
          break;
        case 3:
          switch(in.what) {
            case ATV_GROUP:
              if (in.type=="fischland::TLayer")
                ok = true;
              break;
            case ATV_VALUE:
            case ATV_FINISHED:
              ok = true;
              break;
          }
          break;
        case 4:
          switch(in.what) {
            case ATV_GROUP: {
              TObjectStore &os(getDefaultStore());
              TSerializable *s = os.clone(in.type);
              if (s) {
//                cout << "parse object" << endl;
                in.setInterpreter(s);
in.stop();
                ok = in.parse();
//                cout << "parsed object" << endl;
                if (!ok) {
                  cout << "not okay: " << in.getErrorText() << endl;
                }
                in.setInterpreter(0);
                
                TFigure *nf = dynamic_cast<TFigure*>(s);
                if (!nf)
                  delete nf;
                else {
                  g->gadgets.add(nf);
                }
              }
            }
            break;
          }
      }
      if (!ok) {
        cout << "parse error in icon file" << endl;
        exit(1);
      }
    }
  } else {
    cout << "failed to open vector icon files" << endl;
  }


}

TSymbol::TSymbol()
{
  set(0,0,32,32);
}
 
unsigned
TSymbol::mouseLDown(TFigureEditor *editor, int mx, int my, unsigned)
{
  if (editor->state==TFigureEditor::STATE_START_CREATE) {
    x = mx;
    y = my;
    editor->invalidateFigure(this);
    return STOP;
  }
  return CONTINUE;
}

void
TSymbol::paint(TPenBase &pen, EPaintType ptype)
{
  pen.setLineColor(0,0,0);
  int cx = this->x - w/2;
  int cy = this->y - h/2; 

  loadimages();

  map<string, TFigure*>::iterator f = figtxt.find(type);
  if (f!=figtxt.end()) {
    TRectangle br;
    br.set(cx,cy,32,32);

    pen.setColor(0,0,0);
    pen.setLineWidth(1);
    if (type.compare(0, 9, "Computer:", 9)==0) {
      pen.setFillColor(0, 0, 255);
      pen.fillRectangle(br);
    } else
    if (type.compare(0, 10, "Connector:", 10)==0) {
      pen.setFillColor(255,255,0);
      TPoint p[4];
      p[0].set(br.x + br.w / 2, br.y);
      p[1].set(br.x + br.w    , br.y + br.h / 2);
      p[2].set(br.x + br.w / 2, br.y + br.h);
      p[3].set(br.x           , br.y + br.h / 2);
      pen.fillPolygon(p, 4);
    } else
    if (type.compare(0, 4, "Map:", 4)==0) {
      if (objid==0) // id 0 isn't a valid map
        pen.setFillColor(191,191,191);
      else
        pen.setFillColor(0,0,255);
      pen.fillCircle(br);
    }

    TFigure *figure = f->second;
    pen.push();
    pen.translate(cx, cy);
    if (figure->mat)
      pen.multiply(figure->mat);
    figure->paint(pen, ptype);
    pen.pop();

    pen.setLineColor(0,0,0);
    pen.setLineWidth(1);
  } else {
    pen.setFillColor(255,128,0);
    pen.fillRectanglePC(x,y,w,h);
  }

  if (ptype!=NORMAL) {
    pen.drawRectanglePC(cx-1,cy-1,w+2,h+2);
  }

  pen.setFillColor(255,255,255);

  pen.setFont("arial,helvetica,sans-serif:size=8");
  
  string txt;
  if (label.empty())
    txt = sysName;
  else
    txt = label;

  int tw = pen.getTextWidth(txt);
  int th = pen.getHeight();
  int tx = cx + w/2 - tw/2;
  int ty = cy + h;
  pen.fillRectanglePC(tx-2,ty,tw+4,th+4);
  pen.drawString(tx,ty+2,txt);
  
  if (ptype!=NORMAL) {
    pen.drawRectanglePC(tx-3,ty-1,tw+6,th+6);
  }
}
 
void
TSymbol::paintSelection(TPenBase&, int)
{
}
 
double
TSymbol::distance(int mx, int my)
{
  TRectangle r(*this);
  r.x-=w/2;
  r.y-=w/2;
#if 0
  return r.isInside(mx,my) ? INSIDE : OUT_OF_RANGE;
#else
  return r.isInside(mx,my) ? -1.0 : HUGE_VAL;
#endif
}

void
TSymbol::getShape(TRectangle *rect)
{
  *rect = *this;
  rect->x -= w/2;
  rect->y -= h/2;

  TFont *fnt = TPen::lookupFont("arial,helvetica,sans-serif:size=10");

  string s;
  if (label.empty())
    s = sysName;
  else
    s = label;

  int tw = fnt->getTextWidth(s) + 4;
  if (tw>rect->w) {
    rect->x = rect->x + rect->w/2 - tw/2;
    rect->w = tw;
  }
  
  rect->h += fnt->getHeight() + 4;
  
  rect->x--;
  rect->y--;
  rect->w+=2;
  rect->h+=2;
}

bool
TSymbol::editEvent(TFigureEditEvent &ee)
{
  TMapModel *model = dynamic_cast<TMapModel*>(ee.model);
  assert(model!=0);

  // no server means that the server does not need to be informed
  if (!model->server) {
    return true;
  }
  
  switch(ee.type) {
    // figure was added to model
    case TFigureEditEvent::ADDED:
      id = model->uniqueDeviceID(this);
//      cout << "figure " << id << " was added" << endl;
      model->server->sndAddSymbol(model->id, id, x, y);
      break;
    // figure was removed from a model
    case TFigureEditEvent::REMOVED:
      cout << "figure was removed" << endl;
      model->server->sndDeleteSymbol(model->id, id);
      break;
    // figure was moved
    case TFigureEditEvent::TRANSLATE:
      x+=ee.x; y+=ee.y;
      model->server->sndTranslateSymbol(model->id, id, ee.x, ee.y);
      break;
    case TFigureEditEvent::START_IN_PLACE:
      if (type.compare(0, 4, "Map:", 4)==0) {
        TEditorWindow *ew = dynamic_cast<TEditorWindow*>(ee.editor->getParent());
        assert(ew);
        cout << "go to edit map id " << objid << endl;
        if (objid>0)
          ew->gotoMapByID(objid);
      } else {
        model->server->sndOpenNode(objid);
      }
      return false;
    default:
      ;
  }
  return true;
}

void
TSymbol::translate(int dx, int dy)
{
  x+=dx; y+=dy;
}
 
class TMyPopupMenu:
  public TPopupMenu
{
  public:
    TMyPopupMenu(TWindow *p, const string &t): TPopupMenu(p, t)
    {
//cerr << "create menu " << this << endl;
    }
    ~TMyPopupMenu() {
//cerr << "delete tree " << tree << endl;
      delete tree;
    }
 
    void closeRequest() {
      TPopupMenu::closeRequest();
//cerr << "delete menu " << this << endl;
      delete this;
    }
  
    TInteractor *tree;
};

unsigned
TSymbol::mouseRDown(TFigureEditor *editor, int x, int y, unsigned modifier)
{
  TInteractor *dummy;
  dummy = new TInteractor(0, "dummy interactor");
  TAction *action;
  action = new TAction(dummy, "snmp", TAction::ALWAYS);
  TCLOSURE1(
    action->sigClicked,
    nd, this,
    TSNMPDialog dlg(0, "SNMP");
    dlg.hostname = nd->sysName;
    // dlg->community = 
    // dlg->comment  = nd->comment;
    // dlg->port     = nd->port;
    // dlg->protocol =
    dlg.doModalLoop();
  )
  action = new TAction(dummy, "ping");
  action = new TAction(dummy, "traceroute");
  
  TMyPopupMenu *menu = new TMyPopupMenu(editor, "popup");
  menu->tree = dummy; // to delete the interactor
  menu->setScopeInteractor(dummy);
  menu->open(x, y, modifier);
  return 0;
}

 
void
TSymbol::store(TOutObjectStream &out) const
{
  super::store(out);
  ::store(out, "id", id);
  ::store(out, "x", x);
  ::store(out, "y", y);
  ::store(out, "sysName", sysName);
  // ::store(out, "protocol", protocol);
  ::store(out, "port", port);
  ::store(out, "comment", comment);
}
 
bool
TSymbol::restore(TInObjectStream &in)
{
  if (
    ::restore(in, "id", &id) ||
    ::restore(in, "x", &x) ||
    ::restore(in, "y", &y) ||
    ::restore(in, "sysName", &sysName) ||
    // ::restore(in, "protocol", &protocol) ||
    ::restore(in, "port", &port) ||
    ::restore(in, "comment", &comment) ||
    super::restore(in)
  ) return true;
  ATV_FAILED(in)

  return false;
}
