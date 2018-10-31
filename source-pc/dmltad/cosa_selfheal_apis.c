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


/**************************************************************************

    module: cosa_diagnostic_apis.c

        For COSA Data Model Library Development

    -------------------------------------------------------------------

    copyright:

        Cisco Systems, Inc.
        All Rights Reserved.

    -------------------------------------------------------------------

    description:

        This file implementes back-end apis for the COSA Data Model Library

        *  CosaDiagCreate
        *  CosaDiagInitialize
        *  CosaDiagRemove
    -------------------------------------------------------------------

    environment:

        platform independent

    -------------------------------------------------------------------

    author:

        COSA XML TOOL CODE GENERATOR 1.0

    -------------------------------------------------------------------

    revision:

        01/11/2011    initial revision.

**************************************************************************/
#include "plugin_main_apis.h"
#include "cosa_selfheal_apis.h"
#include "cosa_logbackup_dml.h"

static char *Ipv4_Server ="Ipv4_PingServer_%d";
static char *Ipv6_Server ="Ipv6_PingServer_%d";

void copy_command_output(char * cmd, char * out, int len)
{
    FILE * fp;
    char   buf[256];
    char * p;

    fp = popen(cmd, "r");

    if (fp)
    {
        fgets(buf, sizeof(buf), fp);

        /*we need to remove the \n char in buf*/
        if ((p = strchr(buf, '\n'))) *p = 0;

        strncpy(out, buf, len-1);

        pclose(fp);
    }

}

