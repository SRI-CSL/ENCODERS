#!/bin/bash -ev

./install_docker.sh

pushd $HOME
test -e "ENCODERS-Docker" || git clone https://github.com/vehiclecloud/ENCODERS-Docker
pushd ENCODERS-Docker
sudo docker build -t local-encoders --build-arg CACHEBUST=$(date +%s) .
popd
popd
