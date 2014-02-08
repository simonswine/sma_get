/* 
  sma_get.c 
 
  v0.910 2012-10-14 beta version
  v0.920 2012-10-15 beta version : include debug, and async device detection
  v0.930 2012-10-16 beta version : max exit in detection waitloop, prevents for endless loop
  v0.940 2012-10-16 beta version : core-dump fix and lots of code improvements
  v0.950 2012-10-24 beta version : more comments and minor bugfixes
  v0.960 2012-11-25 beta version : sync or async detect option, monitor loop, daemon mode, Pdc in stead off Pac
  v0.970 2012-11-26 beta version : changed pipes to ipc socket
  v0.971 2012-11-27 beta version : solved mem-leak
  v0.972 2012-11-28 beta version : deamon running request
  v0.980 2012-11-29 beta version : deamon request for multiple sma inverters
  v0.981 2012-12-02 beta version : minor bug-fix for 32bit compile
  v0.982 2013-02-13 beta version : not existing inverter in daemon mode will result as 0, 
                                   bugfix for coredump with some SMA models
                                   detection of A or mA, output will be in Amps
                                   

  Project site: http://code.google.com/p/sma-get
 
  build by Roland Breedveld, based on libyasdi library's
  for SMA support with the 123Solar project http://www.123solar.org/
 
  mail: Roland@Breedveld.net
 
  yasdiconf should be placed here: /etc/yasdi.ini
 
  To run (or to compile) you need the libs from libyasdi_1_8_1
  http://www.sma.de/en/products/monitoring-systems/yasdi.html
 
  location of sma_get.c : /usr/local/src/libyasdi_1_8_1/sma_get_<version>/sma_get.c

  Compile command:
  gcc sma_get.c -I../include/ -I../smalib -I../libs -o sma_get -lyasdimaster

  Licences:
  sma_get : GNU GPL v3
  YASDI   : GNU GPL v2 written by SMA Solar Technology AG.
 */

#include "libyasdimaster.h"
#include "stdio.h"
#include "time.h"
#include "string.h"
#include "stdlib.h"
#include "unistd.h"
#include "errno.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "sys/socket.h"
#include "sys/un.h"
#include "fcntl.h"
#define SOCK_PATH "/var/tmp/sma_get_socket"


int print_help(char sma_get_version[7], char version_date[11] ) {
  sma_get_version[7]='\0';
  version_date[11]='\0';
  printf("\n%s\n", "sma_get for 123Solar"); 
  printf("%s\n", "based on libyasdi from SMA Solar Technology ag"); 
  printf("%s %s %s\n\n", sma_get_version, version_date, "by roland@breedveld.net"); 
  printf("%s\n\n", "Config file: /etc/yasdi.ini"); 
  printf("%s\n", "Options:"); 
  printf("%s\n", " -h show this help"); 
  printf("%s\n", " -d show data"); 
  printf("%s\n", " -b show debug information, written to errout"); 
  printf("%s\n", " -i show info"); 
  printf("%s\n", " -e show events"); 
  printf("%s\n", " -c show data with comments"); 
  printf("%s\n", " -a show alarms"); 
  printf("%s\n", " -s show possible status fields"); 
  printf("%s\n", " -r show state of Daemon"); 
  printf("%s\n", " -S synchrone detection, default is asynchrone"); 
  printf("%s\n", " -D start sma_get as daemon"); 
  printf("%s\n", " -m monitor loop"); 
  printf("%s\n", " -x send exit to daemon"); 
  printf("%s\n", " -v show version"); 
  printf("%s\n\n", " -n<0-9> deviceNr, default -n0"); 
  printf("%s\n", "sma_get  project:  http://code.google.com/p/sma-get"); 
  printf("%s\n", "123Solar project:  http://www.123solar.org"); 
  printf("%s\n\n", "YASDI    software: http://www.sma.de/en/products/monitoring-systems/yasdi.html");
  return 0;
}

