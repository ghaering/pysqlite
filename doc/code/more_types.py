# Demonstrate the advanced type system of pysqlite.

from pysqlite2 import dbapi2 as sqlite
import datetime

con = sqlite.connect("db", more_types=True)

# With more_types it is possible to build an advanced type system for pysqlite2.
# At the moment, there are no default type converters, you will have to register
# them all yourself.

def datetime_from_db(s):
    # Construct a Python datetime object from a TIMESTAMP string like stored in
    # SQLite and constructed by its CURRENT_TIMESTAMP function.
    datepart, timepart = s.split(" ")
    y, m, d = [int(x) for x in datepart.split("-")]
    hh, mm, ss = [int(x) for x in timepart.split(":")]
    return datetime.datetime(y, m, d, hh, mm, ss)

con.register_converter("integer", int)
con.register_converter("float", float)
con.register_converter("bool", bool)
con.register_converter("timestamp", datetime_from_db)

cur = con.cursor()
cur.execute("""
    update person set
        age=?,
        weight=?,
        lastseen=CURRENT_TIMESTAMP
    where firstname='Jennifer'
    """, (31, 63.5))

cur.execute("""
    update person set
        married=?
    where firstname='Jennifer'
    """, (True,))

# The row we're interested in in the database now looks like this:
# cur.execute("select * from person where firstname='Jennifer'")
# (1, 'Jennifer', 'DeVolta', 31, 'F', None, 'True', 63.5, None, '2005-03-02 20:13:37', None)

cur.execute("""
    select firstname, age, sex, weight, married, lastseen
    from person
    where firstname='Jennifer'""")
row = cur.fetchone()

for i, value in enumerate(row):
    print "%20s %20s %20s" % (cur.description[i][0], value, type(value))

"""
           firstname             Jennifer         <type 'str'>
                 age                   31         <type 'int'>
                 sex                    F         <type 'str'>
              weight                 63.5       <type 'float'>
             married                 True        <type 'bool'>
            lastseen  2005-03-02 20:29:26 <type 'datetime.datetime'>
"""

cur.close()
con.close()

