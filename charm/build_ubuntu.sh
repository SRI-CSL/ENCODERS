#!/bin/bash -e

sudo apt-get install -y python2.7 python2.7-dev libgmp-dev flex bison wget

#use setuptools install, not distribute.py
sudo apt install -y python-pip
#sudo pip uninstall distribute
pip install setuptools

mkdir -p dependencies
pushd dependencies
wget https://crypto.stanford.edu/pbc/files/pbc-0.5.14.tar.gz
tar -xvf pbc-0.5.14.tar.gz 
pushd pbc-0.5.14/
./configure
make
sudo make install
popd

popd
./configure.sh
make
sudo make install

pushd /usr/lib
sudo ln -fs /usr/local/lib/libpbc.so.1 .
popd

pwd
cd ../ccb/c++
make
