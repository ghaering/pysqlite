#!/usr/bin/env python
import testsupport
import os, string, sys, types, unittest
import sqlite

class TransactionTests(unittest.TestCase, testsupport.TestSupport):
    def setUp(self):
        self.filename = self.getfilename()
        self.cnx = sqlite.connect(self.filename)
        self.cur = self.cnx.cursor()

    def tearDown(self):
        try:
            self.cnx.close()
            self.removefile()
        except AttributeError:
            pass
        except sqlite.InterfaceError:
            pass

    def CheckValueInTransaction(self):
        self.cur.execute("create table test (a)")
        self.cur.execute("insert into test (a) values (%s)", "foo")
        self.cur.execute("-- types int")
        self.cur.execute("select count(a) as count from test")
        res = self.cur.fetchone()
        self.failUnlessEqual(res.count, 1,
                             "Wrong number of rows during transaction.")

    def CheckValueAfterCommit(self):
        self.cur.execute("create table test (a)")
        self.cur.execute("insert into test (a) values (%s)", "foo")
        self.cur.execute("-- types int")
        self.cur.execute("select count(a) as count from test")
        self.cnx.commit()
        res = self.cur.fetchone()
        self.failUnlessEqual(res.count, 1,
                             "Wrong number of rows during transaction.")

    def CheckValueAfterRollback(self):
        self.cur.execute("create table test (a)")
        self.cnx.commit()
        self.cur.execute("insert into test (a) values (%s)", "foo")
        self.cnx.rollback()
        self.cur.execute("-- types int")
        self.cur.execute("select count(a) as count from test")
        res = self.cur.fetchone()
        self.failUnlessEqual(res.count, 0,
                             "Wrong number of rows during transaction.")

    def CheckImmediateCommit(self):
        try:
            self.cnx.commit()
        except:
            self.fail("Immediate commit raises exeption.")

    def CheckImmediateRollback(self):
        try:
            self.cnx.rollback()
        except:
            self.fail("Immediate rollback raises exeption.")

class AutocommitTests(unittest.TestCase, testsupport.TestSupport):
    def setUp(self):
        self.filename = self.getfilename()
        self.cnx = sqlite.connect(self.filename, autocommit=1)
        self.cur = self.cnx.cursor()

    def tearDown(self):
        try:
            self.cnx.close()
            self.removefile()
        except AttributeError:
            pass
        except sqlite.InterfaceError:
            pass

    def CheckCommit(self):
        self.cur.execute("select abs(5)")
        try:
            self.cnx.commit()
        except:
            self.fail(".commit() raised an exception")

    def CheckRollback(self):
        self.cur.execute("select abs(5)")
        self.failUnlessRaises(sqlite.ProgrammingError, self.cnx.rollback)

class ChangeAutocommitTests(unittest.TestCase):
    pass

def suite():
    transaction_tests = unittest.makeSuite(TransactionTests, "Check")
    autocommit_tests = unittest.makeSuite(AutocommitTests, "Check")
    change_autocommit_tests = unittest.makeSuite(ChangeAutocommitTests, "Check")

    test_suite = unittest.TestSuite((transaction_tests, autocommit_tests,
                                    change_autocommit_tests))
    return test_suite

def main():
    runner = unittest.TextTestRunner()
    runner.run(suite())

if __name__ == "__main__":
    main()
