/*
* If not stated otherwise in this file or this component's LICENSE file the
* following copyright and licenses apply:
*
* Copyright 2016 RDK Management
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

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "test/mocks/mock_utils.h"
#include "test/mocks/mock_rbus.h"
#include "test/mocks/mock_wanconnectivity.h"
#include "test/mocks/mock_cosa_wanconnectivity_dml.h"

extern "C" {
#include "plugin_main_apis.h"
#include <rbus/rbus.h>

char g_Subsystem[32] = "eRT.";
typedef ANSC_STATUS
(*COSAGetParamValueByPathNameProc)
    (
        void*                       bus_handle,
        parameterValStruct_t        *val,
        ULONG                       *parameterValueLength
    );

COSAGetParamValueByPathNameProc     g_GetParamValueByPathNameProc   = NULL;
}

using namespace std;
using namespace testing;
using ::testing::_;
using ::testing::Return;
using ::testing::SetArgPointee;
using ::testing::StrEq;
using ::testing::SetArrayArgument;

/* This is the actual definition of the mock obj */
UtilsMock* g_utilsMock = NULL;
rbusMock* g_rbusMock = NULL;
WanCnctvtyChkMock* g_wanCnctyChkMock = NULL;

class WanConnectivityCheckTest : public ::testing::Test {
protected:
        UtilsMock mockedUtils;
        rbusMock mockedRbus;
        WanCnctvtyChkMock mockedWanCnctyChk;

