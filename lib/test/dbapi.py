#-*- coding: ISO-8859-1 -*-
# pysqlite2/test/dbapi.py: tests for DB-API compliance
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

class ModuleTests(unittest.TestCase):
    def CheckAPILevel(self):
        self.assertEqual(sqlite.apilevel, "2.0",
                         "apilevel is %s, should be 2.0" % sqlite.apilevel)

    def CheckThreadSafety(self):
        self.assertEqual(sqlite.threadsafety, 1,
                         "threadsafety is %d, should be 1" % sqlite.threadsafety)

    def CheckParamStyle(self):
        self.assertEqual(sqlite.paramstyle, "qmark",
                         "paramstyle is '%s', should be 'qmark'" %
                         sqlite.paramstyle)

    def CheckWarning(self):
        self.assert_(issubclass(sqlite.Warning, StandardError),
                     "Warning is not a subclass of StandardError")

    def CheckError(self):
        self.failUnless(issubclass(sqlite.Error, StandardError),
                        "Error is not a subclass of StandardError")

    def CheckInterfaceError(self):
        self.failUnless(issubclass(sqlite.InterfaceError, sqlite.Error),
                        "InterfaceError is not a subclass of Error")

    def CheckDatabaseError(self):
        self.failUnless(issubclass(sqlite.DatabaseError, sqlite.Error),
                        "DatabaseError is not a subclass of Error")

    def CheckDataError(self):
        self.failUnless(issubclass(sqlite.DataError, sqlite.DatabaseError),
                        "DataError is not a subclass of DatabaseError")

    def CheckOperationalError(self):
        self.failUnless(issubclass(sqlite.OperationalError, sqlite.DatabaseError),
                        "OperationalError is not a subclass of DatabaseError")

    def CheckIntegrityError(self):
        self.failUnless(issubclass(sqlite.IntegrityError, sqlite.DatabaseError),
                        "IntegrityError is not a subclass of DatabaseError")

    def CheckInternalError(self):
        self.failUnless(issubclass(sqlite.InternalError, sqlite.DatabaseError),
                        "InternalError is not a subclass of DatabaseError")

    def CheckProgrammingError(self):
        self.failUnless(issubclass(sqlite.ProgrammingError, sqlite.DatabaseError),
                        "ProgrammingError is not a subclass of DatabaseError")

    def CheckNotSupportedError(self):
        self.failUnless(issubclass(sqlite.NotSupportedError,
                                   sqlite.DatabaseError),
                        "NotSupportedError is not a subclass of DatabaseError")

class ConnectionTests(unittest.TestCase):
    def setUp(self):
        self.cx = sqlite.connect(":memory:")
        cu = self.cx.cursor()
        cu.execute("create table test(id integer primary key, name text)")
        cu.execute("insert into test(name) values (?)", ("foo",))

    def tearDown(self):
        self.cx.close()

    def CheckCommit(self):
        self.cx.commit()

    def CheckCommitAfterNoChanges(self):
        """
        A commit should also work when no changes were made to the database.
        """
        self.cx.commit()
        self.cx.commit()

    def CheckRollback(self):
        self.cx.rollback()

    def CheckRollbackAfterNoChanges(self):
        """
        A rollback should also work when no changes were made to the database.
        """
        self.cx.rollback()
        self.cx.rollback()

    def CheckCursor(self):
        cu = self.cx.cursor()

    def CheckClose(self):
        self.cx.close()

