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
    git checkout 9a5cbf9f7e96293f5dd806e15a4988faa1b03f54
fi
make
rm -rf .git
 
# install.sh ends here
