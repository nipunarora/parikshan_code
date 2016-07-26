
Download From:

wget https://downloads.mysql.com/archives/get/file/mysql-5.0.15.tar.gz


Installation Instructions:

Install mysql using the following instructions:
apt-get install libncurses-dev
groupadd mysql
shell> useradd -g mysql mysql
shell> gunzip < mysql-VERSION.tar.gz | tar -xvf -
shell> cd mysql-VERSION
shell> ./configure --prefix=/usr/local/mysql
shell> make
shell> make install
shell> cp support-files/my-medium.cnf /etc/my.cnf
shell> cd /usr/local/mysql
shell> bin/mysql_install_db --user=mysql
shell> chown -R root  .
shell> chown -R mysql var
shell> chgrp -R mysql .
shell> bin/mysqld_safe --user=mysql &


ALLOWING INSECURE ACCESS

--skip-grant-tables 


CONNECT USING MYSQL CLIENT

cd mysql-version
cd client
 ./mysql --host=172.17.0.14 --port=3306


Description:
It takes extremely long time for mysql client tool to execute long INSERT statement. This occurs at 5.0.15, but not at 5.0.13 or 4.1.15.
I think this is a serious problem, because mysqldump creates very long
INSERT statement.

How to repeat:
mysql> use test;
drop table if exists t1;
create table t1(c1 char(10)) engine=myisam default charset=latin1;
insert into t1 values('1234567890');
insert into t1 select * from t1;
insert into t1 select * from t1;
insert into t1 select * from t1;
insert into t1 select * from t1;
insert into t1 select * from t1;
insert into t1 select * from t1;
insert into t1 select * from t1;
insert into t1 select * from t1;
insert into t1 select * from t1;
insert into t1 select * from t1;
insert into t1 select * from t1;
insert into t1 select * from t1;
insert into t1 select * from t1;
insert into t1 select * from t1;

Now 16384 records there. Then, dump it, and load it again.
# mysqldump test t1 > temp.sql
# mysql test < temp.sql

At my PC, loading took about 60 sec with 5.0.15,
while it's <1 sec at 4.0.15.
