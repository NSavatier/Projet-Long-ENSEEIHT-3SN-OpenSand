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
 * OpenSAND is free software : you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY, without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see http://www.gnu.org/licenses/.
 *
 */

/**
 * @file NetSocketEvent.h
 * @author Cyrille GAILLARDET / <cgaillardet@toulouse.viveris.com>
 * @author Julien BERNARD / <jbernard@toulouse.viveris.com>
 * @brief  The event for message read on network socket, can also be used
 *         by any fd-like oject such as file
 *
 */


#ifndef NET_SOCKET_EVENT_H
#define NET_SOCKET_EVENT_H

#include "Types.h"
#include "MessageEvent.h"

#include <sys/time.h>

#define MAX_SOCK_SIZE 1500


/**
  * @class NetSocketEvent
  * @brief Events describing data received on a nework socket
  *
  */
class NetSocketEvent: public Event
{

  public:

	/**
	 * @brief NetSocketEvent constructor
	 *
	 * @param name      The name of the event
	 * @param fd        The file descriptor to monitor for the event
	 * @param priority  The priority of the event
	 */
	NetSocketEvent(const string &name,
	               int32_t fd = -1,
	               uint8_t priority = 8);
	~NetSocketEvent();


	/**
	 * @brief Get the message content
	 *
	 * @return the data contained in the message
	 */
	unsigned char *getData() const {return this->data;};

	/*
	 * @brief Get the size of data in the message
	 *
	 * @return the size of data in the message
	 */
	size_t getSize() const {return this->size;};

	/**
	 * @brief Set the message content
	 *
	 * @param data  The message data
	 * @param size  The message size
	 */
	void setData(unsigned char *data, size_t size)
	{
		this->data = data;
		this->size = size;
	};

	/**
	 * @brief Set the message size
	 *
	 * @param size  The message size
	 */
	void setSize(size_t size)
	{
		this->size = size;
	};

  protected:

	/// data pointer
	unsigned char *data;

	/// data size
	size_t size;


};

#endif
