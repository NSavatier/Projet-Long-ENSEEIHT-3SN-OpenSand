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
 * @file BlockDvb.h
 * @brief This bloc implements a DVB-S2/RCS stack.
 * @author SatIP6
 * @author Didier Barvaux / Viveris Technologies
 * @author Emmanuelle Pechereau <epechereau@b2i-toulouse.com>
 * @author Julien Bernard <julien.bernard@toulouse.viveris.com>
 *
 *
 * <pre>
 *
 *            ^
 *            | encap burst
 *            v
 *    ------------------
 *   |                  |
 *   |       DVB        |
 *   |       Dama       |
 *   |                  |
 *    ------------------
 *            ^
 *            | DVB Frame / BBFrame
 *            v
 *
 * </pre>
 *
 */

#ifndef BLOCK_DVB_H
#define BLOCK_DVB_H

#include "PhysicStd.h"
#include "NccPepInterface.h"
#include "TerminalCategory.h"
#include "BBFrame.h"
#include "Sac.h"
#include "Ttp.h"

#include <opensand_output/Output.h>
#include <opensand_rt/Rt.h>
#include <opensand_conf/conf.h>

class BlockDvbSat;
class BlockDvbNcc;
class BlockDvbTal;


class DvbChannel: public RtChannel
{
 public:
	DvbChannel(Block *const bl, chan_type_t chan):
		RtChannel(bl, chan),
		satellite_type(),
		with_phy_layer(false),
		super_frame_counter(0),
		frames_per_superframe(-1),
		frame_counter(0),
		frame_duration_ms(),
		pkt_hdl(NULL),
		fmt_simu(),
		stats_period_ms(),
		stats_timer(-1)
	{
	};

 protected:

	/**
	 * @brief Read the satellite type
	 *
	 * @return true on success, false otherwise
	 */
	bool initSatType(void);

	/**
	 * @brief Read the encapsulation shcemes to get packet handler
	 *
	 * @param encap_schemes The section in configuration file for encapsulation
	 *                      schemes (up/return or down/forward)
	 * @param pkt_hdl       The packet handler corresponding to the encapsulation scheme
	 * @return true on success, false otherwise
	 */
	bool initPktHdl(const char *encap_schemes,
	                EncapPlugin::EncapPacketHandler **pkt_hdl);


	/**
	 * @brief Read the common configuration parameters
	 *
	 * @param encap_schemes The section in configuration file for encapsulation
	 *                      schemes (up/return or down/forward)
	 * @return true on success, false otherwise
	 */
	bool initCommon(const char *encap_schemes);

	/**
	 * @brief Read configuration for the MODCOD definition/simulation files
	 *
	 * @param def     The section in configuration file for MODCOD definitions
	 *                (up/return or down/forward)
	 * @param simu    The section in configuration file for MODCOD simulation
	 *                (up/return or down/forward)
	 * @return  true on success, false otherwise
	 */
	bool initModcodFiles(const char *def, const char *simu);

	/**
	 * @brief Read configuration for link MODCOD definition/simulation files
	 *
	 * @param def       The section in configuration file for MODCOD definitions
	 *                  (up/return or down/forward)
	 * @param simu      The section in configuration file for MODCOD simulation
	 *                  (up/return or down/forward)
	 * @param fmt_simu  The FMT simulation attribute to initialize
	 * @return  true on success, false otherwise
	 */
	bool initModcodFiles(const char *def, const char *simu,
	                     FmtSimulation &fmt_simu);

	/**
	 * @brief init the band according to configuration
	 *
	 * @tparam  T The type of terminal category to create
	 * @param   band                 The section in configuration file
	 *                               (up/return or down/forward)
	 * @param   access_type          The access type value
	 * @param   duration_ms          The frame duration on this band
	 * @param   fmt_def              The FMT definition table
	 * @param   categories           OUT: The terminal categories
	 * @param   terminal_affectation OUT: The terminal affectation in categories
	 * @param   default_category     OUT: The default category if terminal is not
	 *                                  in terminal affectation
	 * @param   fmt_groups           OUT: The groups of FMT ids
	 * @return true on success, false otherwise
	 */
	template<class T>
	bool initBand(const char *band,
	              access_type_t access_type,
	              time_ms_t duration_ms,
	              const FmtDefinitionTable *fmt_def,
	              TerminalCategories<T> &categories,
	              TerminalMapping<T> &terminal_affectation,
	              T **default_category,
	              fmt_groups_t &fmt_groups);

