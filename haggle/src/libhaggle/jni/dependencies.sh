#!/bin/bash -ev

cd $HOME
mkdir -p installs
pushd installs

wget http://mirrors.gigenet.com/apache/maven/maven-3/3.5.2/binaries/apache-maven-3.5.2-bin.tar.gz
tar -xvf apache-maven-3.5.2-bin.tar.gz

export PATH=/home/ubuntu/installs/apache-maven-3.5.2/bin:$PATH
