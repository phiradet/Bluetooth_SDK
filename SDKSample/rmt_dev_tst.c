/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*
* Copyright (c) 1999-2005 IVT Corporation
*
* All rights reserved.
*
---------------------------------------------------------------------------*/
 
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
Module Name:
    rmt_dev_tst.c
Abstract:
	Sample codes of remote device manager

Revision History:
2007-6-7   Guan Tengfei Created

---------------------------------------------------------------------------*/

#include "sdk_tst.h"

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

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
Description:
    This function is to map a string between two different code pages. 
	Help function, all arguments refer to Windows API              
Arguments:
Return:
        0 if two code pages are same or pointer to source string is NULL.
		The number of characters is translated.		 
--------------------------------------------------------------------------*/
int MultibyteToMultibyte(BTUINT32 dwSrcCodePage, char *lpSrcStr, int cbSrcStr,
						 BTUINT32 dwDestCodePage, char *lpDestStr, int cbDestStr)
{
	short *wzTemp = NULL;
	int nSrcLen = 0;

	if (dwSrcCodePage == dwDestCodePage)
	{
		return 0;
	}

	if (lpSrcStr == NULL)
	{
		return 0;
	}

	if (-1 == cbSrcStr)
	{
		nSrcLen = strlen(lpSrcStr) + 1;
	}
	else
	{
		nSrcLen = cbSrcStr;
	}

	if ((NULL == lpDestStr) || (0 == cbDestStr))
	{
		return nSrcLen;
	}

	wzTemp = (short*) malloc(nSrcLen * sizeof(short));
	if (NULL == wzTemp)
	{
		return 0;
	}
	memset(wzTemp, 0, nSrcLen * sizeof(short));
	MultiByteToWideChar(dwSrcCodePage, 0, lpSrcStr, cbSrcStr, wzTemp, nSrcLen);
	nSrcLen = WideCharToMultiByte(dwDestCodePage, 0, wzTemp, -1, lpDestStr, cbDestStr, NULL, FALSE);

	free(wzTemp);
	wzTemp = NULL;
	
	return nSrcLen;
}


/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
Description:
	This function prints bluetooth address
Arguments:
	BTUINT8 *bd_addr: [in] bluetooth address
Return:
	void 
---------------------------------------------------------------------------*/
void PrintBdAddr(BTUINT8 *bd_addr)
{	
	int j;

	if (NULL == bd_addr)
	{
		return;
	}

	for (j = 5; j > 0; j--)
	{
		printf("%02X:", bd_addr[j]);
	}
	printf("%02X", bd_addr[0]);
}
	
	
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
Description:
	This function is to report each device discovered separately
Arguments:
	BTDEVHDL dev_hdl: [in] device handle
Return:
	void 
---------------------------------------------------------------------------*/
void AppInquiryInd(BTDEVHDL dev_hdl)
{
	BTUINT32 dev_class = 0;
	Btsdk_GetRemoteDeviceClass(dev_hdl, &dev_class);
	/*Just store devices of specified device class.*/
	if ( (s_rmt_dev_cls == 0)||(s_rmt_dev_cls == BTSDK_DEVCLS_MASK(dev_class & DEVICE_CLASS_MASK)))
	{
		s_rmt_dev_hdls[s_rmt_dev_num++] = dev_hdl;
	}	
}


/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
Description:
	This function is the callback function to process inquiry complete result ind
Arguments:
	void
Return:
	void 
---------------------------------------------------------------------------*/
void AppInqCompInd(void)
{
	/* notify the thread which starts device discovery */
	s_hBrowseDevEventHdl = OpenEvent(EVENT_ALL_ACCESS, FALSE, "completeBrowseDevice");
	SetEvent(s_hBrowseDevEventHdl);
}


/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
Description:
	This function is the callback function for connection complete event
Arguments:
	BTDEVHDL dev_hdl: [in] device handle
Return:
	void 
---------------------------------------------------------------------------*/
void AppConnCompleteInd(BTDEVHDL dev_hdl)
{
	BTUINT32 dev_class = 0;
	Btsdk_GetRemoteDeviceClass(dev_hdl, &dev_class);
}


