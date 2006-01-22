#-*- coding: ISO-8859-1 -*-
# setup.py: the distutils script
#
# Copyright (C) 2004-2005 Gerhard Häring <gh@ghaering.de>
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

from ez_setup import use_setuptools
use_setuptools()

from setuptools import setup, Extension, Command

# If you need to change anything, it should be enough to change setup.cfg.

PYSQLITE_VERSION = "2.1.3"

sqlite = "sqlite"

sources = ["src/module.c", "src/connection.c", "src/cursor.c", "src/cache.c",
           "src/microprotocols.c", "src/adapters.c", "src/converters.c",
           "src/prepare_protocol.c", "src/statement.c", "src/util.c", "src/row.c"]

include_dirs = []
library_dirs = []
libraries = []
runtime_library_dirs = []
extra_objects = []
define_macros = []

long_description = \
"""Python interface to SQLite 3

pysqlite is an interface to the SQLite 3.x embedded relational database engine.
It is almost fully compliant with the Python database API version 2.0 also
exposes the unique features of SQLite."""

if sys.platform != "win32":
    define_macros.append(('PYSQLITE_VERSION', '"%s"' % PYSQLITE_VERSION))
else:
    define_macros.append(('PYSQLITE_VERSION', '\\"'+PYSQLITE_VERSION+'\\"'))
define_macros.append(('PY_MAJOR_VERSION', str(sys.version_info[0])))
define_macros.append(('PY_MINOR_VERSION', str(sys.version_info[1])))

class DocBuilder(Command):
    description = "Builds the documentation"
    user_options = []

    def initialize_options(self):
        pass

    def finalize_options(self):
        pass

    def run(self):
        import os, stat

        try:
            import docutils.core
            import docutilsupport
        except ImportError:
            print "Error: the build-docs command requires docutils and SilverCity to be installed"
            return

        docfiles = {
            "usage-guide.html": "usage-guide.txt",
            "install-source.html": "install-source.txt",
            "install-source-win32.html": "install-source-win32.txt"
        }

        os.chdir("doc")
        for dest, source in docfiles.items():
            if not os.path.exists(dest) or os.stat(dest)[stat.ST_MTIME] < os.stat(source)[stat.ST_MTIME]:
                print "Building documentation file %s." % dest
                docutils.core.publish_cmdline(
                    writer_name='html',
                    argv=[source, dest])

        os.chdir("..")

def main():
    data_files = [("pysqlite2-doc",
                        glob.glob("doc/*.html") \
                      + glob.glob("doc/*.txt") \
                      + glob.glob("doc/*.css")),
                   ("pysqlite2-doc/code",
                        glob.glob("doc/code/*.py"))]

    py_modules = ["sqlite"]
    setup ( # Distribution meta-data
            name = "pysqlite",
            version = PYSQLITE_VERSION,
            description = "DB-API 2.0 interface for SQLite 3.x",
            long_description=long_description,
            author = "Gerhard Haering",
            author_email = "gh@ghaering.de",
            license = "zlib/libpng license",
            platforms = "ALL",
            url = "http://pysqlite.org/",
            download_url = "http://initd.org/tracker/pysqlite/wiki/PysqliteDownloads",
            test_suite = "pysqlite2.test.suite",
            cmdclass = {"build_docs": DocBuilder},
            extras_require = {"build_docs": ["docutils", "SilverCity"]},

            # Description of the modules and packages in the distribution
            package_dir = {"pysqlite2": "pysqlite2"},
            packages = ["pysqlite2", "pysqlite2.test"],
            scripts=[],
            data_files = data_files,

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
            "Development Status :: 5 - Production/Stable",
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
