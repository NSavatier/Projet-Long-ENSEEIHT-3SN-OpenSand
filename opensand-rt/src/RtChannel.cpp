/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2019 TAS
 *
 *
 * This file is part of the OpenSAND testbed.
 *
 *
 * OpenSAND is free software : you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY, without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see http://www.gnu.org/licenses/.
 *
 */

/**
 * @file RtChannel.cpp
 * @author Cyrille GAILLARDET / <cgaillardet@toulouse.viveris.com>
 * @author Julien BERNARD / <jbernard@toulouse.viveris.com>
 * @author Aurelien DELRIEU / <adelrieu@toulouse.viveris.com>
 * @brief  The channel included in blocks
 *
 */

#include "RtChannel.h"

#include "Rt.h"

#include <opensand_output/Output.h>

#include <errno.h>
#include <cstring>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <stdarg.h>
#ifdef TIME_REPORTS
	#include <numeric>
	#include <algorithm>
#endif

#define SIG_STRUCT_SIZE 128

using std::ostringstream;


// TODO pointer on onEventUp/Down
RtChannel::RtChannel(const string &name, const string &type):
	log_init(NULL),
	log_rt(NULL),
	log_receive(NULL),
	log_send(NULL),
	channel_name(name),
	channel_type(type),
	block_initialized(false),
	previous_fifo(NULL),
	in_opp_fifo(NULL),
	max_input_fd(-1),
	stop_fd(-1),
	w_sel_break(-1),
	r_sel_break(-1)
{
	FD_ZERO(&(this->input_fd_set));
}

RtChannel::~RtChannel()
{
	// delete all events
	this->updateEvents(); // update to also clear new events
	for(map<event_id_t, RtEvent *>::iterator iter = this->events.begin();
	    iter != this->events.end(); ++iter)
	{
		if((*iter).second == NULL)
		{
			continue;
		}
		delete((*iter).second);
	}
	this->events.clear();
	delete this->in_opp_fifo;
	if(this->previous_fifo)
	{
		delete this->previous_fifo;
	}
	close(this->w_sel_break);
	close(this->r_sel_break);
#ifdef TIME_REPORTS
	this->getDurationsStatistics();
#endif
}

bool RtChannel::enqueueMessage(void **data, size_t size, uint8_t type)
{
	return this->pushMessage(this->next_fifo, data, size, type);
}

bool RtChannel::shareMessage(void **data, size_t size, uint8_t type)
{
	return this->pushMessage(this->out_opp_fifo, data, size, type);
}

bool RtChannel::init(void)
{
	sigset_t signal_mask;
	int32_t pipefd[2];

	// Output Log
	this->log_rt = Output::registerLog(LEVEL_WARNING, "%s.%s.rt",
	                                   this->channel_name.c_str(),
	                                   this->channel_type.c_str());
	this->log_init = Output::registerLog(LEVEL_WARNING, "%s.%s.init",
	                                     this->channel_name.c_str(),
	                                     this->channel_type.c_str());
	this->log_receive = Output::registerLog(LEVEL_WARNING, "%s.%s.receive",
	                                        this->channel_name.c_str(),
	                                        this->channel_type.c_str());
	this->log_send = Output::registerLog(LEVEL_WARNING, "%s.%s.send",
	                                     this->channel_name.c_str(),
	                                     this->channel_type.c_str());

	LOG(this->log_init, LEVEL_INFO,
	    "Starting initialization\n");

	// pipe used to break select when a new event is received
	if(pipe(pipefd) != 0)
	{
		this->reportError(true,
		                  "cannot initialize pipe\n");
		return false;
	}
	this->r_sel_break = pipefd[0];
	this->w_sel_break = pipefd[1];
	this->addInputFd(this->r_sel_break);

	// create the signal mask for stop (highest priority)
	sigemptyset(&signal_mask);
	sigaddset(&signal_mask, SIGINT);
	sigaddset(&signal_mask, SIGQUIT);
	sigaddset(&signal_mask, SIGTERM);
	this->stop_fd = this->addSignalEvent("stop", signal_mask, 0);

	// initialize fifos and create associated messages
	if(!this->in_opp_fifo->init())
	{
		this->reportError(true,
		                  "cannot initialize opposite fifo\n");
		return false;
	}
	if(this->addMessageEvent(this->in_opp_fifo, 4, true) < 0)
	{
		this->reportError(true,
		                  "cannot create opposite message event\n");
		return false;
	}
	if(this->previous_fifo)
	{
		if(!this->previous_fifo->init())
		{
			this->reportError(true,
			                  "cannot initialize previous fifo\n");
			return false;
		}
		if(this->addMessageEvent(this->previous_fifo) < 0)
		{
			this->reportError(true,
			                  "cannot create previous message event\n");
			return false;
		}
	}

	return true;
}

