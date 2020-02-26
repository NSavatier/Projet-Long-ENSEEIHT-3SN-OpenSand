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
 * @file BlockDvbTal.cpp
 * @brief This bloc implements a DVB-S/RCS stack for a Terminal, compatible
 *        with Legacy and RrmQosDama agent
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 * @author Julien Bernard <julien.bernard@toulouse.viveris.com>
 * @author Aurelien DELRIEU <adelrieu@toulouse.viveris.com>
 * @author Joaquin MUGUERZA <jmuguerza@toulouse.viveris.com>
 */


#include <opensand_output/Output.h>

#include "BlockDvbTal.h"

#include "DamaAgentRcsLegacy.h"
#include "DamaAgentRcsRrmQos.h"
#include "DamaAgentRcs2Legacy.h"
#include "TerminalCategoryDama.h"
#include "ScpcScheduling.h"
#include "SlottedAlohaPacketData.h"

#include "DvbRcsStd.h"
#include "DvbS2Std.h"
#include "Ttp.h"
#include "Sof.h"

#include "UnitConverterFixedBitLength.h"
#include "UnitConverterFixedSymbolLength.h"

#include <opensand_rt/Rt.h>


#include "Plugin.h"
#include <opensand_conf/Configuration.h>
#include "ConfUpdateXMLParser.h"

#include <sstream>
#include <assert.h>
#include <unistd.h>

int BlockDvbTal::Downward::qos_server_sock = -1;


/*****************************************************************************/
/*                                Block                                      */
/*****************************************************************************/


BlockDvbTal::BlockDvbTal(const string &name, tal_id_t UNUSED(mac_id)):
	BlockDvb(name),
	input_sts(NULL),
	output_sts(NULL)
{
}

BlockDvbTal::~BlockDvbTal()
{
	map<tal_id_t, StFmtSimu *>::iterator it;

	if(this->input_sts != NULL)
	{
		delete this->input_sts;
	}

	if(this->output_sts != NULL)
	{
		delete this->output_sts;
	}
}

bool BlockDvbTal::onInit(void)
{
	if(!this->initListsSts())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Failed to initialize the lists of Sts\n");
		return false;
	}

	return true;
}

bool BlockDvbTal::initListsSts()
{
	bool is_scpc = false;

	this->input_sts = new StFmtSimuList("in");
	if(this->input_sts == NULL)
	{
		return false;
	}

	// no output except for SCPC because it is directly handled
	// in Dama Agent (this->modcod_id)

	((Upward *)this->upward)->setInputSts(this->input_sts);
	((Downward *)this->downward)->setInputSts(this->input_sts);

	if(!Conf::getValue(Conf::section_map[DVB_TAL_SECTION], IS_SCPC, is_scpc))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "section '%s': missing parameter '%s'\n",
		    DVB_TAL_SECTION, IS_SCPC);
		return false;
	}
	if(is_scpc)
	{
		this->output_sts = new StFmtSimuList("out");
		if(this->output_sts == NULL)
		{
			return false;
		}

		((Upward *)this->upward)->setOutputSts(this->output_sts);
		((Downward *)this->downward)->setOutputSts(this->output_sts);
	}

	return true;
}



/*****************************************************************************/
/*                              Downward                                     */
/*****************************************************************************/

BlockDvbTal::Downward::Downward(const string &name, tal_id_t mac_id):
	DvbDownward(name),
	mac_id(mac_id),
	state(state_initializing),
	group_id(),
	tal_id(),
	gw_id(),
	spot_id(),
	is_scpc(false),
    conf_update_interface(),
	cra_kbps(0),
	max_rbdc_kbps(0),
	max_vbdc_kb(0),
	dama_agent(NULL),
	saloha(NULL),
	scpc_carr_duration_ms(0),
	scpc_timer(-1),
	ret_fmt_groups(),
	scpc_sched(NULL),
	scpc_frame_counter(0),
	carrier_id_ctrl(),
	carrier_id_logon(),
	carrier_id_data(),
	dvb_fifos(),
	default_fifo_id(0),
	sync_period_frame(-1),
	obr_slot_frame(-1),
	complete_dvb_frames(),
	logon_timer(-1),
	qos_server_host(),
	event_login(NULL),
	log_frame_tick(NULL),
	log_qos_server(NULL),
	log_saloha(NULL),
	probe_st_queue_size(),
	probe_st_queue_size_kb(),
	probe_st_l2_to_sat_before_sched(),
	probe_st_l2_to_sat_after_sched(),
	l2_to_sat_total_bytes(0),
	probe_st_l2_to_sat_total()
{
}

BlockDvbTal::Downward::~Downward()
{
	if(this->dama_agent != NULL)
	{
		delete this->dama_agent;
	}

	if(this->saloha)
	{
		delete this->saloha;
	}

	if(this->scpc_sched)
	{
		delete this->scpc_sched;
	}

	// delete FMT groups here because they may be present in many carriers
	// TODO do something to avoid groups here
	for(fmt_groups_t::iterator it = this->ret_fmt_groups.begin();
	    it != this->ret_fmt_groups.end(); ++it)
	{
		delete (*it).second;
	}

	// delete fifos
	for(fifos_t::iterator it = this->dvb_fifos.begin();
	    it != this->dvb_fifos.end(); ++it)
	{
		delete (*it).second;
	}
	this->dvb_fifos.clear();

	// close QoS Server socket if it was opened
	if(BlockDvbTal::Downward::qos_server_sock != -1)
	{
		close(BlockDvbTal::Downward::qos_server_sock);
	}

	this->complete_dvb_frames.clear();
}


bool BlockDvbTal::Downward::onInit(void)
{
	this->log_qos_server = Output::registerLog(LEVEL_WARNING,
	                                           "Dvb.QoSServer");
	this->log_frame_tick = Output::registerLog(LEVEL_WARNING,
	                                           "Dvb.DamaAgent.FrameTick");
	if(!this->initModcodDefinitionTypes())
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed to initialize MOCODS definitions types\n");
		return false;
	}

	// get the common parameters
	if(!this->initCommon(RETURN_UP_ENCAP_SCHEME_LIST))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to complete the common part of the initialisation\n");
		return false;
	}
	if(!this->initDown())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to complete the downward common initialisation\n");
		return false;
	}

	if(!this->initCarrierId())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to complete the carrier IDs part of the initialisation\n");
		return false;
	}

	if(!this->initMacFifo())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to complete the MAC FIFO part of the initialisation\n");
		return false;
	}

	// Initialization od fow_modcod_def (useful to send SAC)
	if(!this->initModcodDefFile(MODCOD_DEF_S2,
	                            &this->s2_modcod_def))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to initialize the up/return MODCOD definition file\n");
		return false;
	}

	if(!Conf::getValue(Conf::section_map[DVB_TAL_SECTION], IS_SCPC, this->is_scpc))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "section '%s': missing parameter '%s'\n",
		    DVB_TAL_SECTION, IS_SCPC);
		return false;
	}

	if(!this->is_scpc)
	{
		if(!this->initDama())
		{
			LOG(this->log_init, LEVEL_ERROR,
					"failed to complete the DAMA part of the initialisation\n");
			return false;
		}

		if(!this->initSlottedAloha())
		{
			LOG(this->log_init, LEVEL_ERROR,
					"failed to complete the initialisation of Slotted Aloha\n");
			return false;
		}
	}
	else
	{
		if(!this->initScpc())
		{
			LOG(this->log_init, LEVEL_ERROR,
					"failed to complete the SCPC part of the initialisation\n");
			return false;
		}
	}

	if(!this->dama_agent && !this->saloha && !this->scpc_sched)
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "unable to instanciate DAMA or Slotted Aloha or SCPC, "
		    "check your configuration\n");
		return false;
	}

	if(!this->initQoSServer())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to complete the QoS Server part of the initialisation\n");
		return false;
	}

	if (this->dama_agent || this->saloha)
	{ 
		this->initStatsTimer(this->ret_up_frame_duration_ms);
	}
	else //Scpc mode
	{
		this->initStatsTimer(this->scpc_carr_duration_ms);
	}

	// Init the output here since we now know the FIFOs
	if(!this->initOutput())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to complete the initialisation of output\n");
		return false;
	}

	if(!this->initTimers())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to complete the initialization of timers\n");
		return false;
	}

    // listen for connections from external ConfUpdate components
    if(!this->conf_update_interface.initConfUpdateSocket())
    {
        LOG(this->log_init, LEVEL_ERROR,
            "failed to listen for ConfUpdate connections\n");
        return false;
    } else {
        LOG(this->log_init, LEVEL_WARNING,
                "Now Listening for ConfUpdate connections\n");
    }
    this->addTcpListenEvent("conf_update_listen",
                            this->conf_update_interface.getConfUpdateListenSocket(), 200);

	// now everyhing is initialized so we can do some processing

	// after all of things have been initialized successfully,
	// send a logon request
	LOG(this->log_init, LEVEL_DEBUG,
	    "send a logon request with MAC ID %d to NCC\n",
	    this->mac_id);
	this->state = state_wait_logon_resp;
	if(!this->sendLogonReq())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to send the logon request to the NCC\n");
		return false;
	}

	return true;
}


