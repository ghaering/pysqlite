from pysqlite2 import dbapi2 as sqlite

def dict_factory(cursor, row):
    d = {}
    for idx, col in enumerate(cursor.description):
        d[col[0]] = row[idx]
    return d

class DictCursor(sqlite.Cursor):
    def __init__(self, *args, **kwargs):
        sqlite.Cursor.__init__(self, *args, **kwargs)
        self.row_factory = dict_factory

con = sqlite.connect(":memory:")
cur = con.cursor(factory=DictCursor)
cur.execute("select 1 as a")
print cur.fetchone()["a"]
