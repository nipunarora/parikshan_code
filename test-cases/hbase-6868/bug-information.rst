[hbase] HBASE-6868 (Release)
HBase-6868 Report
https://issues.apache.org/jira/browse/HBASE-6868

1. Symptom

   When “hbase.regionserver.checksum.verify” is set to “true” (HBase will perform the checksum verification), the correct semantic is the underlying HDFS should not do the checksum. But this semantic is broken.
   This will result in extra seek to disk in every io operation, and slow down the performance!
   Related failure:
   http://comments.gmane.org/gmane.comp.java.hadoop.hbase.devel/38229
   https://issues.apache.org/jira/browse/HDFS-4960
   1.1 Severity

   Blocker
   1.2 Was there exception thrown?

   No
   1.2.1 Were there multiple exceptions?

   No
   1.3 Scope of the failure

   Entire cluster’s performance will be affected
2. How to reproduce this failure

   2.0 Version

   0.94.0 HBase + 1.0.4 (or any) Hadoop
   2.1 Configuration

   <property>
   <name>hbase.regionserver.checksum.verify</name>
   <value>true</value>
   </property>
   Add this to HBase-site.xml
   2.2 Reproduction procedure

   Just start hdfs & hbase servers, and the error behavior already occurs.
   2.2.1 Timing order

   No timing order

   2.2.2 Events order externally controllable?

   Yes

   2.3 Can the logs tell how to reproduce the failure?

     Yes.
   2.4 How many machines needed?

   1. Regions Server + DN
      3. Diagnosis procedure

      3.1 Detailed Symptom (where you start)

   The user first noticed the performance degradation. But it will take him/her a while to understand why it happened. Eventually, the user will notice the disk is the cause, and he/she will start using strace to monitor the hadoop/hbase behavior (as described here:
   http://comments.gmane.org/gmane.comp.java.hadoop.hbase.devel/38229
   ). Then he/she will notice that there is always extra seek/verification in HDFS.
   3.2 Backward inference

   Once he noticed that the HDFS/HBase are both doing verification of checksums, the developer can quickly locate the root cause:
   this.useHBaseChecksum = conf.getBoolean(
   HConstants.HBASE_CHECKSUM_VERIFICATION, true);
   /* Ding: HFileSystem is the function to create a new HDFS instance.
   * The passed in parameter, useHBaseChecksum, is by default true, or user
     * can set it using “hbase.regionserver.checksum.verify” directive.
       */
       public HFileSystem(Configuration conf, boolean useHBaseChecksum)
       throws IOException {
       // Create the default filesystem with checksum verification switched on.
       // By default, any operation to this FilterFileSystem occurs on
       // the underlying filesystem that has checksums switched on.
       this.fs = FileSystem.get(conf);
       this.useHBaseChecksum = useHBaseChecksum;

       fs.initialize(getDefaultUri(conf), conf);
       // If hbase checksum verification is switched on, then create a new
       // filesystem object that has cksum verification turned off.
       // We will avoid verifying checksums in the fs client, instead do it
       // inside of hbase.
       // If this is the local file system hadoop has a bug where seeks
       // do not go to the correct location if setVerifyChecksum(false) is called.
       // This manifests itself in that incorrect data is read and HFileBlocks won't be able to read
       // their header magic numbers. See HBASE-5885
       if (useHBaseChecksum && !(fs instanceof LocalFileSystem)) {
       --- HBase is doing the checksum, the noChecksumFs (HDFS) should not be using checksum. What it should do is to create a new filesystem, and set “dfs.client.read.shortcircut.skip.checksum” to true. But the code didn’t do this config setup. So it will create a new HDFS instance under the default config, which is to do checksum in hdfs...
       conf = new Configuration(conf);
       conf.setBoolean("dfs.client.read.shortcircuit.skip.checksum", true);
       this.noChecksumFs = newInstanceFileSystem(conf);
       this.noChecksumFs.setVerifyChecksum(false);
       } else {
       this.noChecksumFs = fs;
       }
       }
4. Root cause

   When creating the HDFS instance from HBase, if HBase is already using checksum, it shoudn’t be using checksum in HDFS. The developer didn’t do this properly.
   4.1 Category:

   semantic.
5. Fix

   5.1 How?

   When creating the HDFS instance, set it to be not using checksum:

   // This manifests itself in that incorrect data is read and HFileBlocks won't be able to read
   // their header magic numbers. See HBASE-5885
   if (useHBaseChecksum && !(fs instanceof LocalFileSystem)) {
   +      conf = new Configuration(conf);
   +      conf.setBoolean("dfs.client.read.shortcircuit.skip.checksum", true);
	  this.noChecksumFs = newInstanceFileSystem(conf);
	  this.noChecksumFs.setVerifyChecksum(false);
	  } else {
