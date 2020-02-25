/**
 * @file InSimulationConfUpdateInterface.cpp
 * @brief Interface that allows for update of some parameters (Bandwidth so far) while the simulation is running,
 * Heavily inspired by the existing NccPepInterface
 * @author Nicolas Savatier <savatier.nicolas@gmail.com>
 */

#include "InSimulationConfUpdateInterface.h"

#include <opensand_output/Output.h>
#include <opensand_conf/conf.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>

#include <sstream>


/**
 * @brief Initialize the interface between NCC and ConfUpdate components
 */
InSimulationConfUpdateInterface::InSimulationConfUpdateInterface():
	NccInterface(),
	requests_list()
{
}


/**
 * @brief Destroy the interface between NCC and ConfUpdate components
 */
InSimulationConfUpdateInterface::~InSimulationConfUpdateInterface()
{
	std::vector<ConfUpdateRequest *>::iterator it;

	// free all ConfUpdate requests stored in list
	for(it = this->requests_list.begin();
	    it != this->requests_list.end();
		it = requests_list.erase(it));

    //TODO : see if this destructor call is useful
	//this->~NccInterface();

}


/**
 * @brief Get the TCP socket that listens for incoming ConfUpdate connections
 *
 * @return  the listen socket or -1 if not initialized
 */
int InSimulationConfUpdateInterface::getConfUpdateListenSocket()
{
	return this->getSocketListen();
}


/**
 * @brief Get the TCP socket connected to the the ConfUpdate component
 *
 * @return  the client socket or -1 if not connected
 */
int InSimulationConfUpdateInterface::getConfUpdateClientSocket()
{
	return this->getSocketClient();
}


/**
 * @brief Get the type of current ConfUpdate requests
 *
 * @return  PEP_REQUEST_ALLOCATION or PEP_REQUEST_RELEASE
 */
conf_update_request_type_t InSimulationConfUpdateInterface::getConfUpdateRequestType()
{
    conf_update_request_type_t type;

	if(this->requests_list.empty())
	{
		type = CONF_UPDATE_UNKNOWN;
	}
	else
	{
		type = this->requests_list.front()->getType();
	}

	return type;
}


/**
 * @brief Get the first request of the list of ConfUpdate requests
 *
 * @return  the first request of the list of ConfUpdate requests,
 *          NULL if no request is available
 */
ConfUpdateRequest * InSimulationConfUpdateInterface::getNextConfUpdateRequest()
{
	ConfUpdateRequest *request;
	std::vector<ConfUpdateRequest *>::iterator it;

	if(this->requests_list.empty())
	{
		request = NULL;
	}
	else
	{
		// get the first request of the list and make a copy of it.
		// the erase() method calls the object's destructor,
		// so we have to deference the request object in order to preserve it.
		it = this->requests_list.begin();
		request = *it;
		*it = NULL;

		// remove the first request of the list
		this->requests_list.erase(it);
	}

	return request;
}


/**
 * @brief Create a TCP socket that listens for incoming ConfUpdate connections
 *
 * @return  true on success, false on failure
 */
bool InSimulationConfUpdateInterface::initConfUpdateSocket()
{
	int tcp_port;

	// retrieve the TCP communication port dedicated
	// for NCC/ConfUpdate communications
    // TODO - Possibly -  move configuration reading in bloc
	if(!Conf::getValue(Conf::section_map["conf_update_interface"], "conf_update_port", tcp_port))//TODO for some reason , using SECTION_CONF_UPDATE & CONF_UPDATE_PORT do not work anymore despite beeing declared in conf.h ...
	{
		LOG(this->log_ncc_interface, LEVEL_NOTICE,
		    "section '%s': missing parameter '%s'\n",
            "conf_update_interface", "conf_update_port");//TODO for some reason , using SECTION_CONF_UPDATE & CONF_UPDATE_PORT do not work anymore despite beeing declared in conf.h ...
		return false;
	}

	if(tcp_port <= 0 && tcp_port >= 0xffff)
	{
		LOG(this->log_ncc_interface, LEVEL_ERROR,
		    "section '%s': bad value for parameter '%s'\n",
            "conf_update_interface", "conf_update_port");//TODO for some reason , using SECTION_CONF_UPDATE & CONF_UPDATE_PORT do not work anymore despite beeing declared in conf.h ...
		return false;
	}

	LOG(this->log_ncc_interface, LEVEL_NOTICE,
	    "TCP port to listen for ConfUpdate connections = %d\n", tcp_port);

	return this->initSocket(tcp_port);
}


