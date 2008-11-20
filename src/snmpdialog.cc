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

#include "netedit.hh"
#include "snmpdialog.hh"

#include <toad/textfield.hh>
#include <toad/textarea.hh>
#include <toad/table.hh>
#include <toad/pushbutton.hh>
#include <toad/ioobserver.hh>
#include <toad/simpletimer.hh>
#include <toad/treeadapter.hh>

using namespace netedit;

namespace netedit {

class TSNMPQueryModel:
  public TSNMPTree, public TTreeModel/*GTreeModel<TSNMPTree::node_t>*/
{
  public:
    enum EReason {
      MODIFIED,
      INSERT
    } reason;
    TOIDNode *node;

    void* _createNode() { return 0; }
    void _deleteNode(void*) { cerr << "deleted node" << endl; }
    void* _getRoot() const { return root; }
    void _setRoot(void*) { cerr << "can't set root" << endl; }
    void* _getDown(void *ptr) const {
      TSNMPTree::node_t *node = static_cast<TSNMPTree::node_t*>(ptr);
/*
      if (node->sminode->nodekind == SMI_NODEKIND_TABLE) {
        return 0;
      }
*/
      return node->down;
    }
    void _setDown(void*, void*) { cerr << "can't set down" << endl; }
    void* _getNext(void *p) const { return static_cast<node_t*>(p)->next; }
    void _setNext(void*, void*)  { cerr << "can't set next" << endl; }
    TSNMPTree::node_t& operator[](size_t i) {
      if (i>=rows->size())
        return *static_cast<TSNMPTree::node_t*>(0);
      return *static_cast<TSNMPTree::node_t*>( (*rows)[i].node );
    }

    bool insert(SmiSubid *oid, size_t oidlen, SmiNode *sminode, const std::string &raw)
    {
      bool b = TSNMPTree::insert(oid, oidlen, sminode, raw);
      update();
      return b;
    }
};

}

class TSNMPTreeRenderer:
  public GTreeAdapter<TSNMPQueryModel>
{
  public:
    TSNMPTreeRenderer(TSNMPQueryModel *tree) {
      setModel(tree);
    }
    
    void tableEvent(TTableEvent &te);
    
    
    SmiNode* sminode(unsigned row) const {
      TSNMPTree::node_t *node = static_cast<TSNMPTree::node_t*>(getModel()->at(row));
      return node->sminode;
    }
#if 0
    const string& tail(unsigned row) const {
      TSNMPTree::node_t *node = static_cast<TSNMPTree::node_t*>(at(row));
      return node->tail;
    }
#endif
    const string& raw(unsigned row) const {
      TSNMPTree::node_t *node = static_cast<TSNMPTree::node_t*>(getModel()->at(row));
      return node->raw;
    }
};

void
TSNMPTreeRenderer::tableEvent(TTableEvent &te)
{
  renderBackground(te);
  if (!model)
    return;
  switch(te.col) {
    case 0: {
      if (te.type==TTableEvent::GET_COL_SIZE) {
        te.w = 4096;
        return;
      }
/*
      switch(te.type) {
        case TTableEvent::KEY:
        case TTableEvent::PAINT:
*/
          string s;
          TSNMPTree::node_t *n = &(*model)[te.row];
          if (n) {
            if (n->sminode) {
              if (n->taillen==0 ||                  // branch
                  (n->taillen==1 && n->tail[0]==0)) // singular value
                s = n->sminode->name;
            } else {
              s = "(no mibinfo?)";
            }
            for(size_t i=0; i<n->taillen; ++i) {
              char buffer[40];
              snprintf(buffer, sizeof(buffer), ".%d", n->tail[i]);
              s+=buffer;
            }
          } else {
            s = "(no entry?)";
          }
          handleString(te, &s, getLeafPos(te.row));
/*
          break;

        case TTableEvent::GET_COL_SIZE:
          te.w = 4096;
          break;
        case TTableEvent::GET_ROW_SIZE:
            
        }
      } 
*/
      handleTree(te);
    } break;
  }
}

const char*
getSmiLanguage(SmiLanguage l)
{
  static const char *table[] = {
    "unknown",
    "SMIv1",  
    "SMIv2",  
    "SMIng",  
    "SPPI"    
  };
  if (l>4)
    l = SMI_LANGUAGE_UNKNOWN;
  return table[l];
}

const char*
getSmiBasetype(SmiBasetype t)
{
  static const char *table[] = {
    "unknown",
    "INTEGER32",
    "OCTETSTRING",
    "OBJECTIDENTIFIER",
    "UNSIGNED32",
    "INTEGER64", 
    "UNSIGNED64",
    "FLOAT32",   
    "FLOAT64",   
    "FLOAT128",  
    "ENUM",
    "BITS" 
  };
  if (t>11)
    t = SMI_BASETYPE_UNKNOWN;
  return table[t];
}

const char*
getSmiIndexkind(SmiIndexkind t)
{
  static const char *table[] = {
    "unknown",
    "index",  
    "augment",
    "reorder",
    "sparse", 
    "expand"  
  };
  if (t>5)
    t = SMI_INDEX_UNKNOWN;
  return table[t];
}

