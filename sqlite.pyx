cdef extern from "sqlite3.h":
    ctypedef struct sqlite3:
        pass
    ctypedef struct sqlite3_stmt:
        pass

    int sqlite3_open_v2(char *filename, sqlite3 **ppDb, int flags, char *zVfs)
    int sqlite3_prepare_v2(sqlite3 *db, char *zSql, int nByte, sqlite3_stmt **ppStmt, char **pzTail)
    int sqlite3_step(sqlite3_stmt*)
    char* sqlite3_column_text(sqlite3_stmt*, int iCol)
    int sqlite3_errcode(sqlite3 *db)
    char *sqlite3_errmsg(sqlite3* db)

    int SQLITE_ABORT
    int SQLITE_BUSY
    int SQLITE_CANTOPEN
    int SQLITE_CONSTRAINT
    int SQLITE_CORRUPT
    int SQLITE_DONE
    int SQLITE_EMPTY
    int SQLITE_ERROR
    int SQLITE_FULL
    int SQLITE_INTERRUPT
    int SQLITE_IOERR
    int SQLITE_LOCKED
    int SQLITE_MISMATCH
    int SQLITE_MISUSE
    int SQLITE_NOMEM
    int SQLITE_NOTFOUND
    int SQLITE_OK
    int SQLITE_PERM
    int SQLITE_PROTOCOL
    int SQLITE_READONLY
    int SQLITE_ROW
    int SQLITE_SCHEMA
    int SQLITE_TOOBIG

class Error(BaseException):
    pass

class InterfaceError(Error):
    pass

class DatabaseError(Error):
    pass

class DataError(DatabaseError):
    pass

class OperationalError(DatabaseError):
    pass

class IntegrityError(DatabaseError):
    pass

class InternalError(DatabaseError):
    pass

class ProgrammingError(DatabaseError):
    pass

class NotSupportedError(DatabaseError):
    pass

cdef class Connection:
    cdef sqlite3* db

    def __init__(self, filename):
        cdef int rc

        rc = sqlite3_open_v2(filename, &self.db, 0, NULL)
        set_error(rc, self, None)

    def cursor(self):
        return Cursor(self)

cdef class Statement:
    pass

cdef set_error(int errorcode, Connection con, Statement st):
    if errorcode == SQLITE_OK:
        return
    msg = None
    if con:
        msg = sqlite3_errmsg(con.db)

    if errorcode == SQLITE_NOTFOUND:
        exc = InternalError
    elif errorcode == SQLITE_NOMEM:
        exc = InternalError
    elif errorcode in (SQLITE_ERROR, SQLITE_PERM, SQLITE_ABORT, SQLITE_BUSY, SQLITE_LOCKED, SQLITE_READONLY, SQLITE_INTERRUPT, SQLITE_IOERR, SQLITE_FULL, SQLITE_CANTOPEN, SQLITE_PROTOCOL, SQLITE_EMPTY, SQLITE_SCHEMA):
        exc = OperationalError
    elif errorcode == SQLITE_CORRUPT:
        exc = DatabaseError
    elif errorcode == SQLITE_TOOBIG:
        exc = DataError
    elif errorcode in (SQLITE_MISMATCH, SQLITE_CONSTRAINT):
        exc = IntegrityError
    elif errorcode == SQLITE_MISUSE:
        exc = ProgrammingError
    else:
        exc = DatabaseError
    raise exc, msg

cdef class Cursor:
    cdef Connection _con
    cdef sqlite3_stmt* _st

    def __init__(self, connection):
        self._con = connection

    def execute(self, sql):
        cdef int rc
        cdef char* tail
        rc = sqlite3_prepare_v2(self._con.db, sql, -1, &self._st, &tail)
        set_error(rc, None, None)

        rc = sqlite3_step(self._st)
        if rc != SQLITE_DONE and rc != SQLITE_ROW:
            set_error(rc, None, None)

    def fetchone(self):
        return (sqlite3_column_text(self._st, 0),)

def connect(filename):
    return Connection(filename)

