CASSANDRA-1837
1. Symptom

Deleted columns become available again after a flush.
 
Category (in the spreadsheet):

wrong computation

1.1 Severity
blocker

1.2 Was there exception thrown? (Exception column in the spreadsheet)

no
 
1.2.1 Were there multiple exceptions?

no exceptions, just returns more results than expected (previously deleted rows)
 
1.3 Was there a long propagation of the failure?

no
 
1.4 Scope of the failure (e.g., single client, all clients, single file, entire fs, etc.)

single file
 
Catastrophic? (spreadsheet column)
no 

2. How to reproduce this failure

2.0 Version

0.7.0rc3
2.1 Configuration

standard configuration of cassandra version 0.7.0rc3
 
# of Nodes?

1
2.2 Reproduction procedure

1) insert column into cassandra without flushing (file write)
2) delete the inserted column (file write)
3) flush (file write)
4) query for the column (file read)
 
Num triggering events

4 
2.2.1 Timing order (Order important column)

yes
2.2.2 Events order externally controllable? (Order externally controllable? column)

yes
2.3 Can the logs tell how to reproduce the failure?

Have all the events been logged properly? Note: only  if the log contains (directly or indirectly) all the input events. If, for example, 2 out of 3 input events are logged, this should .
2.4 How many machines needed?

1
2.5 How hard is the reproduction?

easy
3. Diagnosis procedure

Error msg?

no
3.1 Detailed Symptom (where you start)

Deleted columns become available again after a flush. Very simple error.
3.2 Backward inference

With some domain knowledge, a developer found the error. This is happening because of a bug in a the way deleted rows are not interpreted once they leave the memtable in the CFS.getRangeSlice code. I.e. the flush does not recognize the delete and the purged data does not contain the delete operation. Thus querying for the data shows  content as well.
3.3 Are the printed log sufficient for diagnosis?

yes
3.4 Are logs misleading?

no
3.5 Do we need to examine different s log for diagnosis?

no
3.6 Is it a multi-components failure?

no
3.7 How hard is the diagnosis?

easy if developer has domain knowledge
 
4. Root cause

Deleted rows are not interpreted once they leave the memtable in the CFS.getRangeSlice code
4.1 Category:

semantic
4.2 Are there multiple fault?

no
4.2 Can we automatically test it?

yes
5. Fix

5.1 How?

Fix the code and allows the cassandra to interpret the delete operation when flushing the data to SStable.
+                        columns.hasNext(); // force cf initializtion
+                        try
+                        {
+                            if (columns.getColumnFamily().isMarkedForDelete())
+                                lastDeletedAt = Math.max(lastDeletedAt, columns.getColumnFamily().getMarkedForDeleteAt());
+                        }
5.2 Exception behavior?

no exceptions
5.3 # of discussion threads?

11
6.Any interesting findings?

none
 
7. Scope of the failure
single file (for all affected files)componentdeletedthe nobe yesanswer 

------------------------------------------

[default@unknown] create keyspace testks;
2785d67c-02df-11e0-ac09-e700f669bcfc
[default@unknown] use testks;
Authenticated to keyspace: testks
[default@testks] create column family testcf;
2fbad20d-02df-11e0-ac09-e700f669bcfc
[default@testks] set testcf['test']['foo'] = 'foo';
Value inserted.
[default@testks] set testcf['test']['bar'] = 'bar';
Value inserted.
[default@testks] list testcf;
Using default limit of 100

-------------------

RowKey: test
=> (column=626172, value=626172, timestamp=1291821869120000)
=> (column=666f6f, value=666f6f, timestamp=1291821857320000)

1 Row Returned.
[default@testks] del testcf['test'];
row removed.
[default@testks] list testcf;
Using default limit of 100

-------------------

RowKey: test

1 Row Returned.

>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

[default@testks] list testcf;
Using default limit of 100

-------------------

RowKey: test
=> (column=626172, value=626172, timestamp=1291821869120000)
=> (column=666f6f, value=666f6f, timestamp=1291821857320000)

1 Row Returned.

>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
