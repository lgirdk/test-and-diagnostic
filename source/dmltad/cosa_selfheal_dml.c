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

#include <syscfg/syscfg.h>

#include "ansc_platform.h"
#include "cosa_selfheal_apis.h"
#include "cosa_selfheal_dml.h"
#include "plugin_main_apis.h"

#define DEFAULT_MONITOR_INTERVAL    15 /* in minute */ 
#define DEFAULT_CPU_THRESHOLD       100 /* in percentage */
#define DEFAULT_MEMORY_THRESHOLD    100 /* in percentage */

/***********************************************************************

/***********************************************************************

 APIs for Object:

    SelfHeal.

    *  SelfHeal_GetParamBoolValue
    *  SelfHeal_SetParamBoolValue
    *  SelfHeal_GetParamUlongValue
    *  SelfHeal_SetParamUlongValue
    *  SelfHeal_Validate
    *  SelfHeal_Commit
    *  SelfHeal_Rollback

***********************************************************************/
/**********************************************************************

    caller:     owner of this object

    prototype

        BOOL
        SelfHeal_GetParamBoolValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                BOOL*                        bValue
            );

    description:

        This function is called to retrieve BOOL parameter value;

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                BOOL*                        bValue 
                The buffer of returned BOOL value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL SelfHeal_GetParamBoolValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL*                        bValue
    )
{
    PCOSA_DATAMODEL_SELFHEAL            pMyObject    = (PCOSA_DATAMODEL_SELFHEAL)g_pCosaBEManager->hSelfHeal; 
    if (strcmp(ParamName, "X_RDKCENTRAL-COM_Enable") == 0)
    {
        *bValue = pMyObject->Enable;
        return TRUE;
    }

    if (strcmp(ParamName, "X_RDK_CPUProcAnalyzer_Enable") == 0)
    {
        *bValue = FALSE;
        return TRUE;
    }

    if (strcmp(ParamName, "X_RDKCENTRAL-COM_DNS_PINGTEST_Enable") == 0)
    {
        *bValue = pMyObject->DNSPingTest_Enable;
        return TRUE;
    }

    if (strcmp(ParamName, "X_RDKCENTRAL-COM_DiagnosticMode") == 0)
    {
        *bValue = pMyObject->DiagnosticMode;
        return TRUE;
    }

    if (strcmp(ParamName, "X_RDKCENTRAL-COM_NoWaitLogSync") == 0)
    {
        *bValue = pMyObject->NoWaitLogSync;
        return TRUE;
    }

    return FALSE;
}

/**********************************************************************

    caller:     owner of this object

    prototype

        BOOL
        SelfHeal_SetParamBoolValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                BOOL                        bValue
            );

    description:

        This function is called to set BOOL parameter value;

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                BOOL                        bValue 
                The buffer of returned BOOL value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL SelfHeal_SetParamBoolValue  
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL                        bValue
    )
{
    PCOSA_DATAMODEL_SELFHEAL            pMyObject    = (PCOSA_DATAMODEL_SELFHEAL)g_pCosaBEManager->hSelfHeal;
    char buf[128];

    if (strcmp(ParamName, "X_RDKCENTRAL-COM_Enable") == 0)
    {
        if( pMyObject->Enable == bValue )
        {
            return TRUE;
	}

        memset(buf, 0, sizeof(buf));
        snprintf(buf,sizeof(buf),"%s",bValue ? "true" : "false");
        if (syscfg_set(NULL, "selfheal_enable", buf) != 0)
        {
	    CcspTraceWarning(("%s: syscfg_set failed for %s\n", __FUNCTION__, ParamName));
	    return FALSE;
        }
        else 
        { 
	    if (syscfg_commit() != 0)
	    {
                CcspTraceWarning(("%s: syscfg commit failed for %s\n", __FUNCTION__, ParamName));
		return FALSE;
	    }

	    char cmd[128];
            if ( bValue == TRUE )
            {

                memset(cmd, 0, sizeof(cmd));
                AnscCopyString(cmd, "/usr/ccsp/tad/self_heal_connectivity_test.sh &");
                system(cmd); 

                memset(cmd, 0, sizeof(cmd));
                AnscCopyString(cmd, "/usr/ccsp/tad/resource_monitor.sh &");
                system(cmd); 

                memset(cmd, 0, sizeof(cmd));
                AnscCopyString(cmd, "/usr/ccsp/tad/selfheal_aggressive.sh &");
                system(cmd);
	    }
            else
	    {
                memset(cmd, 0, sizeof(cmd));
                memset(buf, 0, sizeof(buf));
                sprintf(cmd, "busybox pidof self_heal_connectivity_test.sh");
                copy_command_output(cmd, buf, sizeof(buf));
                buf[strlen(buf)] = '\0';

                if (!strcmp(buf, "")) {
	            CcspTraceWarning(("%s: SelfHeal Monitor script is not running\n", __FUNCTION__));
                } else {    
	            CcspTraceWarning(("%s: Stop SelfHeal Monitor script\n", __FUNCTION__));
                    sprintf(cmd, "kill -9 %s", buf);
                    system(cmd);
                }
    
                memset(cmd, 0, sizeof(cmd));
                memset(buf, 0, sizeof(buf));
                sprintf(cmd, "busybox pidof resource_monitor.sh");
                copy_command_output(cmd, buf, sizeof(buf));
                buf[strlen(buf)] = '\0';

                if (!strcmp(buf, "")) {
	            CcspTraceWarning(("%s: Resource Monitor script is not running\n", __FUNCTION__));
                } else {    
	            CcspTraceWarning(("%s: Stop Resource Monitor script\n", __FUNCTION__));
                    sprintf(cmd, "kill -9 %s", buf);
                    system(cmd);
                }   
       
                memset(cmd, 0, sizeof(cmd));
                memset(buf, 0, sizeof(buf));
                sprintf(cmd, "busybox pidof selfheal_aggressive.sh");
                copy_command_output(cmd, buf, sizeof(buf));
                buf[strlen(buf)] = '\0';

                if (!strcmp(buf, "")) {
	            CcspTraceWarning(("%s: Aggressive self heal script is not running\n", __FUNCTION__));
                } else {
	            CcspTraceWarning(("%s: Aggressive self heal script\n", __FUNCTION__));
                    sprintf(cmd, "kill -9 %s", buf);
                    system(cmd);
                }
	    }
	    pMyObject->Enable = bValue;
	}
        return TRUE;
    }

    if (strcmp(ParamName, "X_RDK_CPUProcAnalyzer_Enable") == 0)
    {
        if (bValue) 
        {
            char cmd[128] = "\0";

            memset (cmd, 0, sizeof(cmd));
            memset (buf, 0, sizeof(buf));
            sprintf (cmd, "busybox pidof cpuprocanalyzer");
            copy_command_output(cmd, buf, sizeof(buf));
            buf[strlen(buf)] = '\0';
            CcspTraceWarning(("Value of cmd: %s and Value of buf: %s and Check for strcmp: %d \n", cmd, buf, strcmp(buf, "")));
            if (strcmp(buf, "")) 
            {
                CcspTraceWarning(("%s: CPUProcAnalyzer is already running!\n", __FUNCTION__));
            } 
            else 
            {
                CcspTraceWarning(("%s: Triggering RunCPUProcAnalyzer script\n", __FUNCTION__));
                system("/lib/rdk/RunCPUProcAnalyzer.sh start &");
            }
        }
        return TRUE;
    }

    if (strcmp(ParamName, "X_RDKCENTRAL-COM_DNS_PINGTEST_Enable") == 0)
    {
        if( pMyObject->DNSPingTest_Enable == bValue )
        {
            return TRUE;
        }

		/* To change the PING Test Enable status */
		if ( ANSC_STATUS_SUCCESS == CosaDmlModifySelfHealDNSPingTestStatus( pMyObject, bValue ) )
		{
			return TRUE;
		}
    }

    if (strcmp(ParamName, "X_RDKCENTRAL-COM_DiagnosticMode") == 0)
    {
        if( pMyObject->DiagnosticMode == bValue )
        {
            return TRUE;
        }

		/* To change the diagnostic mode status */
		if ( ANSC_STATUS_SUCCESS == CosaDmlModifySelfHealDiagnosticModeStatus( pMyObject, bValue ) )
		{
			return TRUE;
		}
    }


    if (strcmp(ParamName, "X_RDKCENTRAL-COM_NoWaitLogSync") == 0)
    {
        if( pMyObject->NoWaitLogSync == bValue )
        {
            return TRUE;
        }

        memset(buf, 0, sizeof(buf));	
        if(bValue)
            strcpy(buf,"true");
        else
            strcpy(buf,"false");

        if (syscfg_set(NULL, "log_backup_enable", buf) != 0)
        {
            AnscTraceWarning(("syscfg_set failed\n"));
        }
        else
        {
            if (syscfg_commit() != 0)
            {
                AnscTraceWarning(("syscfg_commit failed\n"));
            }
            else
            {
                pMyObject->NoWaitLogSync = bValue;
            }
        }

        return TRUE;
    }

    return FALSE;
}

