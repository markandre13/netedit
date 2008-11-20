/**
 * Server which listens for SNMP packets and print's 'em on stdout
 */

#include "der.hh"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
 
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

#include <strstream>

#define IP4ADDR(a,b,c,d) ((a<<24)+(b<<16)+(c<<8)+d)

using namespace netedit;

#if 0
void
server()
{
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
    int n = recvfrom(sock, buffer, 0xFFFF, MSG_TRUNC, (sockaddr*)&remote, &len);  
    printf("got %i bytes from %s\n", n, inet_ntoa(remote.sin_addr) );
    istrstream der(buffer, n);
    TASN1Item * item = TASN1Item::decode(der);
    item->print();
    delete item;
    cout << endl;
  }
  close(sock);
}
#endif

void
snmpQuery(char *host, char *oid, bool next)
{
  int sock = socket (AF_INET, SOCK_DGRAM, 0);
  if (sock<0) {
    perror("socket");
    exit(1);
  }
  
  int yes = 1;
  setsockopt( sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int) );
  
  sockaddr_in name;
  name.sin_family = AF_INET;
  name.sin_addr.s_addr  = htonl(INADDR_ANY);
  name.sin_port         = 0;

  if (bind(sock, (sockaddr*) &name, sizeof(sockaddr_in)) < 0) {
    perror("bind");
  }
  
  printf("sending data\n");
  
  name.sin_addr.s_addr  = htonl(IP4ADDR(127,0,0,1));
//  name.sin_addr.s_addr  = htonl(IP4ADDR(192,168,1,95));
//  name.sin_addr.s_addr  = htonl(IP4ADDR(192,168,1,10));
  name.sin_port         = htons(161);

  unsigned n = 8192;
  unsigned char buffer[n];

  TASN1Item *s1, *s2, *s3, *s4;
  
  /* Build a simple SNMP Command */
  s1 = new TASN1Sequence();                               // Message
  s1->add(new TASN1Integer(0));                           // version
  s1->add(new TASN1OctetString("public"));                // community
  s2 = s1->add(new TASN1Item(TASN1Item::CONTEXT, next?1:0));     // PDU
    s2->add(new TASN1Integer(0));                         // request id
    s2->add(new TASN1Integer(0));                         // error-status
    s2->add(new TASN1Integer(0));                         // error-index
    s3=s2->add(new TASN1Sequence());                      // variables-bindings
      s4=s3->add(new TASN1Sequence());
        s4->add(new TASN1OID(oid));                       // name
        s4->add(new TASN1Null());                         // value
  string out;
  s1->encodeBody();
  out = s1->getHead() + s1->getBody();

  /* Send command */
  if (sendto(sock, 
             out.c_str(), out.size(), 0, 
             (sockaddr*) &name, sizeof(sockaddr_in)) != 7)
  {
    perror("sendto");
  }

  /* wait for incoming data */
  {
  printf("waiting for data\n");
  sockaddr_in remote;
  socklen_t len = sizeof(remote);
  char buffer[0xFFFF];
  int n = recvfrom(sock, buffer, 0xFFFF, MSG_TRUNC, (sockaddr*)&remote, &len);  
  printf("got %i bytes from %s\n", n, inet_ntoa(remote.sin_addr) );
  istrstream der(buffer, n);
  TASN1Item * item = TASN1Item::decode(der);
  item->print();
  cout << endl;
  delete item;
  }

  close(sock);
}

int
main(int argc, char **argv)
{
  char *host = "localhost";
  char *oid  = "1.3.6.1.2.1.1.1";
  bool next = false;

  for(int i=1; i<argc; ++i) {
    if (strcmp(argv[i], "--next")==0) {
      next = true;
    } else
    if (strcmp(argv[i], "--host")==0) {
      ++i;
      host = argv[i];
    } else
    if (strcmp(argv[i], "--oid")==0) {
      ++i;
      oid = argv[i];
    } else {
      printf("unknown option %s\n", argv[i]);
      return EXIT_FAILURE;
    }
  }
  snmpQuery(host, oid, next);
  
  return EXIT_SUCCESS;
}