	/**
	 * @brief  Compute the bandplan.
	 *
	 * Compute available carrier frequency for each carriers group in each
	 * category, according to the current number of users in these groups.
	 *
	 * @tparam  T The type of terminal category to create
	 * @param   available_bandplan_khz  available bandplan (in kHz).
	 * @param   roll_off                roll-off factor
	 * @param   duration_ms             The frame duration on this band
	 * @param   categories              pointer to category list.
	 *
	 * @return  true on success, false otherwise.
	 */
	template<class T>
	bool computeBandplan(freq_khz_t available_bandplan_khz,
	                     double roll_off,
	                     time_ms_t duration_ms,
	                     TerminalCategories<T> &categories);

	/**
	 * Receive Push data in FIFO
	 *
	 * @param fifo          The FIFO to put data in
	 * @param data          The data
	 * @param fifo_delay    The minimum delay the data must stay in the
	 *                      FIFO (used on SAT to emulate delay)
	 * @return              true on success, false otherwise
	 */
	bool pushInFifo(DvbFifo *fifo,
	                NetContainer *data,
	                time_ms_t fifo_delay);

	/// the satellite type (regenerative o transparent)
	sat_type_t satellite_type;

	/// Physical layer enable
	bool with_phy_layer;

	/// the current super frame number
	time_sf_t super_frame_counter;

	/// the number of frame per superframe
	unsigned int frames_per_superframe;

	/// the current frame number inside the current super frame
	time_frame_t frame_counter; // from 1 to frames_per_superframe

	/// the frame duration
	time_ms_t frame_duration_ms;

	/// The encapsulation packet handler
	EncapPlugin::EncapPacketHandler *pkt_hdl;

	/// The MODCOD simulation elements
	FmtSimulation fmt_simu;

	/// The statistics period
	time_ms_t stats_period_ms;

	/// Statistics timer
	event_id_t stats_timer;
};


class BlockDvb: public Block
{
 public:

	/**
	 * @brief DVB block constructor
	 *
	 */
	BlockDvb(const string &name):
		Block(name)
	{
		// register static logs
		BBFrame::bbframe_log = Output::registerLog(LEVEL_WARNING, "Dvb.Net.BBFrame");
		Sac::sac_log = Output::registerLog(LEVEL_WARNING, "Dvb.SAC");
		Ttp::ttp_log = Output::registerLog(LEVEL_WARNING, "Dvb.TTP");
	};


	~BlockDvb();

	class DvbUpward: public DvbChannel
	{
	 public:
		DvbUpward(Block *const bl):
			DvbChannel(bl, upward_chan),
			receptionStd(NULL)
		{};


		~DvbUpward();

	 protected:
		/// reception standard (DVB-RCS or DVB-S2)
		PhysicStd *receptionStd;
	};

	class DvbDownward: public DvbChannel
	{
	 public:
		DvbDownward(Block *const bl):
			DvbChannel(bl, downward_chan),
			fwd_timer_ms(),
			dvb_scenario_refresh(-1)
		{};


	 protected:
		/**
		 * @brief Read the common configuration parameters for downward channels
		 *
		 * @return true on success, false otherwise
		 */
		bool initDown(void);

		/**
		 * Receive Packet from upper layer
		 *
		 * @param packet        The encapsulation packet received
		 * @param fifo          The MAC FIFO to put the packet in
		 * @param fifo_delay    The minimum delay the packet must stay in the
		 *                      MAC FIFO (used on SAT to emulate delay)
		 * @return              true on success, false otherwise
		 */
		bool onRcvEncapPacket(NetPacket *packet,
		                      DvbFifo *fifo,
		                      time_ms_t fifo_delay);

		/**
		 * Send the complete DVB frames created
		 * by \ref DvbRcsStd::scheduleEncapPackets or
		 * \ref DvbRcsDamaAgent::globalSchedule for Terminal
		 *
		 * @param complete_frames the list of complete DVB frames
		 * @param carrier_id      the ID of the carrier where to send the frames
		 * @return true on success, false otherwise
		 */
		bool sendBursts(list<DvbFrame *> *complete_frames,
		                uint8_t carrier_id);

		/**
		 * @brief Send message to lower layer with the given DVB frame
		 *
		 * @param frame       the DVB frame to put in the message
		 * @param carrier_id  the carrier ID used to send the message
		 * @return            true on success, false otherwise
		 */
		bool sendDvbFrame(DvbFrame *frame, uint8_t carrier_id);

