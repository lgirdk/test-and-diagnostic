#include <stdio.h>
#include <stdlib.h>
#include  "safec_lib_common.h"
#include <syscfg/syscfg.h>
#include "ccsp_trace.h"
#include "secure_wrapper.h"
#include "ImageHealthChecker_bus_connection.h"
#include <telemetry_busmessage_sender.h>

#define  STR_HLTH "store-health"
#define  BT_CHCK  "bootup-check"

int fnd_cli_diff(int old_val,int new_val);
void report_t2(char * event,char type,char *val);
void get_Clients_Count(char * arg_type,char * ret_buf,int size);
void get_dml_values();
int Iscli_Wap_Pass_Changed(char *arg_type,int old_value,char *curr_pass);