bool BlockDvbTal::Downward::initCarrierId(void)
{
	// get current spot id withing sat switching table
	ConfigurationList::iterator spot_iter;
	// get satelite carrier spot configuration
	ConfigurationList current_gw;
	ConfigurationList carrier_list ;
	ConfigurationList::iterator iter;
	this->gw_id = 0;

	if(OpenSandConf::spot_table.find(this->mac_id) != OpenSandConf::spot_table.end())
	{
		this->spot_id = OpenSandConf::spot_table[this->mac_id];
	}
	else if(!Conf::getValue(Conf::section_map[SPOT_TABLE_SECTION],
		                    DEFAULT_SPOT, this->spot_id))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "couldn't find spot for tal %d",
		    this->mac_id);
		return false;
	}

	if(OpenSandConf::gw_table.find(this->mac_id) != OpenSandConf::gw_table.end())
	{
		this->gw_id = OpenSandConf::gw_table[this->mac_id];
	}
	else if(!Conf::getValue(Conf::section_map[GW_TABLE_SECTION],
		                    DEFAULT_GW, this->gw_id))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "couldn't find gw for tal %d",
		    this->mac_id);
		return false;
	}

	if(!OpenSandConf::getSpot(SATCAR_SECTION,
		                      this->spot_id,
		                      this->gw_id, current_gw))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "section '%s', missing spot for id %d and gw id %d\n",
		    SATCAR_SECTION, this->spot_id, this->gw_id);
		return false;
	}

	// get satellite channels from configuration
	if(!Conf::getListItems(current_gw, CARRIER_LIST, carrier_list))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "section '%s, %s': missing satellite channels\n",
		    SATCAR_SECTION, CARRIER_LIST);
		goto error;
	}

	// check id du spot correspond au id du spot dans lequel est le bloc actuel!
	for(iter = carrier_list.begin(); iter != carrier_list.end(); ++iter)
	{

		unsigned int carrier_id;
		string carrier_type;

		// Get the carrier id
		if(!Conf::getAttributeValue(iter, CARRIER_ID, carrier_id))
		{
			LOG(this->log_init_channel, LEVEL_ERROR,
			    "section '%s/%s%d/%s': missing parameter '%s'\n",
			    SATCAR_SECTION, SPOT_LIST, this->spot_id,
			    CARRIER_LIST, CARRIER_ID);
			goto error;
		}

		// Get the carrier type
		if(!Conf::getAttributeValue(iter, CARRIER_TYPE, carrier_type))
		{
			LOG(this->log_init_channel, LEVEL_ERROR,
			    "section '%s/%s%d/%s': missing parameter '%s'\n",
			    SATCAR_SECTION, SPOT_LIST, this->spot_id,
			    CARRIER_LIST, CARRIER_TYPE);
			goto error;
		}

		// Get the ID for control carrier
		if(strcmp(carrier_type.c_str(), CTRL_IN) == 0)
		{
			this->carrier_id_ctrl = carrier_id;
		}
		// Get the ID for data carrier
		else if(strcmp(carrier_type.c_str(), DATA_IN_ST) == 0)
		{
			this->carrier_id_data = carrier_id;
		}
		// Get the ID for logon carrier
		else if(strcmp(carrier_type.c_str(), LOGON_IN) == 0)
		{
			this->carrier_id_logon = carrier_id;
		}
	}

	// Check carrier error

	// Control carrier error
	if(this->carrier_id_ctrl == 0)
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "SF#%u %s missing from section %s/%s%d\n",
		    this->super_frame_counter,
		    DVB_CAR_ID_CTRL, SATCAR_SECTION,
		    SPOT_LIST, this->spot_id);
		goto error;
	}

	// Logon carrier error
	if(this->carrier_id_logon == 0)
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "SF#%u %s missing from section %s/%s%d\n",
		    this->super_frame_counter,
		    DVB_CAR_ID_LOGON, SATCAR_SECTION,
		    SPOT_LIST, this->spot_id);
		goto error;
	}

	// Data carrier error
	if(this->carrier_id_data == 0)
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "SF#%u %s missing from section %s/%s%d\n",
		    this->super_frame_counter,
		    DVB_CAR_ID_DATA, SATCAR_SECTION,
		    SPOT_LIST, this->spot_id);
		goto error;
	}

	LOG(this->log_init, LEVEL_NOTICE,
	    "SF#%u: carrier IDs for Ctrl = %u, Logon = %u, "
	    "Data = %u\n", this->super_frame_counter,
	    this->carrier_id_ctrl,
	    this->carrier_id_logon, this->carrier_id_data);

	return true;
error:
	return false;
}

bool BlockDvbTal::Downward::initMacFifo(void)
{
	ConfigurationList fifo_list;
	ConfigurationList::iterator iter;

	/*
	* Read the MAC queues configuration in the configuration file.
	* Create and initialize MAC FIFOs
	*/
	if(!Conf::getListItems(Conf::section_map[DVB_TAL_SECTION],
		                   FIFO_LIST, fifo_list))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "section '%s, %s': missing fifo list\n", DVB_TAL_SECTION,
		    FIFO_LIST);
		goto err_fifo_release;
	}

	for(iter = fifo_list.begin(); iter != fifo_list.end(); ++iter)
	{
		qos_t fifo_priority = 0;
		vol_pkt_t fifo_size = 0;
		string fifo_name;
		string fifo_access_type;
		DvbFifo *fifo;

		// get fifo_id --> fifo_priority
		if(!Conf::getAttributeValue(iter, FIFO_PRIO, fifo_priority))
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "cannot get %s from section '%s, %s'\n",
			    FIFO_PRIO, DVB_TAL_SECTION, FIFO_LIST);
			goto err_fifo_release;
		}
		// get fifo_name
		if(!Conf::getAttributeValue(iter, FIFO_NAME, fifo_name))
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "cannot get %s from section '%s, %s'\n",
			    FIFO_NAME, DVB_TAL_SECTION, FIFO_LIST);
			goto err_fifo_release;
		}
		// get fifo_size
		if(!Conf::getAttributeValue(iter, FIFO_SIZE, fifo_size))
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "cannot get %s from section '%s, %s'\n",
			    FIFO_SIZE, DVB_TAL_SECTION, FIFO_LIST);
			goto err_fifo_release;
		}
		// get the fifo CR type
		if(!Conf::getAttributeValue(iter, FIFO_ACCESS_TYPE, fifo_access_type))
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "cannot get %s from section '%s, %s'\n",
			    FIFO_ACCESS_TYPE, DVB_TAL_SECTION,
			    FIFO_LIST);
			goto err_fifo_release;
		}

		fifo = new DvbFifo(fifo_priority, fifo_name,
		                   fifo_access_type, fifo_size);

		LOG(this->log_init, LEVEL_NOTICE,
		    "Fifo priority = %u, FIFO name %s, size %u, "
		    "CR type %d\n",
		    fifo->getPriority(),
		    fifo->getName().c_str(),
		    fifo->getMaxSize(),
		    fifo->getAccessType());

		// the default FIFO is the last one = the one with the smallest priority
		// actually, the IP plugin should add packets in the default FIFO if
		// the DSCP field is not recognize, default_fifo_id should not be used
		// this is only used if traffic categories configuration and fifo configuration
		// are not coherent.
		this->default_fifo_id = std::max(this->default_fifo_id, fifo->getPriority());

		this->dvb_fifos.insert(pair<unsigned int, DvbFifo *>(fifo->getPriority(),
		                       fifo));
	} // end for(queues are now instanciated and initialized)


	this->l2_to_sat_total_bytes = 0;

	return true;

err_fifo_release:
	for(fifos_t::iterator it = this->dvb_fifos.begin();
	    it != this->dvb_fifos.end(); ++it)
	{
		delete (*it).second;
	}
	this->dvb_fifos.clear();
	return false;
}


bool BlockDvbTal::Downward::initDama(void)
{
	time_ms_t sync_period_ms = 0;
	time_sf_t rbdc_timeout_sf = 0;
	time_sf_t msl_sf = 0;
	string dama_algo;
	bool is_dama_fifo = false;

	TerminalCategories<TerminalCategoryDama> dama_categories;
	TerminalMapping<TerminalCategoryDama> terminal_affectation;
	TerminalCategoryDama *default_category;
	TerminalCategoryDama *tal_category = NULL;
	TerminalMapping<TerminalCategoryDama>::const_iterator tal_map_it;
	TerminalCategories<TerminalCategoryDama>::iterator cat_it;

	ConfigurationList current_spot;

	for(fifos_t::iterator it = this->dvb_fifos.begin();
	    it != this->dvb_fifos.end(); ++it)
	{
		if((*it).second->getAccessType() == access_dama_rbdc ||
		   (*it).second->getAccessType() == access_dama_vbdc ||
		   (*it).second->getAccessType() == access_dama_cra)
		{
			is_dama_fifo = true;
		}
	}

	// init
	if(!this->initModcodDefFile(this->modcod_def_rcs_type.c_str(),
	                            &this->rcs_modcod_def,
	                            this->req_burst_length))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to initialize the up/return MODCOD definition file\n");
		return false;
	}

	// get current spot into return up band section
	if(!OpenSandConf::getSpot(RETURN_UP_BAND,
		                      this->spot_id,
		                      NO_GW, current_spot))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "section '%s', missing spot for id %d\n",
		    RETURN_UP_BAND, this->spot_id);
		return false;
	}

	// init band
	if(!this->initBand<TerminalCategoryDama>(current_spot,
	                                         RETURN_UP_BAND,
	                                         DAMA,
	                                         this->ret_up_frame_duration_ms,
	                                         this->satellite_type,
	                                         this->rcs_modcod_def,
	                                         dama_categories,
	                                         terminal_affectation,
	                                         &default_category,
	                                         this->ret_fmt_groups))
	{
		return false;
	}

	if(dama_categories.size() == 0)
	{
		LOG(this->log_init, LEVEL_INFO,
		    "No DAMA carriers\n");
		return true;
	}

	// Find the category for this terminal
	tal_map_it = terminal_affectation.find(this->mac_id);
	if(tal_map_it == terminal_affectation.end())
	{
		// check if the default category is concerned by DAMA
		if(!default_category)
		{
			LOG(this->log_init, LEVEL_INFO,
			    "ST not affected to a DAMA category\n");
			goto release_cat;
		}
		tal_category = default_category;
	}
	else
	{
		tal_category = (*tal_map_it).second;
	}

	// check if there is DAMA carriers
	if(!tal_category)
	{
		LOG(this->log_init, LEVEL_INFO,
		    "No DAMA carrier\n");
		if(is_dama_fifo)
		{
			LOG(this->log_init, LEVEL_WARNING,
			    "Remove DAMA FIFOs because there is no "
			    "DAMA carrier\n");
			for(fifos_t::iterator it = this->dvb_fifos.begin();
			    it != this->dvb_fifos.end(); ++it)
			{
				if((*it).second->getAccessType() == access_dama_rbdc ||
				   (*it).second->getAccessType() == access_dama_vbdc ||
				   (*it).second->getAccessType() == access_dama_cra)
				{
					delete (*it).second;
					this->dvb_fifos.erase(it);
				}
			}
		}
		goto release_cat;
	}

	if(!is_dama_fifo)
	{
		LOG(this->log_init, LEVEL_WARNING,
		    "The DAMA carrier won't be used as there is no DAMA FIFO\n");
		goto release_cat;
	}

	//  allocated bandwidth in CRA mode traffic -- in kbits/s
	if(!Conf::getValue(Conf::section_map[DVB_TAL_SECTION],
		               CRA, this->cra_kbps))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Missing %s\n", CRA);
		goto error;
	}

	LOG(this->log_init, LEVEL_NOTICE,
	    "cra_kbps = %d kbits/s\n", this->cra_kbps);

	// Max RBDC (in kbits/s) and RBDC timeout (in frame number)
	if(!Conf::getValue(Conf::section_map[DA_TAL_SECTION],
		               DA_MAX_RBDC_DATA,
	                   this->max_rbdc_kbps))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Missing %s\n",
		    DA_MAX_RBDC_DATA);
		goto error;
	}

	// Max VBDC
	if(!Conf::getValue(Conf::section_map[DA_TAL_SECTION],
	                   DA_MAX_VBDC_DATA,
	                   this->max_vbdc_kb))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Missing %s\n", DA_MAX_VBDC_DATA);
		goto error;
	}

	// MSL duration -- in frames number
	if(!Conf::getValue(Conf::section_map[DA_TAL_SECTION],
		               DA_MSL_DURATION, msl_sf))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Missing %s\n", DA_MSL_DURATION);
		goto error;
	}

	// get the OBR period
	if(!Conf::getValue(Conf::section_map[COMMON_SECTION],
	                   SYNC_PERIOD, sync_period_ms))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Missing %s", SYNC_PERIOD);
		goto error;
	}
	this->sync_period_frame = (time_frame_t)round((double)sync_period_ms /
	                                              (double)this->ret_up_frame_duration_ms);

	// deduce the Obr slot position within the multi-frame, from the mac
	// address and the OBR period
	// ObrSlotFrame= MacAddress 'modulo' Obr Period
	// NB : ObrSlotFrame is within [0, Obr Period -1]
	this->obr_slot_frame = this->mac_id % this->sync_period_frame;
	LOG(this->log_init, LEVEL_NOTICE,
	    "SF#%u: MAC adress = %d, SYNC period = %d, "
	    "OBR slot frame = %d\n", this->super_frame_counter,
	    this->mac_id, this->sync_period_frame, this->obr_slot_frame);

	rbdc_timeout_sf = this->sync_period_frame + 1;

	LOG(this->log_init, LEVEL_NOTICE,
	    "ULCarrierBw %d kbits/s, "
	    "RBDC max %d kbits/s, RBDC Timeout %d frame, "
	    "VBDC max %d kbits, mslDuration %d frame\n",
	    this->cra_kbps, this->max_rbdc_kbps,
	    rbdc_timeout_sf, this->max_vbdc_kb, msl_sf);

	// dama algorithm
	if(!Conf::getValue(Conf::section_map[DVB_TAL_SECTION],
	                   DAMA_ALGO, dama_algo))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "section '%s': missing parameter '%s'\n",
		    DVB_TAL_SECTION, DAMA_ALGO);
		goto error;
	}

	if(dama_algo == "Legacy")
	{
		LOG(this->log_init, LEVEL_NOTICE,
		    "SF#%u: create Legacy DAMA agent\n",
		    this->super_frame_counter);

		if(this->return_link_std == DVB_RCS)
		{
			this->dama_agent = new DamaAgentRcsLegacy(this->rcs_modcod_def);
		}
		else if(this->return_link_std == DVB_RCS2)
		{
			this->dama_agent = new DamaAgentRcs2Legacy(this->rcs_modcod_def);
		}
		else
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "cannot create DAMA agent: algo named '%s' is not "
			    "managed by current MAC layer\n", dama_algo.c_str());
			goto error;
		}
	}
	else if(dama_algo == "RrmQos")
	{
		LOG(this->log_init, LEVEL_NOTICE,
		    "SF#%u: create RrmQos DAMA agent\n",
		    this->super_frame_counter);

		if(this->return_link_std == DVB_RCS)
		{
			this->dama_agent = new DamaAgentRcsRrmQos(this->rcs_modcod_def);
		}
		else
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "cannot create DAMA agent: algo named '%s' is not "
			    "managed by current MAC layer\n", dama_algo.c_str());
			goto error;
		}
	}
	else
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "cannot create DAMA agent: algo named '%s' is not "
		    "managed by current MAC layer\n", dama_algo.c_str());
		goto error;
	}

	if(this->dama_agent == NULL)
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to create DAMA agent\n");
		goto error;
	}

	// Initialize the DamaAgent parent class
	if(!this->dama_agent->initParent(this->ret_up_frame_duration_ms,
	                                 this->cra_kbps,
	                                 this->max_rbdc_kbps,
	                                 rbdc_timeout_sf,
	                                 this->max_vbdc_kb,
	                                 msl_sf,
	                                 this->sync_period_frame,
	                                 this->pkt_hdl,
	                                 this->dvb_fifos))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "SF#%u Dama Agent Initialization failed.\n",
		    this->super_frame_counter);
		goto err_agent_release;
	}

	// Initialize the DamaAgentRcsXXX class
	if(!this->dama_agent->init())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Dama Agent initialization failed.\n");
		goto err_agent_release;
	}

release_cat:
	for(cat_it = dama_categories.begin();
	    cat_it != dama_categories.end(); ++cat_it)
	{
		delete (*cat_it).second;
	}
	return true;

err_agent_release:
	delete this->dama_agent;
error:
	for(cat_it = dama_categories.begin();
	    cat_it != dama_categories.end(); ++cat_it)
	{
		delete (*cat_it).second;
	}
	return false;
}

bool BlockDvbTal::Downward::initSlottedAloha(void)
{
	bool is_sa_fifo = false;

	TerminalCategories<TerminalCategorySaloha> sa_categories;
	TerminalMapping<TerminalCategorySaloha> terminal_affectation;
	TerminalCategorySaloha *default_category;
	TerminalCategorySaloha *tal_category = NULL;
	TerminalMapping<TerminalCategorySaloha>::const_iterator tal_map_it;
	TerminalCategories<TerminalCategorySaloha>::iterator cat_it;
	UnitConverter *converter = NULL;

	for(fifos_t::iterator it = this->dvb_fifos.begin();
	    it != this->dvb_fifos.end(); ++it)
	{
		if((*it).second->getAccessType() == access_saloha)
		{
			is_sa_fifo = true;
		}
	}

	// get current spot into return up band section
	ConfigurationList current_spot;
	if(!OpenSandConf::getSpot(RETURN_UP_BAND,
		                      this->spot_id,
		                      NO_GW, current_spot))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "section '%s', missing spot for id %d\n",
				RETURN_UP_BAND, this->spot_id);
		return false;
	}

	if(!this->initBand<TerminalCategorySaloha>(current_spot,
	                                           RETURN_UP_BAND,
	                                           ALOHA,
	                                           this->ret_up_frame_duration_ms,
	                                           this->satellite_type,
	                                           // initialized in DAMA
	                                           this->rcs_modcod_def,
	                                           sa_categories,
	                                           terminal_affectation,
	                                           &default_category,
	                                           this->ret_fmt_groups))
	{
		return false;
	}

	if(sa_categories.size() == 0)
	{
		LOG(this->log_init, LEVEL_INFO,
		    "No Slotted Aloha carriers\n");
		return true;
	}

	// TODO should manage several Saloha carrier
	for(cat_it = sa_categories.begin();
	    cat_it != sa_categories.end(); ++cat_it)
	{	if((*cat_it).second->getCarriersGroups().size() > 1)
		{
			LOG(this->log_init, LEVEL_WARNING,
			    "If you use more than one Slotted Aloha carrier group "
			    "with different parameters, the behaviour won't be correct "
			    "for time division and MODCOD support.\n");
			break;
		}
	}

	// Find the category for this terminal
	tal_map_it = terminal_affectation.find(this->mac_id);
	if(tal_map_it == terminal_affectation.end())
	{
		// check if the default category is concerned by Slotted Aloha
		if(!default_category)
		{
			LOG(this->log_init, LEVEL_INFO,
			    "ST not affected to a Slotted Aloha category\n");
			return true;
		}
		tal_category = default_category;
	}
	else
	{
		tal_category = (*tal_map_it).second;
	}

	// check if there is Slotted Aloha carriers
	if(!tal_category)
	{
		LOG(this->log_init, LEVEL_INFO,
		    "No Slotted Aloha carrier\n");
		if(is_sa_fifo)
		{
			LOG(this->log_init, LEVEL_WARNING,
			    "Remove Slotted Aloha FIFOs because there is no "
			    "Slotted Aloha carrier\n");
			for(fifos_t::iterator it = this->dvb_fifos.begin();
			    it != this->dvb_fifos.end(); ++it)
			{
				if((*it).second->getAccessType() == access_saloha)
				{
					delete (*it).second;
					this->dvb_fifos.erase(it);
				}
			}
		}
		return true;
	}

	if(!is_sa_fifo)
	{
		LOG(this->log_init, LEVEL_WARNING,
		    "The Slotted Aloha carrier won't be used as there is no "
		    "Slotted Aloha FIFO\n");
		for(cat_it = sa_categories.begin();
		    cat_it != sa_categories.end(); ++cat_it)
		{
			delete (*cat_it).second;
		}
		return true;
	}

	for(cat_it = sa_categories.begin();
	    cat_it != sa_categories.end(); ++cat_it)
	{
		if((*cat_it).second->getLabel() != tal_category->getLabel())
		{
			delete (*cat_it).second;
		}
	}

	// cannot use Slotted Aloha with regenerative satellite
	if(this->satellite_type == REGENERATIVE)
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Carrier configured with Slotted Aloha while satellite "
		    "is regenerative\n");
		return false;
	}

	// Create the Slotted ALoha part
	this->saloha = new SlottedAlohaTal();
	if(!this->saloha)
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to create Slotted Aloha\n");
		return false;
	}

	// Initialize the Slotted Aloha parent class
	// Unlike (future) scheduling, Slotted Aloha get all categories because
	// it also handles received frames and in order to know to which
	// category a frame is affected we need to get source terminal ID
	if(!this->saloha->initParent(this->ret_up_frame_duration_ms,
	                             this->pkt_hdl))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Slotted Aloha Tal Initialization failed.\n");
		goto release_saloha;
	}

	if(this->return_link_std == DVB_RCS2)
	{
		vol_sym_t length_sym = 0;
		if(!Conf::getValue(Conf::section_map[COMMON_SECTION],
		                   RCS2_BURST_LENGTH, length_sym))
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "cannot get '%s' value", DELAY_BUFFER);
			goto release_saloha;
		}
		converter = new UnitConverterFixedSymbolLength(this->ret_up_frame_duration_ms,
		                                               0,
		                                               length_sym
		                                              );
	}
	else
	{
		converter = new UnitConverterFixedBitLength(this->ret_up_frame_duration_ms,
		                                            0,
		                                            this->pkt_hdl->getFixedLength() << 3
		                                           );
	}

	if(!this->saloha->init(this->mac_id,
	                       tal_category,
	                       this->dvb_fifos,
	                       converter))
	{
		delete converter;
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to initialize the Slotted Aloha Tal\n");
		goto release_saloha;
	}
	delete converter;

	return true;

release_saloha:
	delete this->saloha;
	return false;
}


bool BlockDvbTal::Downward::initScpc(void)
{
	TerminalCategories<TerminalCategoryDama> scpc_categories;
	TerminalMapping<TerminalCategoryDama> terminal_affectation;
	TerminalCategoryDama *default_category;
	TerminalCategoryDama *tal_category = NULL;
	TerminalCategoryDama *cat;
	TerminalMapping<TerminalCategoryDama>::const_iterator tal_map_it;
	TerminalCategories<TerminalCategoryDama>::iterator cat_it;

	ConfigurationList current_spot;

	//  Duration of the carrier -- in ms
	if(!Conf::getValue(Conf::section_map[SCPC_SECTION],
	                   SCPC_C_DURATION,
	                   this->scpc_carr_duration_ms))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Missing %s\n", SCPC_C_DURATION);
		return false;
	}

	LOG(this->log_init, LEVEL_NOTICE,
	    "scpc_carr_duration_ms = %d ms\n", this->scpc_carr_duration_ms);

	// get current spot into return up band section
	if(!OpenSandConf::getSpot(RETURN_UP_BAND,
		                      this->spot_id,
		                      NO_GW, current_spot))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "section '%s', missing spot for id %d\n",
		    RETURN_UP_BAND, this->spot_id);
		return false;
	}

	if(!this->initBand<TerminalCategoryDama>(current_spot,
	                                         RETURN_UP_BAND,
	                                         SCPC,
	                                         this->scpc_carr_duration_ms,
	                                         this->satellite_type,
	                                         // input modcod for S2
	                                         this->s2_modcod_def,
	                                         scpc_categories,
	                                         terminal_affectation,
	                                         &default_category,
	                                         this->ret_fmt_groups))
	{
		LOG(this->log_init, LEVEL_WARNING,
		    "InitBand not correctly initialized \n");

		return false;
	}

	if(scpc_categories.size() == 0)
	{
		LOG(this->log_init, LEVEL_WARNING,
		    "No SCPC carriers\n");
		// no SCPC return
		return false;
	}
	// Find the category for this terminal
	tal_map_it = terminal_affectation.find(this->mac_id);
	if(tal_map_it == terminal_affectation.end())
	{
		// check if the default category is concerned by SCPC
		if(!default_category)
		{
			LOG(this->log_init, LEVEL_INFO,
			    "ST not affected to a SCPC category\n");
			goto error;
		}
		tal_category = default_category;
	}
	else
	{
		tal_category = (*tal_map_it).second;
	}
	// check if there are SCPC carriers
	if(!tal_category)
	{
		LOG(this->log_init, LEVEL_INFO,
		    "No SCPC carrier\n");
		LOG(this->log_init, LEVEL_ERROR,
				"Remove SCPC FIFOs because there is no "
				"SCPC carrier in the return_up_band configuration\n");
		goto error;
	}

	// Check if there are DAMA or SALOHA FIFOs in the terminal
	if(this->dama_agent || this->saloha)
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Conflict: SCPC FIFOs and DAMA or SALOHA FIFOs "
		    "in the same Terminal\n");
		goto error;
	}

	//TODO: veritfy that 2ST are not using the same carrier and category

	// TODO cannot use SCPC with regenerative satellite
	if(this->satellite_type == REGENERATIVE)
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Carrier configured with SCPC while satellite "
		    "is regenerative\n");
		goto error;
	}

	// Initialise Encapsulation scheme
	if(!this->initScpcPktHdl(&this->pkt_hdl))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed get packet handler\n");
		goto error;
	}

	if(!this->initModcodDefFile(MODCOD_DEF_S2,
	                            &this->s2_modcod_def))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to initialize the return MODCOD definition file for SCPC\n");
		goto error;
	}

	// register GW
	if(!this->addOutputTerminal(this->gw_id, this->s2_modcod_def))
	{
		LOG(this->log_receive, LEVEL_ERROR,
		    "failed to register simulated ST with MAC "
		    "ID %u\n", this->tal_id);
		goto error;
	}

	// Create the SCPC scheduler
	cat = scpc_categories.begin()->second;
	this->scpc_sched = new ScpcScheduling(this->scpc_carr_duration_ms,
	                                      this->pkt_hdl,
	                                      this->dvb_fifos,
	                                      this->output_sts,
	                                      this->s2_modcod_def,
	                                      cat, this->gw_id);
	if(!this->scpc_sched)
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to initialize SCPC\n");
		goto error;
	}
	terminal_affectation.clear();
	cat_it = scpc_categories.begin();
	for(std::advance(cat_it,1);
	    cat_it != scpc_categories.end(); ++cat_it)
	{
		delete (*cat_it).second;
	}
	return true;

error:
	terminal_affectation.clear();
	for(cat_it = scpc_categories.begin();
	    cat_it != scpc_categories.end(); ++cat_it)
	{
		delete (*cat_it).second;
	}
	return false;
}


bool BlockDvbTal::Downward::initQoSServer(void)
{
	// QoS Server: read hostname and port from configuration
	if(!Conf::getValue(Conf::section_map[SECTION_QOS_AGENT],
		               QOS_SERVER_HOST,
	                   this->qos_server_host))
	{
		LOG(this->log_qos_server, LEVEL_ERROR,
		    "section %s, %s missing",
		    SECTION_QOS_AGENT, QOS_SERVER_HOST);
		goto error;
	}

	if(!Conf::getValue(Conf::section_map[SECTION_QOS_AGENT],
		               QOS_SERVER_PORT,
	                   this->qos_server_port))
	{
		LOG(this->log_qos_server, LEVEL_ERROR,
		    "section %s, %s missing\n",
		    SECTION_QOS_AGENT, QOS_SERVER_PORT);
		goto error;
	}
	else if(this->qos_server_port <= 1024 || this->qos_server_port > 0xffff)
	{
		LOG(this->log_qos_server, LEVEL_ERROR,
		    "QoS Server port (%d) not valid\n",
		    this->qos_server_port);
		goto error;
	}

	// QoS Server: catch the SIGFIFO signal that is sent to the process
	// when QoS Server kills the TCP connection
	if(signal(SIGPIPE, BlockDvbTal::Downward::closeQosSocket) == SIG_ERR)
	{
		LOG(this->log_qos_server, LEVEL_ERROR,
		    "cannot catch signal SIGPIPE\n");
		goto error;
	}

	// QoS Server: try to connect to remote host
	this->connectToQoSServer();

	return true;
error:
	return false;
}

bool BlockDvbTal::Downward::initOutput(void)
{
	this->event_login = Output::registerEvent("DVB.login");

	if(this->saloha)
	{
		this->log_saloha = Output::registerLog(LEVEL_WARNING, "Dvb.SlottedAloha");
	}

	for(fifos_t::iterator it = this->dvb_fifos.begin();
	    it != this->dvb_fifos.end(); ++it)
	{
		unsigned int id = (*it).first;

		this->probe_st_queue_size[id] =
			Output::registerProbe<int>("Queue size.packets." + ((*it).second)->getName(),
			                           "Packets", true, SAMPLE_LAST);
		this->probe_st_queue_size_kb[id] =
			Output::registerProbe<int>("Queue size." + ((*it).second)->getName(),
			                           "kbits", true, SAMPLE_LAST);

		this->probe_st_l2_to_sat_before_sched[id] =
			Output::registerProbe<int>("Throughputs.L2_to_SAT_before_sched." +
			                           ((*it).second)->getName(), "Kbits/s", true,
																 SAMPLE_AVG);
		this->probe_st_l2_to_sat_after_sched[id] =
			Output::registerProbe<int>("Throughputs.L2_to_SAT_after_sched." +
			                           ((*it).second)->getName(), "Kbits/s", true,
																 SAMPLE_AVG);
		this->probe_st_queue_loss[id] =
			Output::registerProbe<int>("Queue loss.packets." + ((*it).second)->getName(),
			                           "Packets", true, SAMPLE_LAST);
		this->probe_st_queue_loss_kb[id] =
			Output::registerProbe<int>("Queue loss." + ((*it).second)->getName(),
			                           "kbits", true, SAMPLE_LAST);
	}
	this->probe_st_l2_to_sat_total =
		Output::registerProbe<int>("Throughputs.L2_to_SAT_after_sched.total",
		                           "Kbits/s", true, SAMPLE_AVG);
	return true;
}


bool BlockDvbTal::Downward::initTimers(void)
{
	this->logon_timer = this->addTimerEvent("logon", 5000,
	                                        false, // do not rearm
	                                        false // do not start
	                                        );
	// QoS Server: check connection status in 5 seconds
	this->qos_server_timer = this->addTimerEvent("qos_server", 5000);
	if(this->scpc_sched)
	{
		this->scpc_timer = this->addTimerEvent("scpc_timer",
		                                       this->scpc_carr_duration_ms);
	}

	return true;
}