const char*
getSmiAccess(SmiAccess t)
{
  static const char *table[] = {
    "unknown",
    "not implemented",
    "not accessible", 
    "notify",
    "read only",
    "read write",
    "install",   
    "install notify",
    "report only"
  };
  if (t>8)
    t = SMI_ACCESS_UNKNOWN;
  return table[t];
}

const char*
getSmiNodekind(unsigned u)
{
  static char result[1024];
  static const char *table[] = {
    "unknown",
    "node",   
    "scalar", 
    "table",  
    "row",    
    "column", 
    "notificaton",
    "group",
    "compliance",
    "capabilities"
  };
    
  if (u==SMI_NODEKIND_UNKNOWN)
    return "unknown";
  if (u==SMI_NODEKIND_ANY)
    return "any";
  unsigned p = 0;
  for(unsigned i=1; i<=9; ++i) {
    unsigned j = 1<<i;
    if (u & j) {
      size_t l = strlen(table[i]);
      if (p!=0) {
        result[p++]=',';
        result[p++]=' ';
      }
      memcpy(result+p, table[i], l);
      p+=l;
    }
  }  
  result[p]=0;
  return result;
}


TSNMPDialog::TSNMPDialog(TWindow *parent, const string &title):
  super(parent, title)
{
  tree = new TSNMPQueryModel();
  query = new TSNMPQuery();

  TPushButton *pb;
  TTextArea *tf;
  TTable *tb;

  community = "public";
  hostname  = "localhost";
  oid = "1.3.6.1.2.1.1.1.0";
    
  tf = new TTextField(this, "ip");
  tf->setModel(&hostname);
    
  tf = new TTextField(this, "community");
  tf->setModel(&community);

  tf = new TTextField(this, "oid");
  tf->setModel(&oid);
    
  tf = new TTextArea(this, "result");
  tf->setModel(&result);
    
  tb = new TTable(this, "tree");
  TSNMPTreeRenderer *st = new TSNMPTreeRenderer(tree);
  tb->setAdapter(st);
  TCLOSURE3(
    tb->sigClicked,
    table, tb,
    renderer, st,
    dlg, this,
    int row = table->getCursorRow();
//    cout << "clicked on table entry " << row << endl;
    SmiNode *node = renderer->sminode(row);
    
    dlg->name.clear();
    dlg->value.clear();
    dlg->syntax.clear();
    dlg->description.clear();
    
    if (node /*&& o->type*/) {
      if (node->name)
        dlg->name = node->name;

      dlg->value = renderer->raw(row);

      if (node->description)
        dlg->description = node->description;
      
      printf("access       : %s (%d)\n", getSmiAccess(node->access), node->access);
      printf("format       : %s\n", node->format);
      printf("units        : %s\n", node->units);
      printf("indexkind    : %s\n", getSmiIndexkind(node->indexkind));
      printf("implied      : %d\n", node->implied);
      printf("create       : %d\n", node->create);
      printf("nodekind     : %s (0x%x)\n", getSmiNodekind(node->nodekind), node->nodekind);
      
      SmiType *type = smiGetNodeType(node);
      if (type) {
        dlg->syntax = getSmiBasetype(type->basetype);
        if (type->name) {
          dlg->syntax += " (";
          dlg->syntax += type->name;
          dlg->syntax += ")";
        }
        
        printf("type:\n");
        printf("  name       : %s\n", type->name);
        printf("  basetype   : %s\n", getSmiBasetype(type->basetype));
        printf("  format     : %s\n", type->format);
        printf("  units      : %s\n", type->units);
        printf("  description: %s\n", type->description); // description of the type
        printf("  reference  : %s\n", type->reference);
        printf("  named numbers:\n");
        for(SmiNamedNumber *nn = smiGetFirstNamedNumber(type);
            nn!=0;
            nn = smiGetNextNamedNumber(nn))
        {
          printf("    %s (%d)\n", nn->name, nn->value.value.unsigned32);
        }
      }
      printf("\n");
    }
  )
    
  tf = new TTextArea(this, "name");
  tf->setModel(&name);

  tf = new TTextArea(this, "value");
  tf->setModel(&value);

  tf = new TTextArea(this, "syntax");
  tf->setModel(&syntax);

  tf = new TTextArea(this, "description");
  tf->setModel(&description);
    
  pb = new TPushButton(this, "snmpQuery");
  //CONNECT(pb->sigClicked, this, snmpQuery);

  pb = new TPushButton(this, "stop");
  CONNECT(pb->sigClicked, this, stop);

  pb = new TPushButton(this, "snmpWalk");
  CONNECT(pb->sigClicked, this, snmpWalk);

  pb = new TPushButton(this, "clearResult");
  //connect(pb->sigClicked, clearResult);
    
  loadLayout(resource("querySNMP.atv"));
}

TSNMPDialog::~TSNMPDialog()
{
  // drop references to model
  deleteChildren();

  query->close();
  delete tree;
  delete query;
}


void
TSNMPDialog::snmpWalk()
{
//  tree->lock();
  query->close();
  query->tree = tree;
  query->hostname = hostname;
  query->community = community;
  query->result = &result;
  if (query->open())
    query->snmpWalk();
}

void
TSNMPDialog::stop()
{
  query->stop();
//  tree->unlock();
}
