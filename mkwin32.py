import os

CC = "/usr/bin/i586-mingw32msvc-gcc"
SRC = "src/module.c src/connection.c src/cursor.c src/cache.c src/microprotocols.c src/prepare_protocol.c src/statement.c src/util.c src/row.c amalgamation/sqlite3.c"
CROSS_TOOLS = "../pyext_cross_linux_to_win32"

def compile_module(pyver):
    VER = pyver.replace(".", "")
    INC = "%s/python%s/include" % (CROSS_TOOLS, VER)
    vars = locals()
    vars.update(globals())
    cmd = '%(CC)s -mno-cygwin -mdll -DMODULE_NAME=\\"pysqlite2._sqlite\\" -I amalgamation -I %(INC)s -I . %(SRC)s -L %(CROSS_TOOLS)s/python%(VER)s/libs -lpython%(VER)s -o _sqlite.pyd' % vars
    print cmd
    os.system(cmd)
    # strip --strip-all _sqlite.pyd

compile_module("2.5")
compile_module("2.4")
compile_module("2.3")


