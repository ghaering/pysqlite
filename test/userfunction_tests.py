#!/usr/bin/env python
import testsupport
import os, string, sys, types, unittest
import sqlite

def intreturner(x):
    return int(x) * 2

def floatreturner(x):
    return float(x) * 2.0

def stringreturner(x):
    return "[%s]" % x

def nullreturner(x):
    return None

def exceptionreturner(x):
    return 5 / 0

class MySum:
    def __init__(self):
        self.reset()

    def reset(self):
        self.sum = 0

    def step(self, x):
        self.sum += int(x)

    def finalize(self):
        val = self.sum
        self.reset()
        return val

class MySumFloat:
    def __init__(self):
        self.reset()

    def reset(self):
        self.sum = 0.0

    def step(self, x):
        self.sum += float(x)

    def finalize(self):
        val = self.sum
        self.reset()
        return val

class MySumReturnNull:
    def __init__(self):
        self.reset()

    def reset(self):
        self.sum = 0

    def step(self, x):
        self.sum += int(x)

    def finalize(self):
        return None

class MySumStepExeption:
    def __init__(self):
        self.reset()

    def reset(self):
        self.sum = 0

    def step(self, x):
        self.sum += int(x) / 0

    def finalize(self):
        val = self.sum
        self.reset()
        return val

class MySumFinalizeExeption:
    def __init__(self):
        self.reset()

    def reset(self):
        self.sum = 0

    def step(self, x):
        self.sum += int(x)

    def finalize(self):
        val = self.sum / 0
        self.reset()
        return val

class UserFunctions(unittest.TestCase, testsupport.TestSupport):
    def setUp(self):
        self.filename = self.getfilename()
        self.cnx = sqlite.connect(self.filename)

        sqlite._sqlite.enable_callback_debugging(0)

        self.cnx.create_function("intreturner", 1, intreturner)
        self.cnx.create_function("floatreturner", 1, floatreturner)
        self.cnx.create_function("stringreturner", 1, stringreturner)
        self.cnx.create_function("nullreturner", 1, nullreturner)
        self.cnx.create_function("exceptionreturner", 1, exceptionreturner)

        self.cnx.create_aggregate("mysum", 1, MySum)
        self.cnx.create_aggregate("mysumfloat", 1, MySumFloat)
        self.cnx.create_aggregate("mysumreturnnull", 1, MySumReturnNull )
        self.cnx.create_aggregate("mysumstepexception", 1, MySumStepExeption)
        self.cnx.create_aggregate("mysumfinalizeexception", 1, MySumFinalizeExeption)
        self.cur = self.cnx.cursor()

    def tearDown(self):
        try:
            self.cnx.close()
            self.removefile()
        except AttributeError:
            pass
        except sqlite.InterfaceError:
            pass

    def CheckIntFunction(self):
        self.cur.execute("create table test (a)")
        self.cur.execute("insert into test(a) values (%s)", 5)
        self.cur.execute("-- types int")
        self.cur.execute("select intreturner(a) as a from test")
        res = self.cur.fetchone()
        self.failUnless(isinstance(res.a, types.IntType),
                        "The result should have been an int.")
        self.failUnlessEqual(res.a, 10,
                        "The function returned the wrong result.")

    def CheckFloatFunction(self):
        self.cur.execute("create table test (a)")
        self.cur.execute("insert into test(a) values (%s)", 5.0)
        self.cur.execute("-- types float")
        self.cur.execute("select floatreturner(a) as a from test")
        res = self.cur.fetchone()
        self.failUnless(isinstance(res.a, types.FloatType),
                        "The result should have been a float.")
        self.failUnlessEqual(res.a, 5.0 * 2.0,
                        "The function returned the wrong result.")

    def CheckStringFunction(self):
        mystr = "test"
        self.cur.execute("create table test (a)")
        self.cur.execute("insert into test(a) values (%s)", mystr)
        self.cur.execute("-- types str")
        self.cur.execute("select stringreturner(a) as a from test")
        res = self.cur.fetchone()
        self.failUnless(isinstance(res.a, types.StringType),
                        "The result should have been a string.")
        self.failUnlessEqual(res.a, "[%s]" % mystr,
                        "The function returned the wrong result.")

    def CheckNullFunction(self):
        mystr = "test"
        self.cur.execute("create table test (a)")
        self.cur.execute("insert into test(a) values (%s)", mystr)
        self.cur.execute("-- types str")
        self.cur.execute("select nullreturner(a) as a from test")
        res = self.cur.fetchone()
        self.failUnlessEqual(res.a, None,
                        "The result should have been None.")

    def CheckFunctionWithNullArgument(self):
        mystr = "test"
        self.cur.execute("-- types str")
        self.cur.execute("select nullreturner(NULL) as a")
        res = self.cur.fetchone()
        self.failUnlessEqual(res.a, None,
                        "The result should have been None.")


    def CheckExceptionFunction(self):
        mystr = "test"
        self.cur.execute("create table test (a)")
        self.cur.execute("insert into test(a) values (%s)", mystr)
        self.cur.execute("-- types str")
        try:
            self.cur.execute("select exceptionreturner(a) as a from test")
        except sqlite.DatabaseError, reason:
            pass
        except Exception, reason:
            self.fail("Wrong exception raised: %s", sys.exc_info()[0])

    def CheckAggregateBasic(self):
        self.cur.execute("create table test (a)")
        self.cur.executemany("insert into test(a) values (%s)", [(10,), (20,), (30,)])
        self.cur.execute("-- types int")
        self.cur.execute("select mysum(a) as sum from test")
        res = self.cur.fetchone()
        self.failUnless(isinstance(res.sum, types.IntType),
                        "The result should have been an int.")
        self.failUnlessEqual(res.sum, 60,
                        "The function returned the wrong result.")

    def CheckAggregateFloat(self):
        self.cur.execute("create table test (a)")
        self.cur.executemany("insert into test(a) values (%s)", [(10.0,), (20.0,), (30.0,)])
        self.cur.execute("-- types float")
        self.cur.execute("select mysumfloat(a) as sum from test")
        res = self.cur.fetchone()
        self.failUnless(isinstance(res.sum, types.FloatType),
                        "The result should have been an float.")
        if res.sum <= 59.9 or res.sum >= 60.1:
            self.fail("The function returned the wrong result.")

    def CheckAggregateReturnNull(self):
        self.cur.execute("create table test (a)")
        self.cur.executemany("insert into test(a) values (%s)", [(10,), (20,), (30,)])
        self.cur.execute("-- types int")
        self.cur.execute("select mysumreturnnull(a) as sum from test")
        res = self.cur.fetchone()
        self.failUnlessEqual(res.sum, None,
                        "The result should have been None.")

    def CheckAggregateStepException(self):
        self.cur.execute("create table test (a)")
        self.cur.executemany("insert into test(a) values (%s)", [(10,), (20,), (30,)])
        self.cur.execute("-- types int")
        try:
            self.cur.execute("select mysumstepexception(a) as sum from test")
        except sqlite.DatabaseError, reason:
            pass
        except Exception, reason:
            self.fail("Wrong exception raised: %s" % sys.exc_info()[0])

    def CheckAggregateFinalizeException(self):
        self.cur.execute("create table test (a)")
        self.cur.executemany("insert into test(a) values (%s)", [(10,), (20,), (30,)])
        self.cur.execute("-- types int")
        try:
            self.cur.execute("select mysumfinalizeexception(a) as sum from test")
        except sqlite.DatabaseError, reason:
            pass
        except Exception, reason:
            self.fail("Wrong exception raised: %s", sys.exc_info()[0])

    def CheckAggregateStepNullArgument(self):
        self.cur.execute("-- types int")
        self.cur.execute("select mysum(NULL) as a")
        res = self.cur.fetchone()
        self.failUnlessEqual(res.a, 0,
                        "The result should have been 0.")


def suite():
    user_functions = unittest.makeSuite(UserFunctions, "Check")
    test_suite = unittest.TestSuite((user_functions,))
    return test_suite

def main():
    runner = unittest.TextTestRunner()
    runner.run(suite())

if __name__ == "__main__":
    main()
