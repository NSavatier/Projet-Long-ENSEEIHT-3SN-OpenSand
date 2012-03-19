/*
 *
 * Platine is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2011 CNES
 *
 *
 * This file is part of the Platine testbed.
 *
 *
 * Platine is free software : you can redistribute it and/or modify it under the
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
 * @file Gse.h
 * @brief GSE encapsulation plugin implementation
 * @author Julien BERNARD <jbernard@toulouse.viveris.com>
 */

#ifndef GSE_CONTEXT_H
#define GSE_CONTEXT_H

#include "GseEncapCtx.h"
#include "GseIdentifier.h"

#include <EncapPlugin.h>
#include <NetPacket.h>
#include <NetBurst.h>

#include <vector>
#include <map>

extern "C"
{
	#include <gse/constants.h>
	#include <gse/status.h>
	#include <gse/virtual_fragment.h>
	#include <gse/encap.h>
	#include <gse/deencap.h>
	#include <gse/refrag.h>
	#include <gse/header_fields.h>
}

/**
 * @class Gse
 * @brief GSE encapsulation plugin implementation
 */
class Gse: public EncapPlugin
{
  public:
	  
	/**
	 * @class Context
	 * @brief GSE encapsulation / desencapsulation context
	 */
	class Context: public EncapContext
	{
	  private:

		/// The GSE encapsulation context
		gse_encap_t *encap;
		 /// The GSE deencapsulation context
		gse_deencap_t *deencap;
		/// Vector of GSE virtual fragments
		std::vector<gse_vfrag_t *> vfrag_pkt_vec;
		/// Temporary buffers for encapsulation contexts. Contexts are identified
		/// by an unique identifier
		std::map <GseIdentifier *, GseEncapCtx *, ltGseIdentifier> contexts;
		/// The packing threshold for encapsulation. Packing Threshold is the time
		/// the context can wait for additional SNDU packets to fill the incomplete
		/// GSE packet before sending the GSE packet with padding.
		unsigned long packing_threshold;

	  public:

		/// constructor
		Context(EncapPlugin &plugin);

		/**
		 * Destroy the GSE encapsulation / deencapsulation context
		 */
		~Context();

		NetBurst *encapsulate(NetBurst *burst, std::map<long, int> &time_contexts);
		NetBurst *deencapsulate(NetBurst *burst);
		NetBurst *flush(int context_id);
		NetBurst *flushAll();

	  private:
		bool encapFixedLength(NetPacket *packet, NetBurst *gse_packets,
		                      long &time);
		bool encapVariableLength(NetPacket *packet, NetBurst *gse_packets);
		bool encapPacket(NetPacket *packet, gse_vfrag_t *vfrag_pkt,
		                 NetBurst *gse_packets);
		bool deencapPacket(gse_vfrag_t *vfrag_gse, uint16_t dest_spot,
		                   NetBurst *net_packets);
		bool deencapFixedLength(gse_vfrag_t *vfrag_pdu, uint16_t dest_spot,
		                        uint8_t label[6], NetBurst *net_packets);
		bool deencapVariableLength(gse_vfrag_t *vfrag_pdu, uint16_t dest_spot,
		                           uint8_t label[6], NetBurst *net_packets);

	};

	/**
	 * @class Packet
	 * @brief GSE packet
	 */
	class PacketHandler: public EncapPacketHandler
	{

	  public:

		PacketHandler(EncapPlugin &plugin);

		NetPacket *build(unsigned char *data, size_t data_length,
		                 uint8_t qos, uint8_t src_tal_id, uint8_t dst_tal_id);
		size_t getFixedLength() {return 0;};
		size_t getMinLength() {return 3;};
		size_t getLength(const unsigned char *data);
		bool getChunk(NetPacket *packet, size_t remaining_length,
		              NetPacket **data, NetPacket **remaining_data);
	};

	/// Constructor
	Gse();

	// Static methods: getter/setter for label/fragId
	
	/**
	 * @brief  Set the GSE packet label
	 *
	 * @param   packet  The packet to get label values from.
	 * @param   label   The label to set values of.
	 * @return  true on success, false otherwise.
	 */
	static bool setLabel(NetPacket *packet, uint8_t label[]);

	/**
	 * @brief  Set the GSE packet label
	 *
	 * @param   context  The GSE context to get label values from.
	 * @param   label   The label to set values of.
	 * @return  true on success, false otherwise.
	 */
	static bool setLabel(GseEncapCtx *context, uint8_t label[]);

	/**
	 * @brief   Get the source TAL Id from label.
	 * @param   label  The label to read value from.
	 * @return  the source TAL Id.
	 */
	static uint8_t getSrcTalIdFromLabel(uint8_t label[]);

	/**
	 * @brief   Get the destination TAL Id from label.
	 * @param   label  The label to read value from.
	 * @return  the destination TAL Id.
	 */
	static uint8_t getDstTalIdFromLabel(uint8_t label[]);

	/**
	 * @brief   Get the QoS value from label.
	 * @param   label  The label to read value from.
	 * @return  the QoS value.
	 */
	static uint8_t getQosFromLabel(uint8_t label[]);

	/**
	 * @brief   Create a fragment id from a packet.
	 * @param   packet  The packet to create the frag id from..
	 * @return  the frag id.
	 */
	static uint8_t getFragId(NetPacket *packet);

	/**
	 * @brief   Create a fragment id from a GSE context.
	 * @param   contextt  The context to create the frag id from..
	 * @return  the frag id.
	 */
	static uint8_t getFragId(GseEncapCtx *context);

	/**
	 * @brief   Get the source TAL Id from a fragment id..
	 * @param   frag_id  The fragment dd to read value from.
	 * @return  the source TAL Id.
	 */
	static uint8_t getSrcTalIdFromFragId(uint8_t frag_id);

	/**
	 * @brief   Get the destination TAL Id from a fragment id..
	 * @param   frag_id  The fragment dd to read value from.
	 * @return  the destination TAL Id.
	 */
	static uint8_t getDstTalIdFromFragId(uint8_t frag_id);

	/**
	 * @brief   Get the QoS value from a fragment id..
	 * @param   frag_id  The fragment dd to read value from.
	 * @return  the QoS value.
	 */
	static uint8_t getQosFromFragId(uint8_t frag_id);
};

CREATE(Gse, Gse::Context, Gse::PacketHandler);

#endif
