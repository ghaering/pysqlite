from pysqlite2 import dbapi2 as sqlite
import datetime, time

def adapt_datetime(ts):
    return time.mktime(ts.timetuple())

sqlite.register_adapter(datetime.datetime, adapt_datetime)

con = sqlite.connect(":memory:")
cur = con.cursor()

now = datetime.datetime.now()
cur.execute("select ?", (now,))
print cur.fetchone()[0]
