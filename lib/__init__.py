from _sqlite import connect

paramstyle = "qmark"

threadsafety = 1

apilevel = "2.0"

# Exception objects
from _sqlite import Error, Warning, InterfaceError, DatabaseError,\
    InternalError, OperationalError, ProgrammingError, IntegrityError,\
    DataError, NotSupportedError, STRING, BINARY, NUMBER, DATETIME, ROWID

import datetime, time

Date = datetime.date

Time = datetime.time

Timestamp = datetime.datetime

def DateFromTicks(ticks):
    return apply(Date,time.localtime(ticks)[:3])

def TimeFromTicks(ticks):
    return apply(Time,time.localtime(ticks)[3:6])

def TimestampFromTicks(ticks):
    return apply(Timestamp,time.localtime(ticks)[:6])

Binary = str
