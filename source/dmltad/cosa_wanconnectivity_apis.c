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

#include "plugin_main_apis.h"
#include "safec_lib_common.h"
#include "cosa_wanconnectivity_apis.h"
#include <rbus/rbus.h>
#include <syscfg/syscfg.h>
#include "secure_wrapper.h"


static void eventReceiveHandler(
    rbusHandle_t handle,
    rbusEvent_t const* event,
    rbusEventSubscription_t* subscription);

rbusHandle_t rbus_handle  = NULL;
/* For registering and unregistering at run time, based on feature status, couldn't
able to do properly with single handle, introducing other handles, with single
handle, without rbus_close could see parameters still exists with rbus_unRegDataElements*/
rbusHandle_t rbus_table_handle  = NULL;
BOOL g_wanconnectivity_check_active = FALSE;
BOOL g_wanconnectivity_check_enable = FALSE;
SLIST_HEADER                  gpUrlList;
ULONG                         gulUrlNextInsNum = 1;
ULONG                         gIntfCount = 0;
USHORT                        g_last_sent_actv_txn_id_A;
USHORT                        g_last_sent_actv_txn_id_AAAA;
USHORT                        g_last_sent_actv_txn_id_A_bkp;
USHORT                        g_last_sent_actv_txn_id_AAAA_bkp;
int sysevent_fd_wanchk;
token_t sysevent_token_wanchk;
int sysevent_fd_wanchk_monitor;
token_t sysevent_token_wanchk_monitor;
/* In current scenario don't have scenario more than two, supporting
global DB upto 2. If needed it can be enhanced later*/
WANCNCTVTY_CHK_GLOBAL_INTF_INFO gInterface_List[MAX_NO_OF_INTERFACES] = {0};
pthread_mutex_t gIntfAccessMutex;
pthread_mutex_t gUrlAccessMutex;
pthread_mutex_t gDnsTxnIdAccessMutex;
pthread_mutex_t gDnsTxnIdBkpAccessMutex;

static pthread_t sysevent_monitor_tid;
#if !defined(WAN_FAILOVER_SUPPORTED) && !defined(GATEWAY_FAILOVER_SUPPORTED)
static async_id_t current_wan_asyncid;
#endif
static async_id_t backupwan_router_addr_asyncid;
static async_id_t MeshWANInterface_UlaAddr_asyncid;
static char subscribe_userData[MAX_INTF_NAME_SIZE] ={0};
#if defined(WAN_FAILOVER_SUPPORTED) || defined(GATEWAY_FAILOVER_SUPPORTED)
const char* sub_event_param[] = {CURRENT_PRIMARY_INTF_DML,
                                CURRENT_STANDBY_INTF_DML};
#endif

#ifdef GATEWAY_FAILOVER_SUPPORTED
const char* sub_activegwevent_param[] = {ACTIVE_GATEWAY_DML};
#endif

extern rbusError_t CosaWanCnctvtyChk_RbusInit(VOID);
extern rbusError_t CosaWanCnctvtyChk_Reg_elements(dml_type_t type);
extern rbusError_t CosaWanCnctvtyChk_UnReg_elements(dml_type_t type);
extern ANSC_STATUS wancnctvty_chk_start_threads(ULONG InstanceNumber,service_type_t type);
extern ANSC_STATUS wancnctvty_chk_stop_threads(ULONG InstanceNumber,service_type_t type);
BOOL CosaWanCnctvtyChk_IsPrimary_Configured();

/*********************************************************************************
 * Function to process the feature enable flag change
   1. disabled -> enabled
      * set active status based on current HOST status
      * if host is active and primary, populate interface table
      *passive monitor config is enabled, start passive dns sniffer
      *continous query is enabled, start continous query thread
      *Note continous query result will supersede the passive
       monitor result
    * To Do check do we need passive monitor when continous query is
      enabled
      * if host is not active, set active status as false and exit
   2. Enabled -> disabled
      * stop passive monitor thread and free resources
      * stop continous query thread if running
      * update result object accordingly

*NOTE we are in the context of dmcli so make sure to run the actions in background
as thread and detachable
***********************************************************************************/ 

/**********************************************************************

    caller:     self

    prototype:

        ANSC_STATUS
        CosaWanCnctvtyChkInitialize
            (
                ANSC_HANDLE                 hThisObject
            );

    description:

        This function initiate wan connectivity check object and return handle.

    argument:   ANSC_HANDLE                 hThisObject
                This handle is actually the pointer of this object
                itself.

    return:     operation status.

**********************************************************************/

ANSC_STATUS CosaWanCnctvtyChk_Init (VOID)
{
    char buf[BUFLEN_8] = {0};
    int rc = RBUS_ERROR_SUCCESS;

    sysevent_fd_wanchk = sysevent_open("127.0.0.1", SE_SERVER_WELL_KNOWN_PORT,
                      SE_VERSION, "wan_connectivity_check", &sysevent_token_wanchk);
    if (sysevent_fd_wanchk < 0)
    {
        WANCHK_LOG_ERROR("%s: sysevent int failed \n", __FUNCTION__);
        return ANSC_STATUS_FAILURE;
    }

    sysevent_fd_wanchk_monitor = sysevent_open("127.0.0.1", SE_SERVER_WELL_KNOWN_PORT,
                SE_VERSION, "wan_connectivity_check_sysevent_mntr", &sysevent_token_wanchk_monitor);
    if (sysevent_fd_wanchk_monitor < 0)
    {
        WANCHK_LOG_ERROR("%s: sysevent monitor int failed \n", __FUNCTION__);
        return ANSC_STATUS_FAILURE;
    }
    /* Rbus init*/
    rc = CosaWanCnctvtyChk_RbusInit();
    if(rc)
    {
        WANCHK_LOG_ERROR("RbusInit failure, reason = %d", rc);
        return ANSC_STATUS_FAILURE;
    }

    rc = CosaWanCnctvtyChk_Reg_elements(FEATURE_DML);
    if(rc)
    {
        WANCHK_LOG_ERROR("RbusDML feature Reg failure, reason = %d", rc);
        return ANSC_STATUS_FAILURE;
    }

    /* Initialize url list access mutex */
    pthread_mutexattr_t     mutex_attr_url;
    pthread_mutexattr_init(&mutex_attr_url);
    pthread_mutexattr_settype(&mutex_attr_url, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&(gUrlAccessMutex), &(mutex_attr_url));
    pthread_mutexattr_destroy(&mutex_attr_url);

    /* Initialize interface list access mutex */
    pthread_mutexattr_t     mutex_attr;
    pthread_mutexattr_init(&mutex_attr);
    pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&(gIntfAccessMutex), &(mutex_attr));
    pthread_mutexattr_destroy(&mutex_attr);

    /* Initialize DNS txn id primary interface access mutex */
    pthread_mutexattr_t     mutex_attr_txn;
    pthread_mutexattr_init(&mutex_attr_txn);
    pthread_mutexattr_settype(&mutex_attr_txn, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&(gDnsTxnIdAccessMutex), &(mutex_attr_txn));
    pthread_mutexattr_destroy(&mutex_attr_txn);

    /* Initialize DNS txn id backup interface access mutex */
    pthread_mutexattr_t     mutex_attr_txn_bkp;
    pthread_mutexattr_init(&mutex_attr_txn_bkp);
    pthread_mutexattr_settype(&mutex_attr_txn_bkp, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&(gDnsTxnIdBkpAccessMutex), &(mutex_attr_txn_bkp));
    pthread_mutexattr_destroy(&mutex_attr_txn_bkp);

    pthread_mutex_lock(&gUrlAccessMutex);
    g_wanconnectivity_check_enable = FALSE;
    g_wanconnectivity_check_active = FALSE;
    AnscSListInitializeHeader(&gpUrlList);
    gulUrlNextInsNum          = 1;
    pthread_mutex_unlock(&gUrlAccessMutex);

    for (int i=0;i < MAX_NO_OF_INTERFACES;i++)
    {
        memset(&gInterface_List[i],0,sizeof(WANCNCTVTY_CHK_GLOBAL_INTF_INFO));
    }

    if (syscfg_get(NULL, "wanconnectivity_chk_enable", buf, sizeof(buf)) == 0)
    {
        if (buf[0] != '\0')
        {
            g_wanconnectivity_check_enable = (strcmp(buf,"true") ? FALSE : TRUE);
        }
    }
    else
    {
        WANCHK_LOG_INFO("syscfg wanconnectivity_chk_enable not available,Taking Factory Default as FALSE\n");
        g_wanconnectivity_check_enable = FALSE;
    }

    WANCHK_LOG_INFO("%s: Wan Connectivty Check Enable : %s\n", __FUNCTION__,
                            (g_wanconnectivity_check_enable == TRUE) ? "true" : "false");

    if (g_wanconnectivity_check_enable == TRUE)
    {
        rc = CosaWanCnctvtyChk_Reg_elements(FEATURE_ENABLED_DML);
        if(rc)
        {
            WANCHK_LOG_ERROR("RbusDML Enabled Reg failure, reason = %d", rc);
            return ANSC_STATUS_FAILURE;
        }
    /* start wan connectivty check process*/
       g_wanconnectivity_check_active    = CosaWanCnctvtyChk_GetActive_Status();
       if (g_wanconnectivity_check_active == TRUE)
       {
           if (CosaWanCnctvtyChk_Init_URLTable () != ANSC_STATUS_SUCCESS)
           {
               WANCHK_LOG_ERROR("%s: URL Table Init failed\n",__FUNCTION__);  
           }
           if (CosaWanCnctvtyChk_Init_IntfTable () != ANSC_STATUS_SUCCESS)
           {
               WANCHK_LOG_ERROR("%s: Interface Table Init failed\n",__FUNCTION__);
           }
           /* Lets start the subscription and listen for events*/
           CosaWanCnctvtyChk_SubscribeRbus();
           CosaWanCnctvtyChk_StartSysevent_listener();
        }
        CosaWanCnctvtyChk_SubscribeActiveGW();
    }

    return ANSC_STATUS_SUCCESS;
}

void CosaWanCnctvtyChk_StartSysevent_listener()
{
    int Error = 0;
    Error=pthread_create(&sysevent_monitor_tid,NULL,WanCnctvtyChk_SysEventHandlerThrd,NULL);
    if (Error)
    {
        WANCHK_LOG_ERROR("%s: Unable to create WanCnctvtyChk_SysEventHandlerThrd\n",__FUNCTION__);
        return;
    }
/* Only in case of WAN_FAILOVER not supported, we take wan if from sysevent*/
#if !defined(WAN_FAILOVER_SUPPORTED) && !defined(GATEWAY_FAILOVER_SUPPORTED)
    sysevent_setnotification(sysevent_fd_wanchk_monitor, sysevent_token_wanchk_monitor, "current_wan_ifname", 
                                                                    &current_wan_asyncid);
#endif
    sysevent_setnotification(sysevent_fd_wanchk_monitor, sysevent_token_wanchk_monitor, "backupwan_router_addr",
                                                                              &backupwan_router_addr_asyncid);
    sysevent_setnotification(sysevent_fd_wanchk_monitor, sysevent_token_wanchk_monitor, "MeshWANInterface_UlaAddr",
		    					 		      &MeshWANInterface_UlaAddr_asyncid);
}

void CosaWanCnctvtyChk_StopSysevent_listener()
{
    if (sysevent_monitor_tid)
    {
#if !defined(WAN_FAILOVER_SUPPORTED) && !defined(GATEWAY_FAILOVER_SUPPORTED)
        sysevent_rmnotification(sysevent_fd_wanchk_monitor, sysevent_token_wanchk_monitor, 
                                                                            current_wan_asyncid);
#endif
        pthread_cancel(sysevent_monitor_tid);
        sysevent_monitor_tid = 0;
    }
}

/* Remove wan connectivity feature*/

ANSC_STATUS CosaWanCnctvtyChkRemove ()
{
    ANSC_STATUS                      returnStatus = ANSC_STATUS_SUCCESS;
    CosaWanCnctvtyChk_UnSubscribeRbus();
    CosaWanCnctvtyChk_StopSysevent_listener();
    /*Remove Interface data*/
    CosaWanCnctvtyChk_DeInit_IntfTable();
    /*Remove URL list*/
    CosaWanCnctvtyChk_Remove_Urllist();
    CosaWanCnctvtyChk_UnReg_elements(FEATURE_DML);
    return returnStatus;
}

/* CosaWanCnctvtyChk_Init_URLTable

fetch the URL entries from the db, if we don't have any, make the default URL as
"www.google.com" proposed by architecture*/

ANSC_STATUS CosaWanCnctvtyChk_Init_URLTable (VOID)
{
    PCOSA_CONTEXT_LINK_OBJECT       pCosaContext    = (PCOSA_CONTEXT_LINK_OBJECT)NULL;
    ANSC_STATUS returnStatus = ANSC_STATUS_SUCCESS;
    PCOSA_DML_WANCNCTVTY_CHK_URL_INFO pUrlInfo      = (PCOSA_DML_WANCNCTVTY_CHK_URL_INFO)NULL;
    errno_t rc = -1;
    int index = -1;
    int total_url_entries = 0;
    char buf[BUFLEN_8] = {0};
    BOOL use_default_url = FALSE;
    char *default_url = DEFAULT_URL;
    char paramName[BUFLEN_128] = {0};
    char URL_buf[MAX_URL_SIZE] = {0};
    int rbus_ret = RBUS_ERROR_SUCCESS;

    if (syscfg_get(NULL, "wanconnectivity_chk_maxurl_inst", buf, sizeof(buf)) == 0 && buf[0] != '\0')
    {
        total_url_entries = atoi(buf);
        WANCHK_LOG_INFO("%s: Wan Connectivty Check max url instance : %d\n", __FUNCTION__,
                                                                    total_url_entries);
    }
    else
    {
        total_url_entries = DEFAULT_URL_COUNT;
        use_default_url  = TRUE;
    }
    for (index=0; index < total_url_entries;index++)
    {
        if ( !pUrlInfo)
        {
            pUrlInfo = (PCOSA_DML_WANCNCTVTY_CHK_URL_INFO)
                                        AnscAllocateMemory(sizeof(COSA_DML_WANCNCTVTY_CHK_URL_INFO));
        }
        if ( !pUrlInfo)
        {
           return ANSC_STATUS_RESOURCES;
        }

        rc = sprintf_s(paramName,sizeof(paramName),"wanconnectivity_chk_url_%d",(index+1));
        if (rc < EOK)
        {
            ERR_CHK(rc);
        }

        if (syscfg_get(NULL, paramName, URL_buf, sizeof(URL_buf)) == 0 && URL_buf[0] != '\0')
        {
            rc = strcpy_s(pUrlInfo->URL,MAX_URL_SIZE , URL_buf);
            ERR_CHK(rc);
        }
        else if (use_default_url)
        {
            rc = strcpy_s(pUrlInfo->URL,MAX_URL_SIZE , default_url);
            ERR_CHK(rc);
        }
        else
        {
            /* A corner case,if we delete a row in this order, having rows 1,2,3 delete row 2.
            skip if we have empty entry, skip for now*/
            WANCHK_LOG_ERROR("URL entry is empty in DB for instance:%d\n",(index+1));
            pthread_mutex_lock(&gUrlAccessMutex);
            gulUrlNextInsNum++;
            pthread_mutex_unlock(&gUrlAccessMutex);
            continue;
        }

        pCosaContext = (PCOSA_CONTEXT_LINK_OBJECT)AnscAllocateMemory(sizeof(COSA_CONTEXT_LINK_OBJECT));

       if ( !pCosaContext )
       {
          returnStatus = ANSC_STATUS_RESOURCES;
          goto  EXIT;
       }

       pthread_mutex_lock(&gUrlAccessMutex);
       pCosaContext->InstanceNumber =  gulUrlNextInsNum;
       pUrlInfo->InstanceNumber = gulUrlNextInsNum; 
       gulUrlNextInsNum++;
       if (gulUrlNextInsNum == 0)
            gulUrlNextInsNum = 1;

       pCosaContext->hContext      = (ANSC_HANDLE)AnscAllocateMemory(sizeof(COSA_DML_WANCNCTVTY_CHK_URL_INFO));
       memcpy(pCosaContext->hContext, (ANSC_HANDLE)pUrlInfo, sizeof(COSA_DML_WANCNCTVTY_CHK_URL_INFO));
       pCosaContext->hParentTable  = NULL;
       pCosaContext->bNew          = TRUE;

       CosaSListPushEntryByInsNum(&gpUrlList, pCosaContext);
       rc = rbusTable_registerRow(rbus_table_handle,WANCHK_TEST_URL_TABLE ,pUrlInfo->InstanceNumber,
                                                                                            NULL);
       if(rbus_ret != RBUS_ERROR_SUCCESS)
       {
            WANCHK_LOG_ERROR("\n%s %d - URL Table (%s) Add failed, Error=%d \n", 
                                            __FUNCTION__, __LINE__,WANCHK_TEST_URL_TABLE,rbus_ret);
            pthread_mutex_unlock(&gUrlAccessMutex);
            returnStatus = ANSC_STATUS_FAILURE;
            goto EXIT;
       }
       else
       {
            WANCHK_LOG_INFO("\n%s %d - URL Table (%s) Added Successfully\n",
                                                __FUNCTION__, __LINE__, WANCHK_TEST_URL_TABLE );
       }
       pthread_mutex_unlock(&gUrlAccessMutex);
       AnscFreeMemory(pUrlInfo);
       pUrlInfo = (PCOSA_DML_WANCNCTVTY_CHK_URL_INFO)NULL;
    }
EXIT:
    if (pUrlInfo)
       AnscFreeMemory(pUrlInfo);
    return returnStatus;
}

/* CosaWanCnctvtyChk_Init_IntfTable 

Initialize interface table and fetch the corresponding value from DB or assign to defaults*/

ANSC_STATUS CosaWanCnctvtyChk_Init_IntfTable (VOID)
{
    /* We won't really bother about no of wan interfaces exists,our main
    objective is finding connectivity over primary and backup.
    No of interfaces may depend on the availability of these interfaces in wan
    mgr.

    currently I could see CurrentActiveInterface and CurrentStandbyInterface is asingle
    interface, Not sure on the scope of this having multiple interface
    and also no delimeters are specified for this data model.

    Implementation here aligning with current code, if in future we have multiple interfaces
    then below part of code have to enhanced to support*/

    ANSC_STATUS returnStatus = ANSC_STATUS_SUCCESS;
    returnStatus = CosaWanCnctvtyChk_Init_Intf(PRIMARY_INTF_INDEX);
    if (returnStatus != ANSC_STATUS_SUCCESS)
    {
          /* We can't do anything without primary interface, Not seeing any purpose
          of having secondary alone, returning here*/ 
          WANCHK_LOG_WARN("%s:Primary WAN interface table Initialization failed\n",__FUNCTION__);
          return ANSC_STATUS_FAILURE;
    }

    returnStatus = CosaWanCnctvtyChk_Init_Intf(SECONDARY_INTF_INDEX);
    if (returnStatus != ANSC_STATUS_SUCCESS)
    { 
        WANCHK_LOG_WARN("%s:Backup WAN interface table Initialization failed\n",__FUNCTION__);
        return ANSC_STATUS_FAILURE;
    }
    return returnStatus;
}

/* CosaWanCnctvtyChk_Init_Intf

Initialize interface table and fetch the corresponding value from DB or assign to defaults*/

ANSC_STATUS CosaWanCnctvtyChk_Init_Intf (wan_intf_index_t index)
{
    ANSC_STATUS returnStatus = ANSC_STATUS_SUCCESS;
    COSA_DML_WANCNCTVTY_CHK_INTF_INFO IPInterface;
    int rc = RBUS_ERROR_SUCCESS;

    memset(&IPInterface,0,sizeof(COSA_DML_WANCNCTVTY_CHK_INTF_INFO));

    IPInterface.InstanceNumber = index;

    if (CosaWanCnctvtyChk_IfGetEntry(&IPInterface) != ANSC_STATUS_SUCCESS)
    {
        WANCHK_LOG_ERROR("Unable to get interface config for instance %ld\n",
                                                                        IPInterface.InstanceNumber);
        return ANSC_STATUS_FAILURE;
    }

    rc = rbusTable_registerRow(rbus_table_handle,WANCHK_INTF_TABLE,IPInterface.InstanceNumber,
                                                                                IPInterface.Alias);
    if(rc != RBUS_ERROR_SUCCESS)
    {
        WANCHK_LOG_ERROR("%s %d - Interface(%ld) Table (%s) Registartion failed, Error=%d \n", 
                                                __FUNCTION__, __LINE__,IPInterface.InstanceNumber,
                                                WANCHK_INTF_TABLE,rc);
        return ANSC_STATUS_FAILURE;
    }
    else
    {
         WANCHK_LOG_INFO("%s %d - Iterface(%ld) Table (%s) Registartion Successfully, AliasName(%s)\n",
                                                __FUNCTION__, __LINE__,IPInterface.InstanceNumber, 
                                                WANCHK_INTF_TABLE, IPInterface.Alias);
    }

    pthread_mutex_lock(&gIntfAccessMutex);
    PWANCNCTVTY_CHK_GLOBAL_INTF_INFO gIntfInfo = &gInterface_List[IPInterface.InstanceNumber-1];
    gIntfCount++;
    gIntfInfo->IPInterface.Configured = TRUE;
    WANCHK_LOG_DBG("Updated Interface count:%ld\n",gIntfCount);
    pthread_mutex_unlock(&gIntfAccessMutex);


    if (returnStatus == ANSC_STATUS_SUCCESS)
    {
        returnStatus = wancnctvty_chk_start_threads(IPInterface.InstanceNumber,ALL_THREADS);
        if (returnStatus != ANSC_STATUS_SUCCESS)
        {
            WANCHK_LOG_ERROR("%s:%d Unable to start threads\n",__FUNCTION__,__LINE__);
            return ANSC_STATUS_FAILURE;
        }
    }
    CosaWanCnctvtyChk_Interface_dump(IPInterface.InstanceNumber);
    return returnStatus;
}

ANSC_STATUS CosaWanCnctvtyChk_IfGetEntry(PCOSA_DML_WANCNCTVTY_CHK_INTF_INFO pIPInterface)
{
    char Value[MAX_INTF_NAME_SIZE] = {0};
    ANSC_STATUS returnStatus = ANSC_STATUS_SUCCESS;
    errno_t rc = -1;
    /* Get stored config Values or use default values since we are in initialization*/
    CosaDmlGetIntfCfg(pIPInterface,TRUE);
    if (PRIMARY_INTF_INDEX == pIPInterface->InstanceNumber)
    {
        if(CosaWanCnctvtyChk_GetPrimary_IntfName(Value) == TRUE)
        {
            rc = strcpy_s(pIPInterface->InterfaceName,MAX_INTF_NAME_SIZE , Value);
            ERR_CHK(rc);
        }
        else
        {
            WANCHK_LOG_ERROR("%s:%d Unable to get Primary interface\n",__FUNCTION__,__LINE__);
            return ANSC_STATUS_FAILURE;
        }

        rc = strcpy_s(pIPInterface->Alias,MAX_INTF_NAME_SIZE , "Primary");
        ERR_CHK(rc);

    }
    else
    {
        if(CosaWanCnctvtyChk_GetSecondary_IntfName(Value) == TRUE)
        {
            // Assuming there will be only one standby node
            rc = strcpy_s(pIPInterface->InterfaceName,MAX_INTF_NAME_SIZE , Value);
            ERR_CHK(rc);
        }
        else
        {
            WANCHK_LOG_ERROR("%s:%d Unable to get Backup interface\n",__FUNCTION__,__LINE__);
            return ANSC_STATUS_FAILURE;
        }
        rc = strcpy_s(pIPInterface->Alias,MAX_INTF_NAME_SIZE , "Backup");
        ERR_CHK(rc);
        memset(Value,0,MAX_INTF_NAME_SIZE);
    }

    pIPInterface->MonitorResult         = MONITOR_RESULT_UNKNOWN;
    pIPInterface->QueryNow              = FALSE;
    pIPInterface->QueryNowResult        = QUERYNOW_RESULT_UNKNOWN;
    pIPInterface->Cfg_bitmask           = INTF_CFG_ALL;
    returnStatus = CosaDml_glblintfdb_updateentry(pIPInterface);
    if (returnStatus != ANSC_STATUS_SUCCESS)
    {
        WANCHK_LOG_ERROR("%s:Unable to update global db entry\n",__FUNCTION__);
        return ANSC_STATUS_FAILURE;
    }

    /* ??? failures in dns config may not be useful, still we can show only interface config
    And also based on interface name, we have to fetch dns information, since brRWAN can be
    primary interface in failover scenario.*/

    if (CosaWanCnctvtyChk_DNS_GetEntry(pIPInterface->InterfaceName) != ANSC_STATUS_SUCCESS)
    {
        WANCHK_LOG_WARN("Unable to get dns config for instance %ld\n",
                                                                pIPInterface->InstanceNumber);
    }
    return ANSC_STATUS_SUCCESS;
}

