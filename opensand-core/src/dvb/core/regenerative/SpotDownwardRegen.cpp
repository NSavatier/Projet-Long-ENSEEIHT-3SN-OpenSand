/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2015 TAS
 * Copyright © 2015 CNES
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
 * @file SpotDownwardRegen.cpp
 * @brief Downward spot related functions for DVB NCC block
 * @author Bénédicte Motto <bmotto@toulouse.viveris.com>
 * @author Julien Bernard <julien.bernard@toulouse.viveris.com>
 *
 */

#include "SpotDownwardRegen.h"

#include "ForwardSchedulingS2.h"
#include "UplinkSchedulingRcs.h"
#include "DamaCtrlRcsLegacy.h"

#include <errno.h>


SpotDownwardRegen::SpotDownwardRegen(spot_id_t spot_id,
                                     tal_id_t mac_id,
                                     time_ms_t fwd_down_frame_duration,
                                     time_ms_t ret_up_frame_duration,
                                     time_ms_t stats_period,
                                     sat_type_t sat_type,
                                     EncapPlugin::EncapPacketHandler *pkt_hdl,
                                     bool phy_layer):
	SpotDownward(spot_id, mac_id, 
	             fwd_down_frame_duration, 
	             ret_up_frame_duration, 
	             stats_period, sat_type,
	             pkt_hdl, phy_layer)
{
}

SpotDownwardRegen::~SpotDownwardRegen()
{
}


bool SpotDownwardRegen::onInit(void)
{
	this->up_return_pkt_hdl = this->pkt_hdl;

	// Get the carrier Ids
	if(!this->initCarrierIds())
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed to complete the carrier IDs part of the "
		    "initialisation\n");
		goto error;
	}

	if(!this->initFifo())
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed to complete the FIFO part of the "
		    "initialisation\n");
		goto release_dama;
	}

	// Get and open the files
	if(!this->initModcodSimu())
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed to complete the files part of the "
		    "initialisation\n");
		return false;
	}
	
	if(!this->initMode())
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed to complete the mode part of the "
		    "initialisation\n");
		goto error;
	}

	// get and launch the dama algorithm
	if(!this->initDama())
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed to complete the DAMA part of the "
		    "initialisation\n");
		goto error;
	}

	this->initStatsTimer(this->fwd_down_frame_duration_ms);

	// initialize the column ID for FMT simulation
	if(!this->initColumns())
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed to initialize the columns ID for FMT "
		    "simulation\n");
		goto release_dama;
	}

	if(!this->initRequestSimulation())
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed to complete the request simulation part of "
		    "the initialisation\n");
		goto error;
	}

	if(!this->initOutput())
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed to complete the initialization of "
		    "statistics\n");
		goto release_dama;
	}
	// everything went fine
	return true;

release_dama:
	delete this->dama_ctrl;
error:
	return false;
}


bool SpotDownwardRegen::initMode(void)
{
	TerminalCategoryDama *cat;

	// get RETURN_UP_BAND section
	ConfigurationList return_up_band = Conf::section_map[RETURN_UP_BAND];
	ConfigurationList spots;
	ConfigurationList current_spot;
	ConfigurationList current_gw;

	// Get the spot list
	if(!Conf::getListNode(return_up_band, SPOT_LIST, spots))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "there is no %s into %s section\n",
		    SPOT_LIST, RETURN_UP_BAND);
		return false;
	}

	// get the spot which have the same id as SpotDownwardRegen
	if(!Conf::getElementWithAttributeValue(spots, ID,
	                                       this->spot_id, current_spot))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "there is no attribute %s with value: %d into %s/%s\n",
		    ID, this->spot_id, RETURN_UP_BAND, SPOT_LIST);
		return false;
	}

	if(!Conf::getElementWithAttributeValue(current_spot, GW,
	                                       this->mac_id, current_gw))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "there is no attribute %s with value: %d into %s/%s\n",
		    ID, this->spot_id, RETURN_UP_BAND, SPOT_LIST);
		return false;
	}
	if(!this->initBand<TerminalCategoryDama>(current_gw,
		                                     RETURN_UP_BAND,
	                                         DAMA,
	                                         this->ret_up_frame_duration_ms,
	                                         this->satellite_type,
	                                         this->up_ret_fmt_simu.getModcodDefinitions(),
	                                         this->categories,
	                                         this->terminal_affectation,
	                                         &this->default_category,
	                                         this->ret_fmt_groups))
	{
		return false;
	}

	// here we need the category to which the GW belongs
	if(this->terminal_affectation.find(this->mac_id) != this->terminal_affectation.end())
	{
		cat = this->terminal_affectation[this->mac_id];
	}
	else
	{
		if(!this->default_category)
		{
			LOG(this->log_init_channel, LEVEL_ERROR,
			    "No default category and GW has no affectation\n");
			return false;
		}
		cat = this->default_category;
	}
	this->scheduling = new UplinkSchedulingRcs(this->pkt_hdl,
	                                           this->dvb_fifos,
	                                           &this->up_ret_fmt_simu,
	                                           cat,
	                                           this->mac_id);
	
	if(!this->scheduling)
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed to create the scheduling\n");
		goto error;
	}

	return true;

