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

    module: cosa_wanconnectivity_apis.h

        For COSA Data Model Library Development

    -------------------------------------------------------------------

    description:

        This file defines the apis for diagnostic related
        objects to support Data Model Library.

    -------------------------------------------------------------------


    author:

        COSA XML TOOL CODE GENERATOR 1.0

    -------------------------------------------------------------------

    revision:

        01/11/2011    initial revision.

**************************************************************************/


#ifndef  _COSA_WANCHK_APIS_H
#define  _COSA_WANCHK_APIS_H

#include "cosa_apis.h"
#include "ccsp_base_api.h"
#include <rbus/rbus.h>
#include "sysevent/sysevent.h"

/* supporting only primary and secondary now*/
#define MAX_NO_OF_INTERFACES 2
#define MAX_INTF_NAME_SIZE 64
#define MAX_URL_SIZE 255
#define MAX_RECORD_TYPE_SIZE 64
#define MAX_SERVER_TYPE_SIZE 64
#define IPv6_STR_LEN 46
#define IPv4_STR_LEN 16
#define MACADDR_SZ 18
#define  ARRAY_SZ(x) (sizeof(x) / sizeof((x)[0]))

#define CURRENT_PRIMARY_INTF_DML "Device.X_RDK_WanManager.CurrentActiveInterface"
#define CURRENT_STANDBY_INTF_DML "Device.X_RDK_WanManager.CurrentStandbyInterface"
#define ACTIVE_GATEWAY_DML       "Device.X_RDK_GatewayManagement.ActiveGateway"
#define DNS_SRV_COUNT_DML        "Device.DNS.Client.ServerNumberOfEntries"
#define DNS_SRV_TABLE_DML        "Device.DNS.Client.Server."
#define DNS_SRV_ENTRY_DML        "Device.DNS.Client.Server.%d.DNSServer"
#define X_RDK_REMOTE_INVOKE      "Device.X_RDK_Remote.Invoke()"
#define WANCHK_TEST_URL_TABLE    "Device.Diagnostics.X_RDK_DNSInternet.TestURL."
#define WANCHK_TEST_URL_INSTANCE "Device.Diagnostics.X_RDK_DNSInternet.TestURL.%d.TestURL"
#define WANCHK_INTF_TABLE        "Device.Diagnostics.X_RDK_DNSInternet.WANInterface."

// Debug definitions. This will be enabled/disabled via debug.ini
#define WANCHK_LOG_ENTER(fmt...)  RDK_LOG(RDK_LOG_TRACE1,  "LOG.RDK.WANCNCTVTYCHK", fmt)
#define WANCHK_LOG_NOTICE(fmt...) RDK_LOG(RDK_LOG_NOTICE,  "LOG.RDK.WANCNCTVTYCHK", fmt)
#define WANCHK_LOG_RETURN(fmt...) RDK_LOG(RDK_LOG_TRACE1,  "LOG.RDK.WANCNCTVTYCHK", fmt)
#define WANCHK_LOG_ERROR(fmt...)  RDK_LOG(RDK_LOG_ERROR,   "LOG.RDK.WANCNCTVTYCHK", fmt)
#define WANCHK_LOG_WARN(fmt...)   RDK_LOG(RDK_LOG_WARN,    "LOG.RDK.WANCNCTVTYCHK", fmt)
#define WANCHK_LOG_INFO(fmt...)   RDK_LOG(RDK_LOG_INFO,    "LOG.RDK.WANCNCTVTYCHK", fmt)
#define WANCHK_LOG_DBG(fmt...)    RDK_LOG(RDK_LOG_DEBUG,   "LOG.RDK.WANCNCTVTYCHK", fmt)


#define DEFAULT_URL_COUNT 1
#define DEFAULT_URL "www.google.com"