ANSC_STATUS CosaWanCnctvtyChk_DNS_GetEntry(char *InterfaceName)
{   
    ANSC_STATUS returnStatus = ANSC_STATUS_SUCCESS;
    errno_t rc = -1;
    int ind = -1;
    /* We have to Fetch the DNS server information from Device.DNS.Client, for primary 
    for Remote we have to use remote WAN interface address as DNS,since as per Architecture
    all dns packets will be re-directed in failover scenario,irrespective of dns config in primary*/

    WANCHK_LOG_INFO("Getting DNS fetch Request for Interface %s\n",InterfaceName);

    rc = strcmp_s( "brRWAN",strlen("brRWAN"),InterfaceName, &ind);
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        // have to fetch peer interface ip address
        returnStatus = CosaWanCnctvtyChk_GetDNS_PeerInfo(InterfaceName);
    }
    else
    {
        returnStatus = CosaWanCnctvtyChk_GetDNS_HostInfo(InterfaceName);
    }
    return returnStatus;
}

/**********************************************************************

    caller:     self

    prototype: CosaWanCnctvtyChk_GetDNS_HostInfo(void)

        BOOL
        
        (
         );

    description:

        This function will check whether host is active or not.

    argument:   ANSC_HANDLE                 hThisObject
                This handle is actually the pointer of this object
                itself.

    return:     operation status.

**********************************************************************/
ANSC_STATUS CosaWanCnctvtyChk_GetDNS_HostInfo(char *InterfaceName)
{
    ANSC_STATUS returnStatus = ANSC_STATUS_SUCCESS;
    int rbus_ret = RBUS_ERROR_SUCCESS;
    char Value[BUFLEN_128] = {0};
    unsigned int   DnsServerCount = 0;
    char rbus_eventname[BUFLEN_256] = {0};
    errno_t rc = -1;
    PCOSA_DML_WANCNCTVTY_CHK_DNSSRV_INFO pDnsSrvInfo = (PCOSA_DML_WANCNCTVTY_CHK_DNSSRV_INFO) NULL;

    returnStatus = WanCnctvtyChk_GetParameterValue(DNS_SRV_COUNT_DML,Value);
    if (returnStatus == ANSC_STATUS_SUCCESS)
    {
       if (Value[0] != '\0')
       {
           WANCHK_LOG_DBG("%s: Number of server_entries : %d\n", __FUNCTION__,DnsServerCount);
           DnsServerCount=atoi(Value);
           if (DnsServerCount)
           {
                pDnsSrvInfo = (PCOSA_DML_WANCNCTVTY_CHK_DNSSRV_INFO) malloc (
                                DnsServerCount * sizeof(COSA_DML_WANCNCTVTY_CHK_DNSSRV_INFO));
                if (!pDnsSrvInfo)
                {
                    WANCHK_LOG_ERROR("%s:%d Unable to allocate memory\n",__FUNCTION__,__LINE__);
                    return ANSC_STATUS_FAILURE;
                }
                memset(pDnsSrvInfo,0,DnsServerCount * sizeof(COSA_DML_WANCNCTVTY_CHK_DNSSRV_INFO));
           }
           else
           {
                /* current entries is zero, lets subscribe and wait for change to happen*/
                WANCHK_LOG_WARN("%s: Number of server_entries is zero\n", __FUNCTION__);
           }
       }
    }
    else
    {
        WANCHK_LOG_ERROR("%s: Unable to fetch dns server count entry for instance %s\n",
                                                            __FUNCTION__,InterfaceName);
        return ANSC_STATUS_FAILURE;
    }

    /* Subscribe to server count dml, so that we will be notified when there is a change
    Note in rbus_subscription we are sending interface name so that we will know for which interface
    this actually happens*/
    memset(subscribe_userData,0,MAX_INTF_NAME_SIZE);
    rc = strcpy_s(subscribe_userData,MAX_INTF_NAME_SIZE,InterfaceName);
    ERR_CHK(rc);
    rbus_ret = rbusEvent_Subscribe(rbus_table_handle, DNS_SRV_COUNT_DML, eventReceiveHandler,
                                                                            subscribe_userData, 0);
    if (rbus_ret != RBUS_ERROR_SUCCESS)
    {
        WANCHK_LOG_WARN("rbusEvent_Subscribe failed for %s ret: %d\n", 
                                                         DNS_SRV_COUNT_DML,rbus_ret); 
    }

    if (DnsServerCount)
    {
        returnStatus = WanCnctvtyChk_GetPrimaryDNS_Entry(pDnsSrvInfo);
        if (returnStatus == ANSC_STATUS_SUCCESS)
        {
            returnStatus = CosaDml_glblintfdb_update_dnsentry(InterfaceName,DnsServerCount,
                                                              pDnsSrvInfo);
            if (returnStatus != ANSC_STATUS_SUCCESS)
            {
                WANCHK_LOG_WARN("%s:%d Unable to update global db dns entry\n",__FUNCTION__,__LINE__);
            }
            /* This is a one time initialization, but there are potential for "Device.DNS.Client.Server"
            will be updated at run time, Not a huge probability but a valid corner case, lets
            subscribe for changes,if any update our DB again*/
            int iter;
            for(iter=1;iter<=DnsServerCount;iter++)
            {
                memset(rbus_eventname,0,sizeof(rbus_eventname));
                rc = sprintf_s(rbus_eventname, sizeof(rbus_eventname),
                                        DNS_SRV_ENTRY_DML,iter);
                if (rc < EOK)
                {
                    ERR_CHK(rc);
                }
                rbus_ret = rbusEvent_Subscribe(rbus_table_handle,rbus_eventname,eventReceiveHandler,
                                                                            subscribe_userData,0);
                if (rbus_ret != RBUS_ERROR_SUCCESS)
                {
                    WANCHK_LOG_ERROR("rbusEvent_Subscribe failed for %s ret: %d\n", 
                                                             rbus_eventname,rbus_ret);
                }
            }
        }
        else
        {
           WANCHK_LOG_ERROR("Unable to populate Dns server list for intf instance %s\n",
                                                                            InterfaceName);        
        }
    }

    if (returnStatus!= ANSC_STATUS_SUCCESS && pDnsSrvInfo)
    {
        AnscFreeMemory(pDnsSrvInfo);
        pDnsSrvInfo = NULL;
    }
    return returnStatus;

}

ANSC_STATUS CosaWanCnctvtyChk_GetDNS_PeerInfo(char *InterfaceName)
{
    int ret = -1;
    char IPv4_DNS[IPv4_STR_LEN] = {0};
    char *IPv6_DNS;
    char buf[BUFLEN_64] = {0};
    unsigned int   DnsServerCount = 0;
    BOOL IPv4_DNS_EXISTS = FALSE;
    BOOL IPv6_DNS_EXISTS = FALSE;
    errno_t rc = -1;
    ANSC_STATUS returnStatus = ANSC_STATUS_SUCCESS;
    char tmpStr[BUFLEN_256] = {0};
    char *saveptr   = NULL;
    PCOSA_DML_WANCNCTVTY_CHK_DNSSRV_INFO pDnsSrvInfo = (PCOSA_DML_WANCNCTVTY_CHK_DNSSRV_INFO) NULL;

    ret = sysevent_get(sysevent_fd_wanchk, sysevent_token_wanchk, "backupwan_router_addr", 
                                                                IPv4_DNS, sizeof(IPv4_DNS));
    if ((ret == 0) && strlen(IPv4_DNS))
    {
        IPv4_DNS_EXISTS = TRUE;
        DnsServerCount++;
    }
    ret = sysevent_get(sysevent_fd_wanchk, sysevent_token_wanchk, "MeshWANInterface_UlaAddr", 
                                                                    buf, sizeof(buf));
    if ((ret == 0) && strlen(buf))
    {
        rc = strcpy_s(tmpStr, strlen(buf)+1 , buf);
        ERR_CHK(rc);
        IPv6_DNS = strtok_r(tmpStr, "/",&saveptr);
        if(IPv6_DNS)
        {
            IPv6_DNS_EXISTS = TRUE;
            DnsServerCount++;
        }
    }

    if (DnsServerCount)
    {
        WANCHK_LOG_DBG("%s: Number of server_entries : %d\n", __FUNCTION__,DnsServerCount);
        unsigned int dns_idx =0;
        pDnsSrvInfo = (PCOSA_DML_WANCNCTVTY_CHK_DNSSRV_INFO) malloc (
                        DnsServerCount * sizeof(COSA_DML_WANCNCTVTY_CHK_DNSSRV_INFO));
        if (!pDnsSrvInfo)
        {
            WANCHK_LOG_ERROR("%s:%d Unable to allocate memory\n",__FUNCTION__,__LINE__);
            return ANSC_STATUS_FAILURE;
        }
        memset(pDnsSrvInfo,0,DnsServerCount * sizeof(COSA_DML_WANCNCTVTY_CHK_DNSSRV_INFO));
        if (IPv4_DNS_EXISTS)
        {
            rc = strcpy_s(pDnsSrvInfo[dns_idx].IPAddress.IPv4Address,IPv4_STR_LEN,IPv4_DNS);
            ERR_CHK(rc);
            pDnsSrvInfo[dns_idx].dns_type = DNS_SRV_IPV4;
            dns_idx++;
        }

        if (IPv6_DNS_EXISTS)
        {
            rc = strcpy_s(pDnsSrvInfo[dns_idx].IPAddress.IPv6Address,IPv6_STR_LEN,IPv6_DNS);
            ERR_CHK(rc);
            pDnsSrvInfo[dns_idx].dns_type = DNS_SRV_IPV6;
        }
        returnStatus = CosaDml_glblintfdb_update_dnsentry(InterfaceName,DnsServerCount,
                                                          pDnsSrvInfo);
        if (returnStatus != ANSC_STATUS_SUCCESS)
        {
            WANCHK_LOG_WARN("%s:%d Unable to update global db dns entry\n",__FUNCTION__,__LINE__);
        }
    }
    else
    {
        /* current entries is zero, lets subscribe and wait for change to happen*/
        WANCHK_LOG_WARN("%s: Number of server_entries is zero\n", __FUNCTION__);
    }

    if (returnStatus!= ANSC_STATUS_SUCCESS && pDnsSrvInfo)
    {
        AnscFreeMemory(pDnsSrvInfo);
        pDnsSrvInfo = NULL;
    }
    return returnStatus;

}
/**********************************************************************

    caller:     self

    prototype:

        BOOL
        CosaWanCnctvtyChk_GetActive_Status
        (
         );

    description:

        This function will check whether host is active or not.

    argument:   ANSC_HANDLE                 hThisObject
                This handle is actually the pointer of this object
                itself.

    return:     operation status.

**********************************************************************/
BOOL CosaWanCnctvtyChk_GetActive_Status(void)
{

#ifdef GATEWAY_FAILOVER_SUPPORTED
   ANSC_STATUS ret = ANSC_STATUS_SUCCESS;
   char Value[128] = {0};
   errno_t      rc = -1;
   int ind         = -1;

   ret = WanCnctvtyChk_GetParameterValue(ACTIVE_GATEWAY_DML,Value);

   if (ret == ANSC_STATUS_SUCCESS)
   {
       if (Value[0] != '\0')
       {
           WANCHK_LOG_INFO("%s: Active Gateway Status : %s\n", __FUNCTION__,Value);
           rc = strcmp_s( "true",strlen("true"),Value, &ind);
           ERR_CHK(rc);
           if((!ind) && (rc == EOK))
           {
             return TRUE;
           }
       }
   }
   return FALSE;
#else
   return TRUE;
#endif

}

