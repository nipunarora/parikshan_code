INSTALLATION INSTRUCTIONS
****

For installing this version of cassandra the following steps must be followed

git clone https://github.com/apache/cassandra
cd cassandra
git checkout c9e6991b2d46b9c224a56f06355c88fb5d996997

The checked out ivysettings.xml and ivy.xml do not work, copy the one that I have provided in the folder.

cd cassandra
ant

STARTING UP CASSANDRA
*************

If you see this error: 

```

The stack size specified is too small, Specify at least 228k
Error: Could not create the Java Virtual Machine.
Error: A fatal exception has occurred. Program will exit.

```

Modify the cassandra/conf/cassandra-env.sh file with the following:
JVM_OPTS="$JVM_OPTS -Xss256k"  instead of JVM_OPTS="$JVM_OPTS -Xss180k

To start up cassandra service:

./cassandra/bin/cassandra
