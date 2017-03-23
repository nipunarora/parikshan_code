#!/bin/bash
# $1 - hostname - 138.15.170.40:8080
# $2 - traces.txt
# $3 - output.txt

echo cat $2|java -cp target/Parikshan-1.0-SNAPSHOT.jar:target/dependency/* SimpleWorker -h $1 > $3
cat $2|java -cp target/Parikshan-1.0-SNAPSHOT.jar:target/dependency/* SimpleWorker -h $1 > $3
#java -cp target/Parikshan-1.0-SNAPSHOT.jar:target/dependency/* SimpleWorker -h 138.15.170.140
