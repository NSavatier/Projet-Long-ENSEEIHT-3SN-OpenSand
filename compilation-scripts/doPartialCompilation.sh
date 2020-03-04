#/bin/bash

#this script copies this whole shared folder into /tmp/opensand_installer_dir
#and then cds to it

startingDir=$(pwd)

installDIR="/tmp/opensand_installer_dir"

sudo rm -rf $installDIR

mkdir -p $installDIR

echo "Copying open-sand archive ..."
cp -R ./opensand.tar.gz $installDIR

echo "Copying install_scripts"
cp -R ./install_scripts $installDIR

cd $installDIR
echo "Extracting open-sand archive ..."
tar -zxf opensand.tar.gz

echo "Launching partial compilation and installation"
sudo ./install_scripts/buildAndInstallAll.sh

cd $startingDir

echo "done !"
#return
