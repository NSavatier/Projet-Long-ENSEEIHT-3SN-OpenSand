#!/bin/bash

#installs ALL OpenSAND packages

folder=/share/opensand-compiled-packages

#install requirements
sudo apt-add-repository "deb http://packages.net4sat.org/opensand xenial stable"
sudo apt-get update
sudo apt-get install -y --allow-unauthenticated libgse-dev librle-dev librohc-dev

#install output module
sudo apt install -y --allow-unauthenticated $folder/libopensand-output_5.1.2_amd64.deb
sudo apt install -y --allow-unauthenticated $folder/libopensand-output-dbg_5.1.2_amd64.deb
sudo apt install -y --allow-unauthenticated $folder/libopensand-output-dev_5.1.2_amd64.deb

#install configuration module
sudo apt install -y --allow-unauthenticated $folder/libopensand-conf_5.1.2_amd64.deb
sudo apt install -y --allow-unauthenticated $folder/libopensand-conf-dbg_5.1.2_amd64.deb
sudo apt install -y --allow-unauthenticated $folder/libopensand-conf-dev_5.1.2_amd64.deb

#install RT Module
sudo apt install -y --allow-unauthenticated $folder/libopensand-rt_5.1.2_amd64.deb
sudo apt install -y --allow-unauthenticated $folder/libopensand-rt-dbg_5.1.2_amd64.deb
sudo apt install -y --allow-unauthenticated $folder/libopensand-rt-dev_5.1.2_amd64.deb


#install Core Module
sudo apt install -y --allow-unauthenticated $folder/opensand-core_5.1.2_amd64.deb
sudo apt install -y --allow-unauthenticated $folder/opensand-core-conf_5.1.2_amd64.deb
sudo apt install -y --allow-unauthenticated $folder/opensand-core-dbg_5.1.2_amd64.deb
#sous partie de core (Plugin library)
sudo apt install -y --allow-unauthenticated $folder/libopensand-plugin_5.1.2_amd64.deb
sudo apt install -y --allow-unauthenticated $folder/libopensand-plugin-dbg_5.1.2_amd64.deb
sudo apt install -y --allow-unauthenticated $folder/libopensand-plugin-dev_5.1.2_amd64.deb

#install daemon module
sudo apt install -y --allow-unauthenticated $folder/opensand-daemon_5.1.2_amd64.deb


#install collector module
sudo apt install -y --allow-unauthenticated $folder/opensand-collector_5.1.2_amd64.deb

#install manager module
sudo apt install -y --allow-unauthenticated $folder/opensand-manager_5.1.2_amd64.deb
sudo apt install -y --allow-unauthenticated $folder/opensand-manager-core_5.1.2_amd64.deb
sudo apt install -y --allow-unauthenticated $folder/opensand-manager-gui_5.1.2_amd64.deb



#install GSE plugin
sudo apt install -y --allow-unauthenticated $folder/libopensand-gse-encap-plugin_5.1.2_amd64.deb
sudo apt install -y --allow-unauthenticated $folder/libopensand-gse-encap-plugin-conf_5.1.2_amd64.deb
sudo apt install -y --allow-unauthenticated $folder/libopensand-gse-encap-plugin-dbg_5.1.2_amd64.deb
sudo apt install -y --allow-unauthenticated $folder/libopensand-gse-encap-plugin-manager_5.1.2_amd64.deb

#install RLE plugin
sudo apt install -y --allow-unauthenticated $folder/libopensand-rle-encap-plugin_5.1.2_amd64.deb
sudo apt install -y --allow-unauthenticated $folder/libopensand-rle-encap-plugin-conf_5.1.2_amd64.deb
sudo apt install -y --allow-unauthenticated $folder/libopensand-rle-encap-plugin-dbg_5.1.2_amd64.deb
sudo apt install -y --allow-unauthenticated $folder/libopensand-rle-encap-plugin-manager_5.1.2_amd64.deb

#install ROHC plugin
sudo apt install -y --allow-unauthenticated $folder/libopensand-rohc-lan-adapt-plugin_5.1.2_amd64.deb
sudo apt install -y --allow-unauthenticated $folder/libopensand-rohc-lan-adapt-plugin-conf_5.1.2_amd64.deb
sudo apt install -y --allow-unauthenticated $folder/libopensand-rohc-lan-adapt-plugin-dbg_5.1.2_amd64.deb
sudo apt install -y --allow-unauthenticated $folder/libopensand-rohc-lan-adapt-plugin-manager_5.1.2_amd64.deb

#installs all packages required by an OpenSAND entity (ST, GW or SAT) with required plugins
#TODO useless ?
#sudo apt install $folder/opensand_5.1.2_amd64.deb




