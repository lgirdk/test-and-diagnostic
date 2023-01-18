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


// C Program for Message Queue (Reader Process)
#include <stdio.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#include <pcap.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include<time.h>
#include <pthread.h>
#include<stdio.h>
#include<stdlib.h>
#include<getopt.h>
#include<string.h>
#include<stdbool.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <errno.h>

#include <rbus/rbus.h>

static rbusHandle_t bus_handle_rbus = NULL;

//#define LATENCY_REPORT_FILE "/tmp/LatencyReport.txt"
#define SYN 0x2 //1
#define SYN_ACK 0x12 //18
#define ACK 0x10 //16
#define SIZE 1024

//#define MAX_REPORT_SIZE 5124
#define MAX_TCP_SYN_ACK_TIMEOUT 3


#define INDEX_SYN 0
#define INDEX_SYN_ACK 1
#define INDEX_ACK 2
#define NUM_TCP_ISN 3

#define TRUE 1
#define FALSE 0

typedef struct mesg_buffer {
    u_char mesg_type;
    u_int   th_flag;
    u_int   th_seq;                
    u_int   th_ack; 
    u_int key;
    long long tv_sec;
    long long tv_usec;
    char    mac[18];
} msg;

typedef struct _tcp_header_
{
    u_int   th_flag;
    u_int   th_seq;                
    u_int   th_ack;
    long long tv_sec;
    long long tv_usec; 

}TcpHeader;
typedef struct TCP_SNIFFER_ {
 //   u_char mesg_type;
    
    u_short key;
    long long latency_sec;
    long long latency_usec;
    long long Lan_latency_sec;
    long long Lan_latency_usec;  
    TcpHeader TcpInfo[NUM_TCP_ISN];
    char    mac[18];
    char bComputed;
}TcpSniffer;
msg message;
u_int g_HashCount = 0;
//msg PcktHashTable[SIZE];
  
// structure for message queue
/*struct mesg_buffer {
    long mesg_type;
    char mesg_text[100];
} message;*/


/*
struct DataItem {
   int data;   
   int key;
};*/

TcpSniffer hashArray[SIZE]; 
msg dummyItem;
msg item;


/* Command line options. */
struct option longopts[] =
{
  { "Report Type",                          required_argument,       NULL, 't'},
  { "Report Interval",                      required_argument,       NULL, 'i'},
  { "Report size",                          required_argument,       NULL, 's'},
  { "Report Name",                          required_argument,       NULL, 'n'},
  { "Aggregated Report",                    no_argument,             NULL, 'a'},
  { "Aggregated Report per MAC per PORT",   no_argument,             NULL, 'p'},
  { "Latency Measurement",                  required_argument,       NULL, 'L'},
  { "DebugMode",                            no_argument,             NULL, 'D'},
  { "FilePath",                             required_argument,       NULL, 'F'},
  { "help",                                 no_argument,             NULL, 'h'},
  { 0 }
};

#define MAX_LOG_BUFF_SIZE 2048
FILE *logFp = NULL;
char log_buff[MAX_LOG_BUFF_SIZE] ;
#define VALIDATION_SUCCESS 0
#define VALIDATION_FAILED  -1

#define dbg_log(fmt ...)    {\
                            if (args.dbg_mode){\
                            snprintf(log_buff, MAX_LOG_BUFF_SIZE-1,fmt);\
                            if(logFp != NULL){ \
                                            fprintf(logFp,"DBG_LOG : %s", log_buff);\
                                            fflush(logFp);}\
                            else \
                                printf("%s",log_buff);\
                            }\
                         }

enum rep_type
{
    REP_TYPE_FILE=0,
    REP_TYPE_T2=1 
};

enum latency_measurment_type
{
    LANWAN = 0 ,
    LANONLY = 1 ,
    WANONLY = 2
};

