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

#include "netedit.hh"
#include "nodeeditor.hh"

#include <toad/textfield.hh>
#include <toad/textarea.hh>
#include <toad/table.hh>
#include <toad/pushbutton.hh>
#include <toad/checkbox.hh>
#include <toad/treeadapter.hh>
#include <toad/combobox.hh>

using namespace netedit;

class TLockButton:
  public TButtonBase
{
    typedef TButtonBase super;
    PServer server;
    TNodeModel *node;
    
    void keyDown(TKey key, char* str, unsigned modifier);
    void mouseLDown(int, int, unsigned);
    void paint();
    
    void update();
  public:
    TLockButton(TWindow *parent, const string &title, TNodeModel *node, TServer *server);
    ~TLockButton();
};

TLockButton::TLockButton(TWindow *parent, const string &title, TNodeModel *node, TServer *server):
  super(parent, title)
{
  this->node = node;
  this->server = server;
  CONNECT(node->lock.sigChanged, this, update);
  update();
}

TLockButton::~TLockButton()
{
  disconnect(node->lock.sigChanged, this);
}

void
TLockButton::keyDown(TKey key, char* str, unsigned modifier)
{
  if (!modifier && (key==TK_RETURN || *str==' '))
    mouseLDown(0,0,0);
}
      

void
TLockButton::mouseLDown(int, int, unsigned)
{
  if (!isEnabled()) {
    cout << "lock button isn't enabled" << endl;
    return;
  }

  switch(node->lock.get()) {
    case UNLOCKED:
      server->sndLockNode(node->node_id);
      break;
    case LOCKED_LOCAL:
//      server->sndUnlockNode(node->node_id);
      break;
    default:
      ;
  }  
}

void
TLockButton::paint()
{
  TPen pen(this);

  bool down;
  string txt;
  
  switch(node->lock.get()) {
    case UNLOCKED:
      down = false;
      pen.setColor(TColor::BTNFACE);
      txt = "Lock";
      setToolTip("Click here to lock node for editing.");
      break;
    case LOCKED_REMOTE:
      down = true;
      pen.setColor(128,0,0);
      txt = "Locked";
      setToolTip("The node is locked by " + node->lock.login + 
                 " at " + node->lock.hostname + ".");
      break;
    case LOCKED_LOCAL:
      down = true;
      pen.setColor(0,128,0);
      txt = "Locked";
      setToolTip("Click on 'Reset' or 'Apply' to unlock the node.");
      break;
  }
  pen.fillRectangle(0, 0, getWidth(), getHeight());
  drawShadow(pen, down);
  drawLabel(pen, txt, down);
}

class TInterfacesAdapter:
  public TTableAdapter, GModelOwner<TNodeModel::TInterfaces>
{
    public:
      TInterfacesAdapter(TNodeModel::TInterfaces *interfaces) {
        setModel(interfaces);
      }
      ~TInterfacesAdapter() {
        setModel(0);
      }
      TNodeModel::TInterfaces *getModel() const {
        return GModelOwner<TNodeModel::TInterfaces>::getModel();
      }
      void modelChanged(bool newmodel) {
        TTableAdapter::modelChanged(newmodel);
      }
      virtual size_t getCols() { return 4; }
      void tableEvent(TTableEvent &te);
      
      void addInterface();
      void delInterface();
};

void
TInterfacesAdapter::addInterface()
{
  TInterface *in = new TInterface;
  in->ifDescr = "(new)";
  model->push_back(in);
}

void
TInterfacesAdapter::delInterface()
{
  size_t row = table->getCursorRow();
  if (row<model->size())
    model->erase(model->begin()+row);
}

void
TInterfacesAdapter::tableEvent(TTableEvent &te)
{
  if (!model) {
//    cout << te << endl;
    return;
  }
  TInterface *in = 0;
  if (te.row<model->size())
    in = model->at(te.row);
  switch(te.col) {
    case 0:
//      handleInteger(te, &(*interfaces)[te.row].ifIndex);
//      break;
    case 1:
      handleString(te, in ? &in->ifDescr : 0);
      break;
    case 2:
      handleString(te, in ? &in->ipaddress : 0);
      break;
    case 3:
      handleString(te, in ? &in->ifPhysAddress : 0);
      break;
  }
}

