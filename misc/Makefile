#####################################################################
# 
# Author: Hans Oesterholt-Dijkema
# 
# $Id$
#
# Makefile for SQLITE for use with GNU Make (MinGW/MSYS) 
# and the MSVC2003 free toolkit. Expects the MSVC Free SDK
# installed along with the MSVC2003 free toolkit.
#  
# Expects $INCLUDE, $LIB, $PATH set right for use with CL.EXE in
# MSYS. NEEDS MSYS for clean: and install:, for making only: 
# Works also in the Visual C++ Toolkit 2003 Command Prompt,
# provided %INCLUDE%, %LIB%, %PATH% are set accordingly.
#
#####################################################################

CL=cl
CLFLAGS=-O2 -Og -G7
LINK=link
PREFIX=$$VCTOOLKITINSTALLDIR
INCINST=$(PREFIX)/include
LIBINST=$(PREFIX)/lib
BININST=$(PREFIX)/bin

SQLITE_OBJ = alter.o analyze.o attach.o auth.o btree.o build.o \
         callback.o complete.o date.o \
         delete.o expr.o func.o hash.o insert.o \
         main.o opcodes.o os_unix.o os_win.o \
         pager.o parse.o pragma.o prepare.o printf.o random.o \
         select.o table.o tokenize.o trigger.o update.o \
         util.o vacuum.o \
         vdbe.o vdbeapi.o vdbeaux.o vdbefifo.o vdbemem.o \
         where.o utf.o legacy.o
SQLITE_PRG_OBJ=shell.o 

all: sqlite3.lib
	@echo "done"
 
install:
	cp sqlite3.exe $(BININST)
	cp sqlite3.lib sqlite3.dll sqlite3.exp sqlite3.def $(LIBINST)
	cp sqlite3.h $(INCINST)

clean:
	rm -f *.dll *.lib *.exp *.exe *.o

sqlite3.exe: sqlite3.dll $(SQLITE_PRG_OBJ)
	$(LINK) -OUT:sqlite3.exe $(SQLITE_PRG_OBJ) sqlite3.lib

sqlite3.lib: $(SQLITE_OBJ)
	$(LINK) -LIB -OUT:sqlite3.lib $(SQLITE_OBJ)

sqlite3.dll: $(SQLITE_OBJ)
	$(LINK) -OUT:sqlite3.dll -dll -def:sqlite3.def $(SQLITE_OBJ)




attach.o: attach.c
	$(CL) -c $(CLFLAGS) -Fo$@ $<

auth.o: auth.c
	$(CL) -c $(CLFLAGS) -Fo$@ $<

btree.o: btree.c
	$(CL) -c $(CLFLAGS) -Fo$@ $<

build.o: build.c
	$(CL) -c $(CLFLAGS) -Fo$@ $<

date.o: date.c
	$(CL) -c $(CLFLAGS) -Fo$@ $<

delete.o: delete.c
	$(CL) -c $(CLFLAGS) -Fo$@ $<

expr.o: expr.c
	$(CL) -c $(CLFLAGS) -Fo$@ $<

func.o: func.c
	$(CL) -c $(CLFLAGS) -Fo$@ $<

hash.o: hash.c
	$(CL) -c $(CLFLAGS) -Fo$@ $<

insert.o: insert.c
	$(CL) -c $(CLFLAGS) -Fo$@ $<

legacy.o: legacy.c
	$(CL) -c $(CLFLAGS) -Fo$@ $<

main.o: main.c
	$(CL) -c $(CLFLAGS) -Fo$@ $<

opcodes.o: opcodes.c
	$(CL) -c $(CLFLAGS) -Fo$@ $<

os_mac.o: os_mac.c
	$(CL) -c $(CLFLAGS) -Fo$@ $<

os_unix.o: os_unix.c
	$(CL) -c $(CLFLAGS) -Fo$@ $<

os_win.o: os_win.c
	$(CL) -c $(CLFLAGS) -Fo$@ $<

pager.o: pager.c
	$(CL) -c $(CLFLAGS) -Fo$@ $<

parse.o: parse.c
	$(CL) -c $(CLFLAGS) -Fo$@ $<

pragma.o: pragma.c
	$(CL) -c $(CLFLAGS) -Fo$@ $<

printf.o: printf.c
	$(CL) -c $(CLFLAGS) -Fo$@ $<

random.o: random.c
	$(CL) -c $(CLFLAGS) -Fo$@ $<

select.o: select.c
	$(CL) -c $(CLFLAGS) -Fo$@ $<

shell.o: shell.c
	$(CL) -c $(CLFLAGS) -Fo$@ $<

table.o: table.c
	$(CL) -c $(CLFLAGS) -Fo$@ $<

tclsqlite.o: tclsqlite.c
	$(CL) -c $(CLFLAGS) -Fo$@ $<

tokenize.o: tokenize.c
	$(CL) -c $(CLFLAGS) -Fo$@ $<

trigger.o: trigger.c
	$(CL) -c $(CLFLAGS) -Fo$@ $<

update.o: update.c
	$(CL) -c $(CLFLAGS) -Fo$@ $<

utf.o: utf.c
	$(CL) -c $(CLFLAGS) -Fo$@ $<

util.o: util.c
	$(CL) -c $(CLFLAGS) -Fo$@ $<

vacuum.o: vacuum.c
	$(CL) -c $(CLFLAGS) -Fo$@ $<

vdbe.o: vdbe.c
	$(CL) -c $(CLFLAGS) -Fo$@ $<

vdbeapi.o: vdbeapi.c
	$(CL) -c $(CLFLAGS) -Fo$@ $<

vdbeaux.o: vdbeaux.c
	$(CL) -c $(CLFLAGS) -Fo$@ $<

vdbemem.o: vdbemem.c
	$(CL) -c $(CLFLAGS) -Fo$@ $<

where.o: where.c
	$(CL) -c $(CLFLAGS) -Fo$@ $<