class CursorTests(unittest.TestCase):
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

    def CheckExecuteArgInt(self):
        self.cu.execute("insert into test(id) values (?)", (42,))

    def CheckExecuteArgFloat(self):
        self.cu.execute("insert into test(income) values (?)", (2500.32,))

    def CheckExecuteArgString(self):
        self.cu.execute("insert into test(name) values (?)", ("Hugo",))

    def CheckClose(self):
        self.cu.close()

    # Checks for executemany:
    # Sequences are required by the DB-API, iterators
    # enhancements in pysqlite.

    def CheckExecuteManySequence(self):
        self.cu.execute("insert into test(income) values (?)", [(x,) for x in range(100, 110)])

    def CheckExecuteManyIterator(self):
        class MyIter:
            def __init__(self):
                self.value = 5

            def next(self):
                if self.value == 10:
                    raise StopIteration
                else:
                    self.value += 1
                    return (self.value,)

        self.cu.execute("insert into test(income) values (?)", MyIter())

    def CheckExecuteManyGenerator(self):
        def mygen():
            for i in range(5):
                yield (i,)

        self.cu.execute("insert into test(income) values (?)", mygen())

    def CheckFetchIter(self):
        self.cu.execute("delete from test")
        self.cu.execute("insert into test(id) values (?)", (5,))
        self.cu.execute("insert into test(id) values (?)", (6,))
        self.cu.execute("select id from test order by id")
        lst = []
        for row in self.cu:
            lst.append(row[0])
        self.failUnlessEqual(lst[0], 5)
        self.failUnlessEqual(lst[1], 6)

    def CheckFetchone(self):
        self.cu.execute("select name from test")
        row = self.cu.fetchone()
        self.failUnlessEqual(row[0], "foo")
        row = self.cu.fetchone()
        self.failUnlessEqual(row, None)

    def CheckFetchmany(self):
        self.cu.execute("select name from test")
        res = self.cu.fetchmany(100)
        self.failUnlessEqual(len(res), 1)

    def CheckFetchall(self):
        self.cu.execute("select name from test")
        res = self.cu.fetchall()
        self.failUnlessEqual(len(res), 1)

    def CheckSetinputsizes(self):
        self.cu.setinputsizes([3, 4, 5])

    def CheckSetoutputsize(self):
        self.cu.setoutputsize(5, 0)

    def CheckSetoutputsizeNoColumn(self):
        self.cu.setoutputsize(42)

class TypeTests(unittest.TestCase):
    def setUp(self):
        self.cx = sqlite.connect(":memory:")
        self.cu = self.cx.cursor()
        self.cu.execute("create table test(id integer primary key, name text, bin binary, ratio number, ts timestamp)")

    def tearDown(self):
        self.cu.close()
        self.cx.close()

    def CheckROWID(self):
        self.cu.execute("insert into test(name) values (?)", ("foo",))
        self.cu.execute("select id from test")
        res = self.cu.fetchone()
        self.failUnlessEqual(self.cu.description[0][1], sqlite.ROWID)

    def CheckSTRING(self):
        self.cu.execute("insert into test(name) values (?)", ("foo",))
        self.cu.execute("select name from test")
        res = self.cu.fetchone()
        self.failUnlessEqual(self.cu.description[0][1], sqlite.STRING)

    def CheckBINARY(self):
        # if we want to get binary data back from SQLite by default for a
        # column, we must make sure that we insert a binary value. To do so,
        # we would need a different binary constructor, and intercept the type
        # at the _pysqlite side, then use sqlite3_result_blob() to set it.
        pass

    def CheckNUMBER(self):
        self.cu.execute("insert into test(ratio) values (?)", (3.14,))
        self.cu.execute("select ratio from test")
        res = self.cu.fetchone()
        self.failUnlessEqual(self.cu.description[0][1], sqlite.NUMBER)

    def CheckDATETIME(self):
        # SQLite does not know about date/time objects internally, so we cannot
        # test that.
        pass

class ConstructorTests(unittest.TestCase):
    def CheckDate(self):
        d = sqlite.Date(2004, 10, 28)

    def CheckTime(self):
        t = sqlite.Time(12, 39, 35)

    def CheckTimestamp(self):
        ts = sqlite.Timestamp(2004, 10, 28, 12, 39, 35)

    def CheckDateFromTicks(self):
        d = sqlite.DateFromTicks(42)

    def CheckTimeFromTicks(self):
        t = sqlite.TimeFromTicks(42)

    def CheckTimestampFromTicks(self):
        ts = sqlite.TimestampFromTicks(42)

    def CheckBinary(self):
        b = sqlite.Binary(chr(0) + "'")

def suite():
    module_suite = unittest.makeSuite(ModuleTests, "Check")
    connection_suite = unittest.makeSuite(ConnectionTests, "Check")
    cursor_suite = unittest.makeSuite(CursorTests, "Check")
    type_suite = unittest.makeSuite(TypeTests, "Check")
    constructor_suite = unittest.makeSuite(ConstructorTests, "Check")
    return unittest.TestSuite((module_suite, connection_suite, cursor_suite, type_suite, constructor_suite))

def test():
    runner = unittest.TextTestRunner()
    runner.run(suite())

if __name__ == "__main__":
    test()
