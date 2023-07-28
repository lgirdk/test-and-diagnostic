/*
 * If not stated otherwise in this file or this component's Licenses.txt file the
 * following copyright and licenses apply:
 *
 * Copyright 2017 RDK Management
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

#include "syscfg/syscfg.h"
#include "secure_wrapper.h"
#include "safec_lib_common.h"
#include "cosa_wanconnectivity_apis.h"
#include "ansc_status.h"
#include <rbus/rbus.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <netinet/in.h>
#include <netinet/udp.h>
#include <netinet/ether.h>
#include <libnetfilter_queue/libnetfilter_queue.h>
#include <libnetfilter_queue/libnetfilter_queue_ipv4.h>
#include <libnetfilter_queue/libnetfilter_queue_ipv6.h>
#include <libnetfilter_queue/libnetfilter_queue_udp.h>
#include <libnetfilter_queue/pktbuff.h>
#include <linux/netfilter.h>
#include <linux/if_ether.h>

extern rbusHandle_t rbus_handle;

void WanCnctvtyChk_CreateEthernetHeader (struct ethhdr *ethernet_header,char *src_mac, char *dst_mac, int protocol);
void WanCnctvtyChk_CreateIPHeader (int family, char *srcIp, char *dstIp, unsigned int dataSize,void *);
void WanCnctvtyChk_CreateUdpHeader(int family, unsigned short sport, unsigned short dport,
                                                                      unsigned int dataSize,struct udphdr *);
void WanCnctvtyChk_CreatePseudoHeaderAndComputeUdpChecksum (int family, struct udphdr *udp_header, void *ip_header,
                                                        unsigned char *data, unsigned int dataSize);
unsigned short WanCnctvtyChk_udp_checksum (unsigned short *buffer, int byte_count);
static unsigned short ComputeChecksum (void *data, unsigned long length);
uint8_t *WanCnctvtyChk_get_transport_header(struct ip6_hdr *ip6h,uint8_t target,uint8_t *payload_tail);

static int CHECK_V4_V6(char *str);

typedef struct PseudoHeader {
    u_int32_t source_ip;
    u_int32_t dest_ip;
    u_int8_t reserved;
    u_int8_t protocol;
    u_int16_t length;
} PseudoHeader;

typedef struct PseudoHeaderv6 {
    u_int8_t source_ipv6[16];
    u_int8_t dest_ipv6[16];
    u_int32_t up_len;
    u_int8_t reserved[3];
    u_int8_t next_hdr;
} PseudoHeaderv6;

/* Get parameter value API */
ANSC_STATUS WanCnctvtyChk_GetParameterValue(  const char *pParamName, char *pReturnVal)
{
    int                    ret = 0;
    rbusValue_t            value;
    rbusValueType_t        rbusValueType ;
    char                   *pStrVal            = NULL;

    /* rbus get parameter value */
    if(rbus_handle == NULL)
    {
        return ANSC_STATUS_FAILURE;
    }

    WANCHK_LOG_DBG("%s Rbus Invoke\n",__FUNCTION__);
    /* Init rbus variable */
    rbusValue_Init(&value);

    /* Get the value of a single parameter */
    ret = rbus_get(rbus_handle, pParamName, &value);

    if(ret != RBUS_ERROR_SUCCESS )
    {
        WANCHK_LOG_ERROR("%s-%d Rbus Error code:%d\n",__FUNCTION__,__LINE__, ret);
        return ANSC_STATUS_FAILURE;
    }

    rbusValueType = rbusValue_GetType(value);

    /* Update the parameter value */
    if(rbusValueType == RBUS_BOOLEAN)
    {
        if (rbusValue_GetBoolean(value)){
            pStrVal= "true";
        } else {
            pStrVal = "false";
        }
        strncpy( pReturnVal, pStrVal, strlen( pStrVal ) + 1 );
    }
    else
    {
        pStrVal = rbusValue_ToString(value, NULL, 0);
        if (pStrVal)
        {
            strncpy( pReturnVal, pStrVal, strlen( pStrVal ) + 1 );
            free(pStrVal);
            pStrVal = NULL;
        }
    }

    /* release rbus variable */
    rbusValue_Release(value);
    return ANSC_STATUS_SUCCESS;
}

