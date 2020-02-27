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
	 * Modifies the bandwidth value of the forwardLink in the /etc/opensand/core_global.conf configuration file.
	 * This function parses the XML file looking for the tag with attribute "id" = spot and "gw" = gw_id
	 * if it finds said tag, it odifies its value to newValue, and returns true.
	 * If the tag is not found or any error occurs, the function returns false.
	 * @param spot id of the spot for which we want to modify the bandwidth in the configuration file
	 * @param gw_id id of the gateway for which we want to modify the bandwidth in the configuration file
	 * @param newValue new value of the bandwidth to be written
	 * @return true on success, else false
	 */
    bool modifyForwardBandwidthInGlobalConf(spot_id_t spot, tal_id_t gw_id, freq_mhz_t newValue);

    /**
	 * Modifies the bandwidth value of the returnLink in the /etc/opensand/core_global.conf configuration file.
	 * This function parses the XML file looking for the tag with attribute "id" = spot and "gw" = gw_id
	 * if it finds said tag, it odifies its value to newValue, and returns true.
	 * If the tag is not found or any error occurs, the function returns false.
	 * @param spot id of the spot for which we want to modify the bandwidth in the configuration file
	 * @param gw_id id of the gateway for which we want to modify the bandwidth in the configuration file
	 * @param newValue new value of the bandwidth to be written
	 * @return true on success, else false
	 */
    bool modifyReturnBandwidthInGlobalConf(spot_id_t spot, tal_id_t gw_id, freq_mhz_t newValue);

private:

    /**
	 * Modifies the bandwidth value contained in rootTag in the /etc/opensand/core_global.conf configuration file.
	 * This function parses the XML file looking for rootTag,
     * then parses its children, looking the tag with attribute "id" = spot and "gw" = gw_id
	 * if it finds said tag, it odifies its value to newValue, and returns true.
	 * If the tag is not found or any error occurs, the function returns false.
     *
     * Basically, this function factorizes the code in common between the two functions above (ie 99.9% of their code).
     *
	 * @param spot id of the spot for which we want to modify the bandwidth in the configuration file
	 * @param gw_id id of the gateway for which we want to modify the bandwidth in the configuration file
	 * @param newValue new value of the bandwidth to be written
	 * @return true on success, else false
	 */
    bool modifyBandwidthInGlobalConf(spot_id_t spot, tal_id_t gw_id, freq_mhz_t newValue, string rootTag);
};


#endif //OPENSAND_CONFUPDATEXMLPARSER_H