/**********************************************************************

    caller:     owner of this object

    prototype

        BOOL
        SelfHeal_GetParamUlongValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                ULONG*                      puLong
            );

    description:

        This function is called to retrieve ULONG parameter value;

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                ULONG*                      puLong
                The buffer of returned ULONG value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
SelfHeal_GetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        ULONG*                      puLong
    )
{
    PCOSA_DATAMODEL_SELFHEAL            pMyObject    = (PCOSA_DATAMODEL_SELFHEAL)g_pCosaBEManager->hSelfHeal; 


	if (strcmp(ParamName, "X_RDKCENTRAL-COM_FreeMemThreshold") == 0)
	{
			*puLong = pMyObject->FreeMemThreshold;
			return TRUE;
	}

	if (strcmp(ParamName, "X_RDKCENTRAL-COM_MemFragThreshold") == 0)
	{
			*puLong = pMyObject->MemFragThreshold;
			return TRUE;
	}

	if (strcmp(ParamName, "X_RDKCENTRAL-COM_CpuMemFragInterval") == 0)
	{
			*puLong = pMyObject->CpuMemFragInterval;
			return TRUE;
	}

    if (strcmp(ParamName, "X_RDKCENTRAL-COM_MaxRebootCount") == 0)
    {
        *puLong = pMyObject->MaxRebootCnt;
        return TRUE;
    }

    if (strcmp(ParamName, "X_RDKCENTRAL-COM_MaxResetCount") == 0)
    {
        *puLong = pMyObject->MaxResetCnt;
        return TRUE;
    }

    if (strcmp(ParamName, "X_RDKCENTRAL-COM_DiagMode_LogUploadFrequency") == 0)
    {
        *puLong = pMyObject->DiagModeLogUploadFrequency;
        return TRUE;
    }

    if (strcmp(ParamName, "X_RDKCENTRAL-COM_LogBackupThreshold") == 0)
    {
        *puLong = pMyObject->LogBackupThreshold;
        return TRUE;
    }

    return FALSE;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL
        SelfHeal_SetParamUlongValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                ULONG                       uValue
            );

    description:

        This function is called to set ULONG parameter value;

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                ULONG                       uValue
                The updated ULONG value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
SelfHeal_SetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        ULONG                       uValue
    )
{
    PCOSA_DATAMODEL_SELFHEAL            pMyObject    = (PCOSA_DATAMODEL_SELFHEAL)g_pCosaBEManager->hSelfHeal;
    char buf[8];

	if (strcmp(ParamName, "X_RDKCENTRAL-COM_FreeMemThreshold") == 0)
	{
		memset(buf, 0, sizeof(buf));
		snprintf(buf,sizeof(buf),"%d",uValue);

		if (syscfg_set(NULL, "Free_Mem_Threshold", buf) != 0)
		{
			CcspTraceWarning(("%s: syscfg_set failed for %s\n", __FUNCTION__, ParamName));
			return FALSE;
		}
		if (syscfg_commit() != 0)
		{
			CcspTraceWarning(("%s: syscfg commit failed for %s\n", __FUNCTION__, ParamName));
			return FALSE;
		}
                CcspTraceWarning(("%s : %d \n",ParamName,uValue));
		pMyObject->FreeMemThreshold = uValue;
		return TRUE;
	}

	if (strcmp(ParamName, "X_RDKCENTRAL-COM_MemFragThreshold") == 0)
	{
		memset(buf, 0, sizeof(buf));
		snprintf(buf,sizeof(buf),"%d",uValue);

		if (syscfg_set(NULL, "Mem_Frag_Threshold", buf) != 0)
		{
			CcspTraceWarning(("%s: syscfg_set failed for %s\n", __FUNCTION__, ParamName));
			return FALSE;
		}
		if (syscfg_commit() != 0)
		{
			CcspTraceWarning(("%s: syscfg commit failed for %s\n", __FUNCTION__, ParamName));
			return FALSE;
		}
                CcspTraceWarning(("%s : %d \n",ParamName,uValue));
		pMyObject->MemFragThreshold = uValue;
		return TRUE;
	}

	if (strcmp(ParamName, "X_RDKCENTRAL-COM_CpuMemFragInterval") == 0)
	{
		if((uValue >= 1) && (uValue <= 120))
		{
			memset(buf, 0, sizeof(buf));
			snprintf(buf,sizeof(buf),"%d",uValue);

			if (syscfg_set(NULL, "CpuMemFrag_Interval", buf) != 0)
			{
					CcspTraceWarning(("%s: syscfg_set failed for %s\n", __FUNCTION__, ParamName));
					return FALSE;
			}
			if (syscfg_commit() != 0)
			{
					CcspTraceWarning(("%s: syscfg commit failed for %s\n", __FUNCTION__, ParamName));
					return FALSE;
			}
			pMyObject->CpuMemFragInterval = uValue;

			CpuMemFragCronSchedule(uValue,TRUE);

			return TRUE;
		}
		else
		{
			CcspTraceWarning(("%s: [ParamName: %s] Please Enter Value between 1 to 120 hrs \n", __FUNCTION__, ParamName));
		}
	}

    if (strcmp(ParamName, "X_RDKCENTRAL-COM_MaxRebootCount") == 0)
    {
        if( pMyObject->MaxRebootCnt == uValue )
        {
            return TRUE;
	}

        memset(buf, 0, sizeof(buf));
        snprintf(buf,sizeof(buf),"%d",uValue);
        if (syscfg_set(NULL, "max_reboot_count", buf) != 0)
        {
			CcspTraceWarning(("%s: syscfg_set failed for %s\n", __FUNCTION__, ParamName));
			return FALSE;
        }
		if (syscfg_commit() != 0)
		{
            CcspTraceWarning(("%s: syscfg commit failed for %s\n", __FUNCTION__, ParamName));
			return FALSE;
		}
        pMyObject->MaxRebootCnt = uValue;
        return TRUE;
    }

    if (strcmp(ParamName, "X_RDKCENTRAL-COM_MaxResetCount") == 0)
    {
        if( pMyObject->MaxResetCnt == uValue )
        {
            return TRUE;
        }

        memset(buf, 0, sizeof(buf));
        snprintf(buf,sizeof(buf),"%d",uValue);
        if (syscfg_set(NULL, "max_reset_count", buf) != 0)
        {
            CcspTraceWarning(("%s: syscfg_set failed for %s\n", __FUNCTION__, ParamName));
            return FALSE;
        }
        if (syscfg_commit() != 0)
        {
            CcspTraceWarning(("%s: syscfg commit failed for %s\n", __FUNCTION__, ParamName));
            return FALSE;
        }
        pMyObject->MaxResetCnt = uValue;
        return TRUE;
    }

    if (strcmp(ParamName, "X_RDKCENTRAL-COM_DiagMode_LogUploadFrequency") == 0)
    {
        if( pMyObject->DiagModeLogUploadFrequency == uValue )
        {
            return TRUE;
        }

        memset(buf, 0, sizeof(buf));
        snprintf(buf,sizeof(buf),"%d",uValue);
        if (syscfg_set(NULL, "diagMode_LogUploadFrequency", buf) != 0)
        {
            CcspTraceWarning(("%s: syscfg_set failed for %s\n", __FUNCTION__, ParamName));
            return FALSE;
        }
        if (syscfg_commit() != 0)
        {
            CcspTraceWarning(("%s: syscfg commit failed for %s\n", __FUNCTION__, ParamName));
            return FALSE;
        }
        pMyObject->DiagModeLogUploadFrequency = uValue;

		/* Modify the cron scheduling based on configured Loguploadfrequency */
		CosaSelfHealAPIModifyCronSchedule( FALSE );
		
        return TRUE;
    }

    if (strcmp(ParamName, "X_RDKCENTRAL-COM_LogBackupThreshold") == 0)
    {
        if( pMyObject->LogBackupThreshold == uValue )
        {
            return TRUE;
        }

	memset(buf, 0, sizeof(buf));
        sprintf(buf, "%d", uValue);

        if (syscfg_set(NULL, "log_backup_threshold", buf) != 0)
        {
            AnscTraceWarning(("syscfg_set failed\n"));
        }
        else
        {
            if (syscfg_commit() != 0)
            {
                AnscTraceWarning(("syscfg_commit failed\n"));
            }
            else
            {
                pMyObject->LogBackupThreshold = uValue;
            }
        }

        return TRUE;
    }

    return FALSE;
}

