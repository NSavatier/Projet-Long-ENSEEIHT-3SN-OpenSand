/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2011 TAS
 * Copyright © 2011 CNES
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
 * @file bloc_encap_sat.cpp
 * @brief Generic Encapsulation Bloc for SE
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 * @author Julien Bernard <julien.bernard@toulouse.viveris.com>
 */

#include "bloc_encap_sat.h"

// debug
#undef DBG_PACKAGE
#define DBG_PACKAGE PKG_ENCAP
#include "opensand_conf/uti_debug.h"

// environment plane
#include "opensand_env_plane/EnvironmentAgent_e.h"
extern T_ENV_AGENT EnvAgent;

BlocEncapSat::BlocEncapSat(mgl_blocmgr * blocmgr,
                           mgl_id fatherid,
                           const char *name,
                           std::map<std::string, EncapPlugin *> encap_plug):
	mgl_bloc(blocmgr, fatherid, name),
	encap_plug(encap_plug)
{
	this->initOk = false;
	this->ip_handler = new IpPacketHandler(*((EncapPlugin *)NULL));
}

BlocEncapSat::~BlocEncapSat()
{
	delete this->ip_handler;
}

mgl_status BlocEncapSat::onEvent(mgl_event *event)
{
	const char *FUNCNAME = "[BlocEncapSat::onEvent]";
	mgl_status status = mgl_ko;

	if(MGL_EVENT_IS_INIT(event))
	{
		// initialization event
		if(this->initOk)
		{
			UTI_ERROR("%s bloc has already been initialized, "
			          "ignore init event\n", FUNCNAME);
		}
		else if(this->onInit() == mgl_ok)
		{
			this->initOk = true;
			status = mgl_ok;
		}
		else
		{
			UTI_ERROR("%s bloc initialization failed\n", FUNCNAME);
			ENV_AGENT_Error_Send(&EnvAgent, C_ERROR_CRITICAL, 0, 0,
			                     C_ERROR_INIT_COMPO);
		}
	}
	else if(!this->initOk)
	{
		UTI_ERROR("%s satellite encapsulation bloc not initialized, ignore "
		          "non-init event\n", FUNCNAME);
	}
	else if(MGL_EVENT_IS_TIMER(event))
	{
		// timer event, flush corresponding encapsulation context
		status = this->onTimer((mgl_timer) event->event.timer.id);
	}
	else if(MGL_EVENT_IS_MSG(event))
	{
		// message received from another bloc

		if(MGL_EVENT_MSG_GET_SRCBLOC(event) == this->getLowerLayer())
		{
			UTI_DEBUG("%s message received from the lower layer\n", FUNCNAME);

			if(MGL_EVENT_MSG_IS_TYPE(event, msg_encap_burst))
			{
				NetBurst *burst;
				burst = (NetBurst *) MGL_EVENT_MSG_GET_BODY(event);
				status = this->onRcvBurstFromDown(burst);
			}
			else
			{
				UTI_ERROR("%s message type is unknown\n", FUNCNAME);
			}
		}
		else
		{
			UTI_ERROR("%s message received from an unknown bloc\n", FUNCNAME);
		}
	}
	else
	{
		UTI_ERROR("%s unknown event (type %ld) received\n",
		          FUNCNAME, event->type);
	}

	return status;
}

mgl_status BlocEncapSat::setUpperLayer(mgl_id bloc_id)
{
	const char *FUNCNAME = "[BlocEncapSat::setUpperLayer]";

	UTI_ERROR("%s bloc does not accept an upper-layer bloc\n", FUNCNAME);

	return mgl_ko;
}

