pysqlite
========

The original DB-API 2.0 module for SQLite3, developed by Gerhard Häring
primarily between 2004 and 2006.

It was incorporated into the Python standard library with the Python 2.5
release in 2006 as the _sqlite3_ module. For some time, it still received
updates in this repository, both for existing users and to cross-port fixes
between this repo and the Python standard library version.

The version in the Python standard library, however, quickly got a lot more
usage and traction, so this repository became less and less useful. You are
safe to consider this repository mostly unmaintained.

If all of this does not deter you, online documentation can be found
[here](https://pysqlite.readthedocs.org/en/latest/sqlite3.html).

You can get help from the community on the Google Group:
https://groups.google.com/forum/#!forum/python-sqlite

If having the full SQLite3 API exposed to Python is more important than DB-API
2.0 compatibility, then you should use APSW instead:
https://github.com/rogerbinns/apsw/ It is a great piece of software and well
maintained by Roger Binns.