ANSC_STATUS WanCnctvtyChk_GetPrimaryDNS_Entry(PCOSA_DML_WANCNCTVTY_CHK_DNSSRV_INFO pDnsSrvInfo)
{
    int                    ret = 0;
    int numOfInputParams   = 1;
    int numOfOutVals = 0;
    const char *pInputParam = "Device.DNS.Client.Server.";
    rbusProperty_t outputVals = NULL;
    errno_t rc = -1;
    int ind = -1;
    char eventName[128] = {0};
    dns_entrytype_t dns_family;
    int dns_idx =0;

    /* rbus get parameter value */
    if(rbus_handle == NULL)
    {
        return ANSC_STATUS_FAILURE;
    }

    if (!pDnsSrvInfo)
    {
        WANCHK_LOG_ERROR("pDnsSrvInfo is NULL");
        return ANSC_STATUS_FAILURE;
    }

    /* Get the value of a tree*/
    ret = rbus_getExt(rbus_handle, numOfInputParams, &pInputParam, &numOfOutVals, &outputVals);

    if(ret != RBUS_ERROR_SUCCESS )
    {
        WANCHK_LOG_ERROR("%s-%d Rbus Error code:'%d'",__FUNCTION__,__LINE__, ret);
        return ANSC_STATUS_FAILURE;
    }
    else if (ret == RBUS_ERROR_SUCCESS)
    {
        rbusProperty_t next = outputVals;
        for (int i = 0; i < numOfOutVals; i++)
        {
            rbusValue_t val = rbusProperty_GetValue(next);
            char *pStrVal = rbusValue_ToString(val,NULL,0);

            WANCHK_LOG_DBG("Parameter %2d:\n\r", i+1);
            WANCHK_LOG_DBG("              Name  : %s\n\r", rbusProperty_GetName(next));
            WANCHK_LOG_DBG("              Value : %s\n\r", pStrVal);

            memset(eventName,0,sizeof(eventName));
            rc = sprintf_s(eventName,sizeof(eventName),"Device.DNS.Client.Server.%d.DNSServer",
                                                                                    (dns_idx+1));
            if (rc < EOK)
            {
                ERR_CHK(rc);
            }

            rc = strcmp_s(eventName, strlen(eventName),rbusProperty_GetName(next),&ind);
            if ((rc == EOK) && (!ind))
            {
               /*Lets store in dns srv data*/
               /* eventhough in DNS code, we are setting the type only v4 or v6, but enum still has
               other options e.g RA etc. not sure may be later improvement may break us. so 
               sticking the check based on  address*/
               dns_family = CHECK_V4_V6(pStrVal);
               if (dns_family == DNS_SRV_IPV4)
               {
                   memset(pDnsSrvInfo[dns_idx].IPAddress.IPv4Address,0,IPv4_STR_LEN);
                   rc = strcpy_s(pDnsSrvInfo[dns_idx].IPAddress.IPv4Address,IPv4_STR_LEN,pStrVal);
                   ERR_CHK(rc);
                   pDnsSrvInfo[dns_idx].dns_type = dns_family;
               }
               else if (dns_family == DNS_SRV_IPV6)
               {
                   memset(pDnsSrvInfo[dns_idx].IPAddress.IPv6Address,0,IPv6_STR_LEN);
                   rc = strcpy_s(pDnsSrvInfo[dns_idx].IPAddress.IPv6Address,IPv6_STR_LEN,pStrVal);
                   ERR_CHK(rc);
                   pDnsSrvInfo[dns_idx].dns_type = dns_family;
               }
               dns_idx++;
            }

            if(pStrVal)
                free(pStrVal);

            next = rbusProperty_GetNext(next);
        }
        rbusProperty_Release(outputVals);
    }
    return ANSC_STATUS_SUCCESS;
}

static int CHECK_V4_V6(char *str){
    if( NULL != strchr(str, ':'))
        return DNS_SRV_IPV6;
    if( NULL != strchr(str, '.'))
        return DNS_SRV_IPV4;

    return DNS_SRV_NONE;
}