ULONG
SelfHeal_GetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pValue,
        ULONG*                      pUlSize
    )

{
    PCOSA_DATAMODEL_SELFHEAL            pMyObject    = (PCOSA_DATAMODEL_SELFHEAL)g_pCosaBEManager->hSelfHeal; 

    /* check the parameter name and return the corresponding value */
    if (strcmp(ParamName, "X_RDKCENTRAL-COM_DNS_URL") == 0)
    {
        /* collect value */
        if ( AnscSizeOfString(pMyObject->DNSPingTest_URL) < *pUlSize)
        {
            AnscCopyString(pValue, pMyObject->DNSPingTest_URL);
		    return 0;
        }
        else
        {
            *pUlSize = AnscSizeOfString(pMyObject->DNSPingTest_URL)+1;
            return 1;
        }
    }
	    return -1;
}

BOOL
SelfHeal_SetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       strValue
    )

{
    PCOSA_DATAMODEL_SELFHEAL            pMyObject    = (PCOSA_DATAMODEL_SELFHEAL)g_pCosaBEManager->hSelfHeal; 

	if (strcmp(ParamName, "X_RDKCENTRAL-COM_DNS_URL") == 0)
    {
		if ( ANSC_STATUS_SUCCESS == CosaDmlModifySelfHealDNSPingTestURL( pMyObject, strValue ) )
		{
			return TRUE;
		}
	}
	
    	return FALSE;
}

/**********************************************************************^M

    caller:     owner of this object

    prototype:

        BOOL
        SelfHeal_Validate
            (
                ANSC_HANDLE                 hInsContext,
                char*                       pReturnParamName,
                ULONG*                      puLength
            );

    description:

        This function is called to finally commit all the update.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       pReturnParamName,
                The buffer (128 bytes) of parameter name if there's a validation.

                ULONG*                      puLength
                The output length of the param name

    return:     TRUE if there's no validation.

**********************************************************************/
BOOL
SelfHeal_Validate
   (
        ANSC_HANDLE                 hInsContext,
        char*                       pReturnParamName,
        ULONG*                      puLength
    )
{
    return TRUE;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        ULONG
	SelfHeal_Commit
            (
                ANSC_HANDLE                 hInsContext
            );

    description:

        This function is called to finally commit all the update.

    argument:   ANSC_HANDLE                 hInsContext,
               The instance handle;

    return:     The status of the operation.

**********************************************************************/
ULONG
SelfHeal_Commit
    (
        ANSC_HANDLE                 hInsContext
    )
{
    return ANSC_STATUS_SUCCESS;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        ULONG
        SelfHeal_Rollback
            (
                ANSC_HANDLE                 hInsContext
            );

    description:

        This function is called to roll back the update whenever there's a
        validation found.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The status of the operation.

**********************************************************************/
ULONG
SelfHeal_Rollback
    (
        ANSC_HANDLE                 hInsContext
    )
{
    return ANSC_STATUS_SUCCESS;
}

/***********************************************************************

 APIs for Object:

    SelfHeal.ConnectivityTest.

    *  ConnectivityTest_GetParamBoolValue
    *  ConnectivityTest_SetParamBoolValue
    *  ConnectivityTest_GetParamUlongValue
    *  ConnectivityTest_SetParamUlongValue
    *  ConnectivityTest_GetParamIntValue
    *  ConnectivityTest_SetParamIntValue
    *  ConnectivityTest_Validate
    *  ConnectivityTest_Commit
    *  ConnectivityTest_Rollback

***********************************************************************/



BOOL ConnectivityTest_GetParamBoolValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL*                        bValue
    )
{
    PCOSA_DATAMODEL_SELFHEAL            pMyObject    = (PCOSA_DATAMODEL_SELFHEAL)g_pCosaBEManager->hSelfHeal; 
    if (strcmp(ParamName, "X_RDKCENTRAL-COM_CorrectiveAction") == 0)
    {
        *bValue = pMyObject->pConnTest->CorrectiveAction;
        return TRUE;
    }

    return FALSE;
}

/**********************************************************************

    caller:     owner of this object

    prototype

        BOOL
        ConnectivityTest_SetParamBoolValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                BOOL                        bValue
            );

    description:

        This function is called to set BOOL parameter value;

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                BOOL                        bValue 
                The buffer of returned BOOL value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL ConnectivityTest_SetParamBoolValue  
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL                        bValue
    )
{
    PCOSA_DATAMODEL_SELFHEAL            pMyObject    = (PCOSA_DATAMODEL_SELFHEAL)g_pCosaBEManager->hSelfHeal; 
    if (strcmp(ParamName, "X_RDKCENTRAL-COM_CorrectiveAction") == 0)
    {
        if ( pMyObject->pConnTest->CorrectiveAction == bValue )
        {
            return  TRUE;
        }
	CcspTraceWarning(("%s Changing X_RDKCENTRAL-COM_CorrectiveAction state to %d \n",__FUNCTION__,bValue));
        /* save update to backup */
        char buf[128];
        memset(buf, 0, sizeof(buf));
        snprintf(buf,sizeof(buf),"%s",bValue ? "true" : "false");
		if (syscfg_set(NULL, "ConnTest_CorrectiveAction", buf) != 0)
		{
			CcspTraceWarning(("%s syscfg set failed for ConnTest_CorrectiveAction\n",__FUNCTION__));
			return FALSE;
		}
    	if (syscfg_commit() != 0)
		{
        	CcspTraceWarning(("%s syscfg commit failed for ConnTest_CorrectiveAction\n",__FUNCTION__));
		    return FALSE;
        }
        pMyObject->pConnTest->CorrectiveAction = bValue;
        return TRUE;
    }
    return FALSE;
}


