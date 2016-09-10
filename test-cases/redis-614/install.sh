# install.sh --- 
# 
# Filename: install.sh
# Description: 
# Author: Nipun Arora
# Maintainer: 
# Created: Thu Jun 30 01:55:46 2016 (+0000)
# Code:

if [ ! -d redis ] 
then
    echo "Cloning Git Repository of redis"
    git clone https://github.com/antirez/redis.git
    cd redis
    git checkout bfc197c3b604baf0dba739ea174d5054284133f0
fi
make
rm -rf .git
 
# install.sh ends here