void RtChannel::setIsBlockInitialized(bool initialized)
{
	this->block_initialized = initialized;
}

int32_t RtChannel::addTimerEvent(const string &name,
                                 double duration_ms,
                                 bool auto_rearm,
                                 bool start,
                                 uint8_t priority)
{
	TimerEvent *event = new TimerEvent(name, duration_ms,
	                                   auto_rearm, start,
	                                   priority);
	if(!event)
	{
		this->reportError(true, "cannot create timer event\n");
		return -1;
	}
	if(!this->addEvent((RtEvent *)event))
	{
		return -1;
	}

	return event->getFd();
}

int32_t RtChannel::addTcpListenEvent(const string &name,
                                     int32_t fd,
                                     size_t max_size,
                                     uint8_t priority)
{
	TcpListenEvent *event = new TcpListenEvent(name,
	                                           fd,
	                                           max_size,
	                                           priority);
	if(!event)
	{
		this->reportError(true, "cannot create file event\n");
		return -1;
	}
	if(!this->addEvent((RtEvent *)event))
	{
		return -1;
	}

	return event->getFd();
}

int32_t RtChannel::addFileEvent(const string &name,
                                int32_t fd,
                                size_t max_size,
                                uint8_t priority)
{
	FileEvent *event = new FileEvent(name,
	                                 fd,
	                                 max_size,
	                                 priority);
	if(!event)
	{
		this->reportError(true, "cannot create file event\n");
		return -1;
	}
	if(!this->addEvent((RtEvent *)event))
	{
		return -1;
	}

	return event->getFd();
}

int32_t RtChannel::addNetSocketEvent(const string &name,
                                     int32_t fd,
                                     size_t max_size,
                                     uint8_t priority)
{
	NetSocketEvent *event = new NetSocketEvent(name,
	                                           fd,
	                                           max_size,
	                                           priority);
	if(!event)
	{
		this->reportError(true, "cannot create net socket event\n");
		return -1;
	}
	if(!this->addEvent((RtEvent *)event))
	{
		return -1;
	}

	return event->getFd();
}

int32_t RtChannel::addSignalEvent(const string &name,
                                  sigset_t signal_mask,
                                  uint8_t priority)
{
	SignalEvent *event = new SignalEvent(name, signal_mask, priority);
	if(!event)
	{
		this->reportError(true, "cannot create signal event\n");
		return -1;
	}
	if(!this->addEvent((RtEvent *)event))
	{
		return -1;
	}

	return event->getFd();
}

bool RtChannel::addMessageEvent(RtFifo *out_fifo,
                                uint8_t priority,
                                bool opposite)
{
	MessageEvent *event;
	
	string name = this->channel_type;
	std::transform(name.begin(), name.end(), name.begin(), ::tolower);
	
	if(opposite)
	{
		name += "_opposite";
	}
	event = new MessageEvent(out_fifo, name,
	                         out_fifo->getSigFd(),
	                         priority);
	if(!event)
	{
		this->reportError(true, "cannot create message event\n");
		return false;
	}
	if(!this->addEvent((RtEvent *)event))
	{
		return false;
	}

	return true;
}

bool RtChannel::addEvent(RtEvent *event)
{
	map<event_id_t, RtEvent *>::iterator it;
	it = this->events.find(event->getFd());
	if(it != this->events.end())
	{
		this->reportError(true, "duplicated fd\n");
		return false;
	}
	this->new_events.push_back(event);

	// break the select loop
	if(write(this->w_sel_break,
	         MAGIC_WORD,
	         strlen(MAGIC_WORD)) != strlen(MAGIC_WORD))
	{
		LOG(this->log_rt, LEVEL_ERROR,
		    "failed to break select upon a new "
		    "event reception\n");
	}

#ifdef TIME_REPORTS
	this->durations[event->getName()] = list<double>();
#endif

	return true;
}

