#!/bin/bash -ev

sudo timedatectl set-ntp on
sudo apt-get update
sudo apt-get install -y automake sqlite3 build-essential autoconf libtool git libxml2 libxml2-dev sqlite libsqlite3-dev python-dev libbluetooth-dev libdbus-1-3 libdbus-1-dev libssl-dev htop

