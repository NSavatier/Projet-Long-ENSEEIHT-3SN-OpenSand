################################################################################
#   Name       : Makefile
#   Author     : Julien BERNARD <julien.bernard@toulouse.viveris.com>
#   Description: create the ROHC LAN adaptation plugin for OpenSAND
################################################################################

SUBDIRS = 

plugins_LTLIBRARIES = libopensand_rohc_lan_adapt_plugin.la

libopensand_rohc_lan_adapt_plugin_la_cpp = \
	RohcPacket.cpp \
	Rohc.cpp

libopensand_rohc_lan_adapt_plugin_la_h = \
	RohcPacket.h \
	Rohc.h

libopensand_rohc_lan_adapt_plugin_la_SOURCES = \
    $(libopensand_rohc_lan_adapt_plugin_la_cpp) \
    $(libopensand_rohc_lan_adapt_plugin_la_h)

pluginsdir = $(prefix)/lib/opensand/plugins

EXTRA_DIST = \
	rohc.py

