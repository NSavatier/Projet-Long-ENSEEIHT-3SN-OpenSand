#/bin/bash
cd ../../opensand/opensand-collector
sudo python2.7 setup.py install
returnVal=$?

exit $returnVal