bool BlockDvbTal::Downward::onEvent(const RtEvent *const event)
{
	switch(event->getType())
	{
		case evt_message:
		{
			// first handle specific messages
			if(((MessageEvent *)event)->getMessageType() == msg_sig)
			{
				DvbFrame *frame = (DvbFrame *)((MessageEvent *)event)->getData();
				if(!this->handleDvbFrame(frame))
				{
					return false;
				}
				break;
			}
			// messages from upper layer: burst of encapsulation packets
			NetBurst *burst;
			NetBurst::iterator pkt_it;
			std::string message;
			std::ostringstream oss;
			int ret;
			// TODO move saloha handling in a specific function ?
			// Slotted Aloha variables
			unsigned int sa_burst_size = 0; // burst size
			unsigned int sa_offset = 0; // packet position (offset) in the burst
			SlottedAlohaPacketData *sa_packet = NULL; // Slotted Aloha packet

			burst = (NetBurst *)((MessageEvent *)event)->getData();

			sa_burst_size = burst->length();
			LOG(this->log_receive, LEVEL_INFO,
			    "SF#%u: encapsulation burst received (%d "
			    "packets)\n", this->super_frame_counter,
			    sa_burst_size);


			// set each packet of the burst in MAC FIFO
			for(pkt_it = burst->begin(); pkt_it != burst->end(); pkt_it++)
			{
				qos_t fifo_priority = (*pkt_it)->getQos();

				LOG(this->log_receive, LEVEL_DEBUG,
				    "SF#%u: encapsulation packet has QoS value %u\n",
				    this->super_frame_counter, fifo_priority);

				// find the FIFO associated to the IP QoS (= MAC FIFO id)
				// else use the default id
				if(this->dvb_fifos.find(fifo_priority) == this->dvb_fifos.end())
				{
					fifo_priority = this->default_fifo_id;
				}

				// Slotted Aloha
				sa_packet = NULL;
				if(this->saloha &&
				   this->dvb_fifos[fifo_priority]->getAccessType() == access_saloha)
				{
					sa_packet = this->saloha->addSalohaHeader(*pkt_it,
					                                          sa_offset++,
					                                          sa_burst_size);
					if(!sa_packet)
					{
						LOG(this->log_saloha, LEVEL_ERROR,
						    "SF#%u: unable to "
						    "store received Slotted Aloha encapsulation "
						    "packet (see previous errors)\n",
						    this->super_frame_counter);
						return false;
					}
				}

				LOG(this->log_receive, LEVEL_INFO,
				    "SF#%u: store one encapsulation packet "
				    "(QoS = %d)\n", this->super_frame_counter,
				    fifo_priority);

				// store the encapsulation packet in the FIFO
				if(!this->onRcvEncapPacket(sa_packet ? sa_packet : *pkt_it,
				                           this->dvb_fifos[fifo_priority],
				                           0))
				{
					// a problem occured, we got memory allocation error
					// or fifo full and we won't empty fifo until next
					// call to onDownwardEvent => return
					LOG(this->log_receive, LEVEL_ERROR,
					    "SF#%u: unable to "
					    "store received encapsulation "
					    "packet (see previous errors)\n",
					    this->super_frame_counter);
					burst->clear();
					delete burst;
					return false;
				}

			}
			burst->clear(); // avoid deteleting packets when deleting burst
			delete burst;

			// Cross layer information: if connected to QoS Server, build XML
			// message and send it
			// TODO move in a dedicated class
			if(BlockDvbTal::Downward::qos_server_sock == -1)
			{
				break;
			}

			message = "";
			message.append("<?xml version = \"1.0\" encoding = \"UTF-8\"?>\n");
			message.append("<XMLQoSMessage>\n");
			message.append(" <Sender>");
			message.append("CrossLayer");
			message.append("</Sender>\n");
			message.append(" <Type type=\"CrossLayer\" >\n");
			message.append(" <Infos ");
			for(fifos_t::iterator it = this->dvb_fifos.begin();
			    it != this->dvb_fifos.end(); ++it)
			{
				int nbFreeFrames = (*it).second->getMaxSize() -
				                   (*it).second->getCurrentSize();
				// bits
				int nbFreeBits = nbFreeFrames * this->pkt_hdl->getFixedLength() * 8;
				// bits/ms or kbits/s
				float macRate = nbFreeBits / this->ret_up_frame_duration_ms ;
				oss << "File=\"" << (int) macRate << "\" ";
				message.append(oss.str());
				oss.str("");
			}
			message.append("/>");
			message.append(" </Type>\n");
			message.append("</XMLQoSMessage>\n");

			ret = write(BlockDvbTal::Downward::qos_server_sock,
			            message.c_str(),
			            message.length());
			if(ret == -1)
			{
				LOG(this->log_receive, LEVEL_NOTICE,
				    "failed to send message to QoS Server: %s "
				    "(%d)\n", strerror(errno), errno);
			}
		}
		break;

		case evt_timer:
		{
			if(*event == this->logon_timer)
			{
				if(this->state == state_wait_logon_resp)
				{
					// send another logon_req and raise timer
					// only if we are in the good state
					LOG(this->log_receive, LEVEL_NOTICE,
					    "still no answer from NCC to the "
					    "logon request we sent for MAC ID %d, "
					    "send a new logon request\n",
					    this->mac_id);
					return this->sendLogonReq();
				}
				return true;
			}
			if(this->state != state_running)
			{
				LOG(this->log_receive, LEVEL_DEBUG,
				    "Ignore timer event %s while not logged\n",
				    event->getName().c_str());
				return true;
			}

			if(*event == this->qos_server_timer)
			{
				// try to re-connect to QoS Server if not already connected
				if(BlockDvbTal::Downward::qos_server_sock == -1)
				{
					if(!this->connectToQoSServer())
					{
						LOG(this->log_receive, LEVEL_INFO,
						    "failed to connect with QoS Server, "
						    "cannot send cross layer informationi\n");
					}
				}
			}
			else if(*event == this->scpc_timer)
			{
				// TODO fct ++ add extension dans GSE
				uint32_t remaining_alloc_sym = 0;

				this->updateStats();
				this->scpc_frame_counter++;

				if(!this->addCniExt())
				{
					LOG(this->log_send_channel, LEVEL_ERROR,
					    "fail to add CNI extension");
					return false;
				}

				//Schedule Creation
				// TODO we should send packets containing CNI extension with
				//      the most robust MODCOD
				if(!this->scpc_sched->schedule(this->scpc_frame_counter,
				                               getCurrentTime(),
				                               &this->complete_dvb_frames,
				                               remaining_alloc_sym))
				{
					LOG(this->log_receive, LEVEL_ERROR,
					    "failed to schedule SCPC encapsulation "
					    "packets stored in DVB FIFO\n");
					return false;

				}

				LOG(this->log_receive, LEVEL_INFO,
				    "SF#%u: %u symbol remaining after "
				    "scheduling\n", this->super_frame_counter,
				    remaining_alloc_sym);

				// send on the emulated DVB network the DVB frames that contain
				// the encapsulation packets scheduled by the SCPC agent algorithm
				if(!this->sendBursts(&this->complete_dvb_frames,
					this->carrier_id_data))
					{
						LOG(this->log_frame_tick, LEVEL_ERROR,
						    "failed to send bursts in DVB frames\n");
						return false;
					}
			}
			else
			{
				LOG(this->log_receive, LEVEL_ERROR,
				    "SF#%u: unknown timer event received %s\n",
				    this->super_frame_counter, event->getName().c_str());
				return false;
			}
			break;
		}
        case evt_net_socket:
        {
            if(*event == this->conf_update_interface.getConfUpdateClientSocket())
            {
                ConfUpdateRequest *request;

                // event received on ConfUpdate client socket
                LOG(this->log_receive, LEVEL_NOTICE,
                    "event received on ConfUpdate client socket\n");

                // read the message sent by ConfUpdate or delete socket
                // if connection is dead
                if(!this->conf_update_interface.readConfUpdateMessage((NetSocketEvent *)event))
                {
                    LOG(this->log_receive, LEVEL_WARNING,
                        "network problem encountered with ConfUpdate, "
                        "connection was therefore closed\n");
                    // Free the socket
                    if(shutdown(this->conf_update_interface.getConfUpdateClientSocket(), SHUT_RDWR) != 0)
                    {
                        LOG(this->log_init, LEVEL_ERROR,
                            "failed to clase socket: "
                            "%s (%d)\n", strerror(errno), errno);
                    }
                    this->removeEvent(this->conf_update_interface.getConfUpdateClientSocket());
                    return false;
                }
                // we have received a set of commands from the
                // ConfUpdate component, let's apply the configuration updates they asked for

                //apply the commands, one by one
                request = this->conf_update_interface.getNextConfUpdateRequest();
                while(request != NULL)
                {
                    // first get the spot concerned by the request
                    //if the request's SPOT matches this ST's Spot, apply the request, else drop it
                    if(request->getSpotId() == this->spot_id){
                        if(!this->applyConfUpdateCommand(request)) // TODO : note the call here (ajouter au futur diagramme expliquant mes modifs)
                        {
                            LOG(this->log_receive, LEVEL_ERROR,
                                "Cannot apply ConfUpdate interface request\n");
                            return false;
                        } else {
                            LOG(this->log_receive, LEVEL_WARNING,
                                "confUpdate request applied successfully\n");
                        }
                    } else {
                        LOG(this->log_receive, LEVEL_WARNING,
                            "Received a ConfUpdate request with wrong spot id : %d, while this ST spot ID is %d. Drop it.",
                            request->getSpotId(), this->spot_id);
                        //nothing to do
                    }

                    delete request;
                    request = this->conf_update_interface.getNextConfUpdateRequest();
                }

                // Free the socket
                if(shutdown(this->conf_update_interface.getConfUpdateClientSocket(), SHUT_RDWR) != 0)
                {
                    LOG(this->log_init, LEVEL_ERROR,
                        "failed to close socket: "
                        "%s (%d)\n", strerror(errno), errno);
                }
                this->removeEvent(this->conf_update_interface.getConfUpdateClientSocket());
            }
        }
        break;
        case evt_tcp_listen:
        {
            if(*event == this->conf_update_interface.getConfUpdateListenSocket())
            {
                this->conf_update_interface.setSocketClient(((TcpListenEvent *)event)->getSocketClient());
                this->conf_update_interface.setIsConnected(true);

                // event received on ConfUpdate listen socket
                LOG(this->log_receive, LEVEL_NOTICE,
                    "event received on ConfUpdate listen socket\n");

                // create the client socket to receive messages
                LOG(this->log_receive, LEVEL_NOTICE,
                    "ST is now connected to ConfUpdate\n");
                // add a fd to handle events on the client socket
                this->addNetSocketEvent("conf_update_client",
                                        this->conf_update_interface.getConfUpdateClientSocket(),
                                        200);
            }
            break;
        }
		default:
			LOG(this->log_receive, LEVEL_ERROR,
			    "SF#%u: unknown event received %s",
			    this->super_frame_counter,
			    event->getName().c_str());
			return false;
	}

	return true;
}

bool BlockDvbTal::Downward::addCniExt(void)
{
	bool in_fifo = false;

	// Create list of first packet from FIFOs
	for(fifos_t::iterator fifos_it = this->dvb_fifos.begin();
	    fifos_it != this->dvb_fifos.end(); ++fifos_it)
	{
		DvbFifo *fifo = (*fifos_it).second;
		vector<MacFifoElement *> queue = fifo->getQueue();
		vector<MacFifoElement *>::iterator queue_it;

		for(queue_it = queue.begin() ;
		    queue_it != queue.end()  ;
		    ++queue_it)
		{
			std::vector<NetPacket*> packet_list;
			MacFifoElement* elem = (*queue_it);
			NetPacket *packet = (NetPacket*)elem->getElem();
			tal_id_t gw = packet->getDstTalId();
			NetPacket *extension_pkt = NULL;

			if(gw == this->gw_id &&
			   this->is_scpc && this->getCniInputHasChanged(this->tal_id))
			{
				packet_list.push_back(packet);
				if(!this->setPacketExtension(this->pkt_hdl,
					                         elem, fifo,
					                         packet_list,
					                         &extension_pkt,
					                         this->tal_id, gw,
					                         ENCODE_CNI_EXT,
					                         this->super_frame_counter,
					                         false))
				{
					return false;
				}

				LOG(this->log_send_channel, LEVEL_DEBUG,
				    "SF #%d: packet belongs to FIFO #%d\n",
				    this->super_frame_counter, (*fifos_it).first);
				// Delete old packet
				delete packet;
				in_fifo = true;
			}
		}
	}

	if(this->is_scpc && this->getCniInputHasChanged(this->tal_id)
	   && !in_fifo)
	{
		std::vector<NetPacket*> packet_list;
		NetPacket *extension_pkt = NULL;

		// set packet extension to this new empty packet
		if(!this->setPacketExtension(this->pkt_hdl,
			                         NULL, this->dvb_fifos[0],
			                         packet_list,
				                     &extension_pkt,
					                 this->tal_id ,this->gw_id,
					                 ENCODE_CNI_EXT,
					                 this->super_frame_counter,
					                 false))
		{
			return false;
		}

		LOG(this->log_send_channel, LEVEL_DEBUG,
			"SF #%d: adding empty packet into FIFO NM\n",
		    this->super_frame_counter);
	}

	return true;
}

bool BlockDvbTal::Downward::sendLogonReq(void)
{
	LogonRequest *logon_req = new LogonRequest(this->mac_id,
	                                           this->cra_kbps,
	                                           this->max_rbdc_kbps,
	                                           this->max_vbdc_kb,
	                                           this->is_scpc);

	// send the message to the lower layer
	if(!this->sendDvbFrame((DvbFrame *)logon_req,
		                   this->carrier_id_logon))
	{
		LOG(this->log_send, LEVEL_ERROR,
		    "Failed to send Logon Request\n");
		goto error;
	}
	LOG(this->log_send, LEVEL_DEBUG,
	    "SF#%u Logon Req. sent to lower layer\n",
	    this->super_frame_counter);

	if(!this->startTimer(this->logon_timer))
	{
		LOG(this->log_send, LEVEL_ERROR,
		    "cannot start logon timer");
		goto error;
	}

	// send the corresponding event
	Output::sendEvent(this->event_login, "Login sent to GW");
	return true;

error:
	return false;
}


