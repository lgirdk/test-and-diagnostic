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
#include "cosa_wanconnectivity_operations.h"
#include <rbus/rbus.h>
#include "syscfg.h"
#include <ifaddrs.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <resolv.h>
#include "poll.h"
#include <netinet/ip.h>
#include <netinet/in.h>
#include <netinet/udp.h>
#include <netinet/ip6.h>
#include <netinet/ether.h>
#include <libnetfilter_queue/libnetfilter_queue.h>
#include <linux/netfilter.h>
#include "secure_wrapper.h"
#include <linux/if_ether.h>
#include <linux/if_packet.h>
#include <ctype.h>


/*Macro's*/
#define WANCHK_NFQUEUE_V4 31
#define WANCHK_NFQUEUE_V6 32

/*Extern variables*/
extern WANCNCTVTY_CHK_GLOBAL_INTF_INFO gInterface_List[MAX_NO_OF_INTERFACES];
extern BOOL g_wanconnectivity_check_active;
extern BOOL g_wanconnectivity_check_enable;
extern pthread_mutex_t gIntfAccessMutex;
extern pthread_mutex_t gUrlAccessMutex;
extern SLIST_HEADER    gpUrlList;
extern int sysevent_fd_wanchk;
extern token_t sysevent_token_wanchk;


/* Function declarations*/
static int get_ipv4_addr(const char *ifName, char *ipv4Addr);
static int get_ipv6_addr(const char *ifName, char *ipv6Addr);
char** get_url_list(int *total_url_entries);
static int dns_response_v4_callback(struct nfq_q_handle *queueHandle, struct nfgenmsg *nfmsg,
                                 struct nfq_data *pkt, void *data);
static int dns_response_v6_callback(struct nfq_q_handle *queueHandle, struct nfgenmsg *nfmsg,
                                 struct nfq_data *pkt, void *data);
static void cleanup_querynow(void *arg);
static void cleanup_querynow_fd(void *arg);
static void cleanup_activemonitor_ev(void *arg);
static void cleanup_activequery(void *arg);
static void _get_shell_output (FILE *fp, char *buf, size_t len);
void WanCnctvtyChk_CreateEthernetHeader (struct ethhdr *ethernet_header,char *src_mac, char *dst_mac, int protocol);
void WanCnctvtyChk_CreatePseudoHeaderAndComputeUdpChecksum (int family, struct udphdr *udp_header, void *ip_header,
                                                        unsigned char *data, unsigned int dataSize);
void WanCnctvtyChk_CreateIPHeader (int family, char *srcIp, char *dstIp, unsigned int dataSize,void *);
void WanCnctvtyChk_CreateUdpHeader(int family, unsigned short sport, unsigned short dport,
                                                                      unsigned int dataSize,struct udphdr *);
unsigned short WanCnctvtyChk_udp_checksum (unsigned short *buffer, int byte_count);
static int get_mac_addr(char* ifName, char* mac);
static int64_t clock_mono_ms();
static int socket_dns_filter_attach(int fd,struct sock_filter *filter, int filter_len);

uint8_t *WanCnctvtyChk_get_transport_header(struct ip6_hdr *ip6h,uint8_t target,uint8_t *payload_tail);
bool validatemac(char *mac);

/* Logic

    Start threads
    a) QueryNow 
        update QueryNowResult to 3, start QueryNow thread, revert back Query Now to False.
    b) PassiveMonitor Threads
        Start Passive monitor threads.
    c) ActiveMonitor Threads
        Start Active Monitor threads.
*/

ANSC_STATUS wancnctvty_chk_start_threads(ULONG InstanceNumber,service_type_t type)
{
    pthread_mutex_lock(&gIntfAccessMutex);
    PWANCNCTVTY_CHK_GLOBAL_INTF_INFO gIntfInfo = &gInterface_List[InstanceNumber-1];
    if (!gIntfInfo)
    {
        WANCHK_LOG_ERROR("Unable to start threads,global data is NULL for InstanceNumber %ld\n",
                                                                                InstanceNumber);
        pthread_mutex_unlock(&gIntfAccessMutex);
        return ANSC_STATUS_FAILURE;
    }

    if ((g_wanconnectivity_check_enable == TRUE) && (gIntfInfo->IPInterface.Enable == TRUE))
    {
        if (g_wanconnectivity_check_active == TRUE)
        {
            if ( ((type== QUERYNOW_THREAD) || (type == ALL_THREADS)) &&
                                                    (gIntfInfo->IPInterface.QueryNow == TRUE))
            {
                WANCHK_LOG_INFO("%s Start QueryNow for interface :%s\n",__FUNCTION__,
                                                            gIntfInfo->IPInterface.InterfaceName);
                wancnctvty_chk_start_querynow(gIntfInfo);
                /*Reset the switch back to false*/
                gIntfInfo->IPInterface.QueryNow = FALSE;
            }

            if ( ((type== PASSIVE_ACTIVE_MONITOR_THREADS)  || (type== PASSIVE_MONITOR_THREAD) ||
                 (type == ALL_THREADS)) && (gIntfInfo->IPInterface.PassiveMonitor == TRUE) && 
                                                    (gIntfInfo->PassiveMonitor_Running == FALSE))
            {
                WANCHK_LOG_INFO("%s Start PassiveMonitor for interface :%s\n",__FUNCTION__,
                                                        gIntfInfo->IPInterface.InterfaceName);
                wancnctvty_chk_start_passive(gIntfInfo);
            }
            else if ( ((type== PASSIVE_ACTIVE_MONITOR_THREADS)  || (type== ACTIVE_MONITOR_THREAD) ||
                      (type == ALL_THREADS)) && (gIntfInfo->IPInterface.ActiveMonitor == TRUE) &&
                                                    (gIntfInfo->ActiveMonitor_Running == FALSE))
            {
               /* if passive monitor is enabled, Monitor Result will be set based on the
               passive monitor, if passive monitor is disabled and active monitor is enabled
               lets start here*/
                WANCHK_LOG_INFO("%s Start ActiveMonitor for interface :%s\n",__FUNCTION__,
                                                            gIntfInfo->IPInterface.InterfaceName);
                wancnctvty_chk_start_active(gIntfInfo);
            }
        }
    }
    pthread_mutex_unlock(&gIntfAccessMutex);
    return ANSC_STATUS_SUCCESS;
}

ANSC_STATUS wancnctvty_chk_stop_threads(ULONG InstanceNumber,service_type_t type)
{
    pthread_mutex_lock(&gIntfAccessMutex);
    PWANCNCTVTY_CHK_GLOBAL_INTF_INFO gIntfInfo = &gInterface_List[InstanceNumber-1];
    WANCHK_LOG_INFO ("%s: Stopping threads if any for interface %s\n",__FUNCTION__,
                                                            gIntfInfo->IPInterface.InterfaceName);
    if (!gIntfInfo)
    {
        WANCHK_LOG_ERROR("Unable to stop threads,global data is NULL for InstanceNumber %ld\n",
                                                                                InstanceNumber);
        pthread_mutex_unlock(&gIntfAccessMutex);
        return ANSC_STATUS_FAILURE;
    }

    if ( ( (type== QUERYNOW_THREAD)  || (type == ALL_THREADS)) &&
                                                    gIntfInfo->QueryNow_Running == TRUE)
    {
        if (gIntfInfo->wancnctvychkquerynowthread_tid)
        {
            WANCHK_LOG_INFO("Stop Querynow thread id:%lu\n",
                                            gIntfInfo->wancnctvychkquerynowthread_tid);
            pthread_cancel(gIntfInfo->wancnctvychkquerynowthread_tid);
            gIntfInfo->wancnctvychkquerynowthread_tid = 0;
            gIntfInfo->QueryNow_Running = FALSE;
        }
    }

    if ( ((type== PASSIVE_ACTIVE_MONITOR_THREADS)  || (type== ACTIVE_MONITOR_THREAD) ||
                      (type == ALL_THREADS)) && gIntfInfo->ActiveMonitor_Running == TRUE)
    {
        if (gIntfInfo->wancnctvychkactivethread_tid)
        {
            WANCHK_LOG_INFO("Stop Active Monitor thread id:%lu\n",
                                            gIntfInfo->wancnctvychkactivethread_tid);
            pthread_cancel(gIntfInfo->wancnctvychkactivethread_tid);
            gIntfInfo->wancnctvychkactivethread_tid = 0;
            gIntfInfo->ActiveMonitor_Running = FALSE;
        }
    }

    if ( ((type== PASSIVE_ACTIVE_MONITOR_THREADS)  || (type== PASSIVE_MONITOR_THREAD) ||
                 (type == ALL_THREADS)) && gIntfInfo->PassiveMonitor_Running == TRUE)
    {
        if (gIntfInfo->wancnctvychkpassivethread_tid)
        {
            WANCHK_LOG_INFO("Stop PassiveMonitor Monitor thread id:%lu\n",
                                            gIntfInfo->wancnctvychkpassivethread_tid);
            pthread_cancel(gIntfInfo->wancnctvychkpassivethread_tid);
            gIntfInfo->wancnctvychkpassivethread_tid = 0;
            gIntfInfo->PassiveMonitor_Running = FALSE;
        }

    }
    pthread_mutex_unlock(&gIntfAccessMutex);
    return ANSC_STATUS_SUCCESS;
}

ANSC_STATUS wancnctvty_chk_start_querynow(PWANCNCTVTY_CHK_GLOBAL_INTF_INFO gIntfInfo)
{
    /* we are coming here from the context of commit*/
    /* copy pinterface to a local (not really,taking from heap) instance and pass it to 
    thread, make sure to free on completion*/
    PCOSA_DML_WANCNCTVTY_CHK_INTF_INFO pIPInterface = &gIntfInfo->IPInterface;
    if (!pIPInterface)
    {
        WANCHK_LOG_ERROR("pIPInterface is NULL,Abort query\n");
        return ANSC_STATUS_FAILURE;
    }
    WANCHK_LOG_INFO("QueryNow Interface %s Config\n",gIntfInfo->IPInterface.InterfaceName);
    CosaWanCnctvtyChk_Interface_dump(pIPInterface->InstanceNumber);
    /* Setting the state to busy*/
    ULONG previous_result = gIntfInfo->IPInterface.QueryNowResult;
    gIntfInfo->IPInterface.QueryNowResult = QUERYNOW_RESULT_BUSY;
    WANCHK_LOG_INFO("Updated QueryNowResult for Interface %s:%ld->%ld\n",
                                        gIntfInfo->IPInterface.InterfaceName,previous_result,
                                        gIntfInfo->IPInterface.QueryNowResult);
    PCOSA_DML_WANCNCTVTY_CHK_QUERYNOW_CTXT_INFO pQuerynowCtxt = NULL;
    pQuerynowCtxt = (PCOSA_DML_WANCNCTVTY_CHK_QUERYNOW_CTXT_INFO)
                        AnscAllocateMemory(sizeof(COSA_DML_WANCNCTVTY_CHK_QUERYNOW_CTXT_INFO));
    if (!pQuerynowCtxt)
    {
        WANCHK_LOG_ERROR("Unable to allocate memory for query thread, Abort\n");
        return ANSC_STATUS_FAILURE;
    }

    wancnctvty_chk_copy_ctxt_data(gIntfInfo,pQuerynowCtxt);

    gIntfInfo->wancnctvychkquerynowthread_tid = 0;
    gIntfInfo->QueryNow_Running = FALSE;
    pthread_create( &gIntfInfo->wancnctvychkquerynowthread_tid, NULL,
                                            wancnctvty_chk_querynow_thread, (void *)pQuerynowCtxt);
    gIntfInfo->QueryNow_Running = TRUE;
    return ANSC_STATUS_SUCCESS;
}

