/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2019 TAS
 * Copyright © 2019 CNES
 *
 *
 * This file is part of the OpenSAND testbed.
 *
 *
 * OpenSAND is free software : you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY, without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see http://www.gnu.org/licenses/.
 *
 */

/**
 * @file StackPlugin.h
 * @brief Generic plugin for stack elements
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 * @author Julien Bernard <julien.bernard@toulouse.viveris.com>
 * @author Joaquin Muguerza <joaquin.muguerza@toulouse.viveris.com>
 * @author Aurelien DELRIEU <adelrieu@toulouse.viveris.com>
 */

#ifndef STACK_CONTEXT_H
#define STACK_CONTEXT_H

#include "NetPacket.h"
#include "NetBurst.h"
#include "OpenSandCore.h"
#include "OpenSandPlugin.h"
#include <opensand_output/Output.h>

#include <vector>
#include <algorithm>
#include <map>


using std::map;
using std::vector;

/**
 * @class StackPlugin
 * @brief Generic stack plugin
 */
class StackPlugin: public OpenSandPlugin
{

 public:

	/**
	 * @class StackPacketHandler
	 * @brief Functions to handle the encapsulated packets
	 */
	class StackPacketHandler
	{

	  public:

		/**
		 * @brief StackPacketHandler constructor
		 */
		/* Allow packets to access StackPlugin members */
		StackPacketHandler(StackPlugin &pl): plugin(pl) {};

		/**
		 * @brief StackPacketHandler destructor
		 */
		virtual ~StackPacketHandler() {};

		/**
		 * @brief get the packet length if constant
		 *
		 * @return the packet length if constant, 0 otherwise
		 */
		virtual size_t getFixedLength() const = 0;

		/**
		 * @brief Create a NetPacket from data with the relevant attributes.
		 *
		 * @param data        The packet data
		 * @param data_length The packet length
		 * @param qos         The QoS value to associate with the packet
		 * @param src_tal_id  The source terminal ID to associate with the packet
		 * @param dst_tal_id  The destination terminal ID to associate with the packet
		 *
		 * @return The packet
		 */
		virtual NetPacket *build(const Data &data,
		                         size_t data_length,
		                         uint8_t qos,
		                         uint8_t src_tal_id,
		                         uint8_t dst_tal_id) const = 0;

		/**
		 * @brief Get a packet length
		 *
		 * @param data The packet content
		 * @return the packet length
		 */
		virtual size_t getLength(const unsigned char *data) const = 0;

		/**
		 * @brief Get the EtherType associated with the related protocol
		 *
		 * return The EtherType
		 */
		virtual uint16_t getEtherType() const {return plugin.ether_type;};

		/**
		 * @brief Get the type of stack
		 *
		 * @return the name of the stack
		 */
		virtual string getName() const {return plugin.name;};

		/* The functions below are only used by EncapPlugin but we need them to avoid
		 * casting upper packet handlers for EncapPlugins that does not support
		 * lan adaptation upper packets */

		/**
		 * @brief get the minimum packet length
		 *
		 * @return the minimum packet length
		 */
		virtual size_t getMinLength() const = 0;

		/**
		 * @brief Encapsulate the packet and store unencapsulable part
		 *
		 * @param[in]  packet            The packet to encapsulate
		 * @param[in]  remaining_length  The remaining length
		 * @param[in]  new_burst         The new burst status
		 * @param[out] partial_encap     The status about encapsulation
		 *                               (true if data remains after encapsulation,
		 *                               false otherwise)
		 * @param[out] encap_packet      The encapsulated packet (null in error case)
		 *
		 * @return  true if success, false otherwise
		 */
		virtual bool encapNextPacket(NetPacket *packet,
			size_t remaining_length,
			bool new_burst,
			bool &partial_encap,
			NetPacket **encap_packet) = 0;