int SyncServerlistInDb(PingServerType type, int EntryCount)
{
	int urlIndex =0;
	int i =0;
	int j =0;
	for(urlIndex=1; urlIndex <= EntryCount; urlIndex++ )
	{
		char uri[256];
		char recName[64];
		memset(uri,0,sizeof(uri));
		memset(recName,0,sizeof(recName));
		if(type == PingServerType_IPv4)
		{
			sprintf(recName, Ipv4_Server, urlIndex);
		}
		else
		{
			sprintf(recName, Ipv6_Server, urlIndex);
		}
		syscfg_get( NULL, recName, uri, sizeof(uri));
		if(strcmp(uri,""))
		{
			i++;
		}
		else
		{
			j = urlIndex+1;
			while(j <= EntryCount)
			{
				memset(recName,0,sizeof(recName));
				memset(uri,0,sizeof(uri));
				if(type == PingServerType_IPv4)
				{
					sprintf(recName, Ipv4_Server, j);
				}
				else
				{
					sprintf(recName, Ipv6_Server, j);
				}
				syscfg_get( NULL, recName, uri, sizeof(uri));
				if(strcmp(uri,""))
				{
					/* copy the URL  of index j to index urlIndex. Remove entery of index j */
					SavePingServerURI(type, uri, urlIndex);
					RemovePingServerURI(type,j);
					i++;
					break;
				}
				else
				{
					j++;
				}

			}
		}

	}
		char buf[8];
		memset(buf,0,sizeof(buf));
		snprintf(buf,sizeof(buf),"%d",i);
		if(type == PingServerType_IPv4)
		{
			if (syscfg_set(NULL, "Ipv4PingServer_Count", buf) != 0) 
			{
				CcspTraceWarning(("syscfg_set failed\n"));
			}
			else 
			{
				if (syscfg_commit() != 0) 
				{
					CcspTraceWarning(("syscfg_commit failed\n"));
				}
			}
		}
		else
		{
			if (syscfg_set(NULL, "Ipv6PingServer_Count", buf) != 0) 
			{
				CcspTraceWarning(("syscfg_set failed\n"));
			}
			else 
			{
				if (syscfg_commit() != 0) 
				{
					CcspTraceWarning(("syscfg_commit failed\n"));
				}
			}

		}
	return i;
}
void FillEntryInList(PCOSA_DATAMODEL_SELFHEAL pSelfHeal,PCOSA_CONTEXT_SELFHEAL_LINK_OBJECT   pSelfHealCxtLink,PingServerType type)
{
	PCOSA_DML_SELFHEAL_IPv4_SERVER_TABLE pServerIpv4 = NULL;
	PCOSA_DML_SELFHEAL_IPv4_SERVER_TABLE pServerIpv6 = NULL;
	int Qdepth = 0;
    pSelfHealCxtLink = (PCOSA_CONTEXT_SELFHEAL_LINK_OBJECT)AnscAllocateMemory(sizeof(COSA_CONTEXT_SELFHEAL_LINK_OBJECT));
    if ( !pSelfHealCxtLink )
    {
        return;
    }
	if(type == PingServerType_IPv4)
	{
		Qdepth = AnscSListQueryDepth( &pSelfHeal->IPV4PingServerList );
		/* now we have this link content */
		pSelfHealCxtLink->hContext = (ANSC_HANDLE)pSelfHeal->pConnTest->pIPv4Table;
		pSelfHealCxtLink->InstanceNumber =  pSelfHeal->ulIPv4NextInstanceNumber;
		pSelfHeal->pConnTest->pIPv4Table[Qdepth].InstanceNumber =  pSelfHeal->ulIPv4NextInstanceNumber;
		pSelfHeal->ulIPv4NextInstanceNumber++;
		if (pSelfHeal->ulIPv4NextInstanceNumber == 0)
			pSelfHeal->ulIPv4NextInstanceNumber = 1;
	
		pServerIpv4 = &pSelfHeal->pConnTest->pIPv4Table[Qdepth];
		pSelfHealCxtLink->hContext = (ANSC_HANDLE)pServerIpv4;
		CosaSListPushEntryByInsNum(&pSelfHeal->IPV4PingServerList, (PCOSA_CONTEXT_LINK_OBJECT)pSelfHealCxtLink);
	}
	else
	{
		Qdepth = AnscSListQueryDepth( &pSelfHeal->IPV6PingServerList );
		/* now we have this link content */
		pSelfHealCxtLink->hContext = (ANSC_HANDLE)pSelfHeal->pConnTest->pIPv6Table;
		pSelfHealCxtLink->InstanceNumber =  pSelfHeal->ulIPv6NextInstanceNumber;
		pSelfHeal->pConnTest->pIPv6Table[Qdepth].InstanceNumber =  pSelfHeal->ulIPv6NextInstanceNumber;
		pSelfHeal->ulIPv6NextInstanceNumber++;
		if (pSelfHeal->ulIPv6NextInstanceNumber == 0)
			pSelfHeal->ulIPv6NextInstanceNumber = 1;
		
		pServerIpv6 = &pSelfHeal->pConnTest->pIPv6Table[Qdepth];
		pSelfHealCxtLink->hContext = (ANSC_HANDLE)pServerIpv6;
		CosaSListPushEntryByInsNum(&pSelfHeal->IPV6PingServerList, (PCOSA_CONTEXT_LINK_OBJECT)pSelfHealCxtLink);
	}
 
}
PCOSA_DML_RESOUCE_MONITOR
CosaDmlGetSelfHealMonitorCfg(    
        ANSC_HANDLE                 hThisObject
    )
{
    PCOSA_DATAMODEL_SELFHEAL      pMyObject            = (PCOSA_DATAMODEL_SELFHEAL)hThisObject;
    PCOSA_DML_RESOUCE_MONITOR     pRescTest            = (PCOSA_DML_CONNECTIVITY_TEST)NULL;

    pRescTest = (PCOSA_DATAMODEL_SELFHEAL)AnscAllocateMemory(sizeof(COSA_DATAMODEL_SELFHEAL));
    if(!pRescTest) {
        printf("\n %s Resource monitor allocation failed\n",__FUNCTION__);
        return NULL;
    }
    char buf[8];
    memset(buf, 0, sizeof(buf));
    syscfg_get(NULL, "resource_monitor_interval", buf, sizeof(buf));
    pRescTest->MonIntervalTime = atoi(buf);

    memset(buf, 0, sizeof(buf));
    syscfg_get(NULL, "avg_cpu_threshold", buf, sizeof(buf));
    pRescTest->AvgCpuThreshold = atoi(buf);

    memset(buf, 0, sizeof(buf));
    syscfg_get(NULL, "avg_memory_threshold", buf, sizeof(buf));
    pRescTest->AvgMemThreshold = atoi(buf);

    return pRescTest;
}

