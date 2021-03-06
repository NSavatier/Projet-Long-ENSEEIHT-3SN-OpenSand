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

# Author: Vincent Duvert / Viveris Technologies <vduvert@toulouse.viveris.com>


"""
service_handler.py - OpenSAND collector Avahi service handler.
"""

from dbus.mainloop.glib import DBusGMainLoop
from dbus.exceptions import DBusException
import gobject
import avahi
import dbus
import logging

LOGGER = logging.getLogger('sand-collector')


def on_error(self, *args):
    """ error handler """
    if len(args) == 0:
        return
    LOGGER.error('service error handler: ' + str(args[0]))

class ServiceHandler(object):
    """
    Avahi service handler to publish the collector listening port (so the
    daemons can find it) and find the OpenSAND daemons and their IPs.
    """

    def __init__(self, host_manager, listen_port, transfer_port,
                 service_type, iface):
        self._host_manager = host_manager
        self._listen_port = listen_port
        self._transfer_port = transfer_port
        self._service_type = service_type
        self._iface = iface
        self._pub_group = None
        self._disco_server = None
        self._known_hosts = set()

    def __enter__(self):
        """
        Publish the service and set up service discovery
        """
        bus = dbus.SystemBus(mainloop=DBusGMainLoop())

        # Publishing
        pub_server = dbus.Interface(bus.get_object(avahi.DBUS_NAME,
                        avahi.DBUS_PATH_SERVER), avahi.DBUS_INTERFACE_SERVER)

        self._pub_group = dbus.Interface(bus.get_object(avahi.DBUS_NAME,
                   pub_server.EntryGroupNew()), avahi.DBUS_INTERFACE_ENTRY_GROUP)

        additional_data = ["transfer_port=%d" % self._transfer_port]

        if self._iface != '':
            try:
                iface = pub_server.GetNetworkInterfaceIndexByName(self._iface)
            except DBusException:
                LOGGER.warning("Cannot publish Avahi service on %s iface",
                               self._iface)
                iface = avahi.IF_UNSPEC
        else:
            iface = avahi.IF_UNSPEC

        try:
            self._pub_group.AddService(iface, avahi.PROTO_INET, dbus.UInt32(0),
                                       "collector", self._service_type, "", "",
                                       dbus.UInt16(self._listen_port),
                                       additional_data)
        except dbus.exceptions.DBusException as error:
            LOGGER.error("cannot add Avahi service (%s)" % error)
            raise KeyboardInterrupt
        self._pub_group.Commit()

        self._mainloop = gobject.MainLoop()
        gobject.threads_init()  # Necessary for the transfer_server thread

        # Discovery
        self._disco_server = dbus.Interface(bus.get_object(avahi.DBUS_NAME,
            '/'), 'org.freedesktop.Avahi.Server')

        disco_browser = dbus.Interface(bus.get_object(avahi.DBUS_NAME,
            self._disco_server.ServiceBrowserNew(iface,
                avahi.PROTO_INET, self._service_type, 'local',
                dbus.UInt32(0))), avahi.DBUS_INTERFACE_SERVICE_BROWSER)

        disco_browser.connect_to_signal("ItemNew", self._handle_new)
        disco_browser.connect_to_signal("ItemRemove", self._handle_remove)

        LOGGER.info("Avahi service handler started.")

        return self
    
    def run(self):
        """ start the mainloop for service listening """
        self._mainloop.run()


    def _handle_new(self, interface, protocol, name, stype, domain, flags):
        """
        Handles a newly detected service
        """
        if name == "collector":
            return

        def error_handler(*_args):
            """
            Error handler for service resolution errors
            """
            LOGGER.error("Unable to get resolve service %s, ignoring.", name)
            return

        self._disco_server.ResolveService(interface, protocol, name, stype,
            domain, avahi.PROTO_INET, dbus.UInt32(0),
            reply_handler=self._handle_resolve, error_handler=error_handler)

    def _handle_resolve(self, *args):
        """
        Called when a detected service is resolved
        """
        name = args[2]
        addr = args[7]
        txt = args[9]

        try:
            items = dict("".join(chr(i) for i in arg).split("=", 1)
                for arg in txt)
            port = int(items.get('ext_port', ""))
        except ValueError:
            LOGGER.error("Failed to get UDP port from '%s' daemon.", name)
            return

        if name in self._known_hosts:
            self._host_manager.add_host_addr(name, (addr, port))
            return

        LOGGER.info("Daemon on host '%s' has address %s:%d.", name, addr, port)
        self._known_hosts.add(name)
        self._host_manager.add_host(name, (addr, port))

    def _handle_remove(self, _interface, _proto, name, _stype, _domain, _flags):
        """
        Called when a service is removed
        """
        if name == "collector":
            return

        LOGGER.info("Daemon on host '%s' has exited.", name)
        self._host_manager.remove_host(name)
        self._known_hosts.discard(name)

    def stop(self):
        """ stop the mainloop """
        self._mainloop.quit()

    def __exit__(self, exc_type, exc_val, exc_tb):
        """
        Unpublish the service and exit the event loop
        """
        self._pub_group.Reset()

        LOGGER.info("Avahi service handler stopped.")

        return False
