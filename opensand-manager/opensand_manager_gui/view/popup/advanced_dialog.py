#!/usr/bin/env python2
# -*- coding: utf-8 -*-

#
#
# OpenSAND is an emulation testbed aiming to represent in a cost effective way a
# satellite telecommunication system for research and engineering activities.
#
#
# Copyright © 2013 TAS
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
advanced_dialog.py - The OpenSAND advanced configuration
"""

import gobject
import gtk
import threading

from opensand_manager_gui.view.window_view import WindowView
from opensand_manager_gui.view.popup.infos import error_popup
from opensand_manager_core.my_exceptions import ModelException, XmlException
from opensand_manager_gui.view.utils.config_elements import ConfigurationTree, \
                                                           ConfigurationNotebook

(TEXT, VISIBLE, ACTIVE, ACTIVATABLE) = range(4)

class AdvancedDialog(WindowView):
    """ an advanced configuration window """
    def __init__(self, model, manager_log):
        WindowView.__init__(self, None, 'advanced_dialog')

        self._dlg = self._ui.get_widget('advanced_dialog')
        self._dlg.set_keep_above(True)
        self._model = model
        self._log = manager_log
        self._host_tree = None
        self._host_conf_view = None
        self._current_host_notebook = None
        self._hosts_name = []
        self._enabled = []
        self._saved = []
        self._host_lock = threading.Lock()
        self._refresh_trees = None
        self._current_host = None
        # modules
        self._modules_tree = {}
        self._modules_conf_view = None
        self._modules_tree_view = None
        self._modules_name = {}
        self._module_label = None
        self._current_module_notebook = None
        self._all_modules = False

    def go(self):
        """ run the window """
        try:
            self.load()
        except ModelException, msg:
            error_popup(str(msg))
        self._dlg.set_title("Advanced configuration - OpenSAND Manager")
        self._dlg.set_icon_name('gtk-properties')
        self._dlg.run()

    def close(self):
        """ close the window """
        if self._refresh_trees is not None:
            gobject.source_remove(self._refresh_trees)
        self._dlg.destroy()

    def on_advanced_dialog_delete_event(self, source=None, event=None):
        """ delete-event on window """
        self.close()

    def load(self):
        """ load the hosts configuration """
        self.reset()

        # add a widget to the scroll view, once it will be destroyed the
        # scroll view will also be destroyed because it won't have
        # reference anymore with the vbox we can add the new widget before
        # destroying the older
        self._host_conf_view = gtk.VBox()
        host_config = self._ui.get_widget('host_config')
        host_config.add_with_viewport(self._host_conf_view)

        treeview = self._ui.get_widget('hosts_selection_tree')
        self._host_tree = ConfigurationTree(treeview, 'Host', 'Enabled',
                                            self.on_host_selected,
                                            self.toggled_cb)
        for host in [elt for elt in self._model.get_hosts_list()
                         if elt.is_enabled()]:
            self._enabled.append(host)

        # add the global configuration
        gobject.idle_add(self._host_tree.add_host, self._model,
                         {})

        # get the modules tree
        self._modules_conf_view = gtk.VBox()
        modules_config = self._ui.get_widget('plugins_config')
        modules_config.add_with_viewport(self._modules_conf_view)
        self._modules_tree_view = self._ui.get_widget('plugins_tree')

        # update trees immediatly then add a periodic update
        self.update_trees()
        self._refresh_trees = gobject.timeout_add(1000,
                                                 self.update_trees)

        # disable apply button
        self._ui.get_widget('apply_advanced_conf').set_sensitive(False)

    def reset(self):
        """ reset the advanced configuration """
        self._host_lock.acquire()
        for host in self._model.get_hosts_list() + [self._model]:
            adv = host.get_advanced_conf()
            if adv is None:
                self._host_lock.release()
                error_popup("cannot get advanced configuration for %s" %
                            host.get_name().upper())
                self._host_lock.acquire()
                continue

            try:
                adv.reload_conf()
            except ModelException, msg:
                self._host_lock.release()
                error_popup("error when reloading %s advanced configuration: %s"
                            % (host.get_name().upper(), msg))
                self._host_lock.acquire()

            # remove modules configuration view to reload them
            for module in host.get_modules():
                module.set_conf_view(None)
        self._host_lock.release()

    def update_trees(self):
        """ update the host and modules trees """
        self._host_lock.acquire()

        for host in [elt for elt in self._model.get_hosts_list()
                         if elt.get_name() not in self._hosts_name]:
            name = host.get_name()
            self._hosts_name.append(name)
            gobject.idle_add(self._host_tree.add_host, host)

        real_names = []
        for host in self._model.get_hosts_list():
            real_names.append(host.get_name())

        old_host_names = set(self._hosts_name) - set(real_names)
        for host_name in old_host_names:
            gobject.idle_add(self._host_tree.del_host, host_name.upper())
            self._hosts_name.remove(host_name)
            # old host, remove module tree
            del self._modules_tree[host_name]

        # update modules
        self.update_modules_tree()
        self._host_lock.release()

        # continue to refresh
        return True

    def update_modules_tree(self):
        """ update the modules tree """
        if self._current_host is None:
            return

        host_name = self._current_host.get_name()

        if not host_name in self._modules_tree:
            # new host, add a module tree
            treeview = gtk.TreeView()
            self._modules_tree[host_name] = \
                    ConfigurationTree(treeview, 'Plugin', '',
                                      self.on_module_selected, None)
            self._modules_name[host_name] = []
        tree =  self._modules_tree[host_name]

        modules = self.get_used_modules()
        used_names = []
        for module in modules:
            module_name = module.get_name()
            used_names.append(module_name)
            if module_name in self._modules_name[host_name]:
                # module already loaded in tree
                continue
            # second argument to say if we use the module_type in tree
            gobject.idle_add(tree.add_module, module, self._all_modules)
            # add module in dic
            self._modules_name[host_name].append(module_name)

        if self._all_modules:
            # nothing more to do
            return

        old_names = set(self._modules_name[host_name]) - set(used_names)
        for module_name in old_names:
            gobject.idle_add(tree.del_elem, module_name)
            self._modules_name[host_name].remove(module_name)

    def get_used_modules(self):
        """ get the modules used by a host """
        all_modules = list(self._current_host.get_modules())
        # header modifications modules have their configuration in st and gw
        # but a global target si get them
        all_modules += self._model.get_global_lan_adaptation_modules().values()
        if self._all_modules:
            return all_modules

        with_phy_layer = self._model.get_conf().get_enable_physical_layer()
        modules = []
        adv = self._current_host.get_advanced_conf()
        try:
            modules += adv.get_stack("lan_adaptation_schemes",
                                     'proto').itervalues()
        except ModelException:
            pass
        try:
            modules += adv.get_stack("up_return_encap_schemes",
                                     'encap').itervalues()
        except ModelException:
            pass
        try:
            modules += adv.get_stack("down_forward_encap_schemes",
                                     'encap').itervalues()
        except ModelException:
            pass
        if with_phy_layer == "true":
            modules += adv.get_params("attenuation_model_type")
            modules += adv.get_params("minimal_condition_type")
            modules += adv.get_params("error_insertion_type")
        used_modules = []
        for module in all_modules:
            if module.get_name() in modules:
                used_modules.append(module)
        return used_modules


    def on_host_selected(self, selection):
        """ callback called when a host is selected """
        self._modules_conf_view.hide_all()
        for widget in self._modules_tree_view.get_children():
            self._modules_tree_view.remove(widget)

        (tree, iterator) = selection.get_selected()
        if iterator is None:
            self._host_conf_view.hide_all()
            return

        name = tree.get_value(iterator, TEXT).lower()
        host = self._model.get_host(name)
        if host is None:
            error_popup("cannot find host model for %s" % name.upper())
            self._host_conf_view.hide_all()
            return
        self._current_host = host

        adv = host.get_advanced_conf()
        config = None
        if adv is not None:
            config = adv.get_configuration()
        if config is None:
            tree.set(iterator, ACTIVATABLE, False, ACTIVE, False)
            self._host_conf_view.hide_all()
            return

        notebook = adv.get_conf_view()
        if notebook is None:
            notebook = ConfigurationNotebook(config, self.handle_param_chanded)

        adv.set_conf_view(notebook)
        if notebook != self._current_host_notebook:
            self._host_conf_view.hide_all()
            self._host_conf_view.pack_start(notebook)
            if self._current_host_notebook is not None:
                self._host_conf_view.remove(self._current_host_notebook)
            self._current_host_notebook = notebook
        self._host_conf_view.show_all()

        self.update_modules_tree()
        self._modules_tree_view.add(self._modules_tree[name].get_treeview())
        self._modules_tree_view.show_all()
        # call on_module_selected if a plugin is already selected
        self.on_module_selected(self._modules_tree[name].get_selection())

    def on_module_selected(self, selection):
        """ callback called when a host is selected """
        (tree, iterator) = selection.get_selected()
        if self._module_label is not None:
            self._modules_conf_view.remove(self._module_label)
            self._module_label = None
        if iterator is None:
            self._modules_conf_view.hide_all()
            return
        if tree.iter_parent(iterator) == None and self._all_modules:
            # plugin category, nothin to do
            return

        module_name = tree.get_value(iterator, TEXT)
        module = self._current_host.get_module(module_name)
        if module is None:
            self._module_label = gtk.Label()
            self._module_label.set_markup("<span size='x-large' background='red' " +
                                          "foreground='white'>" +
                                          "Cannot find %s module</span>" %
                                          (module_name))
            self._modules_conf_view.pack_start(self._module_label)
            if self._current_module_notebook is not None:
                self._modules_conf_view.remove(self._current_module_notebook)
            self._current_module_notebook = None
            self._modules_conf_view.show_all()
            return

        config = module.get_config_parser()
        if config is None:
            self._module_label = gtk.Label()
            self._module_label.set_markup("<span size='large'>Nothing to " +
                                          "configure for this module</span>")
            self._modules_conf_view.pack_start(self._module_label)
            if self._current_module_notebook is not None:
                self._modules_conf_view.remove(self._current_module_notebook)
            self._current_module_notebook = None
            self._modules_conf_view.show_all()
            return
        notebook = module.get_conf_view()
        if notebook is None:
            notebook = ConfigurationNotebook(config, self.handle_param_chanded)
        # TODO set tab label red if the host does not declare this module

        module.set_conf_view(notebook)
        if notebook != self._current_module_notebook:
            self._modules_conf_view.hide_all()
            self._modules_conf_view.pack_start(notebook)
            if self._current_module_notebook is not None:
                self._modules_conf_view.remove(self._current_module_notebook)
            self._current_module_notebook = notebook
        self._modules_conf_view.show_all()

    def toggled_cb(self, cell, path):
        """ enable host toggled callback """
        # modify ACTIVE property
        curr_iter = self._host_tree.get_iter_from_string(path)
        name = self._host_tree.get_value(curr_iter, TEXT).lower()
        val = not self._host_tree.get_value(curr_iter, ACTIVE)
        self._host_tree.set(curr_iter, ACTIVE, val)

        self._log.debug("host %s toggled" % name)
        host = self._model.get_host(name)
        if host is None:
            return
        if val:
            self._enabled.append(host)
        elif host in self._enabled:
            self._enabled.remove(host)

        self._ui.get_widget('apply_advanced_conf').set_sensitive(True)

    def on_apply_advanced_conf_clicked(self, source=None, event=None):
        """ 'clicked' event callback on apply button """
        self._host_lock.acquire()
        for host in self._model.get_hosts_list() + [self._model]:
            host.enable(False)
            if host in self._enabled:
                host.enable(True)
            name = host.get_name()
            adv = host.get_advanced_conf()
            notebook = None
            if adv is not None:
                self._log.debug("save %s advanced configuration" % name)
                notebook = adv.get_conf_view()
            if notebook is None:
                continue
            try:
                notebook.save()
            except XmlException, error:
                self._host_lock.release()
                error_popup("%s: %s" % (name, error), error.description)
                self._host_lock.acquire()
            except BaseException, error:
                self._host_lock.release()
                error_popup("Unknown exception when saving configuration: %s" %
                            (error))
                self._host_lock.acquire()

            # remove modules configuration view to reload them
            modules = host.get_modules()
            for module in modules:
                try:
                    self._log.debug("Save module %s on %s" %
                                    (module.get_name(), name))
                    module.save()
                except XmlException, error:
                    self._host_lock.release()
                    error_popup("Cannot save %s module on %s: %s" %
                                (module.get_name(), name,
                                 error.description))
                    self._host_lock.acquire()

        self._ui.get_widget('apply_advanced_conf').set_sensitive(False)

        # copy the list (do not only copy the address)
        self._saved = list(self._enabled)
        self._host_lock.release()

    def handle_param_chanded(self, source=None, event=None):
        """ 'changed' event on configuration value """
        self._ui.get_widget('apply_advanced_conf').set_sensitive(True)

    def on_save_advanced_conf_clicked(self, source=None, event=None):
        """ 'clicked' event callback on OK button """
        self.on_apply_advanced_conf_clicked(source, event)
        self.close()

    def on_undo_advanced_conf_clicked(self, source=None, event=None):
        """ 'clicked' event callback on undo button """
        # delete vbox to reload advanced configurations
        self.reset()

        self._host_lock.acquire()
        # copy the list (do not only copy the address)
        self._enabled = list(self._saved)

        self._host_tree.foreach(self.select_enabled)
        self._host_lock.release()
        self.on_host_selected(self._host_tree.get_selection())
        self._ui.get_widget('apply_advanced_conf').set_sensitive(False)
        self.close()

    def on_plugins_checkbutton_toggled(self, source=None, event=None):
        """ The see all plugins per host checkbutton has been toggled """
        self._all_modules = not self._all_modules
        for widget in self._modules_tree_view.get_children():
            self._modules_tree_view.remove(widget)
        self._modules_tree = {}
        self._modules_name = {}
        self.update_modules_tree()
        if self._current_host is not None:
            treeview =  self._modules_tree[self._current_host.get_name()].get_treeview()
            self._modules_tree_view.add(treeview)
            self._modules_tree_view.show_all()

    def select_enabled(self, tree, path, iterator):
        """ store the saved enabled value in advanced model
            (used in callback so no need to use locks) """
        tree.set(iterator, ACTIVE, False)
        name = tree.get_value(iterator, TEXT).lower()
        for host in self._enabled:
            if host.get_name() == name:
                host.enable(True)
                tree.set(iterator, ACTIVE, True)


if __name__ == "__main__":
    from opensand_manager_core.loggers.manager_log import ManagerLog
    from opensand_manager_core.opensand_model import Model

    LOGGER = ManagerLog('debug', True, True, True)
    MODEL = Model(LOGGER)
    WindowView(None, 'none', 'opensand.glade')
    DIALOG = AdvancedDialog(MODEL, LOGGER)
    DIALOG.go()
    DIALOG.close()
