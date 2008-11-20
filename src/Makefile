#
# NetEdit II
#

CC=`toad-config --cxx` -g
CXX=`toad-config --cxx` -g -Wno-deprecated
CXXFLAGS=`toad-config --cxxflags` -Wall
LIBS=`toad-config --libs` -lsmi
LEX=flex
YACC=bison -v -v

all: netedit

valgrind:
	valgrind --tool=addrcheck --num-callers=30 ./netedit
  

SRC=netedit.cc browser.cc server.cc symbol.cc connection.cc \
    snmpdialog.cc nodeeditor.cc \
    mapmodel.cc servermodel.cc \
    snmp.cc oidnode.cc \
    ../snmpd/asn1.cc

OBJ=$(SRC:.cc=.o)

#netedit: netedit.o asn1der.o
#	$(CXX) `toad-config --libs` -o netedit netedit.o asn1der.o
#
#netedit.o: netedit.cc
#	$(CXX) `toad-config --cxxflags` -c -o netedit.o netedit.cc
#
#snmpd: snmpd.o asn1der.o

netedit: $(OBJ)
	$(CXX) $(LIBS) $(OBJ) -o netedit

depend:
	makedepend $(INCDIRS) -Y $(SRC) 2> /dev/null

clean:
	rm -f *.o
	rm -f snmpd netedit
	rm -f *~ DEADJOE

.SUFFIXES: .cc

.cc.o:
	@echo compiling $*.cc ...
	$(CXX) $(CXXFLAGS) $*.cc -c -o $*.o

# asn1parser:
	
# DO NOT DELETE

netedit.o: mapmodel.hh server.hh symbol.hh nodeeditor.hh ../lib/common.hh
netedit.o: browser.hh snmp.hh oidnode.hh snmpdialog.hh
browser.o: browser.hh symbol.hh mapmodel.hh server.hh
server.o: server.hh symbol.hh mapmodel.hh nodeeditor.hh ../lib/common.hh
server.o: ../lib/binary.hh
symbol.o: netedit.hh mapmodel.hh server.hh symbol.hh snmpdialog.hh snmp.hh
symbol.o: oidnode.hh browser.hh
connection.o: symbol.hh mapmodel.hh server.hh
snmpdialog.o: netedit.hh snmpdialog.hh snmp.hh oidnode.hh
nodeeditor.o: netedit.hh nodeeditor.hh ../lib/common.hh server.hh symbol.hh
mapmodel.o: mapmodel.hh server.hh symbol.hh
snmp.o: ../snmpd/asn1.hh snmp.hh oidnode.hh
oidnode.o: oidnode.hh
../snmpd/asn1.o: ../snmpd/asn1.hh
