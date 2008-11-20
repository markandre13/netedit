#
# NetEdit Daemon
# Copyright (C) 2005 by Mark-André Hopf <mhopf@mark13.org>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307,  USA
#
#-----------------------------------------------------------------------------

PRGFILE		= neteditd
SRCS		= main.cc map.cc

OBJS            = $(SRCS:.cc=.o)

# values from the configure script
#^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
top_builddir=
prefix=/usr/local
exec_prefix=/usr/local
libdir=${exec_prefix}/lib
bindir=${exec_prefix}/bin
includedir=${prefix}/include
datadir=${prefix}/share
INSTALL=install

CXX     = g++
CXXFLAGS=  -g -Wall -I/usr/local/pgsql/include
LD      = $(CXX)

X=$(shell uname)
ifeq ($X, Darwin)
CXXFLAGS+=-DDARWIN
endif

LIBS    = -L/usr/local/pgsql/lib -lecpg -lpq

SHELL   = /bin/sh


#---------------------------------------------------------------------------
# Compiling
#---------------------------------------------------------------------------
.SUFFIXES: .cc

.cc.o:
	@echo compiling $*.cc ...
	ecpg -o $*.cxx $*.cc
	$(CXX) $(CXXFLAGS) $*.cxx -c -o $*.o

#---------------------------------------------------------------------------
# Linking
#---------------------------------------------------------------------------
$(PRGFILE): $(OBJS)
	@echo linking $(PRGFILE) ...
	@$(LD) $(LIBS) $(OBJS) -o $(PRGFILE)
	@echo Ok

# X11R6 makedepend has the `-Y' option
dep:
	@/usr/X11R6/bin/makedepend $(INCDIRS) -Y $(SRCS) 2> /dev/null

valgrind:
	valgrind --tool=addrcheck --num-callers=20 ./$(PRGFILE)

#---------------------------------------------------------------------------
# Clean
#---------------------------------------------------------------------------
clean::
	@echo removing generated files in `pwd`
	@rm -f $(PRGFILE) .depend $(CLEAN)
	@find . -name "*.o" -exec rm -f {} \;
	@find . -name "*.bak" -exec rm -f {} \;
	@find . -name "*~" -exec rm -f {} \;   
	@find . -name "core" -exec rm -f {} \; 
	@find . -name "DEADJOE" -exec rm -f {} \;
	@find . -name "a.out" -exec rm -f {} \;  
	@rm -f output.ps

distclean: clean
	rm -rf config.status config.log config.guess config.sub configure
	rm -rf autom4te.cache *~ config.log
	rm -rf Makefile
