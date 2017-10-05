#!/bin/bash -ev

#needed for core
sudo apt-get install -y libev-dev bridge-utils ebtables

#needed to unzip bonmotion
sudo apt-get install -y unzip

#needed for bonmotion
sudo apt-get install -y default-jdk

#needed for emane
sudo apt-get install -y libxml-libxml-perl libxml-simple-perl