void
TLockButton::update()
{
  switch(node->lock.get()) {
    case LOCKED_LOCAL:
    case UNLOCKED:
      setEnabled(true);
      break;
    case LOCKED_REMOTE:
      setEnabled(false);
      break;
  }
  invalidateWindow();
}

TNodeEditor::TNodeEditor(TWindow *parent, const string &title, TNodeModel *model, TServer *server):
  super(parent, title)
{
  this->model = model;
  this->server= server;

  TPushButton *pb;
  TTextArea *tf;
  TTable *tb;
  TCheckBox *cb;
  TComboBox *co;

  tf = new TTextField(this, "sysObjectID", &model->sysObjectID);
  tf = new TTextField(this, "sysName", &model->sysName);
  tf = new TTextField(this, "sysContact", &model->sysContact);
  tf = new TTextField(this, "sysLocation", &model->sysLocation);
  tf = new TTextField(this, "sysDescr", &model->sysDescr);
  cb = new TCheckBox(this, "ipforwarding", &model->ipforwarding);
  tf = new TTextField(this, "mgmtaddr", &model->mgmtaddr);
  
  co = new TComboBox(this, "mgmt");
  
  cb = new TCheckBox(this, "topo_removed");   // 2
  cb = new TCheckBox(this, "topo_connector"); // 16
  cb = new TCheckBox(this, "topo_gateway");   // 32
  cb = new TCheckBox(this, "topo_star_hub");  // 64
  cb = new TCheckBox(this, "topo_bridge");    // 128
  
  // status
  
  // interfaces
  tb = new TTable(this, "interfaces");
  TInterfacesAdapter *ta = new TInterfacesAdapter(&model->interfaces);
  tb->setAdapter(ta);
  TDefaultTableHeaderRenderer *hdr = new TDefaultTableHeaderRenderer;
  hdr->setText(0, "ifIndex");
  hdr->setText(1, "ifDescr");
  hdr->setText(2, "ipaddress");
  hdr->setText(3, "ifPhysAddress");
  tb->setColHeaderRenderer(hdr);

  pb = new TPushButton(this, "interface_add");
  CONNECT(pb->sigClicked, ta, addInterface);
  pb = new TPushButton(this, "interface_del");
  CONNECT(pb->sigClicked, ta, delInterface);

  TLockButton *lb = new TLockButton(this, "lock", model, server);
  
  reset = pb = new TPushButton(this, "reset");
  CONNECT(pb->sigClicked, this, button, BTN_RESET);

  apply = pb = new TPushButton(this, "apply");
  CONNECT(pb->sigClicked, this, button, BTN_APPLY);
  
  cancel = pb = new TPushButton(this, "cancel");
  CONNECT(pb->sigClicked, this, button, BTN_CANCEL);
  
  ok = pb = new TPushButton(this, "ok");
  CONNECT(pb->sigClicked, this, button, BTN_OK);

  CONNECT(model->lock.sigChanged, this, update);
  update();
  
  loadLayout(resource("TNodeEditor.atv"));
}

TNodeEditor::~TNodeEditor()
{
  // drop references to model before dropping it
  deleteChildren();
  
  // drop model and maybe delete it
  if (server)
    server->sndCloseNode(model->node_id);
}

void
TNodeEditor::update()
{
  switch(model->lock.get()) {
    case LOCKED_LOCAL:
      reset->setEnabled(true);
      apply->setEnabled(true);
      break;
    case UNLOCKED:
    case LOCKED_REMOTE:
      reset->setEnabled(false);
      apply->setEnabled(false);
      break;
  }
}

void
TNodeEditor::button(int b)
{
  assert(server!=0);

  if (b==BTN_APPLY || b==BTN_OK)
    server->sndSetNode(this);
  if (b==BTN_CANCEL || b==BTN_OK)
    sendMessageDeleteWindow(this); // should wait for feedback...
  if (model->lock.get()==LOCKED_LOCAL)
    server->sndUnlockNode(model->node_id);
}

void
TNodeEditor::closeRequest()
{
  sendMessageDeleteWindow(this);
}
