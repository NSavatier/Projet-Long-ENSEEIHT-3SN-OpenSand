#/bin/bash
cd ../../opensand-manager
sudo python2.7 setup.py install
returnVal=$?

exit $returnVal