/* we are still in context of inf lock*/
ANSC_STATUS wancnctvty_chk_start_passive(PWANCNCTVTY_CHK_GLOBAL_INTF_INFO gIntfInfo)
{
      /* copy pIPinterface struct to local struct
         since there are chances for this thread to cancel at any time and also avoid long needed locks*/
    pthread_create( &gIntfInfo->wancnctvychkpassivethread_tid, NULL,
                                        wancnctvty_chk_passive_thread, (void *)
                                                                    &gIntfInfo->IPInterface );
    gIntfInfo->PassiveMonitor_Running = TRUE;
    return ANSC_STATUS_SUCCESS;
}

ANSC_STATUS wancnctvty_chk_start_active(PWANCNCTVTY_CHK_GLOBAL_INTF_INFO gIntfInfo)
{
    PCOSA_DML_WANCNCTVTY_CHK_INTF_INFO pIPInterface = &gIntfInfo->IPInterface;
    if (!pIPInterface)
    {
        WANCHK_LOG_ERROR("pIPInterface is NULL,Abort active thread\n");
        return ANSC_STATUS_FAILURE;
    }

    PCOSA_DML_WANCNCTVTY_CHK_DNSSRV_INFO pDnsSrvInfo = gIntfInfo->DnsServerList;
    if (!pDnsSrvInfo)
    {
        WANCHK_LOG_ERROR("pDnsSrvInfo is NULL,Abort active thread\n");
        return ANSC_STATUS_FAILURE;
    }

    PCOSA_DML_WANCNCTVTY_CHK_ACTIVEQUERY_CTXT_INFO pActvQueryCtxt = NULL;
    pActvQueryCtxt = (PCOSA_DML_WANCNCTVTY_CHK_ACTIVEQUERY_CTXT_INFO)
                        AnscAllocateMemory(sizeof(COSA_DML_WANCNCTVTY_CHK_ACTIVEQUERY_CTXT_INFO));
    if (!pActvQueryCtxt)
    {
        WANCHK_LOG_ERROR("Unable to allocate memory for Active query thread, Abort\n");
        return -1;
    }
    pActvQueryCtxt->ActiveMonitorInterval = pIPInterface->ActiveMonitorInterval;
    pActvQueryCtxt->pQueryCtxt = (PCOSA_DML_WANCNCTVTY_CHK_QUERYNOW_CTXT_INFO)
                        AnscAllocateMemory(sizeof(COSA_DML_WANCNCTVTY_CHK_QUERYNOW_CTXT_INFO));

    wancnctvty_chk_copy_ctxt_data(gIntfInfo,pActvQueryCtxt->pQueryCtxt);
    WANCHK_LOG_INFO("ActiveMonitor Interface %s Config\n",gIntfInfo->IPInterface.InterfaceName);
    pthread_create( &gIntfInfo->wancnctvychkactivethread_tid, NULL, 
                                            wancnctvty_chk_active_thread, (void *)pActvQueryCtxt);
    gIntfInfo->ActiveMonitor_Running = TRUE;
    return ANSC_STATUS_SUCCESS;
}

ANSC_STATUS wancnctvty_chk_copy_ctxt_data (PWANCNCTVTY_CHK_GLOBAL_INTF_INFO gIntfInfo,
                                        PCOSA_DML_WANCNCTVTY_CHK_QUERYNOW_CTXT_INFO pQuerynowCtxt)
{
    /* Copy DNS info from inreface context*/
    /* Copy pIPInterface which we are coming in current context*/
    errno_t rc = -1;

    PCOSA_DML_WANCNCTVTY_CHK_INTF_INFO pIPInterface = &gIntfInfo->IPInterface;
    pQuerynowCtxt->InstanceNumber = pIPInterface->InstanceNumber;
    pQuerynowCtxt->QueryTimeout = pIPInterface->QueryTimeout;
    pQuerynowCtxt->QueryRetry   = pIPInterface->QueryRetry;
    rc = strcpy_s(pQuerynowCtxt->InterfaceName, MAX_INTF_NAME_SIZE , pIPInterface->InterfaceName);
    ERR_CHK(rc);
    rc = strcpy_s(pQuerynowCtxt->Alias, MAX_INTF_NAME_SIZE , pIPInterface->Alias);
    ERR_CHK(rc);
    rc = strcpy_s(pQuerynowCtxt->ServerType, MAX_SERVER_TYPE_SIZE , pIPInterface->ServerType);
    ERR_CHK(rc);
    rc = strcpy_s(pQuerynowCtxt->RecordType, MAX_RECORD_TYPE_SIZE , pIPInterface->RecordType);
    ERR_CHK(rc);
    pQuerynowCtxt->DnsServerCount = gIntfInfo->DnsServerCount;
    PCOSA_DML_WANCNCTVTY_CHK_DNSSRV_INFO pDnsSrvInfo = gIntfInfo->DnsServerList;
    pQuerynowCtxt->DnsServerList = (PCOSA_DML_WANCNCTVTY_CHK_DNSSRV_INFO) AnscAllocateMemory(
                    pQuerynowCtxt->DnsServerCount * sizeof(COSA_DML_WANCNCTVTY_CHK_DNSSRV_INFO));

    memset(pQuerynowCtxt->DnsServerList,0,
                    pQuerynowCtxt->DnsServerCount * sizeof(COSA_DML_WANCNCTVTY_CHK_DNSSRV_INFO));

    memcpy(pQuerynowCtxt->DnsServerList,pDnsSrvInfo,
                    pQuerynowCtxt->DnsServerCount * sizeof(COSA_DML_WANCNCTVTY_CHK_DNSSRV_INFO));
    return ANSC_STATUS_SUCCESS;
}

/* Logic

    This thread will trigger a one shot query based on the provided info and exit upon execution
*/

void *wancnctvty_chk_querynow_thread( void *arg)
{
    pthread_detach( pthread_self( ) );
    PCOSA_DML_WANCNCTVTY_CHK_QUERYNOW_CTXT_INFO pQuerynowCtxt=
                                          (PCOSA_DML_WANCNCTVTY_CHK_QUERYNOW_CTXT_INFO)arg;
    if (!pQuerynowCtxt)
    {
       WANCHK_LOG_ERROR("pQuerynowCtxt data is NULL,unable to execute query now\n");
       return NULL;
    }  
    pthread_cleanup_push(cleanup_querynow, pQuerynowCtxt);
    wancnctvty_chk_frame_and_send_query(pQuerynowCtxt,QUERYNOW_INVOKE);
    pthread_cleanup_pop(1);
    return NULL;
}

/* Logic

    Do a one-shot send query with the provided interface info and start the timer based on
    ActiveMonitor Interval. Further Queries will be triggered from the timer
*/

void *wancnctvty_chk_active_thread( void *arg)
{
    /*lets detach the thread*/
    pthread_detach( pthread_self( ) );
    PCOSA_DML_WANCNCTVTY_CHK_ACTIVEQUERY_CTXT_INFO pActvQueryCtxt   =
                                                (PCOSA_DML_WANCNCTVTY_CHK_ACTIVEQUERY_CTXT_INFO)arg;
    PWAN_CNCTVTY_CHK_ACTIVE_MONITOR   pActive        = NULL;

    pActive = (PWAN_CNCTVTY_CHK_ACTIVE_MONITOR)
                                        AnscAllocateMemory(sizeof(WAN_CNCTVTY_CHK_ACTIVE_MONITOR));

    if (!pActive)
    {
        WANCHK_LOG_ERROR("%s:Unable to allocate memory\n",__FUNCTION__);
        return NULL;
    }

    pthread_cleanup_push(cleanup_activequery, pActvQueryCtxt);
    /* lets send first query and start the timer to go for repeated trigger*/
    wancnctvty_chk_frame_and_send_query(pActvQueryCtxt->pQueryCtxt,ACTIVE_MONITOR_INVOKE);

    WANCHK_LOG_INFO("Start active Monitor\n");

    pActive->loop = ev_loop_new(EVFLAG_AUTO);
    pActive->actvtimer.repeat = (pActvQueryCtxt->ActiveMonitorInterval/1000);
    pActive->actvtimer.data   = pActvQueryCtxt->pQueryCtxt;
    ev_init (&pActive->actvtimer, wanchk_actv_querytimeout_cb);
    ev_timer_again (pActive->loop,&pActive->actvtimer);
    pthread_cleanup_push(cleanup_activemonitor_ev, pActive);

    ev_run (pActive->loop, 0);

    pthread_cleanup_pop(0);
    pthread_cleanup_pop(0);
    return NULL;
}

/* Active query timer will fire on provided activemonitor interval*/

void
wanchk_actv_querytimeout_cb (EV_P_ ev_timer *w, int revents)
{
   WANCHK_LOG_INFO("%s:Active Query timeout happened\n",__FUNCTION__);
   PCOSA_DML_WANCNCTVTY_CHK_QUERYNOW_CTXT_INFO pActvQueryCtxt = 
                                            (PCOSA_DML_WANCNCTVTY_CHK_QUERYNOW_CTXT_INFO)w->data;
   wancnctvty_chk_frame_and_send_query(pActvQueryCtxt,ACTIVE_MONITOR_INVOKE);
}


static void cleanup_activemonitor_ev(void *arg)
{
    PWAN_CNCTVTY_CHK_ACTIVE_MONITOR   pActive = (PWAN_CNCTVTY_CHK_ACTIVE_MONITOR)arg;
    WANCHK_LOG_INFO("stopping active monitor loop\n");
    ev_timer_stop(pActive->loop,&pActive->actvtimer);
    ev_break(pActive->loop,EVBREAK_ALL);
    ev_loop_destroy(pActive->loop);
    if (pActive)
    {
        AnscFreeMemory(pActive);
    }
}

static void cleanup_activequery(void *arg)
{
    PCOSA_DML_WANCNCTVTY_CHK_ACTIVEQUERY_CTXT_INFO pActvQueryCtxt = 
                                            (PCOSA_DML_WANCNCTVTY_CHK_ACTIVEQUERY_CTXT_INFO)arg;
    PCOSA_DML_WANCNCTVTY_CHK_QUERYNOW_CTXT_INFO pQuerynowCtxt = pActvQueryCtxt->pQueryCtxt;

    WANCHK_LOG_DBG("Cleanup Active query for interface %s\n",pQuerynowCtxt->InterfaceName);
    if (pQuerynowCtxt->DnsServerList)
    {
        AnscFreeMemory(pQuerynowCtxt->DnsServerList);
        pQuerynowCtxt->DnsServerList = NULL;
    }
    if (pQuerynowCtxt->url_list)
    {
        int index=0;
        for (index=0;index < pQuerynowCtxt->url_count;index++)
        {
            if (pQuerynowCtxt->url_list[index])
                free(pQuerynowCtxt->url_list[index]);
        }
        free(pQuerynowCtxt->url_list);
        pQuerynowCtxt->url_list = NULL;
    }
    if (pQuerynowCtxt)
    {
        AnscFreeMemory(pQuerynowCtxt);
        pQuerynowCtxt = NULL;
    }
    if (pActvQueryCtxt)
    {
        AnscFreeMemory(pActvQueryCtxt);
        pActvQueryCtxt = NULL;
    }
}

