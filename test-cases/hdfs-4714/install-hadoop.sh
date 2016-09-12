#!/bin/bash
#These are installation instructions for setup of the image

cd ~/

#Get Hadoop Installation
#wget https://archive.apache.org/dist/hadoop/core/hadoop-0.23.7/hadoop-0.23.7.tar.gz
git clone https://github.com/apache/hadoop.git
cd hadoop
git checkout release-2.1.0-beta
git checkout a0a6e7b6fe5d978143125c7a495e02314a4a0485
mvn package -Pdist -DskipTests -Dtar

mkdir -p /home/hadoop/hdfs/datanode
mkdir -p /home/hadoop/hdfs/namenode

#Copy Configuration Files
#cd ~/
#cp core-site.xml /root/hadoop/etc/hadoop/.
#cp hdfs-site.xml /root/hadoop/etc/hadoop/.
#cp mapred-site.xml /root/hadoop/etc/hadoop/.
#cp hadoop-env.sh /root/hadoop/etc/hadoop/.


#Executing Hadoop
#sh /root/hadoop/bin/hadoop namenode -format
#sh /root/hadoop/bin/start-all.sh
