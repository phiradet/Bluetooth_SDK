/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*
* Copyright (c) 1999-2008 IVT Corporation
*
* All rights reserved.
*
---------------------------------------------------------------------------*/
 
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
Module Name:
    spp_tst.c
Abstract:
    this module is to test SPP profiles relative functionality. 
Revision History:
	2008-12-05   Chu Enlai  Created

---------------------------------------------------------------------------*/

#include "sdk_tst.h"
#include "profiles_tst.h"
#include "Btsdk_Stru.h"

static BTCONNHDL s_currSPPConnHdl = BTSDK_INVALID_HANDLE;
static BTDEVHDL s_currRmtSppDevHdl = BTSDK_INVALID_HANDLE;
static BTSHCHDL s_currSppSvcHdl = BTSDK_INVALID_HANDLE;

static BTUINT8 s_ComSerialNum = 0;


/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
Description:
	This function is to select device's handle.
Arguments:
Return:
	void 
---------------------------------------------------------------------------*/
void TestSelectRmtSppDev()
{   
    s_currRmtSppDevHdl = SelectRemoteDevice(0);
	if (BTSDK_INVALID_HANDLE == s_currRmtSppDevHdl)
	{
		printf("Please make sure that the expected device \
				is in discoverable state and search again.\n");
	}
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
Description:
	This function is to select service handle of SPP service.
Arguments:
Return:
	void 
---------------------------------------------------------------------------*/
void TestSelectSppSvc()
{
	s_currSppSvcHdl = SelectRemoteService(s_currRmtSppDevHdl);
	if (BTSDK_INVALID_HANDLE == s_currSppSvcHdl)
	{
		printf("Cann't get expected service handle.\n");
	}
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
Description:
	This function is to connect SPP service by its service handle.
Arguments:
Return:
	void 
---------------------------------------------------------------------------*/
void TestConnectSppSvc()
{
	BTINT32 ulRet = BTSDK_FALSE;
	BTUINT32 OSComPort;

	s_ComSerialNum = Btsdk_GetASerialNum();
	Btsdk_PlugInVComm(s_ComSerialNum , &OSComPort ,1 ,COMM_SET_RECORD|COMM_SET_USAGETYPE, 2200);
	ulRet = Btsdk_InitCommObj((BTUINT8)OSComPort, BTSDK_CLS_SERIAL_PORT);
	
	if(ulRet == BTSDK_OK)		
	{	
		BtSdkSPPConnParamStru sppStru = {0};
		sppStru.size = sizeof(BtSdkSPPConnParamStru);
		sppStru.com_index = OSComPort ;
		ulRet = Btsdk_ConnectEx(s_currRmtSppDevHdl, BTSDK_CLS_SERIAL_PORT, 
					(BTINT32)&sppStru, &s_currSPPConnHdl);
		if (ulRet == BTSDK_OK)
		{
			printf("Connect remote SPP Service with local COM%d\n", OSComPort);
		}
	}
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
Description:
	This function is to show SPP Services user interface.
Arguments:
    void
Return:
	void 
---------------------------------------------------------------------------*/
void SppShowMenu()
{
	printf("*****************************************\n");
	printf("*           SPP Testing Menu            *\n");
	printf("* <1> Connect Remote SPP Service        *\n");
	printf("* <2> Disconnect                        *\n");
	printf("* <r> Return to upper menu              *\n");
	printf("*****************************************\n");
	printf(">");
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
Description:
	This function is the execution for SPP sample.
Arguments:
	BTUINT8 choice
Return:
	void 
---------------------------------------------------------------------------*/
void SppExecCmd(BTUINT8 choice)
{
	BTINT8 ch = 0;
	BTUINT8 comNum = 0;

	if (choice == '1')
	{
		TestSelectRmtSppDev();
		TestSelectSppSvc();
		TestConnectSppSvc();
		SppShowMenu();
		while (ch != 'r')
		{
			scanf(" %c", &ch);
			getchar();		
			if (ch == '\n')
			{
				printf(">>");
			}
			else if('r' == ch)
			{
				break;
			}
			else
			{
				SppExecCmd(ch);
				printf("\n");
				SppShowMenu();
			}
		}
	}
	else if (choice == '2')
	{
		if (s_currSPPConnHdl)
		{
			comNum = Btsdk_GetClientPort(s_currSPPConnHdl);
			Btsdk_Disconnect(s_currSPPConnHdl);
			s_currSPPConnHdl = BTSDK_INVALID_HANDLE;
			Btsdk_DeinitCommObj(comNum);
			Btsdk_PlugOutVComm(s_ComSerialNum, COMM_SET_RECORD);
		}
		else
		{
			printf("make sure exist connection now!");
		}
	}
	else
	{
		printf("Invalid command.\n");
	}
}

void mDUNConnect(char* deviceAddr,char* portNum)
{
	BTINT32 ulRet = BTSDK_FALSE;
	BTUINT32 OSComPort;

	//TestSelectRmtSppDev();	-- s_currRmtSppDevHdl = SelectRemoteDevice(0);
	s_currRmtSppDevHdl=mSelectRemoteDeviceByName(0,deviceAddr);
	//TestSelectSppSvc();		-- s_currSppSvcHdl = SelectRemoteService(s_currRmtSppDevHdl);
	s_currSppSvcHdl=mSelectRemoteServiceByName(s_currRmtSppDevHdl,BTSDK_CLS_DIALUP_NET);

	s_ComSerialNum = Btsdk_GetASerialNum();
	//printf("port number:%d",s_ComSerialNum);
	Btsdk_PlugInVComm(s_ComSerialNum , &OSComPort ,1 ,COMM_SET_RECORD|COMM_SET_USAGETYPE, 2200);
	//ulRet = Btsdk_InitCommObj((BTUINT8)OSComPort, BTSDK_CLS_SERIAL_PORT);
	ulRet = Btsdk_InitCommObj((BTUINT8)OSComPort, BTSDK_CLS_DIALUP_NET);
	
	if(ulRet == BTSDK_OK)		
	{	
		//BtSdkSPPConnParamStru sppStru = {0};
		BtSdkDUNConnParamStru sppStru = {0};
		//sppStru.size = sizeof(BtSdkSPPConnParamStru);
		sppStru.size = sizeof(BtSdkDUNConnParamStru);
		sppStru.com_index = OSComPort ;
		//ulRet = Btsdk_ConnectEx(s_currRmtSppDevHdl, BTSDK_CLS_DIALUP_NET, 
		ulRet = Btsdk_ConnectEx(s_currRmtSppDevHdl, BTSDK_CLS_SERIAL_PORT, 
					(BTINT32)&sppStru, &s_currSPPConnHdl);
		if (ulRet == BTSDK_OK)
		{
			printf("Connect remote DUN Service with local COM%d\n", OSComPort);
		}
		sprintf(portNum,"%d",OSComPort);
	}
	else
	{
		PrintErrorMessage(ulRet, BTSDK_TRUE);
		mPrintErrorMessage(ulRet, portNum);
	}
}

void mDUNterminate()
{
	BTINT8 ch = 0;
	BTUINT8 comNum = 0;
	if (s_currSPPConnHdl)
	{
		comNum = Btsdk_GetClientPort(s_currSPPConnHdl);
		Btsdk_Disconnect(s_currSPPConnHdl);
		s_currSPPConnHdl = BTSDK_INVALID_HANDLE;
		Btsdk_DeinitCommObj(comNum);
		Btsdk_PlugOutVComm(s_ComSerialNum, COMM_SET_RECORD);
	}
	else
	{
		printf("make sure exist connection now!");
	}
}



/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
Description:
	This function is the entry function for SPP sample.
Arguments:
	void
Return:
	void 
---------------------------------------------------------------------------*/
void TestSPPFunc(void)
{
	BTUINT8 ch = 0;
	BTUINT8 comNum = 0;

	SppShowMenu();	
	while (ch != 'r')
	{
		scanf(" %c", &ch);
		getchar();		
		if (ch == '\n')
		{
			printf(">>");
		}
		else if('r' == ch)
		{
			break;
		}
		else
		{
			SppExecCmd(ch);
			printf("\n");
			SppShowMenu();
		}
	}

	if (BTSDK_INVALID_HANDLE != s_currSPPConnHdl)
	{
		comNum = Btsdk_GetClientPort(s_currSPPConnHdl);
		Btsdk_Disconnect(s_currSPPConnHdl);
		s_currSPPConnHdl = BTSDK_INVALID_HANDLE;
		Btsdk_DeinitCommObj(comNum);
		Btsdk_PlugOutVComm(s_ComSerialNum, COMM_SET_RECORD);
	}
}




















