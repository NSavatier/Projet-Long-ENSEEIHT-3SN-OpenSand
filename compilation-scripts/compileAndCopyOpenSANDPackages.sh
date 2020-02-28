#/bin/bash

#this script copies this whole shared folder into /tmp/opensand_installer_dir
#and then cds to it

#check that the command was run as sudo, else demand a privilege elevation
[ "$UID" -eq 0 ] || exec sudo bash "$0" "$@"

installDIR="/tmp/opensand_installer_dir"
currentDIR=$(pwd)

rm -rf $installDIR

mkdir -p $installDIR

echo "Copying open-sand archive ..."
cp -R ./opensand.tar.gz $installDIR

cd $installDIR
echo "Extracting open-sand archive ..."
tar -zxf opensand.tar.gz

echo "done !"

cd $installDIR/opensand

#set rights to 777
chmod -R 777 .

#build and install opensand
./opensand-packaging/build-pkgs -s . -d ../workspace -t xenial all

#set rights to 777
chmod -R 777 .

#check that all packages were correctly created
folder=/share/opensand-compiled-packages
#test -f $folder/libopensand-output_5.1.2_amd64.deb && test -f $folder/libopensand-output-dbg_5.1.2_amd64.deb && test -f $folder/libopensand-output-dev_5.1.2_amd64.deb 

#&& test -f $folder/libopensand-conf_5.1.2_amd64.deb && test -f $folder/libopensand-conf-dbg_5.1.2_amd64.deb && test -f $folder/libopensand-conf-dev_5.1.2_amd64.deb 

#&& test -f $folder/libopensand-rt_5.1.2_amd64.deb && test -f $folder/libopensand-rt-dbg_5.1.2_amd64.deb && test -f $folder/libopensand-rt-dev_5.1.2_amd64.deb 

#&& test -f $folder/opensand-core_5.1.2_amd64.deb && test -f $folder/opensand-core-conf_5.1.2_amd64.deb && test -f $folder/opensand-core-dbg_5.1.2_amd64.deb

#&& test -f $folder/libopensand-plugin_5.1.2_amd64.deb && test -f $folder/libopensand-plugin-dbg_5.1.2_amd64.deb && test -f $folder/libopensand-plugin-dev_5.1.2_amd64.deb

#&& test -f $folder/opensand-daemon_5.1.2_amd64.deb 

#&& test -f $folder/opensand-collector_5.1.2_amd64.deb 

#&& test -f $folder/opensand-manager_5.1.2_amd64.deb && test -f $folder/opensand-manager-core_5.1.2_amd64.deb && test -f $folder/opensand-manager-gui_5.1.2_amd64.deb

#&& test -f $folder/libopensand-gse-encap-plugin_5.1.2_amd64.deb && test -f $folder/libopensand-gse-encap-plugin-conf_5.1.2_amd64.deb && test -f $folder/libopensand-gse-encap-plugin-dbg_5.1.2_amd64.deb && test -f $folder/libopensand-gse-encap-plugin-manager_5.1.2_amd64.deb

#&& test -f $folder/libopensand-rle-encap-plugin_5.1.2_amd64.deb && test -f $folder/libopensand-rle-encap-plugin-conf_5.1.2_amd64.deb && test -f $folder/libopensand-rle-encap-plugin-dbg_5.1.2_amd64.deb && test -f $folder/libopensand-rle-encap-plugin-manager_5.1.2_amd64.deb 

#&& test -f $folder/libopensand-rohc-lan-adapt-plugin_5.1.2_amd64.deb && test -f $folder/libopensand-rohc-lan-adapt-plugin-conf_5.1.2_amd64.deb && test -f $folder/libopensand-rohc-lan-adapt-plugin-dbg_5.1.2_amd64.deb && test -f $folder/libopensand-rohc-lan-adapt-plugin-manager_5.1.2_amd64.deb

#&& echo "Tous les paquets sont presents"
#|| echo "!!! Des paquets sont manquants !!!"

#copy compiled packages in /share/opensand-compiled-packages
rm -rf /share/opensand-compiled-packages
mkdir -p /share/opensand-compiled-packages
cp -R $installDIR/workspace/pkgs/* /share/opensand-compiled-packages

#set rights to 777
sudo chmod -R 777 /share/opensand-compiled-packages

