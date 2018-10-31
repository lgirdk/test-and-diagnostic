/*
 * If not stated otherwise in this file or this component's Licenses.txt file the
 * following copyright and licenses apply:
 *
 * Copyright 2015 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/

/**********************************************************************
   Copyright [2014] [Cisco Systems, Inc.]
 
   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at
 
       http://www.apache.org/licenses/LICENSE-2.0
 
   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
**********************************************************************/


/**************************************************************************

    module: cosa_ip_dml.c

        For COSA Data Model Library Development

    -------------------------------------------------------------------

    copyright:

        Cisco Systems, Inc.
        All Rights Reserved.

    -------------------------------------------------------------------

    description:

        This file implementes back-end apis for the COSA Data Model Library

    -------------------------------------------------------------------

    environment:

        platform independent

    -------------------------------------------------------------------

    author:

        COSA XML TOOL CODE GENERATOR 1.0

    -------------------------------------------------------------------

    revision:

        01/17/2011    initial revision.

**************************************************************************/

#include "ansc_platform.h"
#include "cosa_diagnostic_apis.h"
#include "plugin_main_apis.h"
#include "cosa_ip_dml.h"
#include "diag.h"

static ULONG last_tick;
#define REFRESH_INTERVAL 120
#define SPEEDTEST_ARG_SIZE 64
#define TIME_NO_NEGATIVE(x) ((long)(x) < 0 ? 0 : (x))

#ifndef ROUTEHOPS_HOST_STRING
#define ROUTEHOPS_HOST_STRING		"Host"
#endif


#ifndef _COSA_SIM_
BOOL CosaIpifGetSetSupported(char * pParamName);
#endif

//SpeedTest
BOOL g_enable_speedtest = FALSE;
BOOL g_run_speedtest = FALSE;
BOOL g_is_pingtest_running = FALSE;

char g_argument_speedtest[SPEEDTEST_ARG_SIZE + 1] ;

extern  COSAGetParamValueByPathNameProc     g_GetParamValueByPathNameProc;
extern  ANSC_HANDLE                         bus_handle;

/***********************************************************************
 IMPORTANT NOTE:

 According to TR69 spec:
 On successful receipt of a SetParameterValues RPC, the CPE MUST apply
 the changes to all of the specified Parameters atomically. That is, either
 all of the value changes are applied together, or none of the changes are
 applied at all. In the latter case, the CPE MUST return a fault response
 indicating the reason for the failure to apply the changes.

 The CPE MUST NOT apply any of the specified changes without applying all
 of them.

 In order to set parameter values correctly, the back-end is required to
 hold the updated values until "Validate" and "Commit" are called. Only after
 all the "Validate" passed in different objects, the "Commit" will be called.
 Otherwise, "Rollback" will be called instead.

 The sequence in COSA Data Model will be:

 SetParamBoolValue/SetParamIntValue/SetParamUlongValue/SetParamStringValue
 -- Backup the updated values;

 if( Validate_XXX())
 {
     Commit_XXX();    -- Commit the update all together in the same object
 }
 else
 {
     Rollback_XXX();  -- Remove the update at backup;
 }

***********************************************************************/


/***********************************************************************

 APIs for Object:

    IP.Diagnostics.


***********************************************************************/
/***********************************************************************

 APIs for Object:

    IP.Diagnostics.X_CISCO_COM_ARP.

    *  X_CISCO_COM_ARP_GetParamBoolValue
    *  X_CISCO_COM_ARP_GetParamIntValue
    *  X_CISCO_COM_ARP_GetParamUlongValue
    *  X_CISCO_COM_ARP_GetParamStringValue

***********************************************************************/
/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL
        X_CISCO_COM_ARP_GetParamBoolValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                BOOL*                       pBool
            );

    description:

        This function is called to retrieve Boolean parameter value;

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                BOOL*                       pBool
                The buffer of returned boolean value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
