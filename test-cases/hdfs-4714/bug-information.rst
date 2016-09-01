HDFS-4714 Report

url: https://issues.apache.org/jira/browse/HDFS-4714

1. Symptom

Namenode can slow down significantly due to massive numbers of failing requests. The main reason is the full stack in the exception in namenode’s error log.
Performance.

1.1 Severity

Major.
1.2 Was there exception thrown?

Yes.
1.3 Were there multiple exceptions?

Yes. FileNotFound,InvalidPathException,.. Each can occur multiple times...
1.4 Scope of the failure

All clients slow down.
2. How to reproduce this failure

Any exceptions such as “invalidtoken”, invalid path, will trigger an exception with full stack. For example, see the log in 3083:
2013-07-04 17:12:42,078 INFO org.apache.hadoop.ipc.Server: IPC Server listener on 8020: readAndProcess threw exception javax.security.sasl.SaslException: DIGEST-MD5: IO error acquiring password [Caused by org.apache.hadoop.security.token.SecretManager$InvalidToken: token (HDFS_DELEGATION_TOKEN token 1 for hdfs) can't be found in cache] from client 128.100.23.4. Count of bytes read: 0
javax.security.sasl.SaslException: DIGEST-MD5: IO error acquiring password [Caused by org.apache.hadoop.security.token.SecretManager$InvalidToken: token (HDFS_DELEGATION_TOKEN token 1 for hdfs) can't be found in cache]
        at com.sun.security.sasl.digest.DigestMD5Server.validateClientResponse(DigestMD5Server.java:577)
        at com.sun.security.sasl.digest.DigestMD5Server.evaluateResponse(DigestMD5Server.java:226)
        at org.apache.hadoop.ipc.Server$Connection.saslReadAndProcess(Server.java:1203)
        at org.apache.hadoop.ipc.Server$Connection.readAndProcess(Server.java:1397)
        at org.apache.hadoop.ipc.Server$Listener.doRead(Server.java:712)
        at org.apache.hadoop.ipc.Server$Listener$Reader.doRunLoop(Server.java:511)
        at org.apache.hadoop.ipc.Server$Listener$Reader.run(Server.java:486)
Caused by: org.apache.hadoop.security.token.SecretManager$InvalidToken: token (HDFS_DELEGATION_TOKEN token 1 for hdfs) can't be found in cache
        at org.apache.hadoop.security.token.delegation.AbstractDelegationTokenSecretManager.retrievePassword(AbstractDelegationTokenSecretManager.java:222)
        at org.apache.hadoop.security.token.delegation.AbstractDelegationTokenSecretManager.retrievePassword(AbstractDelegationTokenSecretManager.java:46)
        at org.apache.hadoop.security.SaslRpcServer$SaslDigestCallbackHandler.getPassword(SaslRpcServer.java:194)
        at org.apache.hadoop.security.SaslRpcServer$SaslDigestCallbackHandler.handle(SaslRpcServer.java:219)
        at com.sun.security.sasl.digest.DigestMD5Server.validateClientResponse(DigestMD5Server.java:568)
        ... 6 more
Same reproduction procedure as hdfs-3083.

2.0 Version

0.23.7
2.1 Configuration

Any.
2.2 Reproduction procedure

Issue many of the failing requests.
2.2.1 Events:

many failing requests
2.2.2 Timing order:

No particular order requirements
2.2.3 Externally controllable?

Yes.
2.3 Can the logs tell how to reproduce the failure?

Sure. It logs the exception and all you need to do is to figure out the exception...
2.4 How many machines needed?

2
3. Diagnosis procedure

3.1 Detailed Symptom (where you start)

Many exceptions.
3.2 Backward inference

No need for backward inference.
4. Root cause

Fix: disable the stack in certain exceptions.
    // Set terse exception whose stack trace won't be logged
-    this.server.addTerseExceptions(SafeModeException.class);
+    this.server.addTerseExceptions(SafeModeException.class,
+        FileNotFoundException.class,
+        HadoopIllegalArgumentException.class,
+        FileAlreadyExistsException.class,
+        InvalidPathException.class,
+        ParentNotDirectoryException.class,
+        UnresolvedLinkException.class,
+        AlreadyBeingCreatedException.class,
+        QuotaExceededException.class,
+        RecoveryInProgressException.class,
+        AccessControlException.class,
+        InvalidToken.class);
  }
4.1 Category:

Incorrect handling of exception. To test this, you need to anticipate the error scenario: you trigger the logic, but it’s wrong only under certain context.
Published by Google Drive–Report Abuse–Updated automatically every 5 minutes
