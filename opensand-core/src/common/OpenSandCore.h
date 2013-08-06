/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2013 TAS
 * Copyright © 2013 CNES
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
 * @file OpenSandCore.h
 * @brief Some OpenSAND core utilities
 */

#ifndef OPENSAND_CORE_H
#define OPENSAND_CORE_H


#include <string>
#include <cfloat>
#include <algorithm>
#include <cmath>
#include <stdint.h>
#include <vector>

using std::string;
using std::vector;

/** unused macro to avoid compilation warning with unused parameters. */
#ifdef __GNUC__
#  define UNUSED(x) x __attribute__((unused))
#elif __LCLINT__
#  define UNUSED(x) /*@unused@*/ x
#else               /* !__GNUC__ && !__LCLINT__ */
#  define UNUSED(x) x
#endif              /* !__GNUC__ && !__LCLINT__ */

/// Broadcast tal id is maximal tal_id value authorized (5 bits).
#define BROADCAST_TAL_ID 0x1F
/// Terminal ID for Gateway
#define GW_TAL_ID (0L)

/** The different types of DVB components */
typedef enum
{
	satellite,
	gateway,
	terminal
} component_t;


/** @brief Get the name of a component
 *
 * @param host The component type
 * @return the abbreviated name of the component
 */
inline string getComponentName(component_t host)
{
	switch(host)
	{
		case satellite:
			return "sat";
			break;
		case gateway:
			return "gw";
			break;
		case terminal:
			return "st";
			break;
		default:
			return "";
	}
};

typedef enum
{
	REGENERATIVE,
	TRANSPARENT,
} sat_type_t;

/**
 * @brief get the satellite type according to its name
 *
 * @param type the satellite type name
 *
 * @return the satellite type enum
 */
inline sat_type_t strToSatType(string sat_type)
{
	if(sat_type == "regenerative")
		return REGENERATIVE;
	else
		return TRANSPARENT;
}

/** Compare two floats */
inline bool equals(double val1, double val2)
{
	return std::abs(val1 - val2) < DBL_EPSILON;
};


/**
 * @brief  Tokenize a string
 *
 * @param  str        The string to tokenize.
 * @param  tokens     The list to add tokens into.
 * @param  delimiter  The tokens' delimiter.
 */
inline void tokenize(const string &str,
                     vector<string> &tokens,
                     const string& delimiters=":")
{
	// Skip delimiters at beginning.
	string::size_type last_pos = str.find_first_not_of(delimiters, 0);
	// Find first "non-delimiter".
	string::size_type pos = str.find_first_of(delimiters, last_pos);

	while(string::npos != pos || string::npos != last_pos)
	{
		// Found a token, add it to the vector.
		tokens.push_back(str.substr(last_pos, pos - last_pos));
		// Skip delimiters.  Note the "not_of"
		last_pos = str.find_first_not_of(delimiters, pos);
		// Find next "non-delimiter"
		pos = str.find_first_of(delimiters, last_pos);
	}
}

// The types used in OpenSAND

// addressing
typedef uint16_t tal_id_t; ///< Terminal ID (5 bits but 16 needed for simulated terminal)
typedef uint8_t spot_id_t; ///< Spot ID (5 bits)
typedef uint8_t qos_t; ///< QoS (3 bits)
typedef uint8_t group_id_t; ///< Groupe ID

// TODO check types according to max value
// data
typedef uint16_t rate_kbps_t; ///< Bitrate in kb/s (suffix kbps)
typedef uint16_t rate_pktpf_t; ///< Rate in packets per frame (suffix pktpf)
typedef double rate_symps_t;    ///< Rate in symbols per second (bauds) (suffix symps)

// time
typedef uint16_t time_sf_t; ///< time in number of superframes (suffix sf)
typedef uint8_t time_frame_t; ///< time in number of frames (5 bits) (suffix frame)
typedef uint32_t time_ms_t; ///< time in ms (suffix ms)
typedef uint16_t time_pkt_t; ///< time in number of packets, cells, ... (suffix pkt)

// volume
typedef uint16_t vol_pkt_t; ///< volume in number of packets/cells (suffix pkt)
typedef uint16_t vol_kb_t; ///< volume in kbits (suffix kb)
typedef uint32_t vol_b_t; ///< volume in bits (suffix b)
typedef uint32_t vol_bytes_t; ///< volume in Bytes (suffix bytes)
typedef uint32_t vol_sym_t; ///< volume in number of symbols (suffix sym)

// frequency
typedef uint8_t freq_mhz_t; ///< frequency (MHz)
typedef uint16_t freq_khz_t; ///< frequency (kHz)

/**
 * @brief Generic Superframe description
 *
 *  freq
 *  ^
 *  |
 *  | +--------------+
 *  | |  f   |       |
 *  | |---+--|  sf   | sf_id
 *  | | f |f |       |
 *  | |--------------+
 *  | |   |  sf   |  | sf_id
 *  | +--------------+
 *  |
 *  +-----------------------> time
 *
 *  with sf = superframe and f = frame
 */

/**
 * @brief Superframe for DVB-RCS in OpenSAND
 *
 *  freq
 *  ^
 *  | frame duration (default: 53ms)
 *  | <-->
 *  | +---------------+
 *  | | f | f |  sf   | sf_id
 *  | |---------------+
 *  | |  sf   |  sf   | sf_id
 *  | +---------------+
 *  |
 *  +-----------------------> time
 */


#endif

