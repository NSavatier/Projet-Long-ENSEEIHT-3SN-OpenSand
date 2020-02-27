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
 * @file SpotUpward.cpp
 * @brief Upward spot related functions for DVB NCC block
 * @author Bénédicte Motto <bmotto@toulouse.viveris.com>
 * @author Julien Bernard <julien.bernard@toulouse.viveris.com>
 */


#include "SpotUpward.h"

#include "DamaCtrlRcsLegacy.h"


#include "Plugin.h"
#include <opensand_conf/Configuration.h>
#include "ConfUpdateXMLParser.h"

#include "DvbRcsStd.h"
#include "DvbS2Std.h"
#include "Sof.h"

SpotUpward::SpotUpward(spot_id_t spot_id,
                       tal_id_t mac_id,
                       StFmtSimuList *input_sts,
                       StFmtSimuList *output_sts):
    DvbChannel(),
    DvbFmt(),
	spot_id(spot_id),
	mac_id(mac_id),
	reception_std(NULL),
	reception_std_scpc(NULL),
	scpc_pkt_hdl(NULL),
	ret_fmt_groups(),
	probe_gw_l2_from_sat(NULL),
	probe_received_modcod(NULL),
	probe_rejected_modcod(NULL),
	log_saloha(NULL),
	event_logon_req(NULL)
{
	this->super_frame_counter = 0;
	this->input_sts = input_sts;
	this->output_sts = output_sts;
}

SpotUpward::~SpotUpward()
{
	// delete FMT groups here because they may be present in many carriers
	// TODO do something to avoid groups here
	for(fmt_groups_t::iterator it = this->ret_fmt_groups.begin();
	    it != this->ret_fmt_groups.end(); ++it)
	{
		delete (*it).second;
	}

	if(this->reception_std)
		delete this->reception_std;
	if(this->reception_std_scpc)
		delete this->reception_std_scpc;

}

bool SpotUpward::onInit(void)
{
	// Get and open the files
	if(!this->initModcodSimu())
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed to complete the files part of the "
		    "initialisation\n");
		return false;
	}

	if(!this->initAcmLoopMargin())
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed to complete the ACM loop margin  part of the initialisation\n");
		return false;
	}

	if(!this->initMode())
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed to complete the mode part of the "
		    "initialisation\n");
		return false;
	}

	// synchronized with SoF
	this->initStatsTimer(this->ret_up_frame_duration_ms);

	if(!this->initOutput())
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed to complete the initialization of "
		    "statistics\n");
		goto error_mode;
	}

	// everything went fine
	return true;

error_mode:
	delete this->reception_std;
	return false;

}

bool SpotUpward::confUpdateBandwidth(){
    //nothing to do in the general case
    LOG(this->log_init_channel, LEVEL_WARNING,
        "Nothing done in the general case for confUpdateBandwidth()\n");
    return true;
}


bool SpotUpward::onRcvLogonReq(DvbFrame *dvb_frame)
{
	//TODO find why dynamic cast fail here !?
//	LogonRequest *logon_req = dynamic_cast<LogonRequest *>(dvb_frame);
	LogonRequest *logon_req = (LogonRequest *)dvb_frame;
	uint16_t mac = logon_req->getMac();

	LOG(this->log_receive_channel, LEVEL_INFO,
	    "Logon request from ST%u on spot %u\n", mac, this->spot_id);

	// refuse to register a ST with same MAC ID as the NCC
	// or if it's a gw
	if(OpenSandConf::isGw(mac) or mac == this->mac_id)
	{
		LOG(this->log_receive_channel, LEVEL_ERROR,
		    "a ST wants to register with the MAC ID of the NCC "
		    "(%d), reject its request!\n", mac);
		delete dvb_frame;
		return false;
	}

	// send the corresponding event
	Output::sendEvent(this->event_logon_req,
	                  "Logon request received from ST%u on spot %u",
	                  mac, this->spot_id);

	return true;
}