#define BUFLEN_8 8
#define BUFLEN_64 64
#define BUFLEN_128 128
#define BUFLEN_256 256
#define BUFLEN_1024 1024
#define BUFLEN_2048 2048
#define BUFLEN_4096 4096

#define SE_SERVER_WELL_KNOWN_PORT 52367
#define SE_VERSION 1

#define DEF_INTF_ENABLE FALSE
#define DEF_PASSIVE_MONITOR_PRIMARY_ENABLE TRUE
#define DEF_PASSIVE_MONITOR_BACKUP_ENABLE FALSE
#define DEF_PASSIVE_MONITOR_TIMEOUT 10000
#define DEF_ACTIVE_MONITOR_PRIMARY_ENABLE TRUE
#define DEF_ACTIVE_MONITOR_BACKUP_ENABLE FALSE
#define DEF_ACTIVE_MONITOR_INTERVAL 5000
#define DEF_QUERY_TIMEOUT 200
#define DEF_QUERY_RETRY 2
#define DEF_QUERY_RECORDTYPE "A+AAAA"
#define DEF_QUERY_SERVERTYPE "IPv4+IPv6"

typedef enum _cfg_change_bitmask {
    /* config bit masks*/
    INTF_CFG_ENABLE = (1 << 0),
    INTF_CFG_PASSIVE_ENABLE = (1 << 1),
    INTF_CFG_PASSIVE_TIMEOUT = (1 << 2),
    INTF_CFG_ACTIVE_ENABLE = (1 << 3),
    INTF_CFG_ACTIVE_INTERVAL = (1 << 4),
    INTF_CFG_QUERYNOW_ENABLE = (1 << 5),
    INTF_CFG_QUERY_TIMEOUT = (1 << 6),
    INTF_CFG_QUERY_RETRY = (1 << 7),
    INTF_CFG_RECORDTYPE = (1 << 8),
    INTF_CFG_SERVERTYPE = (1 << 9),
    INTF_CFG_ALL = 0xFFFF,
} cfg_change_bitmask_t;


typedef enum _dns_entry_type {
        DNS_SRV_NONE  = 0,
        DNS_SRV_IPV4  = 1,
        DNS_SRV_IPV6  = 2,
} dns_entrytype_t;

typedef enum _monitor_result {
        MONITOR_RESULT_UNKNOWN  = 0,
        MONITOR_RESULT_CONNECTED  = 1,
        MONITOR_RESULT_DISCONNECTED  = 2,
} monitor_result_t;

typedef enum _querynow_result {
        QUERYNOW_RESULT_UNKNOWN  = 0,
        QUERYNOW_RESULT_CONNECTED  = 1,
        QUERYNOW_RESULT_DISCONNECTED  = 2,
        QUERYNOW_RESULT_BUSY=3,
} querynow_result_t;

typedef enum _IDM_MSG_OPERATION
{
    IDM_SET = 1,
    IDM_GET,
    IDM_SUBS,
    IDM_REQUEST,

}IDM_MSG_OPERATION;

typedef enum _dml_type_t {
        FEATURE_DML  = 1,
        FEATURE_ENABLED_DML  = 2,
} dml_type_t;

typedef struct _idm_invoke_method_Params
{
    IDM_MSG_OPERATION operation;
    char Mac_dest[18];
    char param_name[128];
    char param_value[2048];
    uint timeout;
    enum dataType_e type;
    rbusMethodAsyncHandle_t asyncHandle;
}idm_invoke_method_Params_t;

typedef enum _service_type {
        PASSIVE_MONITOR_THREAD  = 0,
        ACTIVE_MONITOR_THREAD   = 1,
        QUERYNOW_THREAD =2,
        PASSIVE_ACTIVE_MONITOR_THREADS  = 3,
        ALL_THREADS = 4,
} service_type_t;