BOOL
ConnectivityTest_GetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        ULONG*                      pUlong
    )
{

    PCOSA_DATAMODEL_SELFHEAL            pMyObject           = (PCOSA_DATAMODEL_SELFHEAL)g_pCosaBEManager->hSelfHeal;
    if (strcmp(ParamName, "X_RDKCENTRAL-COM_PingInterval") == 0)
    {
        /* collect value */
		*pUlong = pMyObject->pConnTest->PingInterval; 
        return TRUE;
    }

    if (strcmp(ParamName, "X_RDKCENTRAL-COM_NumPingsPerServer") == 0)
    {
        /* collect value */
		*pUlong = pMyObject->pConnTest->PingCount; 
        return TRUE;
    }

    if (strcmp(ParamName, "X_RDKCENTRAL-COM_MinNumPingServer") == 0)
    {
        /* collect value */
		*pUlong = pMyObject->pConnTest->MinPingServer; 
        return TRUE;
    }

    if (strcmp(ParamName, "X_RDKCENTRAL-COM_PingRespWaitTime") == 0)
    {
        /* collect value */
		*pUlong = pMyObject->pConnTest->WaitTime; 
        return TRUE;
    }
    
    if (strcmp(ParamName, "X_RDKCENTRAL-COM_LastReboot") == 0)
    {
        /* collect value */
        char buf[64]={0};
        syscfg_get( NULL, "last_router_reboot_time", buf, sizeof(buf));
    	if( buf[0] != '\0' )
    	{
    		    *pUlong = atoi(buf);
		     return TRUE;
    	}
    }

    return FALSE;
}

BOOL
ConnectivityTest_GetParamIntValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        int*                        pInt
    )
{
    PCOSA_DATAMODEL_SELFHEAL            pMyObject           = (PCOSA_DATAMODEL_SELFHEAL)g_pCosaBEManager->hSelfHeal;

    if (strcmp(ParamName, "X_RDKCENTRAL-COM_RebootInterval") == 0)
    {
        /* collect value */
	    *pInt = pMyObject->pConnTest->RouterRebootInterval ;
	    return TRUE;
    }

    if (strcmp(ParamName, "X_RDKCENTRAL-COM_CurrentCount") == 0)
    {
        /* collect value */
        char buf[16]={0};
        syscfg_get( NULL, "todays_reset_count", buf, sizeof(buf));
        if( buf[0] != '\0' )
        {
                    *pInt = atoi(buf);
                     return TRUE;
        }
    }

    return FALSE;
}

BOOL
ConnectivityTest_SetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        ULONG                       uValue
    )

{

    PCOSA_DATAMODEL_SELFHEAL            pMyObject           = (PCOSA_DATAMODEL_SELFHEAL)g_pCosaBEManager->hSelfHeal;
    if (strcmp(ParamName, "X_RDKCENTRAL-COM_PingInterval") == 0)
    {
        if ( pMyObject->pConnTest->PingInterval == uValue )
        {
            return  TRUE;
        }
        
		if ( uValue < 15 || uValue > 1440 )
		{
			CcspTraceWarning(("%s PingInterval value passed is not in range\n",__FUNCTION__));
			return FALSE;
		}
        /* save update to backup */
		char value[10];
		snprintf(value, sizeof(value), "%u", uValue);
		if (syscfg_set(NULL, "ConnTest_PingInterval", value) != 0)
		{
			CcspTraceWarning(("%s syscfg set failed for ConnTest_PingInterval\n",__FUNCTION__));
			return FALSE;
		}
    	if (syscfg_commit() != 0)
		{
        	CcspTraceWarning(("%s syscfg commit failed for ConnTest_PingInterval\n",__FUNCTION__));
		    return FALSE;
        }
        pMyObject->pConnTest->PingInterval = uValue;
        return TRUE;
    }

    if (strcmp(ParamName, "X_RDKCENTRAL-COM_NumPingsPerServer") == 0)
    {
        if ( pMyObject->pConnTest->PingCount == uValue )
        {
            return  TRUE;
        }
         
		char value[10];
		snprintf(value, sizeof(value), "%u", uValue);
		if (syscfg_set(NULL, "ConnTest_NumPingsPerServer", value) != 0)
		{
			CcspTraceWarning(("%s syscfg set failed for ConnTest_NumPingsPerServer\n",__FUNCTION__));
			return FALSE;
		}
    	if (syscfg_commit() != 0)
		{
        	CcspTraceWarning(("%s syscfg commit failed for ConnTest_NumPingsPerServer\n",__FUNCTION__));
		    return FALSE;
        }
        /* save update to backup */
        pMyObject->pConnTest->PingCount = uValue;
        return TRUE;
    }

    if (strcmp(ParamName, "X_RDKCENTRAL-COM_MinNumPingServer") == 0)
    {
        if ( pMyObject->pConnTest->MinPingServer == uValue )
        {
            return  TRUE;
        }
   
		char value[10];
		snprintf(value, sizeof(value), "%u", uValue);
		if (syscfg_set(NULL, "ConnTest_MinNumPingServer", value) != 0)
		{
			CcspTraceWarning(("%s syscfg set failed for ConnTest_MinNumPingServer\n",__FUNCTION__));
			return FALSE;
		}
    	if (syscfg_commit() != 0)
		{
        	CcspTraceWarning(("%s syscfg commit failed for ConnTest_MinNumPingServer\n",__FUNCTION__));
		    return FALSE;
        }     
        /* save update to backup */
		pMyObject->pConnTest->MinPingServer = uValue;
        return TRUE;
    }

    if (strcmp(ParamName, "X_RDKCENTRAL-COM_PingRespWaitTime") == 0)
    {
        if ( pMyObject->pConnTest->WaitTime == uValue )
        {
            return  TRUE;
        }
		char value[10];
        snprintf(value, sizeof(value), "%u", uValue);
		if (syscfg_set(NULL, "ConnTest_PingRespWaitTime", value) != 0)
		{
			CcspTraceWarning(("%s syscfg set failed for ConnTest_PingRespWaitTime\n",__FUNCTION__));
			return FALSE;
		}
    	if (syscfg_commit() != 0)
		{
        	CcspTraceWarning(("%s syscfg commit failed for ConnTest_PingRespWaitTime\n",__FUNCTION__));
		    return FALSE;
        }     
        /* save update to backup */
		pMyObject->pConnTest->WaitTime = uValue;
        return TRUE;
    }
    
    return FALSE;
}

BOOL
ConnectivityTest_SetParamIntValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        int                         pInt
    )
{
    PCOSA_DATAMODEL_SELFHEAL            pMyObject           = (PCOSA_DATAMODEL_SELFHEAL)g_pCosaBEManager->hSelfHeal;

    if (strcmp(ParamName, "X_RDKCENTRAL-COM_RebootInterval") == 0)
    {
        if ( pMyObject->pConnTest->RouterRebootInterval == pInt )
        {
            return  TRUE;
        }
        char cValue[10];
        memset(cValue, 0, sizeof(cValue));
        snprintf(cValue, sizeof(cValue), "%d", pInt);
        if (syscfg_set(NULL, "router_reboot_Interval", cValue) != 0)
        {
		CcspTraceWarning(("%s syscfg set failed for X_RDKCENTRAL-COM_RebootInterval\n",__FUNCTION__));
		return FALSE;
	}
	if (syscfg_commit() != 0)
	{
		CcspTraceWarning(("%s syscfg commit failed for X_RDKCENTRAL-COM_RebootInterval\n",__FUNCTION__));
		return FALSE;
        }     
        /* save update to backup */
	pMyObject->pConnTest->RouterRebootInterval = pInt;
	return TRUE;
    }
    
    return FALSE;
}

BOOL
ConnectivityTest_Validate
    (
        ANSC_HANDLE                 hInsContext,
        char*                       pReturnParamName,
        ULONG*                      puLength
    )
{
	return TRUE;
}

ULONG
ConnectivityTest_Commit
    (
        ANSC_HANDLE                 hInsContext
    )
{
	return TRUE;
}

ULONG
ConnectivityTest_Rollback
    (
        ANSC_HANDLE                 hInsContext
    )
{
	return TRUE;
}

/***********************************************************************

 APIs for Object:

    SelfHeal.ConnectivityTest.PingServerList.IPv4PingServerTable.{i}.

    *  IPv4PingServerTable_GetEntryCount
    *  IPv4PingServerTable_GetEntry
    *  IPv4PingServerTable_IsUpdated
    *  IPv4PingServerTable_Synchronize
    *  IPv4PingServerTable_AddEntry
    *  IPv4PingServerTable_DelEntry
    *  IPv4PingServerTable_GetParamStringValue
    *  IPv4PingServerTable_SetParamStringValue
    *  IPv4PingServerTable_Validate
    *  IPv4PingServerTable_Commit
    *  IPv4PingServerTable_Rollback

***********************************************************************/

