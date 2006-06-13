from pysqlite2 import dbapi2 as sqlite

def authorizer_callback(action, arg1, arg2, dbname, source):
    if action != sqlite.SQLITE_SELECT:
        return sqlite.SQLITE_DENY
    if arg1 == "private_table":
        return sqlite.SQLITE_DENY
    return sqlite.SQLITE_OK

con = sqlite.connect(":memory:")
con.executescript("""
    create table public_table(c1, c2);
    create table private_table(c1, c2);
    """)
con.set_authorizer(authorizer_callback)

try:
    con.execute("select * from private_table")
except sqlite.DatabaseError, e:
    print "SELECT FROM private_table =>", e.args[0]     # access ... prohibited

try:
    con.execute("insert into public_table(c1, c2) values (1, 2)")
except sqlite.DatabaseError, e:
    print "DML command =>", e.args[0]     # access ... prohibited

