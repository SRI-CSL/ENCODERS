#!/bin/bash -ev

#sudo timedatectl set-ntp on
sudo apt-get update
sudo apt-get install -y automake sqlite3 build-essential autoconf libtool git libxml2 libxml2-dev sqlite libsqlite3-dev python-dev libbluetooth-dev libdbus-1-3 libdbus-1-dev libssl-dev htop

sudo apt-get install -y default-jdk

#haggle requires specific gcc,g++ version
sudo apt-get -y install gcc-4.9
sudo apt-get -y install g++-4.9
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-4.9 49 --slave /usr/bin/g++ g++ /usr/bin/g++-4.9
sudo update-alternatives --set gcc "/usr/bin/gcc-4.9"

