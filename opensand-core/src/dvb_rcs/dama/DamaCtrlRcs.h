/*
 *
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

/*
 * @file DamaAgentRcs.h
 * @brief This library defines DAMA controller interfaces.
 * @author satip6 (Eddy Fromentin)
 */

#ifndef _DAMA_CONTROLLER_RCS_H_
#define _DAMA_CONTROLLER_RCS_H_

#include "DamaCtrl.h"

#include "FmtDefinitionTable.h"
#include "TerminalContextRcs.h"

#include <opensand_conf/conf.h>
#include <opensand_output/Output.h>

#include <stdio.h>
#include <math.h>
#include <map>
#include <vector>


/**
 * @class DamaCtrlRcs
 * @brief Define methods to process DAMA request in the NCC
 */
class DamaCtrlRcs: public DamaCtrl
{

 protected:

 	// output events and probes
	static Event *error_alloc;
	static Event *error_ncc_req;

	static Probe<int> *probe_gw_rdbc_req_num;
	static Probe<int> *probe_gw_rdbc_req_capacity;
	static Probe<int> *probe_gw_vdbc_req_num;
	static Probe<int> *probe_gw_vdbc_req_capacity;
	static Probe<int> *probe_gw_cra_alloc;
	static Probe<int> *probe_gw_cra_st_alloc;
	static Probe<int> *probe_gw_rbdc_alloc;
	static Probe<int> *probe_gw_rbdc_st_alloc;
	static Probe<int> *probe_gw_rbdc_max_alloc;
	static Probe<int> *probe_gw_rbdc_max_st_alloc;
 	static Probe<int> *probe_gw_vbdc_alloc;
	static Probe<int> *probe_gw_logger_st_num;

 public:

	DamaCtrlRcs();
	virtual ~DamaCtrlRcs();

	//bool applyPepCommand(const PepRequest &request);
	bool applyPepCommand(const PepRequest* request);


	/**
	 * @brief  Initializes internal data structure according to configuration file
	 *
	 * @return  true on success, false otherwise
	 */
	virtual bool init();

	virtual bool createTerminal(TerminalContext **terminal,
	                            tal_id_t tal_id,
	                            rate_kbps_t cra_kbps,
	                            rate_kbps_t max_rbdc_kbps,
	                            time_sf_t rbdc_timeout_sf,
	                            vol_pkt_t min_vbdc_pkt);

	virtual bool removeTerminal(TerminalContext *terminal);


	// Process DVB frames
	virtual bool hereIsCR(const CapacityRequest &capacity);

	// Build allocation table
	virtual bool buildTTP(Ttp &ttp);

	// TODO remove wrappers
	virtual int hereIsSACT(unsigned char *ip_buf, long i_len);

	// Reset dama
	virtual bool resetDama() = 0;

	// Update MODCOD for each terminal
	virtual void updateFmt();

};


#endif