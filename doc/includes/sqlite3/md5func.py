from pysqlite2 import dbapi2 as sqlite
import hashlib

def md5sum(t):
    return hashlib.md5(bytes(t, "latin1")).hexdigest()

con = sqlite.connect(":memory:")
con.create_function("md5", 1, md5sum)
cur = con.cursor()
cur.execute("select md5(?)", ("foo",))
print(cur.fetchone()[0])