/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
Description:
	This function is the callback function for connection event
Arguments:
	BTCONNHDL conn_hdl: [in] connection handle
	BTUINT16 event: [in] event
	BTUINT8 *arg: [in] reserved
Return:
	void 
---------------------------------------------------------------------------*/
void AppConnEventInd(BTCONNHDL conn_hdl, BTUINT16 event, BTUINT8 *arg)
{
	PBtSdkConnectionPropertyStru prop = (PBtSdkConnectionPropertyStru)arg;
	BTUINT8 bd_addr[6] = {0};
	BTUINT8 dev_name[120] = {0};

	if (prop != NULL)
	{
		Btsdk_GetRemoteDeviceAddress(prop->device_handle, bd_addr);
		Btsdk_GetRemoteDeviceName(prop->device_handle, dev_name, NULL);
	}
	switch (event)
	{
	case BTSDK_APP_EV_CONN_IND:
		printf("Receive BTSDK_APP_EV_CONN_IND from %s\n", dev_name);
		break;
	case BTSDK_APP_EV_CONN_CFM:
		printf("Receive BTSDK_APP_EV_CONN_CFM to %s\n", dev_name);
		break;
	case BTSDK_APP_EV_DISC_IND:
		printf("Receive BTSDK_APP_EV_DISC_IND from %s\n", dev_name);
		break;
	case BTSDK_APP_EV_DISC_CFM:
		printf("Receive BTSDK_APP_EV_DISC_CFM to %s\n", dev_name);
		break;
	default:
		printf("Receive unknown message %04x\n", event);
		break;
	}
}

/* start another thread to handle pin code event */
DWORD WINAPI ThirdpartyPinCodeThread(BTDEVHDL dev_hdl)
{
	BTUINT8 ch = 0;
	BTUINT32 res = 0;
	char szPincode[16] = {0};
	printf("The pin code is:1234\n");
	sprintf(szPincode, "1234");
	res = Btsdk_PinCodeReply(dev_hdl, szPincode, strlen(szPincode));
	return res;
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
Description:
	This function is the callback function for pin code request event
Arguments:
	BTDEVHDL dev_hdl: [in] device handle
Return:
	void 
---------------------------------------------------------------------------*/
BTUINT8  ApppincodeInd(BTDEVHDL dev_hdl)
{
	HANDLE hThread= INVALID_HANDLE_VALUE;
	printf("Create other thread to handle this event\n");	
 	hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ThirdpartyPinCodeThread, dev_hdl, 0, NULL);
	CloseHandle(hThread);
	return BTSDK_CLIENTCBK_HANDLED;
}

/* create a structure to package parameters */
typedef struct _AuthParaStru
{ 
	BTSVCHDL svc_hdl;
	BTDEVHDL dev_hdl;
}AuthParaStru,*PAuthParaStru;

