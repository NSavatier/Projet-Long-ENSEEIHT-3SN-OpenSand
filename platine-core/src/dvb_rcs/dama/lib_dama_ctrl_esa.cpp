/**
 * @file lib_dama_ctrl_esa.cpp
 * @brief This library defines Esa DAMA controller
 *
 * @author ASP - IUSO, DTP (B. BAUDOIN)
 * @author Didier Barvaux / Viveris Technologies
 */

#include <math.h>
#include <string>
using namespace std;

#include "lib_dvb_rcs.h"
#include "lib_dama_ctrl_esa.h"

// environment plane
#include "platine_env_plane/EnvironmentAgent_e.h"
extern T_ENV_AGENT EnvAgent;

#define DBG_PACKAGE PKG_DAMA_DC
#include "platine_conf/uti_debug.h"
#define DC_DBG_PREFIX "[ESA]"


/**
 * Constructor
 */
DvbRcsDamaCtrlEsa::DvbRcsDamaCtrlEsa():
	DvbRcsDamaCtrl()
{
}


/**
 * Destructor
 */
DvbRcsDamaCtrlEsa::~DvbRcsDamaCtrlEsa()
{
}


/**
 * This function run the Dama
 *
 * We use internal ST contexts for doing that. After DAMA has been run
 * TBTP must have been completed and context must have been reinitialized
 *
 * @return  0 on success, -1 otherwise
 */
int DvbRcsDamaCtrlEsa::runDama()
{
	// Computed values
	int remaining_capacity;   // the total number of valid timeslot after an allocation step
	int total_capacity;       // the total number of atm cell to allocate
	int rbdc_request_number;  // the number of RBDC requests
	int vbdc_request_number;  // the number of VBDC requests
	int rbdc_request_sum;     // RBDC requests sum
	int vbdc_request_sum;     // VBDC requests sum
	int request;

	// Iterators
	DC_Context::iterator st;
	DC_St *ThisSt;  // points to the internal context map associated with the Id

	// The total allocated capacity is the registered total capacity minus
	// the sum of fixed RT bdwdth for registered ST
	total_capacity = m_total_capacity - m_total_cra;

	UTI_DEBUG("starting DAMA allocation: remaining capacity = %d "
	          "(total %ld - rt %ld)\n", total_capacity,
	          this->m_total_capacity, this->m_total_cra);

	// request statistics
	rbdc_request_number = 0;
	vbdc_request_number = 0;
	rbdc_request_sum = 0;
	vbdc_request_sum = 0;

	for(st = m_context.begin(); st != m_context.end(); st++)
	{
		ThisSt = st->second;
		if((request = ThisSt->GetVbdc()) != 0)
		{
			vbdc_request_number++;
			vbdc_request_sum += request;
		}
		if((request = ThisSt->GetRbdc()) != 0)
		{
			rbdc_request_number++;
			rbdc_request_sum += request;
		}

	}
	ENV_AGENT_Probe_PutInt(&EnvAgent,
	                       C_PROBE_GW_RBDC_REQUEST_NUMBER,
	                       0, rbdc_request_number);
	ENV_AGENT_Probe_PutInt(&EnvAgent,
	                       C_PROBE_GW_RBDC_REQUESTED_CAPACITY,
	                       0,
	                       (int) Converter->ConvertFromCellsPerFrameToKbits((double) rbdc_request_sum));
	ENV_AGENT_Probe_PutInt(&EnvAgent, C_PROBE_GW_VBDC_REQUEST_NUMBER, 0,
	                       vbdc_request_number);
	ENV_AGENT_Probe_PutInt(&EnvAgent,
	                       C_PROBE_GW_VBDC_REQUESTED_CAPACITY,
	                       0, vbdc_request_sum);

	// RBDC allocation
	remaining_capacity = runDamaRbdc(total_capacity);
	ENV_AGENT_Probe_PutInt(&EnvAgent,
	                       C_PROBE_GW_RBDC_ALLOCATION,
	                       0,
	                       (int) Converter->ConvertFromCellsPerFrameToKbits((double)
	                               total_capacity - remaining_capacity));
	total_capacity = remaining_capacity;

	// VBDC allocation
	remaining_capacity = runDamaVbdc(total_capacity);
	ENV_AGENT_Probe_PutInt(&EnvAgent,
	                       C_PROBE_GW_VBDC_ALLOCATION,
	                       0,
	                       (int) Converter->ConvertFromCellsPerFrameToKbits((double)
	                               total_capacity - remaining_capacity));
	total_capacity = remaining_capacity;

	// FCA allocation
	if(m_fca > 0)
		remaining_capacity = runDamaFca(total_capacity);
	else
		remaining_capacity = total_capacity;
	ENV_AGENT_Probe_PutInt(&EnvAgent,
	                       C_PROBE_GW_FCA_ALLOCATION,
	                       0,
	                       (int) Converter->ConvertFromCellsPerFrameToKbits((double)
	                               total_capacity - remaining_capacity));

	return 0;
}


/**
 * Perform the RBDC allocation
 * @param Tac is the total available capacity in slot number
 * @return Remaining total available capacity
 */
int DvbRcsDamaCtrlEsa::runDamaRbdc(int Tac)
{
	const char *FUNCNAME = DC_DBG_PREFIX "[runDamaRbdc]";

	int last_ptr, current_ptr;
	DC_St *ThisSt;					  // points to the internal context map associated with the Id
	DC_Context::iterator st;
	int Request, Alloc;
	int TotalRequest = 0;
	double FairShare;
	double RbdcNeed;
	double Credit;

	// some init
	last_ptr = -1;

	// capacity check
	if(Tac > 0)
	{
		UTI_DEBUG("%s starting. Remaining capacity=%d cells/frame.\n",
					 FUNCNAME, Tac);

		// initial loop
		for(st = m_context.begin(); st != m_context.end(); st++)
		{
			ThisSt = st->second;
			UTI_DEBUG_L3("ST%d: %d cells/frame\n", st->first,
			             ThisSt->GetRbdc());

			TotalRequest += ThisSt->GetRbdc();
		}

		if(TotalRequest > 0)
		{
			// Fair share calculation
			FairShare = (double) TotalRequest / (double) Tac;

			ENV_AGENT_Probe_PutFloat(&EnvAgent, C_PROBE_GW_UPLINK_FAIR_SHARE,
			                         0, FairShare);
			// if there is no congestion,
			// force the ratio to 1.0 in order to not limit the requests
			if(FairShare < 1.0)
				FairShare = 1.0;

			UTI_DEBUG("%s sum of all RBDC requests=%d cells/frame -> Fair share=%f.\n",
			          FUNCNAME, TotalRequest, FairShare);

			// first step : serve the integer part of the RBDC need
			for(st = m_context.begin(); st != m_context.end(); st++)
			{
				ThisSt = st->second;

				// apply the fair share coef to all request
				RbdcNeed = (double) ThisSt->GetRbdc() / FairShare;

				// allocate the integer part of the calculated need
				Request = (int) RbdcNeed;
				Alloc = ThisSt->SetAllocation(Request, DVB_CR_TYPE_RBDC);
				UTI_DEBUG_L3("%s st=%d RBDC alloc %d cells/frame (total %d).\n",
				             FUNCNAME, st->first, Request, Alloc);

				// decrease the total capacity
				Tac -= Request;

				if(FairShare > 1.0)
				{
					// add the decimal part of the RBDC need
					ThisSt->AddCredit(RbdcNeed - (double) Request);
				}
			}

			// second step : RBDC decimal part treatment
			if(FairShare > 1.0)
			{
				// retrieve the last not served ST
				if(m_rbdc_start_ptr != -1)
				{
					st = m_context.find(m_rbdc_start_ptr);
					if(st == m_context.end())
					{
						// the previous ST has requested a logoff. choose the first ST
						st = m_context.begin();
						if(st == m_context.end())
						{
							// there is no ST
							return (Tac);
						}
					}
				}
				else
				{
					// start from the beginning
					st = m_context.begin();
					if(st == m_context.end())
					{
						// there is no ST
						return (Tac);
					}
				}

				m_rbdc_start_ptr = st->first;
				current_ptr = st->first;
				ThisSt = st->second;

				while(Tac > 0)
				{
					Credit = ThisSt->GetCredit();
					UTI_DEBUG_L3("%s step 2 scanning st %d tac=%d credit=%f start_ptr=%d.\n",
					             FUNCNAME, current_ptr, Tac, Credit, m_rbdc_start_ptr);

					if(Credit > 1.0)
					{
						if(ThisSt->GetMaxAllocation() > 1)
						{
							// enough capacity to allocate
							ThisSt->SetAllocation(1, DVB_CR_TYPE_RBDC);
							ThisSt->AddCredit(-1.0);
							Tac--;
							UTI_DEBUG_L3("%s step 2 allocating 1 cell to st %d.\n",
							             FUNCNAME, current_ptr);
						}
						else
						{
							if(last_ptr == -1)
								last_ptr = current_ptr;
						}
					}
					ThisSt = RoundRobin(&current_ptr);

					/* stop looping on TTs if all TTs have been checked */
					if(current_ptr == m_rbdc_start_ptr)
						break;
				}

				if(last_ptr != -1)
					m_rbdc_start_ptr = last_ptr;
				else
					m_rbdc_start_ptr = current_ptr;
				UTI_DEBUG_L3("%s start ptr=%d last ptr=%d.\n", FUNCNAME,
				              m_rbdc_start_ptr, last_ptr);

			}
		}
		else
		{
			UTI_DEBUG("%s no RBDC request for this frame.\n", FUNCNAME);
		}
	}
	else
	{
		UTI_INFO("%s skipping. Not enough capacity....\n", FUNCNAME);
	}
	return (Tac);
}

