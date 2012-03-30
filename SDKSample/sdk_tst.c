/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*
* Copyright (c) 1999-2005 IVT Corporation
*
* All rights reserved.
*
---------------------------------------------------------------------------*/
 
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
Module Name:
    sdk_tst.c
Abstract:
	Samples codes of IVT Bluetooth API
Revision History:
2007-5-30   Yang Songhua  Created

---------------------------------------------------------------------------*/

#include "sdk_tst.h"
#include "profiles_tst.h"

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#define MAX_MSG_LEN 1024
#define PORT        12321

#include <windows.h>
#include <winsock.h>

#define SLEEP_MILLI(x)           Sleep(x)
#define SOCKET_INIT()            { WSADATA wsa; WSAStartup(0x0101, &wsa); }
#define CLOSE_SOCKET(x)          closesocket(x)
#define SOCKET_CLEANUP()         WSACleanup()

#define THREAD_HANDLE            uintptr_t

#define THREAD_ROUTINE           DWORD WINAPI

#define CREATE_THREAD(id,routine,arg) \
   id = _beginthreadex(NULL, 0, routine, (LPVOID) arg, 0, NULL);

#define THREAD_JOIN(id) \
   WaitForSingleObject(id, INFINITE)

#define THREAD_EXIT(x) \
   _endthreadex()

// XXX find a way to determine whether strnlen exists on the system
//     as strlen is not very safe
//#define REMOVE_NEWLINE(x) x[strnlen(x,sizeof(x))-1] = '\0'
#define REMOVE_NEWLINE(x) x[strlen(x)-1] = '\0'


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
		//------------ CONNECTION CONSTANT ------------
        const char getExistingDevice[] = "getExist";
        const char syncDevice[] = "sync";
		const char unsyncDevice[] = "unsync";
		const char searchDevice[] = "search";
        const char getProperty[] = "getProp";
        const char makeCall[] = "call";
        const char endCall[] = "endCall";
        const char redial[] = "redial";
        const char createDUN[] = "createDUN";
        const char closeDUN[] = "closeDUN";
        const char connectHF[] = "connectHF";
        const char endHFP[] = "endHF";
        //----------------------------------------------

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
Description:
	This function shows the main menu
Arguments:
Return:
	void 
