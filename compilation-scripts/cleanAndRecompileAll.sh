#!/bin/bash

# ALTERNATIVE COMPILATION SCRIPT

#=> Purge open sand from your system by running : 
cd purgeOpenSand
./removeAndPurgeOpenSand.sh
cd ..

#=> Do the partial compilation of opensand (will compile and install opensand Optput, Conf, and RT
source ./doPartialCompilation.sh
returnVal=$?
cd /share

#=> Finally, do the compilation and packaging
if [ "$returnVal" -eq "0" ]; then
	./compileAndCopyOpenSANDPackages.sh
else 
	exit 1
fi