/**
 * Perform the VBDC allocation
 * @param Tac is the total available capacity in slot number
 * @return Remaining total available capacity
 */
int DvbRcsDamaCtrlEsa::runDamaVbdc(int Tac)
{
	const char *FUNCNAME = DC_DBG_PREFIX "[runDamaVbdc]";

	int last_ptr, current_ptr;
	DC_St *ThisSt; // points to the internal context map associated with the Id
	DC_Context::iterator st;
	int Request;
	int Step;
	last_ptr = -1;


	// capacity check
	if(Tac > 0)
	{
		UTI_DEBUG("%s starting. Remaining capacity=%d cells/frame.\n",
		          FUNCNAME, Tac);

		// retrieve the last not served ST
		if(m_vbdc_start_ptr != -1)
		{
			st = m_context.find(m_vbdc_start_ptr);
			if(st == m_context.end())
			{
				// the previous ST has requested a logoff. choose the first ST
				st = m_context.begin();
				if(st == m_context.end())
				{
					// there is no ST
					return (Tac);
				}
			}
		}
		else
		{
			// start from the beginning
			st = m_context.begin();
			if(st == m_context.end())
			{
				// there is no ST
				return (Tac);
			}
		}

		// some init
		m_vbdc_start_ptr = st->first;
		current_ptr = st->first;
		ThisSt = st->second;

		// main loops
		for(Step = 0; Step < 2; Step++)
		{
			UTI_DEBUG("%s step %d starting\n", FUNCNAME, Step);
			while(Tac > 0)
			{
				Request = ThisSt->GetVbdc();

				if(Step == 0)
				{
					// The VBDC min part is treated first
					Request = MIN(Request, m_min_vbdc);
				}
				else
				{
					// The rest of the request is treated
					// Note that there is no need to decrease the VBDC min as long as it has already been allocated
					Request = MAX(Request, 0);
				}

				UTI_DEBUG_L3("%s step %d ST start %d initial %d tac %d request %d (on %d)\n",
				             FUNCNAME, Step, current_ptr, m_vbdc_start_ptr, Tac, Request,
				             ThisSt->GetVbdc());

				if(Request > 0)
				{
					if(Request <= ThisSt->GetMaxAllocation())
					{
						// enough capacity to allocate
						Tac -= Request;
						ThisSt->SetAllocation(Request, DVB_CR_TYPE_VBDC);
						UTI_DEBUG_L3("%s Allocation ST %d : %d\n", FUNCNAME,
						             current_ptr, Request);

					}
					else
					{
						// not enough capacity to allocate the complete request
						ThisSt->SetAllocation(ThisSt->GetMaxAllocation(),
						                      DVB_CR_TYPE_VBDC);
						UTI_DEBUG_L3("%s Partial allocation ST %d : %d<%d\n",
						             FUNCNAME, current_ptr,
						             ThisSt->GetMaxAllocation(), Request);

						if(last_ptr == -1)
							last_ptr = current_ptr;
					}
				}

				ThisSt = RoundRobin(&current_ptr);

				/* stop looping on TTs if all TTs have been checked */
				if(current_ptr == m_vbdc_start_ptr)
					break;
			}
		}
		if(last_ptr != -1)
			m_vbdc_start_ptr = last_ptr;
		else
			m_vbdc_start_ptr = current_ptr;
		//  m_vbdc_start_ptr = lastcurrent_ptr;
	}
	else
	{
		UTI_INFO("%s skipping. Not enough capacity....\n", FUNCNAME);
	}

	return (Tac);

}