        WanConnectivityCheckTest() {
            g_utilsMock = &mockedUtils;
            g_rbusMock = &mockedRbus;
            g_wanCnctyChkMock = &mockedWanCnctyChk;

            //Populating gInterface_List to be used by Test fixtures
            intf_info1 = new WANCNCTVTY_CHK_GLOBAL_INTF_INFO();
            intf_info1->IPInterface.Enable = FALSE;
            strcpy((char*)intf_info1->IPInterface.Alias, "DOCSIS");
            strcpy((char*)intf_info1->IPInterface.InterfaceName, "erouter0");
            intf_info1->IPInterface.PassiveMonitor = TRUE;
            intf_info1->IPInterface.PassiveMonitorTimeout = 12000;
            intf_info1->IPInterface.ActiveMonitor = TRUE;
            intf_info1->IPInterface.ActiveMonitorInterval = 12000;
            intf_info1->IPInterface.MonitorResult = 0;
            intf_info1->IPInterface.QueryNow = FALSE;
            intf_info1->IPInterface.QueryNowResult = 0;
            intf_info1->IPInterface.QueryTimeout = 10000;
            intf_info1->IPInterface.QueryRetry = 1;
            strcpy((char*)intf_info1->IPInterface.RecordType, "A+AAAA");
            strcpy((char*)intf_info1->IPInterface.ServerType, "IPv4+IPv6");
            intf_info1->IPInterface.InstanceNumber = 1;
            intf_info1->IPInterface.Cfg_bitmask = 0;
            intf_info1->IPInterface.Configured = TRUE;
            intf_info1->IPInterface.MonitorResult_SubsCount = 0;
            intf_info1->IPInterface.QueryNowResult_SubsCount = 0;
            intf_info1->IPv4DnsServerList = NULL;
            intf_info1->IPv6DnsServerList = NULL;
            intf_info1->IPv4DnsServerCount = 0;
            intf_info1->IPv6DnsServerCount = 0;
            intf_info1->Dns_configured = FALSE;
            intf_info1->wancnctvychkpassivethread_tid = 0;
            intf_info1->PassiveMonitor_Running = FALSE;
            intf_info1->wancnctvychkactivethread_tid = 0;
            intf_info1->ActiveMonitor_Running = FALSE;
            intf_info1->wancnctvychkquerynowthread_tid = 0;
            intf_info1->QueryNow_Running = FALSE;

            // Allocate memory for intf_info2 and initialize its members
            intf_info2 = new WANCNCTVTY_CHK_GLOBAL_INTF_INFO();
            intf_info2->IPInterface.Enable = TRUE;
            strcpy((char*)intf_info2->IPInterface.Alias, "REMOTE_LTE");
            strcpy((char*)intf_info2->IPInterface.InterfaceName, "brRWAN");
            intf_info2->IPInterface.PassiveMonitor = TRUE;
            intf_info2->IPInterface.PassiveMonitorTimeout = 12000;
            intf_info2->IPInterface.ActiveMonitor = TRUE;
            intf_info2->IPInterface.ActiveMonitorInterval = 12000;
            intf_info2->IPInterface.MonitorResult = 0;
            intf_info2->IPInterface.QueryNow = FALSE;
            intf_info2->IPInterface.QueryNowResult = 0;
            intf_info2->IPInterface.QueryTimeout = 10000;
            intf_info2->IPInterface.QueryRetry = 1;
            strcpy((char*)intf_info2->IPInterface.RecordType, "A+AAAA");
            strcpy((char*)intf_info2->IPInterface.ServerType, "IPv4+IPv6");
            intf_info2->IPInterface.InstanceNumber = 2;
            intf_info2->IPInterface.Cfg_bitmask = 0;
            intf_info2->IPInterface.Configured = TRUE;
            intf_info2->IPInterface.MonitorResult_SubsCount = 0;
            intf_info2->IPInterface.QueryNowResult_SubsCount = 0;
            intf_info2->IPv4DnsServerList = NULL;
            intf_info2->IPv6DnsServerList = NULL;
            intf_info2->IPv4DnsServerCount = 0;
            intf_info2->IPv6DnsServerCount = 0;
            intf_info2->Dns_configured = FALSE;
            intf_info2->wancnctvychkpassivethread_tid = 0;
            intf_info2->PassiveMonitor_Running = FALSE;
            intf_info2->wancnctvychkactivethread_tid = 0;
            intf_info2->ActiveMonitor_Running = FALSE;
            intf_info2->wancnctvychkquerynowthread_tid = 0;
            intf_info2->QueryNow_Running = FALSE;

            intf_info1->next = intf_info2;
            intf_info2->next = NULL;

            gInterface_List = intf_info1;

        }
        virtual ~WanConnectivityCheckTest() {
            g_utilsMock = NULL;
            g_rbusMock = NULL;
            g_wanCnctyChkMock = NULL;
        }
        virtual void SetUp()
        {
            printf("%s %s %s\n", __func__,
                ::testing::UnitTest::GetInstance()->current_test_info()->test_case_name(),
                ::testing::UnitTest::GetInstance()->current_test_info()->name());
        }

        virtual void TearDown()
        {
            printf("%s %s %s\n", __func__,
                ::testing::UnitTest::GetInstance()->current_test_info()->test_case_name(),
                ::testing::UnitTest::GetInstance()->current_test_info()->name());
        }

        static void SetUpTestCase()
        {
            printf("%s %s\n", __func__,
                ::testing::UnitTest::GetInstance()->current_test_case()->name());
        }

        static void TearDownTestCase()
        {
            printf("%s %s\n", __func__,
                ::testing::UnitTest::GetInstance()->current_test_case()->name());
        }
};

ACTION_TEMPLATE(SetArgNPointeeTo, HAS_1_TEMPLATE_PARAMS(unsigned, uIndex), AND_2_VALUE_PARAMS(pData, uiDataSize))
{
    memcpy(std::get<uIndex>(args), pData, uiDataSize);
}