ULONG
IPv4PingServerTable_GetEntryCount
    (
        ANSC_HANDLE hInsContext
    )

{

    PCOSA_DATAMODEL_SELFHEAL            pMyObject           = (PCOSA_DATAMODEL_SELFHEAL)g_pCosaBEManager->hSelfHeal;
    return AnscSListQueryDepth( &pMyObject->IPV4PingServerList );

}

ANSC_HANDLE
IPv4PingServerTable_GetEntry
    (
        ANSC_HANDLE                 hInsContext,
        ULONG                       nIndex,
        ULONG*                      pInsNumber
    )
{

    PCOSA_DATAMODEL_SELFHEAL                   pMyObject         = (PCOSA_DATAMODEL_SELFHEAL)g_pCosaBEManager->hSelfHeal;
    PSINGLE_LINK_ENTRY                    pSListEntry       = NULL;
    PCOSA_CONTEXT_SELFHEAL_LINK_OBJECT    pCxtLink          = NULL;
    pSListEntry       = AnscSListGetEntryByIndex(&pMyObject->IPV4PingServerList, nIndex);
    if ( pSListEntry )
    {
        pCxtLink      = ACCESS_COSA_CONTEXT_SELFHEAL_LINK_OBJECT(pSListEntry);
        *pInsNumber   = pCxtLink->InstanceNumber;
    }
    return (ANSC_HANDLE)pSListEntry;
}

BOOL
IPv4PingServerTable_IsUpdated
    (
        ANSC_HANDLE                 hInsContext
    )
{
    PCOSA_DATAMODEL_SELFHEAL             SELFHEAL    = (PCOSA_DATAMODEL_SELFHEAL)g_pCosaBEManager->hSelfHeal;
    BOOL                            bIsUpdated   = TRUE;
    return bIsUpdated;
}

ULONG
IPv4PingServerTable_Synchronize
    (
        ANSC_HANDLE                 hInsContext
    )
{

    ANSC_STATUS                           returnStatus      = ANSC_STATUS_FAILURE;
    PCOSA_DATAMODEL_SELFHEAL             SELFHEAL    = (PCOSA_DATAMODEL_SELFHEAL)g_pCosaBEManager->hSelfHeal;
    PCOSA_CONTEXT_SELFHEAL_LINK_OBJECT    pCxtLink          = NULL;
    PSINGLE_LINK_ENTRY                    pSListEntry       = NULL;
    PSINGLE_LINK_ENTRY                    pSListEntry2      = NULL;
    ULONG                                 entryCount        = 0;
}

ANSC_HANDLE
IPv4PingServerTable_AddEntry
    (
        ANSC_HANDLE                 hInsContext,
        ULONG*                      pInsNumber
    )
{
	PCOSA_DATAMODEL_SELFHEAL             pSelfHeal              = (PCOSA_DATAMODEL_SELFHEAL)g_pCosaBEManager->hSelfHeal;
    PCOSA_DML_SELFHEAL_IPv4_SERVER_TABLE pServerIpv4 = NULL;
    PCOSA_CONTEXT_SELFHEAL_LINK_OBJECT   pSelfHealCxtLink  = NULL;
    PSINGLE_LINK_ENTRY                   pSListEntry       = NULL;
    ANSC_STATUS                          returnStatus      = ANSC_STATUS_SUCCESS;
    CHAR                                 tmpBuff[64]       = {0};
	CHAR 								 buf[8];
    BOOL                                      bridgeMode;
	PCOSA_DATAMODEL_SELFHEAL                   pMyObject         = (PCOSA_DATAMODEL_SELFHEAL)g_pCosaBEManager->hSelfHeal;
	int Qdepth = 0;

    pServerIpv4 = (PCOSA_DML_SELFHEAL_IPv4_SERVER_TABLE)AnscAllocateMemory(sizeof(COSA_DML_SELFHEAL_IPv4_SERVER_TABLE));
    if ( !pServerIpv4 )
    {
		CcspTraceWarning(("%s resource allocation failed\n",__FUNCTION__));
        return NULL;
    }
 
	pSelfHealCxtLink = (PCOSA_CONTEXT_SELFHEAL_LINK_OBJECT)AnscAllocateMemory(sizeof(COSA_CONTEXT_SELFHEAL_LINK_OBJECT));
    if ( !pSelfHealCxtLink )
    {
        goto EXIT;
    }
	
	Qdepth = AnscSListQueryDepth( &pSelfHeal->IPV4PingServerList );

	pSelfHealCxtLink->InstanceNumber =  pSelfHeal->ulIPv4NextInstanceNumber;
	pServerIpv4->InstanceNumber = pSelfHeal->ulIPv4NextInstanceNumber;
	    pSelfHeal->ulIPv4NextInstanceNumber++;
    if (pSelfHeal->ulIPv4NextInstanceNumber == 0)
        pSelfHeal->ulIPv4NextInstanceNumber = 1;
	
    /* now we have this link content */
	pSelfHealCxtLink->hContext = (ANSC_HANDLE)pServerIpv4;

	pSelfHeal->pConnTest->IPv4EntryCount++;
	printf("*** pSelfHeal->pConnTest->IPv4EntryCount = %d ***\n",pSelfHeal->pConnTest->IPv4EntryCount);
	snprintf(buf,sizeof(buf),"%d",pSelfHeal->pConnTest->IPv4EntryCount);
	if (syscfg_set(NULL, "Ipv4PingServer_Count", buf) != 0) 
	{
		CcspTraceWarning(("syscfg_set failed\n"));
	}
	else 
	{
		if (syscfg_commit() != 0) 
		{
			CcspTraceWarning(("syscfg_commit failed\n"));
		}
	}
	CosaSListPushEntryByInsNum(&pSelfHeal->IPV4PingServerList, (PCOSA_CONTEXT_LINK_OBJECT)pSelfHealCxtLink);
    return (ANSC_HANDLE)pSelfHealCxtLink;

EXIT:
    AnscFreeMemory(pServerIpv4);

    return NULL;

}

ULONG
IPv4PingServerTable_DelEntry
    (
        ANSC_HANDLE                 hInsContext,
        ANSC_HANDLE                 hInstance
    )

{
    ANSC_STATUS                          returnStatus      = ANSC_STATUS_SUCCESS;
    PCOSA_DATAMODEL_SELFHEAL             pSelfHeal               = (PCOSA_DATAMODEL_SELFHEAL)g_pCosaBEManager->hSelfHeal;
    PCOSA_CONTEXT_SELFHEAL_LINK_OBJECT   pSelfHealCxtLink   = (PCOSA_CONTEXT_SELFHEAL_LINK_OBJECT)hInstance;
    PCOSA_DML_SELFHEAL_IPv4_SERVER_TABLE pServerIpv4      = (PCOSA_DML_SELFHEAL_IPv4_SERVER_TABLE)pSelfHealCxtLink->hContext;
	/* Remove entery from the database */
	returnStatus = RemovePingServerURI(PingServerType_IPv4, pServerIpv4->InstanceNumber);
    if ( returnStatus == ANSC_STATUS_SUCCESS )
	{
			/* Remove entery from the Queue */
        if(AnscSListPopEntryByLink(&pSelfHeal->IPV4PingServerList, &pSelfHealCxtLink->Linkage) == TRUE)
		{
			AnscFreeMemory(pSelfHealCxtLink->hContext);
			AnscFreeMemory(pSelfHealCxtLink);
		}
		else
		{
			return ANSC_STATUS_FAILURE;
		}
	}
    return returnStatus;
}

ULONG
IPv4PingServerTable_GetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pValue,
        ULONG*                      pUlSize
    )

