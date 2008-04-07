cdef extern from "sqlite3.h":
    ctypedef struct sqlite3:
        pass
    ctypedef struct sqlite3_stmt:
        pass

    int sqlite3_open_v2(char *filename, sqlite3 **ppDb, int flags, char *zVfs)
    int sqlite3_prepare_v2(sqlite3 *db, char *zSql, int nByte, sqlite3_stmt **ppStmt, char **pzTail)
    int sqlite3_step(sqlite3_stmt*)
    char* sqlite3_column_text(sqlite3_stmt*, int iCol)


cdef class Connection:
    cdef sqlite3* db

    def __init__(self, filename):
        cdef int res

        res = sqlite3_open_v2(filename, &self.db, 0, "")

    def cursor(self):
        return Cursor(self)

cdef class Cursor:
    cdef Connection _con
    cdef sqlite3_stmt* _st

    def __init__(self, connection):
        self._con = connection

    def execute(self, sql):
        cdef int res
        cdef char* tail
        res = sqlite3_prepare_v2(self._con.db, sql, -1, &self._st, &tail)

    def fetchone(self):
        return (sqlite3_column_text(self._st, 0),)


def connect(filename):
    return Connection(filename)
