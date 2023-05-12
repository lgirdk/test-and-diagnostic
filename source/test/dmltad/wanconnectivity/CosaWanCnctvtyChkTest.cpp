/*
 * If not stated otherwise in this file or this component's LICENSE file the
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

#include <gtest/gtest.h>

extern "C" {
#include "plugin_main_apis.h"
#include "safec_lib_common.h"
#include "cosa_wanconnectivity_operations.h"
#include <rbus/rbus.h>
#include <syscfg/syscfg.h>
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

#define  AnscSListInitializeHeader(ListHead)                                                \
         {                                                                                  \
            (ListHead)->Next.Next = NULL;                                                   \
            (ListHead)->Depth     = 0;                                                      \
            (ListHead)->Sequence  = 0;                                                      \
         }

ANSC_STATUS CosaWanCnctvtyChk_Init_URLTable (VOID);
char** get_url_list(int *total_url_entries);
uint8_t *WanCnctvtyChk_get_transport_header(struct ip6_hdr *ip6h,uint8_t target,uint8_t *payload_tail);
bool validatemac(char *mac);
unsigned short WanCnctvtyChk_udp_checksum (unsigned short *buffer, int byte_count);
void WanCnctvtyChk_CreatePseudoHeaderAndComputeUdpChecksum (int family, struct udphdr *udp_header, void *ip_header,
                                                        unsigned char *data, unsigned int dataSize);
char                                g_Subsystem[32] = "eRT.";

//Packet capture reference:
//    https://packetlife.net/media/captures/DNS%20Question%20%26%20Answer.pcapng.cap
const char expected_checksum[] = "\x28\xa2";
const char g_pkt[] =
//Ethernet
"\xba\xba\xba\xba\xba\xba\x48\xf8\xb3\x26\xdf\x49\x08\x00"
//IPv4_Header
"\x45\x08\x00\xe8\xb2\xef\x00\x00\x37\x11\xfe\x21\x08\x08\x08\x08"
"\xc0\xa8\x01\x34"
//UDP_Header
"\x00\x35\xd5\x39\x00\xd4"
//"\x28\xa2" //Actual UDP checksum from Packet capture --> expected checksum
"\x00\x00"   //UDP checksum kept as 0x0000 to calculate expected checksum
//"\x00\x35\xd5\x39\x00\xd4\x73\xfb"
//DNS_response
"\x00\x03\x81\x80\x00\x01\x00\x0b\x00\x00\x00\x00\x06\x67\x6f\x6f"
"\x67\x6c\x65\x03\x63\x6f\x6d\x00\x00\x01\x00\x01\xc0\x0c\x00\x01"
"\x00\x01\x00\x00\x00\x04\x00\x04\x4a\x7d\xec\x23\xc0\x0c\x00\x01"
"\x00\x01\x00\x00\x00\x04\x00\x04\x4a\x7d\xec\x25\xc0\x0c\x00\x01"
"\x00\x01\x00\x00\x00\x04\x00\x04\x4a\x7d\xec\x27\xc0\x0c\x00\x01"
"\x00\x01\x00\x00\x00\x04\x00\x04\x4a\x7d\xec\x20\xc0\x0c\x00\x01"
"\x00\x01\x00\x00\x00\x04\x00\x04\x4a\x7d\xec\x28\xc0\x0c\x00\x01"
"\x00\x01\x00\x00\x00\x04\x00\x04\x4a\x7d\xec\x21\xc0\x0c\x00\x01"
"\x00\x01\x00\x00\x00\x04\x00\x04\x4a\x7d\xec\x29\xc0\x0c\x00\x01"
"\x00\x01\x00\x00\x00\x04\x00\x04\x4a\x7d\xec\x22\xc0\x0c\x00\x01"
"\x00\x01\x00\x00\x00\x04\x00\x04\x4a\x7d\xec\x24\xc0\x0c\x00\x01"
"\x00\x01\x00\x00\x00\x04\x00\x04\x4a\x7d\xec\x2e\xc0\x0c\x00\x01"
"\x00\x01\x00\x00\x00\x04\x00\x04\x4a\x7d\xec\x26";

const char ipv6_dns_query_pkt[] =
//Ethernet
"\x80\x32\x53\x2c\x5a\xe2\xd2\xfd\x52\xc5\x6c\xa9\x86\xdd"
//IPv6_Header
"\x62\x81\x6e\x30\x00\x40\x11\x39\x20\x01\x48\x60\x48\x60"
"\x00\x00\x00\x00\x00\x00\x00\x00\x88\x88\x24\x09\x40\x72"
"\x6d\x1d\xd2\x4d\xa4\xc5\xec\x25\xeb\x59\x17\xb4"
//UDP_Header
"\x00\x35\xe5\xa9\x00\x40\x2e\x91"
//DNS_response
"\x30\xf6\x81\x80\x00\x01\x00\x01\x00\x00\x00\x00\x06\x67"
"\x6f\x6f\x67\x6c\x65\x03\x63\x6f\x6d\x00\x00\x1c\x00\x01"
"\xc0\x0c\x00\x1c\x00\x01\x00\x00\x00\x14\x00\x10\x24\x04"
"\x68\x00\x40\x07\x08\x21\x00\x00\x00\x00\x00\x00\x20\x0e";

typedef ANSC_STATUS
(*COSAGetParamValueByPathNameProc)
    (
        void*                       bus_handle,
        parameterValStruct_t        *val,
        ULONG                       *parameterValueLength
    );

COSAGetParamValueByPathNameProc     g_GetParamValueByPathNameProc   = NULL;
pthread_mutex_t               gUrlAccessMutex;
SLIST_HEADER                  gpUrlList;
ULONG                         gulUrlNextInsNum = 1;
}

TEST(CosaWanCnctvtyChk, GetUrlListGtest)
{
    AnscSListInitializeHeader(&gpUrlList);
    int total_url_entries = 0;
    char **url_list     = NULL;
    CosaWanCnctvtyChk_Init_URLTable();
    url_list = get_url_list(&total_url_entries);

    BOOL gtest_use_default_url = FALSE;
    char gtest_buf[8] = {0};
    char gtest_default_url[] = "www.google.com";
    char gtest_paramName[128] = {0};
    int gtest_index = 0;
    char gtest_URL_buf[255] = {0};
    int gtest_total_url_entries = 0;
    errno_t rc = -1;

    if (syscfg_get(NULL, "wanconnectivity_chk_maxurl_inst", gtest_buf, sizeof(gtest_buf)) == 0 &&
        gtest_buf[0] != '\0')
    {
        gtest_total_url_entries = atoi(gtest_buf);
    }
    else
    {
        gtest_total_url_entries = 1;
        gtest_use_default_url  = TRUE;
    }

    if (gtest_use_default_url)
    {
        EXPECT_EQ(total_url_entries, gtest_total_url_entries);
        EXPECT_STREQ(url_list[0], "www.google.com");
    }
    else
    {
        for(gtest_index = 0; gtest_index < gtest_total_url_entries; gtest_index++)
        {
            rc = sprintf_s(gtest_paramName,sizeof(gtest_paramName),"wanconnectivity_chk_url_%d",
                           (gtest_index+1));
            if (rc < EOK)
            {
                ERR_CHK(rc);
            }

            if (syscfg_get(NULL, gtest_paramName, gtest_URL_buf, sizeof(gtest_URL_buf)) == 0 &&
                gtest_URL_buf[0] != '\0')
            {
                EXPECT_STREQ(url_list[gtest_index], gtest_URL_buf);
            }
        }
    }
    free(url_list);
}

TEST(CosaWanCnctvtyChk, UdpCheckSumGtest)
{
    unsigned char * pkt = (unsigned char *)malloc(sizeof(g_pkt));
    memcpy(pkt, g_pkt, sizeof(g_pkt));
    struct udphdr *g_udp_header = (struct udphdr *)(pkt+34);
    void *g_ip_header = (void *)(pkt+14);
    unsigned char *g_data = (unsigned char *)(pkt+42);
    unsigned int g_dataSize = sizeof(g_pkt) - 42;
    WanCnctvtyChk_CreatePseudoHeaderAndComputeUdpChecksum (AF_INET, g_udp_header,
                                    g_ip_header, g_data, g_dataSize);
    char *checksum;
    checksum = (char *)(pkt+40);
    EXPECT_STREQ(checksum, expected_checksum);
}

TEST(CosaWanCnctvtyChk, ValidateMacGtestPositive)
{
    char sample_mac[] = "AA:BB:CC:DD:EE:FF";
    bool valid_mac;
    valid_mac = validatemac(sample_mac);
    EXPECT_EQ(1, valid_mac);
}

TEST(CosaWanCnctvtyChk, ValidateMacGtestNegative)
{
    char sample_mac[] = "AA:BB:CC:ZZ:EE:FF";
    bool valid_mac;
    valid_mac = validatemac(sample_mac);
    EXPECT_EQ(0, valid_mac);
}

TEST(CosaWanCnctvtyChk, GetTransportHeaderGtest)
{
    char *transport_hdr_start;
    transport_hdr_start = (char *) WanCnctvtyChk_get_transport_header
                                   ((struct ip6_hdr *)(ipv6_dns_query_pkt + 14), IPPROTO_UDP,
                                   (uint8_t *)(ipv6_dns_query_pkt+sizeof(ipv6_dns_query_pkt)-1));
    EXPECT_STREQ((ipv6_dns_query_pkt+54), transport_hdr_start);
}