X_CISCO_COM_ARP_GetParamBoolValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL*                       pBool
    )
{
    /* check the parameter name and return the corresponding value */

    /* AnscTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL
        X_CISCO_COM_ARP_GetParamIntValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                int*                        pInt
            );

    description:

        This function is called to retrieve integer parameter value;

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                int*                        pInt
                The buffer of returned integer value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
X_CISCO_COM_ARP_GetParamIntValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        int*                        pInt
    )
{
    /* check the parameter name and return the corresponding value */

    /* AnscTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL
        X_CISCO_COM_ARP_GetParamUlongValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                ULONG*                      puLong
            );

    description:

        This function is called to retrieve ULONG parameter value;

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                ULONG*                      puLong
                The buffer of returned ULONG value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
X_CISCO_COM_ARP_GetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        ULONG*                      puLong
    )
{
    /* check the parameter name and return the corresponding value */

    /* AnscTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        ULONG
        X_CISCO_COM_ARP_GetParamStringValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                char*                       pValue,
                ULONG*                      pUlSize
            );

    description:

        This function is called to retrieve string parameter value;

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                char*                       pValue,
                The string value buffer;

                ULONG*                      pUlSize
                The buffer of length of string value;
                Usually size of 1023 will be used.
                If it's not big enough, put required size here and return 1;

    return:     0 if succeeded;
                1 if short of buffer size; (*pUlSize = required size)
                -1 if not supported.

**********************************************************************/
ULONG
X_CISCO_COM_ARP_GetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pValue,
        ULONG*                      pUlSize
    )
{
    /* check the parameter name and return the corresponding value */

    /* AnscTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return -1;
}

/***********************************************************************

 APIs for Object:

    IP.Diagnostics.X_CISCO_COM_ARP.Table.{i}.

    *  ARPTable_GetEntryCount
    *  ARPTable_GetEntry
    *  ARPTable_GetParamBoolValue
    *  ARPTable_GetParamIntValue
    *  ARPTable_GetParamUlongValue
    *  ARPTable_GetParamStringValue

***********************************************************************/
/**********************************************************************

    caller:     owner of this object

    prototype:

        ULONG
        ARPTable_GetEntryCount
            (
                ANSC_HANDLE                 hInsContext
            );

    description:

        This function is called to retrieve the count of the table.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The count of the table

**********************************************************************/
ULONG
ARPTable_GetEntryCount
    (
        ANSC_HANDLE                 hInsContext
    )
{
    PCOSA_DATAMODEL_DIAG            pMyObject           = (PCOSA_DATAMODEL_DIAG)g_pCosaBEManager->hDiag;
    PCOSA_DML_DIAG_ARP_TABLE        pArpTable           = (PCOSA_DML_DIAG_ARP_TABLE)pMyObject->pArpTable;
    ULONG                           entryCount          = pMyObject->ArpEntryCount;

    return entryCount;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        ANSC_HANDLE
        ARPTable_GetEntry
            (
                ANSC_HANDLE                 hInsContext,
                ULONG                       nIndex,
                ULONG*                      pInsNumber
            );

    description:

        This function is called to retrieve the entry specified by the index.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                ULONG                       nIndex,
                The index of this entry;

                ULONG*                      pInsNumber
                The output instance number;

    return:     The handle to identify the entry

**********************************************************************/
ANSC_HANDLE
ARPTable_GetEntry
    (
        ANSC_HANDLE                 hInsContext,
        ULONG                       nIndex,
        ULONG*                      pInsNumber
    )
{
    PCOSA_DATAMODEL_DIAG            pMyObject           = (PCOSA_DATAMODEL_DIAG)g_pCosaBEManager->hDiag;
    PCOSA_DML_DIAG_ARP_TABLE        pArpTable           = (PCOSA_DML_DIAG_ARP_TABLE)pMyObject->pArpTable;
    ULONG                           entryCount          = pMyObject->ArpEntryCount;

    *pInsNumber  = nIndex + 1;

    return (ANSC_HANDLE)&pArpTable[nIndex]; /* return the handle */
}


/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL
        ARPTable_IsUpdated
            (
                ANSC_HANDLE                 hInsContext
            );

    description:

        This function is checking whether the table is updated or not.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     TRUE or FALSE.

**********************************************************************/
BOOL
ARPTable_IsUpdated
    (
        ANSC_HANDLE                 hInsContext
    )
{
    PCOSA_DATAMODEL_DIAG            pMyObject           = (PCOSA_DATAMODEL_DIAG)g_pCosaBEManager->hDiag;
    PCOSA_DML_DIAG_ARP_TABLE        pArpTable           = (PCOSA_DML_DIAG_ARP_TABLE)pMyObject->pArpTable;
    ULONG                           entryCount          = pMyObject->ArpEntryCount;
    BOOL                            bIsUpdated   = TRUE;

    /*
        We can use one rough granularity interval to get whole table in case
        that the updating is too frequent.
        */
    if ( ( AnscGetTickInSeconds() - pMyObject->PreviousVisitTime ) < COSA_DML_DIAG_ARP_TABLE_ACCESS_INTERVAL )
    {
        bIsUpdated  = FALSE;
    }
    else
    {
        pMyObject->PreviousVisitTime =  AnscGetTickInSeconds();
        bIsUpdated  = TRUE;
    }

    return bIsUpdated;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        ULONG
        ARPTable_Synchronize
            (
                ANSC_HANDLE                 hInsContext
            );

    description:

        This function is called to synchronize the table.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The status of the operation.

**********************************************************************/
ULONG
ARPTable_Synchronize
    (
        ANSC_HANDLE                 hInsContext
    )
{
    ANSC_STATUS                     returnStatus      = ANSC_STATUS_FAILURE;
    PCOSA_DATAMODEL_DIAG            pMyObject         = (PCOSA_DATAMODEL_DIAG)g_pCosaBEManager->hDiag;
    PCOSA_DML_DIAG_ARP_TABLE        pArpTable         = (PCOSA_DML_DIAG_ARP_TABLE)pMyObject->pArpTable;
    ULONG                           entryCount        = pMyObject->ArpEntryCount;
    PCOSA_DML_DIAG_ARP_TABLE        pArpTable2        = NULL;

    pArpTable2         = CosaDmlDiagGetARPTable(NULL,&entryCount);
    if ( !pArpTable2 )
    {
        /* Get Error, we don't del link because next time, it may be successful */
        return ANSC_STATUS_FAILURE;
    }

/*    if ( pArpTable )
    {
        AnscFreeMemory(pArpTable);
    } */ //RDKB_EMU

    pMyObject->pArpTable     = pArpTable2;
    pMyObject->ArpEntryCount = entryCount;

    returnStatus =  ANSC_STATUS_SUCCESS;

    return returnStatus;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL
        ARPTable_GetParamBoolValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                BOOL*                       pBool
            );

    description:

        This function is called to retrieve Boolean parameter value;

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                BOOL*                       pBool
                The buffer of returned boolean value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
ARPTable_GetParamBoolValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL*                       pBool
    )
{
    PCOSA_DML_DIAG_ARP_TABLE        pArpTable           = (PCOSA_DML_DIAG_ARP_TABLE)hInsContext;

    /* check the parameter name and return the corresponding value */
    if( AnscEqualString(ParamName, "Static", TRUE))
    {
        /* collect value */
        *pBool    =  pArpTable->Static;

        return TRUE;
    }

    /* AnscTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL
        ARPTable_GetParamIntValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                int*                        pInt
            );

    description:

        This function is called to retrieve integer parameter value;

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                int*                        pInt
                The buffer of returned integer value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
ARPTable_GetParamIntValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        int*                        pInt
    )
{
    /* check the parameter name and return the corresponding value */

    /* AnscTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL
        ARPTable_GetParamUlongValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                ULONG*                      puLong
            );

    description:

        This function is called to retrieve ULONG parameter value;

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                ULONG*                      puLong
                The buffer of returned ULONG value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
ARPTable_GetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        ULONG*                      puLong
    )
{
    /* check the parameter name and return the corresponding value */

    return FALSE;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        ULONG
        ARPTable_GetParamStringValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                char*                       pValue,
                ULONG*                      pUlSize
            );

    description:

        This function is called to retrieve string parameter value;

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                char*                       pValue,
                The string value buffer;

                ULONG*                      pUlSize
                The buffer of length of string value;
                Usually size of 1023 will be used.
                If it's not big enough, put required size here and return 1;

    return:     0 if succeeded;
                1 if short of buffer size; (*pUlSize = required size)
                -1 if not supported.

**********************************************************************/
ULONG
ARPTable_GetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pValue,
        ULONG*                      pUlSize
    )
{
    PCOSA_DML_DIAG_ARP_TABLE        pArpTable           = (PCOSA_DML_DIAG_ARP_TABLE)hInsContext;

    /* check the parameter name and return the corresponding value */

    if( AnscEqualString(ParamName, "IPAddress", TRUE))
    {
        /* collect value */
        AnscCopyString(pValue, pArpTable->IPAddress);

        return 0;
    }

    if( AnscEqualString(ParamName, "MACAddress", TRUE))
    {
        /* collect value */
        if ( sizeof(pArpTable->MACAddress) <= *pUlSize)
        {
            _ansc_sprintf
                (
                    pValue,
                    "%02x:%02x:%02x:%02x:%02x:%02x",
                    pArpTable->MACAddress[0],
                    pArpTable->MACAddress[1],
                    pArpTable->MACAddress[2],
                    pArpTable->MACAddress[3],
                    pArpTable->MACAddress[4],
                    pArpTable->MACAddress[5]
                );

            return 0;
        }
        else
        {
            *pUlSize = sizeof(pArpTable->MACAddress);
            return 1;
        }
    }


    /* AnscTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return -1;
}

/***********************************************************************

 APIs for Object:

    IP.Diagnostics.X_RDKCENTRAL-COM_PingTest.

    *  X_RDKCENTRAL_COM_PingTest_GetParamBoolValue
    *  X_RDKCENTRAL_COM_PingTest_GetParamStringValue
    *  X_RDKCENTRAL_COM_PingTest_SetParamBoolValue
    *  X_RDKCENTRAL_COM_PingTest_Validate
    *  X_RDKCENTRAL_COM_PingTest_Commit
    *  X_RDKCENTRAL_COM_PingTest_Rollback

***********************************************************************/
void COSAIP_pingtest_ProcessThread_Start( void )
{
	pthread_t pingtestthread;
	pthread_create( &pingtestthread, NULL, &COSAIP_pingtest_ProcessThread, NULL );
}

int COSAIP_pingtest_ProcessThread( void *arg )
{
	diag_pingtest_device_details_t *pingtest_devdet = diag_pingtest_getdevicedetails( );
	diag_state_t	state;
	diag_err_t		err, err_return;
	diag_cfg_t		cfg;
	diag_stat_t 	statis;
	char 			pingtestresult[ 64 ] = { 0 },
					tmp_hostname[ 257 ]  = { 0 },
					*ptmp_hostname = NULL;

	//Detach the thread from loop
    pthread_detach( pthread_self( ) );

	if ( CCSP_SUCCESS != COSA_IP_diag_Startpingtest( ) )
	{
		AnscTraceFlow(( "<%s> Failed to execute PING Test\n", __FUNCTION__ ));
		g_is_pingtest_running = FALSE;
		return DIAG_ERR_OTHER;
	}

	//ping test process thread
	while( 1 ) 
	{
		//Get the current state of ping test
		err_return = diag_getstate( DIAG_MD_PING, &state );
		if ( err_return != DIAG_ERR_OK )
		break;

		/* wait till ping test complete or error state */
		if( DIAG_ST_COMPLETE == state || DIAG_ST_ERROR == state )
		{
			break;
		}

		// wait for 1 sec of previous query
		sleep( 1 );
	}

	if( err_return != DIAG_ERR_OK )
	{
		AnscTraceFlow(( "<%s> Failed to execute PING Test\n", __FUNCTION__ ));
		g_is_pingtest_running = 0;
		return DIAG_ERR_OTHER;
	}

//For future requirement if need
#if 0
	/* Get the ping test status */
	switch( state )
	{
		case DIAG_ST_NONE:
		{
			sprintf( pingtestresult, "%s", "None" );
		}
		break; /* DIAG_ST_NONE	*/

		case DIAG_ST_COMPLETE:
		{
			sprintf( pingtestresult, "%s", "Complete" );			
		}
		break; /* DIAG_ST_COMPLETE  */
		
		case DIAG_ST_ERROR:
		{
			diag_geterr(DIAG_MD_PING, &err);
			
			switch ( err ) 
			{
				case DIAG_ERR_RESOLVE:
					sprintf( pingtestresult, "%s", "Error_CannotResolveHostName" );			
					break; /* DIAG_ERR_RESOLVE */
				case DIAG_ERR_INTERNAL:
					sprintf( pingtestresult, "%s", "Error_Internal" );			
					break; /* DIAG_ERR_INTERNAL */
				case DIAG_ERR_OTHER:
				default:
					sprintf( pingtestresult, "%s", "Error_Other" );			
					break; /* DIAG_ERR_OTHER | default */
			}
		}
		break; /* DIAG_ST_ERROR */
		default:
			break;
	}
#endif /* 0 */

	/* Get the ping test configuration */
	diag_getcfg(DIAG_MD_PING, &cfg);

	/* Get the ping test results */
	diag_getstatis(DIAG_MD_PING, &statis);

	//Fill Device Details it's already not filled case
	COSA_IP_diag_FillDeviceDetails( );

	/*
	  * Remove first and last charecter from host name
	  * if host name is 'www.google.com' then we have to display like 
	  * www.google.com
	  */
	sprintf( tmp_hostname, "%s", "NULL" );

	if( cfg.host[ 0 ] != '\0' )
	{
		sprintf( tmp_hostname, "%s", cfg.host );

		ptmp_hostname = tmp_hostname;

		//Remove first special charecter
		if( tmp_hostname[ 0 ] == '\'' )
		ptmp_hostname++;
		
		//Remove last special charecter
		if ( tmp_hostname[ strlen( tmp_hostname ) - 1 ] == '\'' )
		ptmp_hostname[ strlen( ptmp_hostname ) - 1 ] = '\0';

		sprintf( tmp_hostname, "%s", ptmp_hostname );
	}

	AnscTraceFlow(( "DeviceId:%s;CmMac:%s;PartnerId:%s;DeviceModel:%s;Endpoint:%s;Attempts:%d;SuccessCount:%d;AvgRtt:%.2f\n",
					( pingtest_devdet->DeviceID[ 0 ] != '\0' ) ? pingtest_devdet->DeviceID : "NULL",
					( pingtest_devdet->ecmMAC[ 0 ] != '\0' ) ? pingtest_devdet->ecmMAC : "NULL",										
					( pingtest_devdet->PartnerID[ 0 ] != '\0' ) ? pingtest_devdet->PartnerID : "NULL",										
					( pingtest_devdet->DeviceModel[ 0 ] != '\0' ) ? pingtest_devdet->DeviceModel : "NULL", 				
					tmp_hostname,
					cfg.cnt,
					statis.u.ping.success,
					statis.u.ping.rtt_avg ));

	g_is_pingtest_running = 0;
	
	return DIAG_ERR_OK;
}

void COSA_IP_diag_FillDeviceDetails( void )
{
	diag_pingtest_device_details_t *pingtest_devdet = diag_pingtest_getdevicedetails( );

	/* Get CM MAC if already having NULL */
	if( '\0' == pingtest_devdet->ecmMAC[ 0 ] )
	{
		memset( pingtest_devdet->ecmMAC, 0, sizeof( pingtest_devdet->ecmMAC ));

		COSA_IP_diag_getGetParamValue( "Device.X_CISCO_COM_CableModem.MACAddress", 
									  pingtest_devdet->ecmMAC,
									  sizeof( pingtest_devdet->ecmMAC ));
	}

	/* Get Serial number if already having NULL */
	if( '\0' == pingtest_devdet->DeviceID[ 0 ] )
	{
		memset( pingtest_devdet->DeviceID, 0, sizeof( pingtest_devdet->DeviceID ));

		COSA_IP_diag_getGetParamValue( "Device.DeviceInfo.SerialNumber", 
									  pingtest_devdet->DeviceID,
									  sizeof( pingtest_devdet->DeviceID ));
	}

	/* Get ModelName if already having NULL */
	if( '\0' == pingtest_devdet->DeviceModel[ 0 ] )
	{
		memset( pingtest_devdet->DeviceModel, 0, sizeof( pingtest_devdet->DeviceModel ));

		COSA_IP_diag_getGetParamValue( "Device.DeviceInfo.ModelName", 
									  pingtest_devdet->DeviceModel,
									  sizeof( pingtest_devdet->DeviceModel ));
	}
}

int	COSA_IP_diag_Startpingtest( void )
{
	CCSP_MESSAGE_BUS_INFO *bus_info = (CCSP_MESSAGE_BUS_INFO *)bus_handle;
	parameterValStruct_t   param_val[ 1 ]    = { "Device.IP.Diagnostics.IPPing.DiagnosticsState", "Requested", ccsp_string };
	char				   component[ 256 ]  = "eRT.com.cisco.spvtg.ccsp.tdm";
	char				   bus[256]		     = "/com/cisco/spvtg/ccsp/tdm";
	char*				   faultParam 	     = NULL;
	int 				   ret			     = 0;	

	ret = CcspBaseIf_setParameterValues(  bus_handle,
										  component,
										  bus,
										  0,
										  0,
										  &param_val,
										  1,
										  TRUE,
										  &faultParam
										  );
			
	if( ( ret != CCSP_SUCCESS ) && \
		( faultParam )
	  )
	{
	    AnscTraceWarning(("%s -- failed to set parameter %s\n", __FUNCTION__, param_val[ 0 ].parameterName));
		bus_info->freefunc( faultParam );
	}

	return ret;
}

void COSA_IP_diag_getGetParamValue( char *ParamName, char *ParamValue, int size )
{
    ANSC_STATUS   retval  = ANSC_STATUS_FAILURE;

	if( ( NULL != ParamName ) && \
		( NULL != ParamValue ) && \
		( size > 0 ) 
	)
	{
		parameterValStruct_t	ParamStruct = { 0 };
		
		ParamStruct.parameterName	= ParamName;
		ParamStruct.parameterValue  = ParamValue;
		
		AnscTraceFlow(("%s - retrieve param %s\n", __FUNCTION__, ParamName));
		
		retval = g_GetParamValueByPathNameProc( bus_handle, &ParamStruct, &size);
		
		if ( retval != ANSC_STATUS_SUCCESS )
		{
			AnscTraceWarning(("%s -- failed to retrieve parameter %s\n", __FUNCTION__, ParamName));
		}
	}
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL
        X_RDKCENTRAL_COM_PingTest_GetParamBoolValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                BOOL*                       pBool
            );

    description:

        This function is called to retrieve Boolean parameter value;

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                BOOL*                       pBool
                The buffer of returned boolean value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
X_RDKCENTRAL_COM_PingTest_GetParamBoolValue
	(
	    ANSC_HANDLE                 hInsContext,
	    char*                       ParamName,
	    BOOL*                       pBool
	)
{
    /* check the parameter name and return the corresponding value */
	if ( AnscEqualString(ParamName, "Run", TRUE))
    {
	    *pBool = g_is_pingtest_running;
	    return TRUE;
    } 

    /* AnscTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL
        X_RDKCENTRAL_COM_PingTest_SetParamBoolValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                BOOL                        bValue
            );

    description:

        This function is called to set BOOL parameter value;

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                BOOL                        bValue
                The updated BOOL value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
X_RDKCENTRAL_COM_PingTest_SetParamBoolValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL                        bValue
    )
{
    /* check the parameter name and set the corresponding value */
	if ( AnscEqualString(ParamName, "Run", TRUE))
    {
		if( bValue )
		{
			if( FALSE == g_is_pingtest_running )
			{
				AnscTraceFlow(("%s Run Pingtest : %d \n",__FUNCTION__, bValue));
				g_is_pingtest_running = bValue;

				/* Start the PING test as a thread */
				COSAIP_pingtest_ProcessThread_Start( );
			}
			else
			{
				AnscTraceFlow(("%s Pingtest is already running : %d\n",__FUNCTION__, g_is_pingtest_running));
			}
		}
		
		return TRUE;
	 }

    /*AnscTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        ULONG
        X_RDKCENTRAL_COM_PingTest_GetParamStringValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                char*                       pValue,
                ULONG*                      pUlSize
            );

    description:

        This function is called to retrieve string parameter value;

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                char*                       pValue,
                The string value buffer;

                ULONG*                      pUlSize
                The buffer of length of string value;
                Usually size of 1023 will be used.
                If it's not big enough, put required size here and return 1;

    return:     0 if succeeded;
                1 if short of buffer size; (*pUlSize = required size)
                -1 if not supported.

**********************************************************************/
ULONG
X_RDKCENTRAL_COM_PingTest_GetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pValue,
        ULONG*                      pUlSize
    )
{
    diag_pingtest_device_details_t	*devdetails = diag_pingtest_getdevicedetails( );

	//Fill Device Details it's already not filled case
	COSA_IP_diag_FillDeviceDetails( );

    if( AnscEqualString(ParamName, "PartnerID", TRUE))
    {
		if ( *pUlSize < AnscSizeOfString( devdetails->PartnerID ) )
		{
			*pUlSize = AnscSizeOfString( devdetails->PartnerID ) + 1;
			return 1;
		}

		AnscCopyString( pValue, devdetails->PartnerID );
		*pUlSize = AnscSizeOfString( devdetails->PartnerID );
		
		return 0;
    }

    if( AnscEqualString(ParamName, "ecmMAC", TRUE))
    {
		if ( *pUlSize < AnscSizeOfString( devdetails->ecmMAC ) )
		{
			*pUlSize = AnscSizeOfString( devdetails->ecmMAC ) + 1;
			return 1;
		}

		AnscCopyString( pValue, devdetails->ecmMAC );
		*pUlSize = AnscSizeOfString( devdetails->ecmMAC );
		
		return 0;
    }

    if( AnscEqualString(ParamName, "DeviceID", TRUE))
    {
		if ( *pUlSize < AnscSizeOfString( devdetails->DeviceID ) )
		{
			*pUlSize = AnscSizeOfString( devdetails->DeviceID ) + 1;
			return 1;
		}

		AnscCopyString( pValue, devdetails->DeviceID );
		*pUlSize = AnscSizeOfString( devdetails->DeviceID );
		
		return 0;
    }

    if( AnscEqualString(ParamName, "DeviceModel", TRUE))
    {
		if ( *pUlSize < AnscSizeOfString( devdetails->DeviceModel ) )
		{
			*pUlSize = AnscSizeOfString( devdetails->DeviceModel ) + 1;
			return 1;
		}

		AnscCopyString( pValue, devdetails->DeviceModel );
		*pUlSize = AnscSizeOfString( devdetails->DeviceModel );
		
		return 0;
    }

    return -1;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL
        X_RDKCENTRAL_COM_PingTest_Validate
            (
                ANSC_HANDLE                 hInsContext,
                char*                       pReturnParamName,
                ULONG*                      puLength
            );

    description:

        This function is called to finally commit all the update.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       pReturnParamName,
                The buffer (128 bytes) of parameter name if there's a validation.

                ULONG*                      puLength
                The output length of the param name.

    return:     TRUE if there's no validation.

**********************************************************************/
BOOL
X_RDKCENTRAL_COM_PingTest_Validate
    (
        ANSC_HANDLE                 hInsContext,
        char*                       pReturnParamName,
        ULONG*                      puLength
    )
{
    return TRUE;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        ULONG
        X_RDKCENTRAL_COM_PingTest_Commit
            (
                ANSC_HANDLE                 hInsContext
            );

    description:

        This function is called to finally commit all the update.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The status of the operation.

**********************************************************************/
ULONG
X_RDKCENTRAL_COM_PingTest_Commit
    (
        ANSC_HANDLE                 hInsContext
    )
{
    return 0;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        ULONG
        X_RDKCENTRAL_COM_PingTest_Rollback
            (
                ANSC_HANDLE                 hInsContext
            );

    description:

        This function is called to roll back the update whenever there's a
        validation found.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The status of the operation.

**********************************************************************/
ULONG
X_RDKCENTRAL_COM_PingTest_Rollback
    (
        ANSC_HANDLE                 hInsContext
    )
{
    return 0;
}

/***********************************************************************

 APIs for Object:

    IP.Diagnostics.IPPing.

    *  IPPing_GetParamBoolValue
    *  IPPing_GetParamIntValue
    *  IPPing_GetParamUlongValue
    *  IPPing_GetParamStringValue
    *  IPPing_SetParamBoolValue
    *  IPPing_SetParamIntValue
    *  IPPing_SetParamUlongValue
    *  IPPing_SetParamStringValue
    *  IPPing_Validate
    *  IPPing_Commit
    *  IPPing_Rollback

***********************************************************************/
/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL
        IPPing_GetParamBoolValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                BOOL*                       pBool
            );

    description:

        This function is called to retrieve Boolean parameter value;

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                BOOL*                       pBool
                The buffer of returned boolean value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
IPPing_GetParamBoolValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL*                       pBool
    )
{
    /* check the parameter name and return the corresponding value */

    /* AnscTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL
        IPPing_GetParamIntValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                int*                        pInt
            );

    description:

        This function is called to retrieve integer parameter value;

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                int*                        pInt
                The buffer of returned integer value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
IPPing_GetParamIntValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        int*                        pInt
    )
{
    /* check the parameter name and return the corresponding value */

    /* AnscTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL
        IPPing_GetParamUlongValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                ULONG*                      puLong
            );

    description:

        This function is called to retrieve ULONG parameter value;

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                ULONG*                      puLong
                The buffer of returned ULONG value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
IPPing_GetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        ULONG*                      puLong
    )
{
    diag_state_t                    state;
    diag_err_t                      err;
    diag_cfg_t                      cfg;
    diag_stat_t                     statis;

    if (diag_getstate(DIAG_MD_PING, &state) != DIAG_ERR_OK
            || diag_getcfg(DIAG_MD_PING, &cfg) != DIAG_ERR_OK
            || diag_getstatis(DIAG_MD_PING, &statis) != DIAG_ERR_OK
            || diag_geterr(DIAG_MD_PING, &err) != DIAG_ERR_OK)
        return FALSE;

    if( AnscEqualString(ParamName, "DiagnosticsState", TRUE))
    {
        switch (state) {
            case DIAG_ST_NONE:
                *puLong = DSLH_DIAG_STATE_TYPE_None + 1;
                break;
            case DIAG_ST_ACTING:
                *puLong = DSLH_DIAG_STATE_TYPE_Requested + 1;
                break;
            case DIAG_ST_COMPLETE:
                *puLong = DSLH_DIAG_STATE_TYPE_Complete + 1;
                break;
            case DIAG_ST_ERROR:
                switch (err) {
                    case DIAG_ERR_RESOLVE:
                        *puLong = DSLH_DIAG_STATE_TYPE_PING_Error_HostName + 1;
                        break;
                    case DIAG_ERR_INTERNAL:
                        *puLong = DSLH_DIAG_STATE_TYPE_PING_Error_Internal + 1;
                        break;
                    case DIAG_ERR_OTHER:
                    default:
                        *puLong = DSLH_DIAG_STATE_TYPE_PING_Error_Other + 1;
                        break;
                }
                break;
            default:
                return FALSE;
        }

        return TRUE;
    }

    if( AnscEqualString(ParamName, "NumberOfRepetitions", TRUE))
        *puLong = cfg.cnt;
    else if( AnscEqualString(ParamName, "Timeout", TRUE))
        *puLong = cfg.timo * 1000;
    else if( AnscEqualString(ParamName, "DataBlockSize", TRUE))
        *puLong = cfg.size;
    else if( AnscEqualString(ParamName, "DSCP", TRUE))
        *puLong = cfg.tos >> 2;
    else if( AnscEqualString(ParamName, "SuccessCount", TRUE))
        *puLong = statis.u.ping.success;
    else if( AnscEqualString(ParamName, "FailureCount", TRUE))
        *puLong = statis.u.ping.failure;
    else if( AnscEqualString(ParamName, "AverageResponseTime", TRUE))
        *puLong = statis.u.ping.rtt_avg;
    else if( AnscEqualString(ParamName, "MinimumResponseTime", TRUE))
        *puLong = statis.u.ping.rtt_min;
    else if( AnscEqualString(ParamName, "MaximumResponseTime", TRUE))
        *puLong = statis.u.ping.rtt_max;
    else
        return FALSE;

    return TRUE;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        ULONG
        IPPing_GetParamStringValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                char*                       pValue,
                ULONG*                      pUlSize
            );

    description:

        This function is called to retrieve string parameter value;

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                char*                       pValue,
                The string value buffer;

                ULONG*                      pUlSize
                The buffer of length of string value;
                Usually size of 1023 will be used.
                If it's not big enough, put required size here and return 1;

    return:     0 if succeeded;
                1 if short of buffer size; (*pUlSize = required size)
                -1 if not supported.

**********************************************************************/
ULONG
IPPing_GetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pValue,
        ULONG*                      pUlSize
    )
{
    diag_cfg_t                      cfg;

    if (diag_getcfg(DIAG_MD_PING, &cfg) != DIAG_ERR_OK)
        return -1;

    if( AnscEqualString(ParamName, "Interface", TRUE))
    {
        /*
         *  Revert to TR-181 definition -- object reference
         *
        if (*pUlSize <= strlen(cfg.ifname))
        {
            *pUlSize = strlen(cfg.ifname) + 1;
            return 1;
        }

        strncpy(pValue, cfg.ifname, *pUlSize);
         */
        if ( *pUlSize <= _ansc_strlen(cfg.Interface) )
        {
            *pUlSize = _ansc_strlen(cfg.Interface) + 1;
            return 1;
        }
        else
        {
            _ansc_strncpy(pValue, cfg.Interface, *pUlSize);
            *pUlSize = _ansc_strlen(cfg.Interface) + 1;
        }
    }
    else if( AnscEqualString(ParamName, "Host", TRUE))
    {
        if (*pUlSize <= strlen(cfg.host))
        {
            *pUlSize = strlen(cfg.host) + 1;
            return 1;
        }

        strncpy(pValue, cfg.host, *pUlSize);
    }
    else
        return -1;

    return 0;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL
        IPPing_SetParamBoolValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                BOOL                        bValue
            );

    description:

        This function is called to set BOOL parameter value;

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                BOOL                        bValue
                The updated BOOL value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
IPPing_SetParamBoolValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL                        bValue
    )
{
    /* check the parameter name and set the corresponding value */

    /* AnscTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL
        IPPing_SetParamIntValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                int                         iValue
            );

    description:

        This function is called to set integer parameter value;

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                int                         iValue
                The updated integer value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
IPPing_SetParamIntValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        int                         iValue
    )
{
    /* check the parameter name and set the corresponding value */

    /* AnscTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL
        IPPing_SetParamUlongValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                ULONG                       uValue
            );

    description:

        This function is called to set ULONG parameter value;

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                ULONG                       uValue
                The updated ULONG value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
IPPing_SetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        ULONG                       uValue
    )
{
    diag_cfg_t                      cfg;

    if (diag_getcfg(DIAG_MD_PING, &cfg) != DIAG_ERR_OK)
        return FALSE;

    if( AnscEqualString(ParamName, "DiagnosticsState", TRUE)) {
        if (uValue == DSLH_DIAG_STATE_TYPE_Requested + 1) {
            if (diag_start(DIAG_MD_PING) != 0)
                return FALSE;
            return TRUE;
        }
        return FALSE;
    }

    if( AnscEqualString(ParamName, "NumberOfRepetitions", TRUE))
        cfg.cnt = uValue;
    else if( AnscEqualString(ParamName, "Timeout", TRUE))
        cfg.timo = uValue / 1000;
    else if( AnscEqualString(ParamName, "DataBlockSize", TRUE))
    	{
    		char buf[256];
		memset(buf,0,sizeof(buf));
		cfg.size = uValue;
		sprintf(buf, "%d",cfg.size);
        	if (syscfg_set(NULL, "selfheal_ping_DataBlockSize",buf) == 0) 
		{
			syscfg_commit();
		}
	}
   else if( AnscEqualString(ParamName, "DSCP", TRUE))
       cfg.tos = uValue;
    else 
        return FALSE;

    if (diag_setcfg(DIAG_MD_PING, &cfg) != DIAG_ERR_OK)
        return FALSE;

    return TRUE;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL
        IPPing_SetParamStringValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                char*                       pString
            );

    description:

        This function is called to set string parameter value;

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                char*                       pString
                The updated string value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
IPPing_SetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pString
    )
{
    diag_cfg_t cfg;

    if (diag_getcfg(DIAG_MD_PING, &cfg) != DIAG_ERR_OK)
        return FALSE;

    if( AnscEqualString(ParamName, "Interface", TRUE))
    {
        /*
         *  Revert to TR-181 definition -- object reference
         *
        snprintf(cfg.ifname, sizeof(cfg.ifname), "%s", pString);
         */
        _ansc_snprintf(cfg.Interface, sizeof(cfg.Interface), "%s", pString);

        /*
         *  Fill in cfg.ifname based on Interface - Device.IP.Interface.<n>
         */
        {
            ANSC_STATUS             retval  = ANSC_STATUS_FAILURE;
            char                    IfNameParamName[DIAG_CFG_REF_STRING_LENGTH+1+5];
            parameterValStruct_t    ParamVal;
            int                     size = sizeof(cfg.ifname);

            _ansc_snprintf(IfNameParamName, sizeof(IfNameParamName), "%s.Name", cfg.Interface);

            ParamVal.parameterName  = IfNameParamName;
            ParamVal.parameterValue = cfg.ifname;

            AnscTraceFlow(("%s - retrieve param %s\n", __FUNCTION__, IfNameParamName));

            retval = g_GetParamValueByPathNameProc(bus_handle, &ParamVal, &size);

            if ( retval != ANSC_STATUS_SUCCESS )
            {
                AnscTraceWarning(("%s -- failed to retrieve parameter %s\n", __FUNCTION__, IfNameParamName));
            }
            else
            {
                AnscTraceFlow(("%s -- Interface.Name is %s\n", __FUNCTION__, cfg.ifname));
            }
        }
    }
    else if( AnscEqualString(ParamName, "Host", TRUE))
    {
		ANSC_STATUS             ret;
		char wrapped_host[64]={0};
        ret=CosaDmlInputValidation(pString, wrapped_host, AnscSizeOfString(pString), sizeof( wrapped_host ));
		if(ANSC_STATUS_SUCCESS != ret)
			return FALSE;
		
        snprintf(cfg.host, sizeof(cfg.host), "%s", wrapped_host);
    }
    else
        return FALSE;

    if (diag_setcfg(DIAG_MD_PING, &cfg) != DIAG_ERR_OK)
        return FALSE;

    return TRUE;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL
        IPPing_Validate
            (
                ANSC_HANDLE                 hInsContext,
                char*                       pReturnParamName,
                ULONG*                      puLength
            );

    description:

        This function is called to finally commit all the update.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       pReturnParamName,
                The buffer (128 bytes) of parameter name if there's a validation.

                ULONG*                      puLength
                The output length of the param name.

    return:     TRUE if there's no validation.

**********************************************************************/
BOOL
IPPing_Validate
    (
        ANSC_HANDLE                 hInsContext,
        char*                       pReturnParamName,
        ULONG*                      puLength
    )
{
    return TRUE;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        ULONG
        IPPing_Commit
            (
                ANSC_HANDLE                 hInsContext
            );

    description:

        This function is called to finally commit all the update.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The status of the operation.

**********************************************************************/
ULONG
IPPing_Commit
    (
        ANSC_HANDLE                 hInsContext
    )
{
    return 0;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        ULONG
        IPPing_Rollback
            (
                ANSC_HANDLE                 hInsContext
            );

    description:

        This function is called to roll back the update whenever there's a
        validation found.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The status of the operation.

**********************************************************************/
ULONG
IPPing_Rollback
    (
        ANSC_HANDLE                 hInsContext
    )
{
    return 0;
}

/***********************************************************************

 APIs for Object:

    IP.Diagnostics.TraceRoute.

    *  TraceRoute_GetParamBoolValue
    *  TraceRoute_GetParamIntValue
    *  TraceRoute_GetParamUlongValue
    *  TraceRoute_GetParamStringValue
    *  TraceRoute_SetParamBoolValue
    *  TraceRoute_SetParamIntValue
    *  TraceRoute_SetParamUlongValue
    *  TraceRoute_SetParamStringValue
    *  TraceRoute_Validate
    *  TraceRoute_Commit
    *  TraceRoute_Rollback

***********************************************************************/
/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL
        TraceRoute_GetParamBoolValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                BOOL*                       pBool
            );

    description:

        This function is called to retrieve Boolean parameter value;

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                BOOL*                       pBool
                The buffer of returned boolean value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
TraceRoute_GetParamBoolValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL*                       pBool
    )
{
    /* check the parameter name and return the corresponding value */

    /* AnscTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL
        TraceRoute_GetParamIntValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                int*                        pInt
            );

    description:

        This function is called to retrieve integer parameter value;

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                int*                        pInt
                The buffer of returned integer value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
TraceRoute_GetParamIntValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        int*                        pInt
    )
{
    /* check the parameter name and return the corresponding value */

    /* AnscTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL
        TraceRoute_GetParamUlongValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                ULONG*                      puLong
            );

    description:

        This function is called to retrieve ULONG parameter value;

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                ULONG*                      puLong
                The buffer of returned ULONG value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
TraceRoute_GetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        ULONG*                      puLong
    )
{
    diag_state_t                    state;
    diag_err_t                      err;
    diag_cfg_t                      cfg;
    diag_stat_t                     statis;

    if (diag_getstate(DIAG_MD_TRACERT, &state) != DIAG_ERR_OK
            || diag_getcfg(DIAG_MD_TRACERT, &cfg) != DIAG_ERR_OK
            || diag_getstatis(DIAG_MD_TRACERT, &statis) != DIAG_ERR_OK
            || diag_geterr(DIAG_MD_TRACERT, &err) != DIAG_ERR_OK)
        return FALSE;

    /* check the parameter name and return the corresponding value */
    if( AnscEqualString(ParamName, "DiagnosticsState", TRUE))
    {
        switch (state) {
            case DIAG_ST_NONE:
                *puLong = DSLH_DIAG_STATE_TYPE_None + 1;
                break;
            case DIAG_ST_ACTING:
                *puLong = DSLH_DIAG_STATE_TYPE_Requested + 1;
                break;
            case DIAG_ST_COMPLETE:
                *puLong = DSLH_DIAG_STATE_TYPE_Complete + 1;
                break;
            case DIAG_ST_ERROR:
                switch (err) {
                case DIAG_ERR_RESOLVE:
                    *puLong = DSLH_DIAG_STATE_TYPE_TRAC_Error_HostName + 1;
                    break;
                case DIAG_ERR_MAXHOPS:
                    *puLong = DSLH_DIAG_STATE_TYPE_TRAC_Error_MaxHopCount + 1;
                    break;
                case DIAG_ERR_OTHER:
                default:
                    /*
                     * voilate TR-181 has only two error state,
                     * but There really some other errors.
                     * Since UI using TR-181 values, we have to return one of them.
                     */
                    *puLong = DSLH_DIAG_STATE_TYPE_TRAC_Error_MaxHopCount + 1;
                    break;
                }
                break;
            default:
                return FALSE;
        }

        return TRUE;
    }

    if( AnscEqualString(ParamName, "NumberOfTries", TRUE))
        *puLong = cfg.cnt;
    else if( AnscEqualString(ParamName, "Timeout", TRUE))
        *puLong = cfg.timo * 1000;
    else if( AnscEqualString(ParamName, "DataBlockSize", TRUE))
        *puLong = cfg.size;
    else if( AnscEqualString(ParamName, "DSCP", TRUE))
        *puLong = cfg.tos >> 2;
    if( AnscEqualString(ParamName, "MaxHopCount", TRUE))
        *puLong = cfg.maxhop;
    if( AnscEqualString(ParamName, "ResponseTime", TRUE))
        *puLong = statis.u.tracert.resptime;

    return TRUE;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        ULONG
        TraceRoute_GetParamStringValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                char*                       pValue,
                ULONG*                      pUlSize
            );

    description:

        This function is called to retrieve string parameter value;

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                char*                       pValue,
                The string value buffer;

                ULONG*                      pUlSize
                The buffer of length of string value;
                Usually size of 1023 will be used.
                If it's not big enough, put required size here and return 1;

    return:     0 if succeeded;
                1 if short of buffer size; (*pUlSize = required size)
                -1 if not supported.

**********************************************************************/
ULONG
TraceRoute_GetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pValue,
        ULONG*                      pUlSize
    )
{
    diag_cfg_t                      cfg;

    if (diag_getcfg(DIAG_MD_TRACERT, &cfg) != DIAG_ERR_OK)
        return -1;

    /* check the parameter name and return the corresponding value */
    if( AnscEqualString(ParamName, "Interface", TRUE))
    {
        /*
         *  Revert to TR-181 definition -- object reference
         *
        if (*pUlSize <= strlen(cfg.ifname))
        {
            *pUlSize = strlen(cfg.ifname) + 1;
            return 1;
        }

        snprintf(pValue, *pUlSize, "%s", cfg.ifname);
         */
        if ( *pUlSize <= _ansc_strlen(cfg.Interface) )
        {
            *pUlSize = _ansc_strlen(cfg.Interface) + 1;
            return 1;
        }
        else
        {
            _ansc_strncpy(pValue, cfg.Interface, *pUlSize);
            *pUlSize = _ansc_strlen(cfg.Interface) + 1;
        }
    }

    if( AnscEqualString(ParamName, "Host", TRUE))
    {
        if (*pUlSize <= strlen(cfg.host))
        {
            *pUlSize = strlen(cfg.host) + 1;
            return 1;
        }

        snprintf(pValue, *pUlSize, "%s", cfg.host);
    }

    return 0;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL
        TraceRoute_SetParamBoolValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                BOOL                        bValue
            );

    description:

        This function is called to set BOOL parameter value;

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                BOOL                        bValue
                The updated BOOL value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
TraceRoute_SetParamBoolValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL                        bValue
    )
{
    /* check the parameter name and set the corresponding value */

    /* AnscTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL
        TraceRoute_SetParamIntValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                int                         iValue
            );

    description:

        This function is called to set integer parameter value;

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                int                         iValue
                The updated integer value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
TraceRoute_SetParamIntValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        int                         iValue
    )
{
    /* check the parameter name and set the corresponding value */

    /* AnscTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL
        TraceRoute_SetParamUlongValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                ULONG                       uValue
            );

    description:

        This function is called to set ULONG parameter value;

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                ULONG                       uValue
                The updated ULONG value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
TraceRoute_SetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        ULONG                       uValue
    )
{
    diag_cfg_t                      cfg;

    if( AnscEqualString(ParamName, "DiagnosticsState", TRUE))
    {
        if (uValue == DSLH_DIAG_STATE_TYPE_Requested + 1)
            if (diag_start(DIAG_MD_TRACERT) == DIAG_ERR_OK)
                return TRUE;
        return FALSE;
    }

    if (diag_getcfg(DIAG_MD_TRACERT, &cfg) != DIAG_ERR_OK)
        return FALSE;

    if( AnscEqualString(ParamName, "NumberOfTries", TRUE))
        cfg.cnt = uValue;
    else if( AnscEqualString(ParamName, "Timeout", TRUE))
        cfg.timo = uValue / 1000;
    else if( AnscEqualString(ParamName, "DataBlockSize", TRUE))
        cfg.size = uValue;
    else if( AnscEqualString(ParamName, "DSCP", TRUE))
        cfg.tos = uValue << 2;
    else if( AnscEqualString(ParamName, "MaxHopCount", TRUE))
        cfg.maxhop = uValue;
    else
        return FALSE;

    if (diag_setcfg(DIAG_MD_TRACERT, &cfg) != DIAG_ERR_OK)
        return FALSE;

    return TRUE;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL
        TraceRoute_SetParamStringValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                char*                       pString
            );

    description:

        This function is called to set string parameter value;

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                char*                       pString
                The updated string value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
TraceRoute_SetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pString
    )
{
    diag_cfg_t                      cfg;

    if (diag_getcfg(DIAG_MD_TRACERT, &cfg) != DIAG_ERR_OK)
        return FALSE;

    /* check the parameter name and set the corresponding value */
    if( AnscEqualString(ParamName, "Interface", TRUE))
    {
        /*
         *  Revert to TR-181 definition -- object reference
         *
        snprintf(cfg.ifname, sizeof(cfg.ifname), "%s", pString);
         */
        _ansc_snprintf(cfg.Interface, sizeof(cfg.Interface), "%s", pString);

        /*
         *  Fill in cfg.ifname based on Interface - Device.IP.Interface.<n>
         */
        {
            ANSC_STATUS             retval  = ANSC_STATUS_FAILURE;
            char                    IfNameParamName[DIAG_CFG_REF_STRING_LENGTH+1+5];
            parameterValStruct_t    ParamVal;
            int                     size = sizeof(cfg.ifname);

            _ansc_snprintf(IfNameParamName, sizeof(IfNameParamName), "%s.Name", cfg.Interface);

            ParamVal.parameterName  = IfNameParamName;
            ParamVal.parameterValue = cfg.ifname;

            AnscTraceFlow(("%s - retrieve param %s\n", __FUNCTION__, IfNameParamName));

            retval = g_GetParamValueByPathNameProc(bus_handle, &ParamVal, &size);

            if ( retval != ANSC_STATUS_SUCCESS )
            {
                AnscTraceWarning(("%s -- failed to retrieve parameter %s\n", __FUNCTION__, IfNameParamName));
            }
            else
            {
                AnscTraceFlow(("%s -- Interface.Name is %s\n", __FUNCTION__, cfg.ifname));
            }
        }
    }
    else if( AnscEqualString(ParamName, "Host", TRUE))
    {
		ANSC_STATUS             ret;
		char wrapped_host[64]={0};
		ret=CosaDmlInputValidation(pString, wrapped_host, AnscSizeOfString(pString), sizeof( wrapped_host ));
		if(ANSC_STATUS_SUCCESS != ret)
			return FALSE;

        snprintf(cfg.host, sizeof(cfg.host), "%s", wrapped_host);
    }
    else
        return FALSE;

    if (diag_setcfg(DIAG_MD_TRACERT, &cfg) != DIAG_ERR_OK)
        return FALSE;

    return TRUE;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL
        TraceRoute_Validate
            (
                ANSC_HANDLE                 hInsContext,
                char*                       pReturnParamName,
                ULONG*                      puLength
            );

    description:

        This function is called to finally commit all the update.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       pReturnParamName,
                The buffer (128 bytes) of parameter name if there's a validation.

                ULONG*                      puLength
                The output length of the param name.

    return:     TRUE if there's no validation.

**********************************************************************/
BOOL
TraceRoute_Validate
    (
        ANSC_HANDLE                 hInsContext,
        char*                       pReturnParamName,
        ULONG*                      puLength
    )
{
    return TRUE;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        ULONG
        TraceRoute_Commit
            (
                ANSC_HANDLE                 hInsContext
            );

    description:

        This function is called to finally commit all the update.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The status of the operation.

**********************************************************************/
ULONG
TraceRoute_Commit
    (
        ANSC_HANDLE                 hInsContext
    )
{
    return 0;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        ULONG
        TraceRoute_Rollback
            (
                ANSC_HANDLE                 hInsContext
            );

    description:

        This function is called to roll back the update whenever there's a
        validation found.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The status of the operation.

**********************************************************************/
ULONG
TraceRoute_Rollback
    (
        ANSC_HANDLE                 hInsContext
    )
{
    return 0;
}

/***********************************************************************

 APIs for Object:

    IP.Diagnostics.TraceRoute.RouteHops.{i}.

    *  RouteHops_GetEntryCount
    *  RouteHops_GetEntry
    *  RouteHops_IsUpdated
    *  RouteHops_Synchronize
    *  RouteHops_GetParamBoolValue
    *  RouteHops_GetParamIntValue
    *  RouteHops_GetParamUlongValue
    *  RouteHops_GetParamStringValue

***********************************************************************/
/**********************************************************************

    caller:     owner of this object

    prototype:

        ULONG
        RouteHops_GetEntryCount
            (
                ANSC_HANDLE                 hInsContext
            );

    description:

        This function is called to retrieve the count of the table.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The count of the table

**********************************************************************/
ULONG
RouteHops_GetEntryCount
    (
        ANSC_HANDLE                 hInsContext
    )
{
    diag_stat_t stat;

    if (diag_getstatis(DIAG_MD_TRACERT, &stat) != DIAG_ERR_OK)
        return 0;
    return stat.u.tracert.nhop;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        ANSC_HANDLE
        RouteHops_GetEntry
            (
                ANSC_HANDLE                 hInsContext,
                ULONG                       nIndex,
                ULONG*                      pInsNumber
            );

    description:

        This function is called to retrieve the entry specified by the index.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                ULONG                       nIndex,
                The index of this entry;

                ULONG*                      pInsNumber
                The output instance number;

    return:     The handle to identify the entry

**********************************************************************/
ANSC_HANDLE
RouteHops_GetEntry
    (
        ANSC_HANDLE                 hInsContext,
        ULONG                       nIndex,
        ULONG*                      pInsNumber
    )
{
    diag_stat_t stat;

    if (diag_getstatis(DIAG_MD_TRACERT, &stat) != DIAG_ERR_OK)
        return NULL;
    if (nIndex >= stat.u.tracert.nhop)
        return NULL;
    *pInsNumber  = nIndex + 1;
    return &stat.u.tracert.hops[nIndex];
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL
        RouteHops_IsUpdated
            (
                ANSC_HANDLE                 hInsContext
            );

    description:

        This function is checking whether the table is updated or not.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     TRUE or FALSE.

**********************************************************************/
BOOL
RouteHops_IsUpdated
    (
        ANSC_HANDLE                 hInsContext
    )
{
    return  TRUE;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        ULONG
        RouteHops_Synchronize
            (
                ANSC_HANDLE                 hInsContext
            );

    description:

        This function is called to synchronize the table.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The status of the operation.

**********************************************************************/
ULONG
RouteHops_Synchronize
    (
        ANSC_HANDLE                 hInsContext
    )
{
    return 0;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL
        RouteHops_GetParamBoolValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                BOOL*                       pBool
            );

    description:

        This function is called to retrieve Boolean parameter value;

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                BOOL*                       pBool
                The buffer of returned boolean value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
RouteHops_GetParamBoolValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL*                       pBool
    )
{
    /* check the parameter name and return the corresponding value */

    /* AnscTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL
        RouteHops_GetParamIntValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                int*                        pInt
            );

    description:

        This function is called to retrieve integer parameter value;

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                int*                        pInt
                The buffer of returned integer value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
RouteHops_GetParamIntValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        int*                        pInt
    )
{
    /* check the parameter name and return the corresponding value */

    /* AnscTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL
        RouteHops_GetParamUlongValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                ULONG*                      puLong
            );

    description:

        This function is called to retrieve ULONG parameter value;

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                ULONG*                      puLong
                The buffer of returned ULONG value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
RouteHops_GetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        ULONG*                      puLong
    )
{
    tracert_hop_t                   *hop = (tracert_hop_t *)hInsContext;

    if (!hop)
        return FALSE;

    if( AnscEqualString(ParamName, "ErrorCode", TRUE))
    {
        *puLong = hop->icmperr;
        return TRUE;
    }

    return FALSE;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        ULONG
        RouteHops_GetParamStringValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                char*                       pValue,
                ULONG*                      pUlSize
            );

    description:

        This function is called to retrieve string parameter value;

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                char*                       pValue,
                The string value buffer;

                ULONG*                      pUlSize
                The buffer of length of string value;
                Usually size of 1023 will be used.
                If it's not big enough, put required size here and return 1;

    return:     0 if succeeded;
                1 if short of buffer size; (*pUlSize = required size)
                -1 if not supported.

**********************************************************************/
ULONG
RouteHops_GetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pValue,
        ULONG*                      pUlSize
    )
{
    tracert_hop_t                   *hop = (tracert_hop_t *)hInsContext;

    if (!hop)
        return FALSE;

    if( AnscEqualString(ParamName, "Host", TRUE))
    {
        if (strlen(hop->host) >= *pUlSize) {
            *pUlSize = strlen(hop->host) + 1;
            return 1;
        }

        snprintf(pValue, *pUlSize, "%s", hop->host);
        return 0;
    }
    if( AnscEqualString(ParamName, "HostAddress", TRUE))
    {
        if (strlen(hop->addr) >= *pUlSize) {
            *pUlSize = strlen(hop->addr) + 1;
            return 1;
        }

        snprintf(pValue, *pUlSize, "%s", hop->addr);
        return 0;
    }
    if( AnscEqualString(ParamName, "RTTimes", TRUE))
    {
        if (strlen(hop->rtts) >= *pUlSize) {
            *pUlSize = strlen(hop->rtts) + 1;
            return 1;
        }

        snprintf(pValue, *pUlSize, "%s", hop->rtts);
        return 0;
    }

    return -1;
}

/***********************************************************************


 APIs for Object:

    IP.Diagnostics.DownloadDiagnostics.

    *  DownloadDiagnostics_GetParamBoolValue
    *  DownloadDiagnostics_GetParamIntValue
    *  DownloadDiagnostics_GetParamUlongValue
    *  DownloadDiagnostics_GetParamStringValue
    *  DownloadDiagnostics_SetParamBoolValue
    *  DownloadDiagnostics_SetParamIntValue
    *  DownloadDiagnostics_SetParamUlongValue
    *  DownloadDiagnostics_SetParamStringValue
    *  DownloadDiagnostics_Validate
    *  DownloadDiagnostics_Commit
    *  DownloadDiagnostics_Rollback

***********************************************************************/
/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL
        DownloadDiagnostics_GetParamBoolValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                BOOL*                       pBool
            );

    description:

        This function is called to retrieve Boolean parameter value;

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                BOOL*                       pBool
                The buffer of returned boolean value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
DownloadDiagnostics_GetParamBoolValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL*                       pBool
    )
{
    /* check the parameter name and return the corresponding value */

    /* AnscTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL
        DownloadDiagnostics_GetParamIntValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                int*                        pInt
            );

    description:

        This function is called to retrieve integer parameter value;

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                int*                        pInt
                The buffer of returned integer value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
DownloadDiagnostics_GetParamIntValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        int*                        pInt
    )
{
    /* check the parameter name and return the corresponding value */

    /* AnscTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL
        DownloadDiagnostics_GetParamUlongValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                ULONG*                      puLong
            );

    description:

        This function is called to retrieve ULONG parameter value;

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                ULONG*                      puLong
                The buffer of returned ULONG value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
DownloadDiagnostics_GetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        ULONG*                      puLong
    )
{
    PCOSA_DATAMODEL_DIAG            pMyObject          = (PCOSA_DATAMODEL_DIAG)g_pCosaBEManager->hDiag;
    PDSLH_TR143_DOWNLOAD_DIAG_INFO  pDownloadInfo      = pMyObject->hDiagDownloadInfo;
    PDSLH_TR143_DOWNLOAD_DIAG_STATS pDownloadDiagStats = NULL;


    /* check the parameter name and return the corresponding value */
    if ( AnscEqualString(ParamName, "DiagnosticsState", TRUE))
    {
        pDownloadDiagStats = (PDSLH_TR143_DOWNLOAD_DIAG_STATS)CosaDmlDiagGetResults
                                (
                                    DSLH_DIAGNOSTIC_TYPE_Download
                                );

        if ( pDownloadDiagStats )
        {
            *puLong = pDownloadDiagStats->DiagStates + 1;
        }
        else
        {
            AnscTraceWarning(("Download Diagnostics---Failed to get DiagnosticsState\n!"));

            *puLong = 0;
            return FALSE;
        }

        return TRUE;
    }

    if ( AnscEqualString(ParamName, "DSCP", TRUE))
    {
        if ( pDownloadInfo )
        {
            *puLong = pDownloadInfo->DSCP;
        }
        else
        {
            AnscTraceWarning(("Download Diagnostics---Failed to get DSCP \n!"));
            *puLong = 0;
            return FALSE;
        }

        return TRUE;
    }

    if ( AnscEqualString(ParamName, "EthernetPriority", TRUE))
    {
        if ( pDownloadInfo )
        {
            *puLong = pDownloadInfo->EthernetPriority;
        }
        else
        {
            AnscTraceWarning(("Download Diagnostics---Failed to get EthernetPriority \n!"));
            *puLong = 0;
            return FALSE;
        }

        return TRUE;
    }

    if ( AnscEqualString(ParamName, "TestBytesReceived", TRUE))
    {
        pDownloadDiagStats = (PDSLH_TR143_DOWNLOAD_DIAG_STATS)CosaDmlDiagGetResults
                                (
                                    DSLH_DIAGNOSTIC_TYPE_Download
                                );

        if ( pDownloadDiagStats )
        {
            *puLong = pDownloadDiagStats->TestBytesReceived;
        }
        else
        {
            AnscTraceWarning(("Download Diagnostics---Failed to get TestBytesReceived\n!"));

            *puLong = 0;
            return FALSE;
        }

        return TRUE;
    }

    if ( AnscEqualString(ParamName, "TotalBytesReceived", TRUE))
    {
        pDownloadDiagStats = (PDSLH_TR143_DOWNLOAD_DIAG_STATS)CosaDmlDiagGetResults
                                (
                                    DSLH_DIAGNOSTIC_TYPE_Download
                                );


        if ( pDownloadDiagStats )
        {
            *puLong = pDownloadDiagStats->TotalBytesReceived;
        }
        else
        {
            AnscTraceWarning(("Download Diagnostics---Failed to get TotalBytesReceived\n!"));

            *puLong = 0;
            return FALSE;
        }

        return TRUE;
    }

    /* AnscTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        ULONG
        DownloadDiagnostics_GetParamStringValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                char*                       pValue,
                ULONG*                      pUlSize
            );

    description:

        This function is called to retrieve string parameter value;

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                char*                       pValue,
                The string value buffer;

                ULONG*                      pUlSize
                The buffer of length of string value;
                Usually size of 1023 will be used.
                If it's not big enough, put required size here and return 1;

    return:     0 if succeeded;
                1 if short of buffer size; (*pUlSize = required size)
                -1 if not supported.

**********************************************************************/
ULONG
DownloadDiagnostics_GetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pValue,
        ULONG*                      pUlSize
    )
{
    PCOSA_DATAMODEL_DIAG            pMyObject          = (PCOSA_DATAMODEL_DIAG)g_pCosaBEManager->hDiag;
    PDSLH_TR143_DOWNLOAD_DIAG_INFO  pDownloadInfo      = pMyObject->hDiagDownloadInfo;
    PDSLH_TR143_DOWNLOAD_DIAG_STATS pDownloadDiagStats = NULL;
    PANSC_UNIVERSAL_TIME            pTime              = NULL;
    char                            pBuf[128]          = { 0 };


    /* check the parameter name and return the corresponding value */
    if ( AnscEqualString(ParamName, "Interface", TRUE))
    {
        if ( pDownloadInfo )
        {
            AnscCopyString(pValue, pDownloadInfo->Interface);
        }
        else
        {
            AnscTraceWarning(("Download Diagnostics---Failed to get Interface\n!"));
            return -1;
        }

        return 0;
    }

    if ( AnscEqualString(ParamName, "DownloadURL", TRUE))
    {
        if ( pDownloadInfo )
        {
            AnscCopyString(pValue, pDownloadInfo->DownloadURL);
        }
        else
        {
            AnscTraceWarning(("Download Diagnostics---Failed to get DownloadURL \n!"));
            return -1;
        }

        return 0;
    }

    if( AnscEqualString(ParamName, "ROMTime", TRUE))
    {
        pDownloadDiagStats = (PDSLH_TR143_DOWNLOAD_DIAG_STATS)CosaDmlDiagGetResults
                                (
                                    DSLH_DIAGNOSTIC_TYPE_Download
                                );

        if ( pDownloadDiagStats )
        {
            pTime = (PANSC_UNIVERSAL_TIME)&pDownloadDiagStats->ROMTime;

            _ansc_sprintf
            (
                pBuf,
                "%.4d-%.2d-%.2dT%.2d:%.2d:%.2d.%.3d000",
                pTime->Year,
                pTime->Month,
                pTime->DayOfMonth,
                pTime->Hour,
                pTime->Minute,
                pTime->Second,
                pTime->MilliSecond
            );

            AnscCopyString(pValue, pBuf);
        }
        else
        {
            AnscTraceWarning(("Download Diagnostics---Failed to get ROMTime\n!"));

            return -1;
        }

        return 0;
    }

    if ( AnscEqualString(ParamName, "BOMTime", TRUE))
    {
        pDownloadDiagStats = (PDSLH_TR143_DOWNLOAD_DIAG_STATS)CosaDmlDiagGetResults
                                (
                                    DSLH_DIAGNOSTIC_TYPE_Download
                                );

        if ( pDownloadDiagStats )
        {
            pTime = (PANSC_UNIVERSAL_TIME)&pDownloadDiagStats->BOMTime;

            _ansc_sprintf
            (
                pBuf,
                "%.4d-%.2d-%.2dT%.2d:%.2d:%.2d.%.3d000",
                pTime->Year,
                pTime->Month,
                pTime->DayOfMonth,
                pTime->Hour,
                pTime->Minute,
                pTime->Second,
                pTime->MilliSecond
            );

            AnscCopyString(pValue, pBuf);
        }
        else
        {
            AnscTraceWarning(("Download Diagnostics---Failed to get BOMTime\n!"));

            return -1;
        }

        return 0;
    }

    if ( AnscEqualString(ParamName, "EOMTime", TRUE))
    {
        pDownloadDiagStats = (PDSLH_TR143_DOWNLOAD_DIAG_STATS)CosaDmlDiagGetResults
                                (
                                    DSLH_DIAGNOSTIC_TYPE_Download
                                );

        if ( pDownloadDiagStats )
        {
            pTime = (PANSC_UNIVERSAL_TIME)&pDownloadDiagStats->EOMTime;

            _ansc_sprintf
            (
                pBuf,
                "%.4d-%.2d-%.2dT%.2d:%.2d:%.2d.%.3d000",
                pTime->Year,
                pTime->Month,
                pTime->DayOfMonth,
                pTime->Hour,
                pTime->Minute,
                pTime->Second,
                pTime->MilliSecond
            );

            AnscCopyString(pValue, pBuf);
        }
        else
        {
            AnscTraceWarning(("Download Diagnostics---Failed to get EOMTime\n!"));

            return -1;
        }

        return 0;
    }

    if ( AnscEqualString(ParamName, "TCPOpenRequestTime", TRUE))
    {
        pDownloadDiagStats = (PDSLH_TR143_DOWNLOAD_DIAG_STATS)CosaDmlDiagGetResults
                                (
                                    DSLH_DIAGNOSTIC_TYPE_Download
                                );

        if ( pDownloadDiagStats )
        {
            pTime = (PANSC_UNIVERSAL_TIME)&pDownloadDiagStats->TCPOpenRequestTime;

            _ansc_sprintf
            (
                pBuf,
                "%.4d-%.2d-%.2dT%.2d:%.2d:%.2d.%.3d000",
                pTime->Year,
                pTime->Month,
                pTime->DayOfMonth,
                pTime->Hour,
                pTime->Minute,
                pTime->Second,
                pTime->MilliSecond
            );

            AnscCopyString(pValue, pBuf);
        }
        else
        {
            AnscTraceWarning(("Download Diagnostics---Failed to get TCPOpenRequestTime\n!"));

            return -1;
        }

        return 0;
    }

    if ( AnscEqualString(ParamName, "TCPOpenResponseTime", TRUE))
    {
        pDownloadDiagStats = (PDSLH_TR143_DOWNLOAD_DIAG_STATS)CosaDmlDiagGetResults
                                (
                                    DSLH_DIAGNOSTIC_TYPE_Download
                                );
        if ( pDownloadDiagStats )
        {
            pTime = (PANSC_UNIVERSAL_TIME)&pDownloadDiagStats->TCPOpenResponseTime;

            _ansc_sprintf
            (
                pBuf,
                "%.4d-%.2d-%.2dT%.2d:%.2d:%.2d.%.3d000",
                pTime->Year,
                pTime->Month,
                pTime->DayOfMonth,
                pTime->Hour,
                pTime->Minute,
                pTime->Second,
                pTime->MilliSecond
            );

            AnscCopyString(pValue, pBuf);
        }
        else
        {
            AnscTraceWarning(("Download Diagnostics---Failed to get TCPOpenResponseTime\n!"));

            return -1;
        }

        return 0;
    }

	if ( AnscEqualString(ParamName, "DownloadTransports", TRUE) )
	{
		if (!pValue || !pUlSize)
			return -1;

		if (*pUlSize < AnscSizeOfString("HTTP") + 1)
		{
			*pUlSize = AnscSizeOfString("HTTP") + 1;
			return 1;
		}

		AnscCopyString(pValue, "HTTP");
		return 0;
	}

    /* AnscTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return -1;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL
        DownloadDiagnostics_SetParamBoolValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                BOOL                        bValue
            );

    description:

        This function is called to set BOOL parameter value;

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                BOOL                        bValue
                The updated BOOL value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
DownloadDiagnostics_SetParamBoolValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL                        bValue
    )
{
    /* check the parameter name and set the corresponding value */

    /* AnscTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL
        DownloadDiagnostics_SetParamIntValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                int                         iValue
            );

    description:

        This function is called to set integer parameter value;

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                int                         iValue
                The updated integer value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
DownloadDiagnostics_SetParamIntValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        int                         iValue
    )
{
    /* check the parameter name and set the corresponding value */

    /* AnscTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL
        DownloadDiagnostics_SetParamUlongValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                ULONG                       uValue
            );

    description:

        This function is called to set ULONG parameter value;

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                ULONG                       uValue
                The updated ULONG value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
DownloadDiagnostics_SetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        ULONG                       uValue
    )
{
    PCOSA_DATAMODEL_DIAG            pMyObject          = (PCOSA_DATAMODEL_DIAG)g_pCosaBEManager->hDiag;
    PDSLH_TR143_DOWNLOAD_DIAG_INFO  pDownloadInfo      = pMyObject->hDiagDownloadInfo;
    PDSLH_TR143_DOWNLOAD_DIAG_STATS pDownloadDiagStats = NULL;

	/* according to TR-181, set writable params other than DiagnosticsState,
	 * must set DiagnosticsState to "NONE". */
	pDownloadInfo->DiagnosticsState = DSLH_TR143_DIAGNOSTIC_None;

    /* check the parameter name and set the corresponding value */
    if ( AnscEqualString(ParamName, "DiagnosticsState", TRUE))
    {
        uValue--;
        if ( uValue != (ULONG)DSLH_TR143_DIAGNOSTIC_Requested )
        {
            return FALSE;
        }

        pDownloadInfo->DiagnosticsState = DSLH_TR143_DIAGNOSTIC_Requested;
        return TRUE;
    }

    if ( AnscEqualString(ParamName, "DSCP", TRUE))
    {
        pDownloadInfo->DSCP= uValue;
        return TRUE;
    }

    if ( AnscEqualString(ParamName, "EthernetPriority", TRUE))
    {
        pDownloadInfo->EthernetPriority = uValue;
        return TRUE;
    }


    /* AnscTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL
        DownloadDiagnostics_SetParamStringValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                char*                       pString
            );

    description:

        This function is called to set string parameter value;

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                char*                       pString
                The updated string value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
DownloadDiagnostics_SetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pString
    )
{
    PCOSA_DATAMODEL_DIAG            pMyObject          = (PCOSA_DATAMODEL_DIAG)g_pCosaBEManager->hDiag;
    PDSLH_TR143_DOWNLOAD_DIAG_INFO  pDownloadInfo      = pMyObject->hDiagDownloadInfo;

	/* according to TR-181, set writable params other than DiagnosticsState,
	 * must set DiagnosticsState to "NONE". */
	pDownloadInfo->DiagnosticsState = DSLH_TR143_DIAGNOSTIC_None;

    /* check the parameter name and set the corresponding value */
    if ( AnscEqualString(ParamName, "Interface", TRUE))
    {
        AnscCopyString(pDownloadInfo->Interface, pString);
        return TRUE;
    }

    if ( AnscEqualString(ParamName, "DownloadURL", TRUE))
    {
        if ( !pString || !(*pString) )
        {
            return FALSE;
        }

        AnscCopyString(pDownloadInfo->DownloadURL, pString);
        return TRUE;
    }

    /*AnscTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL
        DownloadDiagnostics_Validate
            (
                ANSC_HANDLE                 hInsContext,
                char*                       pReturnParamName,
                ULONG*                      puLength
            );

    description:

        This function is called to finally commit all the update.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       pReturnParamName,
                The buffer (128 bytes) of parameter name if there's a validation.

                ULONG*                      puLength
                The output length of the param name.

    return:     TRUE if there's no validation.

**********************************************************************/
BOOL
DownloadDiagnostics_Validate
    (
        ANSC_HANDLE                 hInsContext,
        char*                       pReturnParamName,
        ULONG*                      puLength
    )
{
    PCOSA_DATAMODEL_DIAG            pMyObject          = (PCOSA_DATAMODEL_DIAG)g_pCosaBEManager->hDiag;
    PDSLH_TR143_DOWNLOAD_DIAG_INFO  pDownloadInfo      = pMyObject->hDiagDownloadInfo;

    if ( pDownloadInfo->DiagnosticsState == DSLH_TR143_DIAGNOSTIC_Requested
      && !AnscSizeOfString(pDownloadInfo->DownloadURL) )
    {
        AnscCopyString(pReturnParamName, "DownloadURL");
        return FALSE;
    }

    return  TRUE;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        ULONG
        DownloadDiagnostics_Commit
            (
                ANSC_HANDLE                 hInsContext
            );

    description:

        This function is called to finally commit all the update.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The status of the operation.

**********************************************************************/
ULONG
DownloadDiagnostics_Commit
    (
        ANSC_HANDLE                 hInsContext
    )
{
    ANSC_STATUS                     returnStatus       = ANSC_STATUS_FAILURE;
    PCOSA_DATAMODEL_DIAG            pMyObject          = (PCOSA_DATAMODEL_DIAG)g_pCosaBEManager->hDiag;
    PDSLH_TR143_DOWNLOAD_DIAG_INFO  pDownloadInfo      = pMyObject->hDiagDownloadInfo;
	char*							pAddrName			= NULL;

    if ( pDownloadInfo->DiagnosticsState != DSLH_TR143_DIAGNOSTIC_Requested )
    {
        pDownloadInfo->DiagnosticsState = DSLH_TR143_DIAGNOSTIC_None;
    }

	if ((pAddrName = CosaGetInterfaceAddrByName(pDownloadInfo->Interface)) != NULL
			&& _ansc_strcmp(pAddrName, "::") != 0)
	{
		AnscCopyString(pDownloadInfo->IfAddrName, pAddrName);
	}
	else
	{
		AnscCopyString(pDownloadInfo->IfAddrName, "");
	}
	if (pAddrName)
		AnscFreeMemory(pAddrName);

    returnStatus = CosaDmlDiagScheduleDiagnostic
                    (
                        DSLH_DIAGNOSTIC_TYPE_Download,
                        (ANSC_HANDLE)pDownloadInfo
                    );

    return 0;
}


/**********************************************************************

    caller:     owner of this object

    prototype:

        ULONG
        DownloadDiagnostics_Rollback
            (
                ANSC_HANDLE                 hInsContext
            );

    description:

        This function is called to roll back the update whenever there's a
        validation found.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The status of the operation.

**********************************************************************/
ULONG
DownloadDiagnostics_Rollback
    (
        ANSC_HANDLE                 hInsContext
    )
{
    PCOSA_DATAMODEL_DIAG            pMyObject          = (PCOSA_DATAMODEL_DIAG)g_pCosaBEManager->hDiag;
    PDSLH_TR143_DOWNLOAD_DIAG_INFO  pDownloadInfo      = pMyObject->hDiagDownloadInfo;
    PDSLH_TR143_DOWNLOAD_DIAG_INFO  pDownloadPreInfo   = NULL;

    if ( !pDownloadInfo )
    {
        return ANSC_STATUS_FAILURE;
    }

    DslhInitDownloadDiagInfo(pDownloadInfo);

    pDownloadPreInfo = (PDSLH_TR143_DOWNLOAD_DIAG_INFO)CosaDmlDiagGetConfigs
                        (
                            DSLH_DIAGNOSTIC_TYPE_Download
                        );

    if ( pDownloadPreInfo )
    {
        AnscCopyString(pDownloadInfo->Interface, pDownloadPreInfo->Interface);
        AnscCopyString(pDownloadInfo->DownloadURL, pDownloadPreInfo->DownloadURL);
        pDownloadInfo->DSCP             = pDownloadPreInfo->DSCP;
        pDownloadInfo->EthernetPriority = pDownloadPreInfo->EthernetPriority;
        pDownloadInfo->DiagnosticsState = pDownloadPreInfo->DiagnosticsState;
    }
    else
    {
       AnscTraceWarning(("Download Diagnostics---Failed to get previous configuration!\n"));
    }

    return 0;
}


/***********************************************************************

 APIs for Object:

    IP.Diagnostics.UploadDiagnostics.

    *  UploadDiagnostics_GetParamBoolValue
    *  UploadDiagnostics_GetParamIntValue
    *  UploadDiagnostics_GetParamUlongValue
    *  UploadDiagnostics_GetParamStringValue
    *  UploadDiagnostics_SetParamBoolValue
    *  UploadDiagnostics_SetParamIntValue
    *  UploadDiagnostics_SetParamUlongValue
    *  UploadDiagnostics_SetParamStringValue
    *  UploadDiagnostics_Validate
    *  UploadDiagnostics_Commit
    *  UploadDiagnostics_Rollback

***********************************************************************/
/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL
        UploadDiagnostics_GetParamBoolValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                BOOL*                       pBool
            );

    description:

        This function is called to retrieve Boolean parameter value;

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                BOOL*                       pBool
                The buffer of returned boolean value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
UploadDiagnostics_GetParamBoolValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL*                       pBool
    )
{
    /* check the parameter name and return the corresponding value */

    /* AnscTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL
        UploadDiagnostics_GetParamIntValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                int*                        pInt
            );

    description:

        This function is called to retrieve integer parameter value;

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                int*                        pInt
                The buffer of returned integer value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
UploadDiagnostics_GetParamIntValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        int*                        pInt
    )
{
    /* check the parameter name and return the corresponding value */

    /* AnscTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL
        UploadDiagnostics_GetParamUlongValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                ULONG*                      puLong
            );

    description:

        This function is called to retrieve ULONG parameter value;

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                ULONG*                      puLong
                The buffer of returned ULONG value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
UploadDiagnostics_GetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        ULONG*                      puLong
    )
{
    PCOSA_DATAMODEL_DIAG            pMyObject        = (PCOSA_DATAMODEL_DIAG)g_pCosaBEManager->hDiag;
    PDSLH_TR143_UPLOAD_DIAG_INFO    pUploadInfo      = pMyObject->hDiagUploadInfo;
    PDSLH_TR143_UPLOAD_DIAG_STATS   pUploadDiagStats = NULL;


    /* check the parameter name and return the corresponding value */
    if ( AnscEqualString(ParamName, "DiagnosticsState", TRUE))
    {
        pUploadDiagStats = (PDSLH_TR143_UPLOAD_DIAG_STATS)CosaDmlDiagGetResults
                            (
                                DSLH_DIAGNOSTIC_TYPE_Upload
                            );

        if ( pUploadDiagStats )
        {
            *puLong = pUploadDiagStats->DiagStates + 1;
        }
        else
        {
            AnscTraceWarning(("Upload Diagnostics---Failed to get DiagnosticsState\n!"));

            *puLong = 0;
            return FALSE;
        }

        return TRUE;
    }

    if( AnscEqualString(ParamName, "DSCP", TRUE))
    {
        if ( pUploadInfo )
        {
            *puLong = pUploadInfo->DSCP;
        }
        else
        {
            AnscTraceWarning(("Upload Diagnostics---Failed to get DSCP \n!"));
            *puLong = 0;
            return FALSE;
        }

        return TRUE;
    }

    if( AnscEqualString(ParamName, "EthernetPriority", TRUE))
    {
        if ( pUploadInfo )
        {
            *puLong = pUploadInfo->EthernetPriority;
        }
        else
        {
            AnscTraceWarning(("Upload Diagnostics---Failed to get EthernetPriority \n!"));
            *puLong = 0;
            return FALSE;
        }

        return TRUE;
    }

    if( AnscEqualString(ParamName, "TestFileLength", TRUE))
    {
        if ( pUploadInfo )
        {
            *puLong = pUploadInfo->TestFileLength;
        }
        else
        {
            AnscTraceWarning(("Upload Diagnostics---Failed to get TestFileLength \n!"));
            *puLong = 0;
            return FALSE;
        }

        return TRUE;
    }

    if( AnscEqualString(ParamName, "TotalBytesSent", TRUE))
    {
        pUploadDiagStats = (PDSLH_TR143_UPLOAD_DIAG_STATS)CosaDmlDiagGetResults
                            (
                                DSLH_DIAGNOSTIC_TYPE_Upload
                            );

        if ( pUploadDiagStats )
        {
            *puLong = pUploadDiagStats->TotalBytesSent;
        }
        else
        {
            AnscTraceWarning(("Upload Diagnostics---Failed to get TotalBytesSent\n!"));

            *puLong = 0;
            return FALSE;
        }

        return TRUE;
    }


    /* AnscTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        ULONG
        UploadDiagnostics_GetParamStringValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                char*                       pValue,
                ULONG*                      pUlSize
            );

    description:

        This function is called to retrieve string parameter value;

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                char*                       pValue,
                The string value buffer;

                ULONG*                      pUlSize
                The buffer of length of string value;
                Usually size of 1023 will be used.
                If it's not big enough, put required size here and return 1;

    return:     0 if succeeded;
                1 if short of buffer size; (*pUlSize = required size)
                -1 if not supported.

**********************************************************************/
ULONG
UploadDiagnostics_GetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pValue,
        ULONG*                      pUlSize
    )
{
    PCOSA_DATAMODEL_DIAG            pMyObject        = (PCOSA_DATAMODEL_DIAG)g_pCosaBEManager->hDiag;
    PDSLH_TR143_UPLOAD_DIAG_INFO    pUploadInfo      = pMyObject->hDiagUploadInfo;
    PDSLH_TR143_UPLOAD_DIAG_STATS   pUploadDiagStats = NULL;
    PANSC_UNIVERSAL_TIME            pTime            = NULL;
    char                            pBuf[128]        = { 0 };


    /* check the parameter name and return the corresponding value */
    if ( AnscEqualString(ParamName, "Interface", TRUE))
    {
        if ( pUploadInfo )
        {
            AnscCopyString(pValue, pUploadInfo->Interface);
        }
        else
        {
            AnscTraceWarning(("Upload Diagnostics---Failed to get Interface \n!"));
            return -1;
        }

        return 0;
    }

    if ( AnscEqualString(ParamName, "UploadURL", TRUE))
    {
        if ( pUploadInfo )
        {
            AnscCopyString(pValue, pUploadInfo->UploadURL);
        }
        else
        {
            AnscTraceWarning(("Upload Diagnostics---Failed to get UploadURL \n!"));
            return -1;
        }

        return 0;
    }

    if ( AnscEqualString(ParamName, "ROMTime", TRUE))
    {
        pUploadDiagStats = (PDSLH_TR143_UPLOAD_DIAG_STATS)CosaDmlDiagGetResults
                            (
                                DSLH_DIAGNOSTIC_TYPE_Upload
                            );

        if ( pUploadDiagStats )
        {
            pTime = (PANSC_UNIVERSAL_TIME)&pUploadDiagStats->ROMTime;

            _ansc_sprintf
            (
                pBuf,
                "%.4d-%.2d-%.2dT%.2d:%.2d:%.2d.%.3d000",
                pTime->Year,
                pTime->Month,
                pTime->DayOfMonth,
                pTime->Hour,
                pTime->Minute,
                pTime->Second,
                pTime->MilliSecond
            );

           AnscCopyString(pValue, pBuf);
        }
       else
       {
            AnscTraceWarning(("Upload Diagnostics---Failed to get ROMTime\n!"));

            return -1;
       }

       return 0;
    }

    if ( AnscEqualString(ParamName, "BOMTime", TRUE))
    {
        pUploadDiagStats = (PDSLH_TR143_UPLOAD_DIAG_STATS)CosaDmlDiagGetResults
                            (
                                DSLH_DIAGNOSTIC_TYPE_Upload
                            );

        if ( pUploadDiagStats )
        {
            pTime = (PANSC_UNIVERSAL_TIME)&pUploadDiagStats->BOMTime;

            _ansc_sprintf
            (
                pBuf,
                "%.4d-%.2d-%.2dT%.2d:%.2d:%.2d.%.3d000",
                pTime->Year,
                pTime->Month,
                pTime->DayOfMonth,
                pTime->Hour,
                pTime->Minute,
                pTime->Second,
                pTime->MilliSecond
            );

            AnscCopyString(pValue, pBuf);
        }
        else
        {
            AnscTraceWarning(("Upload Diagnostics---Failed to get BOMTime\n!"));

            return -1;
        }

        return 0;
    }

    if ( AnscEqualString(ParamName, "EOMTime", TRUE))
    {
        pUploadDiagStats = (PDSLH_TR143_UPLOAD_DIAG_STATS)CosaDmlDiagGetResults
                            (
                                DSLH_DIAGNOSTIC_TYPE_Upload
                            );

        if ( pUploadDiagStats )
        {
            pTime = (PANSC_UNIVERSAL_TIME)&pUploadDiagStats->EOMTime;

            _ansc_sprintf
            (
                pBuf,
                "%.4d-%.2d-%.2dT%.2d:%.2d:%.2d.%.3d000",
                pTime->Year,
                pTime->Month,
                pTime->DayOfMonth,
                pTime->Hour,
                pTime->Minute,
                pTime->Second,
                pTime->MilliSecond
            );

            AnscCopyString(pValue, pBuf);
        }
        else
        {
            AnscTraceWarning(("Upload Diagnostics---Failed to get EOMTime\n!"));

            return -1;
        }

        return 0;
    }

    if ( AnscEqualString(ParamName, "TCPOpenRequestTime", TRUE))
    {
        pUploadDiagStats = (PDSLH_TR143_UPLOAD_DIAG_STATS)CosaDmlDiagGetResults
                            (
                                DSLH_DIAGNOSTIC_TYPE_Upload
                            );

        if ( pUploadDiagStats )
        {
            pTime = (PANSC_UNIVERSAL_TIME)&pUploadDiagStats->TCPOpenRequestTime;

            _ansc_sprintf
            (
                pBuf,
                "%.4d-%.2d-%.2dT%.2d:%.2d:%.2d.%.3d000",
                pTime->Year,
                pTime->Month,
                pTime->DayOfMonth,
                pTime->Hour,
                pTime->Minute,
                pTime->Second,
                pTime->MilliSecond
            );

            AnscCopyString(pValue, pBuf);
        }
        else
        {
            AnscTraceWarning(("Upload Diagnostics---Failed to get TCPOpenRequestTime\n!"));

            return -1;
        }

        return 0;
    }

    if ( AnscEqualString(ParamName, "TCPOpenResponseTime", TRUE))
    {
        pUploadDiagStats = (PDSLH_TR143_UPLOAD_DIAG_STATS)CosaDmlDiagGetResults
                            (
                                DSLH_DIAGNOSTIC_TYPE_Upload
                            );

        if ( pUploadDiagStats )
        {
            pTime = (PANSC_UNIVERSAL_TIME)&pUploadDiagStats->TCPOpenResponseTime;

            _ansc_sprintf
            (
                pBuf,
                "%.4d-%.2d-%.2dT%.2d:%.2d:%.2d.%.3d000",
                pTime->Year,
                pTime->Month,
                pTime->DayOfMonth,
                pTime->Hour,
                pTime->Minute,
                pTime->Second,
                pTime->MilliSecond
            );

            AnscCopyString(pValue, pBuf);
        }
        else
        {
            AnscTraceWarning(("Upload Diagnostics---Failed to get TCPOpenResponseTime\n!"));

            return -1;
        }

        return 0;
    }


    /* AnscTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return -1;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL
        UploadDiagnostics_SetParamBoolValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                BOOL                        bValue
            );

    description:

        This function is called to set BOOL parameter value;

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                BOOL                        bValue
                The updated BOOL value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
UploadDiagnostics_SetParamBoolValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL                        bValue
    )
{
    /* check the parameter name and set the corresponding value */

    /* AnscTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL
        UploadDiagnostics_SetParamIntValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                int                         iValue
            );

    description:

        This function is called to set integer parameter value;

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                int                         iValue
                The updated integer value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
UploadDiagnostics_SetParamIntValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        int                         iValue
    )
{
    /* check the parameter name and set the corresponding value */

    /* AnscTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL
        UploadDiagnostics_SetParamUlongValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                ULONG                       uValue
            );

    description:

        This function is called to set ULONG parameter value;

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                ULONG                       uValue
                The updated ULONG value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
UploadDiagnostics_SetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        ULONG                       uValue
    )
{
    PCOSA_DATAMODEL_DIAG            pMyObject        = (PCOSA_DATAMODEL_DIAG)g_pCosaBEManager->hDiag;
    PDSLH_TR143_UPLOAD_DIAG_INFO    pUploadInfo      = pMyObject->hDiagUploadInfo;
    PDSLH_TR143_UPLOAD_DIAG_STATS   pUploadDiagStats = NULL;

	/* according to TR-181, set writable params other than DiagnosticsState,
	 * must set DiagnosticsState to "NONE". */
	pUploadInfo->DiagnosticsState = DSLH_TR143_DIAGNOSTIC_None;

    /* check the parameter name and set the corresponding value */
    if ( AnscEqualString(ParamName, "DiagnosticsState", TRUE))
    {
        uValue--;
        if ( uValue != (ULONG)DSLH_TR143_DIAGNOSTIC_Requested )
        {
            return FALSE;
        }

        pUploadInfo->DiagnosticsState = DSLH_TR143_DIAGNOSTIC_Requested;
        return TRUE;
    }

    if ( AnscEqualString(ParamName, "DSCP", TRUE))
    {
        pUploadInfo->DSCP = uValue;
        return TRUE;
    }

    if ( AnscEqualString(ParamName, "EthernetPriority", TRUE))
    {
        pUploadInfo->EthernetPriority = uValue;
        return TRUE;
    }

    if ( AnscEqualString(ParamName, "TestFileLength", TRUE))
    {
        if ( uValue == 0 )
        {
            return FALSE;
        }

        pUploadInfo->TestFileLength = uValue;
        return TRUE;
    }

    /* AnscTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL
        UploadDiagnostics_SetParamStringValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                char*                       pString
            );

    description:

        This function is called to set string parameter value;

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                char*                       pString
                The updated string value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
UploadDiagnostics_SetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pString
    )
{
    PCOSA_DATAMODEL_DIAG            pMyObject        = (PCOSA_DATAMODEL_DIAG)g_pCosaBEManager->hDiag;
    PDSLH_TR143_UPLOAD_DIAG_INFO    pUploadInfo      = pMyObject->hDiagUploadInfo;
    PDSLH_TR143_UPLOAD_DIAG_STATS   pUploadDiagStats = NULL;

	/* according to TR-181, set writable params other than DiagnosticsState,
	 * must set DiagnosticsState to "NONE". */
	pUploadInfo->DiagnosticsState = DSLH_TR143_DIAGNOSTIC_None;

    /* check the parameter name and set the corresponding value */
    if ( AnscEqualString(ParamName, "Interface", TRUE))
    {
        AnscCopyString(pUploadInfo->Interface, pString);
        return TRUE;
    }

    if ( AnscEqualString(ParamName, "UploadURL", TRUE))
    {
        if ( !pString || !(*pString) )
        {
            return FALSE;
        }

        AnscCopyString(pUploadInfo->UploadURL, pString);
        return TRUE;
    }


    /* AnscTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL
        UploadDiagnostics_Validate
            (
                ANSC_HANDLE                 hInsContext,
                char*                       pReturnParamName,
                ULONG*                      puLength
            );

    description:

        This function is called to finally commit all the update.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       pReturnParamName,
                The buffer (128 bytes) of parameter name if there's a validation.

                ULONG*                      puLength
                The output length of the param name.

    return:     TRUE if there's no validation.

**********************************************************************/
BOOL
UploadDiagnostics_Validate
    (
        ANSC_HANDLE                 hInsContext,
        char*                       pReturnParamName,
        ULONG*                      puLength
    )
{
    PCOSA_DATAMODEL_DIAG            pMyObject    = (PCOSA_DATAMODEL_DIAG)g_pCosaBEManager->hDiag;
    PDSLH_TR143_UPLOAD_DIAG_INFO    pUploadInfo  = pMyObject->hDiagUploadInfo;

    if ( pUploadInfo->DiagnosticsState == DSLH_TR143_DIAGNOSTIC_Requested
       && !AnscSizeOfString(pUploadInfo->UploadURL) )
    {
        AnscCopyString(pReturnParamName, "UploadURL");
        return FALSE;
    }
    return  TRUE;
}


/**********************************************************************

    caller:     owner of this object

    prototype:

        ULONG
        UploadDiagnostics_Commit
            (
                ANSC_HANDLE                 hInsContext
            );

    description:

        This function is called to finally commit all the update.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The status of the operation.

**********************************************************************/
ULONG
UploadDiagnostics_Commit
    (
        ANSC_HANDLE                 hInsContext
    )
{
    ANSC_STATUS                     returnStatus = ANSC_STATUS_FAILURE;
    PCOSA_DATAMODEL_DIAG            pMyObject    = (PCOSA_DATAMODEL_DIAG)g_pCosaBEManager->hDiag;
    PDSLH_TR143_UPLOAD_DIAG_INFO    pUploadInfo  = pMyObject->hDiagUploadInfo;
	char*							pAddrName			= NULL;

    if ( pUploadInfo->DiagnosticsState != DSLH_TR143_DIAGNOSTIC_Requested )
    {
        pUploadInfo->DiagnosticsState = DSLH_TR143_DIAGNOSTIC_None;
    }

	if ((pAddrName = CosaGetInterfaceAddrByName(pUploadInfo->Interface)) != NULL
			&& _ansc_strcmp(pAddrName, "::") != 0)
	{
		AnscCopyString(pUploadInfo->IfAddrName, pAddrName);
	}
	else
	{
		AnscCopyString(pUploadInfo->IfAddrName, "");
	}
	if (pAddrName)
		AnscFreeMemory(pAddrName);


    returnStatus = CosaDmlDiagScheduleDiagnostic
                    (
                        DSLH_DIAGNOSTIC_TYPE_Upload,
                        (ANSC_HANDLE)pUploadInfo
                    );

    return 0;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        ULONG
        UploadDiagnostics_Rollback
            (
                ANSC_HANDLE                 hInsContext
            );

    description:

        This function is called to roll back the update whenever there's a
        validation found.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The status of the operation.

**********************************************************************/
ULONG
UploadDiagnostics_Rollback
    (
        ANSC_HANDLE                 hInsContext
    )
{
    PCOSA_DATAMODEL_DIAG            pMyObject        = (PCOSA_DATAMODEL_DIAG)g_pCosaBEManager->hDiag;
    PDSLH_TR143_UPLOAD_DIAG_INFO    pUploadInfo      = pMyObject->hDiagUploadInfo;
    PDSLH_TR143_UPLOAD_DIAG_INFO    pUploadPreInfo   = NULL;

    if ( !pUploadInfo )
    {
        return ANSC_STATUS_FAILURE;
    }

    DslhInitUploadDiagInfo(pUploadInfo);

    pUploadPreInfo = (PDSLH_TR143_UPLOAD_DIAG_INFO)CosaDmlDiagGetConfigs
                        (
                            DSLH_DIAGNOSTIC_TYPE_Upload
                        );

    if ( pUploadPreInfo )
    {
        AnscCopyString(pUploadInfo->Interface, pUploadPreInfo->Interface);
        AnscCopyString(pUploadInfo->UploadURL, pUploadPreInfo->UploadURL);
        pUploadInfo->DSCP                 = pUploadPreInfo->DSCP;
        pUploadInfo->EthernetPriority     = pUploadPreInfo->EthernetPriority;
        pUploadInfo->TestFileLength       = pUploadPreInfo->TestFileLength;
        pUploadInfo->DiagnosticsState     = pUploadPreInfo->DiagnosticsState;
    }
    else
    {
        AnscTraceWarning(("Upload Diagnostics---Failed to get previous configuration!\n"));
    }

    return 0;
}


/***********************************************************************

 APIs for Object:

    IP.Diagnostics.UDPEchoConfig.

    *  UDPEchoConfig_GetParamBoolValue
    *  UDPEchoConfig_GetParamIntValue
    *  UDPEchoConfig_GetParamUlongValue
    *  UDPEchoConfig_GetParamStringValue
    *  UDPEchoConfig_SetParamBoolValue
    *  UDPEchoConfig_SetParamIntValue
    *  UDPEchoConfig_SetParamUlongValue
    *  UDPEchoConfig_SetParamStringValue
    *  UDPEchoConfig_Validate
    *  UDPEchoConfig_Commit
    *  UDPEchoConfig_Rollback

***********************************************************************/
/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL
        UDPEchoConfig_GetParamBoolValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                BOOL*                       pBool
            );

    description:

        This function is called to retrieve Boolean parameter value;

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                BOOL*                       pBool
                The buffer of returned boolean value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
UDPEchoConfig_GetParamBoolValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL*                       pBool
    )
{
    PCOSA_DATAMODEL_DIAG            pMyObject     = (PCOSA_DATAMODEL_DIAG)g_pCosaBEManager->hDiag;
    PDSLH_TR143_UDP_ECHO_CONFIG     pUdpEchoInfo  = pMyObject->hDiagUdpechoSrvInfo;
    PDSLH_UDP_ECHO_SERVER_STATS     pUdpEchoStats = NULL;

    /* check the parameter name and return the corresponding value */
    if ( AnscEqualString(ParamName, "Enable", TRUE))
    {
        if ( pUdpEchoInfo )
        {
            *pBool = pUdpEchoInfo->Enable;
        }
        else
        {
            AnscTraceWarning(("UDP echo Diagnostics---Failed to get Enable \n!"));
            *pBool = FALSE;
            return FALSE;
        }

        return TRUE;
    }

    if ( AnscEqualString(ParamName, "EchoPlusEnabled", TRUE))
    {
        if ( pUdpEchoInfo )
        {
            *pBool = pUdpEchoInfo->EchoPlusEnabled;
        }
        else
        {
            AnscTraceWarning(("UDP echo Diagnostics---Failed to get EchoPlusEnabled \n!"));
            *pBool = FALSE;
            return FALSE;
        }

        return TRUE;
    }

    if ( AnscEqualString(ParamName, "EchoPlusSupported", TRUE))
    {
        if ( pUdpEchoInfo )
        {
            *pBool = pUdpEchoInfo->EchoPlusSupported;
        }
        else
        {
            AnscTraceWarning(("UDP echo Diagnostics---Failed to get EchoPlusSupported \n!"));
            *pBool = FALSE;
            return FALSE;
        }

        return TRUE;
    }


    /* AnscTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL
        UDPEchoConfig_GetParamIntValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                int*                        pInt
            );

    description:

        This function is called to retrieve integer parameter value;

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                int*                        pInt
                The buffer of returned integer value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
UDPEchoConfig_GetParamIntValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        int*                        pInt
    )
{
    /* check the parameter name and return the corresponding value */

    /* AnscTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL
        UDPEchoConfig_GetParamUlongValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                ULONG*                      puLong
            );

    description:

        This function is called to retrieve ULONG parameter value;

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                ULONG*                      puLong
                The buffer of returned ULONG value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
UDPEchoConfig_GetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        ULONG*                      puLong
    )
{
    PCOSA_DATAMODEL_DIAG            pMyObject     = (PCOSA_DATAMODEL_DIAG)g_pCosaBEManager->hDiag;
    PDSLH_TR143_UDP_ECHO_CONFIG     pUdpEchoInfo  = pMyObject->hDiagUdpechoSrvInfo;
    PDSLH_UDP_ECHO_SERVER_STATS     pUdpEchoStats = NULL;

    /* check the parameter name and return the corresponding value */

    if ( AnscEqualString(ParamName, "UDPPort", TRUE))
    {
        if ( pUdpEchoInfo )
        {
            *puLong = pUdpEchoInfo->UDPPort;
        }
        else
        {
            AnscTraceWarning(("UDP echo Diagnostics---Failed to get UDPPort \n!"));
            *puLong = 0;
            return FALSE;
        }

        return TRUE;
    }

    if ( AnscEqualString(ParamName, "PacketsReceived", TRUE))
    {
        pUdpEchoStats = (PDSLH_UDP_ECHO_SERVER_STATS)CosaDmlDiagGetResults
                        (
                            DSLH_DIAGNOSTIC_TYPE_UdpEcho
                        );

        if ( pUdpEchoStats )
        {
            *puLong = pUdpEchoStats->PacketsReceived;
        }
        else
        {
            AnscTraceWarning(("UDP echo Diagnostics---Failed to get PacketsReceived\n!"));

            *puLong = 0;
            return FALSE;
        }

        return TRUE;
    }

    if ( AnscEqualString(ParamName, "PacketsResponded", TRUE))
    {
        pUdpEchoStats = (PDSLH_UDP_ECHO_SERVER_STATS)CosaDmlDiagGetResults
                        (
                            DSLH_DIAGNOSTIC_TYPE_UdpEcho
                        );

        if ( pUdpEchoStats )
        {
            *puLong = pUdpEchoStats->PacketsResponded;
        }
        else
        {
            AnscTraceWarning(("UDP echo Diagnostics---Failed to get PacketsResponded\n!"));

            *puLong = 0;
            return FALSE;
        }

        return TRUE;
    }

    if ( AnscEqualString(ParamName, "BytesReceived", TRUE))
    {
        pUdpEchoStats = (PDSLH_UDP_ECHO_SERVER_STATS)CosaDmlDiagGetResults
                        (
                            DSLH_DIAGNOSTIC_TYPE_UdpEcho
                        );

        if ( pUdpEchoStats )
        {
            *puLong = pUdpEchoStats->BytesReceived;
        }
        else
        {
            AnscTraceWarning(("UDP echo Diagnostics---Failed to get BytesReceived\n!"));

            *puLong = 0;
            return FALSE;
        }

        return TRUE;
    }

    if ( AnscEqualString(ParamName, "BytesResponded", TRUE))
    {
        pUdpEchoStats = (PDSLH_UDP_ECHO_SERVER_STATS)CosaDmlDiagGetResults
                        (
                            DSLH_DIAGNOSTIC_TYPE_UdpEcho
                        );

        if ( pUdpEchoStats )
        {
            *puLong = pUdpEchoStats->BytesResponded;
        }
        else
        {
            AnscTraceWarning(("UDP echo Diagnostics---Failed to get BytesResponded\n!"));

            *puLong = 0;
            return FALSE;
        }

        return TRUE;
    }


    /* AnscTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        ULONG
        UDPEchoConfig_GetParamStringValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                char*                       pValue,
                ULONG*                      pUlSize
            );

    description:

        This function is called to retrieve string parameter value;

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                char*                       pValue,
                The string value buffer;

                ULONG*                      pUlSize
                The buffer of length of string value;
                Usually size of 1023 will be used.
                If it's not big enough, put required size here and return 1;

    return:     0 if succeeded;
                1 if short of buffer size; (*pUlSize = required size)
                -1 if not supported.

**********************************************************************/
ULONG
UDPEchoConfig_GetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pValue,
        ULONG*                      pUlSize
    )
{
    PCOSA_DATAMODEL_DIAG            pMyObject     = (PCOSA_DATAMODEL_DIAG)g_pCosaBEManager->hDiag;
    PDSLH_TR143_UDP_ECHO_CONFIG     pUdpEchoInfo  = pMyObject->hDiagUdpechoSrvInfo;
    PDSLH_UDP_ECHO_SERVER_STATS     pUdpEchoStats = NULL;
    PANSC_UNIVERSAL_TIME            pTime         = NULL;
    char                            pBuf[128]     = { 0 };


    /* check the parameter name and return the corresponding value */
    if ( AnscEqualString(ParamName, "Interface", TRUE))
    {
        if ( pUdpEchoInfo )
        {
            AnscCopyString(pValue, pUdpEchoInfo->Interface);
        }
        else
        {
            AnscTraceWarning(("UDP echo Diagnostics---Failed to get Interface\n!"));
            return -1;
        }

        return 0;
    }

    if ( AnscEqualString(ParamName, "SourceIPAddress", TRUE))
    {
        if ( pUdpEchoInfo )
        {
            AnscCopyString(pValue, pUdpEchoInfo->SourceIPName);
        }
        else
        {
            AnscTraceWarning(("UDP echo Diagnostics---Failed to get SourceIPAddress \n!"));
            return -1;
        }

        return 0;
    }

    if ( AnscEqualString(ParamName, "TimeFirstPacketReceived", TRUE))
    {
        pUdpEchoStats = (PDSLH_UDP_ECHO_SERVER_STATS)CosaDmlDiagGetResults
                            (
                                DSLH_DIAGNOSTIC_TYPE_UdpEcho
                            );

        if ( pUdpEchoStats )
        {
            pTime = (PANSC_UNIVERSAL_TIME)&pUdpEchoStats->TimeFirstPacketReceived;

            _ansc_sprintf
            (
                pBuf,
                "%.4d-%.2d-%.2dT%.2d:%.2d:%.2d.%.3d000",
                pTime->Year,
                pTime->Month,
                pTime->DayOfMonth,
                pTime->Hour,
                pTime->Minute,
                pTime->Second,
                pTime->MilliSecond
            );

            AnscCopyString(pValue, pBuf);
        }
        else
        {
            AnscTraceWarning(("UDP echo Diagnostics---Failed to get TimeFirstPacketReceived\n!"));

            return -1;
        }

        return 0;
    }

    if ( AnscEqualString(ParamName, "TimeLastPacketReceived", TRUE))
    {
        pUdpEchoStats = (PDSLH_UDP_ECHO_SERVER_STATS)CosaDmlDiagGetResults
                            (
                                DSLH_DIAGNOSTIC_TYPE_UdpEcho
                            );

        if ( pUdpEchoStats )
        {
            pTime = (PANSC_UNIVERSAL_TIME)&pUdpEchoStats->TimeLastPacketReceived;

            _ansc_sprintf
            (
                pBuf,
                "%.4d-%.2d-%.2dT%.2d:%.2d:%.2d.%.3d000",
                pTime->Year,
                pTime->Month,
                pTime->DayOfMonth,
                pTime->Hour,
                pTime->Minute,
                pTime->Second,
                pTime->MilliSecond
            );

            AnscCopyString(pValue, pBuf);
        }
        else
        {
            AnscTraceWarning(("UDP echo Diagnostics---Failed to get TimeLastPacketReceived\n!"));

            return -1;
        }

        return 0;
    }


    /* AnscTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return -1;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL
        UDPEchoConfig_SetParamBoolValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                BOOL                        bValue
            );

    description:

        This function is called to set BOOL parameter value;

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                BOOL                        bValue
                The updated BOOL value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
UDPEchoConfig_SetParamBoolValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL                        bValue
    )
{
    PCOSA_DATAMODEL_DIAG            pMyObject     = (PCOSA_DATAMODEL_DIAG)g_pCosaBEManager->hDiag;
    PDSLH_TR143_UDP_ECHO_CONFIG     pUdpEchoInfo  = pMyObject->hDiagUdpechoSrvInfo;
    PDSLH_UDP_ECHO_SERVER_STATS     pUdpEchoStats = NULL;

    /* check the parameter name and set the corresponding value */
    if ( AnscEqualString(ParamName, "Enable", TRUE))
    {
        pUdpEchoInfo->Enable = bValue;
        return TRUE;
    }

    if ( AnscEqualString(ParamName, "EchoPlusEnabled", TRUE))
    {
        pUdpEchoInfo->EchoPlusEnabled = bValue;
        return TRUE;
    }

    /* AnscTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL
        UDPEchoConfig_SetParamIntValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                int                         iValue
            );

    description:

        This function is called to set integer parameter value;

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                int                         iValue
                The updated integer value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
UDPEchoConfig_SetParamIntValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        int                         iValue
    )
{
    /* check the parameter name and set the corresponding value */

    /* AnscTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL
        UDPEchoConfig_SetParamUlongValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                ULONG                       uValue
            );

    description:

        This function is called to set ULONG parameter value;

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                ULONG                       uValue
                The updated ULONG value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
UDPEchoConfig_SetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        ULONG                       uValue
    )
{
    PCOSA_DATAMODEL_DIAG            pMyObject     = (PCOSA_DATAMODEL_DIAG)g_pCosaBEManager->hDiag;
    PDSLH_TR143_UDP_ECHO_CONFIG     pUdpEchoInfo  = pMyObject->hDiagUdpechoSrvInfo;
    PDSLH_UDP_ECHO_SERVER_STATS     pUdpEchoStats = NULL;

    /* check the parameter name and set the corresponding value */

    if ( AnscEqualString(ParamName, "UDPPort", TRUE))
    {
        if ( uValue == 0 )
        {
            return FALSE;
        }

        pUdpEchoInfo->UDPPort = uValue;
        return TRUE;
    }


    /* AnscTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL
        UDPEchoConfig_SetParamStringValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                char*                       pString
            );

    description:

        This function is called to set string parameter value;

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                char*                       pString
                The updated string value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
UDPEchoConfig_SetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pString
    )
{
    PCOSA_DATAMODEL_DIAG            pMyObject     = (PCOSA_DATAMODEL_DIAG)g_pCosaBEManager->hDiag;
    PDSLH_TR143_UDP_ECHO_CONFIG     pUdpEchoInfo  = pMyObject->hDiagUdpechoSrvInfo;


    /* check the parameter name and set the corresponding value */
    if ( AnscEqualString(ParamName, "Interface", TRUE))
    {
        AnscCopyString(pUdpEchoInfo->Interface, pString);
        return TRUE;
    }

    if ( AnscEqualString(ParamName, "SourceIPAddress", TRUE))
    {
        AnscCopyString(pUdpEchoInfo->SourceIPName, pString);
        return TRUE;
    }


    /* AnscTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL
        UDPEchoConfig_Validate
            (
                ANSC_HANDLE                 hInsContext,
                char*                       pReturnParamName,
                ULONG*                      puLength
            );

    description:

        This function is called to finally commit all the update.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       pReturnParamName,
                The buffer (128 bytes) of parameter name if there's a validation.

                ULONG*                      puLength
                The output length of the param name.

    return:     TRUE if there's no validation.

**********************************************************************/
BOOL
UDPEchoConfig_Validate
    (
        ANSC_HANDLE                 hInsContext,
        char*                       pReturnParamName,
        ULONG*                      puLength
    )
{
    PCOSA_DATAMODEL_DIAG            pMyObject     = (PCOSA_DATAMODEL_DIAG)g_pCosaBEManager->hDiag;
    PDSLH_TR143_UDP_ECHO_CONFIG     pUdpEchoInfo  = pMyObject->hDiagUdpechoSrvInfo;
    char*                           pAddrName     = NULL;

    if (AnscSizeOfString(pUdpEchoInfo->Interface))    
    {
        pAddrName = CosaGetInterfaceAddrByName(pUdpEchoInfo->Interface);
        if (_ansc_strcmp(pAddrName, "::"))
        {
            AnscCopyString(pUdpEchoInfo->IfAddrName, pAddrName);
            AnscFreeMemory(pAddrName);
        }
        else
        {
            AnscCopyString(pReturnParamName, "Interface");
            AnscFreeMemory(pAddrName);
            return FALSE;  
        }
    }
    else
    {
        AnscCopyString(pUdpEchoInfo->IfAddrName, "::");
    }


    if ( pUdpEchoInfo->EchoPlusEnabled && !pUdpEchoInfo->EchoPlusSupported )
    {
        AnscCopyString(pReturnParamName, "EchoPlusEnabled");
        return FALSE;
    }

    if ( pUdpEchoInfo->Enable && (!pUdpEchoInfo->SourceIPName) )
    {
        AnscCopyString(pReturnParamName, "SourceIPAddress");
        return FALSE;
    }

    if ( pUdpEchoInfo->Enable && (pUdpEchoInfo->UDPPort == 0) )
    {
        AnscCopyString(pReturnParamName, "UDPPort");
        return FALSE;
    }

    return TRUE;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        ULONG
        UDPEchoConfig_Commit
            (
                ANSC_HANDLE                 hInsContext
            );

    description:

        This function is called to finally commit all the update.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The status of the operation.

**********************************************************************/
ULONG
UDPEchoConfig_Commit
    (
        ANSC_HANDLE                 hInsContext
    )
{
    ANSC_STATUS                     returnStatus  = ANSC_STATUS_FAILURE;
    PCOSA_DATAMODEL_DIAG            pMyObject     = (PCOSA_DATAMODEL_DIAG)g_pCosaBEManager->hDiag;
    PDSLH_TR143_UDP_ECHO_CONFIG     pUdpEchoInfo  = pMyObject->hDiagUdpechoSrvInfo;
    PDSLH_UDP_ECHO_SERVER_STATS     pUdpEchoStats = NULL;

    returnStatus = CosaDmlDiagScheduleDiagnostic
                (
                    DSLH_DIAGNOSTIC_TYPE_UdpEcho,
                    (ANSC_HANDLE)pUdpEchoInfo
                );

    return 0;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        ULONG
        UDPEchoConfig_Rollback
            (
                ANSC_HANDLE                 hInsContext
            );

    description:

        This function is called to roll back the update whenever there's a
        validation found.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The status of the operation.

**********************************************************************/
ULONG
UDPEchoConfig_Rollback
    (
        ANSC_HANDLE                 hInsContext
    )
{
    PCOSA_DATAMODEL_DIAG            pMyObject       = (PCOSA_DATAMODEL_DIAG)g_pCosaBEManager->hDiag;
    PDSLH_TR143_UDP_ECHO_CONFIG     pUdpEchoInfo    = pMyObject->hDiagUdpechoSrvInfo;
    PDSLH_TR143_UDP_ECHO_CONFIG     pUdpEchoPreInfo = NULL;

    if ( !pUdpEchoInfo )
    {
        return ANSC_STATUS_FAILURE;
    }

    DslhInitUDPEchoConfig(pUdpEchoInfo);

    pUdpEchoPreInfo = (PDSLH_TR143_UDP_ECHO_CONFIG)CosaDmlDiagGetConfigs
                    (
                        DSLH_DIAGNOSTIC_TYPE_UdpEcho
                    );

    if ( pUdpEchoPreInfo )
    {
        AnscCopyString(pUdpEchoInfo->Interface, pUdpEchoPreInfo->Interface);
        pUdpEchoInfo->Enable               = pUdpEchoPreInfo->Enable;
        AnscCopyString(pUdpEchoInfo->SourceIPName, pUdpEchoPreInfo->SourceIPName);
        pUdpEchoInfo->UDPPort              = pUdpEchoPreInfo->UDPPort;
        pUdpEchoInfo->EchoPlusEnabled      = pUdpEchoPreInfo->EchoPlusEnabled;
        pUdpEchoInfo->EchoPlusSupported    = pUdpEchoPreInfo->EchoPlusSupported;
    }
    else
    {
        AnscTraceWarning(("UDP echo Diagnostics---Failed to get previous configuration!\n"));
    }

    return 0;
}


/***********************************************************************


 APIs for Object:

    IP.Diagnostics.X_RDKCENTRAL-COM_SpeedTest.

    *  SpeedTest_GetParamBoolValue
    *  SpeedTest_GetParamBoolValue
    *  SpeedTest_GetParamStringValue
    *  SpeedTest_SetParamStringValue
  
***********************************************************************/
/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL
        SpeedTest_GetParamBoolValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                BOOL*                       pBool
            );

    description:

        This function is called to retrieve Boolean parameter value;

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                BOOL*                       pBool
                The buffer of returned boolean value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
SpeedTest_GetParamBoolValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL*                       pBool
    )
{
    /* check the parameter name and return the corresponding value */
    if ( AnscEqualString(ParamName, "Enable_Speedtest", TRUE))
    {
	    AnscTraceFlow(("%s Enable_Speedtest : %d\n",  __FUNCTION__, g_enable_speedtest));
	    *pBool = g_enable_speedtest;
	    return TRUE;
    } else
    if ( AnscEqualString(ParamName, "Run", TRUE))
    {
	    AnscTraceFlow(("%s Speedtest Run : %d \n", __FUNCTION__, g_run_speedtest));
	    *pBool = g_run_speedtest;
	    return TRUE;
    } else
    	AnscTraceWarning(("Unsupported parameter '%s'\n", ParamName));
    return FALSE;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL
        SpeedTest_SetParamBoolValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                BOOL                        bValue
            );

    description:

        This function is called to set BOOL parameter value;

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                BOOL                        bValue
                The updated BOOL value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
SpeedTest_SetParamBoolValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL                        bValue
    )
{
    /* check the parameter name and set the corresponding value */
    if ( AnscEqualString(ParamName, "Enable_Speedtest", TRUE))
    {
        AnscTraceFlow(("%s Enable Speedtest : %d \n", __FUNCTION__, bValue));
        g_enable_speedtest = bValue;
        return TRUE;
    }
    else if ( AnscEqualString(ParamName, "Run", TRUE))
    {
        AnscTraceFlow(("%s Run Speedtest : %d \n",__FUNCTION__, bValue));
        g_run_speedtest = bValue;
        return TRUE;
    }

    AnscTraceWarning(("Unsupported parameter '%s'\n", ParamName)); 
    return FALSE;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL
        SpeedTest_Validate
            (
                ANSC_HANDLE                 hInsContext,
                char*                       pReturnParamName,
                ULONG*                      puLength
            );

    description:

        This function is called to finally commit all the update.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       pReturnParamName,
                The buffer (128 bytes) of parameter name if there's a validation.

                ULONG*                      puLength
                The output length of the param name.

    return:     TRUE if there's no validation.

**********************************************************************/
BOOL
SpeedTest_Validate
    (
        ANSC_HANDLE                 hInsContext,
        char*                       pReturnParamName,
        ULONG*                      puLength
    )
{
    if ( g_enable_speedtest == FALSE && g_run_speedtest == TRUE )
    {
        AnscCopyString(pReturnParamName, "Run");
        g_run_speedtest = FALSE;
        return FALSE; 
    }

    return TRUE;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        ULONG
        SpeedTest_Commit
            (
                ANSC_HANDLE                 hInsContext
            );

    description:

        This function is called to finally commit all the update.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The status of the operation.

**********************************************************************/
ULONG
SpeedTest_Commit
    (
        ANSC_HANDLE                 hInsContext
    )
{
    ANSC_STATUS     returnStatus  = ANSC_STATUS_FAILURE;
    BOOL            speedtest_setting = FALSE;

    char buf[128] = {0};
    char cmd[128] = {0};

    memset(buf,0,sizeof(buf));
    if((syscfg_get( NULL, "enable_speedtest", buf, sizeof(buf)) == 0 ) && (buf[0] != '\0') )
    {
            speedtest_setting = (!strcmp(buf, "true")) ? TRUE : FALSE;
    }

    if( g_enable_speedtest != speedtest_setting )
    {
        char buf[8]={0};
        snprintf(buf,sizeof(buf),"%s",g_enable_speedtest ? "true" : "false");
        if (syscfg_set(NULL, "enable_speedtest", buf) != 0)
        {
            AnscTraceWarning(("%s syscfg_set failed  for Enable_Speedtest\n",__FUNCTION__));
            return 1;
        }
        if (syscfg_commit() != 0)
        {
            AnscTraceWarning(("%s syscfg_commit failed for Enable_Speedtest\n",__FUNCTION__));
            return 1;
        }
    }

    if(g_enable_speedtest == TRUE && g_run_speedtest == TRUE)
    {
        memset(cmd, 0, sizeof(cmd));
        AnscCopyString(cmd, "/usr/ccsp/tad/speedtest.sh &");
        AnscTraceFlow(("Executing Speedtest..\n"));
        system(cmd);
        g_run_speedtest = FALSE;
    }

    return 0;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        ULONG
        SpeedTest_Rollback
            (
                ANSC_HANDLE                 hInsContext
            );

    description:

        This function is called to roll back the update whenever there's a
        validation found.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The status of the operation.

**********************************************************************/
ULONG
SpeedTest_Rollback
    (
        ANSC_HANDLE                 hInsContext
    )
{
    char buf[128];

    memset(buf,0,sizeof(buf));
    if((syscfg_get( NULL, "enable_speedtest", buf, sizeof(buf)) == 0 ) && (buf[0] != '\0') )
    {
            g_enable_speedtest = (!strcmp(buf, "true")) ? TRUE : FALSE;
    }

    g_run_speedtest = FALSE;

    return 0;
}


/**********************************************************************

    caller:     owner of this object

    prototype:

        ULONG
        SpeedTest_GetParamStringValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                char*                       pValue,
                ULONG*                      pUlSize
            );

    description:

        This function is called to retrieve string parameter value;

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                char*                       pValue,
                The string value buffer;

                ULONG*                      pUlSize
                The buffer of length of string value;
                Usually size of 256 will be used.
                If it's not big enough, put required size here and return 1;

    return:     0 if succeeded;
                1 if short of buffer size; (*pUlSize = required size)
                -1 if not supported.

**********************************************************************/
ULONG
SpeedTest_GetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pValue,
        ULONG*                      pUlSize
    )
{
    int len = strlen(g_argument_speedtest);
    /* check the parameter name and return the corresponding value */
    if ( AnscEqualString(ParamName, "Argument", TRUE))
    {
        if (  *pUlSize > SPEEDTEST_ARG_SIZE )
        {
		AnscTraceFlow(("SpeedTest Argument get : %s : len :%d: *pUlSize:%d: \n",g_argument_speedtest,len,*pUlSize));
		AnscCopyString(pValue, g_argument_speedtest);
		return 0;
        } else
	{
		AnscTraceWarning(("SpeedTest Argument get :  Incorrect size: %s: current_string_size:%d:  size of buffer :%d: \n",g_argument_speedtest,len, *pUlSize));
		return 1;

	}
    }
    else
    {

		AnscTraceWarning(("SpeedTest Argument get :Unsupported parameter '%s'\n", ParamName));
		return -1;
    } 
}


/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL
        SpeedTest_SetParamStringValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                char*                       pString
            );

    description:

        This function is called to set string parameter value;

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                char*                       pString
                The updated string value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
SpeedTest_SetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pString
    )
{

    int len = strlen(pString);
    /* check the parameter name and set the corresponding value */
    if ( AnscEqualString(ParamName, "Argument", TRUE))
    {
	if ( len <= (SPEEDTEST_ARG_SIZE ) ){
		AnscTraceFlow(("SpeedTest Argument set : %s : string len : %d: \n",pString,len));
		AnscCopyString(g_argument_speedtest, pString);
		return TRUE;
	} else
	{
		AnscTraceWarning(("SpeedTest Argument set : string too long:  %s : string len : %d: \n",pString,len));
		return FALSE;
	}
    } else
    {
     AnscTraceWarning(("SpeedTest Argument set : Unsupported parameter '%s'\n", ParamName));
    }
    return FALSE;
}

