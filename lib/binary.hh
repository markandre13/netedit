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

#ifndef __NETEDIT_BINARY_HH

namespace netedit {

#include <string>

using namespace std;

inline void
addByte(string *s, unsigned n) {
  s->append(1, n & 0xFF);
}

inline void
setByte(string *s, unsigned p, unsigned n)
{
  (*s)[  p] = (unsigned char)n;
}

inline unsigned
getByte(const string &s, unsigned *p)
{
  unsigned u = (unsigned char)s[*p];
  (*p)++;
  return u;
}

inline void
addDWord(string *s, unsigned n) {
  s->append(1, (n>>24)&0xFF);
  s->append(1, (n>>16)&0xFF);
  s->append(1, (n>> 8)&0xFF);
  s->append(1, (n    )&0xFF);
}

inline void
setDWord(string *s, unsigned p, unsigned n)
{
  (*s)[  p] = (unsigned char)(n>>24)&0xFF;
  (*s)[++p] = (unsigned char)(n>>16)&0xFF;
  (*s)[++p] = (unsigned char)(n>> 8)&0xFF;
  (*s)[++p] = (unsigned char)(n    )&0xFF;
}

inline unsigned
getDWord(const string &s, unsigned *p)
{
  unsigned n = ((unsigned char)s[*p  ]<<24) + 
               ((unsigned char)s[*p+1]<<16) + 
               ((unsigned char)s[*p+2]<< 8) + 
                (unsigned char)s[*p+3];
  *p += 4;
  return n;
}

inline void
addSDWord(string *s, int n) {
  unsigned m;
  if (n<0) {
    m = 0xFFFFFFFF + (n+1);
  } else {
    m = n;
  }
  addDWord(s, m);
}

inline int
getSDWord(const string &s, unsigned *p)
{
  unsigned m = getDWord(s, p);
  int n;
  if (m>0x7FFFFFFF) {
    n = - (0xFFFFFFFF - m) - 1;
  } else {
    n = m;
  }
  return n;
}

inline void
addString(string *s, const string &txt)
{
  addDWord(s, txt.size());
  *s+=txt;
}

inline string
getString(const string &s, unsigned *p)
{
  string result;
  unsigned l = getDWord(s, p);
  result = s.substr(*p, l);
  *p += l;
  return result;
}

inline
void getString(const string &s, unsigned *p, char *buf, size_t n)
{
  unsigned l = getDWord(s, p);
  --n;
  if (l<n)
    n=l;
  memcpy(buf, s.c_str()+*p, n);
  buf[n] = 0;
  *p += l;
}

} // namespace netedit

#endif