		/**
		 * Update the statistics
		 */
		virtual void updateStats(void) = 0;

	 protected:

		/// the frame duration
		time_ms_t fwd_timer_ms;

		/// the scenario refresh interval
		time_ms_t dvb_scenario_refresh;
	};
};

/**
 * @brief Get integer values separated by ',' or ';' or a space
 *        in a string.
 *        Used for temporal division in VCM carriers
 *
 * @param values  The value to split
 * @return The vector containing the splitted values
 */
inline vector<unsigned int> tempSplit(string values)
{
	vector<string>::iterator it;
	vector<string> first_step;
	vector<unsigned int> output;

	// first get groups of strings separated by ';'
	tokenize(values, first_step, ";");
	for(it = first_step.begin(); it != first_step.end(); ++it)
	{
		vector<string> second_step;
		vector<string>::iterator it2;

		// then get groups of strings separated by ','
		tokenize(*it, second_step, ",");
		for(it2 = second_step.begin(); it2 != second_step.end(); ++it2)
		{
			vector<string> third_step;
			vector<string>::iterator it3;

			// then split the integers separated by '-'
			tokenize(*it2, third_step, "-");
			for(it3 = third_step.begin(); it3 != third_step.end(); ++it3)
			{
				stringstream str(*it3);
				unsigned int val;
				str >> val;
				if(str.fail())
				{
					continue;
				}
				output.push_back(val);
			}
		}
	}
	return output;
}

// Implementation of functions with templates