bool BlockDvbTal::Downward::handleDvbFrame(DvbFrame *dvb_frame)
{
	// frames transmitted from Upward
	uint8_t msg_type = dvb_frame->getMessageType();
	switch(msg_type)
	{
		case MSG_TYPE_SALOHA_CTRL:
			if(this->saloha && !this->saloha->onRcvFrame(dvb_frame))
			{
				LOG(this->log_saloha, LEVEL_ERROR,
				    "failed to handle Slotted Aloha Signal Controls frame\n");
				goto error;
			}
			break;

		case MSG_TYPE_SOF:
			if(!this->handleStartOfFrame(dvb_frame))
			{
				LOG(this->log_receive, LEVEL_ERROR,
				    "Cannot handle SoF\n");
				delete dvb_frame;
				goto error;
			}
			delete dvb_frame;
			break;

		case MSG_TYPE_TTP:
		{
			// TODO Ttp *ttp = dynamic_cast<Ttp *>(dvb_frame);
			Ttp *ttp = (Ttp *)dvb_frame;
			if(this->dama_agent && !this->dama_agent->hereIsTTP(ttp))
			{
				delete dvb_frame;
				goto error_on_TTP;
			}
			delete dvb_frame;
		}
		break;

		case MSG_TYPE_SESSION_LOGON_RESP:
			if(!this->handleLogonResp(dvb_frame))
			{
				LOG(this->log_receive, LEVEL_ERROR,
				    "Cannot handle logon response\n");
				delete dvb_frame;
				goto error;
			}
			delete dvb_frame;
			break;

		default:
			LOG(this->log_receive, LEVEL_ERROR,
			    "SF#%u: unknown type of DVB frame (%u), ignore\n",
			    this->super_frame_counter,
			    dvb_frame->getMessageType());
			delete dvb_frame;
			goto error;
	}

	return true;

error_on_TTP:
	LOG(this->log_receive, LEVEL_ERROR,
	    "TTP Treatments failed at SF#%u\n",
	    this->super_frame_counter);
	return false;

error:
	LOG(this->log_receive, LEVEL_ERROR,
	    "Treatments failed at SF#%u\n",
	    this->super_frame_counter);
	return false;
}


bool BlockDvbTal::Downward::sendSAC(void)
{
	bool empty;
	Sac *sac;
	double cni;

	if(!this->dama_agent)
	{
		return true;
	}
	sac = new Sac(this->tal_id, this->group_id);
	// Set CR body
	// NB: access_type parameter is not used here as CR is built for both
	// RBDC and VBDC
	if(!this->dama_agent->buildSAC(access_dama_cra,
	                               sac,
	                               empty))
	{
		LOG(this->log_send, LEVEL_ERROR,
		    "SF#%u: DAMA cannot build CR\n",
		    this->super_frame_counter);
		goto error;
	}
	// Set the ACM parameters
	cni = this->getRequiredCniInput(this->tal_id);
	sac->setAcm(cni);

	if(empty)
	{
		LOG(this->log_send, LEVEL_DEBUG,
		    "SF#%u: Empty CR\n",
		    this->super_frame_counter);
		// keep going as we can send ACM parameters
	}

	// Send message
	if(!this->sendDvbFrame((DvbFrame *)sac,
	                       this->carrier_id_ctrl))
	{
		LOG(this->log_send, LEVEL_ERROR,
		    "SF#%u: failed to send SAC\n",
		    this->super_frame_counter);
		delete sac;
		goto error;
	}

	LOG(this->log_send, LEVEL_INFO,
	    "SF#%u: SAC sent\n", this->super_frame_counter);

	return true;

error:
	return false;
}


bool BlockDvbTal::Downward::handleStartOfFrame(DvbFrame *dvb_frame)
{
	uint16_t sfn; // the superframe number piggybacked by SOF packet
	// TODO Sof *sof = dynamic_cast<Sof *>(dvb_frame);
	Sof *sof = (Sof *)dvb_frame;

	sfn = sof->getSuperFrameNumber();

	LOG(this->log_frame_tick, LEVEL_DEBUG,
	    "SOF reception SFN #%u super frame nb counter %u\n",
	    sfn, this->super_frame_counter);
	LOG(this->log_frame_tick, LEVEL_DEBUG,
	    "superframe number: %u\n", sfn);

	// if the NCC crashed, we must reinitiate a logon
	if(sfn < this->super_frame_counter &&
	   (sfn != 0 || (this->super_frame_counter + 1) % 65536 != 0))
	{
		LOG(this->log_frame_tick, LEVEL_ERROR,
		    "SF#%u: it seems NCC rebooted => flush buffer & "
		    "resend a logon request\n",
		    this->super_frame_counter);

		this->deletePackets();
		if(!this->sendLogonReq())
		{
			goto error;
		}

		this->state = state_wait_logon_resp;
		this->super_frame_counter = sfn;
		goto error;
	}

	// update the frame numerotation
	this->super_frame_counter = sfn;

	// Inform dama agent
	if(this->dama_agent && !this->dama_agent->hereIsSOF(sfn))
	{
		goto error;
	}

	// There is a risk of unprecise timing so the following hack

	LOG(this->log_frame_tick, LEVEL_INFO,
	    "SF#%u: all frames from previous SF are "
	    "consumed or it is the first frame\n",
	    this->super_frame_counter);


	// we have consumed all of our frames, we start a new one immediately
	// this is the first frame of the new superframe
	if(this->processOnFrameTick() < 0)
	{
		// exit because the bloc is unable to continue
		LOG(this->log_frame_tick, LEVEL_ERROR,
		    "SF#%u: treatments failed\n",
		    this->super_frame_counter);
		goto error;
	}

	if(this->saloha)
	{
		// Slotted Aloha
		if(!this->saloha->schedule(this->complete_dvb_frames,
		                           this->super_frame_counter))
		{
			LOG(this->log_saloha, LEVEL_ERROR,
			    "SF#%u: failed to process Slotted Aloha frame tick\n",
			     this->super_frame_counter);
			goto error;
		}
	}

	return true;

error:
	return false;
}


bool BlockDvbTal::Downward::processOnFrameTick(void)
{
	this->updateStats();

	LOG(this->log_frame_tick, LEVEL_INFO,
	    "SF#%u: start processOnFrameTick\n",
	    this->super_frame_counter);

	if(this->dama_agent)
	{
		// ---------- tell the DAMA agent that a new frame begins ----------
		// Inform dama agent, and update total Available Allocation
		// for current frame
		if(!this->dama_agent->processOnFrameTick())
		{
			LOG(this->log_frame_tick, LEVEL_ERROR,
			    "SF#%u: failed to process frame tick\n",
			    this->super_frame_counter);
			goto error;
		}

		// ---------- schedule and send data frames ---------
		// schedule packets extracted from DVB FIFOs according to
		// the algorithm defined in DAMA agent
		if(!this->dama_agent->returnSchedule(&this->complete_dvb_frames))
		{
			LOG(this->log_frame_tick, LEVEL_ERROR,
			    "SF#%u: failed to schedule packets from DVB "
			    "FIFOs\n", this->super_frame_counter);
			goto error;
		}
	}

	// send on the emulated DVB network the DVB frames that contain
	// the encapsulation packets scheduled by the DAMA agent algorithm
	if(!this->sendBursts(&this->complete_dvb_frames,
	                     this->carrier_id_data))
	{
		LOG(this->log_frame_tick, LEVEL_ERROR,
		    "failed to send bursts in DVB frames\n");
		goto error;
	}

	// ---------- SAC ----------
	// compute Capacity Request and send SAC...
	// only if the OBR period has been reached
	if((this->super_frame_counter % this->sync_period_frame) == this->obr_slot_frame)
	{
		if(!this->sendSAC())
		{
			LOG(this->log_frame_tick, LEVEL_ERROR,
			    "failed to send SAC\n");
			goto error;
		}
	}

	return true;

error:
	return false;
}


bool BlockDvbTal::Downward::handleLogonResp(DvbFrame *frame)
{
	// TODO static or dynamic_cast
	LogonResponse *logon_resp = (LogonResponse *)frame;
	// Remember the id
	this->group_id = logon_resp->getGroupId();
	this->tal_id = logon_resp->getLogonId();

	// Inform Dama agent
	if(this->dama_agent && !this->dama_agent->hereIsLogonResp(logon_resp))
	{
		return false;
	}

	// Set the state to "running"
	this->state = state_running;

	// send the corresponding event
	Output::sendEvent(event_login, "Login complete with MAC %d",
	                  this->mac_id);

	return true;
}


void BlockDvbTal::Downward::updateStats(void)
{
	if(!this->doSendStats())
	{
		return;
	}

	if(this->dama_agent)
	{
		this->dama_agent->updateStatistics(this->stats_period_ms);
	}

	mac_fifo_stat_context_t fifo_stat;
	// MAC fifos stats
	for(fifos_t::iterator it = this->dvb_fifos.begin();
	    it != this->dvb_fifos.end(); ++it)
	{
		(*it).second->getStatsCxt(fifo_stat);

		this->l2_to_sat_total_bytes += fifo_stat.out_length_bytes;

		// write in statitics file
		this->probe_st_l2_to_sat_before_sched[(*it).first]->put(
			fifo_stat.in_length_bytes * 8 /
			this->stats_period_ms);
		this->probe_st_l2_to_sat_after_sched[(*it).first]->put(
			fifo_stat.out_length_bytes * 8 /
			this->stats_period_ms);

		this->probe_st_queue_size[(*it).first]->put(fifo_stat.current_pkt_nbr);
		this->probe_st_queue_size_kb[(*it).first]->put(
				fifo_stat.current_length_bytes * 8 / 1000);
		this->probe_st_queue_loss[(*it).first]->put(fifo_stat.drop_pkt_nbr);
		this->probe_st_queue_loss_kb[(*it).first]->put(fifo_stat.drop_bytes * 8);
	}
	this->probe_st_l2_to_sat_total->put(this->l2_to_sat_total_bytes * 8 /
	                                    this->stats_period_ms);

	// reset stat
	this->l2_to_sat_total_bytes = 0;
}


// TODO: move to a dedicated class
/**
 * Signal callback called upon SIGFIFO reception.
 *
 * This function is declared as static.
 *
 * @param sig  The signal that called the function
 */
void BlockDvbTal::Downward::closeQosSocket(int UNUSED(sig))
{
	// TODO static function, no this->
	DFLTLOG(LEVEL_NOTICE,
	        "TCP connection broken, close socket\n");
	close(BlockDvbTal::Downward::qos_server_sock);
	BlockDvbTal::Downward::qos_server_sock = -1;
}


// TODO: move to a dedicated class
/**
 * Try to connect to the QoS Server
 *
 * The qos_server_host and qos_server_port class variables must be correctly
 * initialized. The qos_server_sock variable should be -1 when calling this
 * function.
 *
 * @return   true if connection is successful, false otherwise
 */
