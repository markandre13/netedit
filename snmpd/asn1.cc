#include "asn1.hh"
#include <cassert>
#include <iostream>
#include <vector>

using namespace std;

const char *
TASN1::tagName(unsigned tclass, unsigned tag)
{
  if (tclass) {
    static char buffer[256];
    snprintf(buffer, 256, "[%s %i]", className(tclass), tag);
    return buffer;
  }
   
  // universal tag name (bit 4-0)
  static const char *tag_name[] = {
    "EOC",
    "BOOLEAN",
    "INTEGER",
    "BITSTRING",
    "OCTETSTRING",
    "NULL",
    "OID", 
    "OBJDESCRIPTOR",
    "EXTERNAL",
    "REAL",
    "ENUMERATED",
    "EMBEDDED_PDV",
    "UTF8STRING",  
    "",
    "",
    "",
    "SEQUENCE",
    "SET",
    "NUMERICSTRING",
    "PRINTABLESTRING",
    "T61STRING",
    "VIDEOTEXSTRING",
    "IA5STRING",
    "UTCTIME",  
    "GENERALIZEDTIME",
    "GRAPHICSTRING",  
    "VISIBLESTRING",  
    "GENERALSTRING",  
    "UNIVERSALSTRING",
    "BMPSTRING",
  };
    
  return tag <= sizeof(tag_name)/sizeof(char*) ?
            tag_name[tag] :
            "";
}

const char *
TASN1::className(unsigned tclass)
{
  static char * tag_class_name[4] = {
    "UNIVERSAL (ITU X.680)", // defined by ITU X.680
    "APPLICATION",
    "CONTEXT",
    "PRIVATE" 
  };

  return tag_class_name[tclass];
}
 
const char *
TASN1::encodingName(unsigned encoding)
{
  static char * tag_encoding_name[2] = {
    "PRIMITIVE",
    "CONSTRUCTED"
  };
  return tag_encoding_name[encoding];
}

void
TASN1Encoder::encodeNull()
{
  raw.clear();
  encode(0, NULLTAG, 0);
}

void
TASN1Encoder::encodeInteger(int a)
{
  if (a<0) {
    cerr << __PRETTY_FUNCTION__ << ": negative numbers not implemented yet" << endl;
    exit(EXIT_FAILURE);
  }
  unsigned char c;
  raw.clear();
  if (!a) {
    raw+='\0'; // useless but required octet
  } else {
    while(a) {
      c = a & 0xFF;
      raw.insert(raw.begin(), c);
      a>>=8;
    }
  }
  if (c & 0x80)
    raw.insert(raw.begin(), 0);
  encode(0, INTEGER, 0);
}

void
TASN1Encoder::encodeOctetString(const OctetString &str)
{
  encodeHead(0, OCTETSTRING, 0);
  putNumber(&data, str.size);
  data.append(str.data, str.size);
}

void
TASN1Encoder::encodeOctetString(const char *str, size_t size)
{
  encodeHead(0, OCTETSTRING, 0);
  putNumber(&data, size);
  data.append(str, size);
}

bool
string2oid(const string &v, SmiSubid **oid, size_t *oidlen)
{
  *oidlen = 1;
  for(string::const_iterator p = v.begin(); p != v.end(); ++p) {
    if (*p=='.')
      ++(*oidlen);
  }
  *oid = new SmiSubid[*oidlen];
  SmiSubid n = 0;
  for(size_t i=0, p=0; i<=v.size(); ++i) {
    unsigned char c = v[i];
    if (c!='.' && i!=v.size()) {
      if(c<'0' || c>'9') {
        free(*oid);
        return false;
      }
      n *= 10;
      n+=c-'0';
      continue;
    }
    (*oid)[p++] = n;
    n = 0;
  }
  return true;
}

void
TASN1Encoder::encodeOID(const string &v)
{
  SmiSubid *oid;
  size_t oidlen;
  string2oid(v, &oid, &oidlen);
  encodeOID(oid, oidlen);
}

void
TASN1Encoder::encodeOID(SmiSubid *oid, size_t oidlen)
{
  raw.clear();
  if (oidlen>0) {
    SmiSubid n = oid[0] * 40;
    if (oidlen>1)
      n += oid[1];
    putNumber(&raw, n);
  }
  
  for(size_t i=2; i<oidlen; ++i) {
    putNumber(&raw, oid[i]);
  }
  delete[] oid;

  encode(0, OID, 0);
}

void
TASN1Encoder::down()
{
  stack.push(data);
  data.clear();
}

void
TASN1Encoder::up()
{
  assert(!stack.empty());
  raw = data;
  data = stack.top();
  stack.pop();
  putNumber(&data, raw.size());
  data += raw;
  raw.clear();
}

void
TASN1Encoder::downOctetString()
{
  encodeHead(0, OCTETSTRING, 0);
  down();
}

void
TASN1Encoder::downSequence()
{
  encodeHead(0, SEQUENCE, CONSTRUCTED);
  down();
}

void
TASN1Encoder::downContext(unsigned tag)
{
  encodeHead(CONTEXT, tag, CONSTRUCTED);
  down();
}

