/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2013 TAS
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
 * @file MacFifoElement.cpp
 * @brief Fifo Element
 * @author Julien BERNARD <julien.bernard@toulouse.viveris.com>
 */


#include "MacFifoElement.h"

// TODO one constructor with NetContainer
MacFifoElement::MacFifoElement(DvbFrame *dvb_frame,
                               time_t tick_in, time_t tick_out):
	type(0),
	dvb_frame(dvb_frame),
	packet(NULL),
	tick_in(tick_in),
	tick_out(tick_out)
{
}

MacFifoElement::MacFifoElement(NetPacket *packet,
                               time_t tick_in, time_t tick_out):
	type(1),
	dvb_frame(NULL),
	packet(packet),
	tick_in(tick_in),
	tick_out(tick_out)
{
}


MacFifoElement::~MacFifoElement()
{
}

DvbFrame *MacFifoElement::getFrame() const
{
	return this->dvb_frame;
}

void MacFifoElement::setPacket(NetPacket *packet)
{
	this->packet = packet;
}

NetPacket *MacFifoElement::getPacket() const
{
	return this->packet;
}
// TODO getElem for NetContainer and remove GetFrame and getPacket

size_t MacFifoElement::getTotalLength() const
{
	switch(this->type)
	{
		case 0: return this->dvb_frame->getTotalLength();
		case 1: return this->packet->getTotalLength();
		default: return 0;
	}
}

int MacFifoElement::getType() const
{
	return this->type;
}

time_t MacFifoElement::getTickIn() const
{
	return this->tick_in;
}

time_t MacFifoElement::getTickOut() const
{
	return this->tick_out;
}
