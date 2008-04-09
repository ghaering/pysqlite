import pysqlite2.dbapi2 as sqlite

con = sqlite.connect("/foo/bar/baz")
# con = sqlite.connect(":memory:")
# con = sqlite.connect(":memory:")
cur = con.cursor()
cur.execute("select 4 union select 5")
print cur.fetchone()