typedef  struct
_COSA_DML_WANCNCTVTY_CHK_DNSSRV_TABLE
{
    dns_entrytype_t dns_type;
    union {
        UCHAR                           IPv4Address[IPv4_STR_LEN];
        UCHAR                           IPv6Address[IPv6_STR_LEN];
    } IPAddress;

}
COSA_DML_WANCNCTVTY_CHK_DNSSRV_INFO,  *PCOSA_DML_WANCNCTVTY_CHK_DNSSRV_INFO;

typedef  struct
_COSA_DML_WANCNCTVTY_CHK_INTF_TABLE
{
    BOOL                                          Enable;
    UCHAR                                         Alias[MAX_INTF_NAME_SIZE];/* Wan interface primary or backup */
    UCHAR                                         InterfaceName[MAX_INTF_NAME_SIZE];/* Wan interface name */
    BOOL                                          PassiveMonitor;
    ULONG                                         PassiveMonitorTimeout;
    BOOL                                          ActiveMonitor;
    ULONG                                         ActiveMonitorInterval;
    ULONG                                         MonitorResult;
    BOOL                                          QueryNow;
    ULONG                                         QueryNowResult;
    ULONG                                         QueryTimeout;
    ULONG                                         QueryRetry;
    UCHAR                                         RecordType[MAX_RECORD_TYPE_SIZE];
    UCHAR                                         ServerType[MAX_SERVER_TYPE_SIZE];
    ULONG                                         InstanceNumber;
    uint32_t                                      Cfg_bitmask;
    BOOL                                          Configured;
    uint32_t                                      MonitorResult_SubsCount;
    uint32_t                                      QueryNowResult_SubsCount;
}
COSA_DML_WANCNCTVTY_CHK_INTF_INFO,  *PCOSA_DML_WANCNCTVTY_CHK_INTF_INFO;

typedef  struct
_COSA_DML_WANCNCTVTY_CHK_URL_TABLE
{
    UCHAR                           URL[MAX_URL_SIZE];
    ULONG                           InstanceNumber;
}
COSA_DML_WANCNCTVTY_CHK_URL_INFO,  *PCOSA_DML_WANCNCTVTY_CHK_URL_INFO;

#define  COSA_DATAMODEL_WANCNCTVTY_CHK_CLASS_CONTENT                                                     \
    BOOL                          Enable;                                                                \
    BOOL                          Active;                                                                \
    SLIST_HEADER                  pUrlList;                                                              \
    ULONG                         ulUrlNextInsNum;                                                       \
    SLIST_HEADER                  pIntfTableList;                                                        \
    ULONG                         ulNextInterfaceInsNum;                                                 \
    /* End of base object class content*/                                                                \

typedef  struct
_COSA_DATAMODEL_WANCNCTVTY_CHK
{
    COSA_DATAMODEL_WANCNCTVTY_CHK_CLASS_CONTENT
}
COSA_DATAMODEL_WANCNCTVTY_CHK,  *PCOSA_DATAMODEL_WANCNCTVTY_CHK;

typedef enum _dns_record_type {
        IPV4_ONLY  = 1,
        IPV6_ONLY  = 2,
        BOTH_IPV4_IPV6    = 3,
        EITHER_IPV4_IPV6  = 4,
        RECORDTYPE_INVALID
} recordtype_t;

typedef enum _dns_server_type {
        SRVR_IPV4_ONLY  = 1,
        SRVR_IPV6_ONLY  = 2,
        SRVR_BOTH_IPV4_IPV6    = 3,
        SRVR_EITHER_IPV4_IPV6  = 4,
        SRVR_TYPE_INVALID
} servertype_t;