PCOSA_DML_CONNECTIVITY_TEST
CosaDmlGetSelfHealCfg(    
        ANSC_HANDLE                 hThisObject
    )
{
	PCOSA_DATAMODEL_SELFHEAL      pMyObject            = (PCOSA_DATAMODEL_SELFHEAL)hThisObject;
	PCOSA_DML_CONNECTIVITY_TEST    pConnTest            = (PCOSA_DML_CONNECTIVITY_TEST)NULL;
	PCOSA_CONTEXT_SELFHEAL_LINK_OBJECT   pSelfHealCxtLink = NULL;
	int i=0;
	int urlIndex;
	char recName[64];
	char buf[10];
	ULONG entryCountIPv4 = 0;
	ULONG entryCountIPv6 = 0;

	syscfg_init();
	get_logbackupcfg();
	pConnTest = (PCOSA_DML_CONNECTIVITY_TEST)AnscAllocateMemory(sizeof(COSA_DML_CONNECTIVITY_TEST));
	pMyObject->pConnTest = pConnTest;
	memset(buf,0,sizeof(buf));
	syscfg_get( NULL, "selfheal_enable", buf, sizeof(buf));
	pMyObject->Enable = (!strcmp(buf, "true")) ? TRUE : FALSE;
        if ( pMyObject->Enable == TRUE )
        {
            system("/fss/gw/usr/ccsp/tad/self_heal_connectivity_test.sh &");
            //system("/fss/gw/usr/ccsp/tad/resource_monitor.sh &"); //RDKB-EMU
	}  

	memset(buf,0,sizeof(buf));
	syscfg_get( NULL, "max_reboot_count", buf, sizeof(buf));
	pMyObject->MaxRebootCnt = atoi(buf);

	memset(buf,0,sizeof(buf));
	syscfg_get( NULL, "max_reset_count", buf, sizeof(buf));
	/* RDKB-13228 */
	pMyObject->MaxResetCnt = atoi(buf);

	memset(buf,0,sizeof(buf));
	syscfg_get( NULL, "ConnTest_PingInterval", buf, sizeof(buf));
	pConnTest->PingInterval = atoi(buf);
	memset(buf,0,sizeof(buf));
	syscfg_get( NULL, "ConnTest_NumPingsPerServer", buf, sizeof(buf));
	pConnTest->PingCount = atoi(buf);
	memset(buf,0,sizeof(buf));
	syscfg_get( NULL, "ConnTest_MinNumPingServer", buf, sizeof(buf));
	pConnTest->MinPingServer = atoi(buf);
	memset(buf,0,sizeof(buf));
	syscfg_get( NULL, "ConnTest_PingRespWaitTime", buf, sizeof(buf));
	pConnTest->WaitTime = atoi(buf);

	memset(buf,0,sizeof(buf));
	syscfg_get( NULL, "ConnTest_CorrectiveAction", buf, sizeof(buf));
	pConnTest->CorrectiveAction = (!strcmp(buf, "true")) ? TRUE : FALSE;

    memset(buf,0,sizeof(buf));
    syscfg_get( NULL, "router_reboot_Interval", buf, sizeof(buf));
    pConnTest->RouterRebootInterval = atoi(buf);

	memset(buf,0,sizeof(buf));
	syscfg_get( NULL, "Ipv4PingServer_Count", buf, sizeof(buf));
	pConnTest->IPv4EntryCount = atoi(buf);
	entryCountIPv4 = AnscSListQueryDepth(&pMyObject->IPV4PingServerList);
	pConnTest->IPv4EntryCount = SyncServerlistInDb(PingServerType_IPv4, pConnTest->IPv4EntryCount);
	if ( pConnTest->IPv4EntryCount != 0 )
	{
		pConnTest->pIPv4Table     = (PCOSA_DML_SELFHEAL_IPv4_SERVER_TABLE)AnscAllocateMemory(sizeof(COSA_DML_SELFHEAL_IPv4_SERVER_TABLE) * pConnTest->IPv4EntryCount);
	}

	for(urlIndex=1; urlIndex <= pConnTest->IPv4EntryCount; urlIndex++ )
	{
		char uri[256];
		memset(uri,0,sizeof(uri));
		memset(recName,0,sizeof(recName));
		sprintf(recName, Ipv4_Server, urlIndex);
		syscfg_get( NULL, recName, uri, sizeof(uri));
		if(strcmp(uri,""))
		{
			strcpy(pConnTest->pIPv4Table[i].Ipv4PingServerURI,uri);
		}
		i++;
		/* Push entery in the IPv4 queue */
		FillEntryInList(pMyObject,pSelfHealCxtLink,PingServerType_IPv4);
	}
	memset(buf,0,sizeof(buf));
	syscfg_get( NULL, "Ipv6PingServer_Count", buf, sizeof(buf));
	pConnTest->IPv6EntryCount = atoi(buf);
	entryCountIPv6 = AnscSListQueryDepth(&pMyObject->IPV6PingServerList);
	pConnTest->IPv6EntryCount = SyncServerlistInDb(PingServerType_IPv6,pConnTest->IPv6EntryCount);
	if ( pConnTest->IPv6EntryCount != 0 )
	{
		pConnTest->pIPv6Table = (PCOSA_DML_SELFHEAL_IPv6_SERVER_TABLE)AnscAllocateMemory(sizeof(COSA_DML_SELFHEAL_IPv6_SERVER_TABLE) * pConnTest->IPv6EntryCount);
	}
	i=0;
	for(urlIndex=1; urlIndex <= pConnTest->IPv6EntryCount; urlIndex++ )
	{
		char uri[256];
		memset(uri,0,sizeof(uri));
		memset(recName,0,sizeof(recName));
		sprintf(recName, Ipv6_Server, urlIndex);
		syscfg_get( NULL, recName, uri, sizeof(uri));
		if(strcmp(uri,""))
		{
			strcpy(pConnTest->pIPv6Table[i].Ipv6PingServerURI,uri);
		}
		i++;
		/* Push entery in the IPv6 queue */
		FillEntryInList(pMyObject,pSelfHealCxtLink,PingServerType_IPv6);
	}
	return pConnTest;
}

