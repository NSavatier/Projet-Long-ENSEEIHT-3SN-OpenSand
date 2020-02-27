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

    /**
    * Modifies, for the Forward Link, the bandwidth and symbolRate values contained in rootTag
    * in the /etc/opensand/core_global.conf configuration file.
    * @param spot id of the spot for which we want to modify the bandwidth in the configuration file
    * @param gw_id id of the gateway for which we want to modify the bandwidth in the configuration file
    * @param newValue new value of the bandwidth to be written
    * @return true on success, else false
    */
    bool modifyForwardBandwidthAndSymbolRateInGlobalConf(spot_id_t spot, tal_id_t gw_id, freq_mhz_t newValue);

    /**
	 * Modifies, for the Return Link, the bandwidth and symbolRate values contained in rootTag
     * in the /etc/opensand/core_global.conf configuration file.
	 * @param spot id of the spot for which we want to modify the bandwidth in the configuration file
	 * @param gw_id id of the gateway for which we want to modify the bandwidth in the configuration file
	 * @param newValue new value of the bandwidth to be written
	 * @return true on success, else false
	 */
    bool modifyReturnBandwidthAndSymbolRateInGlobalConf(spot_id_t spot, tal_id_t gw_id, freq_mhz_t newValue);

private:

    /**
	 * Modifies the bandwidth and symbolRate values contained in rootTag in the /etc/opensand/core_global.conf configuration file.
	 * This function parses the XML file looking for rootTag,
     * then parses its children, looking the "spot" tag with attributes "id" = spot and "gw" = gw_id
	 * if it finds said tag, it looks for the "bandwidth" tag and modifies its value to newValue.
     * Finally, the function looks for the "carriers_distribution" tag, and for each of its down_carriers children,
     * updates the symbol_rate attribute value to :
     *      symbol_rate_newVal = (newValueForBandwidth / oldValueForBandwidth) * old_symbol_rate_val;
     * This way, if the bandwidth is doubled, the symbol rate will be doubled too.
     *
     * If all these operations succeeded, the function returns true.
	 * If the tag is not found or any error occurs, the function returns false.
     *
     * Why modifying both bandwidth and symbol_rate values ?
     * Because simply modifying bandwidth caused the carriers number for each category to be modified, while our
     * """client"""/supervisor was interested in updating the carrier band-length,
     * while keeping the same number of carriers.
     * To achieve this behavior, we need to modify both bandwidth AND symbol_rate values.
     * For more info about this choice, see DvbChannel::initBand and DvbChannel::computeBandplan functions.
     *
     * Basically, this function factorizes the code in common between the two functions above (ie 99.9% of their code).
     *
	 * @param spot id of the spot for which we want to modify the bandwidth in the configuration file
	 * @param gw_id id of the gateway for which we want to modify the bandwidth in the configuration file
	 * @param newValue new value of the bandwidth to be written
	 * @return true on success, else false
	 */
    bool modifyBandwidthAndSymbolRateInGlobalConf(spot_id_t spot, tal_id_t gw_id, freq_mhz_t newValue, string rootTag);
};


#endif //OPENSAND_CONFUPDATEXMLPARSER_H
