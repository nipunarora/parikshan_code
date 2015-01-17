# 
# 
# Filename: mysqlclient.py
# Author: Nipun Arora
# Created: Sat Jan 17 01:33:43 2015 (-0500)
# URL: http://www.nipunarora.net 
# 
# Description: 
# 

#!/usr/bin/python

import MySQLdb as mdb

con = mdb.connect('192.168.1.106', 'nipun', 'pipoduya', 'test')

with con:
    
    cur = con.cursor()
    cur.execute("SHOW TABLES")

    cur = con.cursor()
    cur.execute("select * from t1")

    rows = cur.fetchall()

    for row in rows:
        print row
