#!/usr/bin/env python

import os, sys
from distutils.core import setup
from distutils.extension import Extension

__version__ = "1.1.6"

sqlite = "sqlite3"
sources = ["_sqlite.c", "encode.c", "port/strsep.c"]
macros = []

if sys.platform in ("linux-i386", "linux2"): # most Linux
    include_dirs = ['/usr/include/sqlite']
    library_dirs = ['/usr/lib/']
    libraries = [sqlite]
    runtime_library_dirs = []
    extra_objects = []
elif sys.platform in ("freebsd4", "freebsd5", "openbsd2", "cygwin", "darwin"):
    if sys.platform == "darwin":
        LOCALBASE = os.environ.get("LOCALBASE", "/opt/local")
    else:
        LOCALBASE = os.environ.get("LOCALBASE", "/usr/local")
    include_dirs = ['%s/include' % LOCALBASE]
    library_dirs = ['%s/lib/' % LOCALBASE]
    libraries = [sqlite]
    runtime_library_dirs = []
    extra_objects = []
elif sys.platform == "win32":
    include_dirs = [r'..\sqlite']
    library_dirs = [r'..\sqlite']
    libraries = [sqlite]
    runtime_library_dirs = []
    extra_objects = []
elif os.name == "posix": # most Unixish platforms
    include_dirs = ['/usr/local/include']
    library_dirs = ['/usr/local/lib']
    libraries = [sqlite]
    # On some platorms, this can be used to find the shared libraries
    # at runtime, if they are in a non-standard location. Doesn't
    # work for Linux gcc.
    ## runtime_library_dirs = library_dirs
    runtime_library_dirs = []
    # This can be used on Linux to force use of static sqlite lib
    ## extra_objects = ['/usr/lib/sqlite/libsqlite.a']
    extra_objects = []
else:
    raise "UnknownPlatform", "sys.platform=%s, os.name=%s" % \
          (sys.platform, os.name)
    
long_description = \
"""Python interface to SQLite

pysqlite is an interface to the SQLite database server for Python. It aims to be
fully compliant with Python database API version 2.0 while also exploiting the
unique features of SQLite.

"""

def main():
    py_modules = ["sqlite.main"]

    # patch distutils if it can't cope with the "classifiers" keyword
    if sys.version < '2.2.3':
        from distutils.dist import DistributionMetadata
        DistributionMetadata.classifiers = None
        DistributionMetadata.download_url = None

    setup ( # Distribution meta-data
            name = "pysqlite",
            version = __version__,
            description = "An interface to SQLite",
            long_description=long_description,
            author = "PySQLite developers",
            author_email = "pysqlite-devel@lists.sourceforge.net",
            license = "Python license",
            platforms = "ALL",
            url = "http://pysqlite.sourceforge.net/",

            # Description of the modules and packages in the distribution
            py_modules = py_modules,

            ext_modules = [Extension( name='_sqlite',
                                      sources=sources,
                                      include_dirs=include_dirs,
                                      library_dirs=library_dirs,
                                      runtime_library_dirs=runtime_library_dirs,
                                      libraries=libraries,
                                      extra_objects=extra_objects,
                                      define_macros=macros
                                      )],
            classifiers = [
            "Development Status :: 5 - Production/Stable",
            "Intended Audience :: Developers",
            "License :: OSI Approved :: MIT License",
            "Operating System :: Microsoft :: Windows :: Windows NT/2000",
            "Operating System :: POSIX",
            "Programming Language :: C",
            "Programming Language :: Python",
            "Topic :: Database :: Database Engines/Servers",
            "Topic :: Database :: Front-Ends"]
            )

if __name__ == "__main__":
    main()