/* Get parameter value API */
ANSC_STATUS WanCnctvtyChk_GetRemoteParameterValue( char *mac_addr, 
                                                        const char *pParamName, char *pReturnVal)
{
    /* rbus get parameter value */
    if(rbus_handle == NULL)
    {
        return ANSC_STATUS_FAILURE;
    }

    rbusObject_t outParams = NULL;
    rbusValue_t value = NULL;
    rbusObject_Init(&outParams, NULL);
    rbusValueType_t        rbusValueType ;
    char *pStrVal = NULL;
    idm_invoke_method_Params_t IDM_request;

    memset(&IDM_request,0, sizeof(idm_invoke_method_Params_t));

    /* Update request parameters */
    strncpy(IDM_request.Mac_dest, mac_addr, sizeof(IDM_request.Mac_dest)-1);
    strncpy(IDM_request.param_name, pParamName, sizeof(IDM_request.param_name)-1);
    IDM_request.timeout = 20;
    IDM_request.operation = IDM_GET;

    if (ANSC_STATUS_SUCCESS != WanCnctvtyChk_IDM_Invoke(&IDM_request, FALSE, &outParams))
    {
        WANCHK_LOG_ERROR("%s : failed for %s.\n", __FUNCTION__, pParamName);
        rbusObject_Release(outParams);
        return ANSC_STATUS_FAILURE;
    }

    value = rbusObject_GetValue(outParams, "param_value");
    rbusValueType = rbusValue_GetType(value);
    /* Update the parameter value */
    if(rbusValueType == RBUS_BOOLEAN)
    {
	/* CID 305987 fix */
        if (rbusValue_GetBoolean(value))
        {
            pStrVal= strdup("true");
        } else {
            pStrVal = strdup("false");
        }
    }
    else
    {
        pStrVal = rbusValue_ToString(value, NULL, 0);
    }
    if (pStrVal)
    {
        strncpy( pReturnVal, pStrVal, strlen( pStrVal ) + 1 );
        free(pStrVal);
        pStrVal = NULL;
    }

    rbusObject_Release(outParams);

    WANCHK_LOG_INFO("%s %d - paramname=%s value=%s\n", __FUNCTION__, __LINE__,
                                                                            pParamName,pReturnVal);
    return ANSC_STATUS_SUCCESS;
}

/* Invoke IDM to get values from Remote Device*/

ANSC_STATUS WanCnctvtyChk_IDM_Invoke(idm_invoke_method_Params_t *IDM_request,BOOL Invoke_Async,
                                                                        rbusObject_t *outParams)
{
    rbusObject_t inParams;
    rbusValue_t value;
    int rc = RBUS_ERROR_SUCCESS;

    WANCHK_LOG_INFO("%s %d - sendind rbus request to %s \n", __FUNCTION__, __LINE__,
                                                                            IDM_request->Mac_dest);

    rbusObject_Init(&inParams, NULL);

    rbusValue_Init(&value);
    rbusValue_SetString(value, IDM_request->Mac_dest );
    rbusObject_SetValue(inParams, "DEST_MAC_ADDR", value);
    rbusValue_Release(value);

    rbusValue_Init(&value);
    rbusValue_SetString(value, IDM_request->param_name);
    rbusObject_SetValue(inParams, "paramName", value);
    rbusValue_Release(value);

    rbusValue_Init(&value);
    rbusValue_SetString(value,IDM_request->param_value);
    rbusObject_SetValue(inParams, "paramValue", value);
    rbusValue_Release(value);

    rbusValue_Init(&value);
    rbusValue_SetInt32(value, IDM_request->timeout);
    rbusObject_SetValue(inParams, "Timeout", value);
    rbusValue_Release(value);

    rbusValue_Init(&value);
    rbusValue_SetInt32(value, IDM_request->type);
    rbusObject_SetValue(inParams, "DataType", value);
    rbusValue_Release(value);

    rbusValue_Init(&value);
    rbusValue_SetInt32(value, IDM_request->operation);
    rbusObject_SetValue(inParams, "Operation", value);
    rbusValue_Release(value);

    // To -Do  check do we have to use Async
    if (Invoke_Async)
    {
        rc = rbusMethod_InvokeAsync(rbus_handle, X_RDK_REMOTE_INVOKE, inParams,
                                                WanCnctvtyChk_IDM_AsyncMethodHandler,
                                                IDM_request->timeout);
    }
    else
    {
        rc = rbusMethod_Invoke(rbus_handle, X_RDK_REMOTE_INVOKE, inParams, outParams);
    }

    if(rc == RBUS_ERROR_SUCCESS)
    {
        WANCHK_LOG_INFO("%s %d -consumer: rbusMethod_Invoke(Device.X_RDK_Remote.Invoke()) success for param %s\n",
                                                                __FUNCTION__, __LINE__,IDM_request->param_name);
        rbusObject_Release(inParams);
        return ANSC_STATUS_SUCCESS;
    }
    else
    {
        WANCHK_LOG_ERROR("%s %d -consumer: rbusMethod_Invoke(Device.X_RDK_Remote.Invoke()) failure for param %s\n"
                                                                ,__FUNCTION__, __LINE__,IDM_request->param_name);
        rbusObject_Release(inParams);
        return ANSC_STATUS_FAILURE;    
    }
}

ANSC_STATUS WanCnctvtyChk_GetRemote_MacAddress(char *mac_addr)
{
    /* get remote device MAC address */
    char dmQuery[BUFLEN_256] = {0};
    char macValue[BUFLEN_256] = {0};
    char capabilties[BUFLEN_1024] = {0};
    char buf[BUFLEN_8] = {0};
    int remote_status = 0;
    int number_of_remote_entries = 0;
    int remote_index = 2;
    BOOL found = FALSE;

    /* Assumptions or logic to fetch remote mac address. Eventhough Device.X_RDK_Remote. can hold
    multiple entries, but more than one device as standby is for future enhancements. currenlty it 
    will be one device. but there can be other devices in the table without modem capabilities.
    so first device with modem capability and status as connected, while be our mac address(remote
    device)*/

    snprintf(dmQuery, sizeof(dmQuery)-1, "Device.X_RDK_Remote.DeviceNumberOfEntries");
    if ( ANSC_STATUS_SUCCESS != WanCnctvtyChk_GetParameterValue(dmQuery, buf))
    {
        WANCHK_LOG_ERROR("%s-%d: %s, Failed to get param value\n", __FUNCTION__, __LINE__, dmQuery);
        return ANSC_STATUS_FAILURE;
    }
    else
    {
        number_of_remote_entries = atoi(buf);
    }

    WANCHK_LOG_DBG("%s %d - number_of_remote_entries=%d\n", __FUNCTION__, __LINE__,
                                                                        number_of_remote_entries);

    /* if have remote device, we are expecting number of entries should be more than one*/
    if (number_of_remote_entries > 1)
    {
        for (remote_index=2; remote_index <= number_of_remote_entries;remote_index++)
        {
            snprintf(dmQuery, sizeof(dmQuery)-1, "Device.X_RDK_Remote.Device.%d.Capabilities",
                                                                                    remote_index);
            if ( ANSC_STATUS_SUCCESS != WanCnctvtyChk_GetParameterValue(dmQuery, capabilties))
            {
                WANCHK_LOG_ERROR("%s-%d: %s, Failed to get param value\n", __FUNCTION__, __LINE__, 
                                                                                        dmQuery);
                return ANSC_STATUS_FAILURE;
            }

            WANCHK_LOG_DBG("%s %d - Capabilities=%s\n", __FUNCTION__, __LINE__,capabilties);

            if (strstr(capabilties,"Modem"))
            {
                memset(dmQuery,0,sizeof(dmQuery));
                memset(buf,0,sizeof(buf));
                snprintf(dmQuery, sizeof(dmQuery)-1, "Device.X_RDK_Remote.Device.%d.Status",
                                                                                    remote_index);
                if ( ANSC_STATUS_SUCCESS != WanCnctvtyChk_GetParameterValue(dmQuery, buf))
                {
                    WANCHK_LOG_ERROR("%s-%d: %s, Failed to get param value\n", __FUNCTION__, 
                                                                                __LINE__, dmQuery);
                    return ANSC_STATUS_FAILURE;
                }
                else
                {
                    remote_status = atoi(buf);
                }

                if (3 == remote_status)
                {
                    memset(dmQuery,0,sizeof(dmQuery));
                    snprintf(dmQuery, sizeof(dmQuery)-1, "Device.X_RDK_Remote.Device.%d.MAC",
                                                                                    remote_index);  
                    if ( ANSC_STATUS_SUCCESS != WanCnctvtyChk_GetParameterValue(dmQuery, macValue))
                    {
                        WANCHK_LOG_ERROR("%s-%d: %s, Failed to get param value\n", __FUNCTION__, 
                                                                                __LINE__, dmQuery);
                        return ANSC_STATUS_FAILURE;
                    }
                    else
                    {
                        found = TRUE;
                        strncpy( mac_addr, macValue, strlen( macValue ) + 1 );
                        break;
                    }

                }
            }
        }
    }

    if (found)
    {
        WANCHK_LOG_INFO("%s %d - Got mac address for remote device=%s\n", __FUNCTION__, __LINE__,
                                                                                mac_addr);
        return ANSC_STATUS_SUCCESS;
    }
    else
    {
        WANCHK_LOG_ERROR("%s-%d: Failed to get mac address value\n", __FUNCTION__, __LINE__);
        return ANSC_STATUS_FAILURE;
    }

}

