#!/bin/bash


#note : this script is written for ubuntu 16.04 LTS 
sudo apt-add-repository "deb http://packages.net4sat.org/opensand xenial stable"
sudo apt-get update
sudo apt-get install libgse-dev librle-dev librohc-dev
#libtoolize seems to be somehow required to compile the sources
#so we install libtool
sudo apt-get install libtool
