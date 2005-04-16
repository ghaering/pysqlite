#-*- coding: ISO-8859-1 -*-
# pysqlite2/test/userfunctions.py: tests for user-defined functions and
#                                  aggregates.
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

def func_returntext():
    return "foo"
def func_returnint():
    return 42
def func_returnfloat():
    return 3.14
def func_returnnull():
    return None
def func_returnblob():
    return buffer("blob")
def func_raiseexception():
    5/0

def func_isstring(v):
    return type(v) is unicode
def func_isint(v):
    return type(v) is int
def func_isfloat(v):
    return type(v) is float
def func_isnone(v):
    return type(v) is type(None)
def func_isblob(v):
    return type(v) is buffer

class FunctionTests(unittest.TestCase):
    def setUp(self):
        self.cx = sqlite.connect(":memory:")
        cu = self.cx.cursor()
        cu.execute("""
            create table test(
                t text,
                i integer,
                f float,
                n,
                b blob
                )
            """)
        cu.execute("insert into test(t, i, f, n, b) values (?, ?, ?, ?, ?)",
            ("foo", 5, 3.14, None, buffer("blob"),)) 

        self.cx.create_function("returntext", 0, func_returntext)
        self.cx.create_function("returnint", 0, func_returnint)
        self.cx.create_function("returnfloat", 0, func_returnfloat)
        self.cx.create_function("returnnull", 0, func_returnnull)
        self.cx.create_function("returnblob", 0, func_returnblob)
        self.cx.create_function("raiseexception", 0, func_raiseexception)

        self.cx.create_function("isstring", 1, func_isstring)
        self.cx.create_function("isint", 1, func_isint)
        self.cx.create_function("isfloat", 1, func_isfloat)
        self.cx.create_function("isnone", 1, func_isnone)
        self.cx.create_function("isblob", 1, func_isblob)

    def tearDown(self):
        self.cx.close()

    def CheckFuncReturnText(self):
        cu = self.cx.cursor()
        cu.execute("select returntext()")
        val = cu.fetchone()[0]
        self.failUnlessEqual(type(val), str)
        self.failUnlessEqual(val, "foo")

    def CheckFuncReturnInt(self):
        cu = self.cx.cursor()
        cu.execute("select returnint()")
        val = cu.fetchone()[0]
        self.failUnlessEqual(type(val), int)
        self.failUnlessEqual(val, 42)

    def CheckFuncReturnFloat(self):
        cu = self.cx.cursor()
        cu.execute("select returnfloat()")
        val = cu.fetchone()[0]
        self.failUnlessEqual(type(val), float)
        if val < 3.139 or val > 3.141:
            self.fail("wrong value")

    def CheckFuncReturnNull(self):
        cu = self.cx.cursor()
        cu.execute("select returnnull()")
        val = cu.fetchone()[0]
        self.failUnlessEqual(type(val), type(None))
        self.failUnlessEqual(val, None)

    def CheckFuncReturnBlob(self):
        cu = self.cx.cursor()
        cu.execute("select returnblob()")
        val = cu.fetchone()[0]
        self.failUnlessEqual(type(val), buffer)
        self.failUnlessEqual(val, buffer("blob"))

    def CheckFuncException(self):
        cu = self.cx.cursor()
        cu.execute("select raiseexception()")
        val = cu.fetchone()[0]
        self.failUnlessEqual(val, None)

    def CheckParamString(self):
        cu = self.cx.cursor()
        cu.execute("select isstring(?)", ("foo",))
        val = cu.fetchone()[0]
        self.failUnlessEqual(val, 1)

    def CheckParamInt(self):
        cu = self.cx.cursor()
        cu.execute("select isint(?)", (42,))
        val = cu.fetchone()[0]
        self.failUnlessEqual(val, 1)

    def CheckParamFloat(self):
        cu = self.cx.cursor()
        cu.execute("select isfloat(?)", (3.14,))
        val = cu.fetchone()[0]
        self.failUnlessEqual(val, 1)

    def CheckParamNone(self):
        cu = self.cx.cursor()
        cu.execute("select isnone(?)", (None,))
        val = cu.fetchone()[0]
        self.failUnlessEqual(val, 1)

    def CheckParamBlob(self):
        cu = self.cx.cursor()
        cu.execute("select isblob(?)", (buffer("blob"),))
        val = cu.fetchone()[0]
        self.failUnlessEqual(val, 1)

class AggregateTests(unittest.TestCase):
    def setUp(self):
        self.cx = sqlite.connect(":memory:")
        self.cu = self.cx.cursor()
        self.cu.execute("create table test(id integer primary key, name text, income number)")
        self.cu.execute("insert into test(name) values (?)", ("foo",))

    def tearDown(self):
        self.cu.close()
        self.cx.close()

    def CheckExecuteNoArgs(self):
        self.cu.execute("delete from test")

def suite():
    function_suite = unittest.makeSuite(FunctionTests, "Check")
    aggregate_suite = unittest.makeSuite(AggregateTests, "Check")
    return unittest.TestSuite((function_suite, aggregate_suite))

def test():
    runner = unittest.TextTestRunner()
    runner.run(suite())

if __name__ == "__main__":
    test()
