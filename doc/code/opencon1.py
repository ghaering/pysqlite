import pysqlite2.dbapi2 as sqlite

# Create a database connection
con1 = sqlite.connect("mydb.db")

# Create a database connection to an in-memory database
con2 = sqlite.connect(":memory:")

