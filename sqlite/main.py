from __future__ import nested_scopes
import _sqlite

import copy, new, sys, time, weakref
from types import *

try:
    from mx import DateTime
    have_datetime = 1
except ImportError:
    have_datetime = 0

if have_datetime:
    # Make the required Date/Time constructor visable in the PySQLite module.
    Date = DateTime.Date
    Time = DateTime.Time
    Timestamp = DateTime.Timestamp
    DateFromTicks = DateTime.DateFromTicks
    TimeFromTicks = DateTime.TimeFromTicks
    TimestampFromTicks = DateTime.TimestampFromTicks

    # And also the DateTime types
    DateTimeType = DateTime.DateTimeType
    DateTimeDeltaType = DateTime.DateTimeDeltaType

class DBAPITypeObject:
    def __init__(self,*values):
        self.values = values

    def __cmp__(self,other):
        if other in self.values:
            return 0
        if other < self.values:
            return 1
        else:
            return -1

def _quote(value):
    """_quote(value) -> string

    This function transforms the Python value into a string suitable to send to
    the SQLite database in a SQL statement.  This function is automatically
    applied to all parameters sent with an execute() call.  Because of this a
    SQL statement string in an execute() call should only use '%s' [or
    '%(name)s'] for variable substitution without any quoting."""

    if value is None:
        return 'NULL'
    elif type(value) in (IntType, LongType, FloatType):
        return value
    elif isinstance(value, StringType):
        return "'%s'" % value.replace("'", "''")
    elif hasattr(value, '__quote__'):
        return value.__quote__()
    elif hasattr(value, '_quote'):
        return value._quote()
    elif have_datetime and type(value) in \
            (DateTime.DateTimeType, DateTime.DateTimeDeltaType):
        return "'%s'" % value
    else:
        return repr(value)

def _quoteall(vdict):
    """_quoteall(vdict)->dict
    Quotes all elements in a list or dictionary to make them suitable for
    insertion in a SQL statement."""

    if type(vdict) is DictType or isinstance(vdict, PgResultSet):
        t = {}
        for k, v in vdict.items():
            t[k]=_quote(v)
    elif isinstance(vdict, StringType) or isinstance(vdict, UnicodeType):
        # Note: a string is a SequenceType, but is treated as a single
        #    entity, not a sequence of characters.
        t = (_quote(vdict), )
    elif type(vdict)in (ListType, TupleType):
        t = tuple(map(_quote, vdict))
    else:
        raise TypeError, \
              "argument to _quoteall must be a sequence or dictionary!"

    return t

class PgResultSet:
    """A DB-API query result set for a single row.
    This class emulates a sequence with the added feature of being able to
    reference a column as attribute or with dictionary access in addition to a
    zero-based numeric index."""

    def __init__(self, value):
        self.__dict__['baseObj'] = value

    def __getattr__(self, key):
        key = key.upper()
        if self._xlatkey.has_key(key):
            return self.baseObj[self._xlatkey[key]]
        raise AttributeError, key

    def __len__(self):
        return len(self.baseObj)

    def __getitem__(self, key):
        if isinstance(key, StringType):
            key = self.__class__._xlatkey[key.upper()]
        return self.baseObj[key]

    def __contains__(self, key):
        return self.has_key(key)

    def __getslice__(self, i, j):
        klass = make_PgResultSetClass(self._desc_[i:j])
        obj = klass(self.baseObj[i:j])
        return obj

    def __repr__(self):
        return repr(self.baseObj)

    def __str__(self):
        return str(self.baseObj)

    def __cmp__(self, other):
        return cmp(self.baseObj, other)

    def description(self):
        return self._desc_

    def keys(self):
        _k = []
        for _i in self._desc_:
            _k.append(_i[0])
        return _k

    def values(self):
        return self.baseObj[:]

    def items(self):
        _items = []
        for i in range(len(self.baseObj)):
            _items.append((self._desc_[i][0], self.baseObj[i]))

        return _items

    def has_key(self, key):
        return self._xlatkey.has_key(key.upper())

    def get(self, key, defaultval=None):
        if self.has_key(key):
            return self[key]
        else:
            return defaultval

def make_PgResultSetClass(description):
    NewClass = new.classobj("PgResultSetConcreteClass", (PgResultSet,), {})
    NewClass.__dict__['_desc_'] = description

    NewClass.__dict__['_xlatkey'] = {}

    for _i in range(len(description)):
        NewClass.__dict__['_xlatkey'][description[_i][0].upper()] = _i

    return NewClass

class Cursor:
    """Abstract cursor class implementing what all cursor classes have in
    common."""

    def __init__(self, conn, rowclass=PgResultSet):
        self.arraysize = 1

        # Add ourselves to the list of cursors for our owning connection.
        self.con = weakref.proxy(conn)
        self.con.cursors[id(self)] = self

        self.rowclass = rowclass

        self._reset()
        self.current_recnum = -1

    def _reset(self):
        # closed is a trinary variable:
        #     == None => Cursor has not been opened.
        #     ==    0 => Cursor is open.
        #     ==    1 => Cursor is closed.
        self.closed = None
        self.rowcount = -1
        self._real_rowcount = 0
        self.description = None
        self.rs = None
        self.current_recnum = 0

    def _checkNotClosed(self, methodname=None):
        if self.closed:
            raise _sqlite.ProgrammingError, \
                "%s failed - the cursor is closed." % (methodname or "")

    def _unicodeConvert(self, obj):
        """Encode all unicode strings that can be found in obj into
        byte-strings using the encoding specified in the connection's
        constructor, available here as self.con.encoding."""

        if isinstance(obj, StringType):
            return obj
        elif isinstance(obj, UnicodeType):
            return obj.encode(*self.con.encoding)
        elif isinstance(obj, ListType) or isinstance(obj, TupleType):
            converted_obj = []
            for item in obj:
                if isinstance(item, UnicodeType):
                    converted_obj.append(item.encode(*self.con.encoding))
                else:
                    converted_obj.append(item)
            return converted_obj
        elif isinstance(obj, DictType):
            converted_obj = {}
            for k, v in obj.items():
                if isinstance(v, UnicodeType):
                    converted_obj[k] = v.encode(*self.con.encoding)
                else:
                    converted_obj[k] = v
            return converted_obj
        elif isinstance(obj, PgResultSet):
            obj = copy.copy(obj)
            for k, v in obj.items():
                if isinstance(v, UnicodeType):
                    obj[k] = v.encode(*self.con.encoding)
            return obj
        else:
            return obj

    def execute(self, SQL, *parms):
        self._checkNotClosed("execute")

        if self.con.autocommit:
            pass
        else:
            if not(self.con.inTransaction or SQL[:6].upper() in ("SELECT","VACUUM","DETACH")):
                self.con._begin()
                self.con.inTransaction = 1

        SQL = self._unicodeConvert(SQL)

        if len(parms) == 0:
            # If there are no paramters, just execute the query.
            self.rs = self.con.db.execute(SQL)
        else:
            if len(parms) == 1 and \
               (type(parms[0]) in (DictType, ListType, TupleType) or \
                        isinstance(parms[0], PgResultSet)):
                parms = (self._unicodeConvert(parms[0]),)
                parms = _quoteall(parms[0])
            else:
                parms = self._unicodeConvert(parms)
                parms = tuple(map(_quote, parms))

            self.rs = self.con.db.execute(SQL % parms)

        self.closed = 0
        self.current_recnum = 0

        self.rowcount, self._real_rowcount = [len(self.rs.row_list)] * 2
        if self.rowcount == 0 and \
         SQL.lstrip()[:6].upper() in ("INSERT", "UPDATE", "DELETE"):
            self.rowcount = self.con.db.sqlite_changes()

        self.description = self.rs.col_defs

        if issubclass(self.rowclass, PgResultSet):
            self.rowclass = make_PgResultSetClass(self.description[:])

    def executemany(self, query, parm_sequence):
        self._checkNotClosed("executemany")

        if self.con is None:
            raise _sqlite.ProgrammingError, "connection is closed."

        for _i in parm_sequence:
            self.execute(query, _i)

    def close(self):
        if self.con and self.con.closed:
            raise _sqlite.ProgrammingError, \
                  "This cursor's connection is already closed."
        if self.closed:
            raise _sqlite.ProgrammingError, \
                  "This cursor is already closed."
        self.closed = 1

        # Disassociate ourselves from our connection.
        try:
            cursors = self.con.cursors
            del cursors.data[id(self)]
        except:
            pass

    def __del__(self):
        # Disassociate ourselves from our connection.
        try:
            cursors = self.con.cursors
            del cursors.data[id(self)]
        except:
            pass

    def setinputsizes(self, sizes):
        """Does nothing, required by DB API."""
        self._checkNotClosed("setinputsize")

    def setoutputsize(self, size, column=None):
        """Does nothing, required by DB API."""
        self._checkNotClosed("setinputsize")

    #
    # DB-API methods:
    #

    def fetchone(self):
        self._checkNotClosed("fetchone")

        # If there are no records
        if self._real_rowcount == 0:
            return None

        # If we have reached the last record
        if self.current_recnum >= self._real_rowcount:
            return None

        if type(self.rowclass) is TupleType:
            retval = self.rs.row_list[self.current_recnum]
        else:
            retval = self.rowclass(self.rs.row_list[self.current_recnum])
        self.current_recnum += 1

        return retval

    def fetchmany(self, howmany=None):
        self._checkNotClosed("fetchmany")

        if howmany is None:
            howmany = self.arraysize

        # If there are no records
        if self._real_rowcount == 0:
            return []

        # If we have reached the last record
        if self.current_recnum >= self._real_rowcount:
            return []

        if type(self.rowclass) is TupleType:
            retval = self.rs.row_list[self.current_recnum:self.current_recnum + howmany]
        else:
            retval = [self.rowclass(row) for row in self.rs.row_list[self.current_recnum:self.current_recnum + howmany]]

        self.current_recnum += howmany
        if self.current_recnum > self._real_rowcount:
            self.current_recnum = self._real_rowcount

        return retval

    def fetchall(self):
        self._checkNotClosed("fetchall")

        # If there are no records
        if self._real_rowcount == 0:
            return []

        # If we have reached the last record
        if self.current_recnum >= self._real_rowcount:
            return []

        if type(self.rowclass) is TupleType:
            retval = self.rs.row_list[self.current_recnum:]
        else:
            retval = [self.rowclass(row) for row in self.rs.row_list[self.current_recnum:]]

        self.current_recnum =self._real_rowcount

        return retval

    #
    # Optional DB-API extensions from PEP 0249:
    #

    def __iter__(self):
        return self

    def next(self):
        item = self.fetchone()
        if item is None:
            if sys.version_info[:2] >= (2,2):
                raise StopIteration
            else:
                raise IndexError
        else:
            return item

    def scroll(self, value, mode="relative"):
        if mode == "relative":
            new_recnum = self.current_recnum + value
        elif mode == "absolute":
            new_recnum = value
        else:
            raise ValueError, "invalid mode parameter"
        if new_recnum >= 0 and new_recnum < self.rowcount:
            self.current_recnum = new_recnum
        else:
            raise IndexError

    def __getattr__(self, key):
        if self.__dict__.has_key(key):
            return self.__dict__[key]
        elif key == "sql":
            # The sql attribute is a PySQLite extension.
            return self.con.db.sql
        elif key == "rownumber":
            return self.current_recnum
        elif key == "lastrowid":
            return self.con.db.sqlite_last_insert_rowid()
        elif key == "connection":
            return self.con
        else:
            raise AttributeError, key

class UnicodeConverter:
    def __init__(self, encoding):
        self.encoding = encoding

    def __call__(self, val):
        return unicode(val, *self.encoding)

class Connection:

    def __init__(self, database=None, mode=0755, converters={}, autocommit=0, encoding=None, timeout=None, command_logfile=None, *arg, **kwargs):
        # Old parameter names, for backwards compatibility
        database = database or kwargs.get("db")
        encoding = encoding or kwargs.get("client_encoding")

        # Set these here, to prevent an attribute access error in __del__
        # in case the connect fails.
        self.closed = 0
        self.db = None
        self.inTransaction = 0
        self.autocommit = autocommit
        self.cursors = weakref.WeakValueDictionary()
        self.rowclass = PgResultSet

        self.db = _sqlite.connect(database, mode)

        if type(encoding) not in (TupleType, ListType):
            self.encoding = (encoding or sys.getdefaultencoding(),)
        else:
            self.encoding = encoding

        register = self.db.register_converter
        # These are the converters we provide by default ...
        register("str", str)
        register("int", int)
        register("long", long)
        register("float", float)
        register("unicode", UnicodeConverter(self.encoding))
        register("binary", _sqlite.decode)

        # ... and DateTime/DateTimeDelta, if we have the mx.DateTime module.
        if have_datetime:
            register("date", DateTime.DateFrom)
            register("time", DateTime.TimeFrom)
            register("timestamp", DateTime.DateTimeFrom)
            register("interval", DateTime.DateTimeDeltaFrom)

        for typename, conv in converters.items():
            register(typename, conv)

        if timeout is not None:
            def busy_handler(timeout, table, count):
                if count == 1:
                    busy_handler.starttime = time.time()
                elapsed_time = time.time() - busy_handler.starttime
                print elapsed_time, timeout
                if elapsed_time > timeout:
                    return 0
                else:
                    time_to_sleep = 0.01 * (2 << min(5, count))
                    time.sleep(time_to_sleep)
                return 1

            self.db.sqlite_busy_handler(busy_handler, timeout/1000.0)

        self.db.set_command_logfile(command_logfile)

    def __del__(self):
        if not self.closed:
            self.close()

    def _checkNotClosed(self, methodname):
        if self.closed:
            raise _sqlite.ProgrammingError, \
                  "%s failed - Connection is closed." % methodname

    def __anyCursorsLeft(self):
        return len(self.cursors.data.keys()) > 0

    def __closeCursors(self, doclose=0):
        """__closeCursors() - closes all cursors associated with this connection"""
        if self.__anyCursorsLeft():
            cursors = map(lambda x: x(), self.cursors.data.values())

            for cursor in cursors:
                try:
                    if doclose:
                        cursor.close()
                    else:
                        cursor._reset()
                except weakref.ReferenceError:
                    pass

    def _begin(self):
        self.db.execute("BEGIN")
        self.inTransaction = 1

    #
    # PySQLite extensions:
    #

    def create_function(self, name, nargs, func):
        self.db.create_function(name, nargs, func)

    def create_aggregate(self, name, nargs, agg_class):
        self.db.create_aggregate(name, nargs, agg_class)

    #
    # DB-API methods:
    #

    def commit(self):
        self._checkNotClosed("commit")
        if self.autocommit:
            # Ignore .commit(), according to the DB-API spec.
            return

        if self.inTransaction:
            self.db.execute("COMMIT")
            self.inTransaction = 0

    def rollback(self):
        self._checkNotClosed("rollback")
        if self.autocommit:
            raise _sqlite.ProgrammingError, "Rollback failed - autocommit is on."

        if self.inTransaction:
            self.db.execute("ROLLBACK")
            self.inTransaction = 0

    def close(self):
        self._checkNotClosed("close")

        self.__closeCursors(1)

        if self.inTransaction:
            self.rollback()

        if self.db:
            self.db.close()
        self.closed = 1

    def cursor(self):
        self._checkNotClosed("cursor")
        return Cursor(self, self.rowclass)

    #
    # Optional DB-API extensions from PEP 0249:
    #

    def __getattr__(self, key):
        if key in self.__dict__.keys():
            return self.__dict__[key]
        elif key in ('IntegrityError', 'InterfaceError', 'InternalError',
                     'NotSupportedError', 'OperationalError',
                     'ProgrammingError', 'Warning'):
            return getattr(_sqlite, key)
        else:
            raise AttributeError, key

    #
    # MySQLdb compatibility stuff
    #

    def insert_id(self):
        return self.db.sqlite_last_insert_rowid()
