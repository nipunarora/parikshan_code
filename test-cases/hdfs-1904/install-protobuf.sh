#!/bin/bash


wget https://github.com/google/protobuf/releases/download/v2.4.1/protobuf-2.4.1.tar.gz
tar -zxvf protobuf-2.4.1.tar.gz
cd protobuf-2.4.1
./configure
make
sudo make install
sudo ldconfig
ln -s /usr/local/bin/protoc /usr/bin/protoc

