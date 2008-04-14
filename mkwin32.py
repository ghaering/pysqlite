#!/usr/bin/env python
#
# The way this works is very ugly, but hey, it *works*!
# And I didn't have to reinvent the wheel using NSIS.
import os

CC = "/usr/bin/i586-mingw32msvc-gcc"
SRC = "src/module.c src/connection.c src/cursor.c src/cache.c src/microprotocols.c src/prepare_protocol.c src/statement.c src/util.c src/row.c amalgamation/sqlite3.c"
CROSS_TOOLS = "../pyext_cross_linux_to_win32"

def compile_module(pyver):
    VER = pyver.replace(".", "")
    INC = "%s/python%s/include" % (CROSS_TOOLS, VER)
    vars = locals()
    vars.update(globals())
    cmd = '%(CC)s -mno-cygwin -mdll -DMODULE_NAME=\\"pysqlite2._sqlite\\" -I amalgamation -I %(INC)s -I . %(SRC)s -L %(CROSS_TOOLS)s/python%(VER)s/libs -lpython%(VER)s -o build/lib.linux-i686-2.5/pysqlite2/_sqlite.pyd' % vars
    print cmd
    os.system(cmd)
    os.system("strip --strip-all build/lib.linux-i686-2.5/pysqlite2/_sqlite.pyd")

for ver in ["2.3", "2.4", "2.5"]:
    os.system("rm -rf build")
    os.system("python2.5 setup.py build")
    os.unlink("build/lib.linux-i686-2.5/pysqlite2/_sqlite.so")
    compile_module(ver)
    os.rename("build/lib.linux-i686-2.5", "build/lib.linux-i686-%s" % ver)
    os.putenv("PYEXT_CROSS", CROSS_TOOLS)
    os.system("python2.5 extended_setup.py cross_bdist_wininst --skip-build --target-version=" + ver)