/* Fetching DNS entries for backup device, currently couldn't do wild card, so fetching
entires based on dns server count*/

ANSC_STATUS WanCnctvtyChk_GetSecondaryDNS_Entry(char *mac_addr,
                                                PCOSA_DML_WANCNCTVTY_CHK_DNSSRV_INFO pDnsSrvInfo,
                                                unsigned int DnsServerCount)
{
    /* rbus get parameter value */
    if(rbus_handle == NULL)
    {
        return ANSC_STATUS_FAILURE;
    }

    if (!pDnsSrvInfo)
    {
        WANCHK_LOG_ERROR("pDnsSrvInfo is NULL");
        return ANSC_STATUS_FAILURE;
    }

    errno_t rc = -1;
    char rbus_eventname[BUFLEN_256] = {0};
    char pStrVal[BUFLEN_256] = {0};
    dns_entrytype_t dns_family;
    int dns_idx =0;
    int iter =0;
    ANSC_STATUS returnStatus = ANSC_STATUS_FAILURE;

    for(iter=1;iter<=DnsServerCount;iter++)
    {
        memset(rbus_eventname,0,sizeof(rbus_eventname));
        rc = sprintf_s(rbus_eventname, sizeof(rbus_eventname),DNS_SRV_ENTRY_DML,iter);
        if (rc < EOK)
        {
            ERR_CHK(rc);
        }
        returnStatus = WanCnctvtyChk_GetRemoteParameterValue(mac_addr,rbus_eventname,pStrVal);
        /*Lets store in dns srv data*/
       /* eventhough in DNS code, we are setting the type only v4 or v6, but enum still has
       other options e.g RA etc. not sure may be later improvement may break us. so 
       sticking the check based on  address*/
       if (returnStatus == ANSC_STATUS_SUCCESS && pStrVal[0] != '\0')
       {
           dns_family = CHECK_V4_V6(pStrVal);
           if (dns_family == DNS_SRV_IPV4)
           {
               memset(pDnsSrvInfo[dns_idx].IPAddress.IPv4Address,0,IPv4_STR_LEN);
               rc = strcpy_s(pDnsSrvInfo[dns_idx].IPAddress.IPv4Address,IPv4_STR_LEN,pStrVal);
               ERR_CHK(rc);
               pDnsSrvInfo[dns_idx].dns_type = dns_family;
               WANCHK_LOG_DBG("DNS server name: %s\n\r",pDnsSrvInfo[dns_idx].IPAddress.IPv4Address);
           }
           else if (dns_family == DNS_SRV_IPV6)
           {
               memset(pDnsSrvInfo[dns_idx].IPAddress.IPv6Address,0,IPv6_STR_LEN);
               rc = strcpy_s(pDnsSrvInfo[dns_idx].IPAddress.IPv6Address,IPv6_STR_LEN,pStrVal);
               ERR_CHK(rc);
               pDnsSrvInfo[dns_idx].dns_type = dns_family;
               WANCHK_LOG_DBG("DNS server name: %s\n\r",pDnsSrvInfo[dns_idx].IPAddress.IPv6Address);
           }
           dns_idx++;
        }
        else
        {
            WANCHK_LOG_ERROR("Unable to fetch value for %s\n",rbus_eventname);
            return ANSC_STATUS_FAILURE;
        }
    }
    return ANSC_STATUS_SUCCESS;
}