/* start another thread to handle authentication event */
DWORD WINAPI ThirdpartyAuthThread(PAuthParaStru stru)
{
	/* authorization allowed */
	BTDEVHDL devHdl = stru->dev_hdl;
	BTSVCHDL svcHdl = stru->svc_hdl;
	SetEvent(s_waitAuthor);
	/*Wait for user handle authorization*/
	Btsdk_AuthorizationResponse(svcHdl, devHdl, BTSDK_AUTHORIZATION_GRANT);
	
	/* authorization denied */
	/* Btsdk_AuthorizationResponse(stru->svc_hdl, stru->dev_hdl, BTSDK_AUTHORIZATION_DENY);*/
	
	return 1;
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
Description:
	This function is the callback function for authentication event
Arguments:
    BTSVCHDL svc_hdl: [in] service handle
	BTDEVHDL dev_hdl: [in] device handle
Return:
	void 
---------------------------------------------------------------------------*/
BTUINT8 AppAuthInd(BTSVCHDL svc_hdl, BTDEVHDL dev_hdl)
{
	HANDLE hThread= INVALID_HANDLE_VALUE;
	char szChoice[4] = {0};
	AuthParaStru AuthStru = {0};
	AuthStru.svc_hdl = svc_hdl;
	AuthStru.dev_hdl = dev_hdl;
	printf("Authentication event is handled by the third party.\n");
	printf("Create other thread to handle this event\n");
	if (!s_waitAuthor)
	{
		s_waitAuthor = CreateEvent(NULL, FALSE, FALSE, NULL);
	}
 	hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ThirdpartyAuthThread, (LPVOID)&AuthStru, 0, NULL);
	WaitForSingleObject(s_waitAuthor, INFINITE);
	CloseHandle(s_waitAuthor);
	s_waitAuthor = NULL;
	CloseHandle(hThread);
	return BTSDK_CLIENTCBK_HANDLED;
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
Description:
	This function is the callback function for link key notification event
Arguments:
	BTDEVHDL device_handle: [in] device handle
	BTUINT8* link_key: [in] link key
Return:
	void 
---------------------------------------------------------------------------*/
BTUINT32 AppLinkNotifInd(BTDEVHDL device_handle, BTUINT8* link_key)
{
	printf("Received the link key of this device handle.\n");
	return 1;
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
Description:
	This function is the callback function for authentication failure event
Arguments:
	BTDEVHDL device_handle: [in] device handle
Return:
	void 
---------------------------------------------------------------------------*/
void AppAuthFailedInd(BTDEVHDL device_handle)
{
	printf("Authentication Failed! the device handle is:%x\n", device_handle);
}


/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
Description:
	This function registers callbacks.
Arguments:
	void
Return:
	void 
---------------------------------------------------------------------------*/
void RegAppIndCallback(void)
{
	BtSdkCallbackStru cb = {0};

	/* inquiry result ind*/
	cb.type = BTSDK_INQUIRY_RESULT_IND;
	cb.func = (void*)AppInquiryInd;
	Btsdk_RegisterCallback4ThirdParty(&cb);

	/* inquiry complete result ind*/
	cb.type = BTSDK_INQUIRY_COMPLETE_IND;
	cb.func = (void*)AppInqCompInd;
	Btsdk_RegisterCallback4ThirdParty(&cb);

	/* connection event ind */
	cb.type = BTSDK_CONNECTION_EVENT_IND;
	cb.func = (void*)AppConnEventInd;
	Btsdk_RegisterCallback4ThirdParty(&cb);

    /* pin code ind */
	cb.type = BTSDK_PIN_CODE_IND;
	cb.func = (BTUINT8*)ApppincodeInd;
    Btsdk_RegisterCallbackEx(&cb, BTSDK_CLIENTCBK_PRIORITY_HIGH);

	/* authorization ind */
	cb.type = BTSDK_AUTHORIZATION_IND;
	cb.func = (BTUINT8*)AppAuthInd;
	Btsdk_RegisterCallbackEx(&cb, BTSDK_CLIENTCBK_PRIORITY_HIGH);
	
	/* link key notification ind */
	cb.type = BTSDK_LINK_KEY_NOTIF_IND;
	cb.func = (BTUINT8*)AppLinkNotifInd;
	Btsdk_RegisterCallback4ThirdParty(&cb);	
	
	/* authentication fail ind */
	cb.type = BTSDK_AUTHENTICATION_FAIL_IND;
	cb.func = (BTUINT8*)AppAuthFailedInd;
	Btsdk_RegisterCallback4ThirdParty(&cb);	
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
Description:
	This function unregisters callbacks.
Arguments:
	void
Return:
	void 
---------------------------------------------------------------------------*/
void UnRegAppIndCallback(void)
{
	BtSdkCallbackStru cb;

	cb.type = BTSDK_INQUIRY_RESULT_IND;
	cb.func = NULL;
	Btsdk_RegisterCallback4ThirdParty(&cb);
	cb.type = BTSDK_INQUIRY_COMPLETE_IND;
	cb.func = NULL;	
	Btsdk_RegisterCallback4ThirdParty(&cb);
	cb.type = BTSDK_CONNECTION_EVENT_IND;
	cb.func = NULL;
	Btsdk_RegisterCallback4ThirdParty(&cb);
}


/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
Description:
	This function is to display remote devices
Arguments:
	BTUINT32 dev_class: [in] device class
Return:
	void 
---------------------------------------------------------------------------*/
void DisplayRemoteDevices(BTUINT32 dev_class)
{
	int i = 0;
	int j = 0;
	BTUINT8 szDevName[BTSDK_DEVNAME_LEN] = { 0 };
	BTUINT8 szTmp[32] = { 0 };
	BTUINT16 usLen = 0;
	BTUINT32 ulDevClass = 0;
	BTUINT8 szBdAddr[BD_ADDR_LEN] = {0};
	char cQuote = ' ';

	printf("Remote devices searched:\n");
	printf("number  device name %21hc device address %4hc device class\n", cQuote, cQuote);

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
			printf("  %d%5hc", i + 1, cQuote);
 		}
		else
		{
			printf("  %d%4hc", i + 1, cQuote);
 		}
		
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
			printf("%02X:", szBdAddr[j]);
		}
		printf("%02X%3hc", szBdAddr[0], cQuote);

		Btsdk_GetRemoteDeviceClass(s_rmt_dev_hdls[i], &ulDevClass);
		printf("0X%08X\r\n", ulDevClass);
	}
}


/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
Description:
	This function is to get stored handles of remote device.
Arguments:
Return:
	void 
---------------------------------------------------------------------------*/
void GetAllRmtDevHdl()
{
	BTSDKHANDLE hEnumHdl = BTSDK_INVALID_HANDLE;
	INT i = 0;
	s_rmt_dev_num = 0;
	memset(s_rmt_dev_hdls, 0, sizeof(s_rmt_dev_hdls));

	hEnumHdl = Btsdk_StartEnumRemoteDevice(BTSDK_ERD_FLAG_NOLIMIT, 0);
	if (BTSDK_INVALID_HANDLE != hEnumHdl)
	{
		for (i = 0; i < MAX_DEV_NUM; i++)
		{
			s_rmt_dev_hdls[i] = Btsdk_EnumRemoteDevice(hEnumHdl, NULL);
			if (BTSDK_INVALID_HANDLE == s_rmt_dev_hdls[i])
			{
				s_rmt_dev_hdls[i] = BTSDK_INVALID_HANDLE;
				break;
			}
			s_rmt_dev_num++;
		}
		Btsdk_EndEnumRemoteDevice(hEnumHdl);
		hEnumHdl = BTSDK_INVALID_HANDLE;
	}

}


char* mDisplayRemoteDevices(BTUINT32 dev_class,char* output)
{
	int nIdx = 0;
	char szChoice[4] = {0};
	char tmp[1024]="";

	int i = 0;
	int j = 0;
	BTUINT8 szDevName[BTSDK_DEVNAME_LEN] = { 0 };
	BTUINT8 szTmp[32] = { 0 };
	BTUINT16 usLen = 0;
	BTUINT32 ulDevClass = 0;
	BTUINT8 szBdAddr[BD_ADDR_LEN] = {0};
	char cQuote = ' ';

	GetAllRmtDevHdl();
	strcpy(output,"");
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
		sprintf(tmp,"%s|", szDevName);
		strcat(output,tmp);

		Btsdk_GetRemoteDeviceAddress(s_rmt_dev_hdls[i], szBdAddr);
		for(j = 5; j > 0; j --)
		{
			printf("%02X:", szBdAddr[j]);
			sprintf(tmp,"%02X:", szBdAddr[j]);
			strcat(output,tmp);
		}
		printf("%02X%3hc", szBdAddr[0], cQuote);
		sprintf(tmp,"%02X|", szBdAddr[0]);
		strcat(output,tmp);

		Btsdk_GetRemoteDeviceClass(s_rmt_dev_hdls[i], &ulDevClass);
		printf("0X%08X\r\n", ulDevClass);
		sprintf(tmp,"0X%08X;", ulDevClass);
		strcat(output,tmp);
	}
	return output;
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
Description:
	This function is to select device you want.
Arguments:
	BTUINT32 dev_class: [in] device class
Return:
	BTDEVHDL 
---------------------------------------------------------------------------*/

BTDEVHDL SelectRemoteDevice(BTUINT32 dev_class)
{
	int nIdx = 0;
	char szChoice[4] = {0};

	GetAllRmtDevHdl();
	/*show remote devices. If there is no device shown, search for them at first*/
	if (0 == s_rmt_dev_num)
	{
		StartSearchDevice(dev_class);
	}
	else
	{
		DisplayRemoteDevices(dev_class);
	}
	
	printf("Select the target device :\n"); 
	printf("if there is no expected device, please press 'a' to search again!\n");
	printf("if you want to exit this procedure, please press 'q' to quit.\n");
	
	do
	{
		printf("Target device number = ");
		scanf(" %s", szChoice);
		getchar();
		if ('a' == szChoice[0])
		{
			StartSearchDevice(dev_class);		
			continue;
		}
		if(('q' == szChoice[0]) || ('Q' == szChoice[0]))
		{
			printf("\nUser abort the operation.\n");
			return BTSDK_INVALID_HANDLE;
 		}
		nIdx = atoi(szChoice);
		if((nIdx <= 0) || (nIdx > s_rmt_dev_num))
		{
			printf("%d is not a valid datum, please select again.\n", nIdx);
			continue;
		}
		else
		{
			break;
		}
	} while (1);

	return (s_rmt_dev_hdls[nIdx - 1]);
}


BTDEVHDL mSelectRemoteDeviceByName(BTUINT32 dev_class,char* devAddr)
{
	int nIdx = 0;
	char tmpAddr[100]="";
	char tmp[100]="";
	char szChoice[4] = {0};
	int i = 0;
	int j = 0;
	BTUINT8 szDevName[BTSDK_DEVNAME_LEN] = { 0 };
	BTUINT8 szTmp[32] = { 0 };
	BTUINT16 usLen = 0;
	BTUINT32 ulDevClass = 0;
	BTUINT8 szBdAddr[BD_ADDR_LEN] = {0};
	char cQuote = ' ';

	GetAllRmtDevHdl();

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
		
		usLen = 32;
		if (Btsdk_GetRemoteDeviceName(s_rmt_dev_hdls[i], szDevName, &usLen) != BTSDK_OK)
		{
			if (Btsdk_UpdateRemoteDeviceName(s_rmt_dev_hdls[i], szDevName, &usLen) != BTSDK_OK)
			{
				strcpy((char*)szDevName, "Unknown");
			}
		}

		//strcpy(szTmp, szDevName);
		//MultibyteToMultibyte(CP_UTF8, szTmp, -1, CP_ACP, szDevName, BTSDK_DEVNAME_LEN);

		//printf("%-34hs", szDevName);
		//sprintf(tmp,"%s|", szDevName);
		//strcat(output,tmp);

		Btsdk_GetRemoteDeviceAddress(s_rmt_dev_hdls[i], szBdAddr);
		for(j = 5; j > 0; j --)
		{
			//printf("%02X:", szBdAddr[j]);
			sprintf(tmp,"%02X:", szBdAddr[j]);
			strcat(tmpAddr,tmp);
		}
		//printf("%02X%3hc", szBdAddr[0], cQuote);
		sprintf(tmp,"%02X", szBdAddr[0]);
		strcat(tmpAddr,tmp);
		if(strcmp(tmpAddr,devAddr)==0)
		{
			nIdx=i+1;
		}
		strcpy(tmpAddr,"");
		//Btsdk_GetRemoteDeviceClass(s_rmt_dev_hdls[i], &ulDevClass);
		//printf("0X%08X\r\n", ulDevClass);
		//sprintf(tmp,"0X%08X;", ulDevClass);
		//strcat(output,tmp);
	}
	
	return s_rmt_dev_hdls[nIdx - 1];

}

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
Description:
	This function searches for devices
Arguments:
	BTUINT32 device_class: [in] device class
Return:
	void 
---------------------------------------------------------------------------*/
void StartSearchDevice(BTUINT32 device_class)
{
	/*init remote devices info discovered.remote device handle and number will be set in AppInquiryInd*/
	memset(s_rmt_dev_hdls, 0, sizeof(s_rmt_dev_hdls));
	s_rmt_dev_num = 0;
	s_rmt_dev_cls = device_class;

	s_hBrowseDevEventHdl = CreateEvent(NULL, FALSE, FALSE, "completeBrowseDevice");
	printf("Please wait for a while searching for remote devices......\n");
	
	if (BTSDK_OK == Btsdk_StartDeviceDiscovery(0, MAX_DEV_NUM, 45))
	{
		/*wait BTSDK_INQUIRY_COMPLETE_IND. When complete inquiry, AppInqCompInd will notify*/
		WaitForSingleObject(s_hBrowseDevEventHdl, INFINITE);		
		
		DisplayRemoteDevices(0);
		printf("Search for remote devices completely.\n");
	}
	else
	{
		printf("Fail to initiate device searching!\n");
	}

	if (NULL != s_hBrowseDevEventHdl)
	{
		CloseHandle(s_hBrowseDevEventHdl);
		s_hBrowseDevEventHdl = NULL;
	}
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
Description:
	This function prints basic information of remote device
Arguments:
	BTDEVHDL dev_hdl: [in] device handle
Return:
	void 
---------------------------------------------------------------------------*/
void Test_Print_Remote_Basic_Info(BTDEVHDL dev_hdl)
{
	int j = 0;
	BTUINT32 ulResult = BTSDK_OK;
	BTUINT8 szDevName[BTSDK_DEVNAME_LEN] = { 0 };
	BtSdkRemoteDevicePropertyStru struDevProperty = { 0 };

	ulResult = Btsdk_GetRemoteDeviceProperty(dev_hdl, &struDevProperty);
	PrintErrorMessage(ulResult, BTSDK_TRUE);
	if(BTSDK_OK == ulResult)
	{
		MultibyteToMultibyte(CP_UTF8, struDevProperty.name, -1, CP_ACP, szDevName, BTSDK_DEVNAME_LEN);
		printf("The basic information of the selected device is :\n");
		printf("Name = \"%s\"\n", szDevName);
		printf("BD Addr: ");
		for(j = 5; j > 0; j--)
		{
			printf("%02X:", struDevProperty.bd_addr[j]);
		}
		printf("%02X\n", struDevProperty.bd_addr[0]);
		printf("Device Class: %08lX\n", struDevProperty.dev_class);		
	}
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
Description:
	This function pairs a remote device
Arguments:
	BTDEVHDL dev_hdl: [in] device handle
Return:
	void 
---------------------------------------------------------------------------*/
void Test_Btsdk_PairDevice(BTDEVHDL dev_hdl)
{
	BTUINT32 ulResult = BTSDK_OK;
	printf("Pairing...\n");
	ulResult = Btsdk_PairDevice(dev_hdl);
	if (BTSDK_OK == ulResult)
	{
		printf("Succeed in pairing with the remote device.\n");
	}
	else
	{
		PrintErrorMessage(ulResult,  BTSDK_TRUE);
	}
}

void mTest_Btsdk_PairDevice(int nIdx,char* output)
{
	BTDEVHDL dev_hdl=s_rmt_dev_hdls[nIdx - 1];
	BTUINT32 ulResult = BTSDK_OK;
	output="";
	s_curr_dev=dev_hdl;
	printf("Pairing...\n");
	ulResult = Btsdk_PairDevice(dev_hdl);
	if (BTSDK_OK == ulResult)
	{
		output="success";
	}
	else
	{
		PrintErrorMessage(ulResult,  BTSDK_TRUE);
		output="error";
	}
}




/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
Description:
	This function unpairs a remote device
Arguments:
	BTDEVHDL dev_hdl: [in] device handle
Return:
	void 
---------------------------------------------------------------------------*/
void Test_Btsdk_UnPairDevice(BTDEVHDL dev_hdl)
{
	BTUINT32 ulResult = BTSDK_OK;
	printf("Unpairing with the remote device...\n");
	ulResult = Btsdk_UnPairDevice(dev_hdl);
	if (BTSDK_OK == ulResult)
	{
		printf("Unpair with the remote device successfully.\n");
	}
	else
	{
		PrintErrorMessage(ulResult,  BTSDK_TRUE);
	}
}
void mTest_Btsdk_UnPairDevice(char* output)
{
	BTUINT32 ulResult = BTSDK_OK;
	printf("Unpairing with the remote device...\n");
	ulResult = Btsdk_UnPairDevice(s_curr_dev);
	if (BTSDK_OK == ulResult)
	{
		printf("Unpair with the remote device successfully.\n");
		strcpy(output,"success");
	}
	else
	{
		PrintErrorMessage(ulResult,  BTSDK_TRUE);
		strcpy(output,"error");
	}
}


/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
Description:
	This function deletes a remote device
Arguments:
	BTDEVHDL dev_hdl: [in] device handle
Return:
	void 
---------------------------------------------------------------------------*/
void Test_Btsdk_DelRemoteDevice(BTDEVHDL dev_hdl)
{
	BTUINT32 ulResult = BTSDK_OK;
	int i = 0;

	ulResult  = Btsdk_DeleteRemoteDeviceByHandle(dev_hdl);
	PrintErrorMessage(ulResult, BTSDK_TRUE);
	
	if (BTSDK_OK == ulResult)
	{
		for (i = 0; i < s_rmt_dev_num; i++)
		{
			if (s_rmt_dev_hdls[i] == s_curr_dev)
			{
				break;
			}
		}
	
		if ( i != s_rmt_dev_num)
		{
			for (; i < s_rmt_dev_num - 1; i++)
			{
				s_rmt_dev_hdls[i] = s_rmt_dev_hdls[i + 1];
			}
			s_curr_dev = 0;
			printf("Delete the remote device successfully.\n");
		}
	}
}


/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
Description:
	This function shows the remote device manager sample menu
Arguments:
Return:
	void 
---------------------------------------------------------------------------*/
void RmtDevShowMenu()
{
	printf("*****************************************\n");
	printf("*  Remote Device Manager Testing Menu   *\n");
	printf("* <1> Search remote devices             *\n");
	printf("* <2> Select a remote device            *\n");
	printf("* <3> Get a remote device's basic Info  *\n");
	printf("* <4> Pair with the selected device     *\n");
	printf("* <5> Unpair with the selected device   *\n");
	printf("* <6> Delete the selected device        *\n");
	printf("* <r> Return to the upper menu          *\n");
	printf("*****************************************\n");    
	printf(">");
}


/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
Description:
	This function is to execute user's choice
Arguments:
	BTUINT8 choice: [in] user's choice
Return:
	void 
---------------------------------------------------------------------------*/
void RmtDevExecCmd(BTUINT8 choice)
{
	switch (choice) 
	{
	case '1':
		StartSearchDevice(0);/*search all devices*/
		break;
	case '2':
		s_curr_dev = SelectRemoteDevice(0);
		if (BTSDK_INVALID_HANDLE != s_curr_dev)
		{
			printf("You have succeeded in getting a remote device's handle.");
		}
		break;
	case '3':
		Test_Print_Remote_Basic_Info(s_curr_dev);
		break;
	case '4':
		Test_Btsdk_PairDevice(s_curr_dev);
		break;
	case '5':
		Test_Btsdk_UnPairDevice(s_curr_dev);
		break;
	case '6':
		Test_Btsdk_DelRemoteDevice(s_curr_dev);
		break;
	default:
		printf("Invalid command.\n");
		break;
	}
}



/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
Description:
	This function is the main function of remote device manager
Arguments:
	void
Return:
	void 
---------------------------------------------------------------------------*/
void TestRmtDevMgr(void)
{
	BTUINT8 cChoice = 0;

	printf("Please carry out the following menu step by step because of the continuity in function.\n");
	RmtDevShowMenu();	
	while (cChoice != 'r')
	{
		scanf(" %c", &cChoice);
		getchar();		
		if (cChoice == '\n')
		{
			printf(">>");
		}
		else if('r' == cChoice)
		{
			break;
		}
		else
		{
			RmtDevExecCmd(cChoice);
			printf("\n");
			RmtDevShowMenu();
		}
	}
}