typedef  struct
_COSA_DML_WANCNCTVTY_CHK_QUERYNOW_CTXT
{
    ULONG     InstanceNumber;
    BOOL      IsPrimary;
    ULONG     QueryTimeout;
    ULONG     QueryRetry;
    unsigned int DnsServerCount;
    unsigned int url_count;
    recordtype_t RecordType;
    servertype_t ServerType;
    UCHAR     InterfaceName[MAX_INTF_NAME_SIZE];
    UCHAR     Alias[MAX_INTF_NAME_SIZE];
    char      **url_list;
    PCOSA_DML_WANCNCTVTY_CHK_DNSSRV_INFO  DnsServerList;
    BOOL      doInfoLogOnce;
}
COSA_DML_WANCNCTVTY_CHK_QUERYNOW_CTXT_INFO,  *PCOSA_DML_WANCNCTVTY_CHK_QUERYNOW_CTXT_INFO;

typedef  struct
_COSA_DML_WANCNCTVTY_CHK_ACTIVEQUERY_CTXT
{
    BOOL      PassiveMonitor;
    ULONG     ActiveMonitorInterval;
    PCOSA_DML_WANCNCTVTY_CHK_QUERYNOW_CTXT_INFO pQueryCtxt;
}
COSA_DML_WANCNCTVTY_CHK_ACTIVEQUERY_CTXT_INFO,  *PCOSA_DML_WANCNCTVTY_CHK_ACTIVEQUERY_CTXT_INFO;

typedef enum
_COSA_WAN_CNCTVTY_CHK_EVENTS
{
      INTF_PRIMARY = 1,
      INTF_SECONDARY

} COSA_WAN_CNCTVTY_CHK_EVENTS;


typedef enum _wan_intf_index {
        PRIMARY_INTF_INDEX  = 1,
        SECONDARY_INTF_INDEX = 2,
} wan_intf_index_t;

typedef  struct
_WANCNCTVTY_CHK_GLOBAL_INTF_INFO
{
    COSA_DML_WANCNCTVTY_CHK_INTF_INFO             IPInterface;
    PCOSA_DML_WANCNCTVTY_CHK_DNSSRV_INFO          DnsServerList;
    unsigned int                                  DnsServerCount;
    BOOL                                          Dns_configured;
    pthread_t                                     wancnctvychkpassivethread_tid;
    BOOL                                          PassiveMonitor_Running;
    pthread_t                                     wancnctvychkactivethread_tid;
    BOOL                                          ActiveMonitor_Running;
    pthread_t                                     wancnctvychkquerynowthread_tid;
    BOOL                                          QueryNow_Running;
}
WANCNCTVTY_CHK_GLOBAL_INTF_INFO,  *PWANCNCTVTY_CHK_GLOBAL_INTF_INFO;

/*************************************
    The actual function declaration
**************************************/

ANSC_STATUS CosaWanCnctvtyChk_Init (VOID);
ANSC_STATUS CosaWanCnctvtyChk_Init_URLTable (VOID);
ANSC_STATUS CosaWanCnctvtyChk_Init_IntfTable (VOID);
ANSC_STATUS CosaWanCnctvtyChk_Init_Intf (wan_intf_index_t index);
ANSC_STATUS CosaWanCnctvtyChk_IfGetEntry(PCOSA_DML_WANCNCTVTY_CHK_INTF_INFO pIPInterface);
ANSC_STATUS CosaWanCnctvtyChk_DNS_GetEntry(char *InterfaceName);
BOOL CosaWanCnctvtyChk_GetActive_Status(void);
BOOL CosaWanCnctvtyChk_GetPrimary_IntfName(char *);
BOOL CosaWanCnctvtyChk_GetSecondary_IntfName(char *);
ANSC_STATUS CosaWanCnctvtyChk_SubscribeRbus(void);
ANSC_STATUS CosaWanCnctvtyChk_UnSubscribeRbus(void);
ANSC_STATUS CosaWanCnctvtyChk_SubscribeActiveGW(void);
ANSC_STATUS CosaWanCnctvtyChk_UnSubscribeActiveGW(void);
void handle_dns_srvrcnt_event (char *InterfaceName,unsigned int new_dns_server_count);
void handle_dns_srv_addrchange_event (char *InterfaceName,int dns_srv_index,
                                                                    const char *newValue);
