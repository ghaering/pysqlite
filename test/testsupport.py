#!/usr/bin/env python
import _sqlite
import os, tempfile, unittest

class TestSupport:
    def getfilename(self):
        if _sqlite.sqlite_version_info() >= (2, 8, 2):
            return ":memory:"
        else:
            return tempfile.mktemp()

    def removefile(self):
        if self.filename != ":memory:":
            os.remove(self.filename)
