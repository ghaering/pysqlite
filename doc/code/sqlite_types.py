# Demonstrate the (primitive) type system of SQLite, which is sufficient for
# some use cases.

from pysqlite2 import dbapi2 as sqlite

# Omitting the advanced_types parameter in the .connect() call means leaving it
# at its default False:
con = sqlite.connect("db")

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
             married                 True         <type 'str'>
            lastseen  2005-03-02 20:18:31         <type 'str'>
"""

cur.close()
con.close()