void WanCnctvtyChk_IDM_AsyncMethodHandler(rbusHandle_t handle,char const* methodName,
                                                            rbusError_t error,rbusObject_t params)
{
    (void)handle;
    WANCHK_LOG_INFO("%s %d - asyncMethodHandler called: %s  error=%d\n", __FUNCTION__,
                                                                    __LINE__, methodName, error);
    if(error == RBUS_ERROR_SUCCESS)
    {
        WANCHK_LOG_INFO("%s %d - asyncMethodHandler request success\n", __FUNCTION__, __LINE__);
        rbusObject_fwrite(params, 1, stdout);
    }else
    {
        WANCHK_LOG_INFO("%s %d - asyncMethodHandler request failed\n", __FUNCTION__, __LINE__);
        return;
    }

    if(strcmp(methodName, "Device.X_RDK_Remote.Invoke()") == 0)
    {
        /* get the return values from IDM */
        rbusValue_t value;
        value = rbusObject_GetValue(params, "param_name");
        const char *pParamName = rbusValue_GetString(value, NULL);
        WANCHK_LOG_INFO("%s %d - param_name %s\n", __FUNCTION__, __LINE__, pParamName);

        value = rbusObject_GetValue(params, "param_value");
        const char *pValue = rbusValue_GetString(value, NULL);
        WANCHK_LOG_INFO("%s %d - param_value %s\n", __FUNCTION__, __LINE__, pValue);

        if (strcmp(pParamName,DNS_SRV_COUNT_DML) == 0)
        {
           unsigned int new_dns_server_count = 0;
           new_dns_server_count = rbusValue_GetUInt32(value);
           WANCHK_LOG_WARN("WanCnctvtyChkEventConsumer : New value of DNS server count = %d\n",
                                                                            new_dns_server_count);
           //handle_dns_srvrcnt_event(INTF_SECONDARY,new_dns_server_count);
        }
        else if (strstr(pParamName,DNS_SRV_TABLE_DML))
        {
           errno_t rc = -1;
           int dns_srv_index = 0;
           rc = sscanf_s(pParamName, DNS_SRV_ENTRY_DML, &dns_srv_index);
           ERR_CHK(rc);
           const char* newValue = rbusValue_GetString(value, NULL);
           WANCHK_LOG_WARN("WanCnctvtyChkEventConsumer : New value of DNS server for index %d = %s\n",
                                                                            dns_srv_index,newValue);
           //handle_dns_srv_addrchange_event(INTF_SECONDARY,dns_srv_index,newValue);
        }
    }
    else
    {
        WANCHK_LOG_INFO("%s %d - Remote Interface Not found \n", __FUNCTION__, __LINE__);
    }
}

void WanCnctvtyChk_CreateEthernetHeader (struct ethhdr *ethernet_header,char *src_mac, char *dst_mac, int protocol)
{
    /* copy the Src mac addr */
    memcpy(ethernet_header->h_source, (void *)ether_aton(src_mac), 6);

    /* copy the Dst mac addr */
    memcpy(ethernet_header->h_dest, (void *)ether_aton(dst_mac), 6);

    /* copy the protocol */
    ethernet_header->h_proto = htons(protocol);

    /* done ...send the header back */
    return;
}

void WanCnctvtyChk_CreateIPHeader (int family, char *srcIp, char *dstIp, unsigned int dataSize,void *header)
{
    if(family == AF_INET6){
        struct ip6_hdr *ipv6Hdr = (struct ip6_hdr *)header;
        if(ipv6Hdr == NULL)
            return;
        memset(ipv6Hdr, 0, sizeof(struct ip6_hdr));
        ipv6Hdr->ip6_flow = htonl ((6 << 28) | (0 << 20) | 0);
        ipv6Hdr->ip6_plen = htons(sizeof(struct udphdr) +  dataSize);
        ipv6Hdr->ip6_nxt = IPPROTO_UDP;
        ipv6Hdr->ip6_hops = 60;
        inet_pton(AF_INET6, srcIp, &(ipv6Hdr->ip6_src));
        inet_pton(AF_INET6, dstIp, &(ipv6Hdr->ip6_dst));
        return;
    }else{
        
    struct iphdr *ip_header = (struct iphdr *)header;
    memset(ip_header, 0, sizeof(struct iphdr));
    ip_header->version = 4;
    ip_header->ihl = (sizeof(struct iphdr))/4 ;
    ip_header->tos = 0;
    ip_header->tot_len = htons(sizeof(struct iphdr) + sizeof(struct udphdr) + dataSize);
    ip_header->id = htons(111);
    ip_header->frag_off = 0;
    ip_header->ttl = 111;
    ip_header->protocol = IPPROTO_UDP;
    ip_header->check = 0; /* We will calculate the checksum later */
    ip_header->saddr = inet_addr(srcIp);
    ip_header->daddr = inet_addr(dstIp);

    /* Calculate the IP checksum now :
       The IP Checksum is only over the IP header */
    ip_header->check = ComputeChecksum((unsigned char *)ip_header, ip_header->ihl*4);
    return;
    }
}

