#/bin/bash

currentDIR=$(pwd)

echo "$currentDIR compilation ..."

#compile module
./compileModule.sh
returnVal=$?

#if compilation succeeded, install module
if [ "$returnVal" -eq "0" ]; then
   echo "$currentDIR installation ..."
   ./installModule.sh
else 
    echo "$currentDIR module : compilation failed. See error above."
    exit 1
fi

#if everything went well, return 0 to signal installatyion success
exit $?
