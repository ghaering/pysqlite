import unittest
import sqlite

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

    def CheckRollback(self):
        self.cx.rollback()

    def CheckCursor(self):
        cu = self.cx.cursor()

    def CheckClose(self):
        self.failUnless(hasattr(self.cx, "close"))

class CursorTests(unittest.TestCase):
    def setUp(self):
        self.cx = sqlite.connect(":memory:")
        self.cu = self.cx.cursor()
        self.cu.execute("create table test(id integer primary key, name text, income number)")
        self.cu.execute("insert into test(name) values (?)", ("foo",))

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
    # Sequences are required by the DB-API, iterators and generators are
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

        self.cu.execute("insert into test(income) values (?)", mygen)

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

def suite():
    module_suite = unittest.makeSuite(ModuleTests, "Check")
    connection_suite = unittest.makeSuite(ConnectionTests, "Check")
    cursor_suite = unittest.makeSuite(CursorTests, "Check")
    return unittest.TestSuite((module_suite, connection_suite, cursor_suite))

def test():
    runner = unittest.TextTestRunner()
    runner.run(suite())

if __name__ == "__main__":
    test()