void WanCnctvtyChk_CreateUdpHeader(int family, unsigned short sport, unsigned short dport,
                                                    unsigned int dataSize,struct udphdr *header)
{
    struct udphdr *udp_header = header;
    memset(udp_header, 0, sizeof(struct udphdr));
    udp_header->uh_sport = htons(sport);
    udp_header->uh_dport = htons(dport);
    udp_header->uh_ulen =  htons(sizeof(struct udphdr) + dataSize);
    udp_header->uh_sum = 0; /* set checksum to zero*/
    return;
}

void WanCnctvtyChk_CreatePseudoHeaderAndComputeUdpChecksum (int family, struct udphdr *udp_header,
                                    void *ip_header, unsigned char *data, unsigned int dataSize)
{
    unsigned char *hdr = NULL;
    int pseudo_offset = 0;
    int header_len;

    /*The TCP Checksum is calculated over the PseudoHeader + TCP header +Data*/
    if(family == AF_INET){
        struct iphdr *ipv4_header = ip_header;
        /* Find the size of the UDP Header + Data */
        int segment_len = ntohs(ipv4_header->tot_len) - ipv4_header->ihl*4;

        /* Total length over which UDP checksum will be computed */
        header_len = sizeof(PseudoHeader) + segment_len;

        /* Allocate the memory */
        hdr = (unsigned char *)malloc(header_len);
        if(hdr == NULL)
            return;
        pseudo_offset = sizeof(PseudoHeader);
        /* Fill in the pseudo header first */
        PseudoHeader *pseudo_header = (PseudoHeader *)hdr;

        pseudo_header->source_ip = ipv4_header->saddr;
        pseudo_header->dest_ip = ipv4_header->daddr;
        pseudo_header->reserved = 0;
        pseudo_header->protocol = ipv4_header->protocol;
        pseudo_header->length = htons(segment_len);

    }
    else{
        struct ip6_hdr *ipv6_header = ip_header;
        /* total len = pseudo header length + tcp length */
        header_len = sizeof(PseudoHeaderv6) + ntohs(ipv6_header->ip6_plen);
         /* Allocate the memory */
        hdr = (unsigned char *)malloc(header_len);
        if(hdr == NULL)
            return;
        pseudo_offset = sizeof(PseudoHeaderv6);
        PseudoHeaderv6 *pseudo_header = (PseudoHeaderv6 *)hdr;
        memcpy(pseudo_header->source_ipv6, &(ipv6_header->ip6_src), 16);
        memcpy(pseudo_header->dest_ipv6, &(ipv6_header->ip6_dst), 16);
        pseudo_header->up_len = ipv6_header->ip6_plen;
        memset(pseudo_header->reserved, 0, 3);
        pseudo_header->next_hdr = ipv6_header->ip6_nxt;
    }
    /* Now copy UDP */
    memcpy((hdr + pseudo_offset), (void *)udp_header, sizeof(struct udphdr));

    /* Now copy the Data */
    memcpy((hdr + pseudo_offset + sizeof(struct udphdr)), data, dataSize);

    /* Calculate the Checksum */
    udp_header->uh_sum = WanCnctvtyChk_udp_checksum((unsigned short *)hdr, header_len);

    /* Free the PseudoHeader */
    free(hdr);

    return ;
}

