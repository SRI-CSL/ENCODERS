#!/bin/bash -ev

#tar -xvf core-4.6-ttm.tgz
#pushd core-4.6
#./bootstrap.sh
#./configure
#make
#sudo make install
#in case there are issues with installing and configuring the serivce and core-daemon service does not appear or exist
#sudo systemctl daemon-reload

#core myservices directory for ubuntu user
sudo mkdir -p /home/ubuntu/.core/myservices
sudo chown -R ubuntu:ubuntu ~/.core/

#cpulimit
pushd cpulimit
make
sudo make install
popd

#emane
tar -xvf emane-0.8.1-release-2.ubuntu-12_04.amd64.tgz
pushd emane-0.8.1-release-2/deb/ubuntu-12_04/amd64/
sudo dpkg -i *.deb
popd

#bonmotion
unzip -u bonnmotion-2.0.zip
pushd bonnmotion-2.0
echo "/usr/bin/" | ./install
popd
