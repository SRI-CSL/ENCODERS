#!/usr/bin/python
# -*- coding: utf-8 -*-

import sqlite3 as lite
import sys

def print_usage():
    print "./ls_db.py n1.haggle.db"

def pretty_hash(rawhashid):
    if 20 != len(rawhashid):
        print "invalid size of hash"
        return "0"
    return str(rawhashid).encode("hex")

def main():
    con = None

    if len(sys.argv) != 2:
        print "Invalid parameters. %d != 2" % len(sys.argv)
        print_usage()
        sys.exit(2)

    db_name = sys.argv[1]
    try:
        con = lite.connect(db_name)
    
        cur = con.cursor()    
        cur.execute('SELECT id, datalen FROM table_dataobjects')
    
        rows = cur.fetchall()
        for hashid, size in rows:
            print pretty_hash(hashid), size
    
    except lite.Error, e:
        print "Error %s:" % e.args[0]
        sys.exit(1)
    
    finally:
        if con:
            con.close()

if __name__ == "__main__":
    main()
