#!/usr/dcs/software/supported/bin/bash

DB=test
PASSWD=#multifacet
HOST=localhost
MYSQL=/home/yyzhou2/shanlu/research/bugbench/mysql-4.1.1-alpha/mysql/bin/mysql

CMD="./runtran --repeat --seed 65323445 --database $DB --trace populate_db.txt --monitor pinot"

TIME="30 360 1"

BNAME="results"

if true; then
    # prepare table a & b
    $MYSQL -u root -D $DB -e 'drop table if exists a'
    $MYSQL -u root -D $DB -e 'drop table if exists b'
echo  $MYSQL -u root -D $DB -e "drop table if exists b"
    # populate table a
    $MYSQL -u root -D $DB -e 'create table a (id int auto_increment not null primary key, tal int not null default 1)';
    rm -f /tmp/t
    for i in `seq 1 9`; do
        echo "insert into a set id=null, tal=$i" >> /tmp/t
        echo '\g' >> /tmp/t
    done;
    $MYSQL -u root -D $DB < /tmp/t
    
    # populate table b
    $MYSQL -u root -D $DB -e 'create table b (id int auto_increment not null primary key, tal int not null default 1);'
    rm -f /tmp/t
    for i in `seq 1 9`; do
        echo "insert into b set id=null, tal=$(( $i * $i ))" >> /tmp/t
        echo '\g' >> /tmp/t
    done;
    $MYSQL -u root -D $DB < /tmp/t
    $MYSQL -u root -D $DB -e 'SELECT * FROM a'
    $MYSQL -u root -D $DB -e 'SELECT * FROM b'
fi

# generate query trace file
if true; then
    rm -f populate_db.txt
    for i in `seq 1 5000`; do
        echo "00:00:00,000 $DB B $i" >> populate_db.txt
        # means a random integer will be used in prepared query
        echo "00:00:00,000 $DB S $i select * from a,b where a.tal+b.tal=?" >> populate_db.txt
        echo "00:00:00,000 $DB C $i" >> populate_db.txt
    done;
fi;

# drive the DB on a single host directly
#$CMD --thread 1 --host pinot $TIME  ${BNAME}

