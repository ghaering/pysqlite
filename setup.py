#!/usr/bin/env python
# -*- coding: latin1 -*-
# (c) 2004 Gerhard Häring

import os, sys
from distutils.core import setup
from distutils.extension import Extension

__version__ = "1.9.0"

sqlite = "sqlite"
sources = ["src/module.c", "src/connection.c", "src/cursor.c", "src/cache.c"]
macros = []

# Hard-coded for now in this project phase:
include_dirs = ["../sqlite"]
library_dirs = ["../sqlite"]
libraries = ["sqlite3"]
libraries = []
runtime_library_dirs = []
extra_objects = ["/usr/local/lib/libsqlite3.a"]

long_description = \
"""Python interface to SQLite 3.x

pysqlite is an interface to the SQLite 3.x embedded relational database engine.
It will be fully compliant with Python database API version 2.0 while also
exploiting the unique features of SQLite."""

def main():
    py_modules = ["sqlite"]
    setup ( # Distribution meta-data
            name = "pysqlite",
            version = __version__,
            description = "DB-API 2.0 interface for SQLite 3.x",
            long_description=long_description,
            author = "Gerhard Häring",
            author_email = "gh@ghaering.de",
            license = "zlib/libpng license",
            platforms = "ALL",
            url = "http://initd.org/svn/initd/pysqlite/trunk/",

            # Description of the modules and packages in the distribution
            package_dir = {"sqlite": "lib"},
            packages = ["sqlite", "sqlite.test"],
            scripts=["scripts/test-pysqlite"],

            ext_modules = [Extension( name="_sqlite",
                                      sources=sources,
                                      include_dirs=include_dirs,
                                      library_dirs=library_dirs,
                                      runtime_library_dirs=runtime_library_dirs,
                                      libraries=libraries,
                                      extra_objects=extra_objects,
                                      define_macros=macros
                                      )],
            classifiers = [
            "Development Status :: 2 - Pre-Alpha",
            "Intended Audience :: Developers",
            "License :: OSI Approved :: zlib/libpng License",
            "Operating System :: MacOS :: MacOS X",
            "Operating System :: Microsoft :: Windows",
            "Operating System :: POSIX",
            "Programming Language :: C",
            "Programming Language :: Python",
            "Topic :: Database :: Database Engines/Servers"]
            )

if __name__ == "__main__":
    main()