/**********************************************************************

    caller:     owner of the object

    prototype:

        ANSC_HANDLE
        CosaDiagCreate
            (
                VOID
            );

    description:

        This function constructs cosa SelfHeal object and return handle.

    argument:

    return:     newly created nat object.

**********************************************************************/

ANSC_HANDLE
CosaSelfHealCreate
    (
        VOID
    )
{
    ANSC_STATUS                     returnStatus = ANSC_STATUS_SUCCESS;
    PCOSA_DATAMODEL_SELFHEAL            pMyObject    = (PCOSA_DATAMODEL_SELFHEAL)NULL;

    /*
     * We create object by first allocating memory for holding the variables and member functions.
     */
    pMyObject = (PCOSA_DATAMODEL_SELFHEAL)AnscAllocateMemory(sizeof(COSA_DATAMODEL_SELFHEAL));
    if ( !pMyObject )
    {
        return  (ANSC_HANDLE)NULL;
    }

    /*
     * Initialize the common variables and functions for a container object.
     */
    pMyObject->Oid               = COSA_DATAMODEL_DIAG_OID;
    pMyObject->Create            = CosaSelfHealCreate;
    pMyObject->Remove            = CosaSelfHealRemove;
    pMyObject->Initialize        = CosaSelfHealInitialize;

    pMyObject->Initialize   ((ANSC_HANDLE)pMyObject);
    return  (ANSC_HANDLE)pMyObject;
}

/**********************************************************************

    caller:     self

    prototype:

        ANSC_STATUS
        CosaSelfHealInitialize
            (
                ANSC_HANDLE                 hThisObject
            );

    description:

        This function initiate  cosa SelfHeal object and return handle.

    argument:   ANSC_HANDLE                 hThisObject
                This handle is actually the pointer of this object
                itself.

    return:     operation status.

**********************************************************************/