typedef struct Params
{
  bool dbg_mode;  
  bool aggregated_data;
  bool aggregated_data_per_port;
  int  report_type;
  int  report_interval;
  int  report_size;
  char report_name[256];
  int  latency_measurement;
  char log_file[64];
}Param;

Param args;

int hashCode(int key) {
   //return key % (SIZE*sizeof(u_int));
      return key % SIZE;
}

int search(int key) {
   //get the hash 
   //int hashIndex = hashCode(key);
    int i = 0;
    int hashIndex = hashCode((u_short)key);    
     dbg_log("search, hashIndex - %d\n",hashIndex);
   //move in array until an empty 
   while(hashIndex < SIZE) {
      if(i > SIZE) // make sure to move only once in array
      {
            dbg_log("search, No SYN Seq found, g_HashCount - %d\n",g_HashCount);
            return -1;
      }

      if(hashArray[hashIndex].key == (u_short)key)
         return hashIndex; 
            
      //go to next cell
      ++hashIndex;
      i++;  
      //wrap around the table
      hashIndex %= SIZE;
      if(g_HashCount > SIZE)
      {
            dbg_log("search, Hash is full last entry update, g_HashCount - %d\n",g_HashCount);
            break;



      }
   }        
    
   return -1;        
}

void insert(int key,msg data) {

   //struct DataItem *item = (struct DataItem*) malloc(sizeof(struct DataItem));
   //item->data = data;  
   //item->key = key;

   //get the hash 
   //u_int hashIndex = hashCode(key);
    //int hashIndex = hashCode(key);
    int hashIndex = hashCode((u_short)key);
  // printf("Insert, hashIndex - %lu\n",hashIndex);
   dbg_log("Insert, hashIndex - %d\n",hashIndex);

   //move in array until an empty or deleted cell
   while(hashIndex < SIZE && hashArray[hashIndex].key != 0) {
      if(hashArray[hashIndex].key == (u_short)key)
      {
        dbg_log("Ignoring insert as SYN entry for seq - %u already exists..\n",data.th_seq);
        return;
      }
      //go to next cell
      ++hashIndex;
        
      //wrap around the table
      hashIndex %= SIZE;
      if(g_HashCount >= SIZE)
      {
        dbg_log("Insert Hash is full, g_HashCount - %d\n",g_HashCount);
        return;
      }
   }
   dbg_log("Insert, call memcpy\n");
   //data.key = hashIndex;
   data.key = (u_short)key;
   g_HashCount++;
   //memcpy(&hashArray[hashIndex],&data,sizeof(mesg)); 
   hashArray[hashIndex].TcpInfo[INDEX_SYN].th_flag = data.th_flag;
   hashArray[hashIndex].TcpInfo[INDEX_SYN].th_seq  = data.th_seq;
   hashArray[hashIndex].TcpInfo[INDEX_SYN].th_ack  = data.th_ack;
   hashArray[hashIndex].TcpInfo[INDEX_SYN].tv_sec  = data.tv_sec;
   hashArray[hashIndex].TcpInfo[INDEX_SYN].tv_usec  = data.tv_usec;
   hashArray[hashIndex].key = (u_short)key;
   memcpy(hashArray[hashIndex].mac,data.mac,18); 

   dbg_log("hashIndex %d MAC: %s FLAG: %d ACK: %u Seq: %u TS: %lld.%06lld\n",hashIndex,hashArray[hashIndex].mac,hashArray[hashIndex].TcpInfo[INDEX_SYN].th_flag,hashArray[hashIndex].TcpInfo[INDEX_SYN].th_ack,hashArray[hashIndex].TcpInfo[INDEX_SYN].th_seq,hashArray[hashIndex].TcpInfo[INDEX_SYN].tv_sec,hashArray[hashIndex].TcpInfo[INDEX_SYN].tv_usec);
  // hashArray[hashIndex] = data;

}
/*
struct DataItem* delete(struct DataItem* item) {
   int key = item->key;

   //get the hash 
   int hashIndex = hashCode(key);

   //move in array until an empty
   while(hashArray[hashIndex] != NULL) {
    
      if(hashArray[hashIndex]->key == key) {
         struct DataItem* temp = hashArray[hashIndex]; 
            
         //assign a dummy item at deleted position
         hashArray[hashIndex] = dummyItem; 
         return temp;
      }
        
      //go to next cell
      ++hashIndex;
        
      //wrap around the table
      hashIndex %= SIZE;
   }      
    
   return NULL;        
}*/