void RtChannel::updateEvents(void)
{
	// add new events
	for(list<RtEvent *>::iterator iter = this->new_events.begin();
		iter != this->new_events.end(); ++iter)
	{
		LOG(this->log_rt, LEVEL_INFO,
		    "Add new event \"%s\" in list\n",
		    (*iter)->getName().c_str());
		this->addInputFd((*iter)->getFd());
		this->events[(*iter)->getFd()] = *iter;
	}
	this->new_events.clear();

	// remove old events
	for(list<event_id_t>::iterator iter = this->removed_events.begin();
		iter != this->removed_events.end(); ++iter)
	{
		map<event_id_t, RtEvent *>::iterator it;

		it = this->events.find(*iter);
		if(it != this->events.end())
		{
			LOG(this->log_rt, LEVEL_INFO,
			    "Remove event \"%s\" from list\n",
			    (*it).second->getName().c_str());
			// remove fd from set
			FD_CLR((*it).first, &(this->input_fd_set));
			if((*it).first == this->max_input_fd)
			{
				this->updateMaxFd();
			}
			// remove fd from map
			delete (*it).second;
			this->events.erase(it);
		}
	}
	this->removed_events.clear();
}

void RtChannel::updateMaxFd(void)
{
	this->max_input_fd = 0;
	// update the greater fd
	for(map<event_id_t, RtEvent *>::iterator iter = this->events.begin();
		iter != this->events.end(); ++iter)
	{
		if((*iter).first > this->max_input_fd)
		{
			this->max_input_fd = (*iter).first;
		}
	}

}

TimerEvent *RtChannel::getTimer(event_id_t id)
{
	map<event_id_t, RtEvent *>::iterator it;
	RtEvent *event = NULL;

	it = this->events.find(id);
	if(it == this->events.end())
	{
		LOG(this->log_rt, LEVEL_DEBUG,
		    "event not found, search in new events\n");
		bool found = false;
		// check in new events
		for(list<RtEvent *>::iterator iter = this->new_events.begin();
			iter != this->new_events.end(); ++iter)
		{
			if(*(*iter) == id)
			{
				LOG(this->log_rt, LEVEL_DEBUG,
				    "event found in new events\n");
				found = true;
				event = *iter;
				break;
			}
		}
		if(!found)
		{
			this->reportError(false, "cannot find timer\n");
			return NULL;
		}
	}
	else
	{
		LOG(this->log_rt, LEVEL_DEBUG,
		    "Timer found\n");
		event = (*it).second;
	}
	if(event && event->getType() != evt_timer)
	{
		this->reportError(false, "cannot start event that is not a timer\n");
		return NULL;
	}

	return (TimerEvent *)event;
}

bool RtChannel::startTimer(event_id_t id)
{
	TimerEvent *event = this->getTimer(id);
	if(!event)
	{
		this->reportError(false, "cannot find timer: should not happend here\n");
		return false;
	}

	event->start();

	return true;
}

bool RtChannel::setDuration(event_id_t id, double new_duration)
{
	TimerEvent *event = this->getTimer(id);
	if(!event)
	{
		this->reportError(false, "cannot find timer: should not happend here\n");
		return false;
	}

	event->setDuration(new_duration);

	return true;
}

bool RtChannel::raiseTimer(event_id_t id)
{
	TimerEvent *event = this->getTimer(id);
	if(!event)
	{
		this->reportError(false, "cannot find timer: should not happend here\n");
		return false;
	}

	event->raise();

	return true;
}

void RtChannel::addInputFd(int32_t fd)
{
	if(fd > this->max_input_fd)
	{
		this->max_input_fd = fd;
	}
	FD_SET(fd, &(this->input_fd_set));
}

void RtChannel::removeEvent(event_id_t id)
{
	this->removed_events.push_back(id);
}

void *RtChannel::startThread(void *pthis)
{
	((RtChannel *)pthis)->executeThread();

	return NULL;
}

