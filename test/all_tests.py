#!/usr/bin/env python
"""
This combines all PySQLite test suites into one big one.
"""

import unittest, sys
import api_tests, logging_tests, lowlevel_tests, pgresultset_tests, type_tests
import userfunction_tests, transaction_tests

def suite():
    suite = unittest.TestSuite((lowlevel_tests.suite(), api_tests.suite(),
            type_tests.suite(), userfunction_tests.suite(),
            transaction_tests.suite(), pgresultset_tests.suite(),
            logging_tests.suite()))

    return suite

def main():
    runner = unittest.TextTestRunner()
    runner.run(suite())

if __name__ == "__main__":
    main()
