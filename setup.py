#-*- coding: ISO-8859-1 -*-
# setup.py: the distutils script
#
# Copyright (C) 2008 Gerhard Häring <gh@ghaering.de>
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

import glob, os, re, sys

from distutils.core import setup, Extension, Command

# If you need to change anything, it should be enough to change setup.cfg.

long_description = \
"""Python interface to SQLite 3

pysqlite is an interface to the SQLite 3.x embedded relational database engine.
It is almost fully compliant with the Python database API version 2.0."""

def get_setup_args():
    from pysqlite2.dbapi2 import version, version_info
    PYSQLITE_VERSION = version
    PYSQLITE_MAJOR_VERSION = version_info[0]
    PYSQLITE_MINOR_VERSION = version_info[1]

    py_modules = ["sqlite"]
    setup_args = dict(
            name = "pysqlite",
            version = version,
            description = "DB-API 2.0 interface for SQLite 3.x",
            long_description=long_description,
            author = "Gerhard Haering",
            author_email = "gh@ghaering.de",
            license = "zlib/libpng license",
            platforms = "ALL",
            url = "http://pysqlite.org/",
            download_url = "http://initd.org/pub/software/pysqlite/releases/%s/%s/" % \
                (PYSQLITE_MINOR_VERSION, PYSQLITE_VERSION),


            # Description of the modules and packages in the distribution
            package_dir = {"pysqlite2": "pysqlite2"},
            packages = ["pysqlite2", "pysqlite2.test"],
            scripts=[],
            data_files = [],

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
    return setup_args

def main():
    setup(**get_setup_args())

if __name__ == "__main__":
    main()