unsigned short WanCnctvtyChk_udp_checksum (unsigned short *buffer, int byte_count)
{
    register long word_sum;
    int word_count;
    int i;

    word_sum = 0;
    word_count = byte_count >> 1;

    for(i = 0; i < word_count ; i++) {
    word_sum += buffer[i];
    }

    if( byte_count & 1 ) {
    word_sum += *(unsigned char*)&buffer[i];
    }

    unsigned short carry = (unsigned short) (word_sum >> 16);
    
    while (0 != carry)
    {
        word_sum = (word_sum & 0xffff) + carry;
        carry = (unsigned short) (word_sum >> 16);
    }

    return (short)(~word_sum);

}

/* ComputeChecksum() */
static unsigned short ComputeChecksum (void *data, unsigned long length)
{
    unsigned short  *tempUshort       = NULL,
                     UshortForPadding = 0;
    unsigned long    checksum         = 0;

    /*
     * retrieve the shortcut pointer
     */
    tempUshort = (unsigned short*)data;

    /*
     * loop to calculate the check sum
     */
    while ( length > 1 )
    {
        checksum += *tempUshort;
        tempUshort++;

        /*
         * if high-order bit set, fold
         */
        if ( checksum & 0x80000000 )
        {
            checksum = ( checksum & 0xFFFF ) + ( checksum >> 16 );
        }

        /*
         * modify length
         */
        length -= 2;
    }

    /*
     * take care of left over bytes.
     * note: although it's impossible...
     */
    if ( length )
    {
        UshortForPadding            = 0;
        *(unsigned char*)&UshortForPadding  = *(unsigned char*)tempUshort;
        checksum                   += UshortForPadding;
    }

    /*
     * fold the result checksum
     */
    while ( checksum >> 16 )
    {
        checksum = ( checksum & 0xFFFF ) + ( checksum >> 16 );
    }

    /*
     * return complement of checksum
     */
    return  ~((unsigned short)checksum);
}


uint8_t *WanCnctvtyChk_get_transport_header(struct ip6_hdr *ip6h,uint8_t target,uint8_t *payload_tail)
{
    uint8_t nexthdr_ipv6 = ip6h->ip6_nxt;
    uint8_t *cur_pos = (uint8_t *)ip6h + sizeof(struct ip6_hdr);

    while (nexthdr_ipv6 == IPPROTO_HOPOPTS ||
           nexthdr_ipv6 == IPPROTO_ROUTING ||
           nexthdr_ipv6 == IPPROTO_FRAGMENT ||
           nexthdr_ipv6 == IPPROTO_AH ||
           nexthdr_ipv6 == IPPROTO_NONE ||
           nexthdr_ipv6 == IPPROTO_DSTOPTS) 
    {
        struct ip6_ext *ip6_ext;
        uint32_t hdrlen;

        /* next hdr found*/
        if (nexthdr_ipv6 == target)
            break;

        /* No more extensions headers, we're done. */
        if (nexthdr_ipv6 == IPPROTO_NONE) {
            cur_pos = NULL;
            break;
        }

        /* No room for extension headers, bad packet. */
        if (payload_tail - cur_pos < sizeof(struct ip6_ext)) {
            cur_pos = NULL;
            break;
        }

        ip6_ext = (struct ip6_ext *)cur_pos;

        if (nexthdr_ipv6 == IPPROTO_FRAGMENT) {
            uint16_t *frag_off;

            if (payload_tail - cur_pos < sizeof(struct ip6_frag)) {
                cur_pos = NULL;
                break;
            }

            frag_off = (uint16_t *)cur_pos +
                    offsetof(struct ip6_frag, ip6f_offlg);

            /* Fragment offset is only 13 bits long. */
            if (htons(*frag_off & ~0x7)) {
                /* Not the first fragment, it does not contain
                 * any headers.
                 */
                cur_pos = NULL;
                break;
            }
            hdrlen = sizeof(struct ip6_frag);
        } else if (nexthdr_ipv6 == IPPROTO_AH)
            hdrlen = (ip6_ext->ip6e_len + 2) << 2;
        else
            hdrlen = (ip6_ext->ip6e_len + 1) << 3;

        nexthdr_ipv6 = ip6_ext->ip6e_nxt;
        cur_pos += hdrlen;
    }

    if (nexthdr_ipv6 != target)
        cur_pos = NULL;
    return cur_pos;
}
