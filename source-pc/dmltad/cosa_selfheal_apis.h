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

#include "cosa_apis.h"
#include "dslh_definitions_tr143.h"

#define   COSA_DML_SELFHEAL_PINGSERVER_ACCESS_INTERVAL   60 /* seconds*/

#define  COSA_CONTEXT_SELFHEAL_LINK_CLASS_CONTENT                                  \
        COSA_CONTEXT_LINK_CLASS_CONTENT                                            \
        BOOL                            bFound;                                    \


/***********************************
    Actual definition declaration
************************************/
#define  COSA_IREP_FOLDER_NAME_SELFHEAL                       "SelfHeal"
#define  COSA_IREP_FOLDER_NAME_PORTMAPPING               "PORTMAPPING"
#define  COSA_IREP_FOLDER_NAME_PORTTRIGGER               "PORTTRIGGER"
#define  COSA_DML_RR_NAME_NATNextInsNumber               "NextInstanceNumber"
#define  COSA_DML_RR_NAME_NATAlias                       "Alias"
#define  COSA_DML_RR_NAME_NATbNew                        "bNew"
typedef enum _PingServerType
{
	PingServerType_IPv4 = 0,
	PingServerType_IPv6
}PingServerType;
typedef  struct
_COSA_CONTEXT_SELFHEAL_LINK_OBJECT
{
    COSA_CONTEXT_SELFHEAL_LINK_CLASS_CONTENT
}
COSA_CONTEXT_SELFHEAL_LINK_OBJECT,  *PCOSA_CONTEXT_SELFHEAL_LINK_OBJECT;

typedef  struct
_COSA_DML_SELFHEAL_IPv4_SERVER
{
    ULONG                           InstanceNumber;
    UCHAR                           Ipv4PingServerURI[256];  /* IPv4 or IPv4 string address */
}
COSA_DML_SELFHEAL_IPv4_SERVER_TABLE,  *PCOSA_DML_SELFHEAL_IPv4_SERVER_TABLE;

typedef  struct
_COSA_DML_SELFHEAL_IPv6_SERVER
{
    ULONG                           InstanceNumber;
    UCHAR                           Ipv6PingServerURI[256];  /* IPv4 or IPv4 string address */
}
COSA_DML_SELFHEAL_IPv6_SERVER_TABLE,  *PCOSA_DML_SELFHEAL_IPv6_SERVER_TABLE;



typedef  struct
_PCOSA_DML_CONNECTIVITY_TEST
{
    BOOL                            CorrectiveAction;  
    ULONG                           PingInterval;  
    ULONG                           PingCount;
    ULONG                           WaitTime;
    ULONG                           MinPingServer;
    ULONG                      	    IPv4EntryCount;                                    
    PCOSA_DML_SELFHEAL_IPv4_SERVER_TABLE    pIPv4Table;    
    ULONG                      	    IPv6EntryCount;                                    
    PCOSA_DML_SELFHEAL_IPv6_SERVER_TABLE    pIPv6Table;
    int                             RouterRebootInterval; 
}
COSA_DML_CONNECTIVITY_TEST,  *PCOSA_DML_CONNECTIVITY_TEST;

typedef struct
_COSA_DML_RESOUCE_MONITOR
{
    ULONG                  MonIntervalTime;
    ULONG                  AvgCpuThreshold;
    ULONG                  AvgMemThreshold;
}
COSA_DML_RESOUCE_MONITOR, *PCOSA_DML_RESOUCE_MONITOR;

#define  COSA_DATAMODEL_SELFHEAL_CLASS_CONTENT                                                  \
    /* duplication of the base object class content */                                      \
    COSA_BASE_CONTENT                                                                       \
    BOOL                       Enable;                                    \
    ULONG                       MaxRebootCnt;                                    \
    ULONG                       MaxResetCnt;                                    \
    ULONG                       PreviousVisitTime;                                    \
	ULONG                       MaxInstanceNumber;                                    \
	ULONG                       ulIPv4NextInstanceNumber;                                    \
	ULONG                       ulIPv6NextInstanceNumber;                                    \
    SLIST_HEADER                IPV4PingServerList;                                        \
    SLIST_HEADER                IPV6PingServerList;                                        \
    PCOSA_DML_CONNECTIVITY_TEST    pConnTest;                                        \
    PCOSA_DML_RESOUCE_MONITOR   pResMonitor;
	ANSC_HANDLE                     hIrepFolderSelfHeal;                                         \
    ANSC_HANDLE                     hIrepFolderSelfHealCoTest;                                       \
    /* end of Diagnostic object class content */                                                    \


typedef  struct
_COSA_DATAMODEL_SELFHEAL
{
    COSA_DATAMODEL_SELFHEAL_CLASS_CONTENT
}
COSA_DATAMODEL_SELFHEAL,  *PCOSA_DATAMODEL_SELFHEAL;

#define  ACCESS_COSA_CONTEXT_SELFHEAL_LINK_OBJECT(p)              \
         ACCESS_CONTAINER(p, COSA_CONTEXT_SELFHEAL_LINK_OBJECT, Linkage)
/**********************************
    Standard function declaration
***********************************/
ANSC_HANDLE
CosaSelfHealCreate
    (
        VOID
    );

ANSC_STATUS
CosaSelfHealInitialize
    (
        ANSC_HANDLE                 hThisObject
    );

ANSC_STATUS
CosaSelfHealRemove
    (
        ANSC_HANDLE                 hThisObject
    );
void SavePingServerURI(PingServerType type, char *URL, int InstNum);
ANSC_STATUS RemovePingServerURI(PingServerType type, int InstNum);