static void cleanup_querynow_fd(void *arg)
{
    int fd = *(int *)arg;
    WANCHK_LOG_DBG("Free fd thread descriptor:%d\n",fd);
    close(fd);
}

static void cleanup_querynow(void *arg)
{
    PCOSA_DML_WANCNCTVTY_CHK_QUERYNOW_CTXT_INFO pQuerynowCtxt =
                                            (PCOSA_DML_WANCNCTVTY_CHK_QUERYNOW_CTXT_INFO)arg;
    WANCHK_LOG_DBG("Cleanup querynow for interface %s\n",pQuerynowCtxt->InterfaceName);
    if (pQuerynowCtxt->DnsServerList)
    {
        free(pQuerynowCtxt->DnsServerList);
        pQuerynowCtxt->DnsServerList = NULL;
    }
    if (pQuerynowCtxt->url_list)
    {
        int index=0;
        for (index=0;index < pQuerynowCtxt->url_count;index++)
        {
            if (pQuerynowCtxt->url_list[index])
                free(pQuerynowCtxt->url_list[index]);
        }
        free(pQuerynowCtxt->url_list);
        pQuerynowCtxt->url_list = NULL;
    }
    if (pQuerynowCtxt)
    {
        /* Clear tid and state*/
        pthread_mutex_lock(&gIntfAccessMutex);
        PWANCNCTVTY_CHK_GLOBAL_INTF_INFO gIntfInfo = &gInterface_List[pQuerynowCtxt->InstanceNumber-1];
        gIntfInfo->QueryNow_Running = FALSE;
        gIntfInfo->wancnctvychkquerynowthread_tid = 0;
        pthread_mutex_unlock(&gIntfAccessMutex);
        AnscFreeMemory(pQuerynowCtxt);
        pQuerynowCtxt = NULL;
    }
}

/* Logic

    Passive Monitor Thread
    Decision of capture depends on server type
    1. IPV4 only, capture only v4 packets.
    2. IPv6 only, capture only v6 packets.
    3. BOTH IPv4/IPv6 or EITHER, capture both v4 and v6.

    Decsion of Result update will based upon record type
    1. IPV4 only, result set to TRUE on successful detection of ns_t_a packets.
    2. IPv6 only, result set to TRUE on successful detection of ns_t_aaaa packets.
    3. BOTH IPv4/IPv6, result set to TRUE on successful detection of both ns_t_a and ns_t_aaaa.
    4. EITHER v4/v6, result set to TRUE on successful detection of either ns_t_a or ns_t_aaaa.

Timer : background timer will be started on initialization of this thread, this timer will be
reset whenever above result criteria was matched.

if timer expires, then active monitor will be fired based on enable status or result will
be updated as disconnected.

*/

void *wancnctvty_chk_passive_thread( void *arg )
{
    /*lets detach the thread*/
    pthread_detach( pthread_self( ));
    PCOSA_DML_WANCNCTVTY_CHK_INTF_INFO pIPInterface   = (PCOSA_DML_WANCNCTVTY_CHK_INTF_INFO)arg;
   // PCOSA_DATAMODEL_WANCNCTVTY_CHK         pMyObject  = (PCOSA_DATAMODEL_WANCNCTVTY_CHK)
   //                                                      g_pCosaBEManager->hWanCnctvty_Chk;

    servertype_t srvrtype;
    PWAN_CNCTVTY_CHK_PASSIVE_MONITOR pPassive         = NULL;
 
    pPassive = (PWAN_CNCTVTY_CHK_PASSIVE_MONITOR)AnscAllocateMemory(sizeof(WAN_CNCTVTY_CHK_PASSIVE_MONITOR));

    if (!pPassive)
    {
        WANCHK_LOG_ERROR("%s:Unable to allocate memory",__FUNCTION__);
        return NULL;
    }
    
    pPassive->loop = EV_DEFAULT;
    /* Lets decide the family of nfq based on the record type we want to monitor*/
    if ( get_server_type(pIPInterface->ServerType,&srvrtype) != -1)
    {
        if ((srvrtype == SRVR_EITHER_IPV4_IPV6) || (srvrtype == SRVR_BOTH_IPV4_IPV6) || (srvrtype == SRVR_IPV4_ONLY))
        {
            if ((wancnctvty_chk_create_nfq(AF_INET,pPassive) != -1))
            {
                /* nfq creation succeeded let's start ev to monitor the fds*/
                /* Register FD for libev events */
                ev_io_init(&pPassive->evio_v4, wanchk_pckt_recv_v4, pPassive->nfq_fd_v4, EV_READ);
                /* Start watching it on the default queue */
                pPassive->evio_v4.data=(void *)pPassive;
                ev_io_start(pPassive->loop, &pPassive->evio_v4);
            }
            else
            {
                WANCHK_LOG_ERROR("%s : Unable to create nfq for server type:%s",__FUNCTION__,
                                                                        pIPInterface->ServerType);
                if (pPassive)
                    AnscFreeMemory(pPassive);
                return NULL;
            }
        }

        if ((srvrtype == SRVR_EITHER_IPV4_IPV6) || (srvrtype == SRVR_BOTH_IPV4_IPV6) || (srvrtype == SRVR_IPV6_ONLY))
        {
            if ((wancnctvty_chk_create_nfq(AF_INET6,pPassive) != -1))
            {
                /* nfq creation succeeded let's start ev to monitor the fds*/
                /* Register FD for libev events */
                ev_io_init(&pPassive->evio_v6, wanchk_pckt_recv_v6, pPassive->nfq_fd_v6, EV_READ);
                /* Start watching it on the default queue */
                pPassive->evio_v6.data=(void *)pPassive;
                ev_io_start(pPassive->loop, &pPassive->evio_v6);
                
            }
            else
            {
                WANCHK_LOG_ERROR("%s : Unable to create nfq for server type:%s",__FUNCTION__,
                                                                        pIPInterface->ServerType);
                if (pPassive)
                    AnscFreeMemory(pPassive);
                return NULL;
            }
        }
    }

    ev_init (&pPassive->bgtimer, wanchk_bgtimeout_cb);
    pPassive->bgtimer.repeat = 30.;
    ev_timer_again (pPassive->loop,&pPassive->bgtimer);
    ev_run (pPassive->loop, 0);

    return NULL;
}

void
wanchk_bgtimeout_cb (EV_P_ ev_timer *w, int revents)
{
   WANCHK_LOG_ERROR("%s:timeput happened\n",__FUNCTION__);
}


/* handler to populate and send query for both active and query now

Logic :


    Decision of trigger of packets depends on server type
    1. IPV4 only, send query only on v4 servers.
    2. IPv6 only, send query only on v6 servers.
    3. BOTH IPv4/IPv6 or EITHER, send query on both v4 and v6 servers.

For each URL
    Packets will be send based on above server type criteria.

For each server
    Decsion of  Query population will based upon record type
    1. IPV4 only, make query only with ns_t_a packets.
    2. IPv6 only, make  query only with ns_t_aaaa packets.
    3. BOTH IPv4/IPv6, make query with both ns_t_a and ns_t_aaaa.
    4. EITHER v4/v6, make query with both ns_t_a or ns_t_aaaa.
   
    Decison of result will be based on the successful responses with above
    record type criteria, upon success Result will be updated and further
    servers will be skipped.
done
    if URL resolved with provided requirements skip other URLS.
done

*/


