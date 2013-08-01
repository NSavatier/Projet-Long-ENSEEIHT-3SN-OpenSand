/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2013 TAS
 * Copyright © 2013 CNES
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
 * @file NetPacket.cpp
 * @brief Network-layer packet
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 */

#include "NetPacket.h"

// debug
#define DBG_PACKAGE PKG_DEFAULT
#include "opensand_conf/uti_debug.h"


NetPacket::NetPacket(Data data):
	data(data),
	name("unknown"),
	type(NET_PROTO_ERROR),
	header_length(0)
{
}

NetPacket::NetPacket(unsigned char *data, size_t length):
	data(),
	name("unknown"),
	type(NET_PROTO_ERROR),
	header_length(0)
{
	this->data.append(data, length);
}

NetPacket::NetPacket():
	data(),
	name("unknown"),
	type(NET_PROTO_ERROR),
	header_length(0)
{
}

NetPacket::NetPacket(unsigned char *data,
                     size_t length,
                     std::string name,
                     uint16_t type,
                     uint8_t qos,
                     uint8_t src_tal_id,
                     uint8_t dst_tal_id,
                     size_t header_length):
	data(),
	name(name),
	type(type),
	qos(qos),
	src_tal_id(src_tal_id),
	dst_tal_id(dst_tal_id),
	header_length(header_length)
{
	this->data.append(data, length);
}


NetPacket::~NetPacket()
{
}


std::string NetPacket::getName()
{
	return this->name;
}

uint16_t NetPacket::getType()
{
	return this->type;
}

Data NetPacket::getData()
{
	return this->data;
}

uint8_t NetPacket::getQos()
{
	return this->qos;
}

uint8_t NetPacket::getSrcTalId()
{
	return this->src_tal_id;
}

uint8_t NetPacket::getDstTalId()
{
	return this->dst_tal_id;
}

Data NetPacket::getPayload()
{
	return Data(this->data, this->header_length,
	            this->getPayloadLength());
}

uint16_t NetPacket::getPayloadLength()
{
	return (this->getTotalLength() - this->header_length);
}

uint16_t NetPacket::getTotalLength()
{
	return this->data.length();
}

void NetPacket::setDstSpot(uint8_t spot_id)
{
	this->dst_spot = spot_id;
}

uint8_t NetPacket::getDstSpot()
{
	return this->dst_spot;
}

size_t NetPacket::getHeaderLength()
{
	return this->header_length;
}