TEST_F(WanConnectivityCheckTest, WANCNCTVTYCHK_StartConnectivityCheck_ValidParams)
{
    printf("Start Connectivity Check with Valid Parameters\n");

    PWANCNCTVTY_CHK_GLOBAL_INTF_INFO intf_list = NULL;
    rbusHandle_t handle = NULL;
    rbusObject_t inParams;
    rbusObject_t outParams = NULL;

    rbusValue_t intf_value = (rbusValue_t)malloc(sizeof(_rbusValue));
    rbusValue_t alias_value = (rbusValue_t)malloc(sizeof(_rbusValue));
    rbusValue_t ipv4_dns_value = (rbusValue_t)malloc(sizeof(_rbusValue));
    rbusValue_t ipv6_dns_value = (rbusValue_t)malloc(sizeof(_rbusValue));

    intf_value->type = RBUS_STRING;
    intf_value->d.bytes = (rbusBuffer_t)malloc(sizeof(_rbusBuffer));
    intf_value->d.bytes->data = (uint8_t*)malloc(strlen("brlan0") + 1);
    strcpy((char*)intf_value->d.bytes->data, "brlan0");
    intf_value->d.bytes->posWrite = strlen("brlan0") + 1;

    alias_value->type = RBUS_STRING;
    alias_value->d.bytes = (rbusBuffer_t)malloc(sizeof(_rbusBuffer));
    alias_value->d.bytes->data = (uint8_t*)malloc(strlen("WANOE") + 1);
    strcpy((char*)intf_value->d.bytes->data, "WANOE");
    alias_value->d.bytes->posWrite = strlen("WANOE") + 1;

    ipv4_dns_value->type = RBUS_STRING;
    ipv4_dns_value->d.bytes = (rbusBuffer_t)malloc(sizeof(_rbusBuffer));
    ipv4_dns_value->d.bytes->data = (uint8_t*)malloc(strlen("75.75.75.75,75.75.76.76") + 1);
    strcpy((char*)ipv4_dns_value->d.bytes->data, "75.75.75.75,75.75.76.76");
    ipv4_dns_value->d.bytes->posWrite = strlen("75.75.75.75,75.75.76.76") + 1;

    ipv6_dns_value->type = RBUS_STRING;
    ipv6_dns_value->d.bytes = (rbusBuffer_t)malloc(sizeof(_rbusBuffer));
    ipv6_dns_value->d.bytes->data = (uint8_t*)malloc(strlen("2001:558:feed::1,2001:558:feed::2") + 1);
    strcpy((char*)ipv6_dns_value->d.bytes->data, "2001:558:feed::1,2001:558:feed::2");
    ipv6_dns_value->d.bytes->posWrite = strlen("2001:558:feed::1,2001:558:feed::2") + 1;

    EXPECT_CALL(*g_rbusMock, rbusObject_GetValue(_, StrEq("linux_interface_name"))).Times(1).WillOnce(Return(intf_value));
    EXPECT_CALL(*g_rbusMock, rbusValue_GetString(intf_value, _)).Times(1).WillOnce(Return("brlan0"));

    EXPECT_CALL(*g_rbusMock, rbusObject_GetValue(_, StrEq("alias"))).Times(1).WillOnce(Return(alias_value));
    EXPECT_CALL(*g_rbusMock, rbusValue_GetString(alias_value, _)).Times(1).WillOnce(Return("WANOE"));

    EXPECT_CALL(*g_rbusMock, rbusObject_GetValue(_, StrEq("IPv4_DNS_Servers"))).Times(1).WillOnce(Return(ipv4_dns_value));
    EXPECT_CALL(*g_rbusMock, rbusValue_GetString(ipv4_dns_value, _)).Times(1).WillOnce(Return("75.75.75.75,75.75.76.76"));

    EXPECT_CALL(*g_rbusMock, rbusObject_GetValue(_, StrEq("IPv6_DNS_Servers"))).Times(1).WillOnce(Return(ipv6_dns_value));
    EXPECT_CALL(*g_rbusMock, rbusValue_GetString(ipv6_dns_value, _)).Times(1).WillOnce(Return("2001:558:feed::1,2001:558:feed::2"));

    EXPECT_CALL(*g_wanCnctyChkMock, is_valid_interface(StrEq("brlan0"))).Times(1).WillOnce(Return(ANSC_STATUS_SUCCESS));
    EXPECT_CALL(*g_wanCnctyChkMock, CosaWanCnctvtyChk_GetActive_Status()).Times(1).WillOnce(Return(TRUE));

    rbusError_t result = WANCNCTVTYCHK_StartConnectivityCheck(handle, "Device.X_RDK_DNSInternet.StartConnectivityCheck()", inParams, outParams, NULL);
    EXPECT_EQ(result, RBUS_ERROR_SUCCESS);
}

