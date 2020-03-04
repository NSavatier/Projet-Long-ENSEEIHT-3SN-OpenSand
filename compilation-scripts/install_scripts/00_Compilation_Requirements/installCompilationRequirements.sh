#!/bin/bash


#note : this script is written for ubuntu 16.04 LTS 

#install opensand's libGSE, libRLE and libROHC libs
sudo apt-add-repository "deb http://packages.net4sat.org/opensand xenial stable"
sudo apt-get update
sudo apt-get install -y --allow-unauthenticated libgse-dev librle-dev librohc-dev

#then install all required packages 
#list taken from https://wiki.net4sat.org/doku.php?id=opensand:manuals:compilation_manual:index
sudo apt-get install -y sudo fakeroot apt-utils software-properties-common python-minimal build-essential debhelper autotools-dev automake libtool pkg-config python-setuptools gcc g++ libxml++2.6-dev libgoogle-perftools-dev python-dev libpcap-dev python-lxml swig libnl-3-dev dh-systemd 


#it seems that this article forgets a few dependencies (or i missed them)
sudo apt-get install -y libnl-3-dev libnl-route-3-dev