/**********************************************************************

    caller:     self

    prototype:

        BOOL
        CosaWanCnctvtyChk_GetActive_Status
        (
         );

    description:

        This function will check whether host is active or not.

    argument:   ANSC_HANDLE                 hThisObject
                This handle is actually the pointer of this object
                itself.

    return:     operation status.

**********************************************************************/
BOOL CosaWanCnctvtyChk_GetPrimary_IntfName(char *Output)
{

/* For standalone setups without wan failover support fetch
from sysevent*/
ANSC_STATUS ret = ANSC_STATUS_SUCCESS;
char Value[MAX_INTF_NAME_SIZE] = {0};
errno_t      rc = -1;
#if defined(WAN_FAILOVER_SUPPORTED) || defined(GATEWAY_FAILOVER_SUPPORTED)
   ret = WanCnctvtyChk_GetParameterValue(CURRENT_PRIMARY_INTF_DML,Value);
   if (ret != ANSC_STATUS_SUCCESS)
   {
     ret = sysevent_get(sysevent_fd_wanchk, sysevent_token_wanchk, "current_wan_ifname", Value, sizeof(Value));
   if (!strlen(Value))
   {
      /*if empty take default wan interface,if default also empty we can't do anything*/
      ret = sysevent_get(sysevent_fd_wanchk, sysevent_token_wanchk, "wan_ifname", Value, sizeof(Value));
   }
   }
#else
   ret = sysevent_get(sysevent_fd_wanchk, sysevent_token_wanchk, "current_wan_ifname", Value, sizeof(Value));
   if (!strlen(Value))
   {
      /*if empty take default wan interface,if default also empty we can't do anything*/
      ret = sysevent_get(sysevent_fd_wanchk, sysevent_token_wanchk, "wan_ifname", Value, sizeof(Value));  
   }
#endif
   if (ret == ANSC_STATUS_SUCCESS)
   {
       if (strlen(Value))
       {
          WANCHK_LOG_INFO("%s: Current Active Interface : %s\n", __FUNCTION__,Value);
          rc = strcpy_s( Output,MAX_INTF_NAME_SIZE,Value);
          ERR_CHK(rc);
          if (rc == EOK)
          {
             return TRUE;
          }
       }
       else
       {
           WANCHK_LOG_INFO("%s: Current Interface Name is Empty\n", __FUNCTION__);
           return FALSE;
       }
   }
   return FALSE;
}

/**********************************************************************

    caller:     self

    prototype:

        BOOL
        CosaWanCnctvtyChk_GetActive_Status
        (
         );

    description:

        This function will check whether host is active or not.

    argument:   ANSC_HANDLE                 hThisObject
                This handle is actually the pointer of this object
                itself.

    return:     operation status.

**********************************************************************/
BOOL CosaWanCnctvtyChk_GetSecondary_IntfName(char *Output)
{

#if defined(WAN_FAILOVER_SUPPORTED) || defined(GATEWAY_FAILOVER_SUPPORTED)
   ANSC_STATUS ret = ANSC_STATUS_SUCCESS;
   char Value[MAX_INTF_NAME_SIZE] = {0};
   errno_t      rc = -1;

   ret = WanCnctvtyChk_GetParameterValue(CURRENT_STANDBY_INTF_DML,Value);
   if (ret == ANSC_STATUS_SUCCESS)
   {
       if (strlen(Value))
       {
          WANCHK_LOG_INFO("%s: Standby Interface Name : %s\n", __FUNCTION__,Value);
          rc = strcpy_s( Output,MAX_INTF_NAME_SIZE,Value);
          ERR_CHK(rc);
          if(rc == EOK)
          {
             return TRUE;
          }
       }
       else
       {
           WANCHK_LOG_INFO("%s: Standby Interface Name is Empty\n", __FUNCTION__);
           return FALSE;
       }
   }
#else
   WANCHK_LOG_INFO("%s: WAN FAILOVER NOT SUPPORTED, Can't Get Backup Interface\n", __FUNCTION__);
#endif
    return FALSE;
}

ANSC_STATUS CosaWanCnctvtyChk_SubscribeRbus(void)
{
#if defined(WAN_FAILOVER_SUPPORTED) || defined(GATEWAY_FAILOVER_SUPPORTED)
    int ret = RBUS_ERROR_SUCCESS;
    int iter=0;
    for(iter=0;iter<ARRAY_SZ(sub_event_param);iter++)
    {
       ret = rbusEvent_Subscribe(rbus_handle, sub_event_param[iter], eventReceiveHandler, NULL, 0);
       if(ret != RBUS_ERROR_SUCCESS)
       {
           WANCHK_LOG_ERROR("WanCnctvtyChkEventConsumer: rbusEvent_Subscribe failed for %s ret: %d\n", 
                                                            sub_event_param[iter],ret);
           return ANSC_STATUS_FAILURE;
       }
    }
#endif
    return ANSC_STATUS_SUCCESS;
}

ANSC_STATUS CosaWanCnctvtyChk_UnSubscribeRbus(void)
{
#if defined(WAN_FAILOVER_SUPPORTED) || defined(GATEWAY_FAILOVER_SUPPORTED)
    int ret = RBUS_ERROR_SUCCESS;
    int iter=0;
    for(iter=0;iter<ARRAY_SZ(sub_event_param);iter++)
    {
       ret = rbusEvent_Unsubscribe(rbus_handle, sub_event_param[iter]);
       if(ret != RBUS_ERROR_SUCCESS)
       {
           WANCHK_LOG_ERROR("WanCnctvtyChkEventConsumer: rbusEvent_Unsubscribe failed for %s ret: %d\n",
                                                            sub_event_param[iter],ret);
           return ANSC_STATUS_FAILURE;
       }
    }
#endif
    return ANSC_STATUS_SUCCESS;
}


ANSC_STATUS CosaWanCnctvtyChk_SubscribeActiveGW(void)
{
#ifdef GATEWAY_FAILOVER_SUPPORTED
    int ret = RBUS_ERROR_SUCCESS;
    int iter=0;
    for(iter=0;iter<ARRAY_SZ(sub_activegwevent_param);iter++)
    {
       ret = rbusEvent_Subscribe(rbus_handle, sub_activegwevent_param[iter], eventReceiveHandler, NULL, 0);
       if(ret != RBUS_ERROR_SUCCESS)
       {
           WANCHK_LOG_ERROR("WanCnctvtyChkEventConsumer: rbusEvent_Subscribe failed for %s ret: %d\n",
                                                            sub_activegwevent_param[iter],ret);
           return ANSC_STATUS_FAILURE;
       }
    }
#endif
    return ANSC_STATUS_SUCCESS;
}

ANSC_STATUS CosaWanCnctvtyChk_UnSubscribeActiveGW(void)
{
#ifdef GATEWAY_FAILOVER_SUPPORTED
    int ret = RBUS_ERROR_SUCCESS;
    int iter=0;
    for(iter=0;iter<ARRAY_SZ(sub_activegwevent_param);iter++)
    {
       ret = rbusEvent_Unsubscribe(rbus_handle, sub_activegwevent_param[iter]);
       if(ret != RBUS_ERROR_SUCCESS)
       {
           WANCHK_LOG_ERROR("WanCnctvtyChkEventConsumer: rbusEvent_Unsubscribe failed for %s ret: %d\n",
                                                            sub_activegwevent_param[iter],ret);
           return ANSC_STATUS_FAILURE;
       }
    }
#endif
    return ANSC_STATUS_SUCCESS;
}

static void eventReceiveHandler(
    rbusHandle_t handle,
    rbusEvent_t const* event,
    rbusEventSubscription_t* subscription)
{
    (void)handle;
    (void)subscription;

    const char* eventName = event->name;
    rbusValue_t valBuff;
    BOOL New_Actv_Status = FALSE;
    char InterfaceName[MAX_INTF_NAME_SIZE] = {0};
    errno_t rc = -1;
    valBuff = rbusObject_GetValue(event->data, NULL );
    if(!valBuff)
    {
        WANCHK_LOG_WARN("WanCnctvtyChkEventConsumer : FAILED, value is NULL\n");
    }
    else
    {
        if ( strcmp(eventName,CURRENT_PRIMARY_INTF_DML) == 0 )
        {
            const char* newValue = rbusValue_GetString(valBuff, NULL);
            WANCHK_LOG_WARN("WanCnctvtyChkEventConsumer : New value of CurrentActiveInterface is = %s\n",
                                                                                        newValue);
            handle_intf_change_event(INTF_PRIMARY, newValue);
        }
        else if (strcmp(eventName,CURRENT_STANDBY_INTF_DML) == 0)
        {
           const char* newValue = rbusValue_GetString(valBuff, NULL);
           WANCHK_LOG_WARN("WanCnctvtyChkEventConsumer : New value of CurrentStandbyInterface is = %s\n",
                                                                                        newValue);
           handle_intf_change_event(INTF_SECONDARY,newValue);
        }
        else if (strcmp(eventName,ACTIVE_GATEWAY_DML) == 0)
        {
           New_Actv_Status = rbusValue_GetBoolean(valBuff);
           WANCHK_LOG_WARN("WanCnctvtyChkEventConsumer : New value of ActiveGateway is = %d\n",
                                                                                New_Actv_Status);
           handle_actv_status_event (New_Actv_Status);
        }
        else if (strcmp(eventName,DNS_SRV_COUNT_DML) == 0)
        {
           unsigned int new_dns_server_count = 0;
           new_dns_server_count = rbusValue_GetUInt32(valBuff);
           if (subscription->userData)
           {
                /* Userdata will have interface name*/
                WANCHK_LOG_INFO("WanCnctvtyChkEventConsumer : New value of DNS server count for Interface %s= %d\n",
                                                    (char *)subscription->userData,new_dns_server_count);
                memset(InterfaceName,0,MAX_INTF_NAME_SIZE);
                rc = strcpy_s(InterfaceName,MAX_INTF_NAME_SIZE,subscription->userData);
                ERR_CHK(rc);
                handle_dns_srvrcnt_event(InterfaceName,new_dns_server_count);

           }
           else
           {
                WANCHK_LOG_ERROR("userData from rbus is NULL,unable to dns count change\n");
           }

        }
        else if (strstr(eventName,DNS_SRV_TABLE_DML))
        {
           errno_t rc = -1;
           int dns_srv_index = 0;
           rc = sscanf_s(eventName, DNS_SRV_ENTRY_DML, &dns_srv_index);
           ERR_CHK(rc);
           const char* newValue = rbusValue_GetString(valBuff, NULL);
           if (subscription->userData)
           {
                memset(InterfaceName,0,MAX_INTF_NAME_SIZE);
                rc = strcpy_s(InterfaceName,MAX_INTF_NAME_SIZE,subscription->userData);
                ERR_CHK(rc);
               WANCHK_LOG_INFO("WanCnctvtyChkEventConsumer : New value of DNS server for Interface %s index %d = %s\n",
                                                                    (char *)subscription->userData,
                                                                    dns_srv_index,newValue);
               handle_dns_srv_addrchange_event(InterfaceName,dns_srv_index,newValue);
           }
           else
           {
                WANCHK_LOG_ERROR("userData from rbus is NULL,unable to dns change for index %d\n",
                                                                                    dns_srv_index);
           }
        }
    }
}

