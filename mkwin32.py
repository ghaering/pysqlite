#!/usr/bin/env python
#
# Cross-compile and build pysqlite installers for win32 on Linux IA32.
#
# The way this works is very ugly, but hey, it *works*!  And I didn't have to
# reinvent the wheel using NSIS.

import os
import urllib
import zipfile

# Cross-compiler
CC = "/usr/bin/i586-mingw32msvc-gcc"

# pysqlite sources + SQLite amalgamation
SRC = "src/module.c src/connection.c src/cursor.c src/cache.c src/microprotocols.c src/prepare_protocol.c src/statement.c src/util.c src/row.c amalgamation/sqlite3.c"

# You will need to fetch these from
# http://oss.itsystementwicklung.de/hg/pyext_cross_linux_to_win32/
CROSS_TOOLS = "../pyext_cross_linux_to_win32"

def get_amalgamation():
    """Download the SQLite amalgamation if it isn't there, already."""
    AMALGAMATION_ROOT = "amalgamation"
    if os.path.exists(AMALGAMATION_ROOT):
        return
    os.mkdir(AMALGAMATION_ROOT)
    print "Downloading amalgation."
    urllib.urlretrieve("http://sqlite.org/sqlite-amalgamation-3_5_7.zip", "tmp.zip")
    zf = zipfile.ZipFile("tmp.zip")
    files = ["sqlite3.c", "sqlite3.h"]
    for fn in files:
        print "Extracting", fn
        outf = open(AMALGAMATION_ROOT + os.sep + fn, "wb")
        outf.write(zf.read(fn))
        outf.close()
    zf.close()
    os.unlink("tmp.zip")

def compile_module(pyver):
    VER = pyver.replace(".", "")
    INC = "%s/python%s/include" % (CROSS_TOOLS, VER)
    vars = locals()
    vars.update(globals())
    cmd = '%(CC)s -mno-cygwin -mdll -DMODULE_NAME=\\"pysqlite2._sqlite\\" -I amalgamation -I %(INC)s -I . %(SRC)s -L %(CROSS_TOOLS)s/python%(VER)s/libs -lpython%(VER)s -o build/lib.linux-i686-2.5/pysqlite2/_sqlite.pyd' % vars
    os.system(cmd)
    os.system("strip --strip-all build/lib.linux-i686-2.5/pysqlite2/_sqlite.pyd")

def main():
    get_amalgamation()
    for ver in ["2.3", "2.4", "2.5"]:
        os.system("rm -rf build")
        # First, compile the Linux version. This is just to get the .py files in place.
        os.system("python2.5 setup.py build")
        # Yes, now delete the Linux extension module. What a waste of time.
        os.unlink("build/lib.linux-i686-2.5/pysqlite2/_sqlite.so")
        # Cross-compile win32 extension module.
        compile_module(ver)
        # Prepare for target Python version.
        os.rename("build/lib.linux-i686-2.5", "build/lib.linux-i686-%s" % ver)
        # And create the installer!
        os.putenv("PYEXT_CROSS", CROSS_TOOLS)
        os.system("python2.5 extended_setup.py cross_bdist_wininst --skip-build --target-version=" + ver)

if __name__ == "__main__":
    main()