void display() {
   int i = 0;
    
   for(i = 0; i<SIZE; i++) {
    
     // if(hashArray[i] != NULL)
        // printf(" i = %d (%d,%d)\n",i,hashArray[i].key,hashArray[i].th_seq);
           //  printf("HashTable %d MAC: %s FLAG: %d ACK: %lu Seq: %lu TS: %lld.%06ld\n", 
            //        i,hashArray[i].mac,hashArray[i].th_flag,hashArray[i].th_ack,hashArray[i].th_seq,hashArray[i].tv_sec,hashArray[i].tv_usec);

            if(hashArray[i].key != 0)
            {
           /*  printf("\nHashTable - %d MAC: %s   Latency   : %lld.%06ld\n        SYN FLAG    : %d ACK: %lu Seq: %lu TS: %lld.%06ld\n        SYN ACK FLAG: %d ACK: %lu Seq: %lu TS: %lld.%06ld\n", 
                    i,hashArray[i].mac,hashArray[i].latency_sec,hashArray[i].latency_usec,
                    hashArray[i].TcpInfo[INDEX_SYN].th_flag,hashArray[i].TcpInfo[INDEX_SYN].th_ack,hashArray[i].TcpInfo[INDEX_SYN].th_seq,hashArray[i].TcpInfo[INDEX_SYN].tv_sec,hashArray[i].TcpInfo[INDEX_SYN].tv_usec,
                    hashArray[i].TcpInfo[INDEX_SYN_ACK].th_flag,hashArray[i].TcpInfo[INDEX_SYN_ACK].th_ack,hashArray[i].TcpInfo[INDEX_SYN_ACK].th_seq,hashArray[i].TcpInfo[INDEX_SYN_ACK].tv_sec,hashArray[i].TcpInfo[INDEX_SYN_ACK].tv_usec);
            
*/
               dbg_log("\n ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ \n");
               dbg_log("\nhashIndex %d | MAC: %s | FLAG: %d | ACK: %u | Seq: %u | TS: %lld.%06lld |\n",i,hashArray[i].mac,hashArray[i].TcpInfo[INDEX_SYN].th_flag,hashArray[i].TcpInfo[INDEX_SYN].th_ack,hashArray[i].TcpInfo[INDEX_SYN].th_seq,hashArray[i].TcpInfo[INDEX_SYN].tv_sec,hashArray[i].TcpInfo[INDEX_SYN].tv_usec);
               dbg_log("hashIndex %d | MAC: %s | FLAG: %d | ACK: %u | Seq: %u | TS: %lld.%06lld |\n",i,hashArray[i].mac,hashArray[i].TcpInfo[INDEX_SYN_ACK].th_flag,hashArray[i].TcpInfo[INDEX_SYN_ACK].th_ack,hashArray[i].TcpInfo[INDEX_SYN_ACK].th_seq,hashArray[i].TcpInfo[INDEX_SYN_ACK].tv_sec,hashArray[i].TcpInfo[INDEX_SYN_ACK].tv_usec);
               dbg_log("hashIndex %d | MAC: %s | FLAG: %d | ACK: %u | Seq: %u | TS: %lld.%06lld |\n",i,hashArray[i].mac,hashArray[i].TcpInfo[INDEX_ACK].th_flag,hashArray[i].TcpInfo[INDEX_ACK].th_ack,hashArray[i].TcpInfo[INDEX_ACK].th_seq,hashArray[i].TcpInfo[INDEX_ACK].tv_sec,hashArray[i].TcpInfo[INDEX_ACK].tv_usec);
               dbg_log("WAN side Latency for MAC: %s | Seq: %u | %lld.%06lld |\n",hashArray[i].mac,hashArray[i].TcpInfo[INDEX_SYN].th_seq,hashArray[i].latency_sec,hashArray[i].latency_usec);
               dbg_log("LAN side Latency for MAC: %s | Seq: %u | %lld.%06lld |\n",hashArray[i].mac,hashArray[i].TcpInfo[INDEX_SYN].th_seq,hashArray[i].Lan_latency_sec,hashArray[i].Lan_latency_usec);
               dbg_log("\n ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ \n");
            }
     // else 
      //   printf(" ~~ ");
   }
    
   dbg_log("\n");
}
 int
 timeval_subtract (struct timeval *result,struct timeval *T1,struct timeval *T2)
 {
   /* Perform the carry for the later subtraction by updating y. */
   if (T1->tv_usec < T2->tv_usec) {
     int nsec = (T2->tv_usec - T1->tv_usec) / 1000000 + 1;
     T2->tv_usec -= 1000000 * nsec;
     T2->tv_sec += nsec;
   }
   if (T1->tv_usec - T2->tv_usec > 1000000) {
     int nsec = (T1->tv_usec - T2->tv_usec) / 1000000;
     T2->tv_usec += 1000000 * nsec;
     T2->tv_sec -= nsec;
   }

   /* Compute the time remaining to wait.
      tv_usec is certainly positive. */
   result->tv_sec = T1->tv_sec - T2->tv_sec;
   result->tv_usec = T1->tv_usec - T2->tv_usec;

   /* Return 1 if result is negative. */
   return T1->tv_sec < T2->tv_sec;
 }