void *WanCnctvtyChk_SysEventHandlerThrd(void *data)
{
    UNREFERENCED_PARAMETER(data);
    pthread_detach(pthread_self());
    int err;
    char name[BUFLEN_128] = {0}, Val[BUFLEN_128] = {0};
    while(1)
    {
        async_id_t getnotification_asyncid;
        memset(name,0,sizeof(name));
        memset(Val,0,sizeof(Val));
        int namelen = sizeof(name);
        int vallen  = sizeof(Val);
        err = sysevent_getnotification(sysevent_fd_wanchk_monitor, sysevent_token_wanchk_monitor, 
                                    name, &namelen, Val , &vallen, &getnotification_asyncid);
        if (err)
        {
            WANCHK_LOG_ERROR("sysevent_getnotification failed with error: %d %s\n", err,__FUNCTION__);
            WANCHK_LOG_ERROR("sysevent_getnotification failed name: %s val : %s\n", name,Val);
            if ( 0 != v_secure_system("pidof syseventd")) 
            {
                WANCHK_LOG_INFO("%s syseventd not running ,breaking the receive notification loop \n"
                                                                                    ,__FUNCTION__);
                break;
            }
        }
        else
        {
            WANCHK_LOG_INFO("%s Recieved notification event  %s with Value %s\n",__FUNCTION__,
                                                                                        name,Val);
            /* Currenlty we need this only on secondary interface, And we can't predict the dns server
            index, since it is not actual DNS info, consider any change in this two events as dns
            server count change, so that list can be repopulated*/
            if(strcmp(name,"backupwan_router_addr")==0 || (strcmp(name,"MeshWANInterface_UlaAddr")==0))
            {
                handle_dns_srvrcnt_event("brRWAN",-1);
            }
#if !defined(WAN_FAILOVER_SUPPORTED) && !defined(GATEWAY_FAILOVER_SUPPORTED)
            else if(strcmp(name,"current_wan_ifname") == 0)
            {
                /* Currently we are dependant only on sysevent for primary in wan failover 
                unsupported*/
                handle_intf_change_event(INTF_PRIMARY, Val);
            }
#endif
        }
    }
    
    return NULL;
}
void handle_dns_srvrcnt_event (char *InterfaceName,unsigned int new_dns_server_count)
{
    ULONG InstanceNumber = 0;
    errno_t rc = -1;
    int ind = -1;
    unsigned int Old_DnsServerCount = 0;
    ANSC_STATUS returnStatus = ANSC_STATUS_SUCCESS;
    if (!InterfaceName)
    {
        WANCHK_LOG_ERROR("%s:InterfaceName is NULL,Unable to update\n",__FUNCTION__);
        return;
    }

    InstanceNumber = GetInstanceNo_FromName(InterfaceName);
    if (InstanceNumber == -1)
    {
        WANCHK_LOG_ERROR("%s:Unable to Find Matching Index for Interface %s\n",__FUNCTION__,InterfaceName);
        return;
    }
    pthread_mutex_lock(&gIntfAccessMutex);
    PWANCNCTVTY_CHK_GLOBAL_INTF_INFO gIntfInfo = &gInterface_List[InstanceNumber];
    Old_DnsServerCount = gIntfInfo->DnsServerCount;
    pthread_mutex_unlock(&gIntfAccessMutex);

    if (Old_DnsServerCount != new_dns_server_count)
    {
        WANCHK_LOG_INFO("%s: Dns server count changed,lets reinitialize dns server data\n",
                                                                        __FUNCTION__);
        rc = strcmp_s( "brRWAN",strlen("brRWAN"),InterfaceName, &ind);
        ERR_CHK(rc);
        if((!ind) && (rc == EOK))
        {
            /* reinitialize dns server entry for the interface*/
            CosaWanCnctvtyChk_DNS_GetEntry(InterfaceName);
        }
        else
        {
            CosaWanCnctvtyChk_DNS_Unsubscribe(Old_DnsServerCount);
            /* reinitialize dns server entry for the interface*/
            CosaWanCnctvtyChk_DNS_GetEntry(InterfaceName);
        }
        returnStatus = wancnctvty_chk_stop_threads(gIntfInfo->IPInterface.InstanceNumber,ACTIVE_MONITOR_THREAD);
        if (returnStatus != ANSC_STATUS_SUCCESS)
        {
            WANCHK_LOG_ERROR("%s:%d Unable to stop threads",__FUNCTION__,__LINE__);
        }

        returnStatus = wancnctvty_chk_start_threads(gIntfInfo->IPInterface.InstanceNumber,ACTIVE_MONITOR_THREAD);
        if (returnStatus != ANSC_STATUS_SUCCESS)
        {
            WANCHK_LOG_ERROR("%s:%d Unable to start threads",__FUNCTION__,__LINE__);
        }
    }


}

void handle_dns_srv_addrchange_event (char *InterfaceName,int dns_srv_index,
                                                                    const char *newValue)
{
    errno_t  rc = -1;
    ANSC_STATUS returnStatus = ANSC_STATUS_SUCCESS;
    BOOL restart_threads     = FALSE;

    if (!InterfaceName)
    {
        WANCHK_LOG_ERROR("%s:InterfaceName is NULL,Unable to update\n",__FUNCTION__);
        return;
    }

    ULONG InstanceNumber = GetInstanceNo_FromName(InterfaceName);
    if (InstanceNumber == -1)
    {
        WANCHK_LOG_ERROR("%s:Unable to Find Matching Index for Interface %s\n",__FUNCTION__,InterfaceName);
        return;
    }

    pthread_mutex_lock(&gIntfAccessMutex);
    PWANCNCTVTY_CHK_GLOBAL_INTF_INFO gIntfInfo = &gInterface_List[InstanceNumber];
    if (gIntfInfo->DnsServerList[dns_srv_index-1].dns_type == DNS_SRV_IPV4)
    {
        if (strcmp(gIntfInfo->DnsServerList[dns_srv_index-1].IPAddress.IPv4Address,newValue))
        {
            WANCHK_LOG_INFO("%s: Dns server IP address changed,update index %d\n",
                                                                __FUNCTION__,dns_srv_index);

            memset(gIntfInfo->DnsServerList[dns_srv_index-1].IPAddress.IPv4Address,0,
                                                                               IPv4_STR_LEN);
            rc = strcpy_s(gIntfInfo->DnsServerList[dns_srv_index-1].IPAddress.IPv4Address,
                                                                      IPv4_STR_LEN,newValue);
            ERR_CHK(rc);
            WANCHK_LOG_INFO("%s: updated index %d with IPAddress %s\n",
                                            __FUNCTION__,dns_srv_index,
                        gIntfInfo->DnsServerList[dns_srv_index-1].IPAddress.IPv4Address);
            restart_threads = TRUE;
        }
        else
        {
            WANCHK_LOG_INFO("%s: Dns server Index %d already has upated value\n",
                                                                __FUNCTION__,dns_srv_index);
        }
    }
    else
    {
        if (strcmp(gIntfInfo->DnsServerList[dns_srv_index-1].IPAddress.IPv6Address,newValue))
        {
            WANCHK_LOG_INFO("%s: Dns server IP address changed,update index %d\n",
                                                                __FUNCTION__,dns_srv_index);

            memset(gIntfInfo->DnsServerList[dns_srv_index-1].IPAddress.IPv6Address,0,
                                                                               IPv6_STR_LEN);
            rc = strcpy_s(gIntfInfo->DnsServerList[dns_srv_index-1].IPAddress.IPv6Address,
                                                                      IPv6_STR_LEN,newValue);
            ERR_CHK(rc);
            WANCHK_LOG_INFO("%s: updated index %d with IPAddress %s\n",
                                            __FUNCTION__,dns_srv_index,
                        gIntfInfo->DnsServerList[dns_srv_index-1].IPAddress.IPv6Address);
            restart_threads = TRUE;
        }
        else
        {
            WANCHK_LOG_INFO("%s: Dns server Index %d already has upated value\n",
                                                                __FUNCTION__,dns_srv_index);
        }

    }
    pthread_mutex_unlock(&gIntfAccessMutex);
    if (restart_threads == TRUE)
    {
        /* Chnage in dns won't affect passive threads*/
        returnStatus = wancnctvty_chk_stop_threads(gIntfInfo->IPInterface.InstanceNumber,ACTIVE_MONITOR_THREAD);
        if (returnStatus != ANSC_STATUS_SUCCESS)
        {
            WANCHK_LOG_ERROR("%s:%d Unable to stop threads",__FUNCTION__,__LINE__);
        }

        returnStatus = wancnctvty_chk_start_threads(gIntfInfo->IPInterface.InstanceNumber,ACTIVE_MONITOR_THREAD);
        if (returnStatus != ANSC_STATUS_SUCCESS)
        {
                WANCHK_LOG_ERROR("%s:%d Unable to start threads",__FUNCTION__,__LINE__);
        }
    }
}


void handle_actv_status_event (BOOL new_status)
{
    if (g_wanconnectivity_check_active != new_status)
    {
       WANCHK_LOG_INFO("%s : New value of ActiveGateway is = %d\n",__FUNCTION__,new_status);
       if (new_status)
       {
          /* we are already in disabled state, no need to free any memory*/
          g_wanconnectivity_check_active = TRUE;
          if (CosaWanCnctvtyChk_Init_URLTable () != ANSC_STATUS_SUCCESS)
          {
              WANCHK_LOG_ERROR("%s: Unable to init URL table\n", __FUNCTION__);
          }
          if (CosaWanCnctvtyChk_Init_IntfTable () != ANSC_STATUS_SUCCESS)
          {
              WANCHK_LOG_ERROR("%s: Unable to init interface table\n", __FUNCTION__);
          }
          CosaWanCnctvtyChk_SubscribeRbus();
          CosaWanCnctvtyChk_StartSysevent_listener();
       }
       else
       {
     /* we are moving to disabled state, bring down*/
        CosaWanCnctvtyChk_UnSubscribeRbus();
        CosaWanCnctvtyChk_StopSysevent_listener();
        /* Deinit interface Table*/
        if (CosaWanCnctvtyChk_DeInit_IntfTable() != ANSC_STATUS_SUCCESS)
        {
             WANCHK_LOG_ERROR("%s: Unable to deinit interface table\n", __FUNCTION__);
        }
        /* Deinit URL list*/
        if (CosaWanCnctvtyChk_Remove_Urllist() != ANSC_STATUS_SUCCESS)
        {
             WANCHK_LOG_ERROR("%s: Unable to remove URL list\n", __FUNCTION__);
        }
        g_wanconnectivity_check_active = FALSE;
        // CosaWanCnctvtyChk_UnReg_elements(FEATURE_ENABLED_DML);
       }
    }
}

ANSC_STATUS CosaWanCnctvtyChk_DeInit_IntfTable(VOID)
{

    ANSC_STATUS returnStatus = ANSC_STATUS_SUCCESS;

    CosaWanCnctvtyChk_Remove_Intf(PRIMARY_INTF_INDEX);
    CosaWanCnctvtyChk_Remove_Intf(SECONDARY_INTF_INDEX);
    // TO - DO un register interface rows

    return returnStatus;
}

void handle_intf_change_event(COSA_WAN_CNCTVTY_CHK_EVENTS event,const char *new_value)
{
   char Value[MAX_INTF_NAME_SIZE] = {0};
   errno_t rc = -1;
   int ind = -1;
   if (event == INTF_PRIMARY)
   {
      WANCHK_LOG_INFO("%s : New value of ActiveGateway is = %s\n",__FUNCTION__,new_value);
      CosaWanCnctvtyChk_Remove_Intf(PRIMARY_INTF_INDEX);
      /* If new_value is empty, no need to call init*/
      if (strlen(new_value))
      {
         if (CosaWanCnctvtyChk_Init_Intf(PRIMARY_INTF_INDEX) == ANSC_STATUS_SUCCESS)
         {
#if defined(WAN_FAILOVER_SUPPORTED) || defined(GATEWAY_FAILOVER_SUPPORTED)
            CosaWanCnctvtyChk_Remove_Intf(SECONDARY_INTF_INDEX);
            CosaWanCnctvtyChk_Init_Intf(SECONDARY_INTF_INDEX);
#endif
         }
      }
   }
   else if(event == INTF_SECONDARY)
   {
      WANCHK_LOG_INFO("%s : New value of Secondary Gateway is = %s\n",__FUNCTION__,new_value);
      if (CosaWanCnctvtyChk_IsPrimary_Configured() == FALSE)
      {
         WANCHK_LOG_INFO("%s : Primary Interface Unconfigured Ignore Secondary event\n",__FUNCTION__);
         return;
      }
      else
      {
          if(CosaWanCnctvtyChk_GetPrimary_IntfName(Value) == TRUE)
          {
              rc = strcmp_s(new_value, strlen(new_value), Value, &ind);
              ERR_CHK(rc);
              if((!ind) && (rc == EOK))
              {
                  WANCHK_LOG_INFO("%s : Primary and Backup Interface are same, Ignore Secondary event\n",
                                                                                      __FUNCTION__);
                  return;
              }
          }
          else
          {
              WANCHK_LOG_ERROR("%s:%d Unable to get Primary interface\n",__FUNCTION__,__LINE__);
              return;
          }
      }
      CosaWanCnctvtyChk_Remove_Intf(SECONDARY_INTF_INDEX);
      if (strlen(new_value))
      {
      	 CosaWanCnctvtyChk_Init_Intf(SECONDARY_INTF_INDEX);
      }
   }       
        
}

