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
# -*- coding: utf-8 -*-

import _mysql
import sys

try:
    con = _mysql.connect('localhost', 'nipun', 'pipoduya', 'test')
        
    con.query("SELECT VERSION()")
    result = con.use_result()
    
    print "MySQL version: %s" % \
        result.fetch_row()[0]
    
except _mysql.Error, e:
  
    print "Error %d: %s" % (e.args[0], e.args[1])
    sys.exit(1)

finally:
    
    if con:
        con.close()
