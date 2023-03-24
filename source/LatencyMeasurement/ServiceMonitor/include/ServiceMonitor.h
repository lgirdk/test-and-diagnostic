#ifndef __SERVICEMONITOR_H__
#define __SERVICEMONITOR_H__

#include "lowlatency_apis.h"
#define NUM_PTHREADS 2 
#define SNIFFER_CMD "busybox pidof xNetSniffer"
#define DP_CMD "busybox pidof xNetDP"

/*
#define	IPV4_LATENCY_ENABLE	"LatencyMeasure_IPv4Enable"
#define	IPV6_LATENCY_ENABLE	"LatencyMeasure_IPv6Enable"
#define	TCPREPORTINTERVAL	"LatencyMeasure_TCPReportInterval" */
#define BUF_SIZE 128
#define TIMERINTERVEL 300 //sec
#define REPORT_SIZE 100
#define ARRAY_SIZE 250
#define ARRAY_LEN 50

typedef enum{
  SYSEVENT_PTHREAD_ID,
  MONITOR_PTHREAD_ID
}pthreadid;

typedef enum{
  SERVICE_NOT_ACTIVE,
  SERVICE_ACTIVE
}ServiceStatus;

typedef enum{
  ROUTER_MODE,
  BRIDGEMODE
}CfgBridgeMode;

typedef enum{
	FILE_PATH,
	RBUS_DATA_MODEL
}Report_type;

typedef enum{
 SNIFFER_SERVICE, 
 DP_SERVICE
}ServiceType;
typedef enum{
  LM_DP_SERVICE,
  LM_IPV4_SNIFFER_SERVICE,
  LM_IPV6_SNIFFER_SERVICE
}Lm_ServiceType;
typedef enum{
	LAN_WAN_LATENCY,
	LAN_SIDE_LATENCY,
	WAN_SIDE_LATENCY
}Latency_measurement ;
  	
int LatencyMeasurementServiceInit();
int LatencyMeasurement_Config_Init();
bool GetLatencyMeasureEnableStatus(char parameter_name[]);
int GetTCPReportInterval();
int CheckLatencyMeasurementServiceStatus(int Service_Type,char *Pidbuf);
void MonitorLatencyMeasurementServices();
void Stop_LatencyMeasurement_Services(Lm_ServiceType LM_Service);
void copy_command_output(FILE *fp, char * buf, int len);
int UpdateLatencyMeasurement_EnableCount(bool LowLatency_Enable);	
void Stop_all_LatencyMeasurement_Services();
int Get_Status_of_bridge_mode();
void SendConditional_pthread_cond_signal();
void *SysEventHandlerThrd_for_Monitorservice(void *data);
void* LatencyMeasurement_MonitorService(void *arg);

#endif //__SERVICEMONITOR_H__ 