#include <stddef.h>
#include <string>
#include <stack>

#include <smi.h>

class TASN1
{
  public:
    enum EEncoding {
      PRIMITIVE,
      CONSTRUCTED
    };

    enum EClass {
      UNIVERSAL = 0,
      APPLICATION,
      CONTEXT,
      PRIVATE
    };

    enum ETag { 
      EOC = 0,
      BOOLEAN,
      INTEGER,   
      BITSTRING,
      OCTETSTRING, 
      NULLTAG,
      OID,
      OBJDESCRIPTOR,
      EXTERNAL,
      REAL,
      ENUMERATED,
      EMBEDDED_PDV,
      UTF8STRING,   
      SEQUENCE = 16,  
      SET,
      NUMERICSTRING,
      PRINTABLESTRING,
      T61STRING,
      VIDEOTEXSTRING,
      IA5STRING,
      UTCTIME,  
      GENERALIZEDTIME,
      GRAPHICSTRING,
      VISIBLESTRING,
      GENERALSTRING,
      UNIVERSALSTRING,
      BMPSTRING   
    };

    static const char *tagName(unsigned tclass, unsigned tag);
    static const char *className(unsigned tclass);
    static const char *encodingName(unsigned encoding);
};

struct OctetString
{
  char *data;
  size_t size;
};

class TASN1Encoder:
  public TASN1
{
  public:
    std::string data;
  private: 
    std::string raw;
    std::stack<std::string> stack;
    
    void encodeHead(unsigned tclass, unsigned tag, unsigned encoding);
    void encode(unsigned tclass, unsigned tag, unsigned encoding);
    void putNumber(std::string *data, int);
    
  public:
    void up();
    void down();
    void downSequence();
    void downOctetString();
    void downContext(unsigned);
    void encodeNull();
    void encodeInteger(int v);
    void encodeOctetString(const OctetString &str);
    void encodeOctetString(const std::string &s) {
      encodeOctetString((const char*)s.c_str(), s.size());
    }
    void encodeOctetString(const char *data, size_t size);
    void encodeOID(SmiSubid *oid, size_t oidlen);
    void encodeOID(const std::string&);
};

class TASN1Decoder:
  public TASN1
{
public:
    char *data;
    size_t size, pos;
    
    struct str {
      str(char *d, size_t s, size_t p):data(d), size(s), pos(p) {}
      char *data;
      size_t size, pos;
    };
    std::stack<str> stack;
  public:
    TASN1Decoder(char *data, size_t size);

    bool decode();
    bool down();
    bool up();
    void print();
    void _print();

    unsigned tag;
    unsigned tclass;
    unsigned encoding;
    size_t hdrlen;
    
    char *raw;
    size_t len;
    
    bool getInteger(int*) const;
    bool getOctetString(OctetString *os) const {
      os->data = raw;
      os->size = len;
      return true;
    }
    bool getOID(SmiSubid **oid, size_t *oidlen);
    
    bool decodeInteger(int *v) {
      return decode() && tclass==UNIVERSAL && tag==INTEGER && getInteger(v);
    }
    bool decodeOctetString(OctetString *v) {
      return decode() && tclass==UNIVERSAL && tag==OCTETSTRING && getOctetString(v);
    }
    bool decodeOID(SmiSubid **oid, size_t *oidlen) {
      return decode() && tclass==UNIVERSAL && tag==OID && getOID(oid, oidlen);
    }
    bool decodeContext(unsigned *v) {
      if (!decode() && tclass!=CONSTRUCTED)
        return false;
      *v = tag;
      return true;
    }
    bool downSequence() {
      return decode() && 
             tclass==UNIVERSAL && tag==SEQUENCE &&
             down();
    }
};
