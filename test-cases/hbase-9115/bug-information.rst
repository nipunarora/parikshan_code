se-9115 (Release)
Hbase-9115 Report
1. Symptom
If client adds columns in an unsorted order, then the append operation may overwrite values.
1.1 Severity
Critical
1.2 Was there exception thrown?
No
1.2.1 Were there multiple exceptions?
No
1.3 Scope of the failure
It may cause original data lost.
2. How to reproduce this failure
2.0 Version
0.94.10
2.1 Configuration
 
hdfs-site.xml:
 
<configuration>
         <property>
         <name>dfs.replication</name>
             <value>1</value>
         </property>
        <property>
        <name>dfs.support.append</name>
            <value>true</value>
        </property>
</configuration>
 
hbase-site.xml:
 
<configuration>
  <property>
        <name>hbase.rootdir</name>
        <value>hdfs://localhost:9000/hbase</value>
  </property>
        <property>
        <name>hbase.cluster.distributed</name>
            <value>true</value>
        </property>
        <property>
        <name>hbase.zookeeper.quorum</name>
            <value>localhost</value>
        </property>
        <property>
        <name>dfs.support.append</name>
            <value>true</value>
        </property>
</configuration>
2.2 Reproduction procedure
1. Add columns in unsorted order.
2. Append values Bytes.toBytes in the columns.
2.2.1 Timing order
Single event
2.2.2 Events order externally controllable?
Yes
2.3 Can the logs tell how to reproduce the failure?
No
2.4 How many machines needed?
One
3. Diagnosis procedure
3.1 Detailed Symptom (where you start)
Append values Bytes.toBytes("one two") and Bytes.toBytes(" three") in 3 columns.
Only for 2 out of these 3 columns the result is "one two three".
Output from the hbase shell:
hbase(main):008:0* scan "mytesttable"
ROW                                    COLUMN+CELL                                                                                                  
 mytestRowKey                    column=TestA:dlbytes, timestamp=1375436156140, value=one two three                                                
 mytestRowKey                    column=TestA:tbytes, timestamp=1375436156140, value=one two three                                                  
 mytestRowKey                    column=TestA:ulbytes, timestamp=1375436156140, value= three                                                  
1 row(s) in 0.0280 seconds
3.2 Backward inference
Apparently, this bug is triggered by appending new values to the columns. However, most appending operations do not fail. So, this root cause can be found only if we know how the client operates. The only difference between this case and the normal appending operation is that the client firstly adds the columns in an unsorted order this time. So we can think about how the order of adding affects the appending result.
4. Root cause
In the append function of HRegion file:
for (KeyValue kv : family.getValue()) {
----This loop iterates the input columns and update existing values if they were found, otherwise add new column initialized to the append value
        KeyValue newKV;
        if (idx < results.size()
         && results.get(idx).matchingQualifier(kv.getBuffer(),
         kv.getQualifierOffset(), kv.getQualifierLength())) {
----If the columns is added in an unsorted order, the results.get(idx).matchingQualifier (kv.getBuffer(), kv.getQualifierOffset(), kv.getQualifierLength() condition will be false. This means it will go to else statement and a new column will be created and overwrite the original column.
              KeyValue oldKv = results.get(idx);
              // allocate an empty kv once
              newKV = new KeyValue(row.length, kv.getFamilyLength(),
              kv.getQualifierLength(), now, KeyValue.Type.Put,
              oldKv.getValueLength() + kv.getValueLength());
              ... ...
              } else {
              ... ...
              }
4.1 Category:
Semantic
5. Fix
src/main/java/org/apache/hadoop/hbase/regionserver/HRegion.java
  Store store = stores.get(family.getKey());
+ Collections.sort(family.getValue(), store.getComparator());
  List<KeyValue> kvs = new
                           ArrayList<KeyValue>(family.getValue().size());
5.1 How?
This patch fixes the bug by sorting the columns before appending.
Published by Google DriveâReport AbuseâUpdated automatically every 5 minutes
