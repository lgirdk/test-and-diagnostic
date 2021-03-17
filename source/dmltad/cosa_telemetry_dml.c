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

#include <syscfg/syscfg.h>

#include "ansc_platform.h"
#include "cosa_telemetry_dml.h"
#include "cosa_telemetry_internal.h"
#include "plugin_main_apis.h"

typedef enum {
    TELE_ST_NONE,
    TELE_ST_START,
    TELE_ST_STOP,
    TELE_ST_RELOAD,
}
telemetry_state_t;

static BOOL start_telemetry_service = FALSE;
static telemetry_state_t tele_state = TELE_ST_NONE;

/***********************************************************************
 APIs for Object: RDKCENTRAL_telemetry.
***********************************************************************/

BOOL Telemetry_GetParamBoolValue(ANSC_HANDLE hInsContext, char* ParamName, BOOL* bValue)
{
    CcspTraceDebug(("%s: Entered\n", __FUNCTION__));
    PCOSA_DATAMODEL_TELEMETRY pMyObject = (PCOSA_DATAMODEL_TELEMETRY)g_pCosaBEManager->hTelemetry;
    if (strcmp(ParamName, "Enable") == 0)
    {
        CcspTraceDebug(("%s: Existing value for Telemetry.Enable is: %d\n", __FUNCTION__, pMyObject->Enable));
        *bValue =  pMyObject->Enable;
        return TRUE;
    }
    else if (strcmp(ParamName, "DCMConfigForceDownload") == 0)
    {
        CcspTraceDebug(("%s: Existing value for Telemetry.DCMConfigForceDownload is: %d\n", __FUNCTION__, pMyObject->DCMConfigForceDownload));
        *bValue = pMyObject->DCMConfigForceDownload;
        return TRUE;
    }
    else
    {
        CcspTraceError(("%s: Unknown parameter %s\n", __FUNCTION__, ParamName));
    }
    return FALSE;
}

BOOL Telemetry_SetParamBoolValue(ANSC_HANDLE hInsContext, char* ParamName, BOOL bValue)
{
    CcspTraceDebug(("%s: Entered\n", __FUNCTION__));
    PCOSA_DATAMODEL_TELEMETRY pMyObject = (PCOSA_DATAMODEL_TELEMETRY)g_pCosaBEManager->hTelemetry;
    if (strcmp(ParamName, "Enable") == 0)
    {
        CcspTraceDebug(("%s Telemetry Enable Set Value is %d: and the existing value is %d\n", __FUNCTION__, bValue, pMyObject->Enable));
        if (pMyObject->Enable == bValue)
        {
            return TRUE;
        }

        /* Telemetry agent receives the events only if T2Enable is enabled */
        if (bValue == TRUE)
        {
#ifdef _PUMA6_ARM_
            system("rpcclient2 'syscfg set telemetry_enable true; syscfg set T2Enable true'");
#endif
            if (syscfg_set(NULL, "T2Enable", "true") != 0)
            {
                CcspTraceError(("syscfg_set failed for Telemetry2 Enable\n"));
                return FALSE;
            }
        }
        else
        {
#ifdef _PUMA6_ARM_
            system("rpcclient2 'syscfg set telemetry_enable false; syscfg set T2Enable false'");
#endif
        }

        if (syscfg_set_commit(NULL, "telemetry_enable", bValue ? "true" : "false") != 0)
        {
            CcspTraceWarning(("%s: syscfg_set failed for %s\n", __FUNCTION__, ParamName));
            return FALSE;
        }
        else
        {
            if (bValue == TRUE)
            {
                tele_state = TELE_ST_START;
            }
            else
            {
                tele_state = TELE_ST_STOP;
            }
        }
        pMyObject->Enable = bValue;
        CcspTraceDebug(("%s: Telemetry Enable Set Value is %d: and the value after set is %d\n", __FUNCTION__, bValue, pMyObject->Enable));

        return TRUE;
    }
    else if (strcmp(ParamName, "DCMConfigForceDownload") == 0)
    {
        if (pMyObject->Enable == TRUE)  // Reload the service only if the Telemetry feature is enabled
        {
            if (bValue == TRUE)
            {
               tele_state = TELE_ST_RELOAD;
            }
        }
        pMyObject->DCMConfigForceDownload = FALSE;
        return TRUE;
    }
    else
    {
        CcspTraceError(("%s Unknown parameter %s\n", __FUNCTION__, ParamName));
    }

    return FALSE;
}

