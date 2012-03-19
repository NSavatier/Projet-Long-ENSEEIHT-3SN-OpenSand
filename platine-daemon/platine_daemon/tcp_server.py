#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Author: Julien BERNARD / Viveris Technologies <jbernard@toulouse.viveris.com>

"""
tcp_server.py - server that get Platine commands
"""

import SocketServer
import select
import logging
from platine_daemon.my_exceptions import Timeout
from platine_daemon.stream import TIMEOUT

#macros
LOGGER = logging.getLogger('PtDmon')

class Plop(SocketServer.TCPServer):
    """ The TCP socket server with reuse address to true """

    allow_reuse_address = True

    def __init__(self, *args):
        SocketServer.TCPServer.__init__(self, *args)

    def run(self):
        """ run the TCP server """
        try:
            self.serve_forever()
        except Exception:
            raise

    def stop(self):
        """ stop the TCP server """
        self.shutdown()


class MyTcpHandler(SocketServer.StreamRequestHandler):
    """ The RequestHandler class for the TCP server """

    # line buffered
    rbufsize = 1
    wbufsize = 0

    def setup(self):
        """ the function called when MyTcpHandler is created """
        self.connection = self.request
        self.rfile = self.connection.makefile('rb', self.rbufsize)
        self.wfile = self.connection.makefile('wb', self.wbufsize)
        self._data = ''

    def finish(self):
        """ the function called when handle returns """
        LOGGER.info("close connection")
        if not self.wfile.closed:
            self.wfile.flush()
        self.wfile.close()
        self.rfile.close()

    def read_data(self, timeout = True):
        """ read data on socket.
            Can raise Timeout and EOFError exceptions """
        if timeout:
            inputready, out, err = select.select([self.rfile], [], [], TIMEOUT)
            if(len(inputready) == 0):
                LOGGER.warning("timeout when trying to read")
                raise Timeout

        self._data = self.rfile.readline().strip()
        if (self._data == ''):
            LOGGER.info("distant socket is closed")
            raise EOFError