ANSC_STATUS wancnctvty_chk_frame_and_send_query(
    PCOSA_DML_WANCNCTVTY_CHK_QUERYNOW_CTXT_INFO pQuerynowCtxt,queryinvoke_type_t invoke_type)
{
    char **url_list     = NULL; 
    recordtype_t rectype;
    struct query query_info;
    int i=0;
    errno_t rc = -1;
   int ind         = -1;
    servertype_t srvrtype;
    BOOL  use_ipv4_dns = FALSE;
    BOOL  use_ipv6_dns = FALSE;
    unsigned int total_url_entries = 0;
    unsigned int dns_server_count = 0;
    int ret = ANSC_STATUS_SUCCESS;

    if (!pQuerynowCtxt)
    {
       WANCHK_LOG_ERROR("pQuerynowCtxt data is NULL,unable to execute query\n");
       ret = ANSC_STATUS_FAILURE;
       goto EXIT;
    }
    /* Fetching here ensures we are taking updated list on consecutive queries, if there is a change
    */
    url_list = get_url_list(&total_url_entries);
    pQuerynowCtxt->url_count = total_url_entries;

    if (!total_url_entries)
    {
        WANCHK_LOG_ERROR("URL count is zero, Abort query\n");
        ret = ANSC_STATUS_FAILURE;
        goto EXIT;
    }

    /* update context structure so that it can be freed up in cleanup*/
    /* Fetch url list*/
    pQuerynowCtxt->url_count = total_url_entries;
    pQuerynowCtxt->url_list = url_list;

    if (!pQuerynowCtxt->DnsServerCount)
    {
        WANCHK_LOG_ERROR("No Dns servers found, Abort query\n");
        ret = ANSC_STATUS_FAILURE;
        goto EXIT;
    }
    else
    {
        dns_server_count = pQuerynowCtxt->DnsServerCount;
    }

// Fetch server type

    if ( get_server_type(pQuerynowCtxt->ServerType,&srvrtype) != -1)
    {
        if ((srvrtype == SRVR_EITHER_IPV4_IPV6) || (srvrtype == SRVR_BOTH_IPV4_IPV6) ||
                                                                    (srvrtype == SRVR_IPV4_ONLY))
        {
            use_ipv4_dns = TRUE;
        }

        if ((srvrtype == SRVR_EITHER_IPV4_IPV6) || (srvrtype == SRVR_BOTH_IPV4_IPV6) || 
                                                                    (srvrtype == SRVR_IPV6_ONLY))
        {
            use_ipv6_dns = TRUE;
        }

        WANCHK_LOG_DBG("Querynow ServerType:%s use_ipv4_dns:%d use_ipv6_dns:%d\n",
                                            pQuerynowCtxt->ServerType,use_ipv4_dns,use_ipv6_dns);
    }
    else
    {
        WANCHK_LOG_ERROR("Unable to fetch server type info, abort query on interface:%s\n",
                                                                  pQuerynowCtxt->InterfaceName);
        ret = ANSC_STATUS_FAILURE;
        goto EXIT;
    }

// Frame Query based on Record Type

    BOOL v4_query = FALSE;
    BOOL v6_query = FALSE;
    int no_of_queries = 0;
    BOOL URL_v4_resolved = FALSE;
    BOOL URL_v6_resolved = FALSE;
    BOOL use_raw_socket = FALSE;
    if ( get_record_type(pQuerynowCtxt->RecordType,&rectype) != -1)
    {
        if ((rectype == EITHER_IPV4_IPV6) || (rectype == BOTH_IPV4_IPV6) || (rectype == IPV4_ONLY))
        {
            v4_query = TRUE;
            no_of_queries++;
        }

        if ((rectype == EITHER_IPV4_IPV6) || (rectype == BOTH_IPV4_IPV6) || (rectype == IPV6_ONLY))
        {
            v6_query = TRUE;
            no_of_queries++;
        }
        WANCHK_LOG_DBG("Querynow RecordType:%d A:%d AAAA:%d dns_server_count:%d\n",
                                                    rectype,v4_query,v6_query,dns_server_count);

        int url_index=0;
        for (url_index=0;url_index < total_url_entries;url_index++)
        {
            WANCHK_LOG_INFO("Checking Connectivity with URL %s on interface: %s\n",url_list[url_index],
                                                                    pQuerynowCtxt->InterfaceName);
            struct mk_query query_list[2];
            int query_index = 0;
            if (v4_query)
            {
                memset(&query_list[query_index].query,0,sizeof(query_list[query_index].query));
                query_list[query_index].qlen = 0;
                query_list[query_index].resp_recvd = 0;
                query_list[query_index].qlen = res_mkquery(QUERY, url_list[url_index], C_IN, ns_t_a,
                                                            /*data:*/ NULL, /*datalen:*/ 0,
                                                            /*newrr:*/ NULL,
                                                            query_list[query_index].query,
                                                            sizeof(query_list[query_index].query));
                query_index++;
            }
            if (v6_query)
            {
                memset(&query_list[query_index].query,0,sizeof(query_list[query_index].query));
                query_list[query_index].qlen = 0;
                query_list[query_index].resp_recvd = 0;
                query_list[query_index].qlen = res_mkquery(ns_o_query, url_list[url_index], C_IN, ns_t_aaaa,
                                                            /*data:*/ NULL, /*datalen:*/ 0,
                                                            /*newrr:*/ NULL,
                                                            query_list[query_index].query,
                                                            sizeof(query_list[query_index].query));
            }
            /* we are trying to mimic cpe dns behaviour, traverse the same way in same order dns servers
             are populated, mark resolution based on results*/
            for (i=0;i < dns_server_count; ++i)
            {
                memset(&query_info,0,sizeof(struct query));
                rc = strcpy_s(query_info.ifname, MAX_INTF_NAME_SIZE , pQuerynowCtxt->InterfaceName);
                ERR_CHK(rc);
                query_info.query_timeout = pQuerynowCtxt->QueryTimeout;
                query_info.query_retry   = pQuerynowCtxt->QueryRetry;
                query_info.query_count = no_of_queries;
                query_info.rqstd_rectype = rectype;
                rc = strcmp_s( "Backup",strlen("Backup"),pQuerynowCtxt->Alias, &ind);
                ERR_CHK(rc);
                if((!ind) && (rc == EOK))
                {
                    use_raw_socket = TRUE;
                }
                if (use_ipv4_dns && !URL_v4_resolved)
                {
                    if (pQuerynowCtxt->DnsServerList && pQuerynowCtxt->DnsServerList[i].dns_type == DNS_SRV_IPV4)
                    {
                        rc = strcpy_s(query_info.ns, MAX_URL_SIZE,
                                            pQuerynowCtxt->DnsServerList[i].IPAddress.IPv4Address);
                        ERR_CHK(rc);
                        query_info.skt_family = AF_INET;
                        query_list[0].resp_recvd = 0;
                        query_list[1].resp_recvd = 0;
                        if (!send_query(&query_info,query_list,use_raw_socket))
                        {
                            WANCHK_LOG_DBG("DNS Resolved Successfully for URL:%s on IPv4 Server:%s\n",
                                                                                url_list[url_index],
                                            pQuerynowCtxt->DnsServerList[i].IPAddress.IPv4Address);
                            URL_v4_resolved = TRUE;
                        }
                        else
                        {
                            WANCHK_LOG_DBG("DNS Resolution Failed for URL:%s on IPv4 Server:%s\n",
                                                                            url_list[url_index],
                                            pQuerynowCtxt->DnsServerList[i].IPAddress.IPv4Address);
                        }
                    }
                }

                if (use_ipv6_dns && !URL_v6_resolved)
                {
                    if (pQuerynowCtxt->DnsServerList &&
                                        pQuerynowCtxt->DnsServerList[i].dns_type == DNS_SRV_IPV6)
                    {
                        memset(query_info.ns,0,MAX_URL_SIZE);
                        rc = strcpy_s(query_info.ns, MAX_URL_SIZE ,
                                            pQuerynowCtxt->DnsServerList[i].IPAddress.IPv6Address);
                        ERR_CHK(rc);
                        query_info.skt_family = AF_INET6;
                        query_list[0].resp_recvd = 0;
                        query_list[1].resp_recvd = 0;
                        if (!send_query(&query_info,query_list,use_raw_socket))
                        {
                            WANCHK_LOG_DBG("DNS Resolved Successfully for URL:%s IPv6 Server:%s\n",
                                                                              url_list[url_index],
                                            pQuerynowCtxt->DnsServerList[i].IPAddress.IPv6Address);
                            URL_v6_resolved = TRUE;
                        }
                        else
                        {
                            WANCHK_LOG_DBG("DNS Resolution Failed for URL:%s on IPv6 Server:%s\n",
                                                                            url_list[url_index],
                                            pQuerynowCtxt->DnsServerList[i].IPAddress.IPv6Address);
                        }
                    }
                }
                if ( ((srvrtype == SRVR_IPV4_ONLY) && URL_v4_resolved) ||
                    ((srvrtype == SRVR_IPV6_ONLY) && URL_v6_resolved) ||
                    ((srvrtype == SRVR_BOTH_IPV4_IPV6) && (URL_v6_resolved && URL_v4_resolved)) ||
                    ((srvrtype == SRVR_EITHER_IPV4_IPV6) && (URL_v6_resolved || URL_v4_resolved)) )
                {
                    WANCHK_LOG_INFO("%s DNS Resolution Succeeded for URL %s on Interface %s ServerType:%d Status IPv4:%d IPv6:%d\n",
                            (invoke_type == QUERYNOW_INVOKE) ? "QueryNow" : "ActiveMonitor",url_list[url_index],
                            pQuerynowCtxt->InterfaceName,srvrtype,URL_v4_resolved,URL_v6_resolved);
                    if (invoke_type == QUERYNOW_INVOKE)
                    {
                        wancnctvty_chk_querynow_result_update(pQuerynowCtxt->InstanceNumber,
                                                           QUERYNOW_RESULT_CONNECTED);
                        ret = ANSC_STATUS_SUCCESS;
                    }
                    else if (invoke_type == ACTIVE_MONITOR_INVOKE)
                    {
                        wancnctvty_chk_monitor_result_update(pQuerynowCtxt->InstanceNumber,
                                                        MONITOR_RESULT_CONNECTED);
                        ret = ANSC_STATUS_SUCCESS;
                    }
                    goto EXIT;
                }
            }

            WANCHK_LOG_ERROR("Resolution Failed for URL %s, lets try next URL\n",url_list[url_index]);
            ret = ANSC_STATUS_FAILURE;
        }
    }
    else
    {
        WANCHK_LOG_ERROR("Unable to fetch record type info, abort query on interface:%s\n",
                                                                    pQuerynowCtxt->InterfaceName);
        ret = ANSC_STATUS_FAILURE;
    }

EXIT:
    if (ret == ANSC_STATUS_FAILURE)
    {
        WANCHK_LOG_ERROR("%s DNS Resolution Failed on Interface %s ServerType:%d Status IPv4:%d IPv6:%d\n",
                    (invoke_type == QUERYNOW_INVOKE) ? "QueryNow" : "ActiveMonitor", 
                    pQuerynowCtxt->InterfaceName,srvrtype,URL_v4_resolved,URL_v6_resolved);
        if (invoke_type == QUERYNOW_INVOKE)
        {
            wancnctvty_chk_querynow_result_update(pQuerynowCtxt->InstanceNumber,
                                                        QUERYNOW_RESULT_DISCONNECTED);
        }
        else if (invoke_type == ACTIVE_MONITOR_INVOKE)
        {
            wancnctvty_chk_monitor_result_update(pQuerynowCtxt->InstanceNumber,
                                                        MONITOR_RESULT_DISCONNECTED);
        }
    }

    return ret;
}

int send_query_create_raw_skt(struct query *query_info)
{
    int fd = -1;
    struct sockaddr_ll socket_ll;
    struct ifreq inf_request;
    unsigned short protocol_family = ETH_P_ALL;

    protocol_family = (query_info->skt_family == AF_INET) ? ETH_P_IP : ETH_P_IPV6;
    fd = socket(PF_PACKET, SOCK_RAW, htons(protocol_family));
    memset(&socket_ll, 0, sizeof(socket_ll));
    memset(&inf_request,0, sizeof(inf_request));

    strncpy((char *)inf_request.ifr_name, query_info->ifname, sizeof(inf_request.ifr_name)-1);
    inf_request.ifr_name[sizeof(inf_request.ifr_name)-1] = '\0';
    if(-1 == (ioctl(fd, SIOCGIFINDEX, &inf_request)))
    {
        WANCHK_LOG_ERROR("Error getting Interface index !\n");
        return -1;
    }

    /* Bind our raw socket to this interface */
    socket_ll.sll_family   = AF_PACKET;
    socket_ll.sll_ifindex  = inf_request.ifr_ifindex;
    socket_ll.sll_protocol = htons(protocol_family);

    /* try attaching a dns bpf filter, Future scope check based on random src port , can we 
    manipulate bpf filter*/

    socket_dns_filter_attach(fd,dns_packet_filter,DNS_PACKET_FILTER_SIZE);

    if(-1 == (bind(fd, (struct sockaddr *)&socket_ll, sizeof(socket_ll))))
    {
        WANCHK_LOG_ERROR("Error binding to socket!\n");
        return -1;
    }

    return fd;

}