ANSC_STATUS Telemetry_GetParamStringValue(ANSC_HANDLE hInsContext, char* ParamName, char* pValue)
{
    CcspTraceDebug(("%s Entered\n", __FUNCTION__));
    PCOSA_DATAMODEL_TELEMETRY pMyObject = (PCOSA_DATAMODEL_TELEMETRY)g_pCosaBEManager->hTelemetry;

    if (strcmp(ParamName, "DCMConfigFileURL") == 0)
    {
        syscfg_get(NULL, "T2ConfigURL", pMyObject->DCMConfigFileURL, sizeof(pMyObject->DCMConfigFileURL));
        AnscCopyString(pValue, pMyObject->DCMConfigFileURL);
        CcspTraceDebug(("%s: Copied the DCMConfigFileURL string into the parameter\n", __FUNCTION__));
        return ANSC_STATUS_SUCCESS;
    }

    if (strcmp(ParamName, "UploadRepositoryURL") == 0)
    {
        syscfg_get(NULL, "UploadRepositoryURL", pMyObject->UploadRepositoryURL, sizeof(pMyObject->UploadRepositoryURL));
        AnscCopyString(pValue, pMyObject->UploadRepositoryURL);
        CcspTraceDebug(("%s: Copied the UploadRepositoryURL string into the parameter\n", __FUNCTION__));
        return ANSC_STATUS_SUCCESS;
    }

    return ANSC_STATUS_FAILURE;
}

BOOL Telemetry_SetParamStringValue(ANSC_HANDLE hInsContext, char* ParamName, char* pString)
{
    CcspTraceDebug(("%s Entered\n", __FUNCTION__));
    PCOSA_DATAMODEL_TELEMETRY pMyObject = (PCOSA_DATAMODEL_TELEMETRY)g_pCosaBEManager->hTelemetry;

    if (strcmp(ParamName, "DCMConfigFileURL") == 0)
    {
        if (strcmp(pMyObject->DCMConfigFileURL, pString) == 0)
        {
            CcspTraceDebug(("%s DCMConfigFileURL is already set with same value  %s\n", __FUNCTION__, pString));
            return TRUE;
        }
        if (syscfg_set_commit(NULL, "T2ConfigURL", pString) != 0)
        {
            CcspTraceWarning(("%s: syscfg_set failed for %s\n", __FUNCTION__, ParamName));
            return FALSE;
        }
        AnscCopyString(pMyObject->DCMConfigFileURL, pString);
        if (pMyObject->Enable == TRUE)  // Reload the service only if the Telemetry feature is enabled
        {
            tele_state = TELE_ST_RELOAD;
        }
        CcspTraceDebug(("%s Set value %s: Exit\n", __FUNCTION__, pString));
        return TRUE;
    }

    return FALSE;
}

BOOL Telemetry_Validate ( ANSC_HANDLE hInsContext, char* pReturnParamName, ULONG* puLength )
{
    return TRUE;
}

BOOL Telemetry_Commit ( ANSC_HANDLE hInsContext )
{
    CcspTraceDebug(("%s: Entered\n", __FUNCTION__));
    PCOSA_DATAMODEL_TELEMETRY pMyObject = (PCOSA_DATAMODEL_TELEMETRY)g_pCosaBEManager->hTelemetry;
    switch(tele_state)
        {
	    case TELE_ST_START:
            {
                CcspTraceInfo(("%s: Telemetry is enabled, downloading DCMConfigFile\n", __FUNCTION__));
                CcspTraceInfo(("%s: Starting  Telemetry service\n", __FUNCTION__));
                system("/usr/bin/telemetry2_0 &");
                break;
            }
	    case TELE_ST_STOP:
            {
                CcspTraceInfo(("%s: Telemetry is disabled \n", __FUNCTION__));
                CcspTraceInfo(("%s: Stop Telemetry Service, removing the cronjob (autodownload_dcmconfig.sh) from the cron table\n", __FUNCTION__));
                system("crontab -l | grep -v 'autodownload_dcmconfig.sh' | crontab -");
#ifdef _PUMA6_ARM_
                system("killall -9 telemetry2_0; rpcclient2 'killall -9 telemetry2_0'");
#else
                system("killall -9 telemetry2_0");
#endif
                break;
            }
	    case TELE_ST_RELOAD :
            {
                CcspTraceInfo(("%s: Reloading Telemetry service with new configuration file \n", __FUNCTION__));
                system("killall -12 telemetry2_0");
                break;
            }
       }
    tele_state = TELE_ST_NONE;
    return TRUE;
}

