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
 * @file BlockDvbSat.h
 * @brief This bloc implements a DVB-S/RCS stack for a Satellite
 * @author Didier Barvaux / Viveris Technologies
 * @author Emmanuelle Pechereau <epechereau@b2i-toulouse.com>
 * @author Julien Bernard <julien.bernard@toulouse.viveris.com>
 *
 * <pre>
 *
 *                  ^
 *                  | DVB Frame / BBFrame
 *                  v
 *           ------------------
 *          |                  |
 *          |  DVB-RCS Sat     |  <- Set carrier infos
 *          |                  |
 *           ------------------
 *
 * </pre>
 *
 */

#ifndef BLOC_DVB_SAT_REGEN_H
#define BLOC_DVB_SAT_REGEN_H


#include "BlockDvbSat.h"
#include "SatSpot.h"
#include "SatGw.h"
#include "PhysicStd.h" 

// output
#include <opensand_output/Output.h>

#include <linux/param.h>


class BlockDvbSatRegen: public BlockDvbSat
{

 public:

	BlockDvbSatRegen(const string &name);
	~BlockDvbSatRegen();

	// TODO move DvbFmt inheritance in SatGw
	class UpwardRegen: public Upward, public DvbFmt
	{
	 public:
		UpwardRegen(Block *const bl);
		~UpwardRegen();

		bool onInit();

	 private:
		/**
		 * Retrieves switching table entries
		 *
		 * @return  true on success, false otherwise
		 */
		bool initSwitchTable(void);
		
		/**
		* handle corrupted frame
		*
		* @param dvb_frame    the DVB or BB frame to forward
		* @return             true on success, false otherwise
		*/
		bool handleCorrupted(DvbFrame *dvb_frame);

		/**
		 * Handle Net Burst packet
		 * 
		 * @return true on success , false otherwise
		 */ 
		bool handleDvbBurst(DvbFrame *dvb_frame,
		                    SatGw *current_gw,
		                    SatSpot *current_spot);

		/**
		 * Handle Sac
		 * 
		 * @return true on success, false otherwise
		 */ 
		bool handleSac(DvbFrame *dvb_frame);
	
		/**
		 * Handle BB Frame
		 * 
		 * @return true on success, false otherwise
		 */ 
		bool handleBBFrame(DvbFrame *dvb_frame, 
		                   SatGw *current_gw,
		                   SatSpot *current_spot);
		/**
		 * Handle Saloha
		 *
		 * @return true on success, false otherwise
		 */ 
		bool handleSaloha(DvbFrame *dvb_frame, 
		                  SatGw *current_gw,
		                  SatSpot *current_spot);
	
	};

	// TODO move DvbFmt inheritance in SatGw
	class DownwardRegen: public Downward, public DvbFmt
	{
	 public:
		DownwardRegen(Block *const bl);
		~DownwardRegen();
		
		bool onInit();

	 private:
		/**
		 * @brief Initialize the link
		 *
		 * @return  true on success, false otherwise
		 */
		bool initSatLink(void);

		/**
		 * @brief Read configuration for the list of STs
		 *
		 * @return  true on success, false otherwise
		 */
		bool initStList(void);

		/**
		 * @brief Read configuration for the different timers
		 *
		 * @return  true on success, false otherwise
		 */
		bool initTimers(void);
		
		/**
		 * @brief Read configuration for the different files and open them
		 *
		 * @return  true on success, false otherwise
		 */
		bool initModcodSimu(void);

		/**
		 *
		 * @param packet    The NetPacket
		 * @return          true on success, false otherwise
		 */ 
		bool handleRcvEncapPacket(NetPacket *packet);

		/**
		 * @brief handle event message
		 *
		 * @return true on success, false otherwise
		 */ 
		bool handleMessageBurst(const RtEvent *const event);
		
		/**
		 * @briel handle event timer
		 *
		 * @return true on success, false otherwise
		 */ 
		bool handleTimerEvent(SatGw *current_gw,
		                      uint8_t spot_id);
		
		/**
		 * @ brief handle scenario event timer
		 *
		 * @return true on success, false otherwise
		 */ 
		bool handleScenarioTimer();
		
		/**
		 * Set the Fmt Simulation on the appropriate Spot and Gw
		 */
		void setFmtSimulation(spot_id_t spot_id, tal_id_t gw_id,
		                      FmtSimulation* new_fmt_simu);

		/**
		 * @brief Go to the first step in adaptive physical layer scenario
		 *        For the appropriate Spot and Gw.
		 *
		 * @param spot_id      the id of the spot
		 * @param gw_id        the id of the gw
		 * @return true on success, false otherwise
		 */
		bool goFirstScenarioStep(spot_id_t spot_id, tal_id_t gw_id);

		/**
		 * @brief Go to next step in adaptive physical layer scenario
		 *        Update current MODCODs IDs of all STs in the list
		 *        For the appropriate Spot and Gw.
		 *
		 * @param spot_id      the id of the spot
		 * @param gw_id        the id of the gw
		 * @param duration     duration before the next step
		 * @return true on success, false otherwise
		 */
		bool goNextScenarioStep(spot_id_t spot_id, tal_id_t gw_id,
		                        double &duration);

		/**
		 * Get a list of the gw ids
		 */
		set<tal_id_t> getGwIds(void);

		/**
		 * Get a list of the spot ids
		 */
		set<spot_id_t> getSpotIds(void);
	};


  protected:

	bool onDownwardEvent(const RtEvent *const event);
	bool onUpwardEvent(const RtEvent *const event);


};
#endif
