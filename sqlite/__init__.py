import _sqlite

"""Python interface to the SQLite embedded database engine."""

#-------------------------------------------------------------------------------
# Module Information
#-------------------------------------------------------------------------------

__revision__ = """$Revision: 1.22 $"""[11:-2]

threadsafety = 1
apilevel = "2.0"
paramstyle = "pyformat"

# This is the version string for the current PySQLite version.
version = "1.0.1"

# This is a tuple with the same digits as the vesrion string, but it's
# suitable for comparisons of various versions.
version_info = (1, 0, 1)

#-------------------------------------------------------------------------------
# Data type support
#-------------------------------------------------------------------------------

from main import DBAPITypeObject, Cursor, Connection, PgResultSet

STRING    = DBAPITypeObject(_sqlite.STRING)

BINARY    = DBAPITypeObject(_sqlite.BINARY)

INT       = DBAPITypeObject(_sqlite.INTEGER)

NUMBER    = DBAPITypeObject(_sqlite.INTEGER,
                            _sqlite.FLOAT)

DATE      = DBAPITypeObject(_sqlite.DATE)

TIME      = DBAPITypeObject(_sqlite.TIME)

TIMESTAMP = DBAPITypeObject(_sqlite.TIMESTAMP)

ROWID     = DBAPITypeObject()

# Nonstandard extension:
UNICODESTRING = DBAPITypeObject(_sqlite.UNICODESTRING)

#-------------------------------------------------------------------------------
# Exceptions
#-------------------------------------------------------------------------------

from _sqlite import Warning, Error, InterfaceError, \
    DatabaseError, DataError, OperationalError, IntegrityError, InternalError, \
    ProgrammingError, NotSupportedError

#-------------------------------------------------------------------------------
# Global Functions
#-------------------------------------------------------------------------------

def connect(*args, **kwargs):
    return Connection(*args, **kwargs)

from _sqlite import encode, decode

Binary = encode

__all__ = ['connect','IntegrityError', 'InterfaceError', 'InternalError',
           'NotSupportedError', 'OperationalError',
           'ProgrammingError', 'Warning',
           'Connection', 'Cursor', 'PgResultSet',
           'apilevel', 'paramstyle', 'threadsafety', 'version', 'version_info',
           'Binary', 'decode']