BOOL Telemetry_Rollback ( ANSC_HANDLE hInsContext )
{
    return TRUE;
}

/***********************************************************************
 APIs for Object: Telemetry.DcmDownloadStatus.
***********************************************************************/

ANSC_STATUS DcmDownloadStatus_GetParamStringValue ( ANSC_HANDLE hInsContext, char* ParamName, char* pValue )
{
    CcspTraceDebug(("%s Entered\n", __FUNCTION__));
    PCOSA_DATAMODEL_TELEMETRY pMyObject = (PCOSA_DATAMODEL_TELEMETRY)g_pCosaBEManager->hTelemetry;

    if (strcmp(ParamName, "HTTPStatusString") == 0)
    {
        char buff[256];

        syscfg_get(NULL, "dcm_httpStatusString", buff, sizeof(buff));
        AnscCopyString(pMyObject->pDcmStatus->HTTPStatusString, buff);
        AnscCopyString(pValue, pMyObject->pDcmStatus->HTTPStatusString);
        CcspTraceDebug(("%s Copied the HTTPStatusString into the parameter\n", __FUNCTION__));
        return ANSC_STATUS_SUCCESS;
    }

    if (strcmp(ParamName, "LastSuccessTimestamp") == 0)
    {
        char buff[30];

        syscfg_get(NULL, "dcm_lastSuccessTimestamp", buff, sizeof(buff));
        AnscCopyString(pMyObject->pDcmStatus->LastSuccessTimestamp, buff);
        AnscCopyString(pValue, pMyObject->pDcmStatus->LastSuccessTimestamp);
        CcspTraceDebug(("%s Copied the LastSuccessTimestamp into the parameter\n", __FUNCTION__));
        return ANSC_STATUS_SUCCESS;
    }

    if (strcmp(ParamName, "LastAttemptTimestamp") == 0)
    {
        char buff[30];

        syscfg_get(NULL, "dcm_lastAttemptTimestamp", buff, sizeof(buff));
        AnscCopyString(pMyObject->pDcmStatus->LastAttemptTimestamp, buff);
        AnscCopyString(pValue, pMyObject->pDcmStatus->LastAttemptTimestamp);
        CcspTraceDebug(("%s Copied the LastAttemptTimestamp into the parameter\n", __FUNCTION__));
        return ANSC_STATUS_SUCCESS;
    }

    return ANSC_STATUS_FAILURE;
}

BOOL DcmDownloadStatus_GetParamUlongValue ( ANSC_HANDLE hInsContext, char* ParamName, ULONG* pUlong )
{
    CcspTraceDebug(("%s Entered\n", __FUNCTION__));
    PCOSA_DATAMODEL_TELEMETRY pMyObject = (PCOSA_DATAMODEL_TELEMETRY)g_pCosaBEManager->hTelemetry;

    if (strcmp(ParamName, "HTTPStatus") == 0)
    {
        char buff[12];

        syscfg_get( NULL, "dcm_httpStatus", buff, sizeof(buff));
        pMyObject->pDcmStatus->HTTPStatus = strtol(buff, NULL, 10);
        *pUlong = (ULONG) pMyObject->pDcmStatus->HTTPStatus;
        return TRUE;
    }

    if (strcmp(ParamName, "AttemptCount") == 0)
    {
        char buff[12];

        syscfg_get( NULL, "dcm_attemptCount", buff, sizeof(buff));
        pMyObject->pDcmStatus->AttemptCount = strtol(buff, NULL, 10);
        *pUlong = (ULONG) pMyObject->pDcmStatus->AttemptCount;
        return TRUE;
    }

    return FALSE;
}