TEST_F(WanConnectivityCheckTest, WANCNCTVTYCHK_StartConnectivityCheck_InvalidInterface)
{
    printf("Start Connectivity Check with Invalid Interface Name (interface: eth0)\n");

    char filename[64] = {0};
    rbusHandle_t handle = NULL;
    rbusObject_t inParams;
    rbusObject_t outParams = NULL;

    rbusValue_t intf_value = (rbusValue_t)malloc(sizeof(_rbusValue));
    rbusValue_t alias_value = (rbusValue_t)malloc(sizeof(_rbusValue));
    rbusValue_t ipv4_dns_value = (rbusValue_t)malloc(sizeof(_rbusValue));
    rbusValue_t ipv6_dns_value = (rbusValue_t)malloc(sizeof(_rbusValue));

    intf_value->type = RBUS_STRING;
    intf_value->d.bytes = (rbusBuffer_t)malloc(sizeof(_rbusBuffer));
    intf_value->d.bytes->data = (uint8_t*)malloc(strlen("eth0") + 1);
    strcpy((char*)intf_value->d.bytes->data, "eth0");
    intf_value->d.bytes->posWrite = strlen("eth0") + 1;

    alias_value->type = RBUS_STRING;
    alias_value->d.bytes = (rbusBuffer_t)malloc(sizeof(_rbusBuffer));
    alias_value->d.bytes->data = (uint8_t*)malloc(strlen("DOCSIS") + 1);
    strcpy((char*)alias_value->d.bytes->data, "DOCSIS");
    alias_value->d.bytes->posWrite = strlen("DOCSIS") + 1;

    ipv4_dns_value->type = RBUS_STRING;
    ipv4_dns_value->d.bytes = (rbusBuffer_t)malloc(sizeof(_rbusBuffer));
    ipv4_dns_value->d.bytes->data = (uint8_t*)malloc(strlen("75.75.75.75,75.75.76.76") + 1);
    strcpy((char*)ipv4_dns_value->d.bytes->data, "75.75.75.75,75.75.76.76");
    ipv4_dns_value->d.bytes->posWrite = strlen("75.75.75.75,75.75.76.76") + 1;

    ipv6_dns_value->type = RBUS_STRING;
    ipv6_dns_value->d.bytes = (rbusBuffer_t)malloc(sizeof(_rbusBuffer));
    ipv6_dns_value->d.bytes->data = (uint8_t*)malloc(strlen("2001:558:feed::1,2001:558:feed::2") + 1);
    strcpy((char*)ipv6_dns_value->d.bytes->data, "2001:558:feed::1,2001:558:feed::2");
    ipv6_dns_value->d.bytes->posWrite = strlen("2001:558:feed::1,2001:558:feed::2") + 1;

    EXPECT_CALL(*g_rbusMock, rbusObject_GetValue(_, StrEq("linux_interface_name"))).Times(1).WillOnce(Return(intf_value));
    EXPECT_CALL(*g_rbusMock, rbusValue_GetString(intf_value, _)).Times(1).WillOnce(Return("eth0"));

    EXPECT_CALL(*g_rbusMock, rbusObject_GetValue(_, StrEq("alias"))).Times(1).WillOnce(Return(alias_value));
    EXPECT_CALL(*g_rbusMock, rbusValue_GetString(alias_value, _)).Times(1).WillOnce(Return("DOCSIS"));

    EXPECT_CALL(*g_rbusMock, rbusObject_GetValue(_, StrEq("IPv4_DNS_Servers"))).Times(1).WillOnce(Return(ipv4_dns_value));
    EXPECT_CALL(*g_rbusMock, rbusValue_GetString(ipv4_dns_value, _)).Times(1).WillOnce(Return("75.75.75.75,75.75.76.76"));

    EXPECT_CALL(*g_rbusMock, rbusObject_GetValue(_, StrEq("IPv6_DNS_Servers"))).Times(1).WillOnce(Return(ipv6_dns_value));
    EXPECT_CALL(*g_rbusMock, rbusValue_GetString(ipv6_dns_value, _)).Times(1).WillOnce(Return("2001:558:feed::1,2001:558:feed::2"));

    EXPECT_CALL(*g_wanCnctyChkMock, is_valid_interface(StrEq("eth0"))).Times(1).WillOnce(Return(ANSC_STATUS_FAILURE));

    rbusError_t result = WANCNCTVTYCHK_StartConnectivityCheck(handle, "Device.X_RDK_DNSInternet.StartConnectivityCheck()", inParams, outParams, NULL);
    EXPECT_EQ(result, RBUS_ERROR_INVALID_INPUT);
}

