import unittest
from sqlite.test import dbapi

def suite():
    return unittest.TestSuite(
        (dbapi.suite(),))
def test():
    runner = unittest.TextTestRunner()
    runner.run(suite())
