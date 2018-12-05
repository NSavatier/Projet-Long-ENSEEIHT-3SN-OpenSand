#!/usr/bin/env python2
# -*- coding: utf-8 -*-

#
#
# OpenSAND is an emulation testbed aiming to represent in a cost effective way a
# satellite telecommunication system for research and engineering activities.
#
#
# Copyright © 2018 TAS
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

# Author: Vincent Duvert / Viveris Technologies <vduvert@toulouse.viveris.com>


"""
env_plane_dispatcher.py - Dispatch events generated by the environment plane
                          model
"""


from opensand_manager_core.loggers.levels import LOG_LEVELS


class EnvPlaneDispatcher(object):
    """
    Environment plane dispatcher object
    """
    def __init__(self, controller, frontend):
        self._event_notebook = None
        self._controller = controller
        self._frontend = frontend

        controller.set_observer(self)


    def program_list_changed(self):
        """
        The controller reported that the program list has changed
        """
        programs = {}
        for program in self._controller.get_programs():
            programs[program.ident] = program

        self._frontend.on_program_list_changed(programs)

    def new_probe_value(self, probe, time, value):
        """
        The controller reported a new probe value
        """
        self._frontend.on_new_probe_value(probe, time, value)

    def new_log(self, program, name, level, message):
        """
        The controller reported a new log
        """
        if level in LOG_LEVELS and LOG_LEVELS[level].critical:
            program.handle_critical_log()

        self._frontend.on_new_program_log(program, name, level, message)