TEST_F(WanConnectivityCheckTest, WANCNCTVTYCHK_StartConnectivityCheck_NullDNSList)
{
    printf("Start Connectivity Check with No DNS List\n");

    char* dns_list = NULL;
    rbusHandle_t handle = NULL;
    rbusObject_t inParams;
    rbusObject_t outParams = NULL;

    rbusValue_t intf_value = (rbusValue_t)malloc(sizeof(_rbusValue));
    rbusValue_t alias_value = (rbusValue_t)malloc(sizeof(_rbusValue));
    rbusValue_t ipv4_dns_value = (rbusValue_t)malloc(sizeof(_rbusValue));
    rbusValue_t ipv6_dns_value = (rbusValue_t)malloc(sizeof(_rbusValue));

    intf_value->type = RBUS_STRING;
    intf_value->d.bytes = (rbusBuffer_t)malloc(sizeof(_rbusBuffer));
    intf_value->d.bytes->data = (uint8_t*)malloc(strlen("erouter0") + 1);
    strcpy((char*)intf_value->d.bytes->data, "erouter0");
    intf_value->d.bytes->posWrite = strlen("erouter0") + 1;

    alias_value->type = RBUS_STRING;
    alias_value->d.bytes = (rbusBuffer_t)malloc(sizeof(_rbusBuffer));
    alias_value->d.bytes->data = (uint8_t*)malloc(strlen("DOCSIS") + 1);
    strcpy((char*)intf_value->d.bytes->data, "DOCSIS");
    alias_value->d.bytes->posWrite = strlen("DOCSIS") + 1;

    ipv4_dns_value->type = RBUS_STRING;
    ipv4_dns_value->d.bytes = (rbusBuffer_t)malloc(sizeof(_rbusBuffer));
    ipv4_dns_value->d.bytes->data = NULL;
    ipv4_dns_value->d.bytes->posWrite = 0;

    ipv6_dns_value->type = RBUS_STRING;
    ipv6_dns_value->d.bytes = (rbusBuffer_t)malloc(sizeof(_rbusBuffer));
    ipv6_dns_value->d.bytes->data = NULL;
    ipv6_dns_value->d.bytes->posWrite = 0;

    EXPECT_CALL(*g_rbusMock, rbusObject_GetValue(_, StrEq("linux_interface_name"))).Times(1).WillOnce(Return(intf_value));
    EXPECT_CALL(*g_rbusMock, rbusValue_GetString(intf_value, _)).Times(1).WillOnce(Return("erouter0"));

    EXPECT_CALL(*g_rbusMock, rbusObject_GetValue(_, StrEq("alias"))).Times(1).WillOnce(Return(alias_value));
    EXPECT_CALL(*g_rbusMock, rbusValue_GetString(alias_value, _)).Times(1).WillOnce(Return("DOCSIS"));

    EXPECT_CALL(*g_rbusMock, rbusObject_GetValue(_, StrEq("IPv4_DNS_Servers"))).Times(1).WillOnce(Return(ipv4_dns_value));
    EXPECT_CALL(*g_rbusMock, rbusValue_GetString(ipv4_dns_value, _)).Times(1).WillOnce(Return(dns_list));

    EXPECT_CALL(*g_rbusMock, rbusObject_GetValue(_, StrEq("IPv6_DNS_Servers"))).Times(1).WillOnce(Return(ipv6_dns_value));
    EXPECT_CALL(*g_rbusMock, rbusValue_GetString(ipv6_dns_value, _)).Times(1).WillOnce(Return(dns_list));

    EXPECT_CALL(*g_wanCnctyChkMock, is_valid_interface(StrEq("erouter0"))).Times(1).WillOnce(Return(ANSC_STATUS_SUCCESS));

    rbusError_t result = WANCNCTVTYCHK_StartConnectivityCheck(handle, "Device.X_RDK_DNSInternet.StartConnectivityCheck()", inParams, outParams, NULL);
    EXPECT_EQ(result, RBUS_ERROR_INVALID_INPUT);
}

