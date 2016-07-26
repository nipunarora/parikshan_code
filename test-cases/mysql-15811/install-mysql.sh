#!/bin/bash
cd ~/
wget https://downloads.mysql.com/archives/get/file/mysql-5.0.15.tar.gz
tar -zxvf mysql-5.0.15.tar.gz
groupadd mysql
useradd -g mysql mysql
mv mysql-5.0.15 mysql
cd mysql
./configure --prefix=/usr/local/mysql
make
make install
cp support-files/my-medium.cnf /etc/my.cnf
cd /usr/local/mysql
./bin/mysql_install_db --user=mysql
chown -R root .
chown -R mysql var
chgrp -R mysql .
#./bin/mysqld_safe --user=mysql --skip-grant-tables &
