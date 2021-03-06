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
 * @file Ipv4Packet.h
 * @brief IPv4 packet
 * @author Didier Barvaux <didier.barvaux@b2i-toulouse.com>
 */

#ifndef IPV4_PACKET_H
#define IPV4_PACKET_H

#include <Data.h>
#include <IpPacket.h>
#include <Ipv4Address.h>


/**
 * @class Ipv4Packet
 * @brief IPv4 Packet
 */
class Ipv4Packet: public IpPacket
{
 protected:

	/// Is the validity of the IPv4 packet already checked?
	mutable bool validity_checked;
	/// If IPv4 packet validity is checked, what is the result?
	mutable bool validity_result;

	/**
	 * Calculate the CRC of the IPv4 packet
	 * @return the CRC of the IPv4 packet
	 */
	uint16_t calcCrc() const;

 public:

	/**
	 * Build an IPv4 packet
	 * @param data raw data from which an IPv4 packet can be created
	 * @param length length of raw data
	 */
	Ipv4Packet(const unsigned char *data, size_t length);

	/**
	 * Build an IPv4 packet
	 * @param data raw data from which an IPv4 packet can be created
	 */
	Ipv4Packet(const Data &data);

	/**
	 * Build an IPv4 packet
	 * @param data raw data from which an IPv4 packet can be created
	 * @param length length of raw data
	 */
	Ipv4Packet(const Data &data, size_t length);

	/**
	 * Build an empty IPv4 packet
	 */
	Ipv4Packet();

	/**
	 * Destroy the IPv4 packet
	 */
	~Ipv4Packet();

	// implementation of virtual functions
	bool isValid() const;
	size_t getTotalLength() const;
	size_t getPayloadLength() const;
	IpAddress *dstAddr();
	IpAddress *srcAddr();
	uint8_t diffServField() const;
	uint8_t diffServCodePoint() const;
	uint8_t explicitCongestionNotification() const;

 protected:

	/**
	 * Retrieve the CRC field of the IPv4 header
	 * @return the CRC field of the IPv4 header
	 */
	uint16_t crc() const;

	/**
	 * Retrieve the header length of the IPv4 packet
	 * @return the header length of the IPv4 packet
	 */
	uint8_t ihl() const;
};

#endif
