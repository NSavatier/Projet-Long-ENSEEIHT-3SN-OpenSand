/**
 * @file ConfUpdateRequest.cpp
 * @brief In-simulation request of a specific simulation parameter
 * @author Nicolas Savatier <savatier.nicolas@gmail.com>
 */

#include "ConfUpdateRequest.h"


/**
 * @brief Build a new configuration update request
 */
ConfUpdateRequest::ConfUpdateRequest(spot_id_t spot_id,
                                     tal_id_t gateway_id,
                                     conf_update_request_type_t type,
                                     freq_mhz_t bandwidth_new_value):
        spot_id(spot_id),
        gateway_id(gateway_id),
        type(type),
        bandwidth_new_value(bandwidth_new_value)
{
}

/**
 * @brief Destroy the request
 */
ConfUpdateRequest::~ConfUpdateRequest()
{
	// nothing to do
}

/**
     * @brief Get the id of the spot concerned by the request
     *
     * @return  the id of the spot concerned by the ConfUpdateRequest
     */
spot_id_t ConfUpdateRequest::getSpotId() const
{
    return this->spot_id;
}

/**
 * @brief Get the id of the gateway concerned by the request
 *
 * @return  the id of the gateway concerned by the ConfUpdateRequest
 */
tal_id_t ConfUpdateRequest::getGatewayId() const
{
    return this->gateway_id;
}

/**
 * @brief Get the type of the configuration update request
 * (Forward Bandwidth Update or Return Bandwidth update)
 *
 * @return  the type of the configuration update request
 */
conf_update_request_type_t ConfUpdateRequest::getType() const
{
    return this->type;
}

/**
 * @brief  Get the new value for the bandwidth requested by the request
 *
 * @return the new value for the bandwidth requested by the request
 */
freq_mhz_t ConfUpdateRequest::getBandwidthNewValue() const
{
    return this->bandwidth_new_value;
}
