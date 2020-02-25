/**
 * @file ConfUpdateRequest.h
 * @brief In-simulation request of a specific simulation parameter
 * @author Nicolas Savatier <savatier.nicolas@gmail.com>
 */

#ifndef CONF_UPDATE_REQUEST_H
#define CONF_UPDATE_REQUEST_H

#include "OpenSandCore.h"

/**
 * @brief The different type of configuration update requests that may be sent
 */
typedef enum
{
    CONF_UPDATE_FORWARD_BANDWIDTH = 0,
    CONF_UPDATE_RETURN_BANDWIDTH = 1,
    CONF_UPDATE_UNKNOWN = 999
} conf_update_request_type_t;



/**
 * @class ConfUpdateRequest
 * @brief Allocation or release request from a PEP component
 */
class ConfUpdateRequest
{

 private:
    /** The spot concerned by the request */
    spot_id_t spot_id;

    /** The gateway concerned by the request */
    tal_id_t gateway_id; //TODO : voir si utile ? + verifier que le type est bien tal_id_t

    /** The type of ChangeConf request (update Forward bandwidth, Update Return Bandwidth) */
    conf_update_request_type_t type;

    /** New Value for the Bandwidth **/
    freq_mhz_t bandwidth_new_value;


	//TODO add request fields here if needed


 public:

    ConfUpdateRequest(spot_id_t spot_id,
                      tal_id_t gateway_id,
                      conf_update_request_type_t type,
                      freq_mhz_t bandwidth_new_value);

    ~ConfUpdateRequest();

    /**
     * @brief Get the id of the spot concerned by the request
     *
     * @return  the id of the spot concerned by the ConfUpdateRequest
     */
    spot_id_t getSpotId() const;

    /**
     * @brief Get the id of the gateway concerned by the request
     *
     * @return  the id of the gateway concerned by the ConfUpdateRequest
     */
    tal_id_t getGatewayId() const;

    /**
     * @brief Get the type of the configuration update request
     * (Forward Bandwidth Update or Return Bandwidth update)
     *
     * @return  the type of the configuration update request
     */
    conf_update_request_type_t getType() const;

    /**
     * @brief  Get the new value for the bandwidth requested by the request
     *
     * @return the new value for the bandwidth requested by the request
     */
    freq_mhz_t getBandwidthNewValue() const;


};

#endif
