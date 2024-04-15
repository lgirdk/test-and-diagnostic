 /*
* If not stated otherwise in this file or this component's LICENSE file the
* following copyright and licenses apply:
*
* Copyright 2020 RDK Management
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

#include "test/mocks/mock_wanconnectivity.h"

using namespace std;

extern WanCnctvtyChkMock *g_wanCnctyChkMock;

extern "C" ANSC_STATUS is_valid_interface(const char *if_name) 
{
    if (!g_wanCnctyChkMock)
    {
        return ANSC_STATUS_FAILURE;
    }
    return g_wanCnctyChkMock->is_valid_interface(if_name);
}

extern "C" BOOL CosaWanCnctvtyChk_GetActive_Status (void) 
{
    if (!g_wanCnctyChkMock)
    {
        return FALSE;
    }
    return g_wanCnctyChkMock->CosaWanCnctvtyChk_GetActive_Status();
}
