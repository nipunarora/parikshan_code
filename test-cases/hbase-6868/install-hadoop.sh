#!/bin/bash

cd ~/

#Get Hadoop Installation
wget https://archive.apache.org/dist/hadoop/core/hadoop-1.0.4/hadoop-1.0.4.tar.gz
tar -xvzf hadoop-1.0.4.tar.gz
mv hadoop-1.0.4 /root/hadoop

#Copy Configuration Files
cp core-site.xml /root/hadoop/conf/.
cp hdfs-site.xml /root/hadoop/conf/.
cp mapred-site.xml /root/hadoop/conf/.
cp hadoop-env.sh /root/hadoop/conf/.

# add ssh key
ssh-keygen -t dsa -P '' -f ~/.ssh/id_dsa
cat ~/.ssh/id_dsa.pub >> ~/.ssh/authorized_keys

#Executing Hadoop
sh /root/hadoop/bin/hadoop namenode -format
sh /root/hadoop/bin/start-all.sh