void SpotUpward::updateStats(void)
{
	if(!this->doSendStats())
	{
		return;
	}
	this->probe_gw_l2_from_sat->put(
		this->l2_from_sat_bytes * 8.0 / this->stats_period_ms);
	this->l2_from_sat_bytes = 0;

	// Send probes
	Output::sendProbes();
}


// should only be called in Transparent mode
bool SpotUpward::scheduleSaloha(DvbFrame *UNUSED(dvb_frame),
                                list<DvbFrame *>* &UNUSED(ack_frames),
                                NetBurst **UNUSED(sa_burst))
{
	return true;
}

// should only be called in Transparent mode
bool SpotUpward::handleSlottedAlohaFrame(DvbFrame *UNUSED(frame))
{
	assert(0);
}

bool SpotUpward::handleSac(const DvbFrame *dvb_frame)
{
	Sac *sac = (Sac *)dvb_frame;

	// transparent : the C/N0 of forward link
	// regenerative : the C/N0 of uplink (updated by sat)
	double cni = sac->getCni();
	tal_id_t tal_id = sac->getTerminalId();
	this->setRequiredCniOutput(tal_id, cni);
	LOG(this->log_receive_channel, LEVEL_INFO,
	    "handle received SAC from terminal %u with cni %f\n",
	    tal_id, cni);
	
	return true;
}


bool SpotUpward::applyConfUpdateCommand(ConfUpdateRequest *conf_update_request){

    bool confUpdated;

    //apply command only in the case of a transparent SAT
    //and if the request asks for RETURN bandwidth modification
    if(this->satellite_type == TRANSPARENT){
        if(conf_update_request->getType() == CONF_UPDATE_RETURN_BANDWIDTH)
        {
            //update the BANDWIDTH entry in the configuration file for the specified Spot
            ConfUpdateXMLParser *parser = new ConfUpdateXMLParser();

            //parser->modifyForwardBandwidthAndSymbolRateInGlobalConf(conf_update_request->getSpotId(), conf_update_request->getGatewayId(), conf_update_request->getBandwidthNewValue());
            confUpdated = parser->modifyReturnBandwidthAndSymbolRateInGlobalConf(conf_update_request->getSpotId(), conf_update_request->getGatewayId(), conf_update_request->getBandwidthNewValue());
            if(!confUpdated){
                LOG(this->log_receive_channel, LEVEL_ERROR,
                    "Error during XML configuration file update in SpotUpward");
                return false;
            }
        } else if(conf_update_request->getType() == CONF_UPDATE_FORWARD_BANDWIDTH) {
            LOG(this->log_receive_channel, LEVEL_WARNING,
                "CONF_UPDATE_FORWARD_BANDWIDTH request received in SpotUpward, ignored");
            return true;
        } else {
            LOG(this->log_receive_channel, LEVEL_ERROR,
                "unknown ConfUpdate request type in SpotUpward");
            return false;
        }
    } else {
        LOG(this->log_receive_channel, LEVEL_WARNING,
            "SAT is Regenerative, nothing to do for ConfUpdate in SpotUpward");
        return true;
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
            LOG(this->log_receive_channel, LEVEL_CRITICAL,
                "SpotUpward : cannot reload configuration files\n");
            return false;
        } else {
            LOG(this->log_receive_channel, LEVEL_WARNING,
                "SpotUpward : configuration file reloaded with success\n");
        }
        OpenSandConf::loadConfig();

        // load the plugins
        if(!Plugin::loadPlugins(true, plugin_conf_path))
        {
            LOG(this->log_init_channel, LEVEL_CRITICAL,
                "SpotUpward : cannot load the plugins\n");
            return false;
        }
    }

    //Finally, recompute the bandwidth allocation
    if(!this->confUpdateBandwidth())//should normally call SpotUpwardTransp::initMode() which does the bandwidth modif
    {
        LOG(this->log_receive_channel, LEVEL_ERROR,
            "failed to reinit bandwidth allocation\n");
        return false;
    }


    return true;
}

