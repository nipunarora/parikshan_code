#!/bin/bash

scp nipun@gandalf.nec-labs.com:/etc/hosts /etc/hosts

ssh-keygen -t rsa
ssh-copy-id -i ~/.ssh/id_rsa.pub hadoop@spark-master
ssh-copy-id -i ~/.ssh/id_rsa.pub hadoop@spark-slave1
ssh-copy-id -i ~/.ssh/id_rsa.pub hadoop@spark-slave2