TEST_F(WanConnectivityCheckTest, WANCNCTVTYCHK_StopConnectivityCheck_ValidParams_1)
{
    printf("Stop Connectivity Check with Valid Input (interface: erouter0, alias: DOCSIS)\n");

    rbusHandle_t handle = NULL;
    rbusObject_t inParams;
    rbusObject_t outParams = NULL;

    rbusValue_t intf_value = (rbusValue_t)malloc(sizeof(_rbusValue));
    rbusValue_t alias_value = (rbusValue_t)malloc(sizeof(_rbusValue));

    // Initialize intf_value with default values
    intf_value->type = RBUS_STRING;
    intf_value->d.bytes = (rbusBuffer_t)malloc(sizeof(_rbusBuffer));
    intf_value->d.bytes->data = (uint8_t*)malloc(strlen("erouter0") + 1);
    strcpy((char*)intf_value->d.bytes->data, "erouter0");
    intf_value->d.bytes->posWrite = strlen("erouter0") + 1;

    // Initialize alias_value with default values
    alias_value->type = RBUS_STRING;
    alias_value->d.bytes = (rbusBuffer_t)malloc(sizeof(_rbusBuffer));
    alias_value->d.bytes->data = (uint8_t*)malloc(strlen("DOCSIS") + 1);
    strcpy((char*)intf_value->d.bytes->data, "DOCSIS");
    alias_value->d.bytes->posWrite = strlen("DOCSIS") + 1;

    EXPECT_CALL(*g_rbusMock, rbusObject_GetValue(_, StrEq("linux_interface_name"))).Times(1).WillOnce(Return(intf_value));
    EXPECT_CALL(*g_rbusMock, rbusValue_GetString(intf_value, _)).Times(1).WillOnce(Return("erouter0"));

    EXPECT_CALL(*g_rbusMock, rbusObject_GetValue(_, StrEq("alias"))).Times(1).WillOnce(Return(alias_value));
    EXPECT_CALL(*g_rbusMock, rbusValue_GetString(alias_value, _)).Times(1).WillOnce(Return("DOCSIS"));

    rbusError_t result = WANCNCTVTYCHK_StopConnectivityCheck(handle, "Device.X_RDK_DNSInternet.StopConnectivityCheck()", inParams, outParams, NULL);
    EXPECT_EQ(result, RBUS_ERROR_SUCCESS);
}