error:
	return false;
}


// TODO this function is NCC part but other functions are related to GW,
//      we could maybe create two classes inside the block to keep them separated
bool SpotDownwardRegen::initDama(void)
{
	time_ms_t sync_period_ms;
	time_frame_t sync_period_frame;
	time_sf_t rbdc_timeout_sf;
	rate_kbps_t fca_kbps;
	string dama_algo;

	TerminalCategories<TerminalCategoryDama> dc_categories;
	TerminalMapping<TerminalCategoryDama> dc_terminal_affectation;
	TerminalCategoryDama *dc_default_category;

	// Retrieving the free capacity assignement parameter
	if(!Conf::getValue(Conf::section_map[DC_SECTION_NCC],
		               DC_FREE_CAP, fca_kbps))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "missing %s parameter\n", DC_FREE_CAP);
		goto error;
	}
	LOG(this->log_init_channel, LEVEL_NOTICE,
	    "fca = %d kb/s\n", fca_kbps);

	if(!Conf::getValue(Conf::section_map[COMMON_SECTION],
		               SYNC_PERIOD, sync_period_ms))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "Missing %s\n", SYNC_PERIOD);
		goto error;
	}
	sync_period_frame = (time_frame_t)round((double)sync_period_ms /
	                                        (double)this->ret_up_frame_duration_ms);
	rbdc_timeout_sf = sync_period_frame + 1;

	LOG(this->log_init_channel, LEVEL_NOTICE,
	    "rbdc_timeout = %d superframes computed from sync period %d superframes\n",
	    rbdc_timeout_sf, sync_period_frame);

	
	// band already initialized in initMode
	dc_categories = this->categories;
	dc_terminal_affectation = this->terminal_affectation;
	dc_default_category = this->default_category;

	// check if there is DAMA carriers
	if(dc_categories.size() == 0)
	{
		// No Slotted Aloha with regenerative satellite,
		// so we need a DAMA
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "No DAMA and regenerative satellite\n");
		return false;
	}

	// dama algorithm
	if(!Conf::getValue(Conf::section_map[DVB_NCC_SECTION],
		               DVB_NCC_DAMA_ALGO,
	                   dama_algo))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "section '%s': missing parameter '%s'\n",
		    DVB_NCC_SECTION, DVB_NCC_DAMA_ALGO);
		goto error;
	}

	/* select the specified DAMA algorithm */
	if(dama_algo == "Legacy")
	{
		LOG(this->log_init_channel, LEVEL_NOTICE,
		    "creating Legacy DAMA controller\n");
		this->dama_ctrl = new DamaCtrlRcsLegacy(this->spot_id);
	}
	else
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "section '%s': bad value for parameter '%s'\n",
		    DVB_NCC_SECTION, DVB_NCC_DAMA_ALGO);
		goto error;
	}

	if(!this->dama_ctrl)
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed to create the DAMA controller\n");
		goto error;
	}

	// Initialize the DamaCtrl parent class
	if(!this->dama_ctrl->initParent(this->ret_up_frame_duration_ms,
	                                this->with_phy_layer,
	                                this->up_return_pkt_hdl->getFixedLength(),
	                                rbdc_timeout_sf,
	                                fca_kbps,
	                                dc_categories,
	                                dc_terminal_affectation,
	                                dc_default_category,
	                                &this->up_ret_fmt_simu,
	                                (this->simulate == none_simu) ?
	                                false : true))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "Dama Controller Initialization failed.\n");
		goto release_dama;
	}

	if(!this->dama_ctrl->init())
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed to initialize the DAMA controller\n");
		goto release_dama;
	}
	this->dama_ctrl->setRecordFile(this->event_file);

	return true;

release_dama:
	delete this->dama_ctrl;
error:
	return false;
}