mgl_status BlocEncapSat::onInit()
{
	const char *FUNCNAME = "[BlocEncapSat::onInit]";
	int i;
	int encap_nbr;
	string upper_name;
	string upper_option;
	// The list of uplink encapsulation protocols to ignore them at downlink
	// encapsulation
	vector <string> up_proto;

	// get the number of encapsulation context to use for up link
	if(!globalConfig.getNbListItems(GLOBAL_SECTION, UP_RETURN_ENCAP_SCHEME_LIST,
	                                encap_nbr))
	{
		UTI_ERROR("%s Section %s, %s missing\n", FUNCNAME, GLOBAL_SECTION,
		          UP_RETURN_ENCAP_SCHEME_LIST);
		goto error;
	}

	for(i = 0; i < encap_nbr; i++)
	{
		string encap_name;

		// get all the encapsulation to use from lower to upper
		if(!globalConfig.getValueInList(GLOBAL_SECTION, UP_RETURN_ENCAP_SCHEME_LIST,
		                                POSITION, toString(i), ENCAP_NAME, encap_name))
		{
			UTI_ERROR("%s Section %s, invalid value %d for parameter '%s'\n",
			          FUNCNAME, GLOBAL_SECTION, i, POSITION);
			goto error;
		}

		up_proto.push_back(encap_name);
	}

	// get the number of encapsulation context to use for forward link
	if(!globalConfig.getNbListItems(GLOBAL_SECTION, DOWN_FORWARD_ENCAP_SCHEME_LIST,
	                                encap_nbr))
	{
		UTI_ERROR("%s Section %s, %s missing\n", FUNCNAME, GLOBAL_SECTION,
		          UP_RETURN_ENCAP_SCHEME_LIST);
		goto error;
	}

	upper_name = upper_option;
	for(i = 0; i < encap_nbr; i++)
	{
		bool next = false;
		string encap_name;
		vector<string>::iterator iter;
		EncapPlugin::EncapContext *context;


		// get all the encapsulation to use from lower to upper
		if(!globalConfig.getValueInList(GLOBAL_SECTION, DOWN_FORWARD_ENCAP_SCHEME_LIST,
		                                POSITION, toString(i), ENCAP_NAME, encap_name))
		{
			UTI_ERROR("%s Section %s, invalid value %d for parameter '%s'\n",
			          FUNCNAME, GLOBAL_SECTION, i, POSITION);
			goto error;
		}

		if(this->encap_plug[encap_name] == NULL)
		{
			UTI_ERROR("%s missing plugin for %s encapsulation",
			          FUNCNAME, encap_name.c_str());
			goto error;
		}

		context = this->encap_plug[encap_name]->getContext();
		for(iter = up_proto.begin(); iter!= up_proto.end(); iter++)
		{
			if(*iter == encap_name)
			{
				upper_name = context->getName();
				// no need to encapsulate with this protocol because it will
				// already be done on uplink
				next = true;
				break;
			}
		}
		if(next)
		{
			continue;
		}

		this->downlink_ctx.push_back(context);
		if(upper_name == "")
		{
			if(!context->setUpperPacketHandler(this->ip_handler,
			                                   REGENERATIVE))
			{
				goto error;
			}
		}
		else if(!context->setUpperPacketHandler(
					this->encap_plug[upper_name]->getPacketHandler(),
					REGENERATIVE))
		{
			UTI_ERROR("%s upper encapsulation type %s not supported for %s "
			          "encapsulation", FUNCNAME, upper_name.c_str(),
			          context->getName().c_str());
			goto error;
		}
		upper_name = context->getName();
		UTI_DEBUG("%s add downlink encapsulation layer: %s\n",
		          FUNCNAME, upper_name.c_str());
	}

	return mgl_ok;

error:
	return mgl_ko;
}

mgl_status BlocEncapSat::onTimer(mgl_timer timer)
{
	const char *FUNCNAME = "[BlocEncapSat::onTimer]";
	std::map < mgl_timer, int >::iterator it;
	int id;
	NetBurst *burst;
	mgl_msg *msg; // margouilla message

	UTI_DEBUG("%s emission timer received, flush corresponding emission "
	          "context\n", FUNCNAME);

	// find encapsulation context to flush
	it = this->timers.find(timer);
	if(it == this->timers.end())
	{
		UTI_ERROR("%s timer not found\n", FUNCNAME);
		goto error;
	}

	// context found
	id = (*it).second;
	UTI_DEBUG("%s corresponding emission context found (ID = %d)\n",
	          FUNCNAME, id);

	// remove emission timer from the list
	this->timers.erase(it);

	// flush the last encapsulation context
	burst = this->downlink_ctx.back()->flush(id);
	if(burst == NULL)
	{
		UTI_DEBUG("%s flushing context %d failed\n", FUNCNAME, id);
		goto error;
	}

	UTI_DEBUG("%s %d encapsulation packet(s) flushed\n",
	          FUNCNAME, burst->size());

	if(burst->size() <= 0)
		goto clean;

	// create the Margouilla message
	// with encapsulation burst as data
	msg = this->newMsgWithBodyPtr(msg_encap_burst, burst, sizeof(burst));
	if(!msg)
	{
		UTI_ERROR("%s newMsgWithBodyPtr() failed\n", FUNCNAME);
		goto clean;
	}

	// send the message to the lower layer
	if(this->sendMsgTo(this->getLowerLayer(), msg) == mgl_ko)
	{
		UTI_ERROR("%s sendMsgTo() failed\n", FUNCNAME);
		goto clean;
	}

	UTI_DEBUG("%s encapsulation burst sent to the lower layer\n", FUNCNAME);

	return mgl_ok;

clean:
	delete burst;
error:
	return mgl_ko;
}

