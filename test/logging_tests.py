#!/usr/bin/env python
import testsupport
import StringIO, unittest
import sqlite

class LogFileTemplate:
    def write(self, s):
        pass

class LogFile:
    def __init__(self):
        pass

def init_LogFile():
    LogFile.write = LogFileTemplate.write

class CommandLoggingTests(unittest.TestCase, testsupport.TestSupport):
    def tearDown(self):
        try:
            self.cnx.close()
            self.removefile()
        except AttributeError:
            pass
        except sqlite.InterfaceError:
            pass

    def CheckNoWrite(self):
        init_LogFile()
        del LogFile.write
        logger = LogFile()
        try:
            self.cnx = sqlite.connect(self.getfilename(),
                command_logfile=logger)

            self.fail("ValueError not raised")
        except ValueError:
            pass

    def CheckWriteNotCallable(self):
        logger = LogFile()
        logger.write = 5
        try:
            self.cnx = sqlite.connect(self.getfilename(),
                command_logfile=logger)

            self.fail("ValueError not raised")
        except ValueError:
            pass

    def CheckLoggingWorks(self):
        logger = StringIO.StringIO()

        expected_output = "\n".join([
            "BEGIN", "CREATE TABLE TEST(FOO INTEGER)",
            "INSERT INTO TEST(FOO) VALUES (5)",
            "ROLLBACK"]) + "\n"

        self.cnx = sqlite.connect(self.getfilename(),
            command_logfile=logger)
        cu = self.cnx.cursor()
        cu.execute("CREATE TABLE TEST(FOO INTEGER)")
        cu.execute("INSERT INTO TEST(FOO) VALUES (%i)", (5,))
        self.cnx.rollback()

        logger.seek(0)
        real_output = logger.read()
        
        if expected_output != real_output:
            self.fail("Logging didn't produce expected output.")

def suite():
    command_logging_suite = unittest.makeSuite(CommandLoggingTests, "Check")
    return command_logging_suite

def main():
    runner = unittest.TextTestRunner()
    runner.run(suite())

if __name__ == "__main__":
    main()
