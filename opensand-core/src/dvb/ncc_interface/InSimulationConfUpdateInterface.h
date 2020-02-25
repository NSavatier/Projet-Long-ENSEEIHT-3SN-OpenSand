/**
 * @file InSimulationConfUpdateInterface.h
 * @brief Interface that allows for in-simulation update of some parameters (Bandwidth so far),
 * Heavily inspired by the existing NccPepInterface
 * @author Nicolas Savatier <savatier.nicolas@gmail.com>
 */

#ifndef NCC_CONF_UPDATE_ITF_H
#define NCC_CONF_UPDATE_ITF_H

#include "ConfUpdateRequest.h"
#include "NccInterface.h"
#include <vector>

#include <opensand_output/Output.h>
#include <opensand_rt/Rt.h>

/**
 * @class InSimulationConfUpdateInterface
 * @brief Interface between NCC and ConfUpdate components
 */
class InSimulationConfUpdateInterface: public NccInterface
{
 private:

	/** The list of commands received from the ConfUpdate component */
	std::vector<ConfUpdateRequest *> requests_list;

 public:

	/**** constructor/destructor ****/

	/* initialize the interface between NCC and ConfUpdate components */
	InSimulationConfUpdateInterface();

	/* destroy the interface between NCC and ConfUpdate components */
	~InSimulationConfUpdateInterface();


	/**** accessors ****/

	/* get the TCP socket that listens for incoming ConfUpdate connections */
	int getConfUpdateListenSocket();

	/* get the TCP socket connected to the the ConfUpdate component */
	int getConfUpdateClientSocket();

    /* Get the type of current ConfUpdate requests */
    conf_update_request_type_t getConfUpdateRequestType();

	/* get the list of ConfUpdate requests */
	ConfUpdateRequest * getNextConfUpdateRequest();




	/**** socket management ****/

	/* create a TCP socket that listens for incoming ConfUpdate connections */
	bool initConfUpdateSocket();

	/**
	 * @brief Read a set of commands to update the configuration
	 *
	 * @param event  The NetSocketEvent for InSimulationConfUpdate file descriptor
	 * @return  The status of the action:
	 *            \li true if command is read and parsed successfully
	 *            \li false if a problem is encountered
	 */
	bool readConfUpdateMessage(NetSocketEvent *const event);

 private:

	/* parse a message sent to update the configuration */
	bool parseConfUpdateMessage(const char *message);

	/* parse one of the commands sent in a message to update the configuration */
	ConfUpdateRequest * parseConfUpdateCommand(const char *cmd);
};

#endif
