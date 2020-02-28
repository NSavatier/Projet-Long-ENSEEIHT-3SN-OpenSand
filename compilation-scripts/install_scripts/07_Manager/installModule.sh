#/bin/bash
cd ../../opensand/opensand-manager
sudo python2.7 setup.py install
returnVal=$?

exit $returnVal