BOOL CosaWanCnctvtyChk_IsPrimary_Configured()
{
    pthread_mutex_lock(&gIntfAccessMutex);
    PWANCNCTVTY_CHK_GLOBAL_INTF_INFO gIntfInfo = &gInterface_List[PRIMARY_INTF_INDEX-1];
    if (gIntfInfo->IPInterface.Configured == FALSE)
    {
        pthread_mutex_unlock(&gIntfAccessMutex);
        return FALSE;
    }
    pthread_mutex_unlock(&gIntfAccessMutex);
    return TRUE;
}


ANSC_STATUS CosaWanCnctvtyChk_Remove_Intf (wan_intf_index_t IntfIndex)
{
    ANSC_STATUS returnStatus = ANSC_STATUS_SUCCESS;
    char InterfaceName[MAX_INTF_NAME_SIZE] = {0};
    unsigned int DnsServerCount = 0;
    errno_t ret = -1;
    pthread_mutex_lock(&gIntfAccessMutex);
    PWANCNCTVTY_CHK_GLOBAL_INTF_INFO gIntfInfo = &gInterface_List[IntfIndex-1];
    if (gIntfInfo->IPInterface.Configured == FALSE)
    {
        WANCHK_LOG_INFO("Unconfigured Interface,No need of removal\n");
        pthread_mutex_unlock(&gIntfAccessMutex);
        return ANSC_STATUS_SUCCESS;
    }
    ret = strcpy_s(InterfaceName,MAX_INTF_NAME_SIZE,gIntfInfo->IPInterface.InterfaceName);
    ERR_CHK(ret);
    DnsServerCount = gIntfInfo->DnsServerCount;
    pthread_mutex_unlock(&gIntfAccessMutex);
    // Future Scope, currently taking primary index as 1 and backup as 2 as hardcoded values
    // currently no other indexes are expected, if needed, derive a logic to fetch instance
    // number based on event
    returnStatus = wancnctvty_chk_stop_threads(IntfIndex,ALL_THREADS);
    if (returnStatus != ANSC_STATUS_SUCCESS)
    {
        WANCHK_LOG_ERROR("%s:%d Unable to stop threads\n",__FUNCTION__,__LINE__);
        return ANSC_STATUS_FAILURE;
    }
    int ind = -1;
    ret = strcmp_s( "brRWAN",strlen("brRWAN"),InterfaceName, &ind);
    ERR_CHK(ret);
    if((!ind) && (ret == EOK))
    {
	/*    
        sysevent_rmnotification(sysevent_fd_wanchk_monitor, sysevent_token_wanchk_monitor, 
                                                                backupwan_router_addr_asyncid);
        sysevent_rmnotification(sysevent_fd_wanchk_monitor, sysevent_token_wanchk_monitor, 
                                                            MeshWANInterface_UlaAddr_asyncid);
	*/
    }
    else
    {
        CosaWanCnctvtyChk_DNS_Unsubscribe(DnsServerCount);
    }
    CosaDml_glblintfdb_delentry(IntfIndex);
    rbusError_t rc;
    char rowName[RBUS_MAX_NAME_LENGTH] = {0};
    snprintf(rowName, RBUS_MAX_NAME_LENGTH, "%s%d",WANCHK_INTF_TABLE, IntfIndex);
    rc = rbusTable_unregisterRow(rbus_table_handle, rowName);
    if(rc != RBUS_ERROR_SUCCESS)
    {
        WANCHK_LOG_ERROR("Unregister failed for %s\n",rowName);
        returnStatus = ANSC_STATUS_FAILURE;
    }
    return returnStatus;
}

ANSC_STATUS CosaWanCnctvtyChk_Remove_Urllist (VOID)
{
    ANSC_STATUS returnStatus                          = ANSC_STATUS_SUCCESS;
    PSINGLE_LINK_ENTRY              pSListEntry       = NULL;
    PCOSA_CONTEXT_LINK_OBJECT       pCxtLink          = NULL;
    PCOSA_DML_WANCNCTVTY_CHK_URL_INFO pUrlInfo        = NULL;
    rbusError_t rc;
    char rowName[RBUS_MAX_NAME_LENGTH] = {0};

    WANCHK_LOG_INFO("%s Deallocate URL List\n",__FUNCTION__);

    pthread_mutex_lock(&gUrlAccessMutex);
    pSListEntry           = AnscSListGetFirstEntry(&gpUrlList);
    while( pSListEntry != NULL)
    {
        pCxtLink          = ACCESS_COSA_CONTEXT_LINK_OBJECT(pSListEntry);
        pSListEntry       = AnscSListGetNextEntry(pSListEntry);
        pUrlInfo = (PCOSA_DML_WANCNCTVTY_CHK_URL_INFO)pCxtLink->hContext;
        if (AnscSListPopEntryByLink(&gpUrlList, &pCxtLink->Linkage) == TRUE)
        {
            WANCHK_LOG_INFO("Found URL %s in list, Deallocate it\n",pUrlInfo->URL);
            memset(rowName,0,RBUS_MAX_NAME_LENGTH);
            snprintf(rowName, RBUS_MAX_NAME_LENGTH, "%s%ld",WANCHK_TEST_URL_TABLE,
                                                                        pUrlInfo->InstanceNumber);
            rc = rbusTable_unregisterRow(rbus_table_handle, rowName);
            if(rc != RBUS_ERROR_SUCCESS)
            {
                WANCHK_LOG_ERROR("Unregister failed for %s\n",rowName);
            }   
            if (pUrlInfo)
            {
                AnscFreeMemory(pUrlInfo);
                pUrlInfo = NULL;
            }
            if (pCxtLink)
            {
                COSA_CONTEXT_LINK_INITIATION_CONTENT(pCxtLink);
                AnscFreeMemory(pCxtLink);
                pCxtLink = NULL; 
            }
        }
    }
    gulUrlNextInsNum = 1;
    pthread_mutex_unlock(&gUrlAccessMutex);
    return returnStatus;
}

ANSC_STATUS CosaWanCnctvtyChk_DNS_Unsubscribe(unsigned int DnsServerCount)
{
    int rbus_ret = RBUS_ERROR_SUCCESS;
        /* Unscubscribe and re-subscribe while if needed while initialization*/
    int iter;
    errno_t rc = -1;
    char rbus_eventname[BUFLEN_256] = {0};
    for(iter=1;iter<=DnsServerCount;iter++)
    {
        memset(rbus_eventname,0,sizeof(rbus_eventname));
        rc = sprintf_s(rbus_eventname, sizeof(rbus_eventname),DNS_SRV_ENTRY_DML,iter);
        if (rc < EOK)
        {
            ERR_CHK(rc);
        }
        rbus_ret = rbusEvent_Unsubscribe(rbus_table_handle, rbus_eventname);
        if (rbus_ret != RBUS_ERROR_SUCCESS)
        {
            WANCHK_LOG_ERROR("rbusEvent_Subscribe failed for %s ret: %d\n",
                                                         rbus_eventname,rbus_ret);
            return ANSC_STATUS_FAILURE;
        }
    }

    rbus_ret = rbusEvent_Unsubscribe(rbus_table_handle, DNS_SRV_COUNT_DML);
    if (rbus_ret != RBUS_ERROR_SUCCESS)
    {
        WANCHK_LOG_ERROR("rbusEvent_UnSubscribe failed for %s ret: %d\n",
                                                     DNS_SRV_COUNT_DML,rbus_ret);
        return ANSC_STATUS_FAILURE;
    }
    memset(subscribe_userData,0,MAX_INTF_NAME_SIZE);
    return ANSC_STATUS_SUCCESS;

}

ANSC_STATUS CosaDmlStoreIntfCfg(PCOSA_DML_WANCNCTVTY_CHK_INTF_INFO pIPInterface)
{
    errno_t rc = -1;
    char paramName[BUFLEN_256] = {0};
    char buf[BUFLEN_256] = {0};
    uint32_t bitmask = 0;

    bitmask = pIPInterface->Cfg_bitmask;

    if (bitmask & INTF_CFG_ENABLE)
    {
        rc = sprintf_s(paramName,sizeof(paramName),"wanconnectivity_chk_intf_enabled_%d",
                                                                        pIPInterface->InstanceNumber);
        if (rc < EOK)
        {
            ERR_CHK(rc);
        }

        if (syscfg_set(NULL, paramName, (pIPInterface->Enable ? "true" : "false")) != 0)
        {
            AnscTraceWarning(("%s: syscfg_set failed for Interface Enable %d\n", __FUNCTION__,
                                                                       pIPInterface->Enable));
            return ANSC_STATUS_FAILURE;
        }
    }

    if (bitmask & INTF_CFG_PASSIVE_ENABLE)
    {
        memset(paramName,0,sizeof(paramName));
        rc = sprintf_s(paramName,sizeof(paramName),"wanconnectivity_chk_passivemntr_enabled_%d",
                                                                        pIPInterface->InstanceNumber);
        if (rc < EOK)
        {
            ERR_CHK(rc);
        }

        if (syscfg_set(NULL, paramName, (pIPInterface->PassiveMonitor ? "true" : "false")) != 0)
        {
            AnscTraceWarning(("%s: syscfg_set failed for PassiveMonitor %d\n", __FUNCTION__,
                                                                        pIPInterface->PassiveMonitor));
            return ANSC_STATUS_FAILURE;
        }
    }

    if (bitmask & INTF_CFG_PASSIVE_ENABLE)
    {
        memset(buf,0,sizeof(buf));
        memset(paramName,0,sizeof(paramName));
        rc = sprintf_s(paramName,sizeof(paramName),"wanconnectivity_chk_passivemntr_timeout_%d",
                                                                        pIPInterface->InstanceNumber);
        if (rc < EOK)
        {
            ERR_CHK(rc);
        }

        rc = sprintf_s(buf,sizeof(buf),"%lu",pIPInterface->PassiveMonitorTimeout);
        if (rc < EOK)
        {
            ERR_CHK(rc);
        }

        if (syscfg_set(NULL, paramName, buf) != 0)
        {
            AnscTraceWarning(("%s: syscfg_set failed for PassiveMonitorTimeout %lu\n", __FUNCTION__,
                                                                pIPInterface->PassiveMonitorTimeout));
            return ANSC_STATUS_FAILURE;
        }
    }

    if (bitmask & INTF_CFG_ACTIVE_ENABLE)
    {
        memset(paramName,0,sizeof(paramName));
        rc = sprintf_s(paramName,sizeof(paramName),"wanconnectivity_chk_actvmntr_enabled_%d",
                                                                        pIPInterface->InstanceNumber);
        if (rc < EOK)
        {
            ERR_CHK(rc);
        }

        if (syscfg_set(NULL, paramName, (pIPInterface->ActiveMonitor ? "true" : "false")) != 0)
        {
            AnscTraceWarning(("%s: syscfg_set failed for ActiveMonitor %d\n", __FUNCTION__,
                                                                        pIPInterface->ActiveMonitor));
            return ANSC_STATUS_FAILURE;
        }
    }

    if (bitmask & INTF_CFG_ACTIVE_INTERVAL)
    {
        memset(buf,0,sizeof(buf));
        memset(paramName,0,sizeof(paramName));
        rc = sprintf_s(paramName,sizeof(paramName),"wanconnectivity_chk_actvmntr_interval_%d",
                                                                        pIPInterface->InstanceNumber);
        if (rc < EOK)
        {
            ERR_CHK(rc);
        }

        rc = sprintf_s(buf,sizeof(buf),"%lu",pIPInterface->ActiveMonitorInterval);
        if (rc < EOK)
        {
            ERR_CHK(rc);
        }

        if (syscfg_set(NULL, paramName, buf) != 0)
        {
            AnscTraceWarning(("%s: syscfg_set failed for ActiveMonitorInterval %ld\n", __FUNCTION__,
                                                                pIPInterface->ActiveMonitorInterval));
            return ANSC_STATUS_FAILURE;
        }
    }

    if (bitmask & INTF_CFG_QUERY_TIMEOUT)
    {
        memset(buf,0,sizeof(buf));
        memset(paramName,0,sizeof(paramName));
        rc = sprintf_s(paramName,sizeof(paramName),"wanconnectivity_chk_query_timeout_%d",
                                                                        pIPInterface->InstanceNumber);
        if (rc < EOK)
        {
            ERR_CHK(rc);
        }

        rc = sprintf_s(buf,sizeof(buf),"%lu",pIPInterface->QueryTimeout);
        if (rc < EOK)
        {
            ERR_CHK(rc);
        }

        if (syscfg_set(NULL, paramName, buf) != 0)
        {
            AnscTraceWarning(("%s: syscfg_set failed for QueryTimeout %lu\n", __FUNCTION__,
                                                                        pIPInterface->QueryTimeout));
            return ANSC_STATUS_FAILURE;
        }
    }

    if (bitmask & INTF_CFG_QUERY_RETRY)
    {
        memset(buf,0,sizeof(buf));
        memset(paramName,0,sizeof(paramName));
        rc = sprintf_s(paramName,sizeof(paramName),"wanconnectivity_chk_query_retry_%d",
                                                                        pIPInterface->InstanceNumber);
        if (rc < EOK)
        {
            ERR_CHK(rc);
        }

        rc = sprintf_s(buf,sizeof(buf),"%lu",pIPInterface->QueryRetry);
        if (rc < EOK)
        {
            ERR_CHK(rc);
        }

        if (syscfg_set(NULL, paramName, buf) != 0)
        {
            AnscTraceWarning(("%s: syscfg_set failed for QueryRetry %ld\n", __FUNCTION__,
                                                                         pIPInterface->QueryRetry));
            return ANSC_STATUS_FAILURE;
        }
    }

    if (bitmask & INTF_CFG_RECORDTYPE)
    {
        memset(buf,0,sizeof(buf));
        memset(paramName,0,sizeof(paramName));
        rc = sprintf_s(paramName,sizeof(paramName),"wanconnectivity_chk_recordtype_%d",
                                                                        pIPInterface->InstanceNumber);
        if (rc < EOK)
        {
            ERR_CHK(rc);
        }

        if (syscfg_set(NULL, paramName, pIPInterface->RecordType) != 0)
        {
            AnscTraceWarning(("%s: syscfg_set failed for RecordType %s\n", __FUNCTION__,
                                                                            pIPInterface->RecordType));
            return ANSC_STATUS_FAILURE;
        }
    }

    if (bitmask & INTF_CFG_SERVERTYPE)
    {
        memset(buf,0,sizeof(buf));
        memset(paramName,0,sizeof(paramName));
        rc = sprintf_s(paramName,sizeof(paramName),"wanconnectivity_chk_servertype_%d",
                                                                        pIPInterface->InstanceNumber);
        if (rc < EOK)
        {
            ERR_CHK(rc);
        }

        if (syscfg_set(NULL, paramName, pIPInterface->ServerType) != 0)
        {
            AnscTraceWarning(("%s: syscfg_set failed for ServerType %s\n", __FUNCTION__,
                                                                            pIPInterface->ServerType));
            return ANSC_STATUS_FAILURE;
        }
    }

    if (bitmask)
    {
        if (syscfg_commit() != 0)
        {
           AnscTraceWarning(("%s: syscfg commit failed for interface %s\n", __FUNCTION__,
                                                                        pIPInterface->InterfaceName));
           return ANSC_STATUS_FAILURE;
        }
    }

    return ANSC_STATUS_SUCCESS;

}