---------------------------------------------------------------------------*/
void SdkTestShowMenu()
{

	printf("\n\n");
	printf("  BlueSoleil SDK Sample App Ver 2.0.5    \n");
	printf("*****************************************\n");
	printf("*         BTSDK Testing Menu            *\n");
	printf("* <1> Local Device Manager              *\n");
	printf("* <2> Remote Device Manager             *\n");
	printf("* <3> Profile Manager                   *\n");
	printf("* <m> Return to This Menu Again         *\n");
	printf("* <q> Quit                              *\n");
	printf("*****************************************\n");
	printf(">");
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
Description:
	This function is a callback function to get status from COM Server
Arguments:
	usMsgType: [in] message type
	pucData:   [in] message base on message type
	param:     [in] device or service handle
	arg:       [in] not used now
Return:
	void 
---------------------------------------------------------------------------*/
void BsStatusCBKFuc(ULONG usMsgType, ULONG pucData, ULONG param, BTUINT8 *arg)
{
	/* message received */
	switch(usMsgType)
	{
	case BTSDK_BLUETOOTH_STATUS_FLAG:
		{
			switch(pucData)
			{
			case BTSDK_BTSTATUS_TURNON:
				{
					//printf("MSG: Bluetooth is turned on.\n");
					break;
				}		
			case BTSDK_BTSTATUS_TURNOFF:
				{
					//printf("MSG: Bluetooth is turned off.\n");
					break;
				}
			case BTSDK_BTSTATUS_HWPLUGGED:
				{
					//printf("MSG: Bluetooth hardware is plugged.\n");
					break;
				}
				
			case BTSDK_BTSTATUS_HWPULLED:
				{
					//printf("MSG: Bluetooth hardware is pulled out.\n");
					break;
				}
			default:
				{
					//printf("MSG: Others.\n");
					break;
				}			
			}		
			break;
		}
	default:
		{
			//printf("MSG Received. Type: OTHER MESSAGES.\n");
			break;
		}		
	}
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
Description:
	This function is to execute user's choice.
Arguments:
	BTUINT8 choice: [in] user's choice
Return:
	void 
---------------------------------------------------------------------------*/
void ExecInputCmd(BTUINT8 choice)
{
	switch (choice) 
	{
		case '1':
			TestLocDevMgr();
			break;
		case '2':
			TestRmtDevMgr();
			break;
		case '3':
			TestProfiles();						//TEST PROFILE HERE		
			break;
		case 'm':
			system("cls");
			SdkTestShowMenu();
			break;
		case 'q':
			break;
		default:
			printf("Invalid command.\n");
			break;
	}
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
Description:
	This function registers callback function to get status change of BlueSoleil.
Arguments:
	void
Return:
	void 
---------------------------------------------------------------------------*/
void Test_RegisterGetStatusCBK(void)
{
	/* register callback function to get the status change of BlueSoleil. */
	Btsdk_RegisterGetStatusInfoCB4ThirdParty(BsStatusCBKFuc);
	Btsdk_SetStatusInfoFlag(BTSDK_BLUETOOTH_STATUS_FLAG);		
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
Description:
	This function is to conduct some initialization operations.
Arguments:
Return:
	BOOL 
---------------------------------------------------------------------------*/
BOOL InitBlueSoleilForSample()
{
	if (BTSDK_TRUE != Btsdk_IsServerConnected()) /* not connected with BlueSoleil */
	{
		if (BTSDK_OK == Btsdk_Init())
		{
			printf("Connected to BlueSoleil Server successfully.\n\n");		
		}
		else
		{
			printf("Fail to connect to BlueSoleil Server.\n\n");
			return FALSE;
		}			
	}
	return TRUE;
}

char* GetExistingDevice()
{
	/*remote device handle through device discovery*/
	BTDEVHDL s_rmt_dev_hdls[MAX_DEV_NUM] = {0};
	/*remote devices number through device discovery*/
	BTINT32 s_rmt_dev_num = 0; 
	/*remote device's class.*/
	BTUINT32 s_rmt_dev_cls = 0;
	/*current used remote device handle*/
	static BTDEVHDL s_curr_dev = BTSDK_INVALID_HANDLE;
	/*event to sync device discovery*/
	static HANDLE s_hBrowseDevEventHdl = NULL;
	static HANDLE s_waitAuthor = NULL;
	int dev_class=0;

	int i = 0;
	int j = 0;
	BTUINT8 szDevName[BTSDK_DEVNAME_LEN] = { 0 };
	BTUINT8 szTmp[32] = { 0 };
	BTUINT16 usLen = 0;
	BTUINT32 ulDevClass = 0;
	BTUINT8 szBdAddr[BD_ADDR_LEN] = {0};
	char cQuote = ' ';

	char output[MAX_MSG_LEN]="";
	char tmp[MAX_MSG_LEN]="";

	//printf("Remote devices searched:\n");
	//printf("number  device name %21hc device address %4hc device class\n", cQuote, cQuote);

	for (i = 0; i < s_rmt_dev_num; i++) /* s_rmt_dev_num is get by inquiry callback */
	{
		Btsdk_GetRemoteDeviceClass(s_rmt_dev_hdls[i], &ulDevClass);
		if ((dev_class != 0) && (dev_class != BTSDK_DEVCLS_MASK(ulDevClass & DEVICE_CLASS_MASK)))
		{			
			for (j=i; j<s_rmt_dev_num-1; j++)
			{
				s_rmt_dev_hdls[j] = s_rmt_dev_hdls[j+1];
			}
			s_rmt_dev_hdls [j] = BTSDK_INVALID_HANDLE;
			s_rmt_dev_num--;
			i--;			
			continue;
		}
		/*In order to display neatly.*/
 		if (i<9)
 		{
			sprintf(tmp,"  %d%5hc", i + 1, cQuote);
 		}
		else
		{
			sprintf(tmp,"  %d%4hc", i + 1, cQuote);
 		}
		strcat(output,tmp);
		usLen = 32;
		if (Btsdk_GetRemoteDeviceName(s_rmt_dev_hdls[i], szDevName, &usLen) != BTSDK_OK)
		{
			if (Btsdk_UpdateRemoteDeviceName(s_rmt_dev_hdls[i], szDevName, &usLen) != BTSDK_OK)
			{
				strcpy((char*)szDevName, "Unknown");
			}
		}

		strcpy(szTmp, szDevName);
		MultibyteToMultibyte(CP_UTF8, szTmp, -1, CP_ACP, szDevName, BTSDK_DEVNAME_LEN);
		printf("%-34hs", szDevName);

		Btsdk_GetRemoteDeviceAddress(s_rmt_dev_hdls[i], szBdAddr);
		for(j = 5; j > 0; j --)
		{
			sprintf(tmp,"%02X:", szBdAddr[j]);
			strcat(output,tmp);
		}
		sprintf(tmp,"%02X%3hc", szBdAddr[0], cQuote);
		strcat(output,tmp);

		Btsdk_GetRemoteDeviceClass(s_rmt_dev_hdls[i], &ulDevClass);
		sprintf(tmp,"0X%08X\r\n", ulDevClass);
		strcat(output,tmp);

		
	}
	return output;
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
Description:
	This function is the main function
Arguments:
	void
Return:
	always return 0 
---------------------------------------------------------------------------*/
int UDPserver()
{
	int sock;
   struct sockaddr_in server_addr, client_addr;
   char raw_data[MAX_MSG_LEN]="";
   char *data=NULL;
   char outputData[MAX_MSG_LEN]="";
   char delims[]="#";
   int sin_size;
   char echoBuffer[255]; 

   SOCKET_INIT();

   if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1) 
   {
      perror("Socket");
      exit(1);
   }

   server_addr.sin_family = AF_INET;
   server_addr.sin_port = htons(PORT);
   server_addr.sin_addr.s_addr = INADDR_ANY;
   memset(&(server_addr.sin_zero), 0, 8);

   /* bind the server socket to the specified interface and port */
   if (bind(sock, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1)
   {
      perror("Unable to bind");
      exit(1);
   }

   sin_size = sizeof(struct sockaddr);

   printf("UDP Chat Server waiting for client...\n");
   fflush(stdout);
   
   while(1)
   {
      int bytes_received;
      
      bytes_received = recvfrom(
            sock, raw_data, sizeof(raw_data), 0, (struct sockaddr*) &client_addr,
            &sin_size);
      if (bytes_received > 0)
      {
         char ip[20];
         int port;

         strcpy(ip, inet_ntoa(client_addr.sin_addr));
         port = ntohs(client_addr.sin_port);
         raw_data[bytes_received] = '\0';
		 data = strtok( raw_data, delims );
		 if(strcmp(getExistingDevice,data)==0)
		 {
			mDisplayRemoteDevices(0,outputData);
		 }
		 else if(strcmp(syncDevice,data)==0)
		 {
			data = strtok(NULL, delims );
			mTest_Btsdk_PairDevice(atoi(data),outputData);
			printf("PAIR REQUEST -successfull");
		 }
		 else if(strcmp(unsyncDevice,data)==0)
		 {
			 mTest_Btsdk_UnPairDevice(outputData);
		 }
		 else if(strcmp(getProperty,data)==0)
		 {

		 }
		 else if(strcmp(makeCall,data)==0)
		 {

		 }
		 else if(strcmp(endCall,data)==0)
		 {

		 }
		 else if(strcmp(redial,data)==0)
		 {

		 }
		 else if(strcmp(createDUN,data)==0)
		 {
			 data = strtok(NULL, delims );
			 mDUNConnect(data,outputData);
		 }
		 else if(strcmp(closeDUN,data)==0)
		 {
			 mDUNterminate();
		 }
		 else if(strcmp(connectHF,data)==0)
		 {
			 data = strtok(NULL, delims );
			 mHfpCreate(data);
		 }
		 else if(strcmp(endHFP,data)==0)
		 {
			 mHfpTerminate();
		 }
		 else
		 {
			 strcpy(outputData,"unknown");

		 }

         //printf("[%s:%d] %s\n", ip, port, data);
         fflush(stdout);

		 

        //we simply send this string to the client
		 printf("------------\ncurr output::%s\n----------------",outputData);
		 sendto(sock, outputData, strlen(outputData), 0, (struct sockaddr *) &client_addr, sizeof(client_addr));
      }
   }

   CLOSE_SOCKET(sock);
   SOCKET_CLEANUP();

   return 0;
}

int main(void)
{
	BTUINT8 chInputCmd = 0;
	BTUINT8 chEnterChoice = 0;

	

	printf("IVT BlueSoleil SDK is being Initialized....\n");
	if (FALSE == InitBlueSoleilForSample())
	{
		printf("Fail to initialize BlueSoleil, assure BlueSoleil is installed!\n");
		printf("Press any key to exit this application please.\n");
		scanf(" %c", &chEnterChoice);
		getchar();
		return 1;
	}
	else
	{		
		RegAppIndCallback();
		Test_RegisterGetStatusCBK();
		
		if (BTSDK_TRUE != Btsdk_IsBluetoothHardwareExisted())
		{
			printf("There isn't any Bluetooth hardware detected.\n");
	        printf("1. Enter 'N' to exit this application.\n");
			printf("2. Plug a Bluetooth hardware and enter 'Y' to continue.\n");
			
			while (TRUE)
			{
				scanf(" %c",&chEnterChoice);
				getchar();

				if (('y'==chEnterChoice)||('Y'==chEnterChoice))
				{
					if (BTSDK_TRUE == Btsdk_IsBluetoothHardwareExisted())
					{
						printf("Bluetooth hardware is detected.\n");
						break;
					}
					else
					{
						printf("Bluetooth hardware isn't detected and plug it again please.\n");
						printf("Enter 'Y' to try again, Enter 'N' to exit this application.\n");
						printf(">");
					}
				}
				else if(('n'==chEnterChoice)||('N'== chEnterChoice))
				{
					return 1;
				}
				else
				{
					printf("You have entered into an invalid character.\n");
				}				
			}
		}		
		
		if (BTSDK_FALSE == Btsdk_IsBluetoothReady())
		{
			Btsdk_StartBluetooth();
		}			
		
		if (BTSDK_TRUE == Btsdk_IsBluetoothReady())
		{
		/*we default expect this application runs on desktop platform. 
			of course, you can set another device class according to your need. */
			Btsdk_SetLocalDeviceClass(BTSDK_COMPCLS_DESKTOP);				
			
			SdkTestShowMenu();	//PRINT MENU
			UDPserver();
			while (chInputCmd != 'q')
			{
				scanf(" %c", &chInputCmd);
				getchar();
				if ('\n' == chInputCmd)
				{
					printf(">>");
				}
				else
				{
					ExecInputCmd(chInputCmd); //EXECUTE HERE
					printf("\n");
					if (chInputCmd != 'q')
					{
						SdkTestShowMenu();
					}
				}
			}
			
		}
		else
		{
			printf("BlueSoleil fail to reset hardware...\n");
		}
		
		printf("IVT BlueSoleil SDK is being quitted....\n");
		Btsdk_RegisterGetStatusInfoCB4ThirdParty(NULL);
		UnRegAppIndCallback();	
		Btsdk_Done();
		return 0;
		
	}	
}