bool InSimulationConfUpdateInterface::readConfUpdateMessage(NetSocketEvent *const event)
{ //TODO code a confirmer
	char *recv_buffer;

	// a ConfUpdate must be connected to read a message from it!
	if(!this->is_connected)
	{
		LOG(this->log_ncc_interface, LEVEL_ERROR,
		    "trying to read on ConfUpdate socket while no ConfUpdate "
		    "component is connected yet\n");
		goto error;
	}

	recv_buffer = (char *)(event->getData());

	// parse message received from ConfUpdate
	if(!this->parseConfUpdateMessage(recv_buffer))
	{
		// an error occured when parsing the ConfUpdate message
		LOG(this->log_ncc_interface, LEVEL_ERROR,
		    "failed to parse message received from PEP "
		    "component\n");
		goto close;
	}

	return true;

close:
	LOG(this->log_ncc_interface, LEVEL_ERROR,
	    "close ConfUpdate client socket because of previous errors\n");
	this->is_connected = false;
error:
	return false;
}


/**
 * @brief Parse a message sent by the ConfUpdate component
 *
 * A message contains one or more lines. Every line is a command. There are
 * allocation commands or release commands. All the commands in a message
 * must be of the same type.
 *
 * @param message   the message sent by the ConfUpdate component
 * @return          true if message was successfully parsed, false otherwise
 */
bool InSimulationConfUpdateInterface::parseConfUpdateMessage(const char *message)
{
	stringstream stream;
	char cmd[64];
	unsigned int nb_cmds;
	int all_cmds_type = -1; /* initialized because GCC is not smart enough
	                           to find that the variable can not be used
	                           uninitialized */

	// for every command in the message...
	nb_cmds = 0;
	stream << message;
	while(stream.getline(cmd, 64))
	{
		ConfUpdateRequest *request;

		// parse the command
		request = this->parseConfUpdateCommand(cmd);
		if(request == NULL)
		{
			LOG(this->log_ncc_interface, LEVEL_ERROR,
			    "failed to parse command #%d in ConfUpdate message, "
			    "skip the command\n", nb_cmds + 1);
			continue;
		}

		// check that all commands are of of the same type
		// (ie. all allocations or all de-allocations)
		if(nb_cmds == 0)
		{
			// first command, set the type
			all_cmds_type = request->getType();
		}
		else if(request->getType() != all_cmds_type)
		{
			LOG(this->log_ncc_interface, LEVEL_ERROR,
			    "command #%d is not of the same type "
			    "as command #1, this is not accepted, "
			    "so ignore the command\n", nb_cmds);
			delete request;
			continue;
		}

		// store the command parameters in context
		this->requests_list.push_back(request);

		nb_cmds++;
	}

	if(nb_cmds == 0)
	{
		// no request correctly processed
		return false;
	}

	return true;
}


/**
 * @brief Parse one of the commands sent in a message by the ConfUpdate component
 *
 * @param cmd       a command sent by the ConfUpdate component
 * @return          the created ConfUpdate request if command was successfully parsed,
 *                  NULL in case of failure
 */
ConfUpdateRequest * InSimulationConfUpdateInterface::parseConfUpdateCommand(const char *cmd)
{
	unsigned int spot_id;   // the ID of the spot the request is for
	unsigned int gateway_id;// the ID of the gateway the request is for
	unsigned int type;      // the type of the request (defines which operation must be performed)
	unsigned int bandwidth_new_value;  // the new value to be given to the bandwidth
	int ret;

	// retrieve values in the command
	ret = sscanf(cmd, "%u:%u:%u:%u", &spot_id, &gateway_id, &type, &bandwidth_new_value);
	if(ret != 4)
	{
		LOG(this->log_ncc_interface, LEVEL_ERROR,
		    "bad formated ConfUpdate command received: '%s'\n", cmd);
		return NULL;
	}
	else
	{
		LOG(this->log_ncc_interface, LEVEL_INFO,
		    "ConfUpdate command well received\n");
	}

	// check that the type was recovered correctly
	if(type != CONF_UPDATE_FORWARD_BANDWIDTH && type != CONF_UPDATE_RETURN_BANDWIDTH)
	{
		LOG(this->log_ncc_interface, LEVEL_ERROR,
		    "bad request type in ConfUpdate command '%s', "
		    "should be %u or %u\n", cmd,
            CONF_UPDATE_FORWARD_BANDWIDTH, CONF_UPDATE_RETURN_BANDWIDTH);
		return NULL;
	}
    //TODO : check if it is indeed Mbits/s and not kbits/s
	LOG(this->log_ncc_interface, LEVEL_INFO,
	    "Conf_Update %s received for spot ID = %u, "
	    "gateway ID = %u, new bandwidth = %u Mbits/s ",
	    ((type == CONF_UPDATE_FORWARD_BANDWIDTH) ? "Update Forward Link Bandwidth" : "Update Return Link Bandwidth"),
        spot_id, gateway_id, bandwidth_new_value);

	// build ConfUpdate request object
	return new ConfUpdateRequest(spot_id, gateway_id, (conf_update_request_type_t) type, bandwidth_new_value);
}
