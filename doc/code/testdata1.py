import pysqlite2.dbapi2 as sqlite
import os

DBFILE = "db"

# Delete the database file
try:
    os.remove(DBFILE)
except OSError:
    pass

data = [
    ("Jennifer", "DeVolta", 30, 'F'),
    ("Daniel", "Olivieri", 49, 'M'),
    ("Frieda", "Kunkel", 82, 'F')
    ]

con = sqlite.connect(DBFILE)
cur = con.cursor()
cur.execute("""
    create table person (
        id integer primary key,
        firstname text,
        lastname text,
        age integer,
        sex char(1),
        image blob,
        married bool,
        weight float,
        birthday date,
        lastseen timestamp,
        overtime interval
    )""")

cur.executemany("insert into person(firstname, lastname, age, sex) values (?, ?, ?, ?)", data)
con.commit()
cur.close()
con.close()
