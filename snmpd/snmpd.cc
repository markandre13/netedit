/**
 * snmp.mark13.org -- a SNMP implementation for embedded devices
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
 
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <iostream>
#include <string>
#include <stack>

#include "global.h"
#include "asn1.hh"
#include "md5.h"
#include "des.hh"

using namespace std;

unsigned char engineID[9] = {
   0x80, 0x00, 0x00, 0x02, // enterprise id with msb set to 1
   0x01,                   // next will be IPv4 address
   0x0a, 0x00, 0x00, 0x03  // the address
};



void
hexdump(unsigned char *ptr, size_t n)
{
  for(size_t i=0; i<n; ++i)
    printf("%02x ", ptr[i]);
  printf("\n");
}

void
HMAC_MD5(char *key, size_t keylen, char *data, size_t datalen, unsigned char digest[16])
{
  unsigned char extendedAuthKey[64];
  memset(extendedAuthKey, 0, 64);
  memcpy(extendedAuthKey, key, keylen < 64 ? keylen : 64);

  unsigned char KxorIPad[64];
  for(unsigned i=0; i<64; ++i)
    KxorIPad[i] = extendedAuthKey[i] ^ 0x36;

  unsigned char KxorOPad[64];
  for(unsigned i=0; i<64; ++i)
    KxorOPad[i] = extendedAuthKey[i] ^ 0x5C;
            
  MD5_CTX md5ctx;
  MD5Init(&md5ctx);
  MD5Update(&md5ctx, KxorIPad, 64);
  MD5Update(&md5ctx, data, datalen);
  MD5Final(digest, &md5ctx);
            
  MD5Init(&md5ctx);
  MD5Update(&md5ctx, KxorOPad, 64);
  MD5Update(&md5ctx, digest, 16);
  MD5Final(digest, &md5ctx);
}

void password_to_key_md5(
   unsigned char *password,    /* IN */
   unsigned int   passwordlen, /* IN */
   unsigned char *engineID,    /* IN  - pointer to snmpEngineID  */
   unsigned int   engineLength,/* IN  - length of snmpEngineID */  
   unsigned char *key)         /* OUT - pointer to caller 16-octet buffer */
{
   MD5_CTX     MD;
   u_char     *cp, password_buf[64];
   u_long      password_index = 0;  
   u_long      count = 0, i;

   MD5Init (&MD);   /* initialize MD5 */

   /**********************************************/
   /* Use while loop until we've done 1 Megabyte */
   /**********************************************/
   while (count < 1048576) {
      cp = password_buf;
      for (i = 0; i < 64; i++) {
          /*************************************************/
          /* Take the next octet of the password, wrapping */
          /* to the beginning of the password as necessary.*/
          /*************************************************/
          *cp++ = password[password_index++ % passwordlen];  
      }
      MD5Update (&MD, password_buf, 64);
      count += 64;
   }
   MD5Final (key, &MD);          /* tell MD5 we're done */

   /*****************************************************/
   /* Now localize the key with the engineID and pass   */
   /* through MD5 to produce final key                  */
   /* May want to ensure that engineLength <= 32,       */
   /* otherwise need to use a buffer larger than 64     */
   /*****************************************************/
   memcpy(password_buf, key, 16);
   memcpy(password_buf+16, engineID, engineLength);
   memcpy(password_buf+16+engineLength, key, 16);  

   MD5Init(&MD);
   MD5Update(&MD, password_buf, 32+engineLength);
   MD5Final(key, &MD);
   return;
}

