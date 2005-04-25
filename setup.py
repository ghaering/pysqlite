#-*- coding: ISO-8859-1 -*-
# setup.py: the distutils script
#
# Copyright (C) 2004 Gerhard Häring <gh@ghaering.de>
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

import os, sys
from distutils.core import setup
from distutils.extension import Extension

# If you need to change anything, it should be enough to change setup.cfg.

PYSQLITE_VERSION = "2.0.alpha4"

sqlite = "sqlite"

sources = ["src/module.c", "src/connection.c", "src/cursor.c", "src/cache.c",
           "src/microprotocols.c", "src/microprotocols_proto.c",
           "src/prepare_protocol.c", "src/util.c"]

include_dirs = []
library_dirs = []
libraries = []
runtime_library_dirs = []
extra_objects = []
define_macros = []

long_description = \
"""Python interface to SQLite 3.x

pysqlite is an interface to the SQLite 3.x embedded relational database engine.
It will be fully compliant with Python database API version 2.0 while also
exploiting the unique features of SQLite."""

if sys.platform != "win32":
    define_macros.append(('PYSQLITE_VERSION', '"%s"' % PYSQLITE_VERSION))
else:
    define_macros.append(('PYSQLITE_VERSION', '\\"'+PYSQLITE_VERSION+'\\"'))
define_macros.append(('PY_MAJOR_VERSION', str(sys.version_info[0])))
define_macros.append(('PY_MINOR_VERSION', str(sys.version_info[1])))

def main():
    py_modules = ["sqlite"]
    setup ( # Distribution meta-data
            name = "pysqlite",
            version = PYSQLITE_VERSION,
            description = "DB-API 2.0 interface for SQLite 3.x",
            long_description=long_description,
            author = "Gerhard Häring",
            author_email = "gh@ghaering.de",
            license = "zlib/libpng license",
            platforms = "ALL",
            url = "http://pysqlite.org/",

            # Description of the modules and packages in the distribution
            package_dir = {"pysqlite2": "lib"},
            packages = ["pysqlite2", "pysqlite2.test"],
            scripts=["scripts/test-pysqlite"],

            ext_modules = [Extension( name="pysqlite2._sqlite",
                                      sources=sources,
                                      include_dirs=include_dirs,
                                      library_dirs=library_dirs,
                                      runtime_library_dirs=runtime_library_dirs,
                                      libraries=libraries,
                                      extra_objects=extra_objects,
                                      define_macros=define_macros
                                      )],
            classifiers = [
            "Development Status :: 3 - Alpha",
            "Intended Audience :: Developers",
            "License :: OSI Approved :: zlib/libpng License",
            "Operating System :: MacOS :: MacOS X",
            "Operating System :: Microsoft :: Windows",
            "Operating System :: POSIX",
            "Programming Language :: C",
            "Programming Language :: Python",
            "Topic :: Database :: Database Engines/Servers",
            "Topic :: Software Development :: Libraries :: Python Modules"]
            )

if __name__ == "__main__":
    main()