int send_query_create_udp_skt(struct query *query_info)
{
    int fd = -1;
    struct sockaddr_in local_addrv4;
    struct sockaddr_in6 local_addrv6;
    struct sockaddr_in serv_addrv4;
    struct sockaddr_in6 serv_addrv6;
    char ipv4_src[IPv4_STR_LEN] = {0};
    char ipv6_src[IPv6_STR_LEN] = {0};
    int flag = 1;

    fd = socket(query_info->skt_family, SOCK_DGRAM, IPPROTO_UDP);
    if ( query_info->skt_family == AF_INET )
    {
        if (get_ipv4_addr(query_info->ifname,ipv4_src) != 0)
        {
            WANCHK_LOG_ERROR("Unable to get IPv4 Address for interface:%s\n",query_info->ifname);
            return -1;
        }
        
        WANCHK_LOG_DBG("got ipv4 address %s for interface:%s\n",ipv4_src,query_info->ifname);
        
        memset(&local_addrv4, 0, sizeof(local_addrv4));
        if (inet_pton(AF_INET, ipv4_src, &(local_addrv4.sin_addr)) != 1)
        {
            WANCHK_LOG_ERROR("Unable to assign ipv4 Address");
            return -1;
        }
        local_addrv4.sin_family = AF_INET;
        local_addrv4.sin_port = 0;

        if(setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,(char *)&flag,sizeof(flag))<0){
            WANCHK_LOG_ERROR("Error: Could not set reuse address option\n");
            return -1;
        }

        if (bind(fd, (struct sockaddr *)&local_addrv4, sizeof(local_addrv4)) < 0)
        {
            WANCHK_LOG_ERROR("Unable to bind socket for interface :%s\n",query_info->ifname);
            return -1;
        }

        serv_addrv4.sin_family = AF_INET;
        serv_addrv4.sin_port = htons(53);
        if (inet_pton(AF_INET, query_info->ns, &(serv_addrv4.sin_addr)) != 1)
        {
           WANCHK_LOG_ERROR("Unable to assign server ipv4 Address\n");
           return -1;
        }

        if (connect(fd, (struct sockaddr *) &serv_addrv4, sizeof(serv_addrv4)) < 0)
        {
            WANCHK_LOG_ERROR("connect socket for interface :%s failed for dst :%s\n",
                                                                query_info->ifname,query_info->ns);
            return -1;
        }
    }
    else if ( query_info->skt_family == AF_INET6 )
    {
        if (get_ipv6_addr(query_info->ifname,ipv6_src) !=0)
        {
           WANCHK_LOG_ERROR("Unable to get IPv6 Address for interface:%s\n",query_info->ifname);
           return -1;
        }

        WANCHK_LOG_DBG("got ipv6 address %s for interface:%s\n",ipv6_src,query_info->ifname);

        memset(&local_addrv6, 0, sizeof(local_addrv6));
        if (inet_pton(AF_INET6, ipv6_src, &(local_addrv6.sin6_addr)) != 1)
        {
            WANCHK_LOG_ERROR("Unable to assign IPv6 Address\n");
            return -1;
        }
        local_addrv6.sin6_family = AF_INET6;
        local_addrv6.sin6_port = 0;

        if (bind(fd, (struct sockaddr *)&local_addrv6, sizeof(local_addrv6)) < 0)
        {
            WANCHK_LOG_ERROR("Unable to bind socket for interface :%s\n",query_info->ifname);
            return -1;
        }
        serv_addrv6.sin6_family = AF_INET6;
        serv_addrv6.sin6_port = htons(53);
        if (inet_pton(AF_INET6, query_info->ns, &(serv_addrv6.sin6_addr)) != 1)
        {
           WANCHK_LOG_ERROR("Unable to assign server ipv6 Address\n");
           return -1;
        }

        if (connect(fd, (struct sockaddr *) &serv_addrv6, sizeof(serv_addrv6)) < 0)
        {
            WANCHK_LOG_ERROR("connect socket for interface :%s failed for dst :%s\n",query_info->ifname,query_info->ns);
            return -1;
        }
    } 
    else
    {
        WANCHK_LOG_ERROR("Unsupported socket family\n");
        return -1;
    } 
    return fd;
}

/* only use for secondary, since no other way*/
unsigned int send_query_frame_raw_pkt(struct query *query_info,struct mk_query *query,unsigned char *packet)
{
    char srcMac[MACADDR_SZ] = {0};
    char dstMac[MACADDR_SZ] = {0};
    char ipv4_src[IPv4_STR_LEN] = {0};
    char ipv6_src[IPv6_STR_LEN] = {0};
    unsigned short source_port = 0;
    struct iphdr ipv4_header = {0};
    struct udphdr udp_header = {0};
    struct ip6_hdr ipv6_header = {0};
    struct ethhdr ethernet_header ={0};
    unsigned int pkt_len = 0;
    int ip_offset = 0;
    FILE *fp = NULL;
        /* Fetch HW address for interface*/

    if (-1 == get_mac_addr(query_info->ifname,srcMac)) {
        WANCHK_LOG_ERROR("Error Fetching Interface HW address!\n");
        return 0;
    } else {
        WANCHK_LOG_DBG("Interface HW address:%s!\n",srcMac);
    }

    /* Fetch dst mac*/
    /* Dns is already our peer address, so do a ping to fetch mac address*/

    if (query_info->skt_family == AF_INET) {
            /* for best effort since in extender mode we don't have extensive traffic*/
            v_secure_system( "ping -c 1 -W 1 -I %s %s",query_info->ifname,query_info->ns);
            fp = v_secure_popen("r","ip -4 neigh show %s dev %s | cut -d ' ' -f 3",query_info->ns,
                                                                        query_info->ifname);
    } else {
            v_secure_system( "ping6 -c 1 -W 1 -I %s %s",query_info->ifname,query_info->ns);
            fp = v_secure_popen("r","ip -6 neigh show %s dev %s | cut -d ' ' -f 3",query_info->ns,
                                                                    query_info->ifname);
    }

    if (fp)
    {
        _get_shell_output(fp, dstMac, sizeof(dstMac));
        v_secure_pclose(fp);
    }

    if (!strlen(dstMac))
    {
        WANCHK_LOG_ERROR("Unable to fetch DST HW address:%s!\n",dstMac);
        return 0;
    }
    else if (validatemac(dstMac) == FALSE) 
    {
        WANCHK_LOG_ERROR("DST HW address Not resolved for %s!\n",query_info->ns);
        return 0;
    }
    else {
        WANCHK_LOG_DBG("Dest HW address:%s!\n",dstMac);         
    }

    /* ??? Ingress traffic on brWAN(LTE side) for port 53 always DNAT to brWAN. then it is upto
    dnsmasq on LTE to forward. So LTE on extender mode doesn't really honors the destination ip address
    And also Device.DNS.Client. has gateway as DNS server not actual LTE DNS servers. Not seeing 
    scope of DNS servers on secondary. revisit on RDKB-43219 discuss with arch team*/

    if (get_ipv4_addr(query_info->ifname,ipv4_src) != 0)
    {
        WANCHK_LOG_ERROR("Unable to get IPv4 Address for interface:%s\n",query_info->ifname);
        return 0;
    }
    WANCHK_LOG_DBG("got ipv4 address %s for interface:%s\n",ipv4_src,query_info->ifname);

    if (get_ipv6_addr(query_info->ifname,ipv6_src) !=0)
    {
       WANCHK_LOG_ERROR("Unable to get IPv6 Address for interface:%s\n",query_info->ifname); 
       return 0;
    }
    WANCHK_LOG_DBG("got ipv6 address %s for interface:%s\n",ipv6_src,query_info->ifname);

        /* slecting some random port*/
    source_port = 40000+rand()%10000;
    if (source_port <= 1024)
    {
        source_port = 40000+rand()%10000;
    }
    WANCHK_LOG_INFO("Random port for UDP DNS Query %d\n",source_port);
    WanCnctvtyChk_CreateUdpHeader(query_info->skt_family, source_port, 53 ,
                                                        query->qlen,&udp_header);

    if ( query_info->skt_family == AF_INET )
    {
        WanCnctvtyChk_CreateIPHeader(query_info->skt_family, ipv4_src, query_info->ns,
                                                    query->qlen,&ipv4_header);
                    /* Create PseudoHeader and compute UDP Checksum  */
        WanCnctvtyChk_CreatePseudoHeaderAndComputeUdpChecksum(query_info->skt_family,
                            &udp_header, &ipv4_header, query->query,query->qlen);
        /* Packet length = ETH + IP header + UDP header + Data*/
        pkt_len = sizeof(struct ethhdr) + ntohs(ipv4_header.tot_len);
        ip_offset = ipv4_header.ihl*4;
    }
    else
    {
        WanCnctvtyChk_CreateIPHeader(query_info->skt_family, ipv6_src, 
                                        query_info->ns, query->qlen,&ipv6_header);
                                    /* Create PseudoHeader and compute UDP Checksum  */
        WanCnctvtyChk_CreatePseudoHeaderAndComputeUdpChecksum(query_info->skt_family,
                            &udp_header, &ipv6_header, query->query,query->qlen);
        pkt_len = sizeof(struct ethhdr) + sizeof(struct ip6_hdr) + ntohs(ipv6_header.ip6_plen);
        ip_offset = sizeof(struct ip6_hdr);
    }

    WanCnctvtyChk_CreateEthernetHeader(&ethernet_header,srcMac, dstMac, 
                            query_info->skt_family == AF_INET ? ETHERTYPE_IP : ETHERTYPE_IPV6);

    /* Copy the Ethernet header first */
    memcpy(packet, &ethernet_header, sizeof(struct ethhdr));

    /* Copy the IP header -- but after the ethernet header */
    if(query_info->skt_family == AF_INET){
        memcpy((packet + sizeof(struct ethhdr)), &ipv4_header, ip_offset);
    } else {
        memcpy((packet + sizeof(struct ethhdr)), &ipv6_header, ip_offset);
    }

    /* Copy the UDP header after the IP header */
    memcpy((packet + sizeof(struct ethhdr) + ip_offset),&udp_header, sizeof(struct udphdr));

    /* Copy the Data after the UDP header */
    memcpy((packet + sizeof(struct ethhdr) + ip_offset + sizeof(struct udphdr)),
                                                query->query, query->qlen);
    return pkt_len;
}