void
handleIncoming(unsigned char *buffer, size_t n)
{
  TASN1Decoder in(buffer, n);
//  in.print();

  if (in.decode() && in.tag==TASN1::SEQUENCE) {
    int version;
    if (in.down() && in.decodeInteger(&version)) {
      cout << "\nSNMP Version " << version << endl;
      switch(version) {
        case 3: { // see RFC 2572
          int msgID;
          int msgMaxSize;
          OctetString msgFlags;
          int msgSecurityModel;
          
          if (!in.decode() || in.tag!=TASN1::SEQUENCE || !in.down()   || 
              !in.decodeInteger(&msgID) ||
              !in.decodeInteger(&msgMaxSize) ||
              !in.decodeOctetString(&msgFlags) ||
              !in.decodeInteger(&msgSecurityModel) ) break;
          in.up();
          
          // 0:any, 1:SNMPv1, 2:SNMPv2, 3:User-Based Security Model (USM) (see 2571)
          if (msgSecurityModel != 3) {
            cout << "SNMPv3 security model " << msgSecurityModel << "isn't handled yet.\n";
            break;
          }
          
          // RFC 2574: 2.4: msgSecurityParameters contains UsmSecurityParameters
          OctetString msgAuthoritativeEngineID;
          int msgAuthoritativeEngineBoots;
          int msgAuthoritativeEngineTime;
          OctetString msgUserName;
          OctetString msgAuthenticationParameters;
          OctetString msgPrivacyParameters;
          
          if (!(in.decode() && in.tag==TASN1::OCTETSTRING && in.down() ))
            break;
          cout << "UsmSecurityParameters:" << endl;
          in.print();

          if (!(in.decode() && in.tag==TASN1::SEQUENCE    && in.down() &&
                in.decodeOctetString(&msgAuthoritativeEngineID) &&
                in.decodeInteger(&msgAuthoritativeEngineBoots) &&
                in.decodeInteger(&msgAuthoritativeEngineTime) &&
                in.decodeOctetString(&msgUserName) &&
                in.decodeOctetString(&msgAuthenticationParameters) &&
                in.decodeOctetString(&msgPrivacyParameters) )) break;

          cout << "msgAuthoritativeEngineID: ";
          hexdump(msgAuthoritativeEngineID.data, msgAuthoritativeEngineID.size);

          cout << "msgUserName: '" << string((char*)msgUserName.data, msgUserName.size) << "'\n";

          // RFC 2574: 6.3.2.  Processing an Incoming Message
          // 1) fail if msgAuthenticationParameters field is not 12 octets long
          if (msgAuthenticationParameters.size!=12) {
            cout << "msgAuthenticationParameters isn't 12 octets\n";
            break;
          }
          // 2) save MAC received in the msgAuthenticationParameters field 
          unsigned char MAC[12];
          memcpy(MAC, msgAuthenticationParameters.data, 12);
          // 3) msgAuthenticationParameters field is replaced by 12 zero octets
          memset(msgAuthenticationParameters.data, 0, 12);
          
          unsigned char mac[16];


          unsigned char key[16];
          char ei[] = "\0\0";
          password_to_key_md5("foorulez", 8, ei, 2, key);

          HMAC_MD5(key, 16, buffer, n, mac);
          if (memcmp(MAC, mac, 12)!=0) {
            cerr << "authentication failed\n" << endl;
            break;
          }

          in.up();
          in.up();
          
          // 8. CBC-DES Symmetric Encryption Protocol
          // 8.3.2.  Processing an Incoming Message
          OctetString scopedPDU;
          if (!in.decodeOctetString(&scopedPDU)) {
            cout << "failed to decode octet string" << endl;
            break;
          }
          
          password_to_key_md5("barrulez", 8, ei, 2, key);
          
          if (scopedPDU.size & 7) {
            cout << "scopedPDU isn't a multiple of 8 in size" << endl;
            break;
          }
          if (msgPrivacyParameters.size!=8) {
            cout << "msgPrivacyParameters.size!=8" << endl;
            break;
          }
          
          des_ctx dc;
          unsigned char iv[8];
          unsigned char deskey[8];
          
          // 8.1.1.1.  DES key and Initialization Vector
          
          // key -> cryptKey
          // msgPrivacyParameters -> salt
          memcpy(deskey, key, 8);
          memcpy(iv, key+8, 8);
          for(int i=0; i<8; ++i)
            iv[i] ^= msgPrivacyParameters.data[i];

          des_key(&dc, deskey);
          cbc_des_dec(&dc, iv, scopedPDU.data, scopedPDU.size / 8);
          
          cout << "decrypted message:" << endl;
          TASN1Decoder(scopedPDU.data, scopedPDU.size).print();
          cout << endl;
          
          cout << "ok\n";
        } break;
      }
    }
  }
}

