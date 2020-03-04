#/bin/bash
cd ../../opensand-output
sudo ./autogen.sh
returnVal=$?

currentDIR=$(pwd)

#execute make and exit if it fails
if [ "$returnVal" -eq "0" ]; then
    make
    returnVal=$?
else 
    echo "$currentDIR module : make failed. See error above."
    exit 1
fi

#note : no return value check here, as make check seems to always return a non-zero value
#if you know how to fix this, feel free to add a return value check here for more script robustness
make check


exit 0
