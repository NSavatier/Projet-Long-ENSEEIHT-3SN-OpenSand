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
# Author: Joaquin MUGUERZA / <jbernard@toulouse.viveris.com>


"""
routes.py - Manage routes for OpenSAND
"""

import threading
import logging
import pickle
import os
import re
from ipaddr import IPNetwork
from opensand_daemon.nl_utils import NlRoute, NlError, NlExists, NlMissing
from lxml import etree

CONF_DIR = "/etc/opensand/"
TOPOLOGY_FILE = "topology.conf"
CACHE_DIR = "/var/cache/sand-daemon/"

#macros
LOGGER = logging.getLogger('sand-daemon')
ROUTE_FILE = "routes"

class OpenSandRoutes(object):
    """ manage the routes for OpenSAND """

    # the OpenSANDRoutes class attributes are shared between command and service
    # threads
    _name = None
    _instance = None
    _routes_lock = threading.Lock()
    _route_hdl = None
    _routes_v4 = {} # the available host and IPv4 networks
    _routes_v6 = {} # the available host and IPv6 networks
    _started = False
    _initialized = False
    _iface = None
    _unused = True
    _is_ws = False
    _cache_dir = CACHE_DIR

    def __init__(self):
        pass

    def delete(self):
        if OpenSandRoutes._is_ws:
            self.remove_routes()

    def load(self, cache_dir, name, iface, is_ws=False, instance=""):
        OpenSandRoutes._routes_lock.acquire()
        OpenSandRoutes._cache_dir = cache_dir
        OpenSandRoutes._name = name.lower()
        OpenSandRoutes._instance = str(instance)
        try:
            OpenSandRoutes._route_hdl = NlRoute(iface)
        except KeyError:
            LOGGER.error("unable to find interface %s" % iface)

        OpenSandRoutes._iface = iface
        OpenSandRoutes._is_ws = is_ws

        # read the routes file
        routes = {}
        try:
            route_file = open(os.path.join(OpenSandRoutes._cache_dir,
                                           ROUTE_FILE), 'rb')
        except IOError, (errno, strerror):
            LOGGER.debug("the route file '%s' cannot be read: "
                         "assume the process are stopped, "
                         "keep an empty route list" %
                         os.path.join(OpenSandRoutes._cache_dir, ROUTE_FILE))
            LOGGER.debug("route list is initialized")
        else:
            try:
                routes = pickle.load(route_file)
            except EOFError:
                LOGGER.info("route file is empty")
                OpenSandRoutes._started = False
                LOGGER.debug("route list is initialized")
            except pickle.PickleError, error:
                LOGGER.error("unable to load the route list: " + str(error))
            else:
                # find routes, assume the platform is started
                OpenSandRoutes._started = True
                for host in routes:
                    OpenSandRoutes._routes_v4[host] = routes[host][0]
                    OpenSandRoutes._routes_v6[host] = routes[host][1]
                LOGGER.debug("route list is initialized")
            finally:
                route_file.close()

        # always set up the routes for WS as they won't receive the 'START'
        # command
        if is_ws:
            OpenSandRoutes._started = True
        OpenSandRoutes._initialized = True
        OpenSandRoutes._unused = False

        OpenSandRoutes._routes_lock.release()
        
    def set_unused(self):
        """ for satellite we do not need routes but we don't want initialization
            errors """
        OpenSandRoutes._initialized = True
        OpenSandRoutes._unused = True

    def is_initialized(self):
        """ is the route object initialized """
        return OpenSandRoutes._initialized

    def add_distant_host(self, name, v4, v6, gw_v4=None, gw_v6=None):
        """ add a new distant host """
        if OpenSandRoutes._unused:
            return
        OpenSandRoutes._routes_lock.acquire()
        LOGGER.info("new distant host %s with addresses %s and %s" %
                    (name, v4, v6))
        if gw_v4 is not None or gw_v6 is not None:
            LOGGER.debug("routers %s %s" % (gw_v4, gw_v6))

        net = IPNetwork(v4)
        prefix_v4 = "%s/%s" % (net.network, net.prefixlen)
        OpenSandRoutes._routes_v4[name] = (prefix_v4, gw_v4)
        net = IPNetwork(v6)
        prefix_v6 = "%s/%s" % (net.network, net.prefixlen)
        OpenSandRoutes._routes_v6[name] = (prefix_v6, gw_v6)

        if OpenSandRoutes._started:
            LOGGER.debug("Platform is started, add a route for this host")
            try:
                self.add_route(name, prefix_v4, prefix_v6, gw_v4, gw_v6)
            except (NlError, NlExists):
                OpenSandRoutes._routes_lock.release()
                raise
            self.serialize()
        OpenSandRoutes._routes_lock.release()

    def remove_distant_host(self, host):
        """ remove a distant host """
        if OpenSandRoutes._unused:
            return
        OpenSandRoutes._routes_lock.acquire()
        LOGGER.debug("remove distant host %s" % host)
        if OpenSandRoutes._started:
            LOGGER.debug("Platform is started, remove the route for this host")
            v4 = None
            v6 = None
            gw_v4 = None
            gw_v6 = None
            try:
                v4 = OpenSandRoutes._routes_v4[host][0]
                gw_v4 = OpenSandRoutes._routes_v4[host][1]
            except KeyError, TypeError:
                pass
            try:
                v6 = OpenSandRoutes._routes_v6[host][0]
                gw_v6 = OpenSandRoutes._routes_v6[host][1]
            except KeyError, TypeError:
                pass
            try:
                self.remove_route(host, v4, v6, gw_v4, gw_v6)
            except NlError:
                pass
            if v4:
                del OpenSandRoutes._routes_v4[host]
            if v6:
                del OpenSandRoutes._routes_v6[host]
            self.serialize()
        else:
            try:
                del OpenSandRoutes._routes_v4[host]
                del OpenSandRoutes._routes_v6[host]
            except KeyError, TypeError:
                pass
        OpenSandRoutes._routes_lock.release()

    def setup_routes(self, iface):
        """ apply the routes when started """
        if OpenSandRoutes._unused:
            return
        # read the conf and get the gw_id by tal_ids
        tal_gw_ids = self.get_all_gw_by_tal()
        try:
            curr_gw_id = tal_gw_ids[OpenSandRoutes._instance]
        except KeyError:
            curr_gw_id = tal_gw_ids["*"]
        OpenSandRoutes._routes_lock.acquire()
        # update the routes gateway
        if iface is not None:
            try:
                OpenSandRoutes._iface = iface
                OpenSandRoutes._route_hdl = NlRoute(iface)
            except KeyError:
                LOGGER.error("unable to find interface %s" % iface)

        OpenSandRoutes._started = True
        LOGGER.info("set route before starting platform")
        self.serialize()
        for host in set(OpenSandRoutes._routes_v4.keys() +
                        OpenSandRoutes._routes_v6.keys()):
            # get host terminal id
            ret = re.findall("^(?:[^0-9]*)([0-9]*)", host)
            if not len(ret):
                continue
            host_id = ret[0]
            # check if host in same_spot list
            try:
                gw_id = tal_gw_ids[host_id]
            except KeyError:
                gw_id = tal_gw_ids["*"]
            if gw_id != curr_gw_id:
                continue
            v4 = None
            v6 = None
            gw_v4 = None
            gw_v6 = None
            try:
                v4 = OpenSandRoutes._routes_v4[host][0]
                gw_v4 = OpenSandRoutes._routes_v4[host][1]
            except KeyError, TypeError:
                pass
            try:
                v6 = OpenSandRoutes._routes_v6[host][0]
                gw_v6 = OpenSandRoutes._routes_v6[host][1]
            except KeyError, TypeError:
                pass
            try:
                self.add_route(host, v4, v6, gw_v4, gw_v6)
            except (NlError, NlExists):
                OpenSandRoutes._routes_lock.release()
                raise
        OpenSandRoutes._routes_lock.release()

    def remove_routes(self):
        """ remove the current routes when stopped """
        if OpenSandRoutes._unused:
            return
        # read the conf and get the gw_id by tal_ids
        tal_gw_ids = self.get_all_gw_by_tal()
        try:
            curr_gw_id = tal_gw_ids[OpenSandRoutes._instance]
        except KeyError:
            curr_gw_id = tal_gw_ids["*"]
        OpenSandRoutes._routes_lock.acquire()
        OpenSandRoutes._started = False
        LOGGER.info("remove route after stopping platform")
        for host in set(OpenSandRoutes._routes_v4.keys() +
                        OpenSandRoutes._routes_v6.keys()):
            # get host terminal id
            ret = re.findall("^(?:[^0-9]*)([0-9]*)", host)
            if not len(ret):
                continue
            host_id = ret[0]
            # check if host in same_spot list
            try:
                gw_id = tal_gw_ids[host_id]
            except KeyError:
                gw_id = tal_gw_ids["*"]
            if gw_id != curr_gw_id:
                continue
            v4 = None
            v6 = None
            gw_v4 = None
            gw_v6 = None
            try:
                v4 = OpenSandRoutes._routes_v4[host][0]
                gw_v4 = OpenSandRoutes._routes_v4[host][1]
            except KeyError, TypeError:
                pass
            try:
                v6 = OpenSandRoutes._routes_v6[host][0]
                gw_v6 = OpenSandRoutes._routes_v6[host][1]
            except KeyError, TypeError:
                pass
            try:
                self.remove_route(host, v4, v6, gw_v4, gw_v6)
            except NlMissing:
                pass
        try:
            os.remove(os.path.join(OpenSandRoutes._cache_dir, ROUTE_FILE))
        except OSError:
            pass
        OpenSandRoutes._routes_lock.release()

    def add_route(self, host, route_v4, route_v6, gw_v4, gw_v6):
        """ add a new route """
        if OpenSandRoutes._unused:
            return
        if host == OpenSandRoutes._name:
            LOGGER.warning("Try to add route for myself...")
            del OpenSandRoutes._routes_v4[host]
            del OpenSandRoutes._routes_v6[host]
            return
        LOGGER.info("add routes for host %s toward %s and %s via %s" %
                    (host, route_v4, route_v6, OpenSandRoutes._iface))
        if gw_v4 is not None or gw_v6 is not None:
            LOGGER.debug("routers %s %s" % (gw_v4, gw_v6))
        try:
            if route_v4:
                OpenSandRoutes._route_hdl.add(route_v4, gw_v4)
        except NlExists:
            LOGGER.info("route already exists on %s" % host)
        except NlError, msg:
            LOGGER.error("fail to add IPv4 route for %s: %s" % (host, msg))
            del OpenSandRoutes._routes_v4[host]
            raise
        try:
            if route_v6:
                OpenSandRoutes._route_hdl.add(route_v6, gw_v6)
        except NlExists:
            LOGGER.info("route already exists on %s" % host)
        except NlError, msg:
            LOGGER.error("fail to add IPv6 route for %s: %s" % (host, msg))
            del OpenSandRoutes._routes_v6[host]
            raise

    def remove_route(self, host, route_v4, route_v6, gw_v4, gw_v6):
        """ remove a route """
        if OpenSandRoutes._unused:
            return
        LOGGER.info("remove route for host %s toward %s and %s via %s" %
                    (host, route_v4, route_v6, OpenSandRoutes._iface))
        if gw_v4 is not None or gw_v6 is not None:
            LOGGER.debug("routers %s %s" % (gw_v4, gw_v6))
        try:
            if route_v4:
                OpenSandRoutes._route_hdl.delete(route_v4, gw_v4)
        except NlMissing:
            LOGGER.info("route already deleted IPv4 for %s" % host)
        except NlError, msg:
            LOGGER.error("fail to delete IPv4 route for %s: %s" % (host, msg))
            raise
        finally:
            # try to remove the IPv6 route anyway
            try:
                if route_v6:
                    OpenSandRoutes._route_hdl.delete(route_v6, gw_v6)
            except NlMissing:
                LOGGER.info("IPv6 route already deleted for %s" % host)
            except NlError, msg:
                LOGGER.error("fail to delete IPv6 route for %s: %s" % (host, msg))
                raise


    def serialize(self):
        """ serialize the routes in order to keep them in case
            of daemon restart """
        if OpenSandRoutes._unused or OpenSandRoutes._is_ws:
            return
        routes = {}
        for host in set(OpenSandRoutes._routes_v4.keys() +
                        OpenSandRoutes._routes_v6.keys()):
            v4 = None
            v6 = None
            try:
                v4 = OpenSandRoutes._routes_v4[host]
            except KeyError, TypeError:
                pass
            try:
                v6 = OpenSandRoutes._routes_v6[host]
            except KeyError, TypeError:
                pass
            routes[str(host)] = (v4, v6)
        if len(routes) == 0:
            return
        try:
            route_file = open(os.path.join(OpenSandRoutes._cache_dir,
                                           ROUTE_FILE), 'wb')
            pickle.dump(routes, route_file)
        except IOError, (errno, strerror):
            LOGGER.error("unable to create %s file (%d: %s)" %
                         (os.path.join(OpenSandRoutes._cache_dir, ROUTE_FILE),
                          errno, strerror))
        except pickle.PickleError, error:
            LOGGER.error("unable to serialize route list: " + str(error))
            route_file.close()
        else:
            route_file.close()

    def get_all_gw_by_tal(self):
        """ read the topology conf file, and get the IDs of the gateway id
            by terminal id """

        # open xml
        tree = etree.parse(os.path.join(CONF_DIR, TOPOLOGY_FILE))
        root = tree.getroot()

        # get the gateways table
        try:
            childs = tree.xpath("/configuration/gw_table")
        except:
            return []
        if childs is None or len(childs) <= 0:
            return []
        gw_tab = childs[0]

        # get the gateway list 
        default_gw_id = None
        tal_gw_ids = {}
        for child in gw_tab.iterchildren():
            if child.tag == "default_gw":
                # get the default gateway
                default_gw_id = child.text
                continue
            if child.tag != "gw":
                continue
            # get the gateway id
            gw_id = child.attrib["id"]
            tal_gw_ids[gw_id] = gw_id
            # load tal ids
            for child2 in child.xpath("terminals/tal"):
                if child2.tag != "tal":
                    continue
                # add the gateway id of the terminal
                tal_id = child2.attrib["id"]
                tal_gw_ids[tal_id] = gw_id

        # add the default gateway id
        if default_gw_id is not None:
            tal_gw_ids["*"] = default_gw_id
        else:
            tal_gw_ids["*"] = 0

        return tal_gw_ids