bool BlockDvbTal::Downward::connectToQoSServer()
{
	struct addrinfo hints;
	struct protoent *tcp_proto;
	struct servent *serv;
	struct addrinfo *addresses;
	struct addrinfo *address;
	char straddr[INET6_ADDRSTRLEN];
	int ret;

	if(BlockDvbTal::Downward::qos_server_sock != -1)
	{
		LOG(this->log_qos_server, LEVEL_NOTICE,
		    "already connected to QoS Server, do not call this "
		    "function when already connected\n");
		goto skip;
	}

	// set criterias to resolve hostname
	bzero(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	// get TCP protocol number
	tcp_proto = getprotobyname("TCP");
	if(tcp_proto == NULL)
	{
		LOG(this->log_qos_server, LEVEL_ERROR,
		    "TCP is not available on the system\n");
		goto error;
	}
	hints.ai_protocol = tcp_proto->p_proto;

	// get service name
	serv = getservbyport(htons(this->qos_server_port), "tcp");
	if(serv == NULL)
	{
		LOG(this->log_qos_server, LEVEL_INFO,
		    "service on TCP/%d is not available\n",
		    this->qos_server_port);
		goto error;
	}

	// resolve hostname
	ret = getaddrinfo(this->qos_server_host.c_str(), serv->s_name, &hints, &addresses);
	if(ret != 0)
	{
		LOG(this->log_qos_server, LEVEL_NOTICE,
		    "cannot resolve hostname '%s': %s (%d)\n",
		    this->qos_server_host.c_str(),
		    gai_strerror(ret), ret);
		goto error;
	}

	// try to create socket with available addresses
	address = addresses;
	while(address != NULL && BlockDvbTal::Downward::qos_server_sock == -1)
	{
		bool is_ipv4;
		void *sin_addr;
		const char *retptr;

		is_ipv4 = (address->ai_family == AF_INET);
		if(is_ipv4)
			sin_addr = &((struct sockaddr_in *) address->ai_addr)->sin_addr;
		else // ipv6
			sin_addr = &((struct sockaddr_in6 *) address->ai_addr)->sin6_addr;

		retptr = inet_ntop(address->ai_family,
		                   sin_addr,
		                   straddr,
		                   sizeof(straddr));
		if(retptr != NULL)
		{
			LOG(this->log_qos_server, LEVEL_INFO,
			    "try IPv%d address %s\n",
			    is_ipv4 ? 4 : 6, straddr);
		}
		else
		{
			LOG(this->log_qos_server, LEVEL_INFO,
			    "try an IPv%d address\n",
			    is_ipv4 ? 4 : 6);
		}

		BlockDvbTal::Downward::qos_server_sock = socket(address->ai_family,
		                                      address->ai_socktype,
		                                      address->ai_protocol);
		if(BlockDvbTal::Downward::qos_server_sock == -1)
		{
			LOG(this->log_qos_server, LEVEL_INFO,
			    "cannot create socket (%s) with address %s\n",
			    strerror(errno), straddr);
			address = address->ai_next;
			continue;
		}

		LOG(this->log_qos_server, LEVEL_INFO,
		    "socket created for address %s\n",
		    straddr);
	}

	if(BlockDvbTal::Downward::qos_server_sock == -1)
	{
		LOG(this->log_qos_server, LEVEL_NOTICE,
		    "no valid address found for hostname %s\n",
		    this->qos_server_host.c_str());
		goto free_dns;
	}

	LOG(this->log_qos_server, LEVEL_INFO,
	    "try to connect with QoS Server at %s[%s]:%d\n",
	    this->qos_server_host.c_str(), straddr,
	    this->qos_server_port);

	// try to connect with the socket
	ret = connect(BlockDvbTal::Downward::qos_server_sock,
	              address->ai_addr, address->ai_addrlen);
	if(ret == -1)
	{
		LOG(this->log_qos_server, LEVEL_INFO,
		    "connect() failed: %s (%d)\n",
		    strerror(errno), errno);
		LOG(this->log_qos_server, LEVEL_INFO,
		    "will retry to connect later\n");
		goto close_socket;
	}

	LOG(this->log_qos_server, LEVEL_NOTICE,
	    "connected with QoS Server at %s[%s]:%d\n",
	    this->qos_server_host.c_str(), straddr,
	    this->qos_server_port);

	// clean allocated addresses
	freeaddrinfo(addresses);

skip:
	return true;

close_socket:
	close(BlockDvbTal::Downward::qos_server_sock);
	BlockDvbTal::Downward::qos_server_sock = -1;
free_dns:
	freeaddrinfo(addresses);
error:
	return false;
}

void BlockDvbTal::Downward::deletePackets()
{
	for(fifos_t::iterator it = this->dvb_fifos.begin();
	    it != this->dvb_fifos.end(); ++it)
	{
		(*it).second->flush();
	}
}


bool BlockDvbTal::Downward::applyConfUpdateCommand(ConfUpdateRequest *conf_update_request){

    bool confUpdated;

    //apply command for all types of SAT
    if(conf_update_request->getType() == CONF_UPDATE_RETURN_BANDWIDTH){
        //update the BANDWIDTH entry in the configuration file for the specified Spot
        ConfUpdateXMLParser *parser = new ConfUpdateXMLParser();
        //parser->modifyForwardBandwidthInGlobalConf(conf_update_request->getSpotId(), conf_update_request->getGatewayId(), conf_update_request->getBandwidthNewValue());
        confUpdated = parser->modifyReturnBandwidthInGlobalConf(conf_update_request->getSpotId(), conf_update_request->getGatewayId(), conf_update_request->getBandwidthNewValue());
        if(!confUpdated){
            LOG(this->log_receive_channel, LEVEL_ERROR,
                "Error during XML configuration file update in BlockDvbTal");
            return false;
        }
    } else if(conf_update_request->getType() == CONF_UPDATE_FORWARD_BANDWIDTH){
        //update the BANDWIDTH entry in the configuration file for the specified Spot
        ConfUpdateXMLParser *parser = new ConfUpdateXMLParser();
        confUpdated = parser->modifyForwardBandwidthInGlobalConf(conf_update_request->getSpotId(), conf_update_request->getGatewayId(), conf_update_request->getBandwidthNewValue());
        //parser->modifyReturnBandwidthInGlobalConf(conf_update_request->getSpotId(), conf_update_request->getGatewayId(), conf_update_request->getBandwidthNewValue());
        if(!confUpdated){
            LOG(this->log_receive_channel, LEVEL_ERROR,
                "Error during XML configuration file update in BlockDvbTal");
            return false;
        }
    } else {
        LOG(this->log_receive_channel, LEVEL_ERROR,
            "unknown ConfUpdate request type in BlockDvbTal");
        return false;
    }

    //Reload the configuration file
    {
        vector <string> conf_files;
        string topology_file = CONF_PATH + string(CONF_TOPOLOGY);
        string global_file = CONF_PATH + string(CONF_GLOBAL_FILE);
        string default_file = CONF_PATH + string(CONF_DEFAULT_FILE);
        string plugin_conf_path = CONF_PATH + string("plugins/");

        conf_files.push_back(topology_file.c_str());
        conf_files.push_back(global_file.c_str());
        conf_files.push_back(default_file.c_str());

        //Unload configuration files content //TODO Dangerous if not reloaded properly !!!
        Conf::unloadConfig();
        // Load configuration files content
        if (!Conf::loadConfig(conf_files)) {
            LOG(this->log_init, LEVEL_CRITICAL,
                "BlockDvbTal : cannot reload configuration files\n");
            return false;
        } else {
            LOG(this->log_init, LEVEL_WARNING,
                "BlockDvbTal : configuration file reloaded with success\n");
        }
        OpenSandConf::loadConfig();

        // load the plugins
        if(!Plugin::loadPlugins(true, plugin_conf_path))
        {
            LOG(this->log_init, LEVEL_CRITICAL,
                    "BlockDvbTal : cannot load the plugins\n");
            return false;
        }
    }



    //Finally, recompute the bandwidth allocation
    if(!this->is_scpc)
    {
        if(!this->initDama())
        {
            LOG(this->log_init, LEVEL_ERROR,
                "failed to complete the DAMA part of the initialisation\n");
            return false;
        }

        if(!this->initSlottedAloha())
        {
            LOG(this->log_init, LEVEL_ERROR,
                "failed to complete the initialisation of Slotted Aloha\n");
            return false;
        }
    }
    else
    {
        if(!this->initScpc())
        {
            LOG(this->log_init, LEVEL_ERROR,
                "failed to complete the SCPC part of the initialisation\n");
            return false;
        }
    }

    return true;
}

/*****************************************************************************/
/*                               Upward                                      */
/*****************************************************************************/

BlockDvbTal::Upward::Upward(const string &name, tal_id_t mac_id):
	DvbUpward(name),
	reception_std(NULL),
	mac_id(mac_id),
	group_id(),
	tal_id(),
	gw_id(),
	spot_id(),
	is_scpc(false),
	state(state_initializing),
	probe_st_l2_from_sat(NULL),
	probe_st_required_modcod(NULL),
	probe_st_received_modcod(NULL),
	probe_st_rejected_modcod(NULL),
	probe_sof_interval(NULL)
{
}

BlockDvbTal::Upward::~Upward()
{
	// release the reception DVB standards
	if(this->reception_std != NULL)
	{
		delete this->reception_std;
	}
}


bool BlockDvbTal::Upward::onEvent(const RtEvent *const event)
{
	switch(event->getType())
	{
		case evt_message:
		{
			DvbFrame *dvb_frame = (DvbFrame *)((MessageEvent *)event)->getData();

			if(this->probe_sof_interval->isEnabled() &&
			   dvb_frame->getMessageType() == MSG_TYPE_SOF)
			{
				struct timeval time = event->getTimeFromCustom();
				float val = time.tv_sec * 1000000L + time.tv_usec;
				event->setCustomTime();
				this->probe_sof_interval->put(val/1000);
			}

			// message from lower layer: DL dvb frame
			LOG(this->log_receive, LEVEL_DEBUG,
			    "SF#%u DVB frame received (len %u)\n",
			    this->super_frame_counter,
			    dvb_frame->getMessageLength());

			if(!this->onRcvDvbFrame(dvb_frame))
			{
				LOG(this->log_receive, LEVEL_DEBUG,
				    "SF#%u: failed to handle received DVB frame\n",
				    this->super_frame_counter);
				// a problem occured, trace is made in onRcvDVBFrame()
				// carry on simulation
				return false;
			}
		}
		break;

		default:
			LOG(this->log_receive, LEVEL_ERROR,
			    "SF#%u: unknown event received %s",
			    this->super_frame_counter,
			    event->getName().c_str());
			return false;
	}

	return true;
}


bool BlockDvbTal::Upward::onInit(void)
{
	// Initialization of spot_id and gw_id
	if(OpenSandConf::spot_table.find(this->mac_id) != OpenSandConf::spot_table.end())
	{
		this->spot_id = OpenSandConf::spot_table[this->mac_id];
		this->gw_id = OpenSandConf::gw_table[this->mac_id];
	}
	else
	{
		if(!Conf::getValue(Conf::section_map[SPOT_TABLE_SECTION],
		                   DEFAULT_SPOT, this->spot_id))
		{
			LOG(this->log_init_channel, LEVEL_ERROR,
				"couldn't find spot for tal %d",
			    this->mac_id);
			return false;
		}

		if(!Conf::getValue(Conf::section_map[GW_TABLE_SECTION],
		                   DEFAULT_GW, this->gw_id))
		{
			LOG(this->log_init_channel, LEVEL_ERROR,
			    "couldn't find gw for tal %d",
			    this->mac_id);
			return false;
		}
	}

	if(!Conf::getValue(Conf::section_map[DVB_TAL_SECTION], IS_SCPC, this->is_scpc))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "section '%s': missing parameter '%s'\n",
		    DVB_TAL_SECTION, IS_SCPC);
		return false;
	}

	if(!this->initModcodDefinitionTypes())
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed to initialize MOCODS definitions types\n");
		return false;
	}

	// get the common parameters
	if(!this->initCommon(FORWARD_DOWN_ENCAP_SCHEME_LIST))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to complete the common part of the "
		    "initialisation\n");
		return false;
	}

	if(!this->initModcodSimu())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to complete the initialisation of the Modcod Simu\n");
		return false;
	}

	if(!this->initMode())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to complete the mode part of the "
		    "initialisation\n");
		return false;
	}

	// Init the output here since we now know the FIFOs
	if(!this->initOutput())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to complete the initialisation of output\n");
		return false;
	}

	// we synchornize with SoF reception so use the return frame duration here
	this->initStatsTimer(this->ret_up_frame_duration_ms);

	return true;
}

