#-*- coding: ISO-8859-1 -*-
# pysqlite2/test/types.py: tests for type conversion and detection
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

import unittest
import pysqlite2.dbapi2 as sqlite

class SqliteTypeTests(unittest.TestCase):
    def setUp(self):
        self.con = sqlite.connect(":memory:")
        self.cur = self.con.cursor()
        self.cur.execute("create table test(i integer, s varchar, f number, b blob)")

    def tearDown(self):
        self.cur.close()
        self.con.close()

    def CheckString(self):
        self.cur.execute("insert into test(s) values (?)", (u"Österreich",))
        self.cur.execute("select s from test")
        row = self.cur.fetchone()
        self.failUnlessEqual(row[0], u"Österreich")

    def CheckSmallInt(self):
        self.cur.execute("insert into test(i) values (?)", (42,))
        self.cur.execute("select i from test")
        row = self.cur.fetchone()
        self.failUnlessEqual(row[0], 42)

    def CheckLargeInt(self):
        num = 2**40
        self.cur.execute("insert into test(i) values (?)", (num,))
        self.cur.execute("select i from test")
        row = self.cur.fetchone()
        self.failUnlessEqual(row[0], num)

    def CheckFloat(self):
        val = 3.14
        self.cur.execute("insert into test(f) values (?)", (val,))
        self.cur.execute("select f from test")
        row = self.cur.fetchone()
        self.failUnlessEqual(row[0], val)

    def CheckBlob(self):
        val = buffer("Guglhupf")
        self.cur.execute("insert into test(b) values (?)", (val,))
        self.cur.execute("select b from test")
        row = self.cur.fetchone()
        self.failUnlessEqual(row[0], val)

    def CheckUnicodeExecute(self):
        self.cur.execute(u"select 'Österreich'")
        row = self.cur.fetchone()
        self.failUnlessEqual(row[0], u"Österreich")

class DeclTypesTests(unittest.TestCase):
    class Foo:
        def __init__(self, _val):
            self.val = _val

        def __cmp__(self, other):
            if not isinstance(other, DeclTypesTests.Foo):
                raise ValueError
            if self.val == other.val:
                return 0
            else:
                return 1

        def __conform__(self, protocol):
            if isinstance(protocol, sqlite.PrepareProtocol):
                return self.val
            else:
                return None

        def __str__(self):
            return "<%s>" % self.val

    def setUp(self):
        self.con = sqlite.connect(":memory:", detect_types=sqlite.PARSE_DECLTYPES)
        self.cur = self.con.cursor()
        self.cur.execute("create table test(i int, s str, f float, b bool, u unicode, foo foo, bin blob)")

        # override float, make them always return the same number
        self.con.converters["float"] = lambda x: 47.2

        # and implement two custom ones
        self.con.converters["bool"] = lambda x: bool(int(x))
        self.con.converters["foo"] = DeclTypesTests.Foo

    def tearDown(self):
        self.cur.close()
        self.con.close()

    def CheckString(self):
        # default
        self.cur.execute("insert into test(s) values (?)", ("foo",))
        self.cur.execute("select s from test")
        row = self.cur.fetchone()
        self.failUnlessEqual(row[0], "foo")

    def CheckSmallInt(self):
        # default
        self.cur.execute("insert into test(i) values (?)", (42,))
        self.cur.execute("select i from test")
        row = self.cur.fetchone()
        self.failUnlessEqual(row[0], 42)

    def CheckLargeInt(self):
        # default
        num = 2**40
        self.cur.execute("insert into test(i) values (?)", (num,))
        self.cur.execute("select i from test")
        row = self.cur.fetchone()
        self.failUnlessEqual(row[0], num)

    def CheckFloat(self):
        # custom
        val = 3.14
        self.cur.execute("insert into test(f) values (?)", (val,))
        self.cur.execute("select f from test")
        row = self.cur.fetchone()
        self.failUnlessEqual(row[0], 47.2)

    def CheckBool(self):
        # custom
        self.cur.execute("insert into test(b) values (?)", (False,))
        self.cur.execute("select b from test")
        row = self.cur.fetchone()
        self.failUnlessEqual(row[0], False)

        self.cur.execute("delete from test")
        self.cur.execute("insert into test(b) values (?)", (True,))
        self.cur.execute("select b from test")
        row = self.cur.fetchone()
        self.failUnlessEqual(row[0], True)

    def CheckUnicode(self):
        # default
        val = u"\xd6sterreich"
        self.cur.execute("insert into test(u) values (?)", (val,))
        self.cur.execute("select u from test")
        row = self.cur.fetchone()
        self.failUnlessEqual(row[0], val)

    def CheckFoo(self):
        val = DeclTypesTests.Foo("bla")
        self.cur.execute("insert into test(foo) values (?)", (val,))
        self.cur.execute("select foo from test")
        row = self.cur.fetchone()
        self.failUnlessEqual(row[0], val)

    def CheckUnsupportedSeq(self):
        class Bar: pass
        val = Bar()
        try:
            self.cur.execute("insert into test(f) values (?)", (val,))
            self.fail("should have raised an InterfaceError")
        except sqlite.InterfaceError:
            pass
        except:
            self.fail("should have raised an InterfaceError")

    def CheckUnsupportedDict(self):
        class Bar: pass
        val = Bar()
        try:
            self.cur.execute("insert into test(f) values (:val)", {"val": val})
            self.fail("should have raised an InterfaceError")
        except sqlite.InterfaceError:
            pass
        except:
            self.fail("should have raised an InterfaceError")

    def CheckBlob(self):
        # default
        val = buffer("Guglhupf")
        self.cur.execute("insert into test(bin) values (?)", (val,))
        self.cur.execute("select bin from test")
        row = self.cur.fetchone()
        self.failUnlessEqual(row[0], val)

