# Import the module
import pysqlite2.dbapi2 as sqlite

# Open the connection
con = sqlite.connect("db")

con.register_converter("TEXT", str, 1)

# Create the cursor
cur = con.cursor()

# Execute the simple query
cur.execute("select id, firstname, lastname, age, sex from person")

# (It's always a good idea to spell out all columns, instead of using SELECT *,
# because then your code will continue to work, even if a new column is
# inserted, or an obsolete one gets deleted from the schema.)

# Iterate over the results, and print them
for row in cur:
    print row

# This will print the following:
#     ('1', 'Jennifer', 'DeVolta', '30', 'F')
#     ('2', 'Daniel', 'Olivieri', '49', 'M')
#     ('3', 'Frieda', 'Kunkel', '82', 'F')

# Now we need to clean up

# Close the cursor
cur.close()

# Close the connection
con.close()