{
    PCOSA_CONTEXT_SELFHEAL_LINK_OBJECT   pSelfHealCxtLink     = (PCOSA_CONTEXT_SELFHEAL_LINK_OBJECT)hInsContext;
    PCOSA_DML_SELFHEAL_IPv4_SERVER_TABLE pServerIpv4  = (PCOSA_DML_SELFHEAL_IPv4_SERVER_TABLE)pSelfHealCxtLink->hContext;
    PUCHAR                                    pString       = NULL;

    /* check the parameter name and return the corresponding value */
    if (strcmp(ParamName, "X_RDKCENTRAL-COM_Ipv4PingServerURI") == 0)
    {
        /* collect value */
        if ( AnscSizeOfString(pServerIpv4->Ipv4PingServerURI) < *pUlSize)
        {
            AnscCopyString(pValue, pServerIpv4->Ipv4PingServerURI);
            return 0;
        }
        else
        {
            *pUlSize = AnscSizeOfString(pServerIpv4->Ipv4PingServerURI)+1;
            return 1;
        }
    }
	    return -1;
}

BOOL
IPv4PingServerTable_SetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       strValue
    )

{
	PCOSA_CONTEXT_SELFHEAL_LINK_OBJECT   pSelfHealCxtLink     = (PCOSA_CONTEXT_SELFHEAL_LINK_OBJECT)hInsContext;
    PCOSA_DML_SELFHEAL_IPv4_SERVER_TABLE pServerIpv4  = (PCOSA_DML_SELFHEAL_IPv4_SERVER_TABLE)pSelfHealCxtLink->hContext;
	if (strcmp(ParamName, "X_RDKCENTRAL-COM_Ipv4PingServerURI") == 0)
    {
		 AnscCopyString(pServerIpv4->Ipv4PingServerURI,strValue);
		 /* Add entery in the database */
		 SavePingServerURI(PingServerType_IPv4, strValue, pServerIpv4->InstanceNumber);
		 return TRUE;
	}
	return FALSE;
}

BOOL
IPv4PingServerTable_Validate
    (
        ANSC_HANDLE                 hInsContext,
        char*                       pReturnParamName,
        ULONG*                      puLength
    )

{
    return TRUE;
}

ULONG
IPv4PingServerTable_Commit
    (
        ANSC_HANDLE                 hInsContext
    )

{
    return 0;
}

ULONG
IPv4PingServerTable_Rollback
    (
        ANSC_HANDLE                 hInsContext
    )

{
    return 0;
}


/***********************************************************************

 APIs for Object:

    SelfHeal.ConnectivityTest.PingServerList.IPv6PingServerTable.{i}.

    *  IPv6PingServerTable_GetEntryCount
    *  IPv6PingServerTable_GetEntry
    *  IPv6PingServerTable_IsUpdated
    *  IPv6PingServerTable_Synchronize
    *  IPv6PingServerTable_AddEntry
    *  IPv6PingServerTable_DelEntry
    *  IPv6PingServerTable_GetParamStringValue
    *  IPv6PingServerTable_SetParamStringValue
    *  IPv6PingServerTable_Validate
    *  IPv6PingServerTable_Commit
    *  IPv6PingServerTable_Rollback

***********************************************************************/

ULONG
IPv6PingServerTable_GetEntryCount
    (
        ANSC_HANDLE hInsContext
    )

{

    PCOSA_DATAMODEL_SELFHEAL            pMyObject           = (PCOSA_DATAMODEL_SELFHEAL)g_pCosaBEManager->hSelfHeal;
    return AnscSListQueryDepth( &pMyObject->IPV6PingServerList );

}

ANSC_HANDLE
IPv6PingServerTable_GetEntry
    (
        ANSC_HANDLE                 hInsContext,
        ULONG                       nIndex,
        ULONG*                      pInsNumber
    )
{

    PCOSA_DATAMODEL_SELFHEAL                   pMyObject         = (PCOSA_DATAMODEL_SELFHEAL)g_pCosaBEManager->hSelfHeal;
    PSINGLE_LINK_ENTRY                    pSListEntry       = NULL;
    PCOSA_CONTEXT_SELFHEAL_LINK_OBJECT    pCxtLink          = NULL;
    pSListEntry       = AnscSListGetEntryByIndex(&pMyObject->IPV6PingServerList, nIndex);
    if ( pSListEntry )
    {
        pCxtLink      = ACCESS_COSA_CONTEXT_SELFHEAL_LINK_OBJECT(pSListEntry);
        *pInsNumber   = pCxtLink->InstanceNumber;
    }
    return (ANSC_HANDLE)pSListEntry;
}

BOOL
IPv6PingServerTable_IsUpdated
    (
        ANSC_HANDLE                 hInsContext
    )
{
    PCOSA_DATAMODEL_SELFHEAL             SELFHEAL    = (PCOSA_DATAMODEL_SELFHEAL)g_pCosaBEManager->hSelfHeal;
    BOOL                            bIsUpdated   = TRUE;
    /*
        We can use one rough granularity interval to get whole table in case
        that the updating is too frequent.
        */
      return bIsUpdated;
}

ULONG
IPv6PingServerTable_Synchronize
    (
        ANSC_HANDLE                 hInsContext
    )
{

    ANSC_STATUS                           returnStatus      = ANSC_STATUS_FAILURE;
    PCOSA_DATAMODEL_SELFHEAL             SELFHEAL    = (PCOSA_DATAMODEL_SELFHEAL)g_pCosaBEManager->hSelfHeal;
    PCOSA_CONTEXT_SELFHEAL_LINK_OBJECT    pCxtLink          = NULL;
    PSINGLE_LINK_ENTRY                    pSListEntry       = NULL;
    PSINGLE_LINK_ENTRY                    pSListEntry2      = NULL;
    ULONG                                 entryCount        = 0;
	
}

ANSC_HANDLE
IPv6PingServerTable_AddEntry
    (
        ANSC_HANDLE                 hInsContext,
        ULONG*                      pInsNumber
    )
{
	PCOSA_DATAMODEL_SELFHEAL             pSelfHeal              = (PCOSA_DATAMODEL_SELFHEAL)g_pCosaBEManager->hSelfHeal;
    PCOSA_DML_SELFHEAL_IPv6_SERVER_TABLE pServerIpv6 = NULL;
    PCOSA_CONTEXT_SELFHEAL_LINK_OBJECT   pSelfHealCxtLink  = NULL;
    PSINGLE_LINK_ENTRY                   pSListEntry       = NULL;
    ANSC_STATUS                          returnStatus      = ANSC_STATUS_SUCCESS;
    CHAR                                 tmpBuff[66]       = {0};
    BOOL                                      bridgeMode;
	PCOSA_DATAMODEL_SELFHEAL                   pMyObject         = (PCOSA_DATAMODEL_SELFHEAL)g_pCosaBEManager->hSelfHeal;
	int Qdepth = 0;
	
	pServerIpv6 = (PCOSA_DML_SELFHEAL_IPv6_SERVER_TABLE)AnscAllocateMemory(sizeof(COSA_DML_SELFHEAL_IPv6_SERVER_TABLE));
    if ( !pServerIpv6 )
    {
		CcspTraceWarning(("%s resource allocation failed\n",__FUNCTION__));
        return NULL;
    }
	printf("********** Inside %s 1 ********\n",__FUNCTION__);
    pSelfHealCxtLink = (PCOSA_CONTEXT_SELFHEAL_LINK_OBJECT)AnscAllocateMemory(sizeof(COSA_CONTEXT_SELFHEAL_LINK_OBJECT));
    if ( !pSelfHealCxtLink )
    {
        goto EXIT;
    }

    /* now we have this link content */
	Qdepth = AnscSListQueryDepth( &pSelfHeal->IPV6PingServerList );
	pSelfHealCxtLink->InstanceNumber =  pSelfHeal->ulIPv6NextInstanceNumber;
	pServerIpv6->InstanceNumber = pSelfHeal->ulIPv6NextInstanceNumber;
	    pSelfHeal->ulIPv6NextInstanceNumber++;
    if (pSelfHeal->ulIPv6NextInstanceNumber == 0)
        pSelfHeal->ulIPv6NextInstanceNumber = 1;

	pSelfHealCxtLink->hContext = (ANSC_HANDLE)pServerIpv6;
	
	char buf[8];
	pSelfHeal->pConnTest->IPv6EntryCount++;
	printf("*** pSelfHeal->pConnTest->IPv6EntryCount = %d ***\n",pSelfHeal->pConnTest->IPv6EntryCount);
	snprintf(buf,sizeof(buf),"%d",pSelfHeal->pConnTest->IPv6EntryCount);
	if (syscfg_set(NULL, "Ipv6PingServer_Count", buf) != 0) 
	{
		CcspTraceWarning(("syscfg_set failed\n"));
	}
	else 
	{
		if (syscfg_commit() != 0) 
		{
			CcspTraceWarning(("syscfg_commit failed\n"));
		}
	}
		CosaSListPushEntryByInsNum(&pSelfHeal->IPV6PingServerList, (PCOSA_CONTEXT_LINK_OBJECT)pSelfHealCxtLink);
    return (ANSC_HANDLE)pSelfHealCxtLink;

EXIT:
    AnscFreeMemory(pServerIpv6);
    return NULL;

}

ULONG
IPv6PingServerTable_DelEntry
    (
        ANSC_HANDLE                 hInsContext,
        ANSC_HANDLE                 hInstance
    )
{
    ANSC_STATUS                          returnStatus      = ANSC_STATUS_SUCCESS;
    PCOSA_DATAMODEL_SELFHEAL             pSelfHeal               = (PCOSA_DATAMODEL_SELFHEAL)g_pCosaBEManager->hSelfHeal;
    PCOSA_CONTEXT_SELFHEAL_LINK_OBJECT   pSelfHealCxtLink   = (PCOSA_CONTEXT_SELFHEAL_LINK_OBJECT)hInstance;
    PCOSA_DML_SELFHEAL_IPv6_SERVER_TABLE pServerIpv6      = (PCOSA_DML_SELFHEAL_IPv6_SERVER_TABLE)pSelfHealCxtLink->hContext;
	/* Remove entery from the database */
	returnStatus = RemovePingServerURI(PingServerType_IPv6, pServerIpv6->InstanceNumber);
    if ( returnStatus == ANSC_STATUS_SUCCESS )
	{
		/* Remove entery from the Queue */
        if(AnscSListPopEntryByLink(&pSelfHeal->IPV6PingServerList, &pSelfHealCxtLink->Linkage) == TRUE)
		{
			AnscFreeMemory(pSelfHealCxtLink->hContext);
			AnscFreeMemory(pSelfHealCxtLink);
		}
		else
		{
			return ANSC_STATUS_FAILURE;
		}
	}
    return returnStatus;
}

ULONG
IPv6PingServerTable_GetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pValue,
        ULONG*                      pUlSize
    )
{
    PCOSA_CONTEXT_SELFHEAL_LINK_OBJECT   pSelfHealCxtLink     = (PCOSA_CONTEXT_SELFHEAL_LINK_OBJECT)hInsContext;
    PCOSA_DML_SELFHEAL_IPv6_SERVER_TABLE pServerIpv6  = (PCOSA_DML_SELFHEAL_IPv6_SERVER_TABLE)pSelfHealCxtLink->hContext;
    PUCHAR                                    pString       = NULL;
	
    /* check the parameter name and return the corresponding value */
    if (strcmp(ParamName, "X_RDKCENTRAL-COM_Ipv6PingServerURI") == 0)
    {
        /* collect value */
        if ( AnscSizeOfString(pServerIpv6->Ipv6PingServerURI) < *pUlSize)
        {
            AnscCopyString(pValue, pServerIpv6->Ipv6PingServerURI);
            return 0;
        }
        else
        {
            *pUlSize = AnscSizeOfString(pServerIpv6->Ipv6PingServerURI)+1;
            return 1;
        }
    }
	    return -1;
}

BOOL
IPv6PingServerTable_SetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       strValue
    )
{
	PCOSA_CONTEXT_SELFHEAL_LINK_OBJECT   pSelfHealCxtLink     = (PCOSA_CONTEXT_SELFHEAL_LINK_OBJECT)hInsContext;
    PCOSA_DML_SELFHEAL_IPv6_SERVER_TABLE pServerIpv6  = (PCOSA_DML_SELFHEAL_IPv6_SERVER_TABLE)pSelfHealCxtLink->hContext;
	if (strcmp(ParamName, "X_RDKCENTRAL-COM_Ipv6PingServerURI") == 0)
    {
		 AnscCopyString(pServerIpv6->Ipv6PingServerURI,strValue);
		 /* Add entery in the database */
		 SavePingServerURI(PingServerType_IPv6, strValue,pServerIpv6->InstanceNumber);
		 return TRUE;
	}
	return FALSE;
}

BOOL
IPv6PingServerTable_Validate
    (
        ANSC_HANDLE                 hInsContext,
        char*                       pReturnParamName,
        ULONG*                      puLength
    )
{

}

ULONG
IPv6PingServerTable_Commit
    (
        ANSC_HANDLE                 hInsContext
    )
{

}

ULONG
IPv6PingServerTable_Rollback
    (
        ANSC_HANDLE                 hInsContext
    )
{

}

