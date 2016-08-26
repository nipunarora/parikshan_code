+--------------+-------------+-----------------+------------------+-----------------+--------+-------------+---------+---------+---------+
|Bug-Name      |Bug-Type     |Explanation      |Non-deterministic/|No.of            |Overhead|Triggering   |Link/Bug |Resource |Software |
|              |             |                 |Deterministic     |Nodes            |        |mechanism and|ID       |         |         |
|              |             |                 |                  |                 |        |setup        |         |         |         |
+--------------+-------------+-----------------+------------------+-----------------+--------+-------------+---------+---------+---------+
|Apache-1      |Concurrency  |Unprotected      |Non-Deterministic |2 nodes          |        |             |#25520   |AVIO     |apache   |
|              |Bugs         |buffer length    |                  |                 |        |             |apache   |benchmark|         |
|              |             |read and write   |                  |                 |        |             |bugzilla |         |         |
|              |             |corrupt log file |                  |                 |        |             |         |         |         |
|              |             |(could crash)    |                  |                 |        |             |         |         |         |
+--------------+-------------+-----------------+------------------+-----------------+--------+-------------+---------+---------+---------+
|Apache-2      |Concurrency  |Atomicity        |Non-Deterministic |2 nodes          |        |             |#21287   |AVIO     |apache   |
|              |Bugs         |violation causes |                  |                 |        |             |apache   |benchmark|         |
|              |             |dangling pointer |                  |                 |        |             |bugzilla |         |         |
|              |             |access           |                  |                 |        |             |         |         |         |
+--------------+-------------+-----------------+------------------+-----------------+--------+-------------+---------+---------+---------+
|Mysql-1       |Concurrency  |datarace         |Non-Deterministic |2 nodes          |        |             |bug#791  |AVIO     |mysql    |
|              |Bugs         |(atomicity       |                  |                 |        |             |         |benchmark|         |
|              |             |violation) bug   |                  |                 |        |             |         |         |         |
+--------------+-------------+-----------------+------------------+-----------------+--------+-------------+---------+---------+---------+
|Mysql-2       |Concurrency  |datarace         |Non-Deterministic |2 nodes          |        |             |bug#644  |AVIO     |mysql    |
|              |Bugs         |(atomicity       |                  |                 |        |             |         |benchmark|         |
|              |             |violation) bug   |                  |                 |        |             |         |         |         |
+--------------+-------------+-----------------+------------------+-----------------+--------+-------------+---------+---------+---------+
|Mysql-3       |Concurrency  |Atomicity        |Non-Deterministic |2 nodes          |        |             |bug#169  |AVIO     |mysql    |
|              |Bugs         |violation        |                  |                 |        |             |         |benchmark|         |
+--------------+-------------+-----------------+------------------+-----------------+--------+-------------+---------+---------+---------+
|Redis-487     |Semantic bugs|Keys exist       |Deterministic     |2 nodes/         |        |             |bug#487  |         |Redis    |
|              |             |despite fulshing |                  |multi-nodes      |        |             |         |         |         |
|              |             |                 |                  |                 |        |             |         |         |         |
+--------------+-------------+-----------------+------------------+-----------------+--------+-------------+---------+---------+---------+
|Cassandra-1837|Semantic bugs|Wrong computation|Deterministic     |2 nodes/         |        |             |bug#1837 |         |cassandra|
|              |             |                 |                  |multi-nodes      |        |             |         |         |         |
|              |             |                 |                  |                 |        |             |         |         |         |
+--------------+-------------+-----------------+------------------+-----------------+--------+-------------+---------+---------+---------+
|              |Resource     |                 |                  |                 |        |             |         |         |         |
|              |Leak         |                 |                  |                 |        |             |         |         |         |
+--------------+-------------+-----------------+------------------+-----------------+--------+-------------+---------+---------+---------+
|              |Configuration|                 |                  |                 |        |             |         |         |         |
|              |Bugs         |                 |                  |                 |        |             |         |         |         |
+--------------+-------------+-----------------+------------------+-----------------+--------+-------------+---------+---------+---------+
|mysql-49491   |Performance  |MD5 and SHA1     |                  |2 nodes - client,|        |Just generate|         |         |         |
|              |Bugs         |hashes take      |                  |server           |        |a lot of MD5 |         |         |         |
|              |             |longer than usual|                  |                 |        |or SHA1      |         |         |         |
|              |             |                 |                  |                 |        |hashes from  |         |         |         |
|              |             |                 |                  |                 |        |some of the  |         |         |         |
|              |             |                 |                  |                 |        |system table |         |         |         |
|              |             |                 |                  |                 |        |values and   |         |         |         |
|              |             |                 |                  |                 |        |note the     |         |         |         |
|              |             |                 |                  |                 |        |query times  |         |         |         |
+--------------+-------------+-----------------+------------------+-----------------+--------+-------------+---------+---------+---------+
|mysql_15811   |Performance  |Bug caused due to|                  |2 nodes:-        |        |This bug can |         |         |         |
|              |Bugs         |multiple calls in|                  |client,          |        |be triggered |         |         |         |
|              |             |a loop when      |                  |server           |        |using mysql  |         |         |         |
|              |             |trying to parse a|                  |                 |        |client ahead |         |         |         |
|              |             |multi-byte string|                  |                 |        |of our proxy.|         |         |         |
|              |             |                 |                  |                 |        |The execution|         |         |         |
|              |             |                 |                  |                 |        |script has   |         |         |         |
|              |             |                 |                  |                 |        |been provided|         |         |         |
+--------------+-------------+-----------------+------------------+-----------------+--------+-------------+---------+---------+---------+
