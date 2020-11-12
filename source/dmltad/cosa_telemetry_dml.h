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

#ifndef  _COSA_TELEMETRY_DML_H
#define  _COSA_TELEMETRY_DML_H

#include "cosa_apis.h"

ANSC_STATUS CosaDmlTelemetryInit ( ANSC_HANDLE hThisObject );

/***********************************************************************
 APIs for Object: RDKCENTRAL_telemetry.
***********************************************************************/
BOOL Telemetry_GetParamBoolValue ( ANSC_HANDLE hInsContext, char *ParamName, BOOL* pBool );
BOOL Telemetry_SetParamBoolValue ( ANSC_HANDLE hInsContext, char *ParamName, BOOL bValue );
ANSC_STATUS Telemetry_GetParamStringValue ( ANSC_HANDLE hInsContext, char *ParamName, char *pValue );
BOOL Telemetry_SetParamStringValue ( ANSC_HANDLE hInsContext, char *ParamName, char *pString );
BOOL Telemetry_Validate ( ANSC_HANDLE hInsContext, char *pReturnParamName, ULONG *puLength );
BOOL Telemetry_Commit ( ANSC_HANDLE hInsContext );
BOOL Telemetry_Rollback ( ANSC_HANDLE hInsContext );

/***********************************************************************
 APIs for Object: Telemetry.DcmDownloadStatus.
***********************************************************************/
BOOL DcmDownloadStatus_GetParamUlongValue ( ANSC_HANDLE hInsContext, char *ParamName, ULONG *pUlong );
BOOL DcmDownloadStatus_SetParamUlongValue ( ANSC_HANDLE hInsContext, char *ParamName, ULONG *pUlong );
ANSC_STATUS DcmDownloadStatus_GetParamStringValue ( ANSC_HANDLE hInsContext, char *ParamName, char *pValue );
BOOL DcmDownloadStatus_SetParamStringValue ( ANSC_HANDLE hInsContext, char *ParamName, char *pValue );
BOOL DcmDownloadStatus_Validate ( ANSC_HANDLE hInsContext, char *pReturnParamName, ULONG *puLength );
BOOL DcmDownloadStatus_Commit ( ANSC_HANDLE hInsContext );
BOOL DcmDownloadStatus_Rollback ( ANSC_HANDLE hInsContext );

/***********************************************************************
 APIs for Object: Telemetry.UploadStatus.
***********************************************************************/
BOOL UploadStatus_GetParamUlongValue ( ANSC_HANDLE hInsContext, char *ParamName, ULONG *pUlong );
ANSC_STATUS UploadStatus_GetParamStringValue ( ANSC_HANDLE hInsContext, char *ParamName, char *pValue );
BOOL UploadStatus_Validate ( ANSC_HANDLE hInsContext, char *pReturnParamName, ULONG *puLength );
BOOL UploadStatus_Commit ( ANSC_HANDLE hInsContext );
BOOL UploadStatus_Rollback ( ANSC_HANDLE hInsContext );

/***********************************************************************
 APIs for Object: Telemetry.DcmRetryConfig.
***********************************************************************/
BOOL DcmRetryConfig_GetParamUlongValue ( ANSC_HANDLE hInsContext, char *ParamName, ULONG *pUlong );
BOOL DcmRetryConfig_SetParamUlongValue ( ANSC_HANDLE hInsContext, char *ParamName, ULONG uValue );
BOOL DcmRetryConfig_Validate ( ANSC_HANDLE hInsContext, char *pReturnParamName, ULONG *puLength );
BOOL DcmRetryConfig_Commit ( ANSC_HANDLE hInsContext );
BOOL DcmRetryConfig_Rollback ( ANSC_HANDLE hInsContext );

#endif //_COSA_TELEMETRY_DML_H
