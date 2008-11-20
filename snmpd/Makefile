#
# NetEdit II
#

CC=gcc
CXX=g++
CXXFLAGS=-g
LIBS=-lsmi

all: snmpd

valgrind:
	valgrind --tool=addrcheck --num-callers=30 ./netedit
  
SRC=snmpd.cc asn1.cc md5.cc des.cc

OBJ=$(SRC:.cc=.o)

snmpd: $(OBJ)
	$(CXX) $(LIBS) $(OBJ) -o snmpd

depend:
	makedepend $(INCDIRS) -Y $(SRC) 2> /dev/null

clean:
	rm -f *.o
	rm -f snmpd
	rm -f *~ DEADJOE

.SUFFIXES: .cc

.cc.o:
	@echo compiling $*.cc ...
	$(CXX) $(CXXFLAGS) $*.cc -c -o $*.o

# DO NOT DELETE

netedit.o: mapmodel.hh server.hh symbol.hh nodeeditor.hh ../lib/common.hh
netedit.o: browser.hh snmp.hh oidnode.hh
browser.o: browser.hh symbol.hh mapmodel.hh server.hh
server.o: server.hh symbol.hh mapmodel.hh nodeeditor.hh ../lib/common.hh
server.o: ../lib/binary.hh
symbol.o: netedit.hh mapmodel.hh server.hh symbol.hh snmpdialog.hh snmp.hh
symbol.o: oidnode.hh browser.hh
connection.o: symbol.hh mapmodel.hh server.hh
snmpdialog.o: netedit.hh snmpdialog.hh snmp.hh oidnode.hh
nodeeditor.o: netedit.hh nodeeditor.hh ../lib/common.hh server.hh symbol.hh
mapmodel.o: mapmodel.hh server.hh symbol.hh
snmp.o: snmp.hh oidnode.hh der.hh
asn1der.o: der.hh
oidnode.o: oidnode.hh
