#-*- coding: ISO-8859-1 -*-
# pysqlite2/dbapi2.py: the DB-API 2.0 interface
#
# Copyright (C) 2004 Gerhard Häring <gh@ghaering.de>
#
# This file is part of pysqlite.
#
# This software is provided 'as-is', without any express or implied
# warranty.  In no event will the authors be held liable for any damages
# arising from the use of this software.
#
# Permission is granted to anyone to use this software for any purpose,
# including commercial applications, and to alter it and redistribute it
# freely, subject to the following restrictions:
#
# 1. The origin of this software must not be misrepresented; you must not
#    claim that you wrote the original software. If you use this software
#    in a product, an acknowledgment in the product documentation would be
#    appreciated but is not required.
# 2. Altered source versions must be plainly marked as such, and must not be
#    misrepresented as being the original software.
# 3. This notice may not be removed or altered from any source distribution.

from pysqlite2._sqlite import connect

paramstyle = "qmark"

threadsafety = 1

apilevel = "2.0"

# Exception objects
from pysqlite2._sqlite import Error, Warning, InterfaceError, DatabaseError,\
    InternalError, OperationalError, ProgrammingError, IntegrityError,\
    DataError, NotSupportedError, STRING, BINARY, NUMBER, DATETIME, ROWID,\
    version, sqlite_version, ISQLQuote

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

_major, _minor, _micro = version.split(".")
version_info = (int(_major), int(_minor), _micro)
_major, _minor, _micro = sqlite_version.split(".")
sqlite_version_info = (int(_major), int(_minor), _micro)

Binary = buffer