int test_daemon_socket(int start_daemon, int show_debug){
  int socket1, socket_len, return_s;

  if(show_debug==1) fprintf(stderr, "%s\n", "Detect daemon...");
  struct sockaddr_un remote;
  if ((socket1 = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
    fprintf(stderr, "%s\n", "Socket Error");
    return_s=1;
  }else{
    if(show_debug==1) fprintf(stderr, "%s\n", "Trying to connect...");
    remote.sun_family = AF_UNIX;
    strcpy(remote.sun_path, SOCK_PATH);
    socket_len = strlen(remote.sun_path) + sizeof(remote.sun_family);
    if (connect(socket1, (struct sockaddr *)&remote, socket_len) == -1) {
      if(show_debug==1) fprintf(stderr, "%s\n", "No daemon found");
      return_s=1;
    }else{
      if(show_debug==1) fprintf(stderr, "%s\n", "Connected to daemon");
      return_s=0;
      if(show_debug==1) fprintf(stderr, "%s\n", "Close socket");
      close(socket1);
      if(start_daemon==1){
        if(show_debug==1) fprintf(stderr, "%s\n", "Close socket");
        fprintf(stderr, "%s\n", "Daemon already running");
        return_s=1;
        exit(1);
      }
    }
  }
  return return_s;
}

int main(int argc, char *argv[])
{
  char sma_get_version[7]="v0.982";
  char version_date[11]="2013-02-13";
  sma_get_version[7]='\0';
  version_date[11]='\0';
  int i;
  DWORD channelHandle=0;  
  DWORD SerNr=0;          
  DWORD DeviceHandle[10]; 
  int device_nr=0; 
  int nr_of_devices=0; 
  int ires=0;             
  DWORD dwBDC=0;          
  double values[32];          
  char units[32][5];          
  char valuetext[17];     
  char st[30];
  size_t ft;
  struct tm tim;
  time_t now;
  char NameBuffer[50];
  int show_debug=0;
  int show_help=0;
  int show_data=0;
  int show_info=0;
  int show_events=0;
  int show_alarms=0;
  int show_comments=0;
  int show_status=0;
  int show_version=0;
  int start_daemon=0;
  int daemon_state=0;
  int request_daemon=0;
  int synchrone_detect=0;
  int return_code=0;
  int devdetect=0;
  int devdetectloop=0;
  int init_comm=0;
  int monitor=0;
  int exit_loop=0;
  int daemon_exit=0;
  int socket1, socket2, socket_size, socket_len;
  char socket_command[2];
  char socket_out[1024];
  int sn;
  int data_state;
  char StatText[30];
  int TxtCnt;

  struct sockaddr_un local, remote;


  /*
     sequence and dummy fields to be compatible with the aurora command output 
     aurora's show the data of 2 individual strings and the total values, didn't see this function with SMA's 
     so, sting 1 is the same as te total data.
     SMA don't have temperature, efficiency, day, week and month counters. 
     efficiency is calculated between panel-power and grid-power, it looks a lot lower than the efficiency with aurora's
     e.g. 98% on an aurora and 87% on a SunnyBoy 1200, don't know if the aurora Eff is right, calculating by hand will
     show lower values.... 

     Original aurora fields for data collection:
        date/time, Input 1 Voltage, Input 1 Current, Input 1 Power, Input 2 Voltage, Input 2 Current, Input 2 Power,
        Grid Voltage Reading, Grid Current Reading, Grid Power Reading, Frequency Reading,
        DC/AC Conversion Efficiency, Inverter Temperature, Booster Temperature,
        Daily Energy, Weekly Energy, Monthly Energy, Yearly Energy, Total Energy, Partial Energy

     Original aurora fields for extended information:
        Vbulk, Vbulk Mid, Vbulk +, Vbulk -, Vbulk (Dc/Dc), Ileak (Dc/Dc) Reading, Ileak (Inverter) Reading, 
        Isolation Resistance (Riso), Grid Voltage (Dc/Dc), Average Grid Voltage (VgridAvg), Grid Voltage neutral,
        Grid Frequency (Dc/Dc), Power Peak, Power Peak Today, Supervisor Temperature, Alim Temperature,
        Heak Sink Temperature, Temperature 1, Temperature 2, Temperature 3, Fan 1 Speed, Fan 2 Speed, Fan 3 Speed,
        Fan 4 Speed, Fan 5 Speed, Pin 1, Pin 2, Power Saturation limit (Der.), Reference Ring Bulk,
        Vpanel micro, Wind Generator Frequency
*/

  char data_set[20][20]={"Upv-Ist", "Ipv", "Pdc", "dummy", "dummy", "dummy", "Uac", "Iac-Ist", "Pac", "Fac", "Effcy", "dummy", "dummy", "dummy", "dummy", "dummy", "dummy", "dummy", "E-Total", "dummy" }; 
  char event_set[31][20]={"Upv-Ist","Upv-Ist","dummy","dummy","Upv-Ist","dummy","dummy","Riso","Uac","Uac","Uac" ,"Fac","Plimit","dummy","dummy","dummy","dummy","dummy","dummy","dummy","dummy","dummy","dummy","dummy","dummy","Pac","dummy","dummy","dummy","dummy","dummy"};

  if(argc<2) {
    fprintf(stderr, "\n%s\n", "ERROR: No options given, showning help");
    show_help=1;
    return_code=1;
    print_help(sma_get_version, version_date);
    return return_code;
  }

  for(i=1; i < argc; i++) {
    switch (argv[i][1]) {
      case 'h':
        show_help=1;
        break;
      case 'd':
        show_data=1;
        init_comm=1;
        break;
      case 'b':
        show_debug=1;
        break;
      case 'i':
        show_info=1;
        init_comm=1;
        break;
      case 'e':
        show_events=1;
        init_comm=1;
        break;
      case 'a':
        show_alarms=1;
        init_comm=1;
        break;
      case 'c':
        show_comments=1;
        break;
      case 'S':
        synchrone_detect=1;
        break;
      case 's':
        show_status=1;
        init_comm=1;
        break;
      case 'n':
        device_nr=atoi(&argv[i][2]);
        break;    
      case 'D':
        start_daemon=1;
        init_comm=1;
        break;
      case 'r':
        daemon_state=1;
        break;
      case 'm':
        monitor=1;
        break;
      case 'x':
        daemon_exit=1;
        break;
      case 'v':
        show_version=1;
        break;
    }
  }
  if(start_daemon==1){
    nr_of_devices=device_nr; 
  }

  if (test_daemon_socket(start_daemon, show_debug) == 0){
    request_daemon=1;
    init_comm=0;
    sleep(1);
  }

  if(daemon_state == 1){
    if(request_daemon == 1){
      printf("%s\n", "Daemon is running"); 
    }else{
      printf("%s\n", "Daemon is down"); 
    }
  }

  if(show_debug==1){
    fprintf(stderr, "   Selected Options:\n" );
    fprintf(stderr, "      debug        :%d", show_debug);
    fprintf(stderr, "      data         :%d", show_data);
    fprintf(stderr, "      events       :%d\n", show_events);
    fprintf(stderr, "      alarms       :%d", show_alarms);
    fprintf(stderr, "      info         :%d", show_info);
    fprintf(stderr, "      comments     :%d\n", show_comments);
    fprintf(stderr, "      help         :%d", show_help);
    fprintf(stderr, "      initcommport :%d", init_comm);
    fprintf(stderr, "      status       :%d\n", show_status);
    fprintf(stderr, "      synchrone    :%d", synchrone_detect);
    fprintf(stderr, "      DeviceNr     :%d", device_nr);
    fprintf(stderr, "      StartDaemon  :%d\n", start_daemon);
    fprintf(stderr, "      DaemonState  :%d", daemon_state);
    fprintf(stderr, "      RequestDaemon:%d", request_daemon);
    fprintf(stderr, "      monitor      :%d\n", monitor);
    fprintf(stderr, "      Version      :%d", show_version);
    fprintf(stderr, "      ExitDaemon   :%d\n", daemon_exit);
  }

  if(show_help==1){
    print_help(sma_get_version, version_date);
  }
  if(show_version==1){
    printf("%s %s\n\n", sma_get_version, version_date); 
  }
  if(init_comm==1){
    if(show_debug==1) fprintf(stderr, "%s\n", "yasdiMasterInitialize /etc/yasdi.ini");
    yasdiMasterInitialize("/etc/yasdi.ini",&dwBDC); 
    
    for(i=0; i < dwBDC; i++) {
      if(show_debug) fprintf(stderr, "%s\n", "yasdiMasterSetDriverOnline");
      yasdiMasterSetDriverOnline( i );
    }

    if (0==dwBDC){
      fprintf(stderr, "%s\n", "Warning: No YASDI driver was found.");
      return 1;
    }

    if(synchrone_detect==0)
    { 
      if(show_debug==1) fprintf(stderr, "%s\n", "DoStartDeviceDetection asyc ...");
      ires = DoStartDeviceDetection(device_nr+1, false);  /* set async with wait loop */
      while (devdetect!=1) {
        sleep(1);
        if(show_debug==1) fprintf(stderr, "%s ", "Wait for GetDeviceHandles");
        ires = GetDeviceHandles(DeviceHandle, 10 );
        if(show_debug==1) fprintf(stderr, "%i\n", ires);
        if (ires>device_nr) {
          if(show_debug==1) {
            fprintf(stderr, "%s", "Found device:");
            GetDeviceName(DeviceHandle[device_nr], NameBuffer, sizeof(NameBuffer)-1);
            fprintf(stderr, "%3lu %s\n\n", (unsigned long)DeviceHandle[device_nr], NameBuffer);
            fprintf(stderr, "%s", "Stopping async device detection...\n");
          }
          devdetect=1;
          DoStopDeviceDetection();
        } else {
          devdetectloop++;
          if (devdetectloop > 15) {
            fprintf(stderr, "%s\n", "Device detect timeout, or device_nr not found");
            return 1;
          }
        }
      }
    }else{
      if(show_debug==1) fprintf(stderr, "%s\n", "DoStartDeviceDetection syc ...");
      ires = DoStartDeviceDetection(device_nr+1, true); 
      if(show_debug==1) fprintf(stderr, "%s ", "wait for GetDeviceHandles");
      ires = GetDeviceHandles(DeviceHandle, 10 );
      if(show_debug==1) {
        fprintf(stderr, "%i\n", ires);
        fprintf(stderr, "%s", "Found device:");
        GetDeviceName(DeviceHandle[device_nr], NameBuffer, sizeof(NameBuffer)-1);
        fprintf(stderr, "%3lu %s\n\n", (unsigned long)DeviceHandle[device_nr], NameBuffer);
      }
    }
    if (ires) {
      while (exit_loop==0) {
        socket_out[0] = '\0';
        if(start_daemon==1){
          if(show_debug==1) fprintf(stderr, "%s\n", "Initializing Socket1");
          if((socket1 = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
            perror("socket");
            exit(1);
          }
          local.sun_family = AF_UNIX;
          strcpy(local.sun_path, SOCK_PATH);
          unlink(local.sun_path);
          socket_len = strlen(local.sun_path) + sizeof(local.sun_family);
          if (bind(socket1, (struct sockaddr *)&local, socket_len) == -1) {
             perror("bind");
             exit(1);
          }
          if (listen(socket1, 5) == -1) {
             perror("listen");
             exit(1);
          }
          socket_size = sizeof(remote);
          /*struct sockaddr_un local, remote;*/
          if(show_debug==1) fprintf(stderr, "%s\n", "Waiting for client connection...");
          if ((socket2 = accept(socket1, (struct sockaddr *)&remote, &socket_size)) == -1) {
            if(show_debug==1) fprintf(stderr, "%s\n", "accept\n");
            perror("accept");
            exit(1);
          }
          exit_loop=0;
          show_data=0;
          show_alarms=0;
          show_events=0;
          show_status=0;
          show_info=0;
          if(show_debug==1) fprintf(stderr, "%s\n", "Client Connected.");
          socket_command[1] = '\0';
          socket_command[0] = '\0';
          device_nr=0;
          sn = recv(socket2, socket_command, 2, 0);
          socket_command[sn] = '\0';
          if(show_debug==1) fprintf(stderr, "%s %s\n", "Received from socket:", socket_command);
          switch (socket_command[0]) {
            case 'x':
              exit_loop=1;
              break;
            case 'd':
              show_data=1;
              break;
            case 'a':
              show_alarms=1;
              break;
            case 'e':
              show_events=1;
              break;
            case 's':
              show_status=1;
              break;
            case 'i':
              show_info=1;
              break;
          }
          device_nr=atoi(&socket_command[1]);
          if(device_nr > nr_of_devices){
            if(show_debug==1) fprintf(stderr, "%s%i[%i]\n", "Requesting uninitialised deviceNr:", device_nr, nr_of_devices);
            show_data=0;
            show_alarms=0;
            show_events=0;
            show_status=0;
            show_info=0;
            if(show_debug==1) fprintf(stderr, "%s\n", "Output data to socket2");
            sprintf(socket_out, "%s\n", "0");
            if (send(socket2, socket_out, sn, 0) == -1){
              perror("send");
              exit(1);
            }
            socket_out[0] = '\0';
          }else{
            if(show_debug==1) fprintf(stderr, "%s %i\n", "Requesting deviceNr", device_nr);
          }
          socket_command[0] = '\0';
        }

        if(show_data==1){
          data_state=0;
          for(i=0; i < 20; i++) {
            units[i][0] = '\0';
            if(strcmp(data_set[i],"dummy") == 0) {
              if(show_debug==1) fprintf(stderr, "%s\n", "dummy, set output to 0");
              values[i]=0;
            } else if(strcmp(data_set[i],"Effcy") == 0) {
              if(show_debug==1) fprintf(stderr, "%s\n", "Efficiency value not in SMA, set to 0");
              values[i]=0;
            } else if(strcmp(data_set[i],"Pdc") == 0) {
              if(show_debug==1) fprintf(stderr, "%s\n", "DC Power value not in SMA, set to 0");
              values[i]=0;
            } else {
              if(data_state==1){
                values[i]=0;
              }else{
                if(show_debug==1) fprintf(stderr, "GetChannelHandle for %s\n ", data_set[i]);
                channelHandle = FindChannelName(DeviceHandle[device_nr],  data_set[i]);
                if(show_debug==1) fprintf(stderr, "GetChannelValue for %u :", channelHandle);
                ires = GetChannelValue(channelHandle, DeviceHandle[device_nr], &values[i], valuetext, 16, 5 );
                ires = GetChannelUnit(channelHandle, units[i], sizeof(units[i])-1 ); 
                units[i][sizeof(units[i])] = '\0';
                if (values[i] == -1){
                  data_state=1;
                  exit_loop=1;
                  if(show_debug==1) fprintf(stderr, "%s\n", "Communication ERROR");
                }else{
                  if(show_debug==1) fprintf(stderr, "%f %s\n", values[i], valuetext);
                }
                if(strcmp(units[i],"mA") == 0) {
                  values[i]=values[i]/1000;
                  strcpy(units[i], "A");
                  if(show_debug==1) fprintf(stderr, "%s %f\n", "mA to A: ",values[i]);
                }
              }
            }
          }

          values[2]=values[0] * values[1];
          if(show_debug==1) fprintf(stderr, "%s %f\n", "Calculated DC Power: ",values[2]);

          if ( values[7] > 0 ) {
            values[10]=100 * ((values[6] * values[7]) / (values[0] * values[1])) ; 
          } else {
            values[10]=0;
          }   
          if(show_debug==1) fprintf(stderr, "%s %f\n", "Calculate Efficiency: ",values[10]);
       
          now = time(NULL);
          tim = *(localtime(&now));
          ft = strftime(st,30,"%Y%m%d-%H:%M:%S",&tim);
          if(values[0] > 0) {
            if(show_comments==1) sprintf(socket_out, "%-15s ", "Date");
            sprintf(socket_out, "%s%s",socket_out, st);
            for(i=0; i < 20; i++) {
              if(show_comments==1) sprintf(socket_out, "%s\n %-15s %-6s",socket_out, data_set[i], units[i]);
              sprintf(socket_out, "%s %lf ", socket_out, values[i]);
            }
            if(show_comments==1) sprintf(socket_out, "%s\n %-15s",socket_out, "State");
            if(data_state==1){
              sprintf(socket_out, "%s %s\n", socket_out, "ERROR");
            }else{
              sprintf(socket_out, "%s %s\n", socket_out, "OK");
            }
            sn=strlen(socket_out);
            socket_out[sn] = '\0';
            if(start_daemon==1){
              if(show_debug==1) fprintf(stderr, "%s\n", "Output data to socket2");
              if (send(socket2, socket_out, sn, 0) == -1){
                perror("send");
                exit(1);
              }
            }else{
              if(show_debug==1) fprintf(stderr, "%s\n", "Output data to stdout");
              printf("%s", socket_out); 
            }
            if(show_debug==1) fprintf(stderr, "%s", socket_out);
            socket_out[0] = '\0';
          }
        }

        if(show_info==1){
           GetDeviceName(DeviceHandle[device_nr], NameBuffer, sizeof(NameBuffer)-1);
           sprintf(socket_out, "%-3lu %s\n", (unsigned long)DeviceHandle[device_nr], NameBuffer);
           channelHandle = FindChannelName(DeviceHandle[device_nr], "Software-BFR");
           ires = GetChannelValue(channelHandle, DeviceHandle[device_nr], &values[0], valuetext, 16, 5 );
           sprintf(socket_out, "%s%s %lf %s\n",socket_out, "Software-BFR",values[0],valuetext);  
           channelHandle = FindChannelName(DeviceHandle[device_nr], "SMA-SN");
           ires = GetChannelValue(channelHandle, DeviceHandle[device_nr], &values[0], valuetext, 16, 5 );
           sprintf(socket_out, "%s%s %lf %s\n",socket_out, "SMA-SN",values[0],valuetext);  
           channelHandle = FindChannelName(DeviceHandle[device_nr], "Default");
           ires = GetChannelValue(channelHandle, DeviceHandle[device_nr], &values[0], valuetext, 16, 5 );
           sprintf(socket_out, "%s%s %lf %s\n",socket_out, "Default",values[0],valuetext);  
           channelHandle = FindChannelName(DeviceHandle[device_nr], "Status");
           ires = GetChannelValue(channelHandle, DeviceHandle[device_nr], &values[0], valuetext, 16, 5 );
           sprintf(socket_out, "%s%s %s\n",socket_out, "Status",valuetext);  
           channelHandle = FindChannelName(DeviceHandle[device_nr], "Betriebsart");
           ires = GetChannelValue(channelHandle, DeviceHandle[device_nr], &values[0], valuetext, 16, 5 );
           sprintf(socket_out, "%s%s %s\n",socket_out, "Betriebsart",valuetext);  
           /* Disabled, wil result in a core-dump with some smas
             channelHandle = FindChannelName(DeviceHandle[device_nr], "Phase");
             ires = GetChannelValue(channelHandle, DeviceHandle[device_nr], &values[0], valuetext, 16, 5 );
             sprintf(socket_out, "%s%s %s\n\n",socket_out, "Phase",valuetext);  
           */
           if(start_daemon==1){
             sprintf(socket_out, "%s%s %s %s\n",socket_out, "sma_get", sma_get_version, "daemon: Running");  
           }else{
             sprintf(socket_out, "%s%s %s %s\n",socket_out, "sma_get", sma_get_version, "daemon: Down");  
           }
           sn=strlen(socket_out);
           socket_out[sn] = '\0';
           if(start_daemon==1){
             if (send(socket2, socket_out, sn, 0) == -1){
               perror("send");
               exit(1);
              }
            }else{
              printf("%s", socket_out); 
            }
            socket_out[0] = '\0';
        }
        if(show_alarms==1){
           channelHandle = FindChannelName(DeviceHandle[device_nr], "Fehler");
           ires = GetChannelValue(channelHandle, DeviceHandle[device_nr], &values[0], valuetext, 16, 5 );
           if(start_daemon==1){
             sprintf(socket_out, "%s %s\n", "Fehler", valuetext);  
             sn=strlen(socket_out);
             socket_out[sn] = '\0';
             if (send(socket2, socket_out, sn, 0) == -1){
               perror("send");
               exit(1);
             }
             if(show_debug==1) fprintf(stderr, "%s\n", "Output data to stdout");
             if(show_debug==1) fprintf(stderr, "%s\n", socket_out);
             socket_out[0] = '\0';
           }else{
             if(show_debug==1) fprintf(stderr, "%s\n", "Output data to stdout");
             printf("\n%s %s\n", "Fehler", valuetext);  
             if(show_debug==1) fprintf(stderr, "%s %s\n", "Fehler", valuetext);
           }
        }
        if(show_status==1){
          channelHandle = FindChannelName(DeviceHandle[device_nr], "Status");
          TxtCnt = GetChannelStatTextCnt( channelHandle );
          if (TxtCnt) {
            sprintf(socket_out, "%s\n", "All possible status texts for Status:");
            for(i=0;i<TxtCnt;i++) {
              GetChannelStatText(channelHandle, i, StatText, sizeof(StatText)-1);
              sprintf(socket_out, "%s(%2d): '%s'\n",socket_out,i,StatText);
            }
          }
          channelHandle = FindChannelName(DeviceHandle[device_nr], "Fehler");
          TxtCnt = GetChannelStatTextCnt( channelHandle );
          if (TxtCnt) {
            sprintf(socket_out, "%s%s\n",socket_out,"All possible status texts for Fehler:");
            for(i=0;i<TxtCnt;i++) {
              GetChannelStatText(channelHandle, i, StatText, sizeof(StatText)-1);
              sprintf(socket_out, "%s(%2d): '%s'\n",socket_out,i,StatText);
            }
          }
          channelHandle = FindChannelName(DeviceHandle[device_nr], "Betriebsart");
          TxtCnt = GetChannelStatTextCnt( channelHandle );
          if (TxtCnt) {
            sprintf(socket_out, "%s%s\n",socket_out,"All possible status texts for Betriebsart:");
            for(i=0;i<TxtCnt;i++) {
              GetChannelStatText(channelHandle, i, StatText, sizeof(StatText)-1);
              sprintf(socket_out, "%s(%2d): '%s'\n",socket_out,i,StatText);
            }
          }
          sn=strlen(socket_out);
          socket_out[sn] = '\0';
          if(start_daemon==1){
            if (send(socket2, socket_out, sn, 0) == -1){
              perror("send");
              exit(1);
            }
          }else{
            printf("%s", socket_out);  
          }
          socket_out[0] = '\0';
        }
        if(show_events==1){
         for(i=0; i < 31; i++) {
           if(strcmp(event_set[i],"dummy") == 0) {
             if(show_debug==1) fprintf(stderr, "%s\n", "dummy, set output to 0");
             values[i]=0;
           } else if(strcmp(event_set[i],"Effcy") == 0) {
             if(show_debug==1) fprintf(stderr, "%s\n", "Efficiency not calcutated in SMA, set to 0");
             values[i]=0;
           } else {
             if(show_debug==1) fprintf(stderr, "GetChannelHandle for %s\n ", event_set[i]);
             channelHandle = FindChannelName(DeviceHandle[device_nr],  event_set[i]);
             if(show_debug==1) fprintf(stderr, "GetChannelValue for %u :", channelHandle);
             ires = GetChannelValue(channelHandle, DeviceHandle[device_nr], &values[i], valuetext, 16, 5 );
             if(show_debug==1) fprintf(stderr, "%f %s\n", values[i], valuetext);
           }
         }
         /* kohm to MOhm */
         values[7]=values[7]/1000;
   
         if(show_debug==1) fprintf(stderr, "%s\n", "Output data");
         if(values[0] > 0) {
           for(i=0; i < 31; i++) {
             if(start_daemon==1){
               sprintf(socket_out, "%s %lf ", socket_out, values[i]);
             }else{
               if(show_comments==1) printf("\n  %-15s", event_set[i]);
               printf(" %lf", values[i]);  
             }
           }
           if(start_daemon==1){
             sprintf(socket_out, "%s %s\n ", socket_out, "OK");
             sn=strlen(socket_out);
             socket_out[sn] = '\0';
             if (send(socket2, socket_out, sn, 0) == -1){
               perror("send");
               exit(1);
             }
             socket_out[0] = '\0';
           }else{
             if(show_comments==1) printf("\n  %-15s", "State");
             printf(" %s\n", "OK"); 
           }
         }
        }
        if((start_daemon==1)&&(exit_loop==1)){
          if(show_debug==1) fprintf(stderr, "%s\n", "Exit Daemon");
          exit_loop=1;
        }
        if(monitor==1){
          sleep(1);
        }else{
          if(start_daemon!=1){
            exit_loop=1;
          }
        }
        if((start_daemon==1)||(request_daemon==1)){
          if(show_debug==1) fprintf(stderr, "%s\n", "Close socket1");
          close(socket1);
          if(show_debug==1) fprintf(stderr, "%s\n", "Close socket2");
          close(socket2);
        }
      }
    } else {
      printf("no devices currently available\n");
    }
  }

  if(request_daemon==1){
    while (exit_loop==0) {
      if ((socket1 = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        fprintf(stderr, "%s\n", "Socket Error");
        exit(1);
      }else{
        if(show_debug==1) fprintf(stderr, "%s\n", "Trying to connect...");
        remote.sun_family = AF_UNIX;
        strcpy(remote.sun_path, SOCK_PATH);
        socket_len = strlen(remote.sun_path) + sizeof(remote.sun_family);
        if (connect(socket1, (struct sockaddr *)&remote, socket_len) == -1) {
          if(show_debug==1) fprintf(stderr, "%s\n", "No daemon found");
          exit(1);
        }else{
          if(show_debug==1) fprintf(stderr, "%s\n", "Connected to daemon");

          socket_command[0]='\0';
          if(show_alarms==1){
            strcpy(socket_command, "a");
          }
          if(show_events==1){
            strcpy(socket_command, "e");
          }
          if(show_status==1){
            strcpy(socket_command, "s");
          }
          if(show_info==1){
            strcpy(socket_command, "i");
          }
          if(show_data==1){
            strcpy(socket_command, "d");
          }
          if(daemon_exit==1){
            strcpy(socket_command, "x");
          }
          if(socket_command[0]=='\0'){
            if((show_debug==1)&&(show_help==0)) fprintf(stderr, "%s\n", "ERROR: No valid socket command");
          }else{
            sprintf(socket_command, "%s%i",socket_command, device_nr);
            if(show_debug==1) fprintf(stderr, "%s %s\n", "Send command to socket1:", socket_command);
            send(socket1, socket_command, 3, 0);
            if ((sn=recv(socket1, socket_out, 512, 0)) > 0) {
              socket_out[sn] = '\0';
              printf("%s", socket_out);
              if(show_debug==1) fprintf(stderr, "%s", socket_out);
              socket_out[0] = '\0';
            } else {
              if (sn < 0){
                perror("recv");
              }else{
                printf("Server closed connection\n");
                exit(1);
              }
            }
          }
          if(show_debug==1) fprintf(stderr, "%s\n", "Close socket1");
          close(socket1);
          if(show_debug==1) fprintf(stderr, "%s\n", "Close socket2");
          close(socket2);
          if(monitor==1){
            sleep(1);
          }else{
            exit_loop=1;
          }
        }
      }
    }
  }
                      
  if(init_comm==1){
     yasdiMasterShutdown( );
  }
   
  return return_code;
} 