ANSC_STATUS
CosaSelfHealInitialize
    (
        ANSC_HANDLE                 hThisObject
    )
{
    ANSC_STATUS                     returnStatus         = ANSC_STATUS_SUCCESS;
    PCOSA_DATAMODEL_SELFHEAL            pMyObject            = (PCOSA_DATAMODEL_SELFHEAL )hThisObject;
	PCOSA_DML_CONNECTIVITY_TEST    pConnTest            = (PCOSA_DML_CONNECTIVITY_TEST)NULL;
	
    /* Initiation all functions */
    AnscSListInitializeHeader( &pMyObject->IPV4PingServerList );
    AnscSListInitializeHeader( &pMyObject->IPV6PingServerList );
    pMyObject->MaxInstanceNumber        = 0;
    pMyObject->ulIPv4NextInstanceNumber   = 1;
	pMyObject->ulIPv6NextInstanceNumber   = 1;
    pMyObject->PreviousVisitTime        = 0;
	
    pMyObject->pConnTest = CosaDmlGetSelfHealCfg((ANSC_HANDLE)pMyObject);
    pMyObject->pResMonitor = CosaDmlGetSelfHealMonitorCfg((ANSC_HANDLE)pMyObject);

    if ( returnStatus != ANSC_STATUS_SUCCESS )
    {
        return returnStatus;
    }

    return returnStatus;
}

/**********************************************************************

    caller:     self

    prototype:

        ANSC_STATUS
        CosaDiagRemove
            (
                ANSC_HANDLE                 hThisObject
            );

    description:

        This function initiate  cosa SelfHeal object and return handle.

    argument:   ANSC_HANDLE                 hThisObject
                This handle is actually the pointer of this object
                itself.

    return:     operation status.

**********************************************************************/

ANSC_STATUS
CosaSelfHealRemove
    (
        ANSC_HANDLE                 hThisObject
    )
{
    ANSC_STATUS                     returnStatus = ANSC_STATUS_SUCCESS;
    PCOSA_DATAMODEL_SELFHEAL            pMyObject    = (PCOSA_DATAMODEL_SELFHEAL)hThisObject;

    /* Remove necessary resounce */
    if ( pMyObject->pConnTest->pIPv4Table)
    {
        AnscFreeMemory(pMyObject->pConnTest->pIPv4Table );
    } 

    if ( pMyObject->pConnTest->pIPv6Table)
    {
        AnscFreeMemory(pMyObject->pConnTest->pIPv6Table );
    } 

    if ( pMyObject->pConnTest)
    {
        AnscFreeMemory(pMyObject->pConnTest);
    } 
    if ( pMyObject->pResMonitor )
    {
        AnscFreeMemory( pMyObject->pResMonitor );
    }

    /* Remove self */
    AnscFreeMemory((ANSC_HANDLE)pMyObject);
    return returnStatus;
}
void SavePingServerURI(PingServerType type, char *URL, int InstNum)
{
		char uri[256];
		char recName[256];
		memset(uri,0,sizeof(uri));
		AnscCopyString(uri,URL);
		memset(recName,0,sizeof(recName));
		if(type == PingServerType_IPv4)
		{
			sprintf(recName, Ipv4_Server, InstNum);
		}
		else
		{
			sprintf(recName, Ipv6_Server, InstNum);
		}
		if (syscfg_set(NULL, recName, uri) != 0) 
		{
			CcspTraceWarning(("syscfg_set failed\n"));
		}
		else 
		{
			if (syscfg_commit() != 0) 
			{
				CcspTraceWarning(("syscfg_commit failed\n"));
			}
		}
}

ANSC_STATUS RemovePingServerURI(PingServerType type, int InstNum)
{
		ANSC_STATUS                     returnStatus = ANSC_STATUS_SUCCESS;
		char recName[256];
		memset(recName,0,sizeof(recName));
		if(type == PingServerType_IPv4)
		{
			sprintf(recName, Ipv4_Server, InstNum);
		}
		else
		{
			sprintf(recName, Ipv6_Server, InstNum);
		}
		if (syscfg_unset(NULL, recName) != 0) 
		{
			CcspTraceWarning(("syscfg_set failed\n"));
			return ANSC_STATUS_FAILURE;
		}
		else 
		{
			if (syscfg_commit() != 0) 
			{
				CcspTraceWarning(("syscfg_commit failed\n"));
				return ANSC_STATUS_FAILURE;
			}
		}
		return returnStatus;
}


