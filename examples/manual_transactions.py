import sys
import sqlite

# The shared connection object
cx = None

def getCon():
    # All code gets the connection object via this function
    global cx
    return cx

def createSchema():
    # Create the schema and make sure we're not accessing an old, incompatible schema
    cu = getCon().cursor()
    cu.execute("select tbl_name from sqlite_master where type='table' order by tbl_name")
    tables = []
    for row in cu.fetchall():
        tables.append(row.tbl_name)
    if tables != ["customer", "orders"]:
        if tables == []:
            # ok, database is empty
            cu.execute("begin")
            cu.execute("""
                create table customer (
                    cust_id integer primary key,
                    cust_firstname text not null,
                    cust_lastname text not null,
                    cust_no text not null
                )
                """)
            cu.execute("""
                create table orders (
                    ord_id integer primary key,
                    ord_customer int,
                    ord_item text not null,
                    ord_quantity integer
                )
            """)
            cu.execute("commit")
        else:
            print "We have an unknown schema here. Please fix manually."
            sys.exit(1)

def createCustomer(firstname, lastname, customerNo):
    # Create a new customer and return the primary key id.
    cu = getCon().cursor()
    cu.execute("""
        insert into customer(cust_firstname, cust_lastname, cust_no)
            values (%s, %s, %s)
            """, (firstname, lastname, customerNo))
    return cu.lastrowid

def createOrder(cust_id, ord_item, ord_quantity):
    # Create a new order for the customer identified by cust_id and return the
    # primary key of the created order row.
    cu = getCon().cursor()
    cu.execute("""
        insert into orders (ord_customer, ord_item, ord_quantity)
            values (%s, %s, %s)
            """, (cust_id, ord_item, ord_quantity))
    return cu.lastrowid

def deleteOrder(ord_id):
    # Delete an order.
    cu = getCon().cursor()
    cu.execute("delete from order where ord_id=%s", (ord_id,))

def deleteCustomer(cust_id):
    # Delete the customer identified by cust_id and all its orders (recursive
    # delete).

    # So now, finally, here we have an example where you *really* need
    # transactions. We either want this to happen all or not at all. So all of
    # these SQL statements need to be atomic, i. e. we need a transaction here.
    cu = getCon().cursor()

    cu.execute("begin")
    cu.execute("delete from orders where ord_customer=%s", (cust_id,))
    cu.execute("delete from customer where cust_id=%s", (cust_id,))
    cu.execute("commit")

def main():
    global cx
    # Open the connection in autocommit mode, because we believe we have reason
    # to :-/
    cx = sqlite.connect("customerdb", autocommit=1)
    createSchema()

    # Create a customer
    cust_id = createCustomer("Jane", "Doe", "JD0001")

    # Create two orders for the customer
    ord_id = createOrder(cust_id, "White Towel", 2)
    ord_id = createOrder(cust_id, "Blue Cup", 5)

    # Delete the customer, and all her orders.
    deleteCustomer(cust_id)

    cx.close()

if __name__ == "__main__":
    main()
