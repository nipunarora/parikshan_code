url: https://docs.google.com/document/d/1_sTDfQsMb9rpGZZwEcNqAEtNC-7CsXe6NkvbX-tjV88/pub

CASSANDRA-5225 (release)
CASSANDRA-5225
1. Symptom

   Missing columns when requesting specific columns from wide row. The data is still in the table, just it might not be returned to the user. It also wone

   1) Insert  a large number of columns into Cassandra (so it is a wide row)
   2) fetch columns in a portion of ranges
      See testcase
      Num triggering events
      
2

2.2.1 Timing order (Order important column)

yes
2.2.2 Events order externally controllable? (Order externally controllable? column)

yes
2.3 Can the logs tell how to reproduce the failure?

Yes
2.4 How many machines needed?

1
3. Diagnosis procedure

Error msg?

yes
3.1 Detailed Symptom (where you start)

When requesting specific columns from a wide row, thrift query does not return with the correct output.
3.2 Backward inference

Taking closer look, Cassandra is reading from the wrong column index. A problem was found with the index checking algorithm. In fact, it was written in reverse.

4. Root cause

4.1 Category:

Semantic
4.2 Are there multiple fault?

no
4.2 Can we automatically test it?

yes
5. Fix

5.1 How?

The fix reverses the algorithm.
Before
1.png
After
2.png
Published by Google Drive