template<class T>
bool DvbChannel::initBand(const char *band,
                          access_type_t access_type,
                          time_ms_t duration_ms,
                          const FmtDefinitionTable *fmt_def,
                          TerminalCategories<T> &categories,
                          TerminalMapping<T> &terminal_affectation,
                          T **default_category,
                          fmt_groups_t &fmt_groups)
{
	freq_khz_t bandwidth_khz;
	double roll_off;
	freq_mhz_t bandwidth_mhz = 0;
	ConfigurationList conf_list;
	ConfigurationList aff_list;
	typename TerminalCategories<T>::iterator cat_iter;
	unsigned int carrier_id = 0;
	int i;
	string default_category_name;

	// Get the value of the bandwidth for return link
	if(!Conf::getValue(band, BANDWIDTH,
	                   bandwidth_mhz))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "section '%s': missing parameter '%s'\n",
		    band, BANDWIDTH);
		goto error;
	}
	bandwidth_khz = bandwidth_mhz * 1000;
	LOG(this->log_init, LEVEL_INFO,
	    "%s: bandwitdh is %u kHz\n", band, bandwidth_khz);

	// Get the value of the roll off
	if(!Conf::getValue(band, ROLL_OFF,
	                   roll_off))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "section '%s': missing parameter '%s'\n",
		    band, ROLL_OFF);
		goto error;
	}

	// get the FMT groups
	if(!Conf::getListItems(band,
	                       FMT_GROUP_LIST,
	                       conf_list))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Section %s, %s missing\n",
		    band, FMT_GROUP_LIST);
		goto error;
	}

	// create group list
	for(ConfigurationList::iterator iter = conf_list.begin();
	    iter != conf_list.end(); ++iter)
	{
		unsigned int group_id;
		string fmt_id;
		FmtGroup *group;

		// Get group id name
		if(!Conf::getAttributeValue(iter, GROUP_ID, group_id))
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "Section %s, problem retrieving %s in FMT "
			    "groups\n", band, GROUP_ID);
			goto error;
		}

		// Get FMT IDs
		if(!Conf::getAttributeValue(iter, FMT_ID, fmt_id))
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "Section %s, problem retrieving %s in FMT "
			    "groups\n", band, FMT_ID);
			goto error;
		}

		if(fmt_groups.find(group_id) != fmt_groups.end())
		{
			LOG(this->log_init, LEVEL_INFO,
			    "Section %s, FMT group %u already loaded\n", band,
			    group_id);
			continue;
		}
		group = new FmtGroup(group_id, fmt_id, fmt_def);
		fmt_groups[group_id] = group;
	}

	conf_list.clear();
	// get the carriers distribution
	if(!Conf::getListItems(band, CARRIERS_DISTRI_LIST, conf_list))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Section %s, %s missing\n", band,
		    CARRIERS_DISTRI_LIST);
		goto error;
	}

	i = 0;
	carrier_id = 0;
	// create terminal categories according to channel distribution
	for(ConfigurationList::iterator iter = conf_list.begin();
	    iter != conf_list.end(); ++iter)
	{
		string name;
		string ratio;
		vector<unsigned int> ratios;
		rate_symps_t symbol_rate_symps;
		string group_id;
		vector<unsigned int> group_ids;
		string access;
		unsigned int vcm_id = 0;
		T *category;

		i++;

		// Get carriers' name
		if(!Conf::getAttributeValue(iter, CATEGORY, name))
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "Section %s, problem retrieving %s in carriers "
			    "distribution table entry %u\n", band,
			    CATEGORY, i);
			goto error;
		}

		// Get carriers' ratio
		if(!Conf::getAttributeValue(iter, RATIO, ratio))
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "Section %s, problem retrieving %s in carriers "
			    "distribution table entry %u\n", band, RATIO, i);
			goto error;
		}
		// parse ratio if there is many values
		ratios = tempSplit(ratio);

		// Get carriers' symbol ratge
		if(!Conf::getAttributeValue(iter, SYMBOL_RATE, symbol_rate_symps))
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "Section %s, problem retrieving %s in carriers "
			    "distribution table entry %u\n", band,
			    SYMBOL_RATE, i);
			goto error;
		}

		// Get carriers' FMT id
		if(!Conf::getAttributeValue(iter, FMT_GROUP, group_id))
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "Section %s, problem retrieving %s in carriers "
			    "distribution table entry %u\n", band,
			    FMT_GROUP, i);
			goto error;
		}
		// parse group ids if there is many values
		group_ids = tempSplit(group_id);

		if(group_ids.size() != ratios.size())
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "There should be as many ratio values as fmt groups values\n");
			goto error;
		}

		// Get carriers' access type
		if(!Conf::getAttributeValue(iter, ACCESS_TYPE, access))
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "Section %s, problem retrieving %s in carriers "
			    "distribution table entry %u\n", band,
			    ACCESS_TYPE, i);
			goto error;
		}
		if(access != "VCM" &&
		   (group_ids.size() > 1 || ratios.size() > 1))
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "Too many FMT groups or ratio for non-VCM access type\n");
			goto error;
		}

		LOG(this->log_init, LEVEL_NOTICE,
		    "%s: new carriers: category=%s, Rs=%G, FMT group=%s, "
		    "ratio=%s, access type=%s\n", band, name.c_str(),
		    symbol_rate_symps, group_id.c_str(), ratio.c_str(),
		    access.c_str());

		for(vector<unsigned int>::iterator it = group_ids.begin();
		    it != group_ids.end(); ++it)
		{
			fmt_groups_t::const_iterator group_it;
			group_it = fmt_groups.find(*it);
			if(group_it == fmt_groups.end())
			{
				LOG(this->log_init, LEVEL_ERROR,
				    "Section %s, no entry for FMT group with ID %u\n",
				    band, (*it));
				goto error;
			}
			if(group_ids.size() > 1 && (*group_it).second->getFmtIds().size() > 1)
			{
				LOG(this->log_init, LEVEL_ERROR,
				    "For each VCM carriers, the FMT group should only "
				    "contain one FMT id\n");
				goto error;
			}

			// create the category if it does not exist
			// we also create categories with wrong access type because:
			//  - we may have many access types in the category
			//  - we need to get all carriers for band computation
			cat_iter = categories.find(name);
			category = dynamic_cast<T *>((*cat_iter).second);
			if(cat_iter == categories.end())
			{
				category = new T(name, access_type);
				categories[name] = category;
			}
			category->addCarriersGroup(carrier_id, (*group_it).second,
			                           ratios[vcm_id],
			                           symbol_rate_symps,
			                           strToAccessType(access));
			vcm_id++;
			// do not increment carrier_id here
		}
		carrier_id++;
	}

	// Compute bandplan
	if(!this->computeBandplan(bandwidth_khz, roll_off, duration_ms, categories))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Cannot compute band plan for %s\n", band);
		goto error;
	}

	// delete category with no carriers corresponding to the access type
	for(cat_iter = categories.begin(); cat_iter != categories.end(); ++cat_iter)
	{
		T *category = (*cat_iter).second;
		// getCarriersNumber returns the number of carriers with the desired
		// access type only
		if(!category->getCarriersNumber())
		{
			LOG(this->log_init, LEVEL_INFO,
			    "Skip category %s with no carriers with desired access type\n",
			    category->getLabel().c_str());
			categories.erase(cat_iter);
			delete category;
		}
	}

	if(categories.size() == 0)
	{
		// no more category here, this will be handled by caller
		return true;
	}

	// get the default terminal category
	if(!Conf::getValue(band, DEFAULT_AFF,
	                   default_category_name))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Section %s, missing %s parameter\n", band,
		    DEFAULT_AFF);
		goto error;
	}

	// Look for associated category
	*default_category = NULL;
	cat_iter = categories.find(default_category_name);
	if(cat_iter != categories.end())
	{
		*default_category = (*cat_iter).second;
	}
	if(*default_category == NULL)
	{
		LOG(this->log_init, LEVEL_NOTICE,
		    "Section %s, could not find category %s, "
		    "no default category for access type %u\n",
		    band, default_category_name.c_str(), access_type);
	}
	else
	{
		LOG(this->log_init, LEVEL_NOTICE,
		    "ST default category: %s in %s\n",
		    (*default_category)->getLabel().c_str(), band);
	}

	// get the terminal affectations
	if(!Conf::getListItems(band, TAL_AFF_LIST, aff_list))
	{
		LOG(this->log_init, LEVEL_NOTICE,
		    "Section %s, missing %s parameter\n", band,
		    TAL_AFF_LIST);
		goto error;
	}

	i = 0;
	for(ConfigurationList::iterator iter = aff_list.begin();
	    iter != aff_list.end(); ++iter)
	{
		// To prevent compilator to issue warning about non initialised variable
		tal_id_t tal_id = -1;
		string name;
		T *category;

		i++;
		if(!Conf::getAttributeValue(iter, TAL_ID, tal_id))
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "Section %s, problem retrieving %s in terminal "
			    "affection table entry %u\n", band, TAL_ID, i);
			goto error;
		}
		if(!Conf::getAttributeValue(iter, CATEGORY, name))
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "Section %s, problem retrieving %s in terminal "
			    "affection table entry %u\n", band, CATEGORY, i);
			goto error;
		}

		// Look for the category
		category = NULL;
		cat_iter = categories.find(name);
		if(cat_iter != categories.end())
		{
			category = (*cat_iter).second;
		}
		if(category == NULL)
		{
			LOG(this->log_init, LEVEL_NOTICE,
			    "Could not find category %s for terminal %u affectation, "
			    "it is maybe concerned by another access type",
			    name.c_str(), tal_id);
			// keep the NULL affectation for this terminal to avoid
			// setting default category
			terminal_affectation[tal_id] = NULL;
		}
		else
		{
			terminal_affectation[tal_id] = category;
			LOG(this->log_init, LEVEL_INFO,
			    "%s: terminal %u will be affected to category %s\n",
			    band, tal_id, name.c_str());
		}
	}

	return true;

