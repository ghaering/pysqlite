import time

from pysqlite2 import dbapi2 as sqlite
#import sqlite

def create_db():
    if sqlite.version_info > (2, 0):
        con = sqlite.connect(":memory:", more_types=True)
    else:
        con = sqlite.connect(":memory:")
    cur = con.cursor()
    cur.execute("""
        create table test(v text, f float, i integer)
        """)
    cur.close()
    return con

def test():
    row = ("sdfffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffasfd", 3.14, 42)
    l = []
    for i in range(1000):
        l.append(row)
    
    con = create_db()
    cur = con.cursor()

    if sqlite.version_info > (2, 0):
        sql = "insert into test(v, f, i) values (?, ?, ?)"
    else:
        sql = "insert into test(v, f, i) values (%s, %s, %s)"
    
    starttime = time.time()
    for i in range(50):
        #cur.executemany(sql, l)
        for r in l:
            cur.execute(sql, r)
    endtime = time.time()

    print "elapsed", endtime - starttime

    cur.execute("select count(*) from test")
    print "rows:", cur.fetchone()[0]

if __name__ == "__main__":
    test()

