from pysqlite2 import dbapi2 as sqlite
import datetime

con = sqlite.connect(":memory:", detect_types=sqlite.PARSE_COLNAMES)
cur = con.cursor()
cur.execute('select ? as "x [timestamp]"', (datetime.datetime.now(),))
dt = cur.fetchone()[0]
print dt, type(dt)
