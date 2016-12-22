
Performance Bug in 5.1.14-log

url:http://bugs.mysql.com/bug.php?id=26527

Recreation mechanism:

ght:~/dbs$ cd 5.1
miguel@light:~/dbs/5.1$ bin/mysql -uroot
Welcome to the MySQL monitor.  Commands end with ; or \g.
Your MySQL connection id is 1
Server version: 5.1.16-beta-debug Source distribution

Type 'help;' or '\h' for help. Type '\c' to clear the buffer.

mysql> create database db1;
Query OK, 1 row affected (0.03 sec)

mysql> use db1
Database changed
mysql> CREATE TABLE t1 (
    ->    f1 int(10) unsigned NOT NULL DEFAULT '0',
    ->    f2 int(10) unsigned DEFAULT NULL,
    ->    f3 char(33) CHARACTER SET latin1 NOT NULL DEFAULT '',
    ->    f4 char(15) DEFAULT NULL,
    ->    f5 datetime NOT NULL DEFAULT '0000-00-00 00:00:00',
    ->    f6 char(40) CHARACTER SET latin1 DEFAULT NULL,
    ->    f7 text CHARACTER SET latin1,
    ->   KEY f1_idx (f1),
    ->   KEY f5_idx (f5)
    -> ) ENGINE=MyISAM DEFAULT CHARSET=utf8 /*!50100 PARTITION BY RANGE
    -> (month(f5)) (PARTITION m1 VALUES LESS THAN (2) ENGINE = MyISAM, PARTITION
    -> m2 VALUES LESS THAN (3) ENGINE = MyISAM, PARTITION m3 VALUES LESS THAN (4)
    -> ENGINE = MyISAM, PARTITION m4 VALUES LESS THAN (5) ENGINE = MyISAM, PARTITION m5
    -> VALUES LESS THAN (6) ENGINE = MyISAM, PARTITION m6 VALUES LESS THAN (7) ENGINE =
    -> MyISAM, PARTITION m7 VALUES LESS THAN (8) ENGINE = MyISAM, PARTITION m8 VALUES
    -> LESS THAN (9) ENGINE = MyISAM, PARTITION m9 VALUES LESS THAN (10) ENGINE =
    -> MyISAM, PARTITION m10 VALUES LESS THAN (11) ENGINE = MyISAM, PARTITION m11
    -> VALUES LESS THAN (12) ENGINE = MyISAM, PARTITION m12 VALUES LESS THAN (13)
    -> ENGINE = MyISAM) */
    -> ;
Query OK, 0 rows affected (0.01 sec)

mysql> load data infile '/home/miguel/fill.sql' into table t1 fields terminated by ',' lines terminated by '\n';
Query OK, 10000000 rows affected (13 min 40.31 sec)
Records: 10000000  Deleted: 0  Skipped: 0  Warnings: 0

mysql> select * from t1 limit 1\G
*************************** 1. row ***************************
f1: 1
f2: 2
f3: some text for f3
f4: some text f4
f5: 2007-02-25 02:00:39
f6: some text f6
f7: some text f7
1 row in set (0.14 sec)

mysql> drop table t1;
Query OK, 0 rows affected (1.83 sec)

mysql> CREATE TABLE t1 (
    ->    f1 int(10) unsigned NOT NULL DEFAULT '0',
    ->    f2 int(10) unsigned DEFAULT NULL,
    ->    f3 char(33) CHARACTER SET latin1 NOT NULL DEFAULT '',
    ->    f4 char(15) DEFAULT NULL,
    ->    f5 datetime NOT NULL DEFAULT '0000-00-00 00:00:00',
    ->    f6 char(40) CHARACTER SET latin1 DEFAULT NULL,
    ->    f7 text CHARACTER SET latin1,
    ->   KEY f1_idx (f1),
    ->   KEY f5_idx (f5)
    -> ) ENGINE=MyISAM DEFAULT CHARSET=utf8;
Query OK, 0 rows affected (0.02 sec)

mysql> load data infile '/home/miguel/fill.sql' into table t1 fields terminated by ',' lines terminated by '\n';
Query OK, 10000000 rows affected (5 min 29.10 sec)
Records: 10000000  Deleted: 0  Skipped: 0  Warnings: 0

mysql> exit
Bye
miguel@light:~/dbs/5.1$ cat /etc/issue
Ubuntu 6.10 \n \l

#include <iostream>
#include <fstream>
#include <ctime>

using namespace std;

void main()
{
  ofstream out("c:\\fill.sql");
  struct tm * tinfo;
  time_t rt;
  char tmb[20];

  for (int n = 1; n <= 10000000; n++)
  {
   time( &rt );
   tinfo = localtime ( &rt );
   strftime (tmb,20,"%Y-%m-%d %H:%M:%S",tinfo);
   out << n << ',' << n+1 << ',' << "some text for f3" << ','
       << "some text f4" << ',' << tmb << ',' << "some text f6"
       << ',' << "some text f7\n";
  }
}
