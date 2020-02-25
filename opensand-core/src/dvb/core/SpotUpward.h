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
 * @file SpotUpward.h
 * @brief Upward spot related functions for DVB NCC block
 * @author Bénédicte Motto <bmotto@toulouse.viveris.com>
 * @author Julien Bernard <julien.bernard@toulouse.viveris.com>
 *
 */

#ifndef SPOT_UPWARD_H
#define SPOT_UPWARD_H

#include "DvbChannel.h"
#include "PhysicStd.h"  
#include "NetBurst.h"
#include "InSimulationConfUpdateInterface.h"

#define SIMU_BUFF_LEN 255

class SpotUpward: public DvbChannel, public DvbFmt
{
	public:
		SpotUpward(spot_id_t spot_id,
		           tal_id_t mac_id,
		           StFmtSimuList *input_sts,
		           StFmtSimuList *output_sts);

		virtual ~SpotUpward();

		/**
		 * @brief Spot Upward initialisation
		 *
		 * @return true on success, false otherwise
		 */ 
		virtual bool onInit();

		/**
		 * @brief ConfUpdate bandwidth update & reallocation
		 *
		 * @return true on success, false otherwise
		 */
		virtual bool confUpdateBandwidth();

		/**
		 * @brief Handle a DVB frame
		 *
		 * @param frame  The frame
		 * @param burst  OUT: the burst of packets
		 * @return true on success, false otherwise
		 */
		virtual bool handleFrame(DvbFrame *frame, NetBurst **burst) = 0;

		/**
		 * @brief get CNI in a frame
		 *
		 * @param dvb_frame the Dvb Frame corrupted
		 */
		virtual void handleFrameCni(DvbFrame *dvb_frame) = 0;

		/**
		 * @brief Schedule Slotted Aloha carriers
		 *
		 *	@param dvb_frame   a SoF
		 *  @param ack_frames  OUT: The generated ACK frames
		 *  @param sa_burst    OUT: The Slotted Aloha bursts received
		 *  @return true on success, false otherwise
		 */
		virtual bool scheduleSaloha(DvbFrame *dvb_frame,
		                            list<DvbFrame *>* &ack_frames,
		                            NetBurst **sa_burst);

		/**
		 *  @brief Handle a logon request transmitted by the lower layer
		 *
		 *  @param logon_req  The frame contining the logon request
		 *  @return true on success, false otherwise
		 */
		virtual bool onRcvLogonReq(DvbFrame *dvb_frame);

		/**
		 *  @brief Handle a Slotted Aloha Data Frame
		 *
		 *  @param frame  The Slotted Aloha data frame
		 *  @return true on success, false otherwise
		 */
		virtual bool handleSlottedAlohaFrame(DvbFrame *frame);

		// statistics update
		void updateStats(void);

		/**
		 * @brief  handle a SAC frame
		 *
		 * @param dvb_frame The SAC frame
		 * @return true on success, false otherwise
		 */
		bool handleSac(const DvbFrame *dvb_frame);

        /**
         * @briel apply ConfUpdate command
         * @param conf_update_request the ConfUpdate request
         * @return true on success, false otherwise
         */
        bool applyConfUpdateCommand(ConfUpdateRequest *conf_update_request);


		/**
		 * @brief  Getter to spot_id
		 *
		 * @return spot_id
		 */
		uint8_t getSpotId(void)
		{
			return this->spot_id;
		}

	protected:
		
		/**
		 * @brief Initialize the transmission mode
		 *
		 * @return  true on success, false otherwise
		 */
		virtual bool initMode(void) = 0;

		/**
		 * @brief Read configuration for the different files and open them
		 *
		 * @return  true on success, false otherwise
		 */
		virtual bool initModcodSimu(void) = 0;

		/**
		 * @brief Initialize the ACM loop margins
		 *        Called in GW SpotUpward only as it will initialize
		 *        StFmtSimuList that are shared
		 *
		 * @return  true on success, false otherwise
		 */
		virtual bool initAcmLoopMargin(void) = 0;

		/**
		 * @brief Initialize the statistics
		 *
		 * @return  true on success, false otherwise
		 */
		virtual bool initOutput(void) = 0;

		/// Spot Id
		uint8_t spot_id;

		/// Gw tal id
		uint8_t mac_id;

		/// reception standard (DVB-RCS or DVB-S2)
		PhysicStd *reception_std; 

		/// reception standard for SCPC
		PhysicStd *reception_std_scpc;

		/// The up/return packet handler for SCPC
		EncapPlugin::EncapPacketHandler *scpc_pkt_hdl;

		/// FMT groups for up/return
		fmt_groups_t ret_fmt_groups;
		
		// Output probes and stats
		// Rates
		// Layer 2 from SAT
		Probe<int> *probe_gw_l2_from_sat;
		int l2_from_sat_bytes;
		// Physical layer information
		Probe<int> *probe_received_modcod;
		Probe<int> *probe_rejected_modcod;

		/// log for slotted aloha
		OutputLog *log_saloha;

		/// logon request events
		OutputEvent *event_logon_req;
};

#endif
