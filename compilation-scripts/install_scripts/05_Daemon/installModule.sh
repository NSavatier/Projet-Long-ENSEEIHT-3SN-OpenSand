#/bin/bash
cd ../../opensand/opensand-daemon
sudo python2.7 setup.py install
returnVal=$?

exit $returnVal