void MeasureTCPLatency(int hashIndex)
{
    //1000000 * header->ts.tv_sec + header->ts.tv_usec
   /* hashArray[hashIndex].latency_sec  = hashArray[hashIndex].TcpInfo[INDEX_SYN_ACK].tv_sec - hashArray[hashIndex].TcpInfo[INDEX_SYN].tv_sec;
    hashArray[hashIndex].latency_usec = (1000000 * hashArray[hashIndex].TcpInfo[INDEX_SYN_ACK].tv_usec) - hashArray[hashIndex].TcpInfo[INDEX_SYN].tv_usec;

    hashArray[hashIndex].Lan_latency_sec  = hashArray[hashIndex].TcpInfo[INDEX_ACK].tv_sec - hashArray[hashIndex].TcpInfo[INDEX_SYN_ACK].tv_sec;
    hashArray[hashIndex].Lan_latency_usec = (1000000 * hashArray[hashIndex].TcpInfo[INDEX_ACK].tv_usec) - hashArray[hashIndex].TcpInfo[INDEX_SYN_ACK].tv_usec;

    printf("WAN Latency for %s %lu is %lld.%ld\n",hashArray[hashIndex].mac,hashArray[hashIndex].TcpInfo[INDEX_SYN].th_seq,hashArray[hashIndex].latency_sec,hashArray[hashIndex].latency_usec);
    printf("LAN Latency for %s %lu is %lld.%ld\n",hashArray[hashIndex].mac,hashArray[hashIndex].TcpInfo[INDEX_SYN].th_seq,hashArray[hashIndex].Lan_latency_sec,hashArray[hashIndex].Lan_latency_usec);
 */
    struct timeval t1, t2, t3, diff_time;
    t2.tv_sec = hashArray[hashIndex].TcpInfo[INDEX_SYN_ACK].tv_sec;
    t2.tv_usec = hashArray[hashIndex].TcpInfo[INDEX_SYN_ACK].tv_usec;
    t1.tv_sec = hashArray[hashIndex].TcpInfo[INDEX_SYN].tv_sec;
    t1.tv_usec = hashArray[hashIndex].TcpInfo[INDEX_SYN].tv_usec;
    t3.tv_sec = hashArray[hashIndex].TcpInfo[INDEX_ACK].tv_sec;
    t3.tv_usec = hashArray[hashIndex].TcpInfo[INDEX_ACK].tv_usec;  
    //timercmp(&t1,&t2,diff_time);

    if ( args.latency_measurement == LANWAN || args.latency_measurement == WANONLY )
    {
        timeval_subtract(&diff_time,&t2,&t1);
        hashArray[hashIndex].latency_sec = diff_time.tv_sec;
        hashArray[hashIndex].latency_usec = diff_time.tv_usec;
        dbg_log("WAN Latency for %s %u is %lld.%lld\n",hashArray[hashIndex].mac,hashArray[hashIndex].TcpInfo[INDEX_SYN].th_seq,hashArray[hashIndex].latency_sec,hashArray[hashIndex].latency_usec);
    }

    if ( args.latency_measurement == LANWAN || args.latency_measurement == LANONLY )
    {
        timeval_subtract(&diff_time,&t3,&t2);
        hashArray[hashIndex].Lan_latency_sec = diff_time.tv_sec;
        hashArray[hashIndex].Lan_latency_usec = diff_time.tv_usec;
        dbg_log("LAN Latency for %s %u is %lld.%lld\n",hashArray[hashIndex].mac,hashArray[hashIndex].TcpInfo[INDEX_SYN].th_seq,hashArray[hashIndex].Lan_latency_sec,hashArray[hashIndex].Lan_latency_usec);
    }

    hashArray[hashIndex].bComputed = TRUE;
    display();
}
void* LatencyReportThread(void* arg)
{
    // detach the current thread 
    int i = 0;
    int byteCount = 0;
    int tempCount = 0;
    int count = 0;
    char str[args.report_size];
    char str1[args.report_size];
    memset(str,0,args.report_size);

    pthread_detach(pthread_self());
  
    dbg_log("Inside the LatencyReportThread\n");
    FILE *fp;
    //fp = fopen("LatencyReport.txt", "w+");

    while(1)
    {
        sleep(5);
        //fp = fopen("LatencyReport.txt", "a+");
        //if(fp != NULL)
        {
           // display();
            while(i < SIZE)
            {
               // printf("hashArray[%d].bComputed = %d\n",i,hashArray[i].bComputed);
                if(hashArray[i].bComputed == TRUE)
                {
                    if (args.latency_measurement == LANWAN )
                        tempCount = sprintf(str1,"%s,%u,%lld.%06lld,%lld.%06lld|",hashArray[i].mac,hashArray[i].TcpInfo[INDEX_SYN].th_seq,hashArray[i].latency_sec,hashArray[i].latency_usec,hashArray[i].Lan_latency_sec,hashArray[i].Lan_latency_usec);
                    else if (args.latency_measurement == WANONLY )
                        tempCount = sprintf(str1,"%s,%u,%lld.%06lld|",hashArray[i].mac,hashArray[i].TcpInfo[INDEX_SYN].th_seq,hashArray[i].latency_sec,hashArray[i].latency_usec);
                    else if (args.latency_measurement == LANONLY )
                        tempCount = sprintf(str1,"%s,%u,%lld.%06lld|",hashArray[i].mac,hashArray[i].TcpInfo[INDEX_SYN].th_seq,hashArray[i].Lan_latency_sec,hashArray[i].Lan_latency_usec);
                    if(tempCount)
                    {
                        if((byteCount+tempCount) < args.report_size)
                        {
                            byteCount += tempCount;
                            memset(&hashArray[i],0,sizeof(TcpSniffer));
                            g_HashCount--;
                            strcat(str,str1);

                        }
                        else
                        {
                                dbg_log("Report size full, this data will go in next report\n");
                        }

                    }
                    //strcat(str,str1);
                    dbg_log("i = %d str = %s\n",i,str);
                    memset(str1,0,args.report_size);
                }
                i++;
            }
            i = 0;
        }
        count++;
        if( byteCount > 0 && ( ((count * 5) >= args.report_interval )||(byteCount >= args.report_size - 24)) ) // send report after 60 seconds or when report around MAX_REPORT_SIZE
        {
            dbg_log("Report generated - count in sec is %d byteCount = %d\n",count * 5,byteCount);

            if (args.report_type  == REP_TYPE_FILE && strlen(args.report_name) !=0 )
            {
                fp = fopen(args.report_name, "a+");
                if(fp != NULL)
                {
                    fputs(str, fp);
                    fclose(fp);
                }                    
                    //system("cat /tmp/LatencyReport.txt");
                    //memset(str1,0,MAX_REPORT_SIZE);  
            }
            else if (args.report_type  == REP_TYPE_T2 && strlen(args.report_name) !=0 )
            {
                //TODO
                dbg_log("Set param %s\n",args.report_name);
                rbus_setStr(bus_handle_rbus, args.report_name,str);
            }

            count = 0;
            byteCount = 0;
            memset(str,0,args.report_size);
            dbg_log("Report generated - Dislpay\n");
            display();
        }

    }
  
    // exit the current thread
    //pthread_exit(NULL);
}


