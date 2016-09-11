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
    git checkout 65606b3bc6fb8698662811ba286be8b5cabacb55
fi
make
rm -rf .git
 
# install.sh ends here