// TODO remove reception_std as functions are merged but contains part
//      dedicated to each host ?
bool BlockDvbTal::Upward::initMode(void)
{
	this->reception_std = new DvbS2Std(this->pkt_hdl);
	((DvbS2Std *)this->reception_std)->setModcodDef(this->s2_modcod_def);
	if(this->reception_std == NULL)
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Failed to initialize reception standard\n");
		return false;
	}
	return true;
}


bool BlockDvbTal::Upward::initModcodSimu(void)
{
	tal_id_t gw_id = 0;

	if(OpenSandConf::gw_table.find(this->mac_id) != OpenSandConf::gw_table.end())
	{
		gw_id = OpenSandConf::gw_table[this->mac_id];
	}
	else if(!Conf::getValue(Conf::section_map[GW_TABLE_SECTION],
		                    DEFAULT_GW, gw_id))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "couldn't find gw for tal %d",
		    this->mac_id);
		return false;
	}

	if(!this->initModcodDefFile(MODCOD_DEF_S2,
	                            &this->s2_modcod_def))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to initialize the down/forward MODCOD definition file\n");
		return false;
	}

	if(this->is_scpc)
	{
		if(!this->initModcodDefFile(MODCOD_DEF_S2,
		                            &this->s2_modcod_def))
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "failed to initialize the up/return MODCOD definition file\n");
			return false;
		}
	}

	return true;
}


bool BlockDvbTal::Upward::initOutput(void)
{
	this->probe_st_received_modcod = Output::registerProbe<int>("Down_Forward_modcod.Received_modcod",
	                                                            "modcod index",
	                                                            true, SAMPLE_LAST);
	this->probe_st_rejected_modcod = Output::registerProbe<int>("Down_Forward_modcod.Rejected_modcod",
	                                                            "modcod index",
	                                                            true, SAMPLE_LAST);
	this->probe_sof_interval = Output::registerProbe<float>("Perf.SOF_interval",
	                                                        "ms", true,
	                                                        SAMPLE_LAST);

	this->probe_st_l2_from_sat =
		Output::registerProbe<int>("Throughputs.L2_from_SAT",
		                           "Kbits/s", true, SAMPLE_AVG);
	this->l2_from_sat_bytes = 0;
	return true;
}


bool BlockDvbTal::Upward::onRcvDvbFrame(DvbFrame *dvb_frame)
{
	uint8_t msg_type = dvb_frame->getMessageType();
	bool corrupted = dvb_frame->isCorrupted();

	switch(msg_type)
	{
		case MSG_TYPE_BBFRAME:
		{
			if(this->state != state_running)
			{
				LOG(this->log_receive, LEVEL_NOTICE,
				    "Ignore received BBFrames while not logged\n");
				delete dvb_frame;
				return true;
			}

			NetBurst *burst = NULL;
			DvbS2Std *std = (DvbS2Std *)this->reception_std;

			// get ACM parameters that will be transmited to GW in SAC
			double cni = dvb_frame->getCn();
			this->setRequiredCniInput(this->tal_id, cni);

			// Update stats
			this->l2_from_sat_bytes += dvb_frame->getMessageLength();
			this->l2_from_sat_bytes -= sizeof(T_DVB_HDR);

			// Set the real modcod of the ST
			std->setRealModcod(this->getCurrentModcodIdInput(this->tal_id));

			if(!std->onRcvFrame(dvb_frame,
			                    this->tal_id,
			                    &burst))
			{
				LOG(this->log_receive, LEVEL_ERROR,
				    "failed to handle the reception of "
				    "BB frame (len = %u)\n",
				    dvb_frame->getMessageLength());
				goto error;
			}

			NetBurst::const_iterator it;
			if(burst)
			{
				for(it = burst->begin(); it != burst->end(); it++)
				{
					const NetPacket *packet = (*it);
					if(packet->getDstTalId() == this->tal_id && this->is_scpc)
					{
						uint32_t opaque = 0;
						if(!this->pkt_hdl->getHeaderExtensions(packet,
						                                       "deencodeCniExt",
						                                       &opaque))
						{
							LOG(this->log_receive, LEVEL_ERROR,
							    "error when trying to read header extensions\n");
							goto error;
						}
						if(opaque != 0)
						{
							// This is the C/N0 value evaluated by the GW and transmitted
							// via GSE extensions
							this->setRequiredCniOutput(this->gw_id, ncntoh(opaque));
							break;
						}
					}
				}
			}

			if(!corrupted)
			{
				// update MODCOD probes
				this->probe_st_received_modcod->put(std->getReceivedModcod());
				this->probe_st_rejected_modcod->put(0);
			}
			else
			{
				this->probe_st_rejected_modcod->put(std->getReceivedModcod());
				this->probe_st_received_modcod->put(0);
			}

			// send the message to the upper layer
			if(burst && !this->enqueueMessage((void **)&burst))
			{
				LOG(this->log_send, LEVEL_ERROR,
				    "failed to send burst of packets to upper layer\n");
				delete burst;
				goto error;
			}
			LOG(this->log_send, LEVEL_INFO,
			    "burst sent to the upper layer\n");
			break;
		}

		// Start of frame (SOF):
		// treat only if state is running --> otherwise just ignore (other
		// STs can be logged)
		case MSG_TYPE_SOF:
			this->updateStats();
			// get superframe number
			if(!this->onStartOfFrame(dvb_frame))
			{
				delete dvb_frame;
				LOG(this->log_receive, LEVEL_ERROR,
				    "on start of frame failed\n");
				goto error;
			}
			// continue here
		case MSG_TYPE_TTP:
			const char *state_descr;

			if(this->state == state_running)
				state_descr = "state_running";
			else if(this->state == state_initializing)
				state_descr = "state_initializing";
			else
				state_descr = "other";

			LOG(this->log_receive, LEVEL_INFO,
			    "SF#%u: received SOF or TTP in state %s\n",
			    this->super_frame_counter, state_descr);

			if(this->state == state_running)
			{
				// get ACM parameters that will be transmited to GW in SAC
				double cni = dvb_frame->getCn();
				this->setRequiredCniInput(this->tal_id, cni);

				if(!this->shareFrame(dvb_frame))
				{
					LOG(this->log_receive, LEVEL_ERROR,
					    "Unable to transmit TTP to opposite channel\n");
					goto error;
				}
			}
			else
			{
				delete dvb_frame;
			}
			break;

		case MSG_TYPE_SESSION_LOGON_RESP:
			if(!this->onRcvLogonResp(dvb_frame))
			{
				LOG(this->log_receive, LEVEL_ERROR,
				    "on receive logon resp failed\n");
				goto error;
			}
			break;

		// messages sent by current or another ST for the NCC --> ignore
		case MSG_TYPE_SAC:
		case MSG_TYPE_SESSION_LOGON_REQ:
			delete dvb_frame;
			break;

		case MSG_TYPE_SALOHA_CTRL:
			if(!this->shareFrame(dvb_frame))
			{
				LOG(this->log_receive, LEVEL_ERROR,
				    "Unable to transmit Slotted Aloha Control frame "
				    "to opposite channel\n");
				goto error;
			}
			break;

		default:
			LOG(this->log_receive, LEVEL_ERROR,
			    "SF#%u: unknown type of DVB frame (%u), ignore\n",
			    this->super_frame_counter,
			    dvb_frame->getMessageType());
			delete dvb_frame;
			goto error;
	}
	return true;

error:
	LOG(this->log_receive, LEVEL_ERROR,
	    "Treatments failed at SF#%u",
	    this->super_frame_counter);
	return false;
}

bool BlockDvbTal::Upward::shareFrame(DvbFrame *frame)
{
	if(!this->shareMessage((void **)&frame, sizeof(frame), msg_sig))
	{
		LOG(this->log_receive, LEVEL_ERROR,
		    "Unable to transmit frame to opposite channel\n");
		delete frame;
		return false;
	}
	return true;
}


bool BlockDvbTal::Upward::onStartOfFrame(DvbFrame *dvb_frame)
{
	uint16_t sfn; // the superframe number piggybacked by SOF packet
	// TODO Sof *sof = dynamic_cast<Sof *>(dvb_frame);
	Sof *sof = (Sof *)dvb_frame;

	sfn = sof->getSuperFrameNumber();

	// update the frame numerotation
	this->super_frame_counter = sfn;

	return true;
}


bool BlockDvbTal::Upward::onRcvLogonResp(DvbFrame *dvb_frame)
{
	T_LINK_UP *link_is_up;
	LogonResponse *logon_resp = (LogonResponse *)(dvb_frame);
	// Retrieve the Logon Response frame
	if(logon_resp->getMac() != this->mac_id)
	{
		LOG(this->log_receive, LEVEL_INFO,
		    "SF#%u Loggon_resp for mac=%d, not %d\n",
		    this->super_frame_counter, logon_resp->getMac(),
		    this->mac_id);
		delete dvb_frame;
		return true;
	}

	// Remember the id
	this->group_id = logon_resp->getGroupId();
	this->tal_id = logon_resp->getLogonId();

	if(!this->shareFrame(dvb_frame))
	{
		LOG(this->log_receive, LEVEL_ERROR,
		    "Unable to transmit LogonResponse to opposite channel\n");
	}

	// Send a link is up message to upper layer
	// link_is_up
	link_is_up = new T_LINK_UP;
	if(link_is_up == 0)
	{
		LOG(this->log_receive, LEVEL_ERROR,
		    "SF#%u Memory allocation error on link_is_up\n",
		    this->super_frame_counter);
		return false;
	}
	link_is_up->group_id = this->group_id;
	link_is_up->tal_id = this->tal_id;

	if(!this->enqueueMessage((void **)(&link_is_up),
	                         sizeof(T_LINK_UP),
	                         msg_link_up))
	{
		LOG(this->log_receive, LEVEL_ERROR,
		    "SF#%u: failed to send link up message to upper layer",
		    this->super_frame_counter);
		delete link_is_up;
		return false;
	}
	LOG(this->log_receive, LEVEL_DEBUG,
	    "SF#%u Link is up msg sent to upper layer\n",
	    this->super_frame_counter);

	// Set the state to "running"
	this->state = state_running;
	LOG(this->log_receive, LEVEL_NOTICE,
	    "SF#%u: logon succeeded, running as group %u and logon"
	    " %u\n", this->super_frame_counter,
	    this->group_id, this->tal_id);

	// Add the st id
	if(!this->addInputTerminal(this->tal_id, this->s2_modcod_def))
	{
		LOG(this->log_receive_channel, LEVEL_ERROR,
		    "failed to handle FMT for ST %u, "
		    "won't send logon response\n", this->tal_id);
		return false;
	}

	return true;
}


void BlockDvbTal::Upward::updateStats(void)
{
	if(!this->doSendStats())
	{
		return;
	}

	this->probe_st_l2_from_sat->put(
		this->l2_from_sat_bytes * 8 / this->stats_period_ms);
	this->l2_from_sat_bytes = 0;
	// send all probes
	// in upward because this block has less events to handle => more time
	Output::sendProbes();

	// reset stat context for next frame
}