mgl_status BlocEncapSat::onRcvBurstFromDown(NetBurst *burst)
{
	const char *FUNCNAME = "[BlocEncapSat::onRcvBurstFromDown]";
	mgl_status status;

	// check burst validity
	if(burst == NULL)
	{
		UTI_ERROR("%s burst is not valid\n", FUNCNAME);
		goto error;
	}

	UTI_DEBUG("%s message contains a burst of %d %s packet(s)\n",
	          FUNCNAME, burst->length(), burst->name().c_str());

	if(this->downlink_ctx.size() > 0)
	{
		status = this->EncapsulatePackets(burst);
	}
	else
	{
		status = this->ForwardPackets(burst);
	}

	return status;

error:
	return mgl_ko;
}

mgl_status BlocEncapSat::ForwardPackets(NetBurst *burst)
{
	const char *FUNCNAME = "[BlocEncapSat::ForwardPackets]";
	mgl_msg *msg; // margouilla message

	// check burst validity
	if(burst == NULL)
	{
		UTI_ERROR("%s burst is not valid\n", FUNCNAME);
		goto error;
	}

	// create the Margouilla message with burst as data
	msg = this->newMsgWithBodyPtr(msg_encap_burst, burst, sizeof(burst));
	if(!msg)
	{
		UTI_ERROR("%s newMsgWithBodyPtr() failed\n", FUNCNAME);
		goto clean;
	}

	// send the message to the lower layer
	if(this->sendMsgTo(this->getLowerLayer(), msg) == mgl_ko)
	{
		UTI_ERROR("%s sendMsgTo() failed\n", FUNCNAME);
		goto clean;
	}

	UTI_DEBUG("%s burst sent to the lower layer\n", FUNCNAME);

	// everthing is fine
	return mgl_ok;

clean:
	delete burst;
error:
	return mgl_ko;
}

mgl_status BlocEncapSat::EncapsulatePackets(NetBurst *burst)
{
	const char *FUNCNAME = "[BlocEncapSat::EncapsulatePackets]";
	NetBurst::iterator pkt_it;
	NetBurst *packets;
	map<long, int> time_contexts;
	mgl_msg *msg; // margouilla message
	vector<EncapPlugin::EncapContext *>::iterator iter;

	// check burst validity
	if(burst == NULL)
	{
		UTI_ERROR("%s burst is not valid\n", FUNCNAME);
		goto error;
	}

	packets = burst;
	for(iter = this->downlink_ctx.begin(); iter != this->downlink_ctx.end();
	    iter++)
	{
		packets = (*iter)->encapsulate(packets, time_contexts);
		if(packets == NULL)
		{
			UTI_ERROR("%s encapsulation failed in %s context\n",
			          FUNCNAME, (*iter)->getName().c_str());
			goto clean;
		}
	}

	// set encapsulate timers if needed
	for(map<long, int>::iterator time_iter = time_contexts.begin();
	    time_iter != time_contexts.end(); time_iter++)
	{
		std::map < mgl_timer, int >::iterator it;
		bool found = false;

		// check if there is already a timer armed for the context
		for(it = this->timers.begin(); !found && it != this->timers.end(); it++)
			found = ((*it).second == (*time_iter).second);

		// set a new timer if no timer was found
		if(!found && (*time_iter).first != 0)
		{
			mgl_timer timer;
			this->setTimer(timer, (*time_iter).first);
			this->timers.insert(std::make_pair(timer, (*time_iter).second));
			UTI_DEBUG("%s timer for context ID %d armed with %ld ms\n",
			          FUNCNAME, (*time_iter).second, (*time_iter).first);
		}
		else
		{
			UTI_DEBUG("%s timer already set for context ID %d\n",
			          FUNCNAME, (*time_iter).second);
		}
	}

	// create and send message only if at least one packet was created
	if(packets->size() <= 0)
	{
		goto clean;
	}

	// create the Margouilla message with burst as data
	msg = this->newMsgWithBodyPtr(msg_encap_burst, packets,
	                              sizeof(packets));
	if(!msg)
	{
		UTI_ERROR("%s newMsgWithBodyPtr() failed\n", FUNCNAME);
		goto clean;
	}

	//  send the message to the lower layer
	if(this->sendMsgTo(this->getLowerLayer(), msg) == mgl_ko)
	{   
		UTI_ERROR("%s sendMsgTo() failed\n", FUNCNAME);
		goto clean;
	}

	UTI_DEBUG("%s %s burst sent to the lower layer\n", FUNCNAME,
	          (this->downlink_ctx.back())->getName().c_str());

	// everthing is fine
	return mgl_ok;

clean:
	delete packets;
error:
	return mgl_ko;
}