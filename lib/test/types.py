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

    def _does_not_work_atm_CheckLargeInt(self):
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

class AdvancedTypeTests(unittest.TestCase):
    class Foo:
        def __init__(self, _val):
            self.val = _val

        def __cmp__(self, other):
            if not isinstance(other, AdvancedTypeTests.Foo):
                print "other type:", type(other), other
                raise ValueError
            if self.val == other.val:
                return 0
            else:
                return 1

        def __conform__(self, protocol):
            # hack: assume `protocol` is the right one
            return self.val

        def __str__(self):
            return "<%s>" % self.val

    def setUp(self):
        self.con = sqlite.connect(":memory:", more_types=True)
        self.cur = self.con.cursor()
        self.cur.execute("create table test(i int, s str, f float, b bool, u unicode, foo foo, bin blob)")

        self.con.register_converter("int", lambda x: int(x))
        self.con.register_converter("float", lambda x: float(x))
        self.con.register_converter("bool", lambda x: x == "1")
        self.con.register_converter("unicode", lambda x: unicode(x, "utf-8"))
        self.con.register_converter("foo", AdvancedTypeTests.Foo)
        self.con.register_converter("blob", buffer)

    def tearDown(self):
        self.cur.close()
        self.con.close()

    def CheckString(self):
        self.cur.execute("insert into test(s) values (?)", ("foo",))
        self.cur.execute("select s from test")
        row = self.cur.fetchone()
        self.failUnlessEqual(row[0], "foo")

    def CheckSmallInt(self):
        self.cur.execute("insert into test(i) values (?)", (42,))
        self.cur.execute("select i from test")
        row = self.cur.fetchone()
        self.failUnlessEqual(row[0], 42)

    def _does_not_work_atm_CheckLargeInt(self):
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

    def CheckBool(self):
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
        val = u"\xd6sterreich"
        self.cur.execute("insert into test(u) values (?)", (val,))
        self.cur.execute("select u from test")
        row = self.cur.fetchone()
        self.failUnlessEqual(row[0], val)

    def _does_not_work_atm_CheckFoo(self):
        val = AdvancedTypeTests.Foo("bla")
        self.cur.execute("insert into test(foo) values (?)", (val,))
        self.cur.execute("select foo from test")
        row = self.cur.fetchone()
        self.failUnlessEqual(row[0], val)

    def CheckBlob(self):
        val = buffer("Guglhupf")
        self.cur.execute("insert into test(bin) values (?)", (val,))
        self.cur.execute("select bin from test")
        row = self.cur.fetchone()
        self.failUnlessEqual(row[0], val)

def suite():
    sqlite_type_suite = unittest.makeSuite(SqliteTypeTests, "Check")
    advanced_type_suite = unittest.makeSuite(AdvancedTypeTests, "Check")
    return unittest.TestSuite((sqlite_type_suite, advanced_type_suite))

def test():
    runner = unittest.TextTestRunner()
    runner.run(suite())

if __name__ == "__main__":
    test()
