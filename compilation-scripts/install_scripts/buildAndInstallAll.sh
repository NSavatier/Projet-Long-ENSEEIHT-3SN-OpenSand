#/bin/bash

#copy archive, extract it, and go in the extracted folder
#source ./install_scripts/deploy_locally_shared_folder.sh

cd ./install_scripts

#install compilation requirements
./00_Compilation_Requirements/installCompilationRequirements.sh
returnVal=$?


#function declaration of a module install
#IN : ($1) - Installation folder
install_module () {
    #if last installation succeeded, install the module
    if [ "$returnVal" -eq "0" ]; then
	#install the module
        cd ./$1
	./00_compileAndInstallModule.sh
	returnVal=$?
    else 
        #else, installation failed, close the program
        echo "INSTALLATION FAILED ! See above for errors."
        exit 1
    fi
    cd ..
}

#calls to the previously defined function to install each module
install_module "01_Output"
install_module "02_Configuration"
install_module "03_RealTime"
#install_module "04_OpenSandCore" #this was decommented in last run
#install_module "05_Daemon"
#install_module "06_Collector"
#install_module "07_Manager"
#install_module "08_GSE"
#install_module "09_RLE"
#install_module "10_ROHC"

#Everything went well, signal it and exit
echo "Partial Compilation and install successful."
echo "Exiting."
exit 0

