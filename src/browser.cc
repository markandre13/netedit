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

#include "browser.hh"

#include <toad/dnd/color.hh>
#include <toad/figure.hh>

#include <fstream>

using namespace netedit;

class TFLabeledImage:
  public TFImage
{
  public:
    string label;
    TFLabeledImage(const string &filename, const string &label);
    
    void paint(TPenBase &, EPaintType);
    void getShape(TRectangle*);
};

TFLabeledImage::TFLabeledImage(const string &filename, const string &label):
  TFImage(filename)
{
  this->label = label;
}

void
TFLabeledImage::paint(TPenBase &pen, EPaintType pt)
{
  TRectangle r;
  TFImage::getShape(&r);
  int iw = r.w;
  int tw = default_font->getTextWidth(label);
  if (tw > r.w)
    r.w = tw;
  int ix = (r.w - iw) / 2;
  int tx = (r.w - tw) / 2;

  int ox = x;
  x += ix;
  TFImage::paint(pen, pt);
  x = ox;
  pen.setColor(0,0,0);  
  pen.drawString(x + tx, y + r.h, label);
}

void
TFLabeledImage::getShape(TRectangle *r)
{
  TFImage::getShape(r);
  int tw = default_font->getTextWidth(label);
  if (tw > r->w)
    r->w = tw;
  r->h += default_font->getHeight();
}

class TFLabeledFigure:
  public TFigure
{
    typedef TFigure super;
    int x, y;
  public:
    string label;
    TFigure *figure;
    TFLabeledFigure(TFigure *figure, const string &label);
    
    void paint(TPenBase &, EPaintType);
    void getShape(TRectangle*);
    bool editEvent(TFigureEditEvent &ee);

    TCloneable* clone() const { return new TFLabeledFigure(*this); }
    const char *getClassName() const { return "toad::TFLabeledFigure"; }
};

void
getTransformedShape(TFigure *f, TRectangle *r)
{
  f->getShape(r);
  if (!f->mat) {
    return;
  }

  TPoint p1, p2;
  bool first = true;
  for(int i=0; i<4; ++i) {
    int x, y;  
    switch(i) {
      case 0:
        f->mat->map(r->x, r->y, &x, &y);
        break;
      case 1:
        f->mat->map(r->x+r->w, r->y, &x, &y);
        break;
      case 2:
        f->mat->map(r->x+r->w, r->y+r->h, &x, &y);
        break;
      case 3:
        f->mat->map(r->x, r->y+r->h, &x, &y);
        break;
    }
    if (first) {
      p1.x = p2.x = x;
      p1.y = p2.y = y;
      first = false;
    } else {
      if (p1.x>x)
        p1.x=x;  
      if (p2.x<x)
        p2.x=x;  
      if (p1.y>y)
        p1.y=y;  
      if (p2.y<y)
        p2.y=y;
    } 
  }
  r->set(p1, p2);
}

TFLabeledFigure::TFLabeledFigure(TFigure *figure, const string &label)
{
  this->figure = figure;
  this->label = label;
}

void
toad::TFigure::getShape(toad::TRectangle*) {}

