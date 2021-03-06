#!/usr/bin/env python2
# -*- coding: utf-8 -*-

#
#
# OpenSAND is an emulation testbed aiming to represent in a cost effective way a
# satellite telecommunication system for research and engineering activities.
#
#
# Copyright © 2019 TAS
# Copyright © 2019 CNES
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
ethernet.py - The Ethernet lan adaptation module
"""

from opensand_manager_core.module import LanAdaptationModule

class EthernetModule(LanAdaptationModule):
    
    _name = 'Ethernet'
    
    def __init__(self):
        LanAdaptationModule.__init__(self)
        self._upper = ("IP",)
        self._handle_upper_bloc = True
        self._iface_type = "TAP"
        self._xml = 'ethernet.conf'
        self._xsd  = 'ethernet.xsd'
        start = "<span size='x-large' foreground='#1088EB'><b>"
        end = "</b></span>"
        self._description = "%sEthernet lan adaptation plugin for OpenSAND%s" % \
                            (start, end)