int send_query(struct query *query_info,struct mk_query *query_list,BOOL use_raw_socket)
{
    struct pollfd pfd;
    int rc = -1;
    volatile int ret = 0;
    int ret_parse = 0;
    unsigned char reply[BUFLEN_4096] = {0};
    uint8_t rcode;

    WANCHK_LOG_INFO("Send Query on Interface :%s to Nameserver: %s\n",query_info->ifname,query_info->ns);
    pfd.events = POLLIN;

    if (use_raw_socket) {
        pfd.fd = send_query_create_raw_skt(query_info);
    } else {
        pfd.fd = send_query_create_udp_skt(query_info);
    }

    if (pfd.fd == -1)
    {
        WANCHK_LOG_ERROR("Error creating socket !\n");
        goto EXIT;

    }
    WANCHK_LOG_DBG("Interface :%s fd :%d\n",query_info->ifname,pfd.fd);
    pthread_cleanup_push(cleanup_querynow_fd,&pfd.fd);
  
    int flags = fcntl(pfd.fd, F_GETFL);
    fcntl(pfd.fd, F_SETFL, flags | O_NONBLOCK);
    volatile BOOL v4_resolved = FALSE;
    volatile BOOL v6_resolved = FALSE;
    int no_of_replies = 0;
    int retry_count = 0;
    int64_t start_time, current_time;
    int64_t timeout = 0;
    char packet[BUFLEN_4096] = {0};
    unsigned int pkt_len = 0;

    while(retry_count <= query_info->query_retry)
    {
        int recvlen=0 ;

        for (int i=0;i < query_info->query_count;i++)
        {
            if (query_list[i].resp_recvd)
               continue;

            if (use_raw_socket)
            {
                WANCHK_LOG_DBG("Frame Raw UDP Packet for Interface :%s fd: %d\n",query_info->ifname,pfd.fd);
                pkt_len = send_query_frame_raw_pkt(query_info,&query_list[i],packet);
                if (!pkt_len)
                {
                    WANCHK_LOG_ERROR("Error in Creating Raw Packet\n");
                    goto EXIT;
                }
            }
            else
            {
                WANCHK_LOG_DBG("Frame UDP Payload for Interface :%s fd: %d\n",query_info->ifname,pfd.fd);
                memcpy(packet,query_list[i].query,query_list[i].qlen);
                pkt_len = query_list[i].qlen;
            }
            if (write(pfd.fd, packet, pkt_len) < 0) {
                WANCHK_LOG_WARN("write failed for nameserver %s\r\n",query_info->ns);
                ret = -1;
                goto EXIT;
            }
        }

        WANCHK_LOG_INFO("Sending %s Attempt:%d\n",retry_count? "Query Retry":"Query",retry_count);
        timeout = query_info->query_timeout;

wait_for_response:
        start_time = clock_mono_ms();
        WANCHK_LOG_DBG("waiting for response with timeout:%lld\n",timeout);
        if (timeout >= 0) {
            rc = poll(&pfd, 1, timeout);
        } else {
            WANCHK_LOG_WARN("Timeout happened on query response for interface: %s\n",query_info->ifname);
            retry_count++;
            continue;
        }

        WANCHK_LOG_DBG("poll return code:%d\n",rc);
        if(rc < 0) {
           WANCHK_LOG_ERROR("poll for fd:%d failed on interface :%s\n",pfd.fd,query_info->ifname);
           ret = -1;
           goto EXIT;
        } else if (rc == 0) {
           WANCHK_LOG_WARN("Timeout happened on query response for interface: %s\n",query_info->ifname);
           retry_count++;
           continue;
        } else {
            memset(reply,0,sizeof(reply));
            recvlen = read(pfd.fd, reply, sizeof(reply));
            if (recvlen < 0) {
                WANCHK_LOG_WARN("recvlen is zero for interface: %s\n",query_info->ifname);
                retry_count++;
                continue;
            }

            char dns_payload[BUFLEN_4096] = {0};
            unsigned int dns_payload_len = 0;

            if (use_raw_socket) {
                if (get_dns_payload(query_info->skt_family,reply+sizeof(struct ethhdr),
                            (recvlen - sizeof(struct ethhdr)),dns_payload,&dns_payload_len) < 0)
                {
                    WANCHK_LOG_ERROR("Unable to fetch dns payload for interface:%s\n",query_info->ifname);
                    current_time = clock_mono_ms();
                    timeout = timeout - (current_time - start_time);
                    goto wait_for_response;
                }
            } else {
                memcpy(dns_payload,reply,recvlen);
                dns_payload_len = recvlen;
            }

            BOOL match_found = FALSE;
            for (int j=0;j < query_info->query_count;j++)
            {
                WANCHK_LOG_DBG("dns payload %02hhx %02hhx\n", ((unsigned char *) dns_payload)[0],
                                                        ((unsigned char *)dns_payload)[1]);

                WANCHK_LOG_DBG("query_list %02hhx %02hhx\n", ((unsigned char *) query_list[j].query)[0],
                                                ((unsigned char *) query_list[j].query)[1]);

                if (memcmp(dns_payload,query_list[j].query,2) == 0) {
                   query_list[j].resp_recvd = 1;
                   no_of_replies++;
                   match_found = TRUE;
                } 
            }
            if (!match_found) {
                WANCHK_LOG_INFO("No matching response found yet,continue for interface: %s\n",
                                                                                query_info->ifname);
                current_time = clock_mono_ms();
                timeout = timeout - (current_time - start_time);
                goto wait_for_response;
            }

            rcode = dns_payload[3] & 0x0f;
            if (rcode != 0) {
                WANCHK_LOG_WARN("Unexpected response code:%d for interface: %s\n",rcode,
                                                                                query_info->ifname);
                ret = -1;
                /* ??? Retry or Abort, For now go for retry*/
                retry_count++;
                continue;
            } else {
                ret_parse = dns_parse(dns_payload, dns_payload_len,FALSE);
                switch (ret_parse) {
                case -1:
                    WANCHK_LOG_WARN("Can't find response for nameserver %s Parse error on interface: %s\n",
                                                                query_info->ns,query_info->ifname);
                    if (no_of_replies < query_info->query_count) {
                        WANCHK_LOG_INFO("Not yet received all replies %d for interface:%s\n",
                                                                no_of_replies,query_info->ifname);
                        current_time = clock_mono_ms();
                        timeout = timeout - (current_time - start_time);
                        goto wait_for_response;
                    } else {
                        WANCHK_LOG_ERROR("No valid response obtained for nameserver %s on interface: %s\n",
                                                                query_info->ns,query_info->ifname);
                        ret = -1;
                        goto EXIT;
                    }
                case IPV4_ONLY:
                     v4_resolved = TRUE;
                     break;
                case  IPV6_ONLY:
                     v6_resolved = TRUE;
                     break;
                case BOTH_IPV4_IPV6:
                     v4_resolved = TRUE;
                     v6_resolved = TRUE;
                     break;
                }
            }
            if (no_of_replies >= query_info->query_count)
              break;
        }
    } 
EXIT:
    close(pfd.fd);

    if ( ((query_info->rqstd_rectype == IPV4_ONLY) && v4_resolved) ||
         ((query_info->rqstd_rectype == IPV6_ONLY) && v6_resolved) ||
         ((query_info->rqstd_rectype == BOTH_IPV4_IPV6) && (v6_resolved && v4_resolved)) ||
         ((query_info->rqstd_rectype == EITHER_IPV4_IPV6) && (v6_resolved || v4_resolved)))
    {
        WANCHK_LOG_INFO("Query Response Succeeded on interface %s Requested RecordType:%d Status TYPE_A:%d TYPE_AAAA:%d\n",
                        query_info->ifname,query_info->rqstd_rectype,v4_resolved,v6_resolved);
    }
    else
    {
        WANCHK_LOG_INFO("Query Response Failed on interface %s Requested RecordType:%d Status TYPE_A:%d TYPE_AAAA:%d\n",
                        query_info->ifname,query_info->rqstd_rectype,v4_resolved,v6_resolved);
        ret = -1;
    }

     pthread_cleanup_pop(0);
     return ret;
}

int get_dns_payload(int family, char *payload,unsigned payload_len,
                                                    char *dns_payload,unsigned int *dns_payload_len)
{
    struct udphdr   *udp_header = NULL;
    char *tmp_dns_payload = NULL;
    unsigned int udp_header_start = 0;

    if (!payload || payload_len <= 0)
    {
        WANCHK_LOG_ERROR("Payload is NULL\n");
        return -1;
    }

    /* supported udp only for now for faster turn around and avoid fragmentation, 
    if payload len more than 512 bytes(udp dns accepted size) no need of processing*/
    if ( payload_len > 512)
    {
        WANCHK_LOG_DBG("No need of processing, Payload length larger\n");
        return -1;
    }
    if (family == AF_INET)
    {
        struct iphdr  *ipv4_header;
        ipv4_header = (struct iphdr *)payload;
        if (!ipv4_header || ipv4_header->version != 4 || ntohs(ipv4_header->tot_len) > payload_len)
        {
            WANCHK_LOG_DBG("Not a valid IPv4 packet\n");
            return -1;
        }

        if ((ipv4_header->protocol == IPPROTO_UDP))
        {
            udp_header = (struct udphdr *)(ipv4_header+(ipv4_header->ihl * 4));
            if (udp_header)
            {
                udp_header_start = (ipv4_header->ihl * 4);
            }
        }
    }
    else
    {
        struct ip6_hdr *ipv6_header;
        uint8_t *payload_tail = payload + payload_len;
        ipv6_header = (struct ip6_hdr *)payload;
        if (payload_len < sizeof(struct ip6_hdr) || ((*(uint8_t *)ipv6_header & 0xf0) != 0x60))
        {
            WANCHK_LOG_DBG("Not a valid IPv6 packet\n");
            return -1;
        }
        udp_header= (struct udphdr *)WanCnctvtyChk_get_transport_header(ipv6_header,IPPROTO_UDP,
                                                                                    payload_tail);
        if (udp_header)
        {   
            udp_header_start = (payload_len - (ntohs(ipv6_header->ip6_plen)));
        }
    }

    if (!udp_header || ((payload_len - udp_header_start) < sizeof(struct udphdr)))
    {
        WANCHK_LOG_DBG("Not a valid UDP DNS packet\n");
        return -1;
    }  
    else
    {
        WANCHK_LOG_DBG("udp_header_start :%d SPT=%u DPT=%u\n",udp_header_start,
                                                ntohs(udp_header->source), ntohs(udp_header->dest));
        tmp_dns_payload = (char *)(payload+udp_header_start+sizeof(struct udphdr));
        *dns_payload_len = (payload_len - (udp_header_start+sizeof(struct udphdr)));
        memcpy(dns_payload,tmp_dns_payload,*dns_payload_len);
    }

    return 0;
}
/* Receive handler for dns v4 packets*/

void wanchk_pckt_recv_v4(EV_P_ ev_io *ev, int revents)
{
    (void)loop;
    (void)revents;
    
    PWAN_CNCTVTY_CHK_PASSIVE_MONITOR self = ev->data;
    /* Ready to receive packets */
    char buf[4096];
    int rv;
    if ((rv = recv(self->nfq_fd_v4, buf, sizeof(buf), 0)) >= 0)
        nfq_handle_packet(self->nfqHandle_v4,buf, rv);
    else
        WANCHK_LOG_ERROR("%s: unable to handle incoming packet",__FUNCTION__);
}

/* Receive handler for dns v6 packets*/
void wanchk_pckt_recv_v6(EV_P_ ev_io *ev, int revents)
{   
    (void)loop;
    (void)revents;
    
    PWAN_CNCTVTY_CHK_PASSIVE_MONITOR self = ev->data;
    /* Ready to receive packets */
    char buf[4096];
    int rv;
    if ((rv = recv(self->nfq_fd_v6, buf, sizeof(buf), 0)) >= 0)
        nfq_handle_packet(self->nfqHandle_v6,buf, rv);
    else
        WANCHK_LOG_ERROR("%s: unable to handle incoming packet",__FUNCTION__);
}

/* Packet processing fucntion for dns v4 response packets*/
static int dns_response_v4_callback(struct nfq_q_handle *queueHandle, struct nfgenmsg *nfmsg, struct nfq_data *pkt, void *data)
{
    int id = 0x00;
    struct nfqnl_msg_packet_hdr *ph = nfq_get_msg_packet_hdr(pkt);
    char dns_payload[BUFLEN_4096] = {0};
    unsigned int dns_payload_len = 0;
    uint8_t rcode;
    int ret_parse = 0;
    char *payload = NULL;
    BOOL v4_resolved = FALSE;
    BOOL v6_resolved = FALSE;
    int len = nfq_get_payload(pkt, (unsigned char **)&payload);

    if (ph)
        id = ntohl(ph->packet_id);

    if (get_dns_payload(AF_INET,payload,len,dns_payload,&dns_payload_len) < 0) 
    {
        WANCHK_LOG_ERROR("Unable to fetch dns payload\n");
        return -1;
    }

    rcode = dns_payload[3] & 0x0f;
    WANCHK_LOG_INFO("response code:%d\n",rcode);
    if (rcode != 0) {
        WANCHK_LOG_WARN("Unexpected response code:%d,Skip\n",rcode);
        return -1;
    } else {
        ret_parse = dns_parse(dns_payload, dns_payload_len,FALSE);
        switch (ret_parse) {
        case -1:
            return -1;
        case IPV4_ONLY: 
            v4_resolved = TRUE;
            break;
        case  IPV6_ONLY: 
            v6_resolved = TRUE;
            break;
        case BOTH_IPV4_IPV6: 
                 v4_resolved = TRUE;
                 v6_resolved = TRUE;
             break;
        }
    }

    WANCHK_LOG_INFO("response detected from V4 server TYPE_A:%d TYPE_AAAA:%d\n",v4_resolved,v6_resolved);
    // To - Do update passive monitor status.
    return nfq_set_verdict(queueHandle, id, NF_ACCEPT, 0, NULL);
}