		/**
		 * @brief Get encapsulated packet from payload
		 *
		 * @param[in]  packet             The packet storing payload
		 * @param[out] partial_decap      The status about decapsulation (true if data
		 *                                is incomplete to decapsulation, false otherwise)
		 * @param[out] decap_packets      The list of decapsulated packet
		 * @param[in decap_packets_count  The packet count to decapsulate (0 if unknown)
		 */
		virtual bool getEncapsulatedPackets(NetContainer *packet,
			bool &partial_decap,
			vector<NetPacket *> &decap_packets,
			unsigned int decap_packet_count = 0) = 0;

		/**
		 * @brief perform some plugin initialization
		 */
		virtual bool init() = 0;

	  protected:

		/// Output Logs
		OutputLog *log;

	  private:

		StackPlugin &plugin;
	};

	/**
	 * @class StackContext
	 * @brief The stack context
	 */
	class StackContext
	{

	  public:

		/* Allow context to access StackPlugin members */
		/**
		 * @brief StackContext constructor
		 */
		StackContext(StackPlugin &pl): current_upper(NULL), plugin(pl)
		{
		};

		/**
		 * @brief StackContext destructor
		 */
		virtual ~StackContext() {};

		/**
		 * Encapsulate some packets into one or several packets.
		 * The function returns a context ID and expiration time map.
		 * It's the caller charge to arm the timers to manage contextis expiration.
		 * It's also the caller charge to delete the returned NetBurst after use.
		 *
		 * @param burst        the packets to encapsulate
		 * @param time_contexts a map of time and context ID where:
		 *                       - context ID identifies the context in which the
		 *                         packet was encapsulated
		 *                       - time is the time before the context identified by
		 *                         the context ID expires
		 * @return              a list of packets
		 */
		virtual NetBurst *encapsulate(NetBurst *burst,
		                              map<long, int> &time_contexts) = 0;

		/**
		 * Encapsulate some packets into one or several packets for contexts with
		 * no timer.
		 *
		 * @param burst  the packets to encapsulate
		 * @return       a list of packets
		 */
		virtual NetBurst *encapsulate(NetBurst *burst)
		{
			map<long, int> time_contexts;
			return this->encapsulate(burst, time_contexts);
		}

		/**
		 * Deencapsulate some packets into one or several packets.
		 * It's the caller charge to delete the returned NetBurst after use.
		 *
		 * @param burst   the stack packets to deencapsulate
		 * @return        a list of packets
		 */
		virtual NetBurst *deencapsulate(NetBurst *burst) = 0;

		/** @brief Get the list of protocols that can be encapsulated
		 *
		 *  @param sat_type The satellite payload type (REGENERATIVE or
		 *                                              TRANSPARENT)
		 *  @param return The list of protocols that can be encapsulated
		 */
		vector<string> getAvailableUpperProto(sat_type_t sat_type) const
		{
			return plugin.upper[sat_type];
		};

		/**
		 * @brief Get the EtherType associated with the encapsulation protocol
		 *
		 * return The EtherType
		 */
		uint16_t getEtherType() const {return plugin.ether_type;};

		/**
		 * @brief Set the encapsulated packet handler
		 *
		 * @param pkt_hdl  The encapsulated packet handler
		 * @param sat_type The type of satellite payload
		 * @return true if this type of packet can be encapsulated, false otherwise
		 */
		virtual bool setUpperPacketHandler(StackPlugin::StackPacketHandler *pkt_hdl,
		                                   sat_type_t sat_type)
		{
			if(!pkt_hdl)
			{
				this->current_upper = NULL;
				return false;
			}

			vector<string>::iterator iter;

			iter = find((plugin.upper[sat_type]).begin(),
			            (plugin.upper[sat_type]).end(), pkt_hdl->getName());

			if(iter == (plugin.upper[sat_type]).end())
				return false;

			this->current_upper = pkt_hdl;
			return true;
		};

		/**
		 * @brief Update statistics periodically
		 *
		 * @param period  The time interval bewteen two updates
		 */
		virtual void updateStats(unsigned int UNUSED(period)) {};

		/**
		 * @brief Get the name of the plugin
		 *
		 * @return the name of the plugin
		 */
		string getName() const {return plugin.name;};

		/**
		 * @brief Get the configuration path
		 *
		 * @return the configuration path
		 */
		string getConfPath() const {return plugin.conf_path;};

