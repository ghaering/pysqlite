#!/usr/bin/env python
# This is a test case I got from a user that will crash PySQLite 0.5.0.
# It stress-tests PySQLite in multithreaded mode.

import os
import sys
import threading
import time
import random
import sqlite

dbname = "test.db"

#MECHANISM = "no timeout"
#MECHANISM = "use timeout"
#MECHANISM = "use slow busy handler"
MECHANISM = "use fast busy handler"

class Modifier(threading.Thread):
    def __init__(self, dbname):
        threading.Thread.__init__(self)
        self.dbname = dbname
    def run(self):
        print "Modifier: start"
        cx = sqlite.connect(self.dbname)
        cu = cx.cursor()
        print "Modifier: INSERTing"
        cu.execute("INSERT INTO meta (name, value) VALUES (%s, %s)",
                   "foo", "blah blah blah")
        for i in range(5):
            print "Modifier: sleeping %d" % i
            time.sleep(1)
        print "Modifier: committing"
        cx.commit()
        print "Modifier: committed"
        cu.close()
        cx.close()
        print "Modifier: end"

class Reader(threading.Thread):
    def __init__(self, name, dbname):
        threading.Thread.__init__(self, name=name)
        self.dbname = dbname
    def busyHandler(self, delay, table, numAttempts):
        print "Reader %s: busyHandler(delay=%r, table=%r, numAttempts=%r)"\
              % (self.getName(), delay, table, numAttempts)
        time.sleep(delay)
        return 1
    def run(self):
        print "Reader %s: start" % self.getName()
        if MECHANISM == "no timeout":
            cx = sqlite.connect(self.dbname)
        elif MECHANISM == "use timeout":
            cx = sqlite.connect(self.dbname, timeout=5000)
        elif MECHANISM == "use slow busy handler":
            cx = sqlite.connect(self.dbname)
            cx.db.sqlite_busy_handler(self.busyHandler, 1.0)
        elif MECHANISM == "use fast busy handler":
            cx = sqlite.connect(self.dbname, Xtimeout=5000.0)
            cx.db.sqlite_busy_handler(self.busyHandler, 0.1)
        else:
            raise ValueError("MECHANISM is not one of the expected values")
        sleepFor = random.randint(0, 3)
        print "Reader %s: sleeping for %d seconds" % (self.getName(), sleepFor)
        time.sleep(sleepFor)
        print "Reader %s: waking up" % self.getName()
        cu = cx.cursor()
        print "Reader %s: SELECTing" % self.getName()
        cu.execute("SELECT name, value FROM meta WHERE name='%s'" % self.getName())
        print "Reader %s: SELECTed %s" % (self.getName(), cu.fetchone())
        cu.close()
        cx.close()
        print "Reader %s: end" % self.getName()

def test_sqlite_busy():
    """Test handling of SQL_BUSY "errors" as discussed here:
        http://www.hwaci.com/sw/sqlite/faq.html#q7
        http://www.sqlite.org/cvstrac/wiki?p=MultiThreading
    
    Algorithm:
        - start one thread that will open the database and start modifying it
          then sleep for a while so other threads can get in there
        - have other thread(s) do selects from the database and see if they
          error out, if they block, if they timeout (play with timeout
          .connect() argument)
    """
    # Create a fresh starting database.
    if os.path.exists(dbname):
        os.remove(dbname)
    journal = dbname+"-journal"
    if os.path.exists(journal):
        os.remove(journal)
    cx = sqlite.connect(dbname)
    cu = cx.cursor()
    cu.execute("CREATE TABLE meta (name STRING, value STRING)")
    cx.commit()
    cu.close()
    cx.close()
    
    modifier = Modifier(dbname)
    readerNames = ("foo",) #XXX "bar", "baz")
    readers = [Reader(name, dbname) for name in readerNames]
    modifier.start()
    for reader in readers:
        reader.start()
    modifier.join()
    for reader in readers:
        reader.join()


if __name__ == "__main__":
    test_sqlite_busy()
