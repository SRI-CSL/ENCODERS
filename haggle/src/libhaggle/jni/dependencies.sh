#!/bin/bash -ev

#sbt
#http://www.scala-sbt.org/1.0/docs/Installing-sbt-on-Linux.html
sudo apt-get install -y apt-transport-https
echo "deb https://dl.bintray.com/sbt/debian /" | sudo tee -a /etc/apt/sources.list.d/sbt.list
sudo apt-key adv --keyserver hkp://keyserver.ubuntu.com:80 --recv 2EE0EA64E40A89B84B2DF73499E82A75642AC823
sudo apt-get update
sudo apt-get install -y sbt


cd $HOME
mkdir -p installs
pushd installs

wget http://mirrors.gigenet.com/apache/maven/maven-3/3.5.2/binaries/apache-maven-3.5.2-bin.tar.gz
tar -xvf apache-maven-3.5.2-bin.tar.gz

export PATH=${HOME}/installs/apache-maven-3.5.2/bin:$PATH



