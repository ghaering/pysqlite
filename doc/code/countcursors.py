from pysqlite2 import dbapi2 as sqlite

class CountCursorsConnection(sqlite.Connection):
    def __init__(self, *args, **kwargs):
        sqlite.Connection.__init__(self, *args, **kwargs)
        self.numcursors = 0

    def cursor(self, *args, **kwargs):
        self.numcursors += 1
        return sqlite.Connection.cursor(self, *args, **kwargs)

con = sqlite.connect(":memory:", factory=CountCursorsConnection)
cur1 = con.cursor()
cur2 = con.cursor()
print con.numcursors
