#!/bin/bash -e

sudo apt-get install -y python2.7 python2.7-dev libgmp-dev flex bison wget


mkdir dependencies
pushd dependencies
wget https://crypto.stanford.edu/pbc/files/pbc-0.5.14.tar.gz
tar -xvf pbc-0.5.14.tar.gz 
cd pbc-0.5.14/
./configure
make
sudo make install
popd

./configure
make
sudo make install

cd /usr/lib
sudo ln -s /usr/local/lib/libpbc.so.1 .

cd ../ccb/c++
make