void
send(int sock, sockaddr_in *remote, socklen_t len)
{
  unsigned char u[] = {
  // 1   2     3      4     5     6     7     8    9     10     11   12    13    14    15    16
/*  0x45, 0x00, 0x00, 0x89, 0x17, 0x27, 0x00, 0x00, 0x40, 0x11, 0x65, 0x3b, 0x7f, 0x00, 0x00, 0x01,
  //17   18   19    20     21    22    23    24    25    26    27    28
  0x7f, 0x00, 0x00, 0x01, 0x00, 0xa1, 0xc3, 0x25, 0x00, 0x75, 0x70, 0xb8,*/ 0x30, 0x6b, 0x02, 0x01,
  0x03, 0x30, 0x11, 0x02, 0x04, 0x01, 0xce, 0x2a, 0x7b, 0x02, 0x03, 0x00, 0xff, 0xe3, 0x04, 0x01,
  0x00, 0x02, 0x01, 0x03, 0x04, 0x1f, 0x30, 0x1d, 0x04, 0x0d, 0x80, 0x00, 0x1f, 0x88, 0x04, 0x31,
  0x30, 0x2e, 0x30, 0x2e, 0x30, 0x2e, 0x33, 0x02, 0x01, 0x05, 0x02, 0x03, 0x01, 0x67, 0x4e, 0x04,
  0x00, 0x04, 0x00, 0x04, 0x00, 0x30, 0x32, 0x04, 0x0d, 0x80, 0x00, 0x1f, 0x88, 0x04, 0x31, 0x30,
  0x2e, 0x30, 0x2e, 0x30, 0x2e, 0x33, 0x04, 0x00, 0xa8, 0x1f, 0x02, 0x04, 0x71, 0xe9, 0xab, 0xcd,
  0x02, 0x01, 0x00, 0x02, 0x01, 0x00, 0x30, 0x11, 0x30, 0x0f, 0x06, 0x0a, 0x2b, 0x06, 0x01, 0x06,
  0x03, 0x0f, 0x01, 0x01, 0x04, 0x00, 0x41, 0x01, 0x11 };

  TASN1Decoder in(u, sizeof(u));
  cout << "send..." << endl;
  in.print();
  
  in.decode();
  in.down();
  in.decode(); in.decode(); in.decode(); 
  in.down();
  cout << "......\n";
  in.print();
  
  sendto(
    sock,
    u, sizeof(u),
    0,
    (sockaddr*)remote, len
  );
}

bool
send2()
{
  string hostname = "127.0.0.1";

  int sock = socket(AF_INET, SOCK_DGRAM, 0);
  if (sock<0) {
    perror("socket");
    sock = -1;
    return false; 
  }

  int yes = 1;
  setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
  
  sockaddr_in name;
  name.sin_family = AF_INET;
  name.sin_addr.s_addr = htonl(INADDR_ANY);
  name.sin_port        = 0;

  if (bind(sock, (sockaddr*) &name, sizeof(sockaddr_in)) < 0) {
    perror("bind");
    ::close(sock); 
    sock = -1;
    return false;
  }
   
  struct hostent *hostinfo;
  hostinfo = gethostbyname(hostname.c_str());
  if (hostinfo==0) {
    cerr << "couldn't resolve hostname '" << hostname << "'\n";
    ::close(sock);
    sock = -1;
    return false;
  }
  name.sin_addr = *(struct in_addr *) hostinfo->h_addr;
  name.sin_port = htons(161);


  int msgID = 563830694;
  unsigned requestID = 1015346394;
  int msgMaxSize = 65507;

  // Erfragen der engineID des DHCP Servers:

  TASN1Encoder out;
  out.downSequence();
    out.encodeInteger(3); // version
    out.downSequence();
      out.encodeInteger(msgID);
      out.encodeInteger(msgMaxSize);
      unsigned char str[1];
      str[0] = 0x04; // msgFlags, no encryption & authentication for now
      out.encodeOctetString(str, 1);
      out.encodeInteger(3);            // msgSecurityModel
    out.up();
    out.downOctetString();
      out.downSequence();
        // out.encodeOctetString(engineID, sizeof(engineID));
        out.encodeOctetString(0, 0);
        out.encodeInteger(0);
        out.encodeInteger(0);
        out.encodeOctetString(0, 0);
        out.encodeOctetString(0, 0);
        out.encodeOctetString(0, 0);
      out.up();
    out.up();
    out.downSequence();
      out.encodeOctetString(0, 0);
      out.encodeOctetString(0, 0);
      out.downContext(0);
        out.encodeInteger(requestID);
        out.encodeInteger(0);
        out.encodeInteger(0);
        out.downSequence();
        out.up();
      out.up();
    out.up();
  out.up();

  ++requestID;
  ++msgID;
  
  TASN1Decoder in(out.data.c_str(), out.data.size());
  in.print();

  hexdump(out.data.c_str(), out.data.size());

#if 1
unsigned char orig[] = {
//  30    3d    02    01    03    30    10    02    04    21    9b    5f
  0x30, 0x3e ,0x02, 0x01, 0x03, 0x30 ,0x11, 0x02 ,0x04, 0x21 ,0x9b, 0x5f,
//  a6    02    02          ff    e3    04    01    04    02    01    03
  0xa6, 0x02 ,0x03, 0x00 ,0xff, 0xe3 ,0x04, 0x01, 0x04, 0x02 ,0x01, 0x03,
  0x04, 0x10 ,0x30, 0x0e ,0x04, 0x00 ,0x02, 0x01 ,0x00, 0x02 ,0x01, 0x00,
  0x04, 0x00 ,0x04, 0x00 ,0x04, 0x00 ,0x30, 0x14 ,0x04, 0x00 ,0x04, 0x00,
  0xa0, 0x0e ,0x02, 0x04, 0x3c, 0x84 ,0xf4, 0xda ,0x02, 0x01 ,0x00, 0x02,
  0x01, 0x00 ,0x30, 0x00
};

TASN1Decoder iorig(orig, sizeof(orig));
iorig.print();
#endif
// exit(0);
  if (sendto(sock,
      out.data.c_str(), out.data.size(),
      0,
      (sockaddr*)&name, sizeof(name)) != (int)out.data.size())
  {
    perror("sendto");
  }
#if 1
  // fetch data from socket
  sockaddr_in remote;
  socklen_t len = sizeof(remote);
  char buffer[0xFFFF];
  int n = recvfrom(sock, buffer, 0xFFFF, MSG_TRUNC, (sockaddr*)&remote, &len);
  printf("got %i bytes from %s\n", n, inet_ntoa(remote.sin_addr) );

  TASN1Decoder(buffer, n).print();
  
  handleIncoming(buffer, n);
  
#endif
  return true;
}

