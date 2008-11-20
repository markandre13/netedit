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
#if 0

#include "servermodel.hh"
#include "symbol.hh"

#include <map>
#include <cstdio>
#include <iostream>
#include <dirent.h>
#include <regex.h>

using namespace netedit;

#define NREG 5
static regex_t reg[NREG];
static char* regstr[NREG] = {
  "^ *map *\"([^\"]*)\" *\"([^\"]*)\"",
  "^ *add *\"([^\"]*)\" *\"([^\"]*)\" *([0-9]+) *([0-9]+) *\"([^\"]*)\" *\"([^\"]*)\" *\"([^\"]*)\"",
  "^ *add *\"([^\"]*)\" *\"([^\"]*)\" *\"([^\"]*)\" *([0-9]+) *([0-9]+) *\"([^\"]*)\" *\"([^\"]*)\" *\"([^\"]*)\"",
  "^ *connect *([0-9]+) *\"([^\"]*)\" *([0-9]+) *\"([^\"]*)\" *\"([^\"]*)\"",
  "^ *connect *\"([^\"]*)\" *\"([^\"]*)\" *\"([^\"]*)\" *\"([^\"]*)\" *\"([^\"]*)\"",
};

bool
TServerModel::open(const string &path)
{
  dirent *de;
  DIR *dd;
  
  dd = opendir(path.c_str());
  if (!dd) {
    perror("opendir");
    return false;
  }

  int r;
  for (int i=0; i<NREG; ++i) {
    r = regcomp(&reg[i], regstr[i], REG_EXTENDED);
    if (r!=0) {
      cerr << "regcomp failed for '" << regstr[i]<< "'" << endl;
      char buffer[1024];
      r = regerror(r, &reg[i], buffer, sizeof(buffer));
      cout << buffer << endl;
      exit(1);
    }
  }
  
  while( (de=readdir(dd))!=0 ) {
    if (de->d_name[0]=='.')
      continue;
    string file(path);
    file+="/";
    file+=de->d_name;
    insertFile(file);
  }
  closedir(dd);

  for(int i=0; i<NREG; ++i) {
    regfree(&reg[i]);
  }


  return true;
}

static
string getstr(const char *buffer, const regmatch_t *match, unsigned p)
{
  string s;
  for(int i=match[p].rm_so; i<match[p].rm_eo; ++i) {
    s += buffer[i];
  }
  return s;
}

static
int getint(const char *buffer, const regmatch_t *match, unsigned p)
{
  string s;
  for(int i=match[p].rm_so; i<match[p].rm_eo; ++i) {
    s += buffer[i];
  }
  return atoi(s.c_str());
}

bool
TServerModel::insertFile(const string &file)
{
  FILE *in = fopen(file.c_str(), "r");
  if (!in) {
    perror(file.c_str());
    return false;
  }

  Map *map = 0;
//cout << "file: " << file << endl;
  unsigned counter = 0;
  while(!feof(in)) {
    char buffer[1024];

    if (!fgets(buffer, sizeof(buffer), in)) {
      // perror(file.c_str());
      break;
    }
//    cout << ":" << buffer;
//    bool f = false;
    regmatch_t match[10];
    for (int i=0; i<NREG; ++i) {
      if (regexec(&reg[i], buffer, 10, match, 0)==0) {
//        cout << "matched" << endl;
        switch(i) {
          case 0:
            if (map)
              break;
            maps.push_back(Map());
            map = &maps.back();
            map->name = getstr(buffer, match, 1);
            break;
          case 1: {
            if (!map)
              break;
            map->objects.push_back(Object());
            Object &obj     = map->objects.back();
            obj.sysName     = getstr(buffer, match, 1);
            obj.type        = getstr(buffer, match, 2);
            obj.x           = getint(buffer, match, 3);
            obj.y           = getint(buffer, match, 4);
            obj.label       = getstr(buffer, match, 5);
            obj.submap      = getstr(buffer, match, 6);
            obj.sysLocation = getstr(buffer, match, 7);
            snprintf(buffer, sizeof(buffer), "%i", counter++);
            obj.id = buffer;
          } break;
          case 2: {
            if (!map)
              break;
            map->objects.push_back(Object());
            Object &obj     = map->objects.back();
            obj.id          = getstr(buffer, match, 1);
            obj.sysName     = getstr(buffer, match, 2);
            obj.type        = getstr(buffer, match, 3);
            obj.x           = getint(buffer, match, 4);
            obj.y           = getint(buffer, match, 5);
            obj.label       = getstr(buffer, match, 6);
            obj.submap      = getstr(buffer, match, 7);
            obj.sysLocation = getstr(buffer, match, 8);
          } break;
          case 3:
          case 4: {
            if (!map)
              break;
            map->connections.push_back(Connection());
            Connection &con = map->connections.back();
            con.id0   = getstr(buffer, match, 1);
            con.if0   = getstr(buffer, match, 2);
            con.id1   = getstr(buffer, match, 3);
            con.if1   = getstr(buffer, match, 4);
            con.label = getstr(buffer, match, 5);
          } break;
        }
//        f = true;
        break;
      }
    }
//    if (!f) {
//      cout << "!" << buffer;
//    }
  }
  fclose(in);
  return true;
}

TMapModel* 
TServerModel::getNetModel(unsigned map)
{
  std::map<string, TSymbol*> db;
  TMapModel *m = new TMapModel();
  for(vector<Object>::iterator p = maps[map].objects.begin();
      p!=maps[map].objects.end();
      ++p)
  {
    TSymbol *nd = new TSymbol;
    nd->label = p->label;
    nd->sysName = p->sysName;
    nd->sysLocation = p->sysLocation;
    nd->type = p->type;
    // nd->submap = p->submap;
    nd->x = p->x + 24;
    nd->y = p->y + 24;
    m->add(nd);
    db[p->id] = nd;
  }
  
  for(vector<Connection>::iterator p = maps[map].connections.begin();
      p!=maps[map].connections.end();
      ++p)
  {
    TSymbol *nd0 = 0, *nd1 = 0;
    std::map<string, TSymbol*>::iterator q;
    q = db.find(p->id0);
    if (q!=db.end())
      nd0 = q->second;
    q = db.find(p->id1);
    if (q!=db.end())
      nd1 = q->second;
    if (nd0 && nd1)
      m->connectDevice(nd0, nd1);
  }
  
  return m;
}

#endif
