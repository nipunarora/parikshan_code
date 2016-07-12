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
    git checkout 2ac546e00cd4000506558e23d11acafb4ce654b7
    cd ..
fi
cd redis
make
 
# install.sh ends here
