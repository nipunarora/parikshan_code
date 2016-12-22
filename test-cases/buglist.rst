+--------------+--------------+-------------+--------------------+------------------+-----------------+-----------------------------------------------+---------+---------+---------+---------+
|ID            |Bug-Name      |Bug-Type     |Explanation         |Non-deterministic/|No.of Nodes      |Triggering mechanism                           |Link/Bug |Resource |Done     |Software |
|              |              |             |                    |Deterministic     |                 |and setup                                      |ID       |         |         |         |
+--------------+--------------+-------------+--------------------+------------------+-----------------+-----------------------------------------------+---------+---------+---------+---------+
|1.            |Apache-1      |Concurrency  |Unprotected buffer  |Non-Deterministic |2 nodes          |                                               |#25520   |AVIO     |    Y    |apache   |
|              |              |Bugs         |length read and     |                  |                 |                                               |apache   |benchmark|         |         |
|              |              |             |write corrupt log   |                  |                 |                                               |bugzilla |         |         |         |
|              |              |             |file (could crash)  |                  |                 |                                               |         |         |         |         |
+--------------+--------------+-------------+--------------------+------------------+-----------------+-----------------------------------------------+---------+---------+---------+---------+
|2.            |Apache-2      |Concurrency  |Atomicity violation |Non-Deterministic |2 nodes          |                                               |#21287   |AVIO     |    Y    |apache   |
|              |              |Bugs         |causes dangling     |                  |                 |                                               |apache   |benchmark|         |         |
|              |              |             |pointer access      |                  |                 |                                               |bugzilla |         |         |         |
+--------------+--------------+-------------+--------------------+------------------+-----------------+-----------------------------------------------+---------+---------+---------+---------+
|3.            |Mysql-1       |Concurrency  |datarace (atomicity |Non-Deterministic |2 nodes          |                                               |bug#791  |AVIO     |    Y    |mysql    |
|              |              |Bugs         |violation) bug      |                  |                 |                                               |         |benchmark|         |         |
+--------------+--------------+-------------+--------------------+------------------+-----------------+-----------------------------------------------+---------+---------+---------+---------+
|4.            |Mysql-2       |Concurrency  |datarace (atomicity |Non-Deterministic |2 nodes          |                                               |bug#644  |AVIO     |    Y    |mysql    |
|              |              |Bugs         |violation) bug      |                  |                 |                                               |         |benchmark|         |         |
|              |              |             |                    |                  |                 |                                               |         |         |         |         |
+--------------+--------------+-------------+--------------------+------------------+-----------------+-----------------------------------------------+---------+---------+---------+---------+
|5.            |Mysql-3       |Concurrency  |Atomicity violation |Non-Deterministic |2 nodes          |                                               |bug#169  |AVIO     |    Y    |mysql    |
|              |              |Bugs         |                    |                  |                 |                                               |         |benchmark|         |         |
+--------------+--------------+-------------+--------------------+------------------+-----------------+-----------------------------------------------+---------+---------+---------+---------+
|6.            |Redis-487     |Semantic bugs|Keys exist despite  |Deterministic     |2 nodes/         |                                               |bug#487  |Aspirator|    Y    |Redis    |
|              |              |             |fulshing            |                  |multi-nodes      |                                               |         |benchmark|         |         |
|              |              |             |                    |                  |                 |                                               |         |         |         |         |
+--------------+--------------+-------------+--------------------+------------------+-----------------+-----------------------------------------------+---------+---------+---------+---------+
|7.            |Cassandra-5225|Semantic bugs|Missing Column      |Deterministic     |2 nodes          |                                               |bug#5225 |         |         |Cassandra|
|              |              |             |error, when         |                  |                 |                                               |         |         |         |         |
|              |              |             |requesting specific |                  |                 |                                               |         |         |         |         |
|              |              |             |column from wide    |                  |                 |                                               |         |         |         |         |
|              |              |             |rows                |                  |                 |                                               |         |         |         |         |
+--------------+--------------+-------------+--------------------+------------------+-----------------+-----------------------------------------------+---------+---------+---------+---------+
|8.            |Cassandra-1837|Semantic bugs|Wrong computation   |Deterministic     |2 nodes/         |                                               |bug#1837 |         |    Y    |cassandra|
|              |              |             |                    |                  |multi-nodes      |                                               |         |         |         |         |
+--------------+--------------+-------------+--------------------+------------------+-----------------+-----------------------------------------------+---------+---------+---------+---------+
|9.            |Redis-614     |Resource Leak|When Master + Slave,|Deterministic     |3 nodes          |0. master + slave                              |bug#614  |         |   Y     |Redis    |
|              |              |             |and lua scripting is|                  |                 |1. brpush/brpop using lua                      |         |         |         |         |
|              |              |             |used, BRPush and    |                  |                 |scripting                                      |         |         |         |         |
|              |              |             |BRPop on a list were|                  |                 |                                               |         |         |         |         |
|              |              |             |not replicated      |                  |                 |                                               |         |         |         |         |
|              |              |             |correctly to slave, |                  |                 |                                               |         |         |         |         |
|              |              |             |and slave's list    |                  |                 |                                               |         |         |         |         |
|              |              |             |(memory) grows      |                  |                 |                                               |         |         |         |         |
|              |              |             |unbounded           |                  |                 |                                               |         |         |         |         |
+--------------+--------------+-------------+--------------------+------------------+-----------------+-----------------------------------------------+---------+---------+---------+---------+
|10.           |Redis-417     |Resource Leak|Memory Leak in      |Deterministic     |2 nodes          |0. start cluster (master + slave)              |bug#417  |         |   Y     |Redis    |
|              |              |             |master server       |                  |                 |1. executes                                    |         |         |         |         |
|              |              |             |                    |                  |                 |two client commands                            |         |         |         |         |
|              |              |             |                    |                  |                 |                                               |         |         |         |         |
|              |              |             |                    |                  |                 |                                               |         |         |         |         |
+--------------+--------------+-------------+--------------------+------------------+-----------------+-----------------------------------------------+---------+---------+---------+---------+
|11.           |Redis-957     |Configuration|Slave cannot sync   |Non Deterministic |2 nodes          |Upload a large db                              |bug#957  |         |N (Could |Redis    |
|              |              |Bugs         |with Master on large|                  |                 |(file write)                                   |         |         |not find |         |
|              |              |             |DB (no replications)|                  |                 |                                               |         |         |bug      |         |
|              |              |             |                    |                  |                 |                                               |         |         |trigger) |         |
+--------------+--------------+-------------+--------------------+------------------+-----------------+-----------------------------------------------+---------+---------+---------+---------+
|12.           |mysql-49491   |Performance  |MD5 and SHA1 hashes |Deterministic     |2 nodes - client,|Just generate a lot                            |         |         |    Y    |mysql    |
|              |              |Bugs         |take longer than    |                  |server           |of MD5 or SHA1                                 |         |         |         |         |
|              |              |             |usual               |                  |                 |hashes from some of                            |         |         |         |         |
|              |              |             |                    |                  |                 |the system table                               |         |         |         |         |
|              |              |             |                    |                  |                 |values and note the                            |         |         |         |         |
|              |              |             |                    |                  |                 |query times                                    |         |         |         |         |
+--------------+--------------+-------------+--------------------+------------------+-----------------+-----------------------------------------------+---------+---------+---------+---------+
|13.           |mysql_15811   |Performance  |Bug caused due to   |Deterministic     |2 nodes:- client,|This bug can be                                |         |         |    Y    |         |
|              |              |Bugs         |multiple calls in a |                  |server           |triggered using                                |         |         |         |         |
|              |              |             |loop when trying to |                  |                 |mysql client ahead                             |         |         |         |         |
|              |              |             |parse a multi-byte  |                  |                 |of our proxy.  The                             |         |         |         |         |
|              |              |             |string              |                  |                 |execution script has                           |         |         |         |         |
|              |              |             |                    |                  |                 |been provided                                  |         |         |         |         |
+--------------+--------------+-------------+--------------------+------------------+-----------------+-----------------------------------------------+---------+---------+---------+---------+
|14.           |Redis-761     |Crash Bugs   |Redis server crashes|Deterministic     |1 node           | zinterstore out 9223372036854775807           |bug#761  |         |   Y     |Redis    |
|              |              |             |on a large integer  |                  |                 |zset zset2                                     |         |         |         |         |
|              |              |             |input to            |                  |                 |                                               |         |         |         |         |
|              |              |             |                    |                  |                 |                                               |         |         |         |         |
+--------------+--------------+-------------+--------------------+------------------+-----------------+-----------------------------------------------+---------+---------+---------+---------+
|15.           |HBASE-9115    |Semantic Bugs|                    |Deterministic     |1 node           |                                               |bug#9115 |         |   Y     |HBASE    |
|              |              |             |                    |                  |                 |                                               |         |         |         |         |
|              |              |             |                    |                  |                 |                                               |         |         |         |         |
|              |              |             |                    |                  |                 |                                               |         |         |         |         |
+--------------+--------------+-------------+--------------------+------------------+-----------------+-----------------------------------------------+---------+---------+---------+---------+
|16.           |HDFS-1904     |Crash Bug and|Trying to create a  | Determinstic     |1 node           |The bug is triggered                           |bug#1904 |         |         |HDFS     |
|              |              |Configuration|child directory in  |                  |                 |when the fsynch                                |         |         |         |         |
|              |              |Bug          |an unexisting parent|                  |                 |interval is set to                             |         |         |         |         |
|              |              |             |directory crashes   |                  |                 |10 seconds                                     |         |         |         |         |
|              |              |             |HDFS NameNode       |                  |                 |                                               |         |         |         |         |
+--------------+--------------+-------------+--------------------+------------------+-----------------+-----------------------------------------------+---------+---------+---------+---------+
|17.           |HDFS-6165     |Semantic Bug |Given a directory   |Deterministic     |1 node           |https://issues.apache.org/jira/browse/HDFS-6165|bug#6165 |         |         |HDFS     |
|              |              |             |owned by user A with|                  |                 |                                               |         |         |         |         |
|              |              |             |WRITE permission    |                  |                 |                                               |         |         |         |         |
|              |              |             |containing an empty |                  |                 |                                               |         |         |         |         |
|              |              |             |directory owned by  |                  |                 |                                               |         |         |         |         |
|              |              |             |user B, it is not   |                  |                 |                                               |         |         |         |         |
|              |              |             |possible to delete  |                  |                 |                                               |         |         |         |         |
|              |              |             |user B's empty      |                  |                 |                                               |         |         |         |         |
|              |              |             |directory with      |                  |                 |                                               |         |         |         |         |
|              |              |             |either "hdfs dfs -rm|                  |                 |                                               |         |         |         |         |
|              |              |             |-r" or "hdfs dfs    |                  |                 |                                               |         |         |         |         |
|              |              |             |-rmdir"             |                  |                 |                                               |         |         |         |         |
+--------------+--------------+-------------+--------------------+------------------+-----------------+-----------------------------------------------+---------+---------+---------+---------+