class ColNamesTests(unittest.TestCase):
    def setUp(self):
        self.con = sqlite.connect(":memory:", detect_types=sqlite.PARSE_COLNAMES|sqlite.PARSE_DECLTYPES)
        self.cur = self.con.cursor()
        self.cur.execute("create table test(x foo)")

        self.con.converters["foo"] = lambda x: "[%s]" % x
        self.con.converters["bar"] = lambda x: "<%s>" % x
        self.con.converters["exc"] = lambda x: 5/0

    def tearDown(self):
        self.cur.close()
        self.con.close()

    def CheckDeclType(self):
        self.cur.execute("insert into test(x) values (?)", ("xxx",))
        self.cur.execute("select x from test")
        val = self.cur.fetchone()[0]
        self.failUnlessEqual(val, "[xxx]")

    def CheckExc(self):
        # Exceptions in type converters result in returned Nones
        self.cur.execute('select 5 as "x [exc]"')
        val = self.cur.fetchone()[0]
        self.failUnlessEqual(val, None)

    def CheckColName(self):
        self.cur.execute("insert into test(x) values (?)", ("xxx",))
        self.cur.execute('select x as "x [bar]" from test')
        val = self.cur.fetchone()[0]
        self.failUnlessEqual(val, "<xxx>")

        # Check if the stripping of colnames works. Everything after the first
        # whitespace should be stripped.
        self.failUnlessEqual(self.cur.description[0][0], "x")

class PrepareProtocolTests(unittest.TestCase):
    class P(sqlite.PrepareProtocol):
        def __adapt__(self, obj):
            if type(obj) is int:
                return float(obj)
            else:
                return None

    def setUp(self):
        self.con = sqlite.connect(":memory:", prepareProtocol=PrepareProtocolTests.P())
        self.cur = self.con.cursor()

    def tearDown(self):
        self.cur.close()
        self.con.close()

    def CheckProtocolIsUsed(self):
        self.cur.execute("select ?", (4,))
        val = self.cur.fetchone()[0]
        self.failUnlessEqual(type(val), float)

def suite():
    sqlite_type_suite = unittest.makeSuite(SqliteTypeTests, "Check")
    decltypes_type_suite = unittest.makeSuite(DeclTypesTests, "Check")
    colnames_type_suite = unittest.makeSuite(ColNamesTests, "Check")
    prepareprotocol_suite = unittest.makeSuite(PrepareProtocolTests, "Check")
    return unittest.TestSuite((sqlite_type_suite, decltypes_type_suite, colnames_type_suite, prepareprotocol_suite))

def test():
    runner = unittest.TextTestRunner()
    runner.run(suite())

if __name__ == "__main__":
    test()
