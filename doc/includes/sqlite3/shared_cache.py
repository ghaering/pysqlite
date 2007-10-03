from pysqlite2 import dbapi2 as sqlite

# The shared cache is only available in SQLite versions 3.3.3 or later
# See the SQLite documentaton for details.

sqlite.enable_shared_cache(True)
