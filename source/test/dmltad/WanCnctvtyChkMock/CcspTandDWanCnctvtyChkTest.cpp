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

#include <stdio.h>
#include <string.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "test/mocks/mock_wanconnectivity.h"

using namespace std;

extern "C" {
#include "plugin_main_apis.h"
#include "safec_lib_common.h"
#include "cosa_wanconnectivity_apis.h"
#include <rbus/rbus.h>
#include <syscfg/syscfg.h>

char                                g_Subsystem[32] = "eRT.";
typedef ANSC_STATUS
(*COSAGetParamValueByPathNameProc)
    (
        void*                       bus_handle,
        parameterValStruct_t        *val,
        ULONG                       *parameterValueLength
    );

COSAGetParamValueByPathNameProc     g_GetParamValueByPathNameProc   = NULL;

BOOL CosaWanCnctvtyChk_GetActive_Status(void);
}

using ::testing::_;
using ::testing::Return;
using ::testing::StrEq;

WanCnctvtyChkMock *g_wanCnctyChkMock = NULL;

class CcspTandDWanCnctvtyChkTestFixture : public ::testing::Test {
    protected:
        WanCnctvtyChkMock mockedWanCnctvtyChk;
  
        CcspTandDWanCnctvtyChkTestFixture()
        {
            g_wanCnctyChkMock = &mockedWanCnctvtyChk;
        }
        virtual ~CcspTandDWanCnctvtyChkTestFixture()
        {
            g_wanCnctyChkMock = NULL;
        }
};

/*
TEST(CcspTandDWanCnctvtyChkTestFixture, CosaWanCnctvtyChk_GetActive_Status_Enable)
{
  EXPECT_EQ(TRUE, CosaWanCnctvtyChk_GetActive_Status());
}

TEST(CcspTandDWanCnctvtyChkTestFixture, CosaWanCnctvtyChk_GetActive_Status_Disable)
{
  EXPECT_EQ(FALSE, CosaWanCnctvtyChk_GetActive_Status());
}
*/

TEST_F(CcspTandDWanCnctvtyChkTestFixture, CosaWanCnctvtyChk_GetActive_Status_Enable)
{
    char paramName[] = "Device.X_RDK_GatewayManagement.ActiveGateway";
    EXPECT_CALL(*g_wanCnctyChkMock, WanCnctvtyChk_GetParameterValue(StrEq(paramName), _))
        .Times(1)
        .WillOnce(Return(ANSC_STATUS_SUCCESS));
    EXPECT_EQ(TRUE, CosaWanCnctvtyChk_GetActive_Status());
}

TEST_F(CcspTandDWanCnctvtyChkTestFixture, CosaWanCnctvtyChk_GetActive_Status_Disable)
{
    char paramName[] = "Device.X_RDK_GatewayManagement.ActiveGateway";
    EXPECT_CALL(*g_wanCnctyChkMock, WanCnctvtyChk_GetParameterValue(StrEq(paramName), _))
        .Times(1)
        .WillOnce(Return(ANSC_STATUS_SUCCESS));
    EXPECT_EQ(FALSE, CosaWanCnctvtyChk_GetActive_Status());
}

TEST_F(CcspTandDWanCnctvtyChkTestFixture, CosaWanCnctvtyChk_GetActive_Status_Enable_Fail)
{
    char paramName[] = "Device.X_RDK_GatewayManagement.ActiveGateway";
    EXPECT_CALL(*g_wanCnctyChkMock, WanCnctvtyChk_GetParameterValue(StrEq(paramName), _))
        .Times(1)
        .WillOnce(Return(ANSC_STATUS_SUCCESS));
    EXPECT_EQ(FALSE, CosaWanCnctvtyChk_GetActive_Status());
}

TEST_F(CcspTandDWanCnctvtyChkTestFixture, CosaWanCnctvtyChk_GetActive_Status_Disable_Fail)
{
    char paramName[] = "Device.X_RDK_GatewayManagement.ActiveGateway";
    EXPECT_CALL(*g_wanCnctyChkMock, WanCnctvtyChk_GetParameterValue(StrEq(paramName), _))
        .Times(1)
        .WillOnce(Return(ANSC_STATUS_SUCCESS));
    EXPECT_EQ(TRUE, CosaWanCnctvtyChk_GetActive_Status());
}

TEST_F(CcspTandDWanCnctvtyChkTestFixture, CosaWanCnctvtyChk_GetActive_Status_Fail)
{
    EXPECT_CALL(*g_wanCnctyChkMock, WanCnctvtyChk_GetParameterValue( _, _))
          .Times(1)
          .WillOnce(Return(ANSC_STATUS_FAILURE));
    EXPECT_EQ(FALSE, CosaWanCnctvtyChk_GetActive_Status());
}