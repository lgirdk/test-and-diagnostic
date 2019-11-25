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

    module: cosa_diagnostic_apis.c

        For COSA Data Model Library Development

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

void  _get_db_value(char * cmd, char * out, int len, char * val)
{
        FILE * fp = NULL;
        char   buf[512] = {0},tem_buf[512] = {0};
        char * p = NULL;
	int i = 0;

        fp = fopen(cmd, "r");

        if (fp)
        {
                while(fgets(buf, sizeof(buf), fp) != NULL){
                        if((strstr(buf,val)) != NULL){
                                p = strtok(buf,"=");
                                p = strtok(NULL, "=");
                                strncpy(tem_buf, p, len-1);
				for(i=0; tem_buf[i] != '\n'; i++)
					out[i] = tem_buf[i];
				out[i]='\0';
                        }
                }
        }
        fclose(fp);
}

void  _set_db_value(char *file_name,char *current_string,char *value)
{
	FILE *fp = NULL;
	char path[1024] = {0},buf[512] = {0},updated_str[512]={0},cmd[512]={0};
	int count = 0;
	char *ch = NULL;
	sprintf(cmd,"%s=%s",current_string,value);
	fp = fopen(file_name,"r");
	if(fp)
	{
		while(fgets(path,sizeof(path),fp) != NULL)
		{
			ch = strstr(path,current_string);
			if(ch != NULL)
			{
				for(count=0;path[count]!='\n';count++)
					updated_str[count] = path[count];
				updated_str[count]='\0';
				sprintf(buf,"sed -i \"s/%s/%s/g\" %s",updated_str,cmd,file_name);
				system(buf);
			}
		}
		if(ch == NULL)
		{
			sprintf(buf,"sed -i '$ a %s' %s",cmd,file_name);
			system(buf);
		}
	}
	fclose(fp);
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
			/*if (syscfg_set(NULL, "Ipv4PingServer_Count", buf) != 0) 
			{
				CcspTraceWarning(("syscfg_set failed\n"));
			}
			else 
			{
				if (syscfg_commit() != 0) 
				{
					CcspTraceWarning(("syscfg_commit failed\n"));
				}
			}*/
		        _set_db_value(SYSCFG_FILE,"Ipv4PingServer_Count",buf);
		}
		else
		{
		        _set_db_value(SYSCFG_FILE,"Ipv6PingServer_Count",buf);
			/*if (syscfg_set(NULL, "Ipv6PingServer_Count", buf) != 0) 
			{
				CcspTraceWarning(("syscfg_set failed\n"));
			}
			else 
			{
				if (syscfg_commit() != 0) 
				{
					CcspTraceWarning(("syscfg_commit failed\n"));
				}
			}*/

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
     _get_db_value(SYSCFG_FILE, buf, sizeof(buf), "resource_monitor_interval");
    pRescTest->MonIntervalTime = atoi(buf);

    memset(buf, 0, sizeof(buf));
     _get_db_value(SYSCFG_FILE, buf, sizeof(buf), "avg_cpu_threshold");
    pRescTest->AvgCpuThreshold = atoi(buf);

    memset(buf, 0, sizeof(buf));
    _get_db_value(SYSCFG_FILE, buf, sizeof(buf), "avg_memory_threshold");
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
	char buf[10] ,dnsURL[512];
	ULONG entryCountIPv4 = 0;
	ULONG entryCountIPv6 = 0;

	syscfg_init();
	get_logbackupcfg();
	pConnTest = (PCOSA_DML_CONNECTIVITY_TEST)AnscAllocateMemory(sizeof(COSA_DML_CONNECTIVITY_TEST));
	pMyObject->pConnTest = pConnTest;
	memset(buf,0,sizeof(buf));
	 _get_db_value(SYSCFG_FILE, buf, sizeof(buf), "selfheal_enable");
	pMyObject->Enable = (!strcmp(buf, "true")) ? TRUE : FALSE;
        if ( pMyObject->Enable == TRUE )
        {
            system("/fss/gw/usr/ccsp/tad/self_heal_connectivity_test.sh &");
            //system("/fss/gw/usr/ccsp/tad/resource_monitor.sh &"); //RDKB-EMU
	}  

	memset(buf,0,sizeof(buf));
	_get_db_value(SYSCFG_FILE, buf, sizeof(buf), "max_reboot_count");
	pMyObject->MaxRebootCnt = atoi(buf);

	memset(buf,0,sizeof(buf));
	_get_db_value(SYSCFG_FILE, buf, sizeof(buf), "max_reset_count");
	/* RDKB-13228 */
	pMyObject->MaxResetCnt = atoi(buf);

	memset(buf,0,sizeof(buf));
        _get_db_value(SYSCFG_FILE, buf, sizeof(buf), "selfheal_dns_pingtest_enable");
	pMyObject->DNSPingTest_Enable = (!strcmp(buf, "true")) ? TRUE : FALSE;

	memset(dnsURL,0,sizeof(dnsURL));
        _get_db_value(SYSCFG_FILE,dnsURL, sizeof(dnsURL), "selfheal_dns_pingtest_url");
        if( '\0' != dnsURL[ 0 ] )
        {
                AnscCopyString(pMyObject->DNSPingTest_URL, dnsURL);
        }
        else
        {
                pMyObject->DNSPingTest_URL[ 0 ] = '\0';
        }


	memset(buf,0,sizeof(buf));
        _get_db_value(SYSCFG_FILE, buf, sizeof(buf), "ConnTest_PingInterval");
	pConnTest->PingInterval = atoi(buf);
	memset(buf,0,sizeof(buf));
        _get_db_value(SYSCFG_FILE, buf, sizeof(buf), "ConnTest_NumPingsPerServer");
	pConnTest->PingCount = atoi(buf);
	memset(buf,0,sizeof(buf));
        _get_db_value(SYSCFG_FILE, buf, sizeof(buf), "ConnTest_MinNumPingServer");
	pConnTest->MinPingServer = atoi(buf);
	memset(buf,0,sizeof(buf));
        _get_db_value(SYSCFG_FILE, buf, sizeof(buf), "ConnTest_PingRespWaitTime");
	pConnTest->WaitTime = atoi(buf);

	memset(buf,0,sizeof(buf));
        _get_db_value(SYSCFG_FILE, buf, sizeof(buf), "ConnTest_CorrectiveAction");
	pConnTest->CorrectiveAction = (!strcmp(buf, "true")) ? TRUE : FALSE;

    memset(buf,0,sizeof(buf));
    _get_db_value(SYSCFG_FILE, buf, sizeof(buf), "router_reboot_Interval");
    pConnTest->RouterRebootInterval = atoi(buf);

	memset(buf,0,sizeof(buf));
        _get_db_value(SYSCFG_FILE, buf, sizeof(buf), "Ipv6PingServer_Count");
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
        _get_db_value(SYSCFG_FILE, buf, sizeof(buf), "Ipv6PingServer_Count");
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
		_set_db_value(SYSCFG_FILE,recName,uri);
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

ANSC_STATUS CosaDmlModifySelfHealDNSPingTestStatus( ANSC_HANDLE hThisObject,
                                                                                                                            BOOL        bValue )
{
        ANSC_STATUS             ReturnStatus    = ANSC_STATUS_SUCCESS;
        BOOLEAN                 bProcessFurther = TRUE;

        /* Validate received param  */
        if( NULL == hThisObject )
        {
                bProcessFurther = FALSE;
                ReturnStatus    = ANSC_STATUS_FAILURE;
                CcspTraceError(("[%s] hThisObject is NULL\n", __FUNCTION__ ));
        }

        if( bProcessFurther )
        {
                PCOSA_DATAMODEL_SELFHEAL          pMyObject  = (PCOSA_DATAMODEL_SELFHEAL)hThisObject;

                /* Modify the DNS ping test flag */
         /*       if ( 0 == syscfg_set( NULL,
                                                          "selfheal_dns_pingtest_enable",
                                                          ( ( bValue == TRUE ) ? "true" : "false" ) ) )
                {
                        if ( 0 == syscfg_commit( ) )
                        {
                                pMyObject->DNSPingTest_Enable = bValue;
                        }
                }*/

		 _set_db_value(SYSCFG_FILE,"selfheal_dns_pingtest_enable",( ( bValue == TRUE ) ? "true" : "false" ) );
                CcspTraceInfo(("[%s] DNSPingTest_Enable:[ %d ]\n",
                                                                        __FUNCTION__,
                                                                        pMyObject->DNSPingTest_Enable ));
        }

        return ReturnStatus;
}

ANSC_STATUS CosaDmlModifySelfHealDNSPingTestURL( ANSC_HANDLE hThisObject,
                                                                                                                          PCHAR           pString )
{
        ANSC_STATUS             ReturnStatus    = ANSC_STATUS_SUCCESS;
        BOOLEAN                 bProcessFurther = TRUE;

        /* Validate received param  */
        if(( NULL == hThisObject ) || \
           ( NULL == pString )
           )
        {
                bProcessFurther = FALSE;
                ReturnStatus    = ANSC_STATUS_FAILURE;
                CcspTraceError(("[%s] hThisObject/pString is NULL\n", __FUNCTION__ ));
        }

        if( bProcessFurther )
        {
                PCOSA_DATAMODEL_SELFHEAL          pMyObject  = (PCOSA_DATAMODEL_SELFHEAL)hThisObject;

                /* Modify the DNS ping test flag */
                /*if ( 0 == syscfg_set( NULL,
                                                          "selfheal_dns_pingtest_url",
                                                          pString ) )
                {
                        if ( 0 == syscfg_commit( ) )
                        {
                                memset(pMyObject->DNSPingTest_URL, 0, sizeof( pMyObject->DNSPingTest_URL ));
                                AnscCopyString(pMyObject->DNSPingTest_URL, pString);
                        }
                }*/
		
	        _set_db_value(SYSCFG_FILE,"selfheal_dns_pingtest_url",pString);
                CcspTraceInfo(("[%s] DNSPingTest_URL:[ %s ]\n",
                                                                        __FUNCTION__,
                                                                        pMyObject->DNSPingTest_URL ));
        }

        return ReturnStatus;
}