void
TFLabeledFigure::paint(TPenBase &pen, EPaintType pt)
{
  TRectangle ir, br;
  getTransformedShape(figure, &ir);
  
  // background size is the size of the image but (32, 32) at minimum
  br.w = ir.w > 32 ? ir.w : 32;
  br.h = ir.h > 32 ? ir.h : 32;
  
  int tw = default_font->getTextWidth(label);
  
  int mw = br.w > tw ? br.w : tw; // maximum width of the symbol
  
  br.x = x + (mw - br.w) / 2;
  br.y = y;

  int ix = (mw - ir.w) / 2 - ir.x + x;
  int iy = (br.h - ir.h) / 2 - ir.y + y;
  
  int tx = (mw - tw) / 2 - ir.y;

//cout << "figure '" << label << "' has background " << br << endl;

  pen.setColor(0,0,0);
  pen.setLineWidth(1);
  if (label.compare(0, 9, "Computer:", 9)==0) {
    pen.setFillColor(0, 0, 255);
    pen.fillRectangle(br);
  }
  if (label.compare(0, 10, "Connector:", 10)==0) {
    pen.setFillColor(255,255,0);
    TPoint p[4];
    p[0].set(br.x + br.w / 2, br.y);
    p[1].set(br.x + br.w    , br.y + br.h / 2);
    p[2].set(br.x + br.w / 2, br.y + br.h);
    p[3].set(br.x           , br.y + br.h / 2);
    pen.fillPolygon(p, 4);
  }
  if (label.compare(0, 4, "Map:", 4)==0) {
    pen.setFillColor(0,0,255);
    pen.fillCircle(br);
  }

  pen.push();
  pen.translate(ix, iy);
  if (figure->mat)
    pen.multiply(figure->mat);
  figure->paint(pen, pt);
  pen.pop();


  pen.setColor(0,0,0);
  pen.setFont(default_font->getFont());
  pen.drawString(x + tx, br.y + br.h, label);
}

void
TFLabeledFigure::getShape(TRectangle *r)
{
  getTransformedShape(figure, r);
  int tw = default_font->getTextWidth(label);
  if (tw > r->w)
    r->w = tw;

if (r->w<32) r->w = 32;
if (r->h<32) r->h = 32;

  r->h += default_font->getHeight();

  r->x = x;
  r->y = y;
}

bool
TFLabeledFigure::editEvent(TFigureEditEvent &ee)
{
  switch(ee.type) {
    case TFigureEditEvent::TRANSLATE:
      x+=ee.x;
      y+=ee.y; 
      break;
    default:
      ;
  }
  return true;
}

TDnDObjectType::TDnDObjectType(const string &objecttype)
{
  setType("x-application/x-netedit-object-type", ACTION_COPY);
  this->objecttype = objecttype;
  this->flatdata   = objecttype;
}

bool
TDnDObjectType::select(TDnDObject &drop)
{
  return TDnDObject::select(drop, "x-application", "x-netedit-object-type");
}

void
TDnDObjectType::flatten()
{
  flatdata = objecttype;
}

string
TDnDObjectType::unflatten(TDnDObject &drop)
{
  return drop.flatdata;
};


void
TDropSiteObjectType::dropRequest(TDnDObject &drop)
{
  if (editor && editor->getModel() && TDnDObjectType::select(drop)) {
    drop.action = ACTION_COPY;
    return;
  }
  drop.action = ACTION_NONE;
}

void
TDropSiteObjectType::drop(TDnDObject &drop)
{
  TSymbol *nd = new TSymbol;
  nd->type = TDnDObjectType::unflatten(drop);

  cout << "dropped '" << nd->type << "'" << endl;

  nd->label = "unnamed";
  
  TRectangle r;
  nd->getShape(&r);
  nd->translate(drop.x - r.x - r.w/2, drop.y - r.y - r.h/2);
  
  editor->addFigure(nd);
}


class TFDragTool:
  public TFigureTool
{
  public:
    void mouseEvent(TFigureEditor *fe, TMouseEvent &me);
};

void
TFDragTool::mouseEvent(TFigureEditor *fe, TMouseEvent &me)
{
  TFigure *figure;
  int x0, y0;
  
  if (me.type==TMouseEvent::LDOWN) {
    fe->mouse2sheet(me.x, me.y, &x0, &y0);
    figure = fe->findFigureAt(x0, y0);
    if (figure) {
//cout << "got figure" << endl;
      TFLabeledImage *li = dynamic_cast<TFLabeledImage*>(figure);
      if (li) {
//cout << "start dragging " << li->label << endl;
        startDrag(new TDnDObjectType(li->label), me.modifier);
        return;
      }
      TFLabeledFigure *lf = dynamic_cast<TFLabeledFigure*>(figure);
      if (lf) {
//cout << "start dragging " << lf->label << endl;
        startDrag(new TDnDObjectType(lf->label), me.modifier);
        return;
      }
      cout << figure->getClassName() << " is not a figure i like" << endl;
    }
  }
}

TBrowser::TBrowser(TWindow *parent, const string &title):
  TFigureEditor(parent, title)
{
  enableGrid(false);
  setTool(new TFDragTool);
#if 0
  // old variant: bitmap icons
  loadimages();
  imgtxt_t *ptr = imgtxt;
  while(ptr->name) {
    string filename;
    filename += "icons/";
    filename += ptr->file;
    addFigure(new TFLabeledImage(filename, ptr->name));
    ++ptr;
  }
#endif
  
  // new variant vector icons
  ifstream fin("netedit-icons.fish");
  
  string iconname;
  
  TFLabeledFigure *f = 0;
  TFGroup *g;
  
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
                if (!f) {
                  g = new TFGroup;
                  f = new TFLabeledFigure(g, "");
                  g->mat = new TMatrix2D();
                  g->mat->scale(1.0/96.0, 1.0/96.0);
                  g->mat->translate(48.0, 48.0);
                }
                ok = true;
            } break;
            case ATV_VALUE:
              if (in.attribute == "name") {
                f->label = in.value;
cout << "load icon '" << iconname << "'" << endl;
              }
              ok = true;
              break;
            case ATV_FINISHED: {
              if (f->label.empty()) {
                g->gadgets.clear();
              } else {
                g->calcSize();
                addFigure(f);
                f = 0;
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
  
  if (f)
    delete f;
  
  arrangeFigures();
}

void
TBrowser::arrangeFigures()
{
  TFigureEditEvent ee;
  ee.model = getModel();
  ee.type = TFigureEditEvent::TRANSLATE;

  TRectangle r;
  int minw, maxw, minh, maxh;
  
  minw = maxw = minh = maxh = 0;
  for(TFigureModel::iterator p = getModel()->begin(); 
      p != getModel()->end(); 
      ++p) 
  {
    (*p)->getShape(&r);

    if ( (*p)->mat ) {
      TMatrix2D m( *(*p)->mat );
      m.map(r.x, r.y, &r.x, &r.y);
      m.map(r.w, r.h, &r.w, &r.h);
    }

cout << "got shape " << r << endl;

    (*p)->editEvent(ee);
    if (p==getModel()->begin()) {
      minw = maxw = r.w;
      minh = maxh = r.h;
    } else {
      if (minw > r.w)
        minw = r.w;
      else if (maxw < r.w)
        maxw = r.w;
      if (minh > r.h)
        minh = r.h;
      else if (maxh < r.h)
        maxh = r.h;
    }
  }
  
  int w = getWidth();
  if (w<640)
    w = 640;
  
  int x, y;
  x = y = 0;
  for(TFigureModel::iterator p = getModel()->begin(); 
      p != getModel()->end(); 
      ++p) 
  {
    (*p)->getShape(&r);

    int fx, fy, mw;
    if ( (*p)->mat ) {
      TMatrix2D m( *(*p)->mat );
      m.invert();
      m.map(x, y, &fx, &fy);
      int foo;
      m.map(maxw, 0, &mw, &foo);
    } else {
      fx = x; fy = y; mw = maxw;
    }

    ee.x = fx - r.x + (mw-r.w)/2;
    ee.y = fy - r.y;
#if 0
    if ( (*p)->mat ) {
      TMatrix2D m( *(*p)->mat );
//      m.invert();
      m.map(ee.x, ee.y, &ee.x, &ee.y);
    }
#endif
    (*p)->editEvent(ee);
    x += maxw;
    if (x + maxw > w) {
      x = 0;
      y += maxh;
    }
  }
}
