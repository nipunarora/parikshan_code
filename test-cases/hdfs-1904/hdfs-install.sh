#!/bin/bash

wget https://archive.apache.org/dist/hadoop/core/hadoop-0.23.7/hadoop-0.23.7-src.tar.gz
tar -zxvf hadoop-0.23.7-src.tar.gz
sudo apt-get install openjdk-7-jdk libssl-dev zlib1g-dev g++ autoconf automake libprotobuf-dev protobuf-compiler
mvn package -Pdist -DskipTests -Dtar
