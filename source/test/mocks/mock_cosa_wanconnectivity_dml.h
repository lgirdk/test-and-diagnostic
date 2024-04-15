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

extern "C" {
#include "dmltad/cosa_wanconnectivity_apis.h"
#include "dmltad/cosa_wanconnectivity_operations.h"
#include "dmltad/cosa_wanconnectivity_rbus_apis.h"
#include "dmltad/cosa_wanconnectivity_rbus_handler_apis.h"
}

typedef  unsigned char              UCHAR,          *PUCHAR;
typedef  unsigned long              ULONG,          *PULONG;
typedef  UCHAR                      BOOL,           *PBOOL;
typedef  ULONG                  ANSC_STATUS,     *PANSC_STATUS;
typedef  void*                  PVOID;
typedef  PVOID                  ANSC_HANDLE,     *PANSC_HANDLE;


PWANCNCTVTY_CHK_GLOBAL_INTF_INFO gInterface_List;
PWANCNCTVTY_CHK_GLOBAL_INTF_INFO intf_info1, intf_info2;

#define atomic_int volatile int
typedef struct rtRetainable
{
  atomic_int refCount;
} rtRetainable;

typedef struct _rbusBuffer
{
    int             lenAlloc;
    int             posWrite;
    int             posRead;
    uint8_t*        data;
    uint8_t         block1[64];
} *rbusBuffer_t;

struct _rbusValue
{
    rtRetainable retainable;
    union
    {
        bool                    b;
        char                    c;
        unsigned char           u;
        int8_t                  i8;
        uint8_t                 u8;
        int16_t                 i16;
        uint16_t                u16;
        int32_t                 i32;
        uint32_t                u32;
        int64_t                 i64;
        uint64_t                u64;
        float                   f32;
        double                  f64;
        rbusDateTime_t          tv;
        rbusBuffer_t            bytes;
        struct  _rbusProperty*  property;
        struct  _rbusObject*    object;
    } d;
    rbusValueType_t type;
};