void* ClearHashThread(void* arg)
{
    // detach the current thread 
    int i = 0;

    pthread_detach(pthread_self());
  
    dbg_log("Inside the ClearHashThread\n");
            while(1)
            {
                sleep(7);
                while(i < SIZE)
                {
                //struct timeval te; 
                //gettimeofday(&te, NULL); // get current time
                //long long milliseconds = te.tv_sec*1000LL + te.tv_usec/1000;
                time_t seconds;
     
                seconds = time(NULL);

                if((hashArray[i].bComputed == FALSE) && (hashArray[i].key != 0))
                {
                  /* printf("te.tv_sec - %lu hashArray[%d].TcpInfo[INDEX_SYN].tv_sec %ld\n",(te.tv_sec*1000LL),i, hashArray[i].TcpInfo[INDEX_SYN].tv_sec);
   
                    if(((te.tv_sec*1000LL) - hashArray[i].TcpInfo[INDEX_SYN].tv_sec) > MAX_TCP_SYN_ACK_TIMEOUT)*/
                    dbg_log("te.tv_sec - %lu hashArray[%d].TcpInfo[INDEX_SYN].tv_sec %u\n",seconds,i,(u_int)hashArray[i].TcpInfo[INDEX_SYN].tv_sec);
                    int diff = seconds - (u_int)hashArray[i].TcpInfo[INDEX_SYN].tv_sec;
                    dbg_log("diff time %u\n",diff);
                    //if((seconds - (u_int)hashArray[i].TcpInfo[INDEX_SYN].tv_sec) > MAX_TCP_SYN_ACK_TIMEOUT)
                    if(diff > MAX_TCP_SYN_ACK_TIMEOUT)
                    {
                            dbg_log("Clearing un-acknowledged SYN enteries\n");
                            memset(&hashArray[i],0,sizeof(TcpSniffer));
                            g_HashCount--;
                    }

                }
                i++;
                }
                i = 0;
            }
    
    //fp = 

}

