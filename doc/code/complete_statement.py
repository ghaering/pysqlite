# A minimal SQLite shell for experiments

from pysqlite2 import dbapi2 as sqlite

con = sqlite.connect(":memory:")
con.isolation_level = None
cur = con.cursor()

buffer = ""

print "Enter your SQL commands to execute in SQLite."
print "Enter a blank line to exit."

while True:
    line = raw_input()
    if line == "":
        break
    buffer += line
    if sqlite.complete_statement(buffer):
        try:
            buffer = buffer.strip()
            cur.execute(buffer)

            if buffer.lstrip().upper().startswith("SELECT"):
                print cur.fetchall()
        except sqlite.Error, e:
            print "An error occured:", e.args[0]
        buffer = ""

con.close()
