from pysqlite2 import dbapi2 as sqlite

class Point(object):
    def __init__(self, x, y):
        self.x, self.y = x, y

    def __conform__(self, protocol):
        if protocol is sqlite.PrepareProtocol:
            return "%f;%f" % (self.x, self.y)

con = sqlite.connect(":memory:")
cur = con.cursor()

p = Point(4.0, -3.2)
cur.execute("select ?", (p,))
print cur.fetchone()[0]

