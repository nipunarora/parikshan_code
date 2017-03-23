#!/bin/bash
cat $1|java -cp target/Parikshan-1.0-SNAPSHOT.jar:target/dependency/httpclient-4.0.jar:target/dependency/httpcore-4.0.jar:target/dependency/commons-logging-1.1.1.jar:target/dependency/commons-codec-1.3.jar SimpleWorker > $2