/* Packet processing fucntion for dns v6 response packets*/
static int dns_response_v6_callback(struct nfq_q_handle *queueHandle, struct nfgenmsg *nfmsg, struct nfq_data *pkt, void *data)
{
    int id = 0x00;
    struct nfqnl_msg_packet_hdr *ph = nfq_get_msg_packet_hdr(pkt);
    char dns_payload[BUFLEN_4096] = {0};
    unsigned int dns_payload_len = 0;
    uint8_t rcode;
    int ret_parse = 0;
    char *payload = NULL;
    BOOL v4_resolved = FALSE;
    BOOL v6_resolved = FALSE;
    int len = nfq_get_payload(pkt, (unsigned char **)&payload);

    if (ph)
        id = ntohl(ph->packet_id);

    if (get_dns_payload(AF_INET,payload,len,dns_payload,&dns_payload_len) < 0) 
    {
        WANCHK_LOG_ERROR("Unable to fetch dns payload\n");
        return -1;
    }

    rcode = dns_payload[3] & 0x0f;
    WANCHK_LOG_INFO("response code:%d\n",rcode);
    if (rcode != 0) {
        WANCHK_LOG_WARN("Unexpected response code:%d,Skip\n",rcode);
        return -1;
    } else {
        ret_parse = dns_parse(dns_payload, dns_payload_len,FALSE);
        switch (ret_parse) {
        case -1:
            return -1;
        case IPV4_ONLY: 
            v4_resolved = TRUE;
            break;
        case  IPV6_ONLY: 
            v6_resolved = TRUE;
            break;
        case BOTH_IPV4_IPV6: 
                 v4_resolved = TRUE;
                 v6_resolved = TRUE;
             break;
        }
    }

    WANCHK_LOG_INFO("response detected from V6 server TYPE_A:%d TYPE_AAAA:%d\n",v4_resolved,v6_resolved);

    return nfq_set_verdict(queueHandle, id, NF_ACCEPT, 0, NULL);
}

int wancnctvty_chk_create_nfq (u_int16_t family, PWAN_CNCTVTY_CHK_PASSIVE_MONITOR pPassive)
{

    struct nfq_handle *nfqHandle = NULL;
    struct nfq_q_handle *queueHandle = NULL;
    int fd;
    int queue_num = (family == AF_INET)? WANCHK_NFQUEUE_V4 : WANCHK_NFQUEUE_V6;

    nfqHandle = nfq_open();
    if (!nfqHandle) {
        WANCHK_LOG_ERROR("%s: nfq open failed\n", __FUNCTION__);
        return -1;
    }

    WANCHK_LOG_INFO("unbinding existing nf_queue handler for %s (if any)\n", family == AF_INET ? "AF_INET" : "AF_INET6");
    if (nfq_unbind_pf(nfqHandle, family) < 0) {
        WANCHK_LOG_ERROR("nfq_handler: error during nfq_unbind_pf");
        return -1;
    }

    WANCHK_LOG_INFO("binding nfnetlink_queue as nf_queue handler for %s\n", family == AF_INET ? "AF_INET" : "AF_INET6");
    if (nfq_bind_pf(nfqHandle, family) < 0) {
        WANCHK_LOG_ERROR("nfq_handler: error during nfq_bind_pf");
        return -1;
    }

    WANCHK_LOG_INFO("binding this socket to queue 31\n");
    if (family == AF_INET)
        queueHandle = nfq_create_queue(nfqHandle, queue_num, &dns_response_v4_callback , NULL);
    else
        queueHandle = nfq_create_queue(nfqHandle, queue_num, &dns_response_v6_callback , NULL);

    if (!queueHandle) {
        WANCHK_LOG_ERROR("nfq_handler: error during nfq_create_queue\n");
        return -1;
    }

    WANCHK_LOG_INFO("setting copy_packet mode\n");
    if (nfq_set_mode(queueHandle, NFQNL_COPY_PACKET, 0xffff) < 0) {
        WANCHK_LOG_ERROR("can't set packet_copy mode\n");
        return -1;
    }

    fd = nfq_fd(nfqHandle);

    if ( family == AF_INET) {
        pPassive->nfqHandle_v4 = nfqHandle;
        pPassive->nfq_fd_v4  = fd;
    }
    else {
        pPassive->nfqHandle_v6 = nfqHandle;
        pPassive->nfq_fd_v6 = fd;
    }

    return 0;
}

/*Uses code from nslookup_lede
Copyright (C) 2017 Jo-Philipp Wich <jo@mein.io>
Licensed under the ISC License
*/
int dns_parse(const unsigned char *msg, size_t len,bool restart_timer)
{
    ns_msg handle;
    ns_rr rr;
    int i,rdlen;
    char astr[INET6_ADDRSTRLEN]={0};
    int restart=0;
    BOOL ns_t_a_found = FALSE;
    BOOL ns_t_aaaa_found = FALSE;

    if (ns_initparse(msg, len, &handle) != 0) {
        WANCHK_LOG_WARN("Unable to parse dns payload\n");
        return -1;
    }

    if(ns_msg_getflag(handle, ns_f_qr) == 0)
    {
        WANCHK_LOG_WARN("Skip parsing dns query\n");
        return 0;
    }

    if(ns_msg_getflag(handle, ns_f_rcode) != ns_r_noerror)
    {
       WANCHK_LOG_WARN("got error response code in dns response:%d\n",ns_msg_getflag(handle, ns_f_rcode));
       return -1;
    }

    for (i = 0; i < ns_msg_count(handle, ns_s_an); i++)
    {
        if (ns_parserr(&handle, ns_s_an, i, &rr) != 0) {
            WANCHK_LOG_ERROR("Unable to parse resource record: %s\n", strerror(errno));
            return -1;
        }

        rdlen = ns_rr_rdlen(rr);

        switch (ns_rr_type(rr))
        {
           case ns_t_a:
                if (rdlen != 4) {
                   WANCHK_LOG_ERROR("unexpected A record length %d\n", rdlen);
                   return -1;
                }
                inet_ntop(AF_INET, ns_rr_rdata(rr), astr, sizeof(astr));
                WANCHK_LOG_INFO("Name:\t%s\tAddress: %s\n", ns_rr_name(rr), astr);
                ns_t_a_found = TRUE;
                break;

           case ns_t_aaaa:
                if (rdlen != 16) {
                    WANCHK_LOG_ERROR("unexpected AAAA record length %d\n", rdlen);
                    return -1;
                }
                inet_ntop(AF_INET6, ns_rr_rdata(rr), astr, sizeof(astr));
                WANCHK_LOG_INFO("Name:\t%s\tAddress: %s\n", ns_rr_name(rr), astr);
                ns_t_aaaa_found = TRUE;
                break;

           default:
                break;
        }
     }

     if(restart && restart_timer)
     {
       WANCHK_LOG_INFO("we have to restart the timer\n");
       //ev_timer_again(wanchk_ptr->loop,&wanchk_ptr->bgtimer);
     }

     if ( ns_t_a_found && ns_t_aaaa_found)
     {
        return BOTH_IPV4_IPV6;
     }
     else if ( ns_t_a_found && ! ns_t_aaaa_found)
     {
        return IPV4_ONLY;
     }
     else if (ns_t_aaaa_found && !ns_t_a_found)
     {
        return IPV6_ONLY;
     }
     else
     {
        return -1;
     }

     return 0;
}

/* Fetch the url list from the global object.

Note : This will allocate memory for the list, make sure
to remove after use*/

char** get_url_list(int *total_url_entries)
{

    int total_entries = 0;
    errno_t rc = -1;
    char **url_list = NULL;
    int count = 0;

    PSINGLE_LINK_ENTRY              pSListEntry       = NULL;
    PCOSA_CONTEXT_LINK_OBJECT       pCxtLink          = NULL;
    PCOSA_DML_WANCNCTVTY_CHK_URL_INFO pUrlInfo        = NULL;

    pthread_mutex_lock(&gUrlAccessMutex);
    total_entries     = AnscSListQueryDepth(&gpUrlList);
    if (!total_entries)
    {
        WANCHK_LOG_ERROR("Url list is empty\n");
        pthread_mutex_unlock(&gUrlAccessMutex);
        return NULL;
    }
    //CosaWanCnctvtyChk_Urllist_dump();
    if (total_entries > 0)
    {
        url_list = (char **)calloc(total_entries,sizeof(char *));
        pSListEntry           = AnscSListGetFirstEntry(&gpUrlList);
        while( pSListEntry != NULL)
        {
            pCxtLink          = ACCESS_COSA_CONTEXT_LINK_OBJECT(pSListEntry);
            pSListEntry       = AnscSListGetNextEntry(pSListEntry);
            pUrlInfo = (PCOSA_DML_WANCNCTVTY_CHK_URL_INFO)pCxtLink->hContext;
            if (pUrlInfo && strlen(pUrlInfo->URL))
            {
                url_list[count] = (char *)calloc(1,MAX_URL_SIZE);
                rc = strcpy_s(url_list[count], MAX_URL_SIZE ,pUrlInfo->URL);
                ERR_CHK(rc);
                WANCHK_LOG_DBG("Got URL %s to execute query now\n",url_list[count]);
                count++;
            }
        }
    }
    pthread_mutex_unlock(&gUrlAccessMutex);
    *total_url_entries = count;
    WANCHK_LOG_DBG("Number of url entries :%d\n",*total_url_entries);
    return url_list;
}
/* To get ipv4 address associated with interface*/

static int get_ipv4_addr(const char *ifName, char *ipv4Addr)
{   
    int fd;
    struct ifreq ifrq;
    char *tmpStr;
    errno_t rc = -1;
    
    if(ifName == NULL || ipv4Addr == NULL)
        return -1;
    
    /* get socket descriptor */
    fd = socket(PF_INET, SOCK_DGRAM, 0);
    if(fd == -1)
    {   
        fprintf(stderr, "%s - create socket error", __FUNCTION__);
        return -1;
    }
    
    rc = strcpy_s(ifrq.ifr_name, sizeof(ifrq.ifr_name), ifName);
    ERR_CHK(rc);

    if (ioctl(fd, SIOCGIFADDR, &ifrq) < 0)
    {   
        close(fd);
        return -1;
    }
    else
    {  
       tmpStr = inet_ntoa(((struct sockaddr_in *)(&(ifrq.ifr_addr)))->sin_addr);
       strncpy(ipv4Addr, tmpStr, 16);
    }
    
    close(fd);
    return 0;
}

