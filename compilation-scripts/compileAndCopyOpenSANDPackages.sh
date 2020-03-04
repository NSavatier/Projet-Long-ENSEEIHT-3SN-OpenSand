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

#set rights to 777
chmod -R 777 .

#build and install opensand
./opensand-packaging/build-pkgs -s . -d ../workspace -t xenial all

#set rights to 777
chmod -R 777 .

#copy compiled packages in $currentDIR/opensand-compiled-packages
rm -rf $currentDIR/opensand-compiled-packages
mkdir -p $currentDIR/opensand-compiled-packages
cp -R $installDIR/../workspace/pkgs/* $currentDIR/opensand-compiled-packages

#set rights to 777
sudo chmod -R 777 $currentDIR/opensand-compiled-packages

