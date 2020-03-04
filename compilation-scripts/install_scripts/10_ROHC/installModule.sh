#/bin/bash

#note : before running this script, make sure that compileOutput.sh has been run
#on the host system !!! (the one containing the shared folder)
#else the installed version will probably be an old one or not work

#check that the command was run as sudo, else demand a privilege elevation
[ "$UID" -eq 0 ] || exec sudo bash "$0" "$@"

currentDIR=$(pwd)

cd ../../opensand-plugins/lan_adaptation/rohc
returnVal=$?

#execute make and exit if it fails
if [ "$returnVal" -eq "0" ]; then
    make install
else 
    echo "$currentDIR module : make failed. See error above."
    exit 1
fi

exit 0