ANSC_STATUS CosaDmlGetIntfCfg(PCOSA_DML_WANCNCTVTY_CHK_INTF_INFO pIPInterface,BOOL use_default)
{
    errno_t rc = -1;    
    char buf[BUFLEN_256] = {0};
    char paramName[BUFLEN_128] = {0};

    /* set to defaults , override if config exists*/
    if (use_default)
    {
        pIPInterface->Enable  = DEF_INTF_ENABLE;
        if (pIPInterface->InstanceNumber == PRIMARY_INTF_INDEX)
        {
            pIPInterface->ActiveMonitor = DEF_ACTIVE_MONITOR_PRIMARY_ENABLE;
            pIPInterface->PassiveMonitor = DEF_PASSIVE_MONITOR_PRIMARY_ENABLE;
        }
        else
        {
            pIPInterface->ActiveMonitor = DEF_ACTIVE_MONITOR_BACKUP_ENABLE;
            pIPInterface->PassiveMonitor = DEF_PASSIVE_MONITOR_BACKUP_ENABLE;
        }
        pIPInterface->PassiveMonitorTimeout = DEF_PASSIVE_MONITOR_TIMEOUT;
        pIPInterface->ActiveMonitorInterval = DEF_ACTIVE_MONITOR_INTERVAL;
        pIPInterface->QueryTimeout = DEF_QUERY_TIMEOUT;
        pIPInterface->QueryRetry = DEF_QUERY_RETRY;
        rc = strcpy_s(pIPInterface->RecordType,MAX_RECORD_TYPE_SIZE , DEF_QUERY_RECORDTYPE);
        ERR_CHK(rc);
        rc = strcpy_s(pIPInterface->ServerType,MAX_SERVER_TYPE_SIZE , DEF_QUERY_SERVERTYPE);
        ERR_CHK(rc);
    }

    rc = sprintf_s(paramName,sizeof(paramName),"wanconnectivity_chk_intf_enabled_%d",
                                                                    pIPInterface->InstanceNumber);
    if (rc < EOK)
    {
        ERR_CHK(rc);
    }

    /* Fetch interface Enable status*/
    if((syscfg_get( NULL, paramName, buf, sizeof(buf)) == 0 ) && (buf[0] != '\0') )
    {
        pIPInterface->Enable = (!strcmp(buf, "true")) ? TRUE : FALSE;
    }

    memset(buf,0,sizeof(buf));
    memset(paramName,0,sizeof(paramName));
    rc = sprintf_s(paramName,sizeof(paramName),"wanconnectivity_chk_passivemntr_enabled_%d",
                                                                    pIPInterface->InstanceNumber);
    if (rc < EOK)
    {
        ERR_CHK(rc);
    }

    /* Fetch passive monitor status*/
    if((syscfg_get( NULL, paramName, buf, sizeof(buf)) == 0 ) && (buf[0] != '\0') )
    {
        pIPInterface->PassiveMonitor = (!strcmp(buf, "true")) ? TRUE : FALSE;
    }

    memset(buf,0,sizeof(buf));
    memset(paramName,0,sizeof(paramName));
    rc = sprintf_s(paramName,sizeof(paramName),"wanconnectivity_chk_passivemntr_timeout_%d",
                                                                    pIPInterface->InstanceNumber);
    if (rc < EOK)
    {
        ERR_CHK(rc);
    }
    /* fetch passive monitor timeout*/
    if((syscfg_get( NULL, paramName, buf, sizeof(buf)) == 0 ) && (buf[0] != '\0') )
    {
        pIPInterface->PassiveMonitorTimeout = atoi(buf);
    }

    memset(buf,0,sizeof(buf));
    memset(paramName,0,sizeof(paramName));
    rc = sprintf_s(paramName,sizeof(paramName),"wanconnectivity_chk_actvmntr_enabled_%d",
                                                                    pIPInterface->InstanceNumber);
    if (rc < EOK)
    {
        ERR_CHK(rc);
    }
    /* Fetch Active monitor status*/
    if((syscfg_get( NULL, paramName, buf, sizeof(buf)) == 0 ) && (buf[0] != '\0') )
    {
        pIPInterface->ActiveMonitor = (!strcmp(buf, "true")) ? TRUE : FALSE;
    }

    memset(buf,0,sizeof(buf));
    memset(paramName,0,sizeof(paramName));
    rc = sprintf_s(paramName,sizeof(paramName),"wanconnectivity_chk_actvmntr_interval_%d",
                                                                    pIPInterface->InstanceNumber);
    if (rc < EOK)
    {
        ERR_CHK(rc);
    }

    /* fetch Active monitor timeout*/
    if((syscfg_get( NULL,paramName, buf, sizeof(buf)) == 0 ) && (buf[0] != '\0') )
    {
        pIPInterface->ActiveMonitorInterval = atoi(buf);
    }

    memset(buf,0,sizeof(buf));
    memset(paramName,0,sizeof(paramName));
    rc = sprintf_s(paramName,sizeof(paramName),"wanconnectivity_chk_query_timeout_%d",
                                                                    pIPInterface->InstanceNumber);
    if (rc < EOK)
    {
        ERR_CHK(rc);
    }

    /* fetch Active monitor timeout*/
    if((syscfg_get( NULL, paramName, buf, sizeof(buf)) == 0 ) && (buf[0] != '\0') )
    {
        pIPInterface->QueryTimeout = atoi(buf);
    }

    memset(buf,0,sizeof(buf));
    memset(paramName,0,sizeof(paramName));
    rc = sprintf_s(paramName,sizeof(paramName),"wanconnectivity_chk_query_retry_%d",
                                                                    pIPInterface->InstanceNumber);
    if (rc < EOK)
    {
        ERR_CHK(rc);
    }

    if((syscfg_get( NULL, paramName, buf, sizeof(buf)) == 0 ) && (buf[0] != '\0') )
    {
        pIPInterface->QueryRetry = atoi(buf);
    }

    memset(buf,0,sizeof(buf));
    memset(paramName,0,sizeof(paramName));
    rc = sprintf_s(paramName,sizeof(paramName),"wanconnectivity_chk_recordtype_%d",
                                                                    pIPInterface->InstanceNumber);
    if (rc < EOK)
    {
        ERR_CHK(rc);
    }

    /* Fetch Record Type*/
    if((syscfg_get( NULL, paramName, buf, sizeof(buf)) == 0 ) && (buf[0] != '\0') )
    {
        memset(pIPInterface->RecordType,0,MAX_RECORD_TYPE_SIZE);
        rc = strcpy_s(pIPInterface->RecordType,MAX_RECORD_TYPE_SIZE , buf);
        ERR_CHK(rc);
    }

    memset(buf,0,sizeof(buf));
    memset(paramName,0,sizeof(paramName));
    rc = sprintf_s(paramName,sizeof(paramName),"wanconnectivity_chk_servertype_%d",
                                                                    pIPInterface->InstanceNumber);
    if (rc < EOK)
    {
        ERR_CHK(rc);
    }

    /* Fetch server Type*/
    if((syscfg_get( NULL, paramName, buf, sizeof(buf)) == 0 ) &&  (buf[0] != '\0') )
    {
        memset(pIPInterface->ServerType,0,MAX_SERVER_TYPE_SIZE);
        rc = strcpy_s(pIPInterface->ServerType,MAX_SERVER_TYPE_SIZE , buf);
        ERR_CHK(rc);
    }

    return ANSC_STATUS_SUCCESS;

}


ANSC_STATUS CosaDml_glblintfdb_delentry(ULONG InstanceNumber)
{
    pthread_mutex_lock(&gIntfAccessMutex);
    PWANCNCTVTY_CHK_GLOBAL_INTF_INFO gIntfInfo = &gInterface_List[InstanceNumber-1];
    if (gIntfInfo &&  gIntfInfo->IPInterface.Configured)
    {
        if (gIntfInfo->DnsServerList)
        {
            free(gIntfInfo->DnsServerList);
            gIntfInfo->DnsServerList = NULL;
        }
        memset(gIntfInfo,0,sizeof(WANCNCTVTY_CHK_GLOBAL_INTF_INFO));
        if (gIntfCount)
            gIntfCount--;
    }
    pthread_mutex_unlock(&gIntfAccessMutex);
    return ANSC_STATUS_SUCCESS;
}

