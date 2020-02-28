#/bin/bash
currentDIR=$(pwd)

#install module
echo "$currentDIR installation ..."
./installModule.sh

#if everything went well, return 0 to signal installatyion success
exit $?
