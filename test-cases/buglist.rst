+--------------+-------------+--------------------+------------------+-----------------+--------+--------------------+---------+---------+---------+---------+
|Bug-Name      |Bug-Type     |Explanation         |Non-deterministic/|No.of Nodes      |Overhead|Triggering mechanism|Link/Bug |Resource |Done     |Software |
|              |             |                    |Deterministic     |                 |        |and setup           |ID       |         |         |         |
+--------------+-------------+--------------------+------------------+-----------------+--------+--------------------+---------+---------+---------+---------+
|Apache-1      |Concurrency  |Unprotected buffer  |Non-Deterministic |2 nodes          |        |                    |#25520   |AVIO     |    Y    |apache   |
|              |Bugs         |length read and     |                  |                 |        |                    |apache   |benchmark|         |         |
|              |             |write corrupt log   |                  |                 |        |                    |bugzilla |         |         |         |
|              |             |file (could crash)  |                  |                 |        |                    |         |         |         |         |
+--------------+-------------+--------------------+------------------+-----------------+--------+--------------------+---------+---------+---------+---------+
|Apache-2      |Concurrency  |Atomicity violation |Non-Deterministic |2 nodes          |        |                    |#21287   |AVIO     |    Y    |apache   |
|              |Bugs         |causes dangling     |                  |                 |        |                    |apache   |benchmark|         |         |
|              |             |pointer access      |                  |                 |        |                    |bugzilla |         |         |         |
+--------------+-------------+--------------------+------------------+-----------------+--------+--------------------+---------+---------+---------+---------+
|Mysql-1       |Concurrency  |datarace (atomicity |Non-Deterministic |2 nodes          |        |                    |bug#791  |AVIO     |    Y    |mysql    |
|              |Bugs         |violation) bug      |                  |                 |        |                    |         |benchmark|         |         |
+--------------+-------------+--------------------+------------------+-----------------+--------+--------------------+---------+---------+---------+---------+
|Mysql-2       |Concurrency  |datarace (atomicity |Non-Deterministic |2 nodes          |        |                    |bug#644  |AVIO     |    Y    |mysql    |
|              |Bugs         |violation) bug      |                  |                 |        |                    |         |benchmark|         |         |
|              |             |                    |                  |                 |        |                    |         |         |         |         |
+--------------+-------------+--------------------+------------------+-----------------+--------+--------------------+---------+---------+---------+---------+
|Mysql-3       |Concurrency  |Atomicity violation |Non-Deterministic |2 nodes          |        |                    |bug#169  |AVIO     |    Y    |mysql    |
|              |Bugs         |                    |                  |                 |        |                    |         |benchmark|         |         |
+--------------+-------------+--------------------+------------------+-----------------+--------+--------------------+---------+---------+---------+---------+
|Redis-487     |Semantic bugs|Keys exist despite  |Deterministic     |2 nodes/         |        |                    |bug#487  |Aspirator|    Y    |Redis    |
|              |             |fulshing            |                  |multi-nodes      |        |                    |         |benchmark|         |         |
+--------------+-------------+--------------------+------------------+-----------------+--------+--------------------+---------+---------+---------+---------+
|Cassandra-5225|Semantic bugs|Missing Column      |Deterministic     |2 nodes          |        |                    |bug#5225 |         |         |Cassandra|
|              |             |error, when         |                  |                 |        |                    |         |         |         |         |
|              |             |requesting specific |                  |                 |        |                    |         |         |         |         |
|              |             |column from wide    |                  |                 |        |                    |         |         |         |         |
|              |             |rows                |                  |                 |        |                    |         |         |         |         |
+--------------+-------------+--------------------+------------------+-----------------+--------+--------------------+---------+---------+---------+---------+
|Cassandra-1837|Semantic bugs|Wrong computation   |Deterministic     |2 nodes/         |        |                    |bug#1837 |         |    Y    |cassandra|
|              |             |                    |                  |multi-nodes      |        |                    |         |         |         |         |
+--------------+-------------+--------------------+------------------+-----------------+--------+--------------------+---------+---------+---------+---------+
|Redis-614     |Resource Leak|When Master + Slave,|Deterministic     |3 nodes          |        |0. master + slave   |bug#614  |         |         |Redis    |
|              |             |and lua scripting is|                  |                 |        |(feature start)     |         |         |         |         |
|              |             |used, BRPush and    |                  |                 |        |1. brpush/brpop     |         |         |         |         |
|              |             |BRPop on a list were|                  |                 |        |using lua scripting |         |         |         |         |
|              |             |not replicated      |                  |                 |        |(file write)        |         |         |         |         |
|              |             |correctly to slave, |                  |                 |        |                    |         |         |         |         |
|              |             |and slave's list    |                  |                 |        |                    |         |         |         |         |
|              |             |(memory) grows      |                  |                 |        |                    |         |         |         |         |
|              |             |unbounded           |                  |                 |        |                    |         |         |         |         |
+--------------+-------------+--------------------+------------------+-----------------+--------+--------------------+---------+---------+---------+---------+
|Redis-417     |Resource Leak|Memory Leak in      |Deterministic     |2 nodes          |        |0. start cluster    |bug#417  |         |         |Redis    |
|              |             |master server       |                  |                 |        |(master + slave)    |         |         |         |         |
|              |             |                    |                  |                 |        |1. executes two     |         |         |         |         |
|              |             |                    |                  |                 |        |client commands     |         |         |         |         |
|              |             |                    |                  |                 |        |(feature start)     |         |         |         |         |
+--------------+-------------+--------------------+------------------+-----------------+--------+--------------------+---------+---------+---------+---------+
|Redis-957     |Configuration|Slave cannot sync   |Non Deterministic |2 nodes          |        |Upload a large db   |bug#957  |         |N (Could |Redis    |
|              |Bugs         |with Master on large|                  |                 |        |(file write)        |         |         |not find |         |
|              |             |DB (no replications)|                  |                 |        |                    |         |         |bug      |         |
|              |             |                    |                  |                 |        |                    |         |         |trigger) |         |
+--------------+-------------+--------------------+------------------+-----------------+--------+--------------------+---------+---------+---------+---------+
|mysql-49491   |Performance  |MD5 and SHA1 hashes |                  |2 nodes - client,|        |Just generate a lot |         |         |    Y    |mysql    |
|              |Bugs         |take longer than    |                  |server           |        |of MD5 or SHA1      |         |         |         |         |
|              |             |usual               |                  |                 |        |hashes from some of |         |         |         |         |
|              |             |                    |                  |                 |        |the system table    |         |         |         |         |
|              |             |                    |                  |                 |        |values and note the |         |         |         |         |
|              |             |                    |                  |                 |        |query times         |         |         |         |         |
+--------------+-------------+--------------------+------------------+-----------------+--------+--------------------+---------+---------+---------+---------+
|mysql_15811   |Performance  |Bug caused due to   |                  |2 nodes:- client,|        |This bug can be     |         |         |    Y    |         |
|              |Bugs         |multiple calls in a |                  |server           |        |triggered using     |         |         |         |         |
|              |             |loop when trying to |                  |                 |        |mysql client ahead  |         |         |         |         |
|              |             |parse a multi-byte  |                  |                 |        |of our proxy.  The  |         |         |         |         |
|              |             |string              |                  |                 |        |execution script has|         |         |         |         |
|              |             |                    |                  |                 |        |been provided       |         |         |         |         |
+--------------+-------------+--------------------+------------------+-----------------+--------+--------------------+---------+---------+---------+---------+
|Redis-761     |Crash Bugs   |Redis server crashes|                  |1 node           |        | zinterstore out    |bug#761  |         |   Y     |Redis    |
|              |             |on a large integer  |                  |                 |        |9223372036854775807 |         |         |         |         |
|              |             |input to            |                  |                 |        |zset zset2 (feature |         |         |         |         |
|              |             |                    |                  |                 |        |start)              |         |         |         |         |
+--------------+-------------+--------------------+------------------+-----------------+--------+--------------------+---------+---------+---------+---------+
