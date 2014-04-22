#!/usr/bin/env python2
# -*- coding: utf-8 -*-

#
#
# OpenSAND is an emulation testbed aiming to represent in a cost effective way a
# satellite telecommunication system for research and engineering activities.
#
#
# Copyright © 2014 TAS
#
#
# This file is part of the OpenSAND testbed.
#
#
# OpenSAND is free software : you can redistribute it and/or modify it under the
# terms of the GNU General Public License as published by the Free Software
# Foundation, either version 3 of the License, or (at your option) any later
# version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY, without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
# details.
#
# You should have received a copy of the GNU General Public License along with
# this program. If not, see http://www.gnu.org/licenses/.
#
#

# Author: Julien BERNARD / <jbernard@toulouse.viveris.com>

"""
global_configuration.py - the global configuration description
"""

import os
import shutil

from opensand_manager_core.model.host_advanced import AdvancedHostModel
from opensand_manager_core.my_exceptions import XmlException, ModelException
from opensand_manager_core.opensand_xml_parser import XmlParser

DEFAULT_CONF = "/usr/share/opensand/core_global.conf"
GLOBAL_XSD = "/usr/share/opensand/core_global.xsd"
CONF_NAME = "core_global.conf"

class GlobalConfig(AdvancedHostModel):
    """ Global OpenSAND configuration, displayed in configuration tab """
    def __init__(self, scenario):
        AdvancedHostModel.__init__(self, 'global', scenario)
        self._payload_type = ''
        self._emission_std = ''
#        self._dama = ''
        self._down_forward = {}
        self._up_return = {}
        self._enable_phy_layer = None

    def load(self, name, scenario):
        """ load the global configuration """
        # create the host configuration directory
        conf_path = scenario
        if not os.path.isdir(conf_path):
            try:
                os.makedirs(conf_path, 0755)
            except OSError, (_, strerror):
                raise ModelException("failed to create directory '%s': %s" %
                                     (conf_path, strerror))

        self._conf_file = os.path.join(conf_path, CONF_NAME)
        # copy the configuration template in the destination directory
        # if it does not exist
        if not os.path.exists(self._conf_file):
            try:
                shutil.copy(DEFAULT_CONF, self._conf_file)
            except IOError, msg:
                raise ModelException("failed to copy %s configuration file in "
                                     "'%s': %s" % (name, self._conf_file, msg))

        self._xsd = GLOBAL_XSD

        try:
            self._configuration = XmlParser(self._conf_file, self._xsd)
        except IOError, msg:
            raise ModelException("cannot load configuration: %s" % msg)
        except XmlException, msg:
            raise ModelException("failed to parse configuration: %s"
                                 % msg)

    def save(self):
        """ save the configuration """
        try:
            self._configuration.set_value(self._payload_type,
                                          "//satellite_type")
#            self._configuration.set_value(self._dama, "//dama_algorithm")
            self.set_stack('down_forward_encap_schemes',
                           self._down_forward, 'encap')
            self.set_stack('up_return_encap_schemes',
                           self._up_return, 'encap')
            self._configuration.set_value(self._enable_phy_layer,
                                          "//physical_layer/enable")
            self._configuration.write()
        except XmlException:
            raise

    def set_payload_type(self, val):
        """ set the payload_type value """
        self._payload_type = val

    def get_payload_type(self):
        """ get the payload_type value """
        return self.get_param('satellite_type')

    def set_emission_std(self, val):
        """ set the payload_type value """
        self._emission_std = val

    def get_emission_std(self):
        """ get the payload_type value """
#        if self.get_dama() != 'MF2-TDMA':
#           return "DVB-RCS"
#       else:
#           return "DVB-S2"
        return "DVB-RCS"

#    def set_dama(self, val):
#        """ set the dama value """
#        self._dama = val
#
#    def get_dama(self):
#        """ get the dama value """
#        return self.get_param("dama_algorithm")

    def set_up_return_encap(self, stack):
        """ set the up_return_encap_schemes values """
        self._up_return = stack

    def get_up_return_encap(self):
        """ get the up_return_encap_schemes values """
        encap = self.get_stack("up_return_encap_schemes", 'encap')
        return encap

    def set_down_forward_encap(self, stack):
        """ set the down_forward_encap_schemes values """
        self._down_forward = stack 

    def get_down_forward_encap(self):
        """ get the down_forward_encap_schemes values """
        encap = self.get_stack("down_forward_encap_schemes", 'encap')
        return encap

    def set_enable_physical_layer(self, val):
        """ set the enable value in physical layer section """
        self._enable_phy_layer = val

    def get_enable_physical_layer(self):
        """ get the enable value from physical layer section """
        return self.get_param("physical_layer/enable")

    def get_param(self, name):
        """ get a parameter in the XML configuration file """
        val = None
        key = self._configuration.get("//" + name)
        if key is not None:
            try:
                val = self._configuration.get_value(key)
            except:
                pass
        return val

        


