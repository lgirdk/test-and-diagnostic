/*********************************************************************************
 * Copyright 2019 Liberty Global B.V.
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
  *********************************************************************************/

#ifndef  _COSA_TELEMETRY_INTERNAL_H
#define  _COSA_TELEMETRY_INTERNAL_H

#include "cosa_apis.h"
#include "ansc_platform.h"
#include "ansc_string_util.h"

#define COSA_DATAMODEL_TELEMETRY_OID                                                100

typedef struct
_DML_DCM_DOWNLOAD_STATUS
{
    CHAR    LastSuccessTimestamp[30];
    CHAR    LastAttemptTimestamp[30];
    CHAR    HTTPStatusString[256];
    ULONG   HTTPStatus;
    ULONG   AttemptCount;
}
DML_DCM_DOWNLOAD_STATUS, *PDML_DCM_DOWNLOAD_STATUS;

typedef struct
_DML_ODH_UPLOAD_STATUS
{
    CHAR    LastSuccessTimestamp[30];
    CHAR    LastAttemptTimestamp[30];
    CHAR    HTTPStatusString[256];
    ULONG   HTTPStatus;
    ULONG   AttemptCount;
}
DML_ODH_UPLOAD_STATUS, *PDML_ODH_UPLOAD_STATUS;

typedef struct
_DML_DCM_RETRY_CONFIG
{
    ULONG   AttemptInterval;
    ULONG   MaxAttempts;
}
DML_DCM_RETRY_CONFIG, *PDML_DCM_RETRY_CONFIG;

typedef  struct
_COSA_DATAMODEL_TELEMETRY
{
    COSA_BASE_CONTENT
    BOOL    Enable;
    BOOL    DCMConfigForceDownload;
    CHAR    DCMConfigFileURL[256];
    CHAR    UploadRepositoryURL[256];
    PDML_DCM_DOWNLOAD_STATUS pDcmStatus;
    PDML_ODH_UPLOAD_STATUS   pUploadStatus;
    PDML_DCM_RETRY_CONFIG    pRetryConfig;
}
COSA_DATAMODEL_TELEMETRY, *PCOSA_DATAMODEL_TELEMETRY;

/*
    Standard function declaration
*/
ANSC_HANDLE CosatelemetryCreate ( void );
ANSC_STATUS CosatelemetryInitialize ( ANSC_HANDLE hThisObject );
ANSC_STATUS CosatelemetryRemove ( ANSC_HANDLE hThisObject );

#endif //_COSA_TELEMETRY_INTERNAL_H