ANSC_STATUS CosaDml_glblintfdb_updateentry(PCOSA_DML_WANCNCTVTY_CHK_INTF_INFO pIPInterface)
{
    if (!pIPInterface)
    {
        WANCHK_LOG_ERROR("%s:%d Interface data is NULL",__FUNCTION__,__LINE__);
        return ANSC_STATUS_FAILURE;
    }

    uint32_t bitmask = pIPInterface->Cfg_bitmask;

    pthread_mutex_lock(&gIntfAccessMutex);
    ULONG InstanceNumber = pIPInterface->InstanceNumber;
    errno_t rc = -1;
    PWANCNCTVTY_CHK_GLOBAL_INTF_INFO gIntfInfo = &gInterface_List[InstanceNumber-1];
    gIntfInfo->IPInterface.InstanceNumber = pIPInterface->InstanceNumber;
    if (bitmask & INTF_CFG_ENABLE)
    {
        gIntfInfo->IPInterface.Enable = pIPInterface->Enable;
    }
    memset(gIntfInfo->IPInterface.Alias,0,MAX_INTF_NAME_SIZE);
    rc = strcpy_s(gIntfInfo->IPInterface.Alias,MAX_INTF_NAME_SIZE , pIPInterface->Alias);
    ERR_CHK(rc);
    memset(gIntfInfo->IPInterface.InterfaceName,0,MAX_INTF_NAME_SIZE);
    rc = strcpy_s(gIntfInfo->IPInterface.InterfaceName,MAX_INTF_NAME_SIZE ,
                                                            pIPInterface->InterfaceName);
    ERR_CHK(rc);
    if (bitmask & INTF_CFG_PASSIVE_ENABLE)
    {
        gIntfInfo->IPInterface.PassiveMonitor = pIPInterface->PassiveMonitor;
    }

    if (bitmask & INTF_CFG_PASSIVE_TIMEOUT)
    {    
        gIntfInfo->IPInterface.PassiveMonitorTimeout = pIPInterface->PassiveMonitorTimeout;
    }
    if (bitmask & INTF_CFG_ACTIVE_ENABLE)
    {
        gIntfInfo->IPInterface.ActiveMonitor = pIPInterface->ActiveMonitor;
    }
    if (bitmask & INTF_CFG_ACTIVE_INTERVAL)
    {
        gIntfInfo->IPInterface.ActiveMonitorInterval = pIPInterface->ActiveMonitorInterval;
    }
    if (bitmask & INTF_CFG_QUERYNOW_ENABLE)
    {
        gIntfInfo->IPInterface.QueryNow = pIPInterface->QueryNow;
    }
    if (bitmask & INTF_CFG_QUERY_TIMEOUT)
    {
        gIntfInfo->IPInterface.QueryTimeout = pIPInterface->QueryTimeout;
    }
    if (bitmask & INTF_CFG_QUERY_RETRY)
    {
        gIntfInfo->IPInterface.QueryRetry = pIPInterface->QueryRetry;
    }
    if (bitmask & INTF_CFG_RECORDTYPE)
    {
        memset(gIntfInfo->IPInterface.RecordType,0,MAX_RECORD_TYPE_SIZE);
        rc = strcpy_s(gIntfInfo->IPInterface.RecordType,MAX_RECORD_TYPE_SIZE ,
                                                                    pIPInterface->RecordType);
        ERR_CHK(rc);
    }
    if (bitmask & INTF_CFG_SERVERTYPE)
    {
        memset(gIntfInfo->IPInterface.ServerType,0,MAX_SERVER_TYPE_SIZE);
        rc = strcpy_s(gIntfInfo->IPInterface.ServerType,MAX_SERVER_TYPE_SIZE ,
                                                                    pIPInterface->ServerType);
        ERR_CHK(rc);
    }
    pthread_mutex_unlock(&gIntfAccessMutex);
    return ANSC_STATUS_SUCCESS;
}

ANSC_STATUS CosaDml_glblintfdb_update_dnsentry(char *InterfaceName,unsigned int DnsServerCount,
                                                PCOSA_DML_WANCNCTVTY_CHK_DNSSRV_INFO pDnsSrvInfo)
{
    PWANCNCTVTY_CHK_GLOBAL_INTF_INFO gIntfInfo = NULL;
    if (!InterfaceName)
    {
        WANCHK_LOG_ERROR("%s:InterfaceName is NULL,Unable to update\n",__FUNCTION__);
        return ANSC_STATUS_FAILURE;
    }

    ULONG InstanceNumber = GetInstanceNo_FromName(InterfaceName);
    if (InstanceNumber == -1)
    {
        WANCHK_LOG_ERROR("%s:Unable to Find Matching Index for Interface %s\n",__FUNCTION__,InterfaceName);
        return ANSC_STATUS_FAILURE;
    }
    pthread_mutex_lock(&gIntfAccessMutex);
    gIntfInfo = &gInterface_List[InstanceNumber];
    uint32_t Old_DnsServerCount = gIntfInfo->DnsServerCount;

    if (gIntfInfo->DnsServerCount && gIntfInfo->DnsServerList)
    {
        WANCHK_LOG_INFO("Free the older dns server list\n");
        free(gIntfInfo->DnsServerList);
        gIntfInfo->DnsServerList = NULL;
        gIntfInfo->DnsServerCount = 0;
    }

    if (DnsServerCount)
    {
        gIntfInfo->DnsServerList = pDnsSrvInfo;
        gIntfInfo->DnsServerCount = DnsServerCount;
        WANCHK_LOG_INFO ("Updated DnsServerCount:%d->%d for interface %s\n",Old_DnsServerCount,
                                            DnsServerCount,gIntfInfo->IPInterface.InterfaceName);
    }
    pthread_mutex_unlock(&gIntfAccessMutex);
    return ANSC_STATUS_SUCCESS;
}

ULONG GetInstanceNo_FromName(char *InterfaceName)
{
    ULONG InstanceNumber;
    errno_t rc = -1;
    int ind = -1;
    PWANCNCTVTY_CHK_GLOBAL_INTF_INFO gIntfInfo = NULL;
    BOOL match_found = FALSE;
    pthread_mutex_lock(&gIntfAccessMutex);
    for (InstanceNumber=0;InstanceNumber< MAX_NO_OF_INTERFACES;InstanceNumber++)
    {
        gIntfInfo = &gInterface_List[InstanceNumber];
        rc = strcmp_s( InterfaceName,strlen(InterfaceName),gIntfInfo->IPInterface.InterfaceName, &ind);
        ERR_CHK(rc);
        if((!ind) && (rc == EOK))
        {
            WANCHK_LOG_DBG("Found Matching Interface Instance %ld\n",InstanceNumber);
            match_found = TRUE;
            break;
        }
    }
    if (!match_found)
    {
        WANCHK_LOG_WARN("No Matching global entry found for InterfaceName %s\n",InterfaceName);
        pthread_mutex_unlock(&gIntfAccessMutex);
        return -1;
    }
    pthread_mutex_unlock(&gIntfAccessMutex);
    return InstanceNumber;
}

ANSC_STATUS CosaDml_querynow_result_get(ULONG InstanceNumber,querynow_result_t *result)
{
    pthread_mutex_lock(&gIntfAccessMutex);
    PWANCNCTVTY_CHK_GLOBAL_INTF_INFO gIntfInfo = &gInterface_List[InstanceNumber-1];
    *result = gIntfInfo->IPInterface.QueryNowResult;
    pthread_mutex_unlock(&gIntfAccessMutex);
    return ANSC_STATUS_SUCCESS;
}

ANSC_STATUS CosaDml_monitor_result_get(ULONG InstanceNumber,monitor_result_t *result)
{
    pthread_mutex_lock(&gIntfAccessMutex);
    PWANCNCTVTY_CHK_GLOBAL_INTF_INFO gIntfInfo = &gInterface_List[InstanceNumber-1];
    *result = gIntfInfo->IPInterface.MonitorResult;
    pthread_mutex_unlock(&gIntfAccessMutex);
    return ANSC_STATUS_SUCCESS;
}


ANSC_STATUS CosaWanCnctvtyChk_Urllist_dump (VOID)
{
    ANSC_STATUS returnStatus                          = ANSC_STATUS_SUCCESS;
    PSINGLE_LINK_ENTRY              pSListEntry       = NULL;
    PCOSA_CONTEXT_LINK_OBJECT       pCxtLink          = NULL;
    PCOSA_DML_WANCNCTVTY_CHK_URL_INFO pUrlInfo        = NULL;

    WANCHK_LOG_INFO("%s Dumping URL List\n",__FUNCTION__);

    pthread_mutex_lock(&gUrlAccessMutex);
    pSListEntry           = AnscSListGetFirstEntry(&gpUrlList);
    while( pSListEntry != NULL)
    {
        pCxtLink          = ACCESS_COSA_CONTEXT_LINK_OBJECT(pSListEntry);
        pSListEntry       = AnscSListGetNextEntry(pSListEntry);
        pUrlInfo = (PCOSA_DML_WANCNCTVTY_CHK_URL_INFO)pCxtLink->hContext;
        if (pUrlInfo)
        {
            WANCHK_LOG_INFO("Found Instance Number %ld URL %s\n",
                                                    pCxtLink->InstanceNumber,
                                                    pUrlInfo->URL);
        }
    }
    pthread_mutex_unlock(&gUrlAccessMutex);
    return returnStatus;
}

ANSC_STATUS CosaWanCnctvtyChk_Interface_dump (ULONG InstanceNumber)
{
    pthread_mutex_lock(&gIntfAccessMutex);
    PWANCNCTVTY_CHK_GLOBAL_INTF_INFO gIntfInfo = &gInterface_List[InstanceNumber-1];

    WANCHK_LOG_INFO("*******************Interface Config for Instance %ld************************\n",InstanceNumber);
    WANCHK_LOG_INFO("Enable                     : %s\n",gIntfInfo->IPInterface.Enable ? "true" : "false");
    WANCHK_LOG_INFO("Alias                      : %s\n",gIntfInfo->IPInterface.Alias);
    WANCHK_LOG_INFO("InterfaceName              : %s\n",gIntfInfo->IPInterface.InterfaceName);
    WANCHK_LOG_INFO("PassiveMonitor             : %s\n",gIntfInfo->IPInterface.PassiveMonitor ? "true" : "false");
    WANCHK_LOG_INFO("PassiveMonitor Timeout     : %ld\n",gIntfInfo->IPInterface.PassiveMonitorTimeout);
    WANCHK_LOG_INFO("ActiveMonitor              : %s\n",gIntfInfo->IPInterface.ActiveMonitor ? "true" : "false");
    WANCHK_LOG_INFO("ActiveMonitorInterval      : %ld\n",gIntfInfo->IPInterface.ActiveMonitorInterval);
    WANCHK_LOG_INFO("MonitorResult              : %ld\n",gIntfInfo->IPInterface.MonitorResult);
    WANCHK_LOG_INFO("QueryNow                   : %s\n",gIntfInfo->IPInterface.QueryNow ? "true" : "false");
    WANCHK_LOG_INFO("QueryNowResult             : %ld\n",gIntfInfo->IPInterface.QueryNowResult);
    WANCHK_LOG_INFO("QueryTimeout               : %ld\n",gIntfInfo->IPInterface.QueryTimeout);
    WANCHK_LOG_INFO("QueryRetry                 : %ld\n",gIntfInfo->IPInterface.QueryRetry);
    WANCHK_LOG_INFO("RecordType                 : %s\n",gIntfInfo->IPInterface.RecordType);
    WANCHK_LOG_INFO("ServerType                 : %s\n",gIntfInfo->IPInterface.ServerType);
    WANCHK_LOG_INFO ("DnsServerCount             : %d\n",gIntfInfo->DnsServerCount);
    WANCHK_LOG_INFO ("QueryNowSubCount           : %d\n",gIntfInfo->IPInterface.QueryNowResult_SubsCount);
    WANCHK_LOG_INFO ("Conf updated               : %d\n",gIntfInfo->IPInterface.Configured);


    for (int i=0;i < gIntfInfo->DnsServerCount;i++)
    {
        if (gIntfInfo->DnsServerList[i].dns_type == DNS_SRV_IPV4)
        {
            WANCHK_LOG_INFO("DNS_ENTRY_%d                : %s\n",(i+1),gIntfInfo->DnsServerList[i].IPAddress.IPv4Address);
        }
        else
            WANCHK_LOG_INFO("DNS_ENTRY_%d                : %s\n",(i+1),gIntfInfo->DnsServerList[i].IPAddress.IPv6Address);
    }
    pthread_mutex_unlock(&gIntfAccessMutex);
    return ANSC_STATUS_SUCCESS;
}