/***********************************************************************

 APIs for Object:

    SelfHeal.ResourceMonitor.

    *  ResourceMonitor_GetParamUlongValue
    *  ResourceMonitor_SetParamUlongValue
    *  ResourceMonitor_Validate
    *  ResourceMonitor_Commit
    *  ResourceMonitor_Rollback

***********************************************************************/
/**********************************************************************

    caller:     owner of this object

    prototype

        BOOL
        ResourceMonitor_GetParamUlongValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                ULONG*                      puLong
            );

    description:

        This function is called to retrieve ULONG parameter value;

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                ULONG*                      puLong
                The buffer of returned ULONG value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
ResourceMonitor_GetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        ULONG*                      puLong
    )
{
    PCOSA_DATAMODEL_SELFHEAL            pMyObject    = (PCOSA_DATAMODEL_SELFHEAL)g_pCosaBEManager->hSelfHeal; 
    PCOSA_DML_RESOUCE_MONITOR           pRescMonitor = pMyObject->pResMonitor;
    if (strcmp(ParamName, "X_RDKCENTRAL-COM_UsageComputeWindow") == 0)
    {
        *puLong = pRescMonitor->MonIntervalTime;
        return TRUE;
    }

    if (strcmp(ParamName, "X_RDKCENTRAL-COM_AvgCPUThreshold") == 0)
    {
        *puLong = pRescMonitor->AvgCpuThreshold;
        return TRUE;
    }

    if (strcmp(ParamName, "X_RDKCENTRAL-COM_AvgMemoryThreshold") == 0)
    {
        *puLong = pRescMonitor->AvgMemThreshold;
        return TRUE;
    }
    return FALSE;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL
	ResourceMonitor_SetParamUlongValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                ULONG                       uValue
            );

    description:

        This function is called to set ULONG parameter value;

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                ULONG                       uValue
                The updated ULONG value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
ResourceMonitor_SetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        ULONG                       uValue
    )
{
    PCOSA_DATAMODEL_SELFHEAL            pMyObject    = (PCOSA_DATAMODEL_SELFHEAL)g_pCosaBEManager->hSelfHeal; 
    PCOSA_DML_RESOUCE_MONITOR           pRescMonitor = pMyObject->pResMonitor;

    if (strcmp(ParamName, "X_RDKCENTRAL-COM_UsageComputeWindow") == 0)
    {
        if ( pRescMonitor->MonIntervalTime == uValue )
        {
            return TRUE;
        }
  
        char buf[8];
#if defined(_ARRIS_XB6_PRODUCT_REQ_) || defined(_CBR_PRODUCT_REQ_) || \
(defined(_XB6_PRODUCT_REQ_) && defined(_COSA_BCM_ARM_))
	ULONG aggressive_interval;
        memset(buf, 0, sizeof(buf));

	syscfg_get( NULL, "AggressiveInterval", buf, sizeof(buf));
	if( buf == NULL )
	{
	    AnscTraceWarning(("syscfg_get returns NULL for AggressiveInterval !\n"));
	    return FALSE;
	}
	aggressive_interval = atol(buf);
	if (uValue <= aggressive_interval)
	{
	    CcspTraceWarning(("resource_monitor_interval should be greater than AggressiveInterval \n"));
	    return FALSE;
	}
#endif
        memset(buf, 0, sizeof(buf));
        snprintf(buf,sizeof(buf),"%d",uValue);
        if (syscfg_set(NULL, "resource_monitor_interval", buf) != 0)
        {
	    CcspTraceWarning(("%s: syscfg_set failed for %s\n", __FUNCTION__, ParamName));
	    return FALSE;
        }
	if (syscfg_commit() != 0)
	{
            CcspTraceWarning(("%s: syscfg commit failed for %s\n", __FUNCTION__, ParamName));
	    return FALSE;
	}
	pRescMonitor->MonIntervalTime = uValue;
	return TRUE;
    }

    if (strcmp(ParamName, "X_RDKCENTRAL-COM_AvgCPUThreshold") == 0)
    { 
        if ( pRescMonitor->MonIntervalTime == uValue )
        {
            return TRUE;
        }

        char buf[8];
        memset(buf, 0, sizeof(buf));
        snprintf(buf,sizeof(buf),"%d",uValue);
        if (syscfg_set(NULL, "avg_cpu_threshold", buf) != 0)
        {
	    CcspTraceWarning(("%s: syscfg_set failed for %s\n", __FUNCTION__, ParamName));
	    return FALSE;
        }
	if (syscfg_commit() != 0)
	{
            CcspTraceWarning(("%s: syscfg commit failed for %s\n", __FUNCTION__, ParamName));
	    return FALSE;
	}
	pRescMonitor->AvgCpuThreshold = uValue;
	return TRUE;
    }

    if (strcmp(ParamName, "X_RDKCENTRAL-COM_AvgMemoryThreshold") == 0)
    { 
        if ( pRescMonitor->MonIntervalTime == uValue )
        {
            return TRUE;
        }

        char buf[8];
        memset(buf, 0, sizeof(buf));
        snprintf(buf,sizeof(buf),"%d",uValue);
        if (syscfg_set(NULL, "avg_memory_threshold", buf) != 0)
        {
	    CcspTraceWarning(("%s: syscfg_set failed for %s\n", __FUNCTION__, ParamName));
	    return FALSE;
        }
	if (syscfg_commit() != 0)
	{
            CcspTraceWarning(("%s: syscfg commit failed for %s\n", __FUNCTION__, ParamName));
	    return FALSE;
	}
	pRescMonitor->AvgMemThreshold = uValue;
	return TRUE;
    }

    return FALSE;
}

/**********************************************************************^M

    caller:     owner of this object

    prototype:

        BOOL
        ResourceMonitor_Validate
            (
                ANSC_HANDLE                 hInsContext,
                char*                       pReturnParamName,
                ULONG*                      puLength
            );

    description:

        This function is called to finally commit all the update.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       pReturnParamName,
                The buffer (128 bytes) of parameter name if there's a validation.

                ULONG*                      puLength
                The output length of the param name

    return:     TRUE if there's no validation.

**********************************************************************/
BOOL
ResourceMonitor_Validate
   (
        ANSC_HANDLE                 hInsContext,
        char*                       pReturnParamName,
        ULONG*                      puLength
    )
{
    return TRUE;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        ULONG
	ResourceMonitor_Commit
            (
                ANSC_HANDLE                 hInsContext
            );

    description:

        This function is called to finally commit all the update.

    argument:   ANSC_HANDLE                 hInsContext,
               The instance handle;

    return:     The status of the operation.

**********************************************************************/
ULONG
ResourceMonitor_Commit
    (
        ANSC_HANDLE                 hInsContext
    )
{
    return ANSC_STATUS_SUCCESS;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        ULONG
        ResourceMonitor_Rollback
            (
                ANSC_HANDLE                 hInsContext
            );

    description:

        This function is called to roll back the update whenever there's a
        validation found.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The status of the operation.

**********************************************************************/
ULONG
ResourceMonitor_Rollback
    (
        ANSC_HANDLE                 hInsContext
    )
{
    return ANSC_STATUS_SUCCESS;
}




/***********************************************************************

 APIs for Object:

    Device.SelfHeal.CpuMemFrag.{i}.

    *  CpuMemFrag_GetEntryCount
    *  CpuMemFrag_GetEntry
    *  CpuMemFrag_GetParamStringValue


***********************************************************************/


ULONG
CpuMemFrag_GetEntryCount
    (
        ANSC_HANDLE                 hInsContext
    )
{
	PCOSA_DATAMODEL_SELFHEAL	pMyObject	= (PCOSA_DATAMODEL_SELFHEAL)g_pCosaBEManager->hSelfHeal;
	PCOSA_DML_CPU_MEM_FRAG pCpuMemFrag = (PCOSA_DML_CPU_MEM_FRAG)pMyObject->pCpuMemFrag;

	return ( pCpuMemFrag->InstanceNumber);
}


ANSC_HANDLE
CpuMemFrag_GetEntry
    (
        ANSC_HANDLE                 hInsContext,
        ULONG                       nIndex,
        ULONG*                      pInsNumber
    )
{
	PCOSA_DATAMODEL_SELFHEAL	pMyObject	= (PCOSA_DATAMODEL_SELFHEAL)g_pCosaBEManager->hSelfHeal;
	PCOSA_DML_CPU_MEM_FRAG pCpuMemFrag = (PCOSA_DML_CPU_MEM_FRAG)pMyObject->pCpuMemFrag;

	if ( pCpuMemFrag )
	{
			*pInsNumber  = nIndex + 1; 
//			/*Get data of Host/Peer from syscfg 	*/
//			CosaDmlGetSelfHealCpuMemFragData(&pCpuMemFrag->pCpuMemFragDma[nIndex]);
			return &pCpuMemFrag->pCpuMemFragDma[nIndex];
	}
	else
			return NULL;
}

ULONG
CpuMemFrag_GetParamStringValue
		(
				ANSC_HANDLE 								hInsContext,
				char* 											ParamName,
				char* 											pValue,
				ULONG*											pUlSize
		)
{

	PCOSA_DML_CPU_MEM_FRAG_DMA pCpuMemFragDma = (PCOSA_DML_CPU_MEM_FRAG_DMA)hInsContext;

	/*Get data of Host/Peer from syscfg 	*/
	CosaDmlGetSelfHealCpuMemFragData(pCpuMemFragDma);

	/* check the parameter name and return the corresponding value */
	if (strcmp(ParamName, "DMA") == 0)
	{
		AnscCopyString( pValue, pCpuMemFragDma->dma);
		return 0;
	}

	if (strcmp(ParamName, "DMA32") == 0)
	{
		AnscCopyString( pValue, pCpuMemFragDma->dma32);
		return 0;
	}

	if (strcmp(ParamName, "Normal") == 0)
	{
		AnscCopyString( pValue, pCpuMemFragDma->normal);
		return 0;
	}

	if (strcmp(ParamName, "Highmem") == 0)
	{
		AnscCopyString( pValue, pCpuMemFragDma->highmem);
		return 0;
	}

return -1;
}

BOOL
CpuMemFrag_GetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        ULONG*                        pInt
    )
{

	PCOSA_DML_CPU_MEM_FRAG_DMA pCpuMemFragDma = (PCOSA_DML_CPU_MEM_FRAG_DMA)hInsContext;

	/*Get data of Host/Peer from syscfg 	*/
	CosaDmlGetSelfHealCpuMemFragData(pCpuMemFragDma);

	/* check the parameter name and return the corresponding value */
	if (strcmp(ParamName, "FragPercentage") == 0)
	{
		 /* collect value */
		*pInt = pCpuMemFragDma->FragPercentage;
		return TRUE;
	}

return FALSE;
}


