#include "symbol.hh"
#include "mapmodel.hh"

using namespace netedit;

bool
TConnection::editEvent(TFigureEditEvent &ee)
{
  TMapModel *model = dynamic_cast<TMapModel*>(ee.model);
  assert(model!=0);

  // no server means that the server does not need to be informed
  if (!model->server) {
    return true;
  }
               
  switch(ee.type) {
    case TFigureEditEvent::REMOVED:
      cout << "connection was removed" << endl;
      if (!model->itsme)
        model->server->sndDeleteConnection(model->id, conn_id);
      break;
    default:
      ;
  }
  return true;                 
}

double
TConnection::distance(int x, int y)
{
  return distance2Line(x, y, nd0->x, nd0->y, nd1->x, nd1->y);
}

void
TConnection::paint(toad::TPenBase &pen, toad::TFigure::EPaintType type)
{
  pen.setColor(0,0,0);
  pen.setLineStyle(TPen::SOLID);
  pen.setLineWidth(type==NORMAL ? 1 : 2);
  pen.drawLine(nd0->x, nd0->y, nd1->x, nd1->y);
}

void
TConnection::getShape(toad::TRectangle *r)
{
  TPoint p0(nd0->x, nd0->y);
  TPoint p1(nd1->x, nd1->y);
  r->set(p0, p1);
}

bool
TConnection::restore(atv::TInObjectStream &in)
{
  if (
    ::restore(in, "nd0", &id0) ||
    ::restore(in, "nd1", &id1) ||
    super::restore(in)
  ) return true;
  ATV_FAILED(in);
  return false;
}

void
TConnection::store(atv::TOutObjectStream &out) const
{
  ::store(out, "nd0", nd0->id);
  ::store(out, "nd1", nd1->id);
}