/* Help information display. */
static void
usage (char *progname, int status)
{
  if (status != 0)
    fprintf (stderr, "Try `%s -h' for more information.\n", progname);
  else
    {
      printf ("Usage : %s [OPTION...]\n\n"\
              "-D,   Enable dbg logs\n"\
              "-F,   Log File\n"\
              "-t,   Report type: 0 = file, 1 = T2\n"\
              "-i,   Report Interval is secs\n"\
              "-n,   Report name: File path name or RBus data model name\n"\
              "-s,   Report Size in Bytes\n"\
             /* "-a,   To get aggregated report \n"\
              "-p,   To get aggregated report per macaddress per port\n"\ */
              "-L,   Latency measurement : 0 - Total (LAN + WAN) latency, 1 - LAN side, 2 - WAN Side \n"\
              "-h,   Display this help and exit\n"\
              "\n",progname
              );
    }
  exit (status);
}
int validateParams(Param args)
{
    if ( REP_TYPE_FILE != args.report_type &&  REP_TYPE_T2 != args.report_type )
    {
        printf("report_type validation failed\n");
        return VALIDATION_FAILED;
    }
    else if ( WANONLY != args.latency_measurement && LANONLY != args.latency_measurement && LANWAN != args.latency_measurement )
    {
        printf("latency_measurement arg validation failed\n");
        return VALIDATION_FAILED;
    }
    else if ( 0 == args.report_size )
    {
        printf("report_size can't be null\n");
        return VALIDATION_FAILED;
    }
    else if ( 0 == args.report_interval )
    {
        printf("report_interval can't be null\n");
        return VALIDATION_FAILED;
    }
    else if (strlen(args.report_name) == 0 )
    {
        printf("report_name can't be null\n");
        return VALIDATION_FAILED;
    }
    return VALIDATION_SUCCESS;
}