BOOL DcmDownloadStatus_Validate ( ANSC_HANDLE hInsContext, char* pReturnParamName, ULONG* puLength )
{
    return TRUE;
}

BOOL DcmDownloadStatus_Commit ( ANSC_HANDLE hInsContext )
{
    return TRUE;
}

BOOL DcmDownloadStatus_Rollback ( ANSC_HANDLE hInsContext )
{
    return TRUE;
}

/***********************************************************************
 APIs for Object: Telemetry.UploadStatus.
***********************************************************************/

ANSC_STATUS UploadStatus_GetParamStringValue ( ANSC_HANDLE hInsContext, char* ParamName, char* pValue )
{
    CcspTraceDebug(("%s Entered\n", __FUNCTION__));
    PCOSA_DATAMODEL_TELEMETRY       pMyObject  = (PCOSA_DATAMODEL_TELEMETRY)g_pCosaBEManager->hTelemetry;

    if (strcmp(ParamName, "HTTPStatusString") == 0)
    {
        char buff[256];

        syscfg_get( NULL, "upload_httpStatusString", buff, sizeof(buff));
        AnscCopyString(pMyObject->pUploadStatus->HTTPStatusString, buff);
        AnscCopyString(pValue, pMyObject->pUploadStatus->HTTPStatusString);
        CcspTraceDebug(("%s Copied the HTTPStatusString into the parameter\n", __FUNCTION__));
        return ANSC_STATUS_SUCCESS;
    }

    if (strcmp(ParamName, "LastSuccessTimestamp") == 0)
    {
        char buff[30];

        syscfg_get( NULL, "upload_lastSuccessTimestamp", buff, sizeof(buff));
        AnscCopyString(pMyObject->pUploadStatus->LastSuccessTimestamp, buff);
        AnscCopyString(pValue, pMyObject->pUploadStatus->LastSuccessTimestamp);
        CcspTraceDebug(("%s Copied the LastSuccessTimestamp into the parameter\n", __FUNCTION__));
        return ANSC_STATUS_SUCCESS;
    }

    if (strcmp(ParamName, "LastAttemptTimestamp") == 0)
    {
        char buff[30];

        syscfg_get( NULL, "upload_lastAttemptTimestamp", buff, sizeof(buff));
        AnscCopyString(pMyObject->pUploadStatus->LastAttemptTimestamp, buff);
        AnscCopyString(pValue, pMyObject->pUploadStatus->LastAttemptTimestamp);
        CcspTraceDebug(("%s Copied the LastAttemptTimestamp into the parameter\n", __FUNCTION__));
        return ANSC_STATUS_SUCCESS;
    }

    return ANSC_STATUS_FAILURE;
}

BOOL UploadStatus_GetParamUlongValue ( ANSC_HANDLE hInsContext, char* ParamName, ULONG* pUlong )
{
    CcspTraceDebug(("%s Entered\n", __FUNCTION__));
    PCOSA_DATAMODEL_TELEMETRY       pMyObject  = (PCOSA_DATAMODEL_TELEMETRY)g_pCosaBEManager->hTelemetry;

    if (strcmp(ParamName, "HTTPStatus") == 0)
    {
        char buff[12];

        syscfg_get( NULL, "upload_httpStatus", buff, sizeof(buff));
        pMyObject->pUploadStatus->HTTPStatus = strtol(buff, NULL, 10);
        *pUlong = pMyObject->pUploadStatus->HTTPStatus;
        return TRUE;
    }

    if (strcmp(ParamName, "AttemptCount") == 0)
    {
        char buff[12];

        syscfg_get( NULL, "upload_attemptCount", buff, sizeof(buff));
        pMyObject->pUploadStatus->AttemptCount = strtol(buff, NULL, 10);
        *pUlong = pMyObject->pUploadStatus->AttemptCount;
        return TRUE;
    }

    return FALSE;
}

BOOL UploadStatus_Validate ( ANSC_HANDLE hInsContext, char* pReturnParamName, ULONG* puLength )
{
    return TRUE;
}

BOOL UploadStatus_Commit ( ANSC_HANDLE hInsContext )
{
    return TRUE;
}

BOOL UploadStatus_Rollback ( ANSC_HANDLE hInsContext )
{
    return TRUE;
}

/***********************************************************************
 APIs for Object: Telemetry.DcmRetryConfig.
***********************************************************************/

BOOL DcmRetryConfig_GetParamUlongValue ( ANSC_HANDLE hInsContext, char* ParamName, ULONG* pUlong )
{
    CcspTraceDebug(("%s Entered\n", __FUNCTION__));
    PCOSA_DATAMODEL_TELEMETRY pMyObject = (PCOSA_DATAMODEL_TELEMETRY)g_pCosaBEManager->hTelemetry;

    if (strcmp(ParamName, "AttemptInterval") == 0)
    {
        char buff[12];

        syscfg_get( NULL, "dcm_retry_attemptInterval", buff, sizeof(buff));
        pMyObject->pRetryConfig->AttemptInterval = strtol(buff, NULL, 10);
        *pUlong = (ULONG) pMyObject->pRetryConfig->AttemptInterval;
        return TRUE;
    }

    if (strcmp(ParamName, "MaxAttempts") == 0)
    {
        char buff[12];

        syscfg_get(NULL, "dcm_retry_maxAttempts", buff, sizeof(buff));
        pMyObject->pRetryConfig->MaxAttempts = strtol(buff, NULL, 10);
        *pUlong = (ULONG) pMyObject->pRetryConfig->MaxAttempts;
        return TRUE;
    }

    return FALSE;
}

BOOL DcmRetryConfig_SetParamUlongValue ( ANSC_HANDLE hInsContext, char* ParamName, ULONG uValue )
{
    CcspTraceDebug(("%s Entered\n", __FUNCTION__));
    PCOSA_DATAMODEL_TELEMETRY pMyObject= (PCOSA_DATAMODEL_TELEMETRY)g_pCosaBEManager->hTelemetry;

    if (strcmp(ParamName, "AttemptInterval") == 0)
    {
        if ( pMyObject->pRetryConfig->AttemptInterval == uValue )
        {
            return TRUE;
        }

        if (uValue < 45 || uValue > 300)    // Range needs to be discussed
        {
            CcspTraceWarning(("%s AttemptInterval value %u is not in range\n", __FUNCTION__, uValue));
            return FALSE;
        }

        if (syscfg_set_u_commit(NULL, "dcm_retry_attemptInterval", uValue) != 0)
        {
            CcspTraceWarning(("%s syscfg set failed for dcm_retry_attemptInterval\n", __FUNCTION__));
            return FALSE;
        }

        pMyObject->pRetryConfig->AttemptInterval = uValue;
        return TRUE;
    }

    if (strcmp(ParamName, "MaxAttempts") == 0)
    {
        if ( pMyObject->pRetryConfig->MaxAttempts == uValue )
        {
            return TRUE;
        }

        if (uValue < 2 || uValue > 5)
        {
            CcspTraceWarning(("%s MaxAttempts value %u is not in range\n", __FUNCTION__, uValue));
            return FALSE;
        }

        if (syscfg_set_u_commit(NULL, "dcm_retry_maxAttempts", uValue) != 0)
        {
            CcspTraceWarning(("%s syscfg set failed for dcm_retry_maxAttempts\n", __FUNCTION__));
            return FALSE;
        }

        pMyObject->pRetryConfig->MaxAttempts = uValue;
        return TRUE;
    }

    return FALSE;
}

BOOL DcmRetryConfig_Validate ( ANSC_HANDLE hInsContext, char* pReturnParamName, ULONG* puLength )
{
    return TRUE;
}

BOOL DcmRetryConfig_Commit ( ANSC_HANDLE hInsContext )
{
    return TRUE;
}

BOOL DcmRetryConfig_Rollback ( ANSC_HANDLE hInsContext )
{
    return TRUE;
}
