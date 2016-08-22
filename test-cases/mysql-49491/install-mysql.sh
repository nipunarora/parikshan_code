#!/bin/bash
cd ~/
wget http://downloads.mysql.com/archives/get/file/mysql-5.1.38.tar.gz
tar -zxvf mysql-5.1.38.tar.gz
groupadd mysql
useradd -g mysql mysql
mv mysql-5.1.38 mysql
cd mysql
./configure --prefix=/usr/local/mysql
cp /root/Makefile /root/mysql/.
make
make install
cp /root/mysql/support-files/my-medium.cnf /etc/my.cnf
cd /usr/local/mysql
./bin/mysql_install_db --user=mysql
chown -R root .
chown -R mysql var
chgrp -R mysql .
./bin/mysqld_safe --user=mysql --skip-grant-tables &