error:
	return false;
}


template<class T>
bool DvbChannel::computeBandplan(freq_khz_t available_bandplan_khz,
                                 double roll_off,
                                 time_ms_t duration_ms,
                                 TerminalCategories<T> &categories)
{
	typename TerminalCategories<T>::const_iterator category_it;

	double weighted_sum_ksymps = 0.0;

	// compute weighted sum
	for(category_it = categories.begin();
	    category_it != categories.end();
	    ++category_it)
	{
		T *category = (*category_it).second;

		// Compute weighted sum in ks/s since available bandplan is in kHz.
		weighted_sum_ksymps += category->getWeightedSum();
	}

	LOG(this->log_init, LEVEL_DEBUG,
	    "Weigthed ratio sum: %f ksym/s\n", weighted_sum_ksymps);

	if(equals(weighted_sum_ksymps, 0.0))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Weighted ratio sum is 0\n");
		goto error;
	}

	// compute carrier number per category
	for(category_it = categories.begin();
	    category_it != categories.end();
		category_it++)
	{
		unsigned int carriers_number = 0;
		T *category = (*category_it).second;
		unsigned int ratio = category->getRatio();

		carriers_number = ceil(
		    (ratio / weighted_sum_ksymps) *
		    (available_bandplan_khz / (1 + roll_off)));
		LOG(this->log_init, LEVEL_NOTICE,
		    "Number of carriers for category %s: %d\n",
		    category->getLabel().c_str(), carriers_number);

		// set the carrier numbers and capacity in carrier groups
		category->updateCarriersGroups(carriers_number,
		                               duration_ms);
	}

	return true;
error:
	return false;
}



#endif
