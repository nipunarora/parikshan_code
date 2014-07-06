#!/bin/bash
########################
#Author: Nipun Arora
#Description: This script installs a typical OpenVZ container
#Usage createVZ <container ID> <hostname> <IP Address>

#****Before starting the script you need to download debian-minimal image***
#> cd /vz/template/cache
#> wget http://download.openvz.org/template/precreated/contrib/debian-7.0-amd64-minimal.tar.gz
########################

echo "Usage createVZ <container ID> <hostname> <IP Address>"
echo "Creating Container" $1
vzctl create $1 --ostemplate debian-7.0-amd64-minimal --config basic --layout ploop
echo "Finished creating container" $1
vzctl set $1 --hostname $2 --save
vzctl set $1 --ipadd $3 --save
vzctl set $1 --numothersock 120 --save
vzctl set $1 --nameserver 8.8.8.8 --nameserver 8.8.4.4 --save
echo "Finished configuration"