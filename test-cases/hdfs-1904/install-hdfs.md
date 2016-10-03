# Deploying Hadoop Cluster
By default Apache Spark supports Hadoop-2.2.0.
Make sure you have JDK installed on each machine and JAVA_HOME settled up for all the users

#### Note (For Cluster Setup)
Do step 1,3 & 4 on individual machine. Then do step 2,5,6 & 7 only on master. After that  scp the `/home/hadoop/hdfs` and `/home/hadoop/hadoop-2.2.0` on all the salves
 
## Step 1
Create a user (ex. "hadoop") on all the machines in the cluster i.e master and slaves.

    $ useradd -m hadoop
    $ passwd hadoop    //use any password
    $ su hadoop

## Step 2
Download Hadoop-2.2.0 on all the machines in hadoop's home 

    $ wget https://archive.apache.org/dist/hadoop/core/hadoop-2.2.0/hadoop-2.2.0.tar.gz
    $ tar -xzf hadoop-2.2.0.tar.gz

## Step 3
Configure hostnames in etc/hosts on all the machines (master and slaves). 
    Ex: 
    
        138.15.170.127 spark-master
        138.15.170.115 spark-slave1
        138.15.170.134 spark-slave2
        
## Step 4
Now, set the ssh-keys for keyless ssh. Execute these commands on all the machines (master and slaves).
   
    $ ssh-keygen -t rsa 
    $ ssh-copy-id -i ~/.ssh/id_rsa.pub hadoop@spark-master 
    $ ssh-copy-id -i ~/.ssh/id_rsa.pub hadoop@spark-slave1 
    $ ssh-copy-id -i ~/.ssh/id_rsa.pub hadoop@spark-slave2 
 
## Step 5
Edit the following configurations of Hadoop on all the machines (master and slaves)

In `$HADOOP_HOME/etc/hadoop/core-site.xml`. 
```xml
<configuration>
   <property> 
      <name>fs.default.name</name> 
      <value>hdfs://spark-master:9000/</value> 
   </property> 
   <property> 
      <name>dfs.permissions</name> 
      <value>false</value> 
   </property> 
</configuration>
```
In `$HADOOP_HOME/etc/hadoop/hdfs-site.xml`. 
```xml
<configuration>
    <property>
            <name>dfs.replication</name>
            <value>2</value>
    </property>
    <property>
            <name>dfs.namenode.name.dir</name>
            <value>file:/home/hadoop/hdfs/namenode</value>
    </property>
    <property>
            <name>dfs.datanode.data.dir</name>
            <value>file:/home/hadoop/hdfs/datanode</value>
    </property>
</configuration>
```

In `$HADOOP_HOME/etc/hadoop/mapred-site.xml`. 
```xml
<configuration>
<property>
      <name>mapred.job.tracker</name>
      <value>spark-master:9001</value>
   </property>
</configuration>
```
## Step 5
Create the following directories for hadoop data (master and slaves)
    
    $ mkdir /home/hadoop/hdfs/namenode
    $ mkdir /home/hadoop/hdfs/datanode
    
## Step 6
In `$HADOOP_HOME/etc/hadoop/hadoop-env.sh`. Set the $JAVA_HOME (for both Master and Slaves)

## Step 7
Enroll slaves in hadoop config files. In `$HADOOP_HOME/etc/hadoop/slaves` add all the slaves
    
    spark-slave1
    spark-slave2


## Step 8
Run the Hadoop Multi-Node cluster from master
   
    $ bin/hadoop namenode –format
   $ sbin/hadoop-deamon.sh start namenode
   $ sbin/hadoop-deamons.sh start datanode

### Step 9
The cluster should be up and running now. Check http://spark-master:50070 to see the HDFS UI and its status.


## Some HDFS shell commands 
This creates a directory in hdfs

    $ bin/hdfs dfs  -mkdir /output
To list all the files 

    $ bin/hdfs dfs -ls /output
To move files from local filesystem to HDFS

    $ bin/hdfs dfs -put <local-path> <hdfs-path>
To remove files from HDFS

    $ bin/hdfs dfs -rm <path to file>

### Configuring Spark 
For spark checkpoint files, add the following  line in your `Spark Driver`
```scala    
ssc.checkpoint("hdfs://spark-master:9000/<path-to-checkpointdirectory>")
```