		/**
		 * @brief Create a NetPacket from data with the relevant attributes
		 *
		 * @param data        The packet data
		 * @param data_length The packet length
		 * @param qos         The QoS value to associate with the packet
		 * @param src_tal_id  The source terminal ID to associate with the packet
		 * @param dst_tal_id  The destination terminal ID to associate with the packet
		 * @return the packet on success, NULL otherwise
		 */
		NetPacket *createPacket(const Data &data,
		                        size_t data_length,
		                        uint8_t qos,
		                        uint8_t src_tal_id,
		                        uint8_t dst_tal_id)
		{
			return plugin.packet_handler->build(data,
			                                    data_length,
			                                    qos,
			                                    src_tal_id,
			                                    dst_tal_id);
		}

		/**
		 * @brief perform some plugin initialization
		 *
		 * @return True if success, false otherwise
		 */
		virtual bool init() = 0;

	  protected:

		/// the current upper encapsulation protocol EtherType
		StackPlugin::StackPacketHandler *current_upper;

		/// Output Logs
		OutputLog *log;

	  private:

		/// The plugin
		StackPlugin &plugin;

	};

	/**
	 * @brief StackPlugin constructor
	 */
	StackPlugin(uint16_t ether_type): OpenSandPlugin()
	{
		this->ether_type = ether_type;
	};


	/**
	 * @brief StackPlugin destructor
	 */
	virtual ~StackPlugin()
	{
		if(this->context)
		{
			delete this->context;
		}

		if(this->packet_handler)
		{
			delete this->packet_handler;
		}
	};

	/**
	 * @brief Get the encapsulation context
	 *
	 * @return the context
	 */
	StackContext *getContext() const {return this->context;};

	/**
	 * @brief Get the encapsulation packet handler
	 *
	 * @return the packet handler
	 */
	StackPacketHandler *getPacketHandler() const {return this->packet_handler;};

	/**
	 * @brief Get The plugin name
	 *
	 * @return the plugin name
	 */
	string getName() const {return this->name;};

	/**
	 * @brief Create the Plugin, this function should be called instead of constructor
	 *
	 * @return The plugin
	 */
	template<class Plugin, class Context, class Handler>
	static OpenSandPlugin *create(const string name, const string conf_path)
	{
		Plugin *plugin = new Plugin();
		Context *context = new Context(*plugin);
		Handler *handler = new Handler(*plugin);
		plugin->context = context;
		plugin->packet_handler = handler;
		plugin->name = name;
		plugin->conf_path = conf_path;
		if(!plugin->init())
		{
			goto error;
		}
		if(!context->init())
		{
			goto error;
		}
		if(!handler->init())
		{
			goto error;
		}
		return plugin;
		
	error:
		delete handler;
		delete context;
		delete plugin;
		return NULL;
	};

	/**
	 * @brief perform some plugin initialization
	 *
	 * @return True if success, false otherwise
	 */
	virtual bool init() = 0;

 protected:

	/// The EtherType (or EtherType like) of the associated protocol
	uint16_t ether_type;

	/** The list of protocols that can be "encapsulated" according to satellite
	 *  payload type */
	map<sat_type_t, vector<string> > upper;

	/// The context
	StackContext *context;

	/// The packet handler
	StackPacketHandler *packet_handler;

	/// Output Logs
	OutputLog *log;
};

typedef vector<StackPlugin::StackContext *> stack_contexts_t;

/// Define the function that will create the plugin class
#define CREATE_STACK(CLASS, CONTEXT, HANDLER, pl_name, pl_type) \
	extern "C" OpenSandPlugin *create_ptr(const string conf_path) \
	{ \
		return CLASS::create<CLASS, CONTEXT, HANDLER>(pl_name, conf_path); \
	}; \
	extern "C" opensand_plugin_t *init() \
	{ \
		opensand_plugin_t *pl = new opensand_plugin_t; \
		pl->create = create_ptr; \
		pl->type = pl_type; \
		pl->name = pl_name; \
		return pl; \
	};


#endif