void RtChannel::executeThread(void)
{
	int32_t number_fd;
	int32_t handled;
	fd_set readfds;

	list<RtEvent *> priority_sorted_events;

	while(true)
	{
		handled = 0;
		
		// get the new events for the next loop
		this->updateEvents();
		readfds = this->input_fd_set;

		// wait for any event
		// we need a timeout in order to refresh event list
		number_fd = select(this->max_input_fd + 1, &readfds, NULL, NULL, NULL);
		if(number_fd < 0)
		{
			this->reportError(true, "select failed: [%u: %s]\n", errno, strerror(errno));
		}
		// unfortunately, FD_ISSET is the only usable thing
		priority_sorted_events.clear();

		// check for select break
		if(FD_ISSET(this->r_sel_break, &readfds))
		{
			unsigned char data[strlen(MAGIC_WORD)];
			if(read(this->r_sel_break, data, strlen(MAGIC_WORD)) < 0)
			{
				LOG(this->log_rt, LEVEL_ERROR,
				    "failed to read in pipe");
			}
			handled++;
		}

		// handle each event
		for(map<event_id_t, RtEvent *>::iterator iter = this->events.begin();
			iter != this->events.end(); ++iter)
		{
			RtEvent *event = (*iter).second;
			if(handled >= number_fd)
			{
				// all events treated, no need to continue the loop
				break;
			}
			// if this event FD has raised
			if(!FD_ISSET(event->getFd(), &readfds))
			{
				continue;
			}
			handled++;

			// fd is set
			if(!event->handle())
			{
				if(event->getType() == evt_signal)
				{
					// this is the only case where it is critical as
					// stop event is a signal
					this->reportError(true, "unable to handle signal event\n");
					pthread_exit(NULL);
				}
				this->reportError(false, "unable to handle event\n");
				// ignore this event
				continue;
			}
			priority_sorted_events.push_back(event);
			if(*event == this->stop_fd)
			{
				// we have to stop
				LOG(this->log_rt, LEVEL_INFO,
				    "stop signal received\n");
				pthread_exit(NULL);
			}
		}
		// sort the list according to priority
		priority_sorted_events.sort(RtEvent::compareEvents);

		// call processEvent on each event
		for(list<RtEvent *>::iterator iter = priority_sorted_events.begin();
			iter != priority_sorted_events.end(); ++iter)
		{
			(*iter)->setTriggerTime();
			LOG(this->log_rt, LEVEL_DEBUG, "event received (%s)",
			    (*iter)->getName().c_str());
			if(!this->onEvent(*iter))
			{
				LOG(this->log_rt, LEVEL_ERROR,
				    "failed to process event %s\n",
				    (*iter)->getName().c_str());
			}
#ifdef TIME_REPORTS
			timeval time = (*iter)->getTimeFromTrigger();
			double val = time.tv_sec * 1000000L + time.tv_usec;
			this->durations[(*iter)->getName()].push_back(val);
#endif
		}
	}
}

void RtChannel::reportError(bool critical, const char *msg_format, ...)
{
	char msg[512];
	va_list args;
	va_start(args, msg_format);

	vsnprintf(msg, 512, msg_format, args);

	va_end(args);

	Rt::reportError(this->channel_name, pthread_self(),
	                critical, msg);
};

void RtChannel::setPreviousFifo(RtFifo *fifo)
{
	this->previous_fifo = fifo;
};

void RtChannel::setNextFifo(RtFifo *fifo)
{
	this->next_fifo = fifo;
};

void RtChannel::setOppositeFifo(RtFifo *in_fifo, RtFifo *out_fifo)
{
	this->in_opp_fifo = in_fifo;
	this->out_opp_fifo = out_fifo;
};

bool RtChannel::pushMessage(RtFifo *out_fifo, void **data, size_t size, uint8_t type)
{
	bool success = true;

	// check that block is initialized (i.e. we are in event processing)
	if(!this->block_initialized)
	{
		LOG(this->log_send, LEVEL_NOTICE,
		    "Be careful, some message are sent while process are not "
		    "started. If too many messages are sent we may block because "
		    "fifo is full\n");
		// FIXME we could separate onInit into a static initialization and an
		//       initialization when threads are started
	}

	if(!out_fifo->push(*data, size, type))
	{
		this->reportError(false,
		                  "cannot push data in fifo for next block\n");
		success = false;
	}

	// be sure that the pointer won't be used anymore
	*data = NULL;
	return success;
}

#ifdef TIME_REPORTS
void RtChannel::getDurationsStatistics(void) const
{
	OutputEvent *event = Output::registerEvent("Time Report");
	map<string, list<double> >::const_iterator it;
	for(it = this->durations.begin(); it != this->durations.end(); ++it)
	{
		list<double> duration = (*it).second;
		if(duration.empty())
		{
			continue;
		}
		double sum = std::accumulate(duration.begin(),
		                             duration.end(), 0.0);
		double max = *std::max_element(duration.begin(),
		                               duration.end());
		double min = *std::min_element(duration.begin(),
		                               duration.end());
		double mean = sum / duration.size();

		Output::sendEvent(event,
		                  "[%s:%s] Event %s: mean = %.2f us, max = %d us, "
		                  "min = %d us, total = %.2f ms\n",
		                  this->channel_name.c_str(),
		                  this->channel_type.c_str(),
		                  (*it).first.c_str(), mean, int(max), int(min), sum / 1000);
	}
}
#endif
