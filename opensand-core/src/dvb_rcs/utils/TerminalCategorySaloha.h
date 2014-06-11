/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2014 TAS
 * Copyright © 2014 CNES
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
 * @file    TerminalCategorySaloha.h
 * @brief   Represent a category of terminal for Slotted Aloha
 * @author  Julien Bernard / Viveris Technologies
 */

#ifndef _TERMINAL_CATEGORY_SALOHA_H
#define _TERMINAL_CATEGORY_SALOHA_H

#include "TerminalCategory.h"
#include "CarriersGroupSaloha.h"

#include "SlottedAlohaPacketData.h"
#include "Slot.h"

/**
 * @class TerminalCategorySaloha
 * @brief Represent a category of terminal for Slotted Aloha
 */
class TerminalCategorySaloha: public TerminalCategory<CarriersGroupSaloha>
{

 public:

	/**
	 * @brief  Create a terminal category.
	 *
	 * @param  label  label of the category.
	 * @param  desired_access  the access type we support for our carriers
	 */
	TerminalCategorySaloha(string label, access_type_t desired_access);

	~TerminalCategorySaloha();

	/**
	 * @brief Set the slots number in carriers groups (for RCS)
	 *
	 * @frame_duration_ms    The frame duration (in ms)
	 * @packet_length_bytes  The length of the packets (in bytes)
	 */
	void setSlotsNumber(time_ms_t frame_duration_ms,
	                    vol_bytes_t packet_length_bytes);

	/**
	 * @brief Get the number of slots in the terminal category
	 *
	 * @return the number of slots in the terminal category
	 */
	unsigned int getSlotsNumber(void) const;

	/**
	 * @brief Get the slots in the category
	 *
	 * @return the slots from all carriers
	 */
	map<unsigned int, Slot *> getSlots(void) const;

	/**
	 * @brief Get the packets that can be transmitted to
	 *        encapsulation block
	 *
	 * @return the accepted packets
	 */
	saloha_packets_data_t *getAcceptedPackets(void);

	/**
	 * @brief increment the number of received packets
	 */
	void increaseReceivedPacketsNbr(void);

	/**
	 * @brief Get the number of received packets
	 */
	unsigned int getReceivedPacketsNbr(void) const;

	/**
	 * @brief Reset the number of received packets
	 */
	void resetReceivedPacketsNbr(void);

 private:
	/// A FIFO containing packet to be transmitted to encapsulation block
	saloha_packets_data_t *accepted_packets;

	/// The number of received packets
	unsigned int received_packets_nbr;
};

#endif
