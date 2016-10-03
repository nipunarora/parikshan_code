

Component/s:
namenode

Affects Version/s:
0.23.0

Steps to reproduce:
1. Configure secondary namenode with fs.checkpoint.period set to a small value (eg 3 seconds)
2. Format filesystem and start HDFS
3. hadoop fs -mkdir /foo/bar ; sleep 5 ; echo | hadoop fs -put - /foo/bar/baz

   2NN will crash with the following trace on the next checkpoint. The primary NN also crashes on next restart
   11/05/10 15:19:28 ERROR namenode.SecondaryNameNode: Throwable Exception in doCheckpoint:
   11/05/10 15:19:28 ERROR namenode.SecondaryNameNode: java.lang.NullPointerException: Panic: parent does not exist
   at org.apache.hadoop.hdfs.server.namenode.FSDirectory.addChild(FSDirectory.java:1693)
   at org.apache.hadoop.hdfs.server.namenode.FSDirectory.addChild(FSDirectory.java:1707)
   at org.apache.hadoop.hdfs.server.namenode.FSDirectory.addNode(FSDirectory.java:1544)
   at org.apache.hadoop.hdfs.server.namenode.FSDirectory.unprotectedAddFile(FSDirectory.java:288)
   at org.apache.hadoop.hdfs.server.namenode.FSEditLogLoader.loadEditRecords(FSEditLogLoader.java:234)
   at org.apache.hadoop.hdfs.server.namenode.FSEditLogLoader.loadFSEdits(FSEditLogLoader.java:116)
   at org.apache.hadoop.hdfs.server.namenode.FSEditLogLoader.loadFSEdits(FSEditLogLoader.java:62)
   at org.apache.hadoop.hdfs.server.namenode.FSImage.loadFSEdits(FSImage.java:723)
   at org.apache.hadoop.hdfs.server.namenode.SecondaryNameNode$CheckpointStorage.doMerge(SecondaryNameNode.java:720)
   at org.apache.hadoop.hdfs.server.namenode.SecondaryNameNode$CheckpointStorage.access$500(SecondaryNameNode.java:610)
   at org.apache.hadoop.hdfs.server.namenode.SecondaryNameNode.doMerge(SecondaryNameNode.java:487)
   at org.apache.hadoop.hdfs.server.namenode.SecondaryNameNode.doCheckpoint(SecondaryNameNode.java:448)
   at org.apache.hadoop.hdfs.server.namenode.SecondaryNameNode.doWork(SecondaryNameNode.java:312)
   at org.apache.hadoop.hdfs.server.namenode.SecondaryNameNode.run(SecondaryNameNode.java:276)