void rbusInit(char *progname)
{
          dbg_log("Entering %s\n", __FUNCTION__);
          int ret = RBUS_ERROR_SUCCESS;
          ret = rbus_open(&bus_handle_rbus, progname);
          if(ret != RBUS_ERROR_SUCCESS) {
              dbg_log("%s: init failed with error code %d \n", __FUNCTION__, ret);
               return ;
         }    
}

int main(int argc,char **argv)
{

      char *progname;
  char *p;

  progname = ((p = strrchr (argv[0], '/')) ? ++p : argv[0]);

  if(argc == 1 )
    usage (progname, 1);

  int opt;
  memset(&args,0,sizeof(Param));

    while (1)
    {
      opt = getopt_long (argc, argv, "apDhi:t:s:n:L:F:", longopts, 0);

      if (opt == EOF)
      {
            break;
      }
        switch (opt)
        {
            case 'i':
              args.report_interval = atoi(optarg);
              break;
            case 't':
              args.report_type = atoi(optarg);
              break;
            case 's':
              args.report_size = atoi(optarg);
              break;
            case 'L':
              args.latency_measurement = atoi(optarg);
              break;
            case 'n':
              strcpy(args.report_name,optarg);
              break;
            case 'D':
              args.dbg_mode = true;
              break;
           /* case 'a':
              args.aggregated_data = true;
              break;
            case 'p':
              args.aggregated_data_per_port = true;
              break; */
            case 'F':
             // data.log_file = optarg;
              strcpy(args.log_file,optarg);
              logFp = fopen(args.log_file,"w+");
              break;
            case 'h':
              usage (progname, 0);
              break;
            default:
              usage (progname, 1);
              break;  
        }
    }

    if ( VALIDATION_SUCCESS == validateParams(args) )
    {
        dbg_log("Arg validation success\n");
    }
    else
    {
        printf("Validation failed, exiting\n");
        exit(0);
    }

    if ( REP_TYPE_T2 == args.report_type )
    {
        if(RBUS_ENABLED == rbus_checkStatus()) 
        {
            rbusInit(progname);
        }
    }

    key_t key;
    int msgid;
    pthread_t ptid;
    pthread_t ptid1;

    // Creating a new thread
    pthread_create(&ptid, NULL, &LatencyReportThread, NULL);
    pthread_create(&ptid1, NULL, &ClearHashThread, NULL);

    memset(hashArray,0,sizeof(TcpSniffer)*SIZE);
    // ftok to generate unique key
    key = ftok("progfile", 65);
  
    // msgget creates a message queue
    // and returns identifier
    msgid = msgget(key, 0666 | IPC_CREAT);
    //if((msgid = msgget(12345, 0666 | IPC_CREAT)) == -1)
    //perror( "server: Failed to create message queue:" );

  
    // msgrcv to receive message
    //msgrcv(msgid, &message, sizeof(message), 1, 0);
    while(msgrcv(msgid, &message, sizeof(message), 1, 0))
    {
  //perror( "server: Failed to create message queue:" );
    // display the message
   // printf("Data Received is : %s \n", message.mesg_text);
    dbg_log("Data Received is : %d \nFLAG: %d \nACK: %u\nSeq %u\n TS: %lld.%06lld\n", 
                    message.mesg_type,message.th_flag,message.th_ack,message.th_seq,message.tv_sec,message.tv_usec);
    if((message.th_flag & SYN_ACK) == SYN_ACK)
    {
        //insert(message.th_ack,message);
        int hashIndex = 0;
        hashIndex = search((message.th_ack - 1)); // Because SYN-ACK is sequence number incremented  by 1
        if(hashIndex != -1)
        {
            dbg_log("Calculate latency\n");
            /*hashArray[hashIndex].th_flag = message.th_flag;
            hashArray[hashIndex].th_ack = message.th_ack;
            hashArray[hashIndex].tv_sec  = message.tv_sec - hashArray[hashIndex].tv_sec;
            hashArray[hashIndex].tv_usec  = message.tv_usec - hashArray[hashIndex].tv_usec;*/
 
            hashArray[hashIndex].TcpInfo[INDEX_SYN_ACK].th_flag = message.th_flag;
            hashArray[hashIndex].TcpInfo[INDEX_SYN_ACK].th_seq  = message.th_seq;
            hashArray[hashIndex].TcpInfo[INDEX_SYN_ACK].th_ack  = message.th_ack;
            hashArray[hashIndex].TcpInfo[INDEX_SYN_ACK].tv_sec  = message.tv_sec;
            hashArray[hashIndex].TcpInfo[INDEX_SYN_ACK].tv_usec  = message.tv_usec; 
            //MeasureTCPLatency(hashIndex);
        }
    }
    else if((message.th_flag & SYN) == SYN)
    {
    insert((u_short)message.th_seq,message);
    }
    else if((message.th_flag & ACK) == ACK)
    {
                //insert(message.th_ack,message);
        int hashIndex = 0;
        hashIndex = search((message.th_seq - 1)); // Because SYN-ACK is sequence number incremented  by 1
        if(hashIndex != -1)
        {
            if(hashArray[hashIndex].bComputed == TRUE)
            {
                dbg_log("Already processed ack sequence... Ignoring\n");

            }
            else
            {
                dbg_log("Calculate latency after ack\n");
                /*hashArray[hashIndex].th_flag = message.th_flag;
                hashArray[hashIndex].th_ack = message.th_ack;
                hashArray[hashIndex].tv_sec  = message.tv_sec - hashArray[hashIndex].tv_sec;
                hashArray[hashIndex].tv_usec  = message.tv_usec - hashArray[hashIndex].tv_usec;*/
                hashArray[hashIndex].TcpInfo[INDEX_ACK].th_flag = message.th_flag;
                hashArray[hashIndex].TcpInfo[INDEX_ACK].th_seq  = message.th_seq;
                hashArray[hashIndex].TcpInfo[INDEX_ACK].th_ack  = message.th_ack;
                hashArray[hashIndex].TcpInfo[INDEX_ACK].tv_sec  = message.tv_sec;
                hashArray[hashIndex].TcpInfo[INDEX_ACK].tv_usec  = message.tv_usec; 
                MeasureTCPLatency(hashIndex);
            }
        }
    }
    else
    {
        //pass it
    }

   // printf("call display\n");
  //  display();
    }
  
    // to destroy the message queue
    msgctl(msgid, IPC_RMID, NULL);

    if (logFp != NULL)
    {
        fclose(logFp);
        logFp=NULL;
    }  
    return 0;
}