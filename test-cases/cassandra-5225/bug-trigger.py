#!/usr/bin/env python

from random import randint

from pycassa.pool import ConnectionPool
from pycassa.columnfamily import ColumnFamily
from pycassa.system_manager import SystemManager

sys = SystemManager()
need_to_populate = 'Keyspace1' not in sys.list_keyspaces()
if need_to_populate:
    sys.create_keyspace('Keyspace1', 'SimpleStrategy', {'replication_factor': '1'})
    sys.create_column_family('Keyspace1', 'CF1')

try:
    pool = ConnectionPool('Keyspace1')
    CF1 = ColumnFamily(pool, 'CF1')

    if need_to_populate:
        print "inserting ..."
        for i in range(100000):
            cols = dict((str(i * 100 + j), 'value%d' % (i * 100 + j)) for j in range(100))
            CF1.insert('key', cols)
            if i % 1000 == 0:
                print "%d%%" % int(100 * float(i) / 100000)

    print "fetching ..."
    for i in range(1000):
        cols = [str(randint(0, 99999)) for i in range(3)]
        expected = len ( set( cols ) )
        results = CF1.get('key', cols)
        if len(results) != expected:
            print "Requested %s, only got %s" % (cols, results.keys())
except:
    sys.drop_keyspace('Keyspace1')
    raise
