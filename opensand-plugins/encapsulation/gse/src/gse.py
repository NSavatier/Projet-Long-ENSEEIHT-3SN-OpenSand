#!/usr/bin/env python2
# -*- coding: utf-8 -*-

#
#
# OpenSAND is an emulation testbed aiming to represent in a cost effective way a
# satellite telecommunication system for research and engineering activities.
#
#
# Copyright © 2014 CNES
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
gse.py - The GSE encapsulation module
"""

from opensand_manager_core.module import EncapModule

class GseModule(EncapModule):
    
    _name = 'GSE'
    
    def __init__(self):
        EncapModule.__init__(self)
        self._handle_upper_bloc = True
        self._upper['regenerative'] = ("AAL5/ATM",
                                       "MPEG2-TS",)
        self._xml = 'gse.conf'
        self._xsd  = 'gse.xsd'
        start = "<span size='x-large' foreground='#1088EB'><b>"
        end = "</b></span>"
        self._description = "%sGSE encapsulation plugin for OpenSAND%s" % \
                            (start, end)
        self._condition['dvb-rcs'] = False
