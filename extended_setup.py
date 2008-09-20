#-*- coding: ISO-8859-1 -*-
# ext_setup.py: setuptools extensions for the distutils script
#
# Copyright (C) 2004-2007 Gerhard Häring <gh@ghaering.de>
#
# This file is part of pysqlite.
#
# This software is provided 'as-is', without any express or implied
# warranty.  In no event will the authors be held liable for any damages
# arising from the use of this software.
#
# Permission is granted to anyone to use this software for any purpose,
# including commercial applications, and to alter it and redistribute it
# freely, subject to the following restrictions:
#
# 1. The origin of this software must not be misrepresented; you must not
#    claim that you wrote the original software. If you use this software
#    in a product, an acknowledgment in the product documentation would be
#    appreciated but is not required.
# 2. Altered source versions must be plainly marked as such, and must not be
#    misrepresented as being the original software.
# 3. This notice may not be removed or altered from any source distribution.

import glob, os, sys
import cross_bdist_wininst

from ez_setup import use_setuptools
from distutils.command.build import build
from distutils.command.build_ext import build_ext
use_setuptools()

import setuptools
import urllib
import zipfile

from setup import get_setup_args, DocBuilder

AMALGAMATION_ROOT = "amalgamation"

def get_amalgamation():
    """Download the SQLite amalgamation if it isn't there, already."""
    if os.path.exists(AMALGAMATION_ROOT):
        return
    os.mkdir(AMALGAMATION_ROOT)
    print "Downloading amalgation."
    urllib.urlretrieve("http://sqlite.org/sqlite-amalgamation-3_6_1.zip", "tmp.zip")
    zf = zipfile.ZipFile("tmp.zip")
    files = ["sqlite3.c", "sqlite3.h"]
    for fn in files:
        print "Extracting", fn
        outf = open(AMALGAMATION_ROOT + os.sep + fn, "wb")
        outf.write(zf.read(fn))
        outf.close()
    zf.close()
    os.unlink("tmp.zip")

class AmalgamationBuilder(build):
    description = "Build a statically built pysqlite using the amalgamtion."

    def __init__(self, *args, **kwargs):
        MyBuildExt.amalgamation = True
        build.__init__(self, *args, **kwargs)

class MyBuildExt(build_ext):
    amalgamation = False

    def build_extension(self, ext):
        get_amalgamation()
        if self.amalgamation:
            ext.sources.append(os.path.join(AMALGAMATION_ROOT, "sqlite3.c"))
            build_ext.build_extension(self, ext)

    def __setattr__(self, k, v):
        # Make sure we don't link against the SQLite library, no matter what setup.cfg says
        if self.amalgamation and k == "libraries":
            v = None
        self.__dict__[k] = v


def main():
    setup_args = get_setup_args()
    setup_args["test_suite"] = "pysqlite2.test.suite"
    setup_args["cmdclass"].update({"build_docs": DocBuilder, "build_ext": MyBuildExt, "build_static": AmalgamationBuilder, "cross_bdist_wininst": cross_bdist_wininst.bdist_wininst})
    setuptools.setup(**setup_args)

if __name__ == "__main__":
    main()