void handle_actv_status_event (BOOL new_status);
ANSC_STATUS CosaWanCnctvtyChk_DeInit_IntfTable(VOID);
void handle_intf_change_event(COSA_WAN_CNCTVTY_CHK_EVENTS event,const char *new_value);
ANSC_STATUS CosaWanCnctvtyChk_Remove_Intf (wan_intf_index_t IntfIndex);
ANSC_STATUS CosaWanCnctvtyChk_Remove_Urllist (VOID);
ANSC_STATUS CosaWanCnctvtyChk_DNS_Unsubscribe(unsigned int DnsServerCount);
ANSC_STATUS CosaDmlStoreIntfCfg(PCOSA_DML_WANCNCTVTY_CHK_INTF_INFO pIPInterface);
ANSC_STATUS CosaDmlGetIntfCfg(PCOSA_DML_WANCNCTVTY_CHK_INTF_INFO pIPInterface,BOOL use_default);
ANSC_STATUS CosaDml_glblintfdb_updateentry(PCOSA_DML_WANCNCTVTY_CHK_INTF_INFO pIPInterface);
ANSC_STATUS CosaDml_glblintfdb_update_dnsentry(char *InterfaceName,unsigned int DnsServerCount,
                                                PCOSA_DML_WANCNCTVTY_CHK_DNSSRV_INFO pDnsSrvInfo);
ANSC_STATUS CosaDml_glblintfdb_delentry(ULONG InstanceNumber);
ANSC_STATUS CosaDml_querynow_result_get(ULONG InstanceNumber,querynow_result_t *result);
ANSC_STATUS CosaDml_monitor_result_get(ULONG InstanceNumber,monitor_result_t *result);
ANSC_STATUS CosaWanCnctvtyChkRemove ();
ANSC_STATUS CosaWanCnctvtyChk_Urllist_dump (VOID);
ANSC_STATUS CosaWanCnctvtyChk_Interface_dump (ULONG InstanceNumber);

ANSC_STATUS WanCnctvtyChk_GetPrimaryDNS_Entry(PCOSA_DML_WANCNCTVTY_CHK_DNSSRV_INFO pDnsSrvInfo);
ANSC_STATUS WanCnctvtyChk_GetRemote_MacAddress(char *mac_addr);
ANSC_STATUS WanCnctvtyChk_IDM_Invoke(idm_invoke_method_Params_t *IDM_request,BOOL Invoke_Async,
                                                                        rbusObject_t *outParams);
ANSC_STATUS WanCnctvtyChk_GetRemoteParameterValue( char *mac_addr,
                                                        const char *pParamName, char *pReturnVal );
ANSC_STATUS WanCnctvtyChk_GetParameterValue(  const char *pParamName, char
*pReturnVal );
ANSC_STATUS WanCnctvtyChk_GetSecondaryDNS_Entry(char *mac_addr,
                                                PCOSA_DML_WANCNCTVTY_CHK_DNSSRV_INFO pDnsSrvInfo,
                                                unsigned int DnsServerCount);
void WanCnctvtyChk_IDM_AsyncMethodHandler(rbusHandle_t handle,char const* methodName,
                                                            rbusError_t error,rbusObject_t params);
rbusError_t WANCNCTVTYCHK_PublishEvent(char* event_name , uint32_t eventNewData, uint32_t eventOldData);
ANSC_STATUS CosaWanCnctvtyChk_GetDNS_PeerInfo(char *InterfaceName);
ANSC_STATUS CosaWanCnctvtyChk_GetDNS_HostInfo(char *InterfaceName);
void *WanCnctvtyChk_SysEventHandlerThrd(void *data);
ULONG GetInstanceNo_FromName(char *InterfaceName);
void CosaWanCnctvtyChk_StartSysevent_listener();
void CosaWanCnctvtyChk_StopSysevent_listener();
#endif