TEST_F(WanConnectivityCheckTest, WANCNCTVTYCHK_StopConnectivityCheck_ValidParams_2)
{
    printf("Stop Connectivity Check with Valid Input (interface: brRWAN, alias: REMOTE_LTE)\n");

    rbusHandle_t handle = NULL;
    rbusObject_t inParams;
    rbusObject_t outParams = NULL;

    rbusValue_t intf_value = (rbusValue_t)malloc(sizeof(_rbusValue));
    rbusValue_t alias_value = (rbusValue_t)malloc(sizeof(_rbusValue));

    // Initialize intf_value with default values
    intf_value->type = RBUS_STRING;
    intf_value->d.bytes = (rbusBuffer_t)malloc(sizeof(_rbusBuffer));
    intf_value->d.bytes->data = (uint8_t*)malloc(strlen("brRWAN") + 1);
    strcpy((char*)intf_value->d.bytes->data, "brRWAN");
    intf_value->d.bytes->posWrite = strlen("brRWAN") + 1;

    // Initialize alias_value with default values
    alias_value->type = RBUS_STRING;
    alias_value->d.bytes = (rbusBuffer_t)malloc(sizeof(_rbusBuffer));
    alias_value->d.bytes->data = (uint8_t*)malloc(strlen("REMOTE_LTE") + 1);
    strcpy((char*)alias_value->d.bytes->data, "REMOTE_LTE");
    alias_value->d.bytes->posWrite = strlen("REMOTE_LTE") + 1;


    EXPECT_CALL(*g_rbusMock, rbusObject_GetValue(_, StrEq("linux_interface_name"))).Times(1).WillOnce(Return(intf_value));
    EXPECT_CALL(*g_rbusMock, rbusValue_GetString(intf_value, _)).Times(1).WillOnce(Return("brRWAN"));

    EXPECT_CALL(*g_rbusMock, rbusObject_GetValue(_, StrEq("alias"))).Times(1).WillOnce(Return(alias_value));
    EXPECT_CALL(*g_rbusMock, rbusValue_GetString(alias_value, _)).Times(1).WillOnce(Return("REMOTE_LTE"));

    rbusError_t result = WANCNCTVTYCHK_StopConnectivityCheck(handle, "Device.X_RDK_DNSInternet.StopConnectivityCheck()", inParams, outParams, NULL);
    EXPECT_EQ(result, RBUS_ERROR_SUCCESS);
}

TEST_F(WanConnectivityCheckTest, WANCNCTVTYCHK_StopConnectivityCheck_InvalidParams)
{
    printf("Stop Connectivity Check with Invalid Input (interface: wlan1, alias: WANOE)\n");

    rbusHandle_t handle = NULL;
    rbusObject_t inParams;
    rbusObject_t outParams = NULL;
    rbusValue_t intf_value = (rbusValue_t)malloc(sizeof(_rbusValue));
    rbusValue_t alias_value = (rbusValue_t)malloc(sizeof(_rbusValue));

    // Initialize intf_value with default values
    intf_value->type = RBUS_STRING;
    intf_value->d.bytes = (rbusBuffer_t)malloc(sizeof(_rbusBuffer));
    intf_value->d.bytes->data = (uint8_t*)malloc(strlen("wlan1") + 1);
    strcpy((char*)intf_value->d.bytes->data, "wlan1");
    intf_value->d.bytes->posWrite = strlen("wlan1") + 1;

    // Initialize alias_value with default values
    alias_value->type = RBUS_STRING;
    alias_value->d.bytes = (rbusBuffer_t)malloc(sizeof(_rbusBuffer));
    alias_value->d.bytes->data = (uint8_t*)malloc(strlen("WANOE") + 1);
    strcpy((char*)intf_value->d.bytes->data, "WANOE");
    alias_value->d.bytes->posWrite = strlen("WANOE") + 1;

    EXPECT_CALL(*g_rbusMock, rbusObject_GetValue(_, StrEq("linux_interface_name"))).Times(1).WillOnce(Return(intf_value));
    EXPECT_CALL(*g_rbusMock, rbusValue_GetString(intf_value, _)).Times(1).WillOnce(Return("wlan1"));

    EXPECT_CALL(*g_rbusMock, rbusObject_GetValue(_, StrEq("alias"))).Times(1).WillOnce(Return(alias_value));
    EXPECT_CALL(*g_rbusMock, rbusValue_GetString(alias_value, _)).Times(1).WillOnce(Return("WANOE"));

    rbusError_t result = WANCNCTVTYCHK_StopConnectivityCheck(handle, "Device.X_RDK_DNSInternet.StopConnectivityCheck()", inParams, outParams, NULL);
    EXPECT_EQ(result, RBUS_ERROR_INVALID_INPUT);
}

