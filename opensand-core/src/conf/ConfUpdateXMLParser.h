//
// Created by nsavatie on 19/02/2020.
//

#ifndef OPENSAND_CONFUPDATEXMLPARSER_H
#define OPENSAND_CONFUPDATEXMLPARSER_H

#include <libxml++/libxml++.h>
#include "OpenSandCore.h"


//TODO maybe avoid coding it directly (infer it from OpenSAND path or something)
#define GLOBAL_CONF_FILE_PATH "/etc/opensand/core_global.conf"
#define CONF_PATH "/etc/opensand/"

/**
 * Simple class to modify entries in an OpenSAND Configuration File using #include libxml++
 */
class ConfUpdateXMLParser {


public:
	ConfUpdateXMLParser();

	~ConfUpdateXMLParser();

    bool modifyForwardBandwidthInGlobalConf(spot_id_t spot, tal_id_t gw_id, freq_mhz_t newValue);

    bool modifyReturnBandwidthInGlobalConf(spot_id_t spot, tal_id_t gw_id, freq_mhz_t newValue);

};


#endif //OPENSAND_CONFUPDATEXMLPARSER_H
