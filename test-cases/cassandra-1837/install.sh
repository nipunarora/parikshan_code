# install.sh --- 
# 
# Filename: install.sh
# Description: 
# Author: Nipun Arora
# Maintainer: 
# Created: Thu Jun 30 01:55:46 2016 (+0000)
# Code:

if [ ! -d cassandra ] 
then
    echo "Cloning Git Repository of cassandra"
    git clone https://github.com/apache/cassandra
    cd cassandra
    git checkout c9e6991b2d46b9c224a56f06355c88fb5d996997
    cd ..
    rm -rf .git
fi
cp ivy.xml cassandra/.
cp ivysettings.xml cassandra/.
cd cassandra
ant
 
# install.sh ends here
