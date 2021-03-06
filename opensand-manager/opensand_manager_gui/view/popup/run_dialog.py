#!/usr/bin/env python2
# -*- coding: utf-8 -*-

#
#
# OpenSAND is an emulation testbed aiming to represent in a cost effective way a
# satellite telecommunication system for research and engineering activities.
#
#
# Copyright © 2019 TAS
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
run_dialog.py - OpenSAND manager scenario dialog
"""

import os
import gtk
import gobject

from opensand_manager_core.utils import ST, GW, SAT
from opensand_manager_gui.view.popup.infos import error_popup
from opensand_manager_gui.view.window_view import WindowView

MAX_ST = 5

class RunDialog(WindowView):
    """ dialog to get the scenario to load """
    def __init__(self, scenario, run):
        WindowView.__init__(self, None, 'run_dialog')

        self._dlg = self._ui.get_widget('run_dialog')
        self._dlg.set_keep_above(True)
        self._dlg.set_modal(True)
        self._dlg.set_icon_name(gtk.STOCK_PREFERENCES)
        # add available runs into combo box
        self._has_runs = self.populate(scenario, run)

    def go(self):
        """ run the window """
        # run the dialog and store the response
        
        run = None
        
        if self._has_runs:
            self._dlg.present()
            result = self._dlg.run()
        
            if result == 0:
                widget = self._ui.get_widget("run_box")
                model = widget.get_model()
                active = widget.get_active_iter()
                run = model.get_value(active, 0)
        
        else:
            error_popup("No runs found in the current scenario.")
        
        self._dlg.destroy()
        
        return run

    def populate(self, scenario, run):
        """ add run elements into the combo box """
        # the list of directories to ignore
        ignore = [SAT,'tools', 'plugins', 'modcod']
        for i in range(MAX_ST + 1):
            ignore.append(ST + str(i))
        for i in [0, 10]:
            ignore.append(GW + str(i))

        
        list_id = 0
        active_id = 0
        store = gtk.ListStore(gobject.TYPE_STRING)
        content = os.listdir(scenario)
        for path in sorted(content):
            if os.path.isdir(os.path.join(scenario, path)) and \
               path not in ignore:
                store.append([path])
                if path == run:
                    active_id = list_id
                list_id += 1

        widget = self._ui.get_widget("run_box")
        widget.set_model(store)
        cell = gtk.CellRendererText()
        widget.pack_start(cell, True)
        widget.add_attribute(cell, 'text', 0)
        widget.set_active(active_id)

        return list_id > 0

    def on_run_dialog_delete_event(self, source=None, event=None):
        """ delete-event on window """
        self.close()