/**
 * Perform the FCA allocation
 * @param Tac is the total available capacity in slot number
 * @return Remaining total available capacity
 */
int DvbRcsDamaCtrlEsa::runDamaFca(int Tac)
{
	const char *FUNCNAME = DC_DBG_PREFIX "[runDamaFca]";
	int last_ptr, current_ptr;
	DC_St *ThisSt; // points to the internal context map associated with the Id

	if(m_fca == 0)
	{
		UTI_INFO("%s no fca, skip.\n", FUNCNAME);
		return (Tac);
	}
	// capacity check
	if(Tac >= m_fca)
	{
		UTI_INFO("%s starting. Remaining capacity=%d cells/frame.\n",
		         FUNCNAME, Tac);

		// some init
		last_ptr = -1;
		current_ptr = m_fca_start_ptr;

		while(Tac >= m_fca)
		{
			ThisSt = RoundRobin(&current_ptr);
			if(ThisSt == (DC_St *) NULL)
				break;

			UTI_DEBUG_L3("%s scanning ST %d tac %d (last ptr %d)\n", FUNCNAME,
			             current_ptr, Tac, last_ptr);

			if(m_fca <= ThisSt->GetMaxAllocation())
			{
				// enough capacity to allocate
				last_ptr = current_ptr;
				Tac -= m_fca;
				UTI_DEBUG_L3("%s allocating ST %d tac %d (last ptr %d)\n",
				             FUNCNAME, current_ptr, Tac, last_ptr);
				ThisSt->SetAllocation(m_fca, DVB_CR_TYPE_FCA);
			}
			else
			{
				// one round robin without any allocation
				if(current_ptr == last_ptr)
					break;
			}
			/* stop looping on TTs if all TTs have been checked */
			if((last_ptr == -1) && (current_ptr == m_fca_start_ptr))
				break;
		}
		m_fca_start_ptr = current_ptr;
	}
	else
	{
		UTI_INFO("%s skipping. Not enough capacity....\n", FUNCNAME);
	}

	return (Tac);
}

/**
 * Retrieve the next ST ontext
 * @param Offset points to the last ST index
 * @return Next ST context
 */
DC_St *DvbRcsDamaCtrlEsa::RoundRobin(int *Offset)
{
	const char *FUNCNAME = DC_DBG_PREFIX "[RoundRobin]";
	DC_Context::iterator st;

	// find the current ST
	if(*Offset != -1)
	{
		st = m_context.find((unsigned short) *Offset);
		if(st != m_context.end())
		{
			// find the next one
			st++;
		}
		else
		{
			st = m_context.begin();
		}
		if(st == m_context.end())
			st = m_context.begin();
	}
	else
	{
		st = m_context.begin();
	}

	if(st != m_context.end())
	{
		UTI_DEBUG_L3("%s searching ST from %d to %d....\n",
		             FUNCNAME, *Offset, st->first);

		// modify the offset
		*Offset = st->first;

		// return the st context
		return (st->second);
	}
	else
	{
		UTI_DEBUG_L3("%s searching ST from %d : no ST\n", FUNCNAME, *Offset);
		*Offset = -1;

		return ((DC_St *) NULL);
	}
}