void
TASN1Encoder::encodeHead(unsigned tclass, unsigned tag, unsigned encoding)
{
  unsigned char c = (tclass << 6) | (encoding << 5);
  if (tag<0x1F) {
    c |= tag;
    data += c;
  } else {
    c |= 0x1F;
    data += c;
    putNumber(&data, tag);
  }
}

void
TASN1Encoder::encode(unsigned tclass, unsigned tag, unsigned encoding)
{
  encodeHead(tclass, tag, encoding);
  putNumber(&data, raw.size());
  data += raw;
  raw.clear();
}

void
TASN1Encoder::putNumber(string *data, int n)
{
  string r;
  while(n>0x7F) {
    r.insert(0, 1, static_cast<unsigned char>( (n &0x7F)));
    n>>=7;
  }
  r.insert(0, 1, static_cast<unsigned char>(n));
  for(unsigned i=0; i<r.size()-1; ++i) {
    r[i]|=0x80;
  }
  *data+=r;
}

TASN1Decoder::TASN1Decoder(char *data, size_t size)
{
  pos = 0;
  this->data = data;
  this->size = size;
}

bool
TASN1Decoder::down()
{
  if (!raw)
    return false;
  stack.push(str(data, size, pos));
  data = raw;
  size = len;
  pos  = 0;
  raw = 0;
  return true;
}

bool
TASN1Decoder::up()
{
  if (stack.empty())
    return false;
  data = stack.top().data;
  size = stack.top().size;
  pos  = stack.top().pos;
  stack.pop();
  return true;
}

void
TASN1Decoder::print()
{
//cout << "print from " << ((void*)data+pos) << " to " << ((void*)data+pos+size) << endl;
  char *data0 = data;
  size_t size0 = size;
  size_t pos0 = pos;
  
  _print();
  
  data = data0;
  size = size0;
  pos = pos0;
}

void
TASN1Decoder::_print()
{
  int v;
  while(decode()) {
    for(unsigned i=0; i<stack.size(); ++i)
      cout << "  ";
    cout << tagName(tclass, tag) << " (" << len << ") ";
    if (tclass==CONTEXT) {
        cout << "\n";
        down();
        _print();
        up();
        continue;
    }
if (tag==OCTETSTRING&&len>16) {
        cout << "\n";
        down();
        _print();
        up();
continue;
}
    switch(tag) {
      case SEQUENCE:
        cout << "\n";
        down();
        _print();
        up();
        break;
      case INTEGER:
        getInteger(&v);
        cout << v << endl;
        break;
      case OCTETSTRING:
        for(size_t i=0; i<len; ++i) {
          printf("%02x ", raw[i]);
        }
        printf("\n");
        break;
      default:
        printf("\n");
    }
  }
}

bool
TASN1Decoder::decode()
{
  unsigned char c;
  int state = 0;  
  hdrlen=0;
  size_t lenlen;

  while(state>=0) {
    if (pos>=size)
      return false;
    c = data[pos++];
    ++hdrlen;
    switch(state) {
      case 0: // tag
        tclass   = (c & 0xC0) >> 6; // 11000000
        encoding = (c & 0x20) >> 5; // 00100000
        tag      = c & 0x1F;        // 00011111
        if (tag==0x1F) {
          state = 1;
          tag = 0;  
        } else {    
          state = 2;
        }
        break;
      case 1: 
        tag<<=7;
        tag|= c & 0x7F;
        if (c & 0x80)  
          state = 2;   
        break;
      case 2: // length
        len = c;
        if (c & 0x80) {
          lenlen = len & 0x7F;
          len=0;
          state = 3;
        } else {
          state = -1;
        }
        break;
      case 3: 
        len<<=8;
        len|=c; 
        if (--lenlen==0)
          state = -1;   
        break;
    }
  }
  raw = data + pos;
  pos += len;
  if(pos>size)
    return false;
  return true;
}

bool
TASN1Decoder::getInteger(int *value) const
{
  *value = 0;
  for(size_t i=0; i<len; ++i) {
    *value<<=8;
    *value|=raw[i];
  }
  return true;
}

bool
TASN1Decoder::getOID(SmiSubid **oid, size_t *oidlen)
{
  int state = 0;
  unsigned akku = 0;
  vector<SmiSubid> buffer;

  for(int i=0; i<len; ++i) {
    unsigned char c = raw[i];
    switch(state) {
      case 0: 
        // the first byte holds two numbers
        buffer.push_back(c/40);
        buffer.push_back(c % 40);
        state=1;
        break;  
       case 1:  
        if (c&0x80) {
          akku = c & 0x7F;
          state = 2;
        } else {
          buffer.push_back(c);
        }
        break;
      case 2: 
        akku<<=7;
        akku|= (c & 0x7F);
        if (!(c&0x80)) {  
          buffer.push_back(akku);
          state = 1;
         }
         break;
    }
  }  
  *oidlen = buffer.size();
  *oid = new SmiSubid[*oidlen];
  for(unsigned i=0; i<*oidlen; ++i)
    (*oid)[i] = buffer[i];
  return true;
}
