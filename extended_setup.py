#-*- coding: ISO-8859-1 -*-
# ext_setup.py: setuptools extensions for the distutils script
#
# Copyright (C) 2004-2006 Gerhard Häring <gh@ghaering.de>
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

import setuptools
import setup

class DocBuilder(setuptools.Command):
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
                    argv=["--stylesheet=default.css", source, dest])

        os.chdir("..")

def main():
    setup_args = setup.get_setup_args()
    setup_args["extras_require"] = {"build_docs": ["docutils", "SilverCity"]}
    setup_args["cmdclass"] = {"build_docs": DocBuilder}
    setup_args["test_suite"] = "pysqlite2.test.suite"
    setup_args["cmdclass"] = {"build_docs": DocBuilder}
    setup_args["extras_require"] = {"build_docs": ["docutils", "SilverCity"]}
    setuptools.setup(**setup_args)

if __name__ == "__main__":
    main()
