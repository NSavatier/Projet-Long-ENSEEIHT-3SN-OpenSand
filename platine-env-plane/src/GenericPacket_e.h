/*--------------------------------------------------------------------------*/
/*  @PROJECT : DOMINO2 ASP
    @COMPANY : ALCATEL SPACE
    @AUTHOR  : David DEBARGE - TRANSICIEL
    @ID      : $Name: v2_0_0 $ $Revision: 1.1.1.1 $ $Date: 2006/08/02 11:50:28 $
	
    @ROLE    : The GenericPacket class implements the generic packet 
               mechanism
    @HISTORY :
    03-02-20 : Creation
*/
/*--------------------------------------------------------------------------*/

#ifndef GenericPacket_e
#   define GenericPacket_e

/* SYSTEM RESOURCES */
/* PROJECT RESOURCES */
#   include "Types_e.h"
#   include "Error_e.h"

typedef enum
{
	C_CONTROLLER_ERROR = 0,
	C_CONTROLLER_EVENT = 1,
	C_CONTROLLER_PROBE = 2,

	C_CONTROLLER_TYPE_NB
} T_CONTROLLER_TYPE;

#   define HD_GEN_PKT_SIZE   8  /* the header size in bytes */
#   define ELT_GEN_PKT_SIZE  8  /* the element size in bytes */

/* The generic packet header */
typedef struct
{
	T_UINT16 _elementNumber;	  /*  number of generic element into packet */
	T_UINT8 _componentId;		  /*  4 bits (MS) for component type , 4 bits (LS) for component index */
	T_UINT8 _FSMNumber;			  /*  FSM number inside frame (major frame) */
	T_UINT32 _frameNumber;		  /*  frame number since beginning of simulation  */
} T_HD_GEN_PKT;


/* The generic packet element */
typedef struct
{
	T_UINT8 _id;					  /*  Id of element */
	T_UINT8 _categoryId;			  /*  category  Id of element */
	T_UINT16 _index;				  /*  index for element  */
	T_UINT32 _value;				  /*  element value  */
} T_ELT_GEN_PKT;


/* The generic packet definition */
typedef struct
{
	T_UINT16 _elementNumber;	  /*  number of generic element into packet */
	T_UINT8 _componentId;		  /*  4 bits (MS) for component type , 4 bits (LS) for component index */
	T_UINT8 _FSMNumber;			  /*  FSM number inside frame (major frame) */
	T_UINT32 _frameNumber;		  /*  frame number since beginning of simulation  */
	T_UINT8 _genEltPkt[];
} T_GENERIC_PKT;


/*  @ROLE    : This function creates a generic packet
    @RETURN  : Error code */
extern T_ERROR GENERIC_PACKET_Create(
													/* INOUT */ T_GENERIC_PKT ** ptr_this,
													/* IN    */ T_UINT16 nb_elt_pkt);


/*  @ROLE    : This function deletes a generic packet
    @RETURN  : Error code */
extern T_ERROR GENERIC_PACKET_Delete(
													/* INOUT */ T_GENERIC_PKT ** ptr_this);


/*  @ROLE    : This function creates a generic packet for the init command
    @RETURN  : Error code */
extern T_ERROR GENERIC_PACKET_MakeInit(
													  /* INOUT */ T_GENERIC_PKT ** ptr_this,
													  /* IN    */ T_UINT32 simRef,
													  /* IN    */ T_UINT8 componentType);

/*  @ROLE    : This function creates a generic packet for the end of simulation 
    @RETURN  : Error code */
T_ERROR GENERIC_PACKET_MakeEnd(
											/* INOUT */ T_GENERIC_PKT ** ptr_this,
											/* IN    */ T_UINT32 frameNumber,
											/* IN    */ T_UINT8 fsmNumber,
											/* IN    */ T_CONTROLLER_TYPE controllerType);


/*  @ROLE    : This function return the size (in byte) of the generic packet
    @RETURN  : Error code */
extern T_ERROR GENERIC_PACKET_SizeOf(
													/* INOUT */ T_GENERIC_PKT * ptr_this,
													/*   OUT */ T_UINT32 * ptr_size);


/*  @ROLE    : return the element packet [0..(elementNumber-1)] of generic packet
    @RETURN  : Error code */
extern T_ERROR GENERIC_PACKET_GetEltPkt(
														/* INOUT */ T_GENERIC_PKT * ptr_this,
														/* IN    */ T_UINT16 eltPktIndex,
														/*   OUT */
														T_ELT_GEN_PKT ** eltGenPkt);


/*  @ROLE    : return the header packet of generic packet
    @RETURN  : Error code */
extern T_ERROR GENERIC_PACKET_GetHdPkt(
													  /* INOUT */ T_GENERIC_PKT * ptr_this,
													  /*   OUT */ T_HD_GEN_PKT ** hdGenPkt);


/* ========================================================================== */
/*  @ROLE    : This function prints the GENERIC_PACKET data
    @RETURN  : None                                                           */
/* ========================================================================== */
#   ifdef _ASP_TRACE
#      define TRACE_LOG_GENERIC_PACKET(param) GENERIC_PrintPacket param;
extern void GENERIC_PrintPacket(
											 /* IN    */ T_TRACE_THREAD_TYPE traceThread,
											 /* IN    */
											 T_TRACE_COMPONENT_TYPE traceComponent,
											 /* IN    */ T_TRACE_LEVEL traceLevel,
											 /* IN    */ FILE * stream,
											 /* IN    */ T_GENERIC_PKT * GenericPacket,
											 /* IN    */ T_CHAR * format, ...);
#   else
#      define TRACE_LOG_GENERIC_PACKET(param)
#   endif
		 /* _ASP_TRACE */

#endif
