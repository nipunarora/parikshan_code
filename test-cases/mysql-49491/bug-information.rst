
Download From:

wget http://downloads.mysql.com/archives/get/file/mysql-5.1.38.tar.gz

Installation Instructions:

```
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

```

ALLOWING INSECURE ACCESS

```

--skip-grant-tables

```

CONNECT USING MYSQL CLIENT

cd mysql-version
cd client
 ./mysql --host=172.17.0.14 --port=3306


 BUG REPORT:

 https://bugs.mysql.com/bug.php?id=49491


DESCRIPTION:

The calculation of MD5 and SHA1 hash values using the built-in MySQL functions does not seem to be as efficient as it could be.

There seem to be two factors that determine the performance of the hash generation:
- computation of the actual hash value (binary value)
- conversion of the binary value into a string field

The run time of the hash computation depends on the length of the input string whereas the overhead of the binary-to-string conversion can be considered as a fixed constant as it will always operate on hash values of 16 (MD5) or 20 (SHA1) bytes length.
The impact of the binary-to-string conversion will become more visible with shorter input strings than with long input strings. For short input strings it seems that more time is spent in the binary-to-string conversion than in the actual hash computation part.

The reason seems to be that the code that transforms the calculated binary hash value back into a string field result is very generic and therefore slower than necessary.

To convert the result into the string field, the code in sql/item_strfunc.cc currently does the following (for MD5, the implementation for SHA1 is similar):

sprintf((char *) str->ptr(),
"%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
digest[0], digest[1], digest[2], digest[3],
digest[4], digest[5], digest[6], digest[7],
digest[8], digest[9], digest[10], digest[11],
digest[12], digest[13], digest[14], digest[15]);
str->length((uint) 32);
return str;

The conversion can probably be sped up by not using the very generic sprintf but a more specialized algorithm.

I have done some tests on Linux 2.6.26-1-amd64 with the following replacement:

```

static char hexmap[]= { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };
int i;
char *p;

p=(char *) str->ptr();
for (i=0;i<=15;i++)
{
(*p++)=hexmap[digest[i] >> 4];
(*p++)=hexmap[digest[i] & 15];
}
str->length((uint) 32);
return str;

I am attached this patch to this bug report.

The savings that can be achieved by moving from sprintf to something more specialized may vary from compiler to compiler and from compile option to option. I can't really tell what works best in all environments.
However, the fixed overhead of the binary-to-string conversion should be minimized as much as possible as at least on some systems massive speedups can be achieved by this.

Example:
Original MySQL 5.1.38:
mysql> select count(*) from (select md5(firstname) from users) sub limit 1\G
*************************** 1. row ***************************
count(*): 840245
1 row in set (3.13 sec)

Patched MySQL 5.1.38:
mysql> select count(*) from (select md5(firstname) from users) sub limit 1\G
*************************** 1. row ***************************
count(*): 840245
1 row in set (1.36 sec)

Original MySQL 5.1.38:
mysql> select count(*) from (select sha1(firstname) from users) sub limit 1\G
*************************** 1. row ***************************
count(*): 840245
1 row in set (3.74 sec)

Patched MySQL 5.1.38:
mysql> select count(*) from (select sha1(firstname) from users) sub limit 1\G

*************************** 1. row ***************************
count(*): 840245
1 row in set (1.70 sec)

```

How to repeat:
Just generate a lot of MD5 or SHA1 hashes from some of the system table values and note the query times:

MD5:

  mysql> select * from (select md5(t.name),md5(t.description),md5(t.example),md5(t.url) from mysql.help_topic t,mysql.help_keyword k) sub limit 1\G
  *************************** 1. row ***************************
  md5(t.name): ce31e2a082d17e038fcc6e3006166653
  md5(t.description): bdcda0595dc1c938038b13d51df0a38d
  md5(t.example): 3279bf9c13baf8b749e29aa71cb2ccd2
  md5(t.url): ccb705dbce9c9827d93fb0d4b6c0b378
  1 row in set (2.99 sec)

  SHA1:

  mysql> select * from (select sha1(t.name),sha1(t.description),sha1(t.example),sha1(t.url) from mysql.help_topic t,mysql.help_keyword k) sub limit 1\G
  *************************** 1. row ***************************
  sha1(t.name): 04e66352aa8f9c4c5f26b71bf380973ada994760
  sha1(t.description): c040e24b56fe6023617ab884d8715da7efb24de3
  sha1(t.example): a52b8b0b11e1d1d82839d0de1805192279ca0b1b
  sha1(t.url): ef7507b561798b9b877232fcb64719cee9eef057
  1 row in set (4.84 sec)
  
  Then apply the attached patch and run the same queries again:

  mysql> select * from (select md5(t.name),md5(t.description),md5(t.example),md5(t.url) from mysql.help_topic t,mysql.help_keyword k) sub limit 1\G
  *************************** 1. row ***************************

  md5(t.name): ce31e2a082d17e038fcc6e3006166653
  md5(t.description): bdcda0595dc1c938038b13d51df0a38d
  md5(t.example): 3279bf9c13baf8b749e29aa71cb2ccd2
  md5(t.url): ccb705dbce9c9827d93fb0d4b6c0b378
  1 row in set (1.27 sec)

  mysql> select * from (select sha1(t.name),sha1(t.description),sha1(t.example),sha1(t.url) from mysql.help_topic t,mysql.help_keyword k) sub limit 1\G
  *************************** 1. row ***************************
  sha1(t.name): 04e66352aa8f9c4c5f26b71bf380973ada994760
  sha1(t.description): c040e24b56fe6023617ab884d8715da7efb24de3
  sha1(t.example): a52b8b0b11e1d1d82839d0de1805192279ca0b1b
  sha1(t.url): ef7507b561798b9b877232fcb64719cee9eef057
  1 row in set (2.77 sec)

The query times for the patched version are a lot better than for the original version.

Suggested fix:
Will attach a patch for MySQL 5.1.38 to this bug report with a suggestion how to modify the binary-to-string conversion in 5.1.38.
