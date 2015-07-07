/*
 *
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2014 TAS
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
 * @file SatSpot.h
 * @brief This bloc implements satellite spots
 * @author Didier Barvaux / Viveris Technologies
 * @author Emmanuelle Pechereau <epechereau@b2i-toulouse.com>
 * @author Julien Bernard <julien.bernard@toulouse.viveris.com>
 *
 */

#ifndef SAT_SPOT_H
#define SAT_SPOT_H

#include "SatGw.h"

#include <opensand_output/OutputLog.h>

#include <sys/times.h>
#include <map>
#include <list>


using std::list;
using std::map;


/**
 * @class SatSpot
 * @brief A DVB-RCS/S2 spot for the satellite emulator
 */
class SatSpot
{

 private:

	spot_id_t spot_id;            ///< Internal identifier of a spoti

	list<SatGw *> sat_gws;        /// List of gw
	
	// Output Log
	OutputLog *log_init;

 public:

	/**
	 * @brief Create spot
	 *
	 * @param spot_id            The spot id
	 * @param data_in_carrier_id The carrierid for incomming data
	 * @param log_id             The FIFO for logon packets
	 * @param ctrl_id            The FIFO for control frames
	 * @param data_out_st_id     The FIFO for outgoing terminal data
	 * @param data_out_gw_id     The FIFO for outgoing GW data
	 * @param fifo_size          The size of data FIFOs
	 */
	SatSpot(spot_id_t spot_id);
	~SatSpot();

	void addGw(SatGw *gw);
	
	/**
	 * @brief Get the spot ID
	 *
	 * @return the spot ID
	 */
	uint8_t getSpotId(void) const;

	/**
	 * @brief Set the Fmt Simulation for the appropriate Gw
	 *
	 * @param the gw id
	 * @param the new Fmt Simulation
	 */
	void setFmtSimulation(tal_id_t gw_id, FmtSimulation* new_fmt_simu);

	/**
	 * @brief Go to the first step in adaptive physical layer scenario
	 *        For the appropriate Gw.
	 *
	 * @param gw_id        the id of the gw
	 * @return true on success, false otherwise
	 */
	bool goFirstScenarioStep(tal_id_t gw_id);

	/**
	 * @brief Go to next step in adaptive physical layer scenario
	 *        Update current MODCODs IDs of all STs in the list
	 *        For the appropriate Gw.
	 *
	 * @param gw_id        the id of the gw
	 * @param need_advert  Whether this is a down/forward MODCOD that will need
	 *                     advertisment process
	 * @param duration     duration before the next step
	 * @return true on success, false otherwise
	 */
	bool goNextScenarioStep(tal_id_t gw_id, bool need_advert, double &duration);

	const list<SatGw *> getGwList(void) const;
	
	SatGw* getGw(tal_id_t gw_id);

	list<SatGw *> getListGw();        /// List of gw

	void print(void); /// For debug
};

/// The map of satellite spots
typedef map<spot_id_t, SatSpot *> sat_spots_t;

#endif