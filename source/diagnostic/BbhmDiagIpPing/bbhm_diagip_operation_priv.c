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


/**********************************************************************

    module:    bbhm_diagip_operation.c

        For Broadband Home Manager Model Implementation (BBHM),
        BroadWay Service Delivery System

    ---------------------------------------------------------------

    description:

        This module implements the advanced operation functions
        of the Pingection Speed Tool Object.

        *    BbhmDiagResolvAddr

    ---------------------------------------------------------------

    environment:

        platform independent

    ---------------------------------------------------------------

    author:

        Venka Gade

    ---------------------------------------------------------------

    revision:

        15/23/14    initial revision.

**********************************************************************/


#include "bbhm_diagip_global.h"


/**********************************************************************

    caller:     owner of this object

    prototype:

        ANSC_STATUS
        BbhmDiagResolvAddr
            (
        		ANSC_HANDLE                 hThisObject
            );

    description:

        This function is called to send the packets.

    argument:   ANSC_HANDLE                 hThisObject
                This handle is actually the pointer of this object
                itself.

    return:     status of operation.

**********************************************************************/

ANSC_STATUS
BbhmDiagResolvAddr
    (
        ANSC_HANDLE                 hThisObject
    )
{
    PBBHM_DIAG_IP_PING_OBJECT       pMyObject    = (PBBHM_DIAG_IP_PING_OBJECT)hThisObject;
    PBBHM_IP_PING_PROPERTY          pProperty    = (PBBHM_IP_PING_PROPERTY  )&pMyObject->Property;
    PBBHM_IP_PING_SINK_OBJECT       pSink        = (PBBHM_IP_PING_SINK_OBJECT)pMyObject->hSinkObject;
    PANSC_XSOCKET_OBJECT            pSocket      = (PANSC_XSOCKET_OBJECT     )pSink->GetXsocket((ANSC_HANDLE)pSink);
    return pSocket->ResolveAddr((ANSC_HANDLE)pSocket);
}

