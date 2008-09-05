from pysqlite2 import dbapi2 as sqlite3

con = sqlite3.connect(":memory:")

# Load the fulltext search extension
con.enable_load_extension(True)
con.execute("select load_extension('./fts3.so')")
con.enable_load_extension(False)

# example from SQLite wiki
con.execute("create virtual table recipe using fts3(name, ingredients)")
con.executescript("""
    insert into recipe (name, ingredients) values ('broccoli stew', 'broccoli peppers cheese tomatoes');
    insert into recipe (name, ingredients) values ('pumpkin stew', 'pumpkin onions garlic celery');
    insert into recipe (name, ingredients) values ('broccoli pie', 'broccoli cheese onions flour');
    insert into recipe (name, ingredients) values ('pumpkin pie', 'pumpkin sugar flour butter');
    """)
for row in con.execute("select rowid, name, ingredients from recipe where name match 'pie'"):
    print row