static int get_mac_addr(char* ifName, char* mac)
{

    int skfd = -1;
    struct ifreq ifr;

    AnscCopyString(ifr.ifr_name, ifName);
    
    skfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(skfd == -1)
       return -1;

    if (ioctl(skfd, SIOCGIFHWADDR, &ifr) < 0) {
        close(skfd);
        return -1;
    }
    close(skfd);
    snprintf(mac, MACADDR_SZ, "%.2x:%.2x:%.2x:%.2x:%.2x:%.2x",
          (unsigned char)ifr.ifr_hwaddr.sa_data[0],
          (unsigned char)ifr.ifr_hwaddr.sa_data[1],
          (unsigned char)ifr.ifr_hwaddr.sa_data[2],
          (unsigned char)ifr.ifr_hwaddr.sa_data[3],
          (unsigned char)ifr.ifr_hwaddr.sa_data[4],
          (unsigned char)ifr.ifr_hwaddr.sa_data[5]);
    return 0; 
}

/* To get global ipv6 attached to interface*/

static int get_ipv6_addr(const char *ifName, char *ipv6Addr)
{
    struct ifaddrs *ifaddr, *ifa;
    int family, n;
    int ipv6AddrExist = 0;

    if (!ifName || !ipv6Addr)
        return -1;

    if (getifaddrs(&ifaddr) == -1) {
        fprintf(stderr, "%s - getifaddrs error", __FUNCTION__);
        return -1;
    }

    for (ifa = ifaddr, n = 0; ifa != NULL; ifa = ifa->ifa_next, n++)
    {
        if (ifa->ifa_addr == NULL)
            continue;

        family = ifa->ifa_addr->sa_family;
        if ((family == AF_INET6) && (!strcmp(ifName, ifa->ifa_name)))
        {
            if (inet_ntop(AF_INET6, &((struct sockaddr_in6 *)(ifa->ifa_addr))->sin6_addr, ipv6Addr, 46)) {
                if(strncmp("fe80", ipv6Addr, 4) != 0)
                {
                        ipv6AddrExist = 1;
                        break;
                }
            }
        }
    }

    freeifaddrs(ifaddr);
    if (ipv6AddrExist)
        return 0;
    else
        return -1;
}

int get_record_type(char *recordtype_string,recordtype_t *output)
{
    BOOL both_v4_v6 = FALSE;
    BOOL either_v4_v6 = FALSE;
    BOOL v4_present = FALSE;
    BOOL v6_present = FALSE;
    errno_t rc = -1;
    int ind = -1;
    char tmpStr[MAX_RECORD_TYPE_SIZE] = {0};

    if (!recordtype_string)
    {
        WANCHK_LOG_ERROR("recordtype_string is NULL\n");
        return -1;
    }
    else if (recordtype_string[0] == '\0')
    {
        WANCHK_LOG_ERROR("recordtype_string is empty\n");
        return -1;
    }

    rc = strcpy_s(tmpStr, MAX_RECORD_TYPE_SIZE , recordtype_string);
    ERR_CHK(rc);

    if (strstr(tmpStr,"*"))
        both_v4_v6 = TRUE;
    else if (strstr(tmpStr,"+"))
        either_v4_v6 = TRUE;

    char *delimiter = (both_v4_v6)? "*" : "+";

    char* tok;

    tok = strtok(tmpStr, delimiter);

    // Checks for delimiter
    while (tok != 0) {
        WANCHK_LOG_DBG("got record =%s\n", tok);
        rc = strcmp_s( "A",strlen("A"),tok, &ind);
        ERR_CHK(rc);
        if((!ind) && (rc == EOK))
        {
            v4_present = TRUE;
        }

        rc = strcmp_s( "AAAA",strlen("AAAA"),tok, &ind);
        ERR_CHK(rc);
        if((!ind) && (rc == EOK))
        {
            v6_present = TRUE;
        }
        tok = strtok(0, delimiter);
    }

    if ( v4_present && v6_present && both_v4_v6)
        *output = BOTH_IPV4_IPV6;
    else if ( v4_present && v6_present && either_v4_v6)
        *output = EITHER_IPV4_IPV6;
    else if ( v4_present && !both_v4_v6 && !either_v4_v6)
        *output = IPV4_ONLY;
    else if ( v6_present && !both_v4_v6 && !either_v4_v6)
        *output = IPV6_ONLY;
    else
    {
        WANCHK_LOG_ERROR("%s : Unknown record type:%s",__FUNCTION__,recordtype_string);
        return -1;
    }
    
    return 0;
}

int get_server_type(char *servertype_string,servertype_t *output)
{
    BOOL both_v4_v6 = FALSE;
    BOOL either_v4_v6 = FALSE;
    BOOL v4_present = FALSE;
    BOOL v6_present = FALSE;
    errno_t rc = -1;
    int ind = -1;
    char tmpStr[MAX_SERVER_TYPE_SIZE] = {0};

    if (!servertype_string)
    {
        WANCHK_LOG_ERROR("servertype_string is NULL\n");
        return -1;
    }
    else if (servertype_string[0] == '\0')
    {
        WANCHK_LOG_ERROR("servertype_string is empty\n");
        return -1;
    }

    rc = strcpy_s(tmpStr, MAX_SERVER_TYPE_SIZE , servertype_string);
    ERR_CHK(rc);
    
    if (strstr(tmpStr,"*"))
        both_v4_v6 = TRUE;
    else if (strstr(tmpStr,"+"))
        either_v4_v6 = TRUE;

    char *delimiter = (both_v4_v6)? "*" : "+";

    char* tok;

    tok = strtok(tmpStr, delimiter);

    // Checks for delimiter
    while (tok != 0) {
        WANCHK_LOG_DBG("got server type =%s\n", tok);
        rc = strcmp_s( "IPv4",strlen("IPv4"),tok, &ind);
        ERR_CHK(rc);
        if((!ind) && (rc == EOK))
        {
            v4_present = TRUE;
        }

        rc = strcmp_s( "IPv6",strlen("IPv6"),tok, &ind);
        ERR_CHK(rc);
        if((!ind) && (rc == EOK))
        {
            v6_present = TRUE;
        }
        tok = strtok(0, delimiter);
    }

    if ( v4_present && v6_present && both_v4_v6)
        *output = SRVR_BOTH_IPV4_IPV6;
    else if ( v4_present && v6_present && either_v4_v6)
        *output = SRVR_EITHER_IPV4_IPV6;
    else if ( v4_present && !both_v4_v6 && !either_v4_v6)
        *output = SRVR_IPV4_ONLY;
    else if ( v6_present && !both_v4_v6 && !either_v4_v6)
        *output = SRVR_IPV6_ONLY;
    else
    {
        WANCHK_LOG_ERROR("%s : Unknown server type:%s",__FUNCTION__,servertype_string);
        return -1;
    }
    
    return 0;
}


ANSC_STATUS wancnctvty_chk_querynow_result_update(ULONG InstanceNumber,querynow_result_t result)
{
    ULONG previous_result = 0;
    char rowName[RBUS_MAX_NAME_LENGTH] = {0};
    BOOL send_publish_event = FALSE;
    pthread_mutex_lock(&gIntfAccessMutex);
    PWANCNCTVTY_CHK_GLOBAL_INTF_INFO gIntfInfo = &gInterface_List[InstanceNumber-1];
    previous_result = gIntfInfo->IPInterface.QueryNowResult;
    gIntfInfo->IPInterface.QueryNowResult  = result;
    if (previous_result != gIntfInfo->IPInterface.QueryNowResult)
    {
        WANCHK_LOG_INFO("Updated QueryNowResult for Interface %s: %ld->%ld\n",
                                                        gIntfInfo->IPInterface.InterfaceName,
                                                        previous_result,
                                                        gIntfInfo->IPInterface.QueryNowResult);
        if (gIntfInfo->IPInterface.QueryNowResult_SubsCount)
            send_publish_event = TRUE;
    }
    pthread_mutex_unlock(&gIntfAccessMutex);
    if (send_publish_event)
    {
        snprintf(rowName, RBUS_MAX_NAME_LENGTH, 
            "Device.Diagnostics.X_RDK_DNSInternet.WANInterface.%ld.QueryNowResult",InstanceNumber);
        WANCNCTVTYCHK_PublishEvent(rowName,result,previous_result);
    }

    return ANSC_STATUS_SUCCESS;
}

ANSC_STATUS wancnctvty_chk_monitor_result_update(ULONG InstanceNumber,monitor_result_t result)
{
    ULONG previous_result = 0;
    char rowName[RBUS_MAX_NAME_LENGTH] = {0};
    BOOL send_publish_event = FALSE;
    pthread_mutex_lock(&gIntfAccessMutex);
    PWANCNCTVTY_CHK_GLOBAL_INTF_INFO gIntfInfo = &gInterface_List[InstanceNumber-1];
    previous_result = gIntfInfo->IPInterface.MonitorResult;
    gIntfInfo->IPInterface.MonitorResult  = result;
    if (previous_result != gIntfInfo->IPInterface.MonitorResult)
    {

        WANCHK_LOG_INFO("Updated MonitorResult for Interface %s:%ld->%ld\n",
                                        gIntfInfo->IPInterface.InterfaceName,previous_result,
                                        gIntfInfo->IPInterface.MonitorResult);
        if (gIntfInfo->IPInterface.MonitorResult_SubsCount)
            send_publish_event = TRUE;

    }
    pthread_mutex_unlock(&gIntfAccessMutex);
    if (send_publish_event)
    {
        snprintf(rowName, RBUS_MAX_NAME_LENGTH, 
                "Device.Diagnostics.X_RDK_DNSInternet.WANInterface.%ld.MonitorResult",InstanceNumber);
        WANCNCTVTYCHK_PublishEvent(rowName,result,previous_result);
    }

    return ANSC_STATUS_SUCCESS;
}

static void _get_shell_output (FILE *fp, char *buf, size_t len)
{
    if (len > 0)
          buf[0] = 0;
      if (fp == NULL)
          return;
      buf = fgets (buf, len, fp);
      if ((len > 0) && (buf != NULL)) {
          len = strlen (buf);
          if ((len > 0) && (buf[len - 1] == '\n'))
              buf[len - 1] = 0;
      }
}

static int64_t clock_mono_ms()
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (int64_t)ts.tv_sec * 1000LL + ts.tv_nsec / 1000000LL;
}


static int socket_dns_filter_attach(int fd,struct sock_filter *filter, int filter_len)
{
     int err;
     struct sock_fprog fprog;
     fprog.len = filter_len;
     fprog.filter = filter;
     err = setsockopt(fd, SOL_SOCKET, SO_ATTACH_FILTER, &fprog, sizeof(fprog));
     if (err < 0)
     {
        WANCHK_LOG_ERROR("setsockop-SOL_SOCKET-SO_ATTACH_FILTER");
     }
     return err;
}

bool validatemac(char *mac) {
    uint32_t bytes[6] = {0};

    if (mac == NULL)
        return false;
    if (strlen(mac) != 17) return false;
    return (6 == sscanf(mac, "%02X:%02X:%02X:%02X:%02X:%02X", &bytes[5], &bytes[4], &bytes[3], &bytes[2], &bytes[1],
                        &bytes[0]));
}
