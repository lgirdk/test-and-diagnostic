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

#include "ansc_platform.h"
#include "plugin_main_apis.h"
#include "cosa_logbackup_dml.h"
#include <syscfg/syscfg.h>

BOOL g_logbackup_enable = TRUE;
ULONG g_logbackup_interval = 30;

/***********************************************************************

 APIs for Object:

    LogBackup.

    *  LogBackup_GetParamBoolValue
    *  LogBackup_SetParamBoolValue
    *  LogBackup_GetParamUlongValue
    *  LogBackup_SetParamUlongValue
    *  LogBackup_Validate
    *  LogBackup_Commit
    *  LogBackup_Rollback

***********************************************************************/
/**********************************************************************

    caller:     owner of this object

    prototype

        BOOL
        LogBackup_GetParamBoolValue
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
BOOL LogBackup_GetParamBoolValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL*                        bValue
    )
{
    if (strcmp(ParamName, "X_RDKCENTRAL-COM_Enable") == 0)
    {
        *bValue = g_logbackup_enable;
        return TRUE;
    }


    if (strcmp(ParamName, "X_RDKCENTRAL-COM_SyncandUploadLogs") == 0)
    {
        *bValue = false;
        return TRUE;
    }
    return FALSE;
}

/**********************************************************************

    caller:     owner of this object

    prototype

        BOOL
        LogBackup_SetParamBoolValue
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
BOOL LogBackup_SetParamBoolValue  
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL                        bValue
    )
{
    if (strcmp(ParamName, "X_RDKCENTRAL-COM_Enable") == 0)
    {

        if( g_logbackup_enable == bValue )
        {
            return TRUE;
	}
        
        char buf[128];
        memset(buf, 0, sizeof(buf));
        snprintf(buf,sizeof(buf),"%s",bValue ? "true" : "false");
        if (syscfg_set(NULL, "logbackup_enable", buf) != 0)
        {
	    	CcspTraceWarning(("%s: syscfg_set failed for %s\n", __FUNCTION__, ParamName));
	   	return FALSE;
        }

	if (syscfg_commit() != 0)
	{
	      	CcspTraceWarning(("%s: syscfg commit failed for %s\n", __FUNCTION__, ParamName));
		return FALSE;
	}
	
        g_logbackup_enable = bValue;
        return TRUE;
    }

    if (strcmp(ParamName, "X_RDKCENTRAL-COM_SyncandUploadLogs") == 0)
    {
	SyncAndUploadLogs();
	return TRUE;
    }
    return FALSE;
}

/**********************************************************************

    caller:     owner of this object

    prototype

        BOOL
        LogBackup_GetParamUlongValue
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
LogBackup_GetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        ULONG*                      puLong
    )
{
    if (strcmp(ParamName, "X_RDKCENTRAL-COM_Interval") == 0)
    {
        *puLong = g_logbackup_interval;
        return TRUE;
    }

    return FALSE;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL
        LogBackup_SetParamUlongValue
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
LogBackup_SetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        ULONG                       uValue
    )
{
    if (strcmp(ParamName, "X_RDKCENTRAL-COM_Interval") == 0)
    {
        if( g_logbackup_interval == uValue )
        {
            return TRUE;
	}
        
        char buf[128];
        memset(buf, 0, sizeof(buf));
        snprintf(buf,sizeof(buf),"%lu",uValue);
        if (syscfg_set(NULL, "logbackup_interval", buf) != 0)
        {
		CcspTraceWarning(("%s: syscfg_set failed for %s\n", __FUNCTION__, ParamName));
		return FALSE;
        }
	if (syscfg_commit() != 0)
	{
        	CcspTraceWarning(("%s: syscfg commit failed for %s\n", __FUNCTION__, ParamName));
		return FALSE;
	}
        g_logbackup_interval = uValue;
        return TRUE;
    }

    return FALSE;
}

/**********************************************************************^M

    caller:     owner of this object

    prototype:

        BOOL
        LogBackup_Validate
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
LogBackup_Validate
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
	LogBackup_Commit
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
LogBackup_Commit
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
        LogBackup_Rollback
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
LogBackup_Rollback
    (
        ANSC_HANDLE                 hInsContext
    )
{
    return ANSC_STATUS_SUCCESS;
}


void
get_logbackupcfg()
{

	char buf[128];
	
	memset(buf,0,sizeof(buf));
	if((syscfg_get( NULL, "logbackup_enable", buf, sizeof(buf)) == 0 ) && (buf[0] != '\0') )
	{
			g_logbackup_enable = (!strcmp(buf, "true")) ? TRUE : FALSE;
	}
	else
	{
	        memset(buf, 0, sizeof(buf));
	        snprintf(buf,sizeof(buf),"true");
	        if (syscfg_set(NULL, "logbackup_enable", buf) != 0)
	        {
		    	CcspTraceWarning(("%s: syscfg_set failed \n", __FUNCTION__));
		   	
	        }
		else
		{
			if (syscfg_commit() != 0)
			{
			      	CcspTraceWarning(("%s: syscfg commit failed \n", __FUNCTION__));
				
			}
		}
	}

	memset(buf,0,sizeof(buf));
	if((syscfg_get( NULL, "logbackup_interval", buf, sizeof(buf)) == 0) && (buf[0] != '\0'))
	{
		g_logbackup_interval = atoi(buf);
	}
	else
	{
	        memset(buf, 0, sizeof(buf));
	        snprintf(buf,sizeof(buf),"30");
	        if (syscfg_set(NULL, "logbackup_interval", buf) != 0)
	        {
			CcspTraceWarning(("%s: syscfg_set failed \n", __FUNCTION__));
	        }
	        else
	        {
			if (syscfg_commit() != 0)
			{
		        	CcspTraceWarning(("%s: syscfg commit failed \n", __FUNCTION__));
			}
	        }
	
	}

}