bool SpotDownwardRegen::initOutput(void)
{
	// Events
	this->event_logon_resp = Output::registerEvent("Spot_%d.DVB.logon_response",
	                                               this->spot_id);

	// Logs
	if(this->simulate != none_simu)
	{
		this->log_request_simulation = Output::registerLog(LEVEL_WARNING,
		                                                   "Spot_%d,Dvb.RequestSimulation",
		                                                   this->spot_id);
	}

	for(fifos_t::iterator it = this->dvb_fifos.begin();
	    it != this->dvb_fifos.end(); ++it)
	{
		const char *fifo_name = ((*it).second)->getName().data();
		unsigned int id = (*it).first;

		this->probe_gw_queue_size[id] =
			Output::registerProbe<int>("Packets", true, SAMPLE_LAST,
		                               "Spot_%d.Queue size.packets.%s",
		                               spot_id, fifo_name);
		this->probe_gw_queue_size_kb[id] =
			Output::registerProbe<int>("kbits", true, SAMPLE_LAST,
		                               "Spot_%d.Queue size.%s", spot_id, fifo_name);
		this->probe_gw_l2_to_sat_before_sched[id] =
			Output::registerProbe<int>("Kbits/s", true, SAMPLE_AVG,
		                               "Spot_%d.Throughputs.L2_to_SAT_before_sched.%s",
		                               spot_id, fifo_name);
		this->probe_gw_l2_to_sat_after_sched[id] =
			Output::registerProbe<int>("Kbits/s", true, SAMPLE_AVG,
		                               "Spot_%d.Throughputs.L2_to_SAT_after_sched.%s",
		                               spot_id, fifo_name);
		this->probe_gw_queue_loss[id] =
			Output::registerProbe<int>("Packets", true, SAMPLE_SUM,
		                               "Spot_%d.Queue loss.packets.%s",
		                               spot_id, fifo_name);
		this->probe_gw_queue_loss_kb[id] =
			Output::registerProbe<int>("Kbits/s", true, SAMPLE_SUM,
		                               "Spot_%d.Queue loss.%s",
		                               spot_id, fifo_name);
	}
	this->probe_gw_l2_to_sat_total =
		Output::registerProbe<int>("Kbits/s", true, SAMPLE_AVG,
		                           "Spot_%d.Throughputs.L2_to_SAT_after_sched.total",
		                           spot_id);

	this->probe_used_modcod = Output::registerProbe<int>("modcod index",
		                                                     true, SAMPLE_LAST,
		                                                     "Spot_%d.ACM.Used_modcod",
		                                                     this->spot_id);

	return true;
}


bool SpotDownwardRegen::handleSalohaAcks(const list<DvbFrame *> *ack_frames)
{
	list<DvbFrame *>::const_iterator ack_it;
	for(ack_it = ack_frames->begin(); ack_it != ack_frames->end();
	    ++ack_it)
	{
		this->complete_dvb_frames.push_back(*ack_it);
	}
	return true;
}

bool SpotDownwardRegen::handleCorruptedFrame(DvbFrame *dvb_frame)
{
	double curr_cni = dvb_frame->getCn();
	// regenerative case:
	//   we need downlink ACM parameters to inform
	//   satellite with a SAC so inform opposite channel
	this->cni = curr_cni;
	
	return true;
}

bool SpotDownwardRegen::handleSac(const DvbFrame *dvb_frame)
{
	Sac *sac = (Sac *)dvb_frame;

	LOG(this->log_receive_channel, LEVEL_DEBUG,
			"handle received SAC\n");

	if(!this->dama_ctrl->hereIsSAC(sac))
	{
		LOG(this->log_receive_channel, LEVEL_ERROR,
				"failed to handle SAC frame\n");
		delete dvb_frame;
		return false;
	}

	if(this->with_phy_layer)
	{
		// transparent : the C/N0 of forward link
		// regenerative : the C/N0 of uplink (updated by sat)
		double cni = sac->getCni();
		tal_id_t tal_id = sac->getTerminalId();
		this->up_ret_fmt_simu.setRequiredModcod(tal_id,
		                                        cni);
	}
	return true;
}

bool SpotDownwardRegen::handleFwdFrameTimer(time_sf_t fwd_frame_counter)
{
	uint32_t remaining_alloc_sym = 0;
	this->fwd_frame_counter = fwd_frame_counter;
	this->updateStatistics();

	// schedule encapsulation packets
	// TODO loop on categories (see todo in initMode)
	// TODO In regenerative mode we should schedule in frame_timer ??
	if(!this->scheduling->schedule(this->fwd_frame_counter,
	                               getCurrentTime(),
	                               &this->complete_dvb_frames,
	                               remaining_alloc_sym))
	{
		LOG(this->log_send_channel, LEVEL_ERROR,
		    "failed to schedule encapsulation "
		    "packets stored in DVB FIFO\n");
		return false;
	}

	if(this->complete_dvb_frames.size() > 0)
	{
		// we can do that because we have only one MODCOD per allocation
		// TODO THIS IS NOT TRUE ! we schedule for each carriers, if
		// desired modcod is low we can send on many carriers
		uint8_t modcod_id;
		modcod_id = ((DvbRcsFrame *)this->complete_dvb_frames.front())->getModcodId();
		this->probe_used_modcod->put(modcod_id);
	}
	LOG(this->log_receive_channel, LEVEL_INFO,
	    "SF#%u: %u symbols remaining after "
	    "scheduling\n", this->super_frame_counter,
	    remaining_alloc_sym);

	return true;
}