/*
  Das folgende ist die Rückantwort, wie sie NetSNMP an die client sendet,
  wenn dieser eine leere UsmSecurityParameters schickt:

  TASN1Encoder out;
  out.downSequence();
    out.encodeInteger(3); // version
    out.downSequence();
      out.encodeInteger(msgID);
      out.encodeInteger(msgMaxSize);
      unsigned char str[1] = { 0x04 }; // msgFlags
      out.encodeOctetString(str, 1);
      out.encodeInteger(3);            // msgSecurityModel
    out.up();
    out.downOctetString(); // UsmSecurityParameters
      out.encodeOctetString(engineID, sizeof(engineID)); // msgAuthoritativeEngineID
      out.encodeInteger(5);         // msgAuthoritativeEngineBoots
      out.encodeInteger(91982);     // msgAuthoritativeEngineTime
      out.encodeOctetString(0, 0);  // msgUserName
      out.encodeOctetString(0, 0);  // msgAuthenticationParameters
      out.encodeOctetString(0, 0);  // msgPrivacyParameters
    out.up();
    out.downSequence();                                   // scopedPDU (RFC 2572, 6.8. Scoped PDU)
      out.encodeOctetString(engineID, sizeof(engineID)); // contextEngineID
      out.encodeOctetString(0, 0);                       // contextName
      out.downContext(8); // PDUs
        out.encodeInteger(requestID); // request ID
        out.encodeInteger(0);          // error-status
        out.encodeInteger(0);          // error-index
        out.downSequence();
          out.downSequence();
            out.encodeOID(...);        // name
            out.encodeApplication(1);  // value
          out.up();
        out.up();
      out.up();
    out.up();
  out.up();
*/

int
main()
{
//  send2();
//  send(0,0,0);
//  return;
/*
  unsigned char mac[16];
  HMAC_MD5("Jefe", 4, "what do ya want for nothing?", 28, mac);
  hexdump(mac, 16);
  exit(1);
*/

  char buffer[0xFFFF];
  int sock = socket (AF_INET, SOCK_DGRAM, 0);
  if (sock<0) {
    perror("socket");
    exit(1);
  }
  
  int yes = 1;
  setsockopt( sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int) );
  
  sockaddr_in local;
  local.sin_family = AF_INET;
  // local.sin_addr.s_addr  = htonl(IP4ADDR(127,0,0,1));
  local.sin_addr.s_addr  = htonl(INADDR_ANY);
  local.sin_port         = htons(4711);
  if (bind(sock, (sockaddr*) &local, sizeof(sockaddr_in)) < 0) {
    perror("bind");
    exit(1);
  }
  
  while(1) {
    printf("waiting for data\n");
    sockaddr_in remote;
    socklen_t len = sizeof(remote);
    ssize_t n = recvfrom(sock, buffer, 0xFFFF, MSG_TRUNC, (sockaddr*)&remote, &len);  
    printf("got %i bytes from %s\n", n, inet_ntoa(remote.sin_addr) );

    handleIncoming(buffer, n);

  }
  close(sock);
}
