/*X&S
__________________________________________________________________________

	IMPORTANT CONDITIONS OF USE ... PLEASE READ CAREFULLY.
	++++++++++++++++++++++++++++++++++++++++++++++++++++++

	Copyright © 2004 Elan Digital Systems Limited. (Elan)
	This source code is the propery of Elan Digital Systems Ltd.

	You are free to re-use, modify, adapt and re-distribute this code
	without royalty as long as you agree to the following conditions:

	i) It carries NO WARRANTY and Elan will not offer any support
		for it.

	ii) Elan have no liability for losses arrising from its use, either
		directly or indirectly.

	iii) It is used ONLY with Elan//s range of PCMCIA cards.

__________________________________________________________________________

		Filename: USBpulse100Drvr.cpp
			Type:
	Originator:
	 $Revision: $
		 $Date: $
		$Author: $
		Compiler: Visual C
__________________________________________________________________________

	 Synopsys:
__________________________________________________________________________

	Description: USBpulse100 Driver for Win98, Me, 2K, XP, NT4
__________________________________________________________________________

	 defaults:
__________________________________________________________________________

		remarks: 32-bit version
__________________________________________________________________________

		 $Log: $
		 PS 18/7/2005: Creation
__________________________________________________________________________
X&E*/

#include "stdafx.h"

#define USBpulse100Drvr_API  extern "C" _declspec(dllexport)

//IMPORTANT: make sure the project settings use _stdcall as the default calling method (so that can link with VB)
//Also the VB declare statement will need an Alias of "_FUNCNAME@numbytes"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <windows.h>
#include <winbase.h>

//One or other ONLY...put it into the project build settings though !
//define USBpulse100Drvr_PPC_Build
//define USBpulse100Drvr_W32_Build

#ifdef USBpulse100Drvr_W32_Build
	#define _UNICODE
#else
	invalid build option
#endif

HANDLE hMe;

#define UCHAR unsigned char

#define cCH1 1	//channels for calls to lib are always 1,2,3 etc

#define cMAXCHANNEL (10+cCH1)	//to size arrays and define the largest channel# possible

UCHAR readbyte;
UCHAR commstr[1000];
int triggerchan;
bool driverdemomode;
HANDLE porthandles[cMAXCHANNEL];
int windowsversion;
int numpulsesfound;
char pulseproductnames[cMAXCHANNEL][100];
char pulseserialnumbers[cMAXCHANNEL][100];
int pulseportnumbers[cMAXCHANNEL];
int pulsehwrevs[cMAXCHANNEL];
int status;

//--Register Mapping-----------------------------------------------------------------------------------
#define cSUR1addr 0
#define cSUR2addr 1
#define cSUR3addr 2
#define cDACARaddr 3
#define cDACDRaddr 4
#define cX0Raddr 5
#define cX1Raddr 6
#define cX2Raddr 7
#define cX3Raddr 8
#define cSR1addr 9
#define cMISCCRaddr 10
#define cSPARE11addr 11
#define cSPARE12addr 12
#define cSPARE13addr 12
#define cSPARE14addr 14
#define cREVRaddr 14
#define cY0Raddr 15
#define cY1Raddr 16
#define cY2Raddr 17
#define cY3Raddr 18
#define cZ0Raddr 19
#define cZ1Raddr 20
#define cZ2Raddr 21
#define cZ3Raddr 22

//--------stuff for Silabs
// Declare statements for all the functions in the CP210x DLL
// NOTE: These statements assume that the DLL file is located in
//       the same directory as this project.
//       If you change the location of the DLL, be sure to change the location
//       in the declare statements also.

int (*CP210x_GetNumDevices) 			(int* lpwdNumDevices);
int (*CP210x_GetProductString) 			(int dwDeviceNum, UCHAR* lpvDeviceString, int dwFlags);
int (*CP210x_Open) 						(int dwDevice, int* cyHandle);
int (*CP210x_Close) 					(int cyHandle);
int (*CP210x_Reset) 					(int cyHandle);
int (*CP210x_GetDeviceVid) 				(int cyHandle, unsigned short* wVid);
int (*CP210x_GetDevicePid) 				(int cyHandle, unsigned short* wPid);
int (*CP210x_GetDeviceProductString) 	(int cyHandle, UCHAR* lpProduct, UCHAR* bLength, bool bConvertToUnicode);
int (*CP210x_GetDeviceSerialNumber) 	(int cyHandle, UCHAR* lpSerialNumber, UCHAR* bLength, bool bConvertToUnicode);
int (*CP210x_GetSelfPower) 				(int cyHandle, bool* bSelfPower);
int (*CP210x_GetMaxPower) 				(int cyHandle, int* bMaxPower);
int (*CP210x_GetDeviceVersion) 			(int cyHandle, int* wVersion);


//Masks for the serial number and description
#define CP210x_RETURN_SERIAL_NUMBER 0x0
#define CP210x_RETURN_DESCRIPTION 0x1
#define CP210x_RETURN_FULL_PATH 0x2

//Masks for return values from the device
#define CP210x_SUCCESS 0x0
#define CP210x_DEVICE_NOT_FOUND 0xFF
#define CP210x_INVALID_HANDLE 0x1
#define CP210x_INVALID_PARAMETER 0x2
#define CP210x_DEVICE_IO_FAILED 0x3

//Maximum Length of Strings
#define CP210x_MAX_DEVICE_STRLEN 256
#define CP210x_MAX_PRODUCT_STRLEN 126
#define CP210x_MAX_SERIAL_STRLEN 63
#define CP210x_MAX_MAXPOWER 250

#define VERSION_WINXP2000 0x5
#define VERSION_WIN98 0x80000004


HMODULE hCP210xDLL;

//__________________________________________________________________________

USBpulse100Drvr_API int USBpulse100Drvr_PortOpen(int channel)
{
    if (driverdemomode)
        return(1);
    else
    if (porthandles[channel])
        return(1);
	else
		return(0);
}

//__________________________________________________________________________

USBpulse100Drvr_API void USBpulse100Drvr_Close(int channel)
{
    if (porthandles[channel])
	{

		EscapeCommFunction(porthandles[channel],SETBREAK);
		EscapeCommFunction(porthandles[channel],SETBREAK);
		EscapeCommFunction(porthandles[channel],CLRBREAK);
		CloseHandle(porthandles[channel]);
		porthandles[channel] = 0;
    }
}

//__________________________________________________________________________

void AbortNow(int channel)
{
    int chanct;

    for( chanct = 1; chanct < cMAXCHANNEL; chanct++)
	{
		if (porthandles[chanct])
		{
			USBpulse100Drvr_Close(chanct);
		}
    }

	if (channel)
		MessageBox(NULL,"Lost contact with one or more Pulse100s ! Please restart USBpulse100.","USBpulse100 Error",MB_OK);

}

//__________________________________________________________________________

USBpulse100Drvr_API HANDLE USBpulse100Drvr_OpenDrvr(void)
{

	hCP210xDLL=(HMODULE)LoadLibrary("CP210xMan.dll");

	memset((void*)&readbyte,0,sizeof(readbyte));
	memset((void*)commstr,0,sizeof(commstr));
	memset((void*)&triggerchan,0,sizeof(triggerchan));
	memset((void*)&driverdemomode,0,sizeof(driverdemomode));
	memset((void*)porthandles,0,sizeof(porthandles));
	memset((void*)&windowsversion,0,sizeof(windowsversion));
	memset((void*)&numpulsesfound,0,sizeof(numpulsesfound));
	memset((void*)pulseproductnames,0,sizeof(pulseproductnames));
	memset((void*)pulseserialnumbers,0,sizeof(pulseserialnumbers));
	memset((void*)pulseportnumbers,0,sizeof(pulseportnumbers));
	memset((void*)pulsehwrevs,0,sizeof(pulsehwrevs));
	memset((void*)&status,0,sizeof(status));

	if(!hCP210xDLL)
	{
		//clear pointers
		CP210x_GetNumDevices = NULL;
		CP210x_GetProductString = NULL;
		CP210x_Open = NULL;
		CP210x_Close = NULL;
		CP210x_Reset = NULL;
		CP210x_GetDeviceVid = NULL;
		CP210x_GetDevicePid = NULL;
		CP210x_GetDeviceProductString = NULL;
		CP210x_GetDeviceSerialNumber = NULL;
		CP210x_GetSelfPower = NULL;
		CP210x_GetMaxPower = NULL;
		CP210x_GetDeviceVersion = NULL;

		return(NULL);
	}
	else
	{
		//set up pointers to dll function calls

		CP210x_GetNumDevices = (int  (*)(int*))GetProcAddress(hCP210xDLL,TEXT("CP210x_GetNumDevices"));
		CP210x_GetProductString = (int  (*)(int,UCHAR*,int))GetProcAddress(hCP210xDLL,TEXT("CP210x_GetProductString"));
		CP210x_Open = (int  (*)(int,int*))GetProcAddress(hCP210xDLL,TEXT("CP210x_Open"));
		CP210x_Close = (int  (*)(int))GetProcAddress(hCP210xDLL,TEXT("CP210x_Close"));
		CP210x_Reset = (int  (*)(int))GetProcAddress(hCP210xDLL,TEXT("CP210x_Reset"));
		CP210x_GetDeviceVid = (int  (*)(int,unsigned short *))GetProcAddress(hCP210xDLL,TEXT("CP210x_GetDeviceVid"));
		CP210x_GetDevicePid = (int  (*)(int,unsigned short *))GetProcAddress(hCP210xDLL,TEXT("CP210x_GetDevicePid"));
		CP210x_GetDeviceProductString = (int  (*)(int,UCHAR*,UCHAR*,bool))GetProcAddress(hCP210xDLL,TEXT("CP210x_GetDeviceProductString"));
		CP210x_GetDeviceSerialNumber = (int  (*)(int,UCHAR*,UCHAR*,bool))GetProcAddress(hCP210xDLL,TEXT("CP210x_GetDeviceSerialNumber"));
		CP210x_GetSelfPower = (int  (*)(int,bool*))GetProcAddress(hCP210xDLL,TEXT("CP210x_GetSelfPower"));
		CP210x_GetMaxPower = (int  (*)(int,int*))GetProcAddress(hCP210xDLL,TEXT("CP210x_GetMaxPower"));
		CP210x_GetDeviceVersion = (int  (*)(int,int*))GetProcAddress(hCP210xDLL,TEXT("CP210x_GetDeviceVersion"));

		return ((HANDLE)1);
	}
}

//Unload the DLL when you exit your program by calling this function...
//__________________________________________________________________________

USBpulse100Drvr_API void USBpulse100Drvr_CloseDrvr(HANDLE hDrvr)
{
	if(hDrvr)
	{
		//unload library
		FreeLibrary(hCP210xDLL);
		AbortNow(0);
	}
}

//__________________________________________________________________________

USBpulse100Drvr_API void USBpulse100Drvr_SetDemoMode(int State)
{
    driverdemomode = (State != 0);
	memset((void*)porthandles,0,sizeof(porthandles));
}

//__________________________________________________________________________

//---------------------SILABS BELOW HERE
//__________________________________________________________________________

void EnumDevices()
{
    status = CP210x_GetNumDevices(&numpulsesfound);
}

//__________________________________________________________________________

int GetPortNumXP2000(unsigned short  vID, unsigned short  Pid, char* SerialString)
{

        HKEY HKLMKey;
        HKEY EnumKey;
        HKEY VidPidKey;
        HKEY PortKey;
        DWORD Length;
        DWORD Valtype;
        char TmpStr[101];
        int portnum;
        char VidPidString[100];

        Length = 100;

        portnum = -1;

        if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\", 0, KEY_QUERY_VALUE, &HKLMKey))   //cant set to all rights ! Wont work if user priviledge//s too low !
		{
            if (ERROR_SUCCESS == RegOpenKey(HKLMKey, "Enum\\USB\\", &EnumKey))
			{
            	sprintf(VidPidString,"Vid_%04X&Pid_%04X&Mi_00\\%s_00",vID,Pid,SerialString);
                //VidPidString = "Vid_" + LCase(Hex(vID)) + "&Pid_" + LCase(Hex(Pid)) + "&Mi_00\" + Replace(SerialString, " ", "_") + "_00"
                if (ERROR_SUCCESS == RegOpenKey(EnumKey, VidPidString, &VidPidKey))
                {
                    if (ERROR_SUCCESS == RegOpenKey(VidPidKey, "Device Parameters", &PortKey))
					{
                        if (ERROR_SUCCESS == RegQueryValueEx(PortKey, "PortName", 0, &Valtype, (UCHAR*)TmpStr, &Length))
                        {
                            TmpStr[0] = 48;
                            TmpStr[1] = 48;
                            TmpStr[2] = 48;
                            portnum = atoi(TmpStr);
                            //portnum = Val(ConvertToString(TmpStr(), Length - 1))
						}
                        RegCloseKey(PortKey);
					}
                    RegCloseKey(VidPidKey);
				}
                RegCloseKey(EnumKey);
			}
            RegCloseKey(HKLMKey);
		}
        RegCloseKey(HKEY_LOCAL_MACHINE);

        return(portnum);
}

//__________________________________________________________________________

int GetPortNum98(unsigned short  vID, unsigned short  Pid, char* SerialString)
{
    HKEY PortKey;
    DWORD Length;
    DWORD Valtype;
    char TmpStr[101];
    int portnum;
    char VidPidString[100];
    int SerialIndex;
    char SerialKeyString[100];
    int status;

    Length = 100;
    portnum = -1;

    SerialIndex = 0;
    sprintf(SerialKeyString,"Enum\\SLABCR\\guid\\000%d",SerialIndex);
    sprintf(VidPidString,"USB\\VID_%04X&PID_%04X&MI_00\\%s_00",vID,Pid,SerialString);

    while (ERROR_SUCCESS == RegOpenKey(HKEY_LOCAL_MACHINE, SerialKeyString, &PortKey))
    {

        status = RegQueryValueEx(PortKey, "CRLowerDeviceID", 0, &Valtype, (UCHAR*)TmpStr, &Length);
        if (status == ERROR_MORE_DATA)
            SerialIndex = SerialIndex - 1;
        else
        if (status == ERROR_SUCCESS)
        {
            if (strcmp(VidPidString ,TmpStr) == 0)
			{
				if (ERROR_SUCCESS == RegQueryValueEx(PortKey, "PortName", 0, &Valtype, (UCHAR*)TmpStr, &Length))
				{
					TmpStr[0] = 48;
					TmpStr[1] = 48;
					TmpStr[2] = 48;
					portnum = atoi(TmpStr);
				}
			}
            //End If
		}

        SerialIndex = SerialIndex + 1;
    	sprintf(SerialKeyString,"Enum\\SLABCR\\guid\\000%d",SerialIndex);

        RegCloseKey(PortKey);
	}
    RegCloseKey(HKEY_LOCAL_MACHINE);

    return(portnum);
}

//__________________________________________________________________________

int GetDeviceData()
{

    //Variables for the Vid, Pid and Serial Number (SN is a String of chars of length SerialLength)
    unsigned short vID=0;
    unsigned short Pid=0;
    char s[CP210x_MAX_SERIAL_STRLEN];
    UCHAR StringLength;
    int hUSBDevice; //global handle that is set when connected with the CP2101 device
    int i;
    int j=0;

    for (i = 0; i<= numpulsesfound - 1; i++)
    {
        status = CP210x_Open(i, &hUSBDevice);
        if (status == CP210x_SUCCESS)
        {
            status = CP210x_GetDeviceVid(hUSBDevice, &vID);
            if (status == CP210x_SUCCESS)
            {
                status = CP210x_GetDevicePid(hUSBDevice, &Pid);
                if ((status == CP210x_SUCCESS) && (Pid ==  0xF001||0xF002||0xF003||0xF004) && (j < cMAXCHANNEL))  //detect any instruments
                {
					j++;
                    status = CP210x_GetDeviceSerialNumber(hUSBDevice, (UCHAR*)s, &StringLength, true);
                    if (status == CP210x_SUCCESS)
                    {
                        strncpy(pulseserialnumbers[j],s,StringLength);
                        status = CP210x_GetDeviceVersion(hUSBDevice, &pulsehwrevs[j]);
                        if (status == CP210x_SUCCESS)
                        {
                            status = CP210x_GetDeviceProductString(hUSBDevice, (UCHAR*)s, &StringLength, true);
                            if (status == CP210x_SUCCESS)
                            {
                                strncpy(pulseproductnames[j], s,StringLength);
                                CP210x_Close(hUSBDevice);

								int Port;

								Port = -1;

								if ((vID) && (Pid))
								{
									if (windowsversion == VERSION_WINXP2000){
									Port = GetPortNumXP2000(vID, Pid, pulseserialnumbers[j]);}
									else
										if (windowsversion == VERSION_WIN98){
											Port = GetPortNum98(vID, Pid, pulseserialnumbers[j]);}
									//handle case for Vista or later

										else{
											Port = GetPortNumXP2000(vID, Pid, pulseserialnumbers[j]);
										}
								}

								pulseportnumbers[j] = Port;
						   	}
                            else
							{
                                CP210x_Close(hUSBDevice);
							}
						}
						else
                            CP210x_Close(hUSBDevice);
					}
                    else
                        CP210x_Close(hUSBDevice);
				}
                else
                    CP210x_Close(hUSBDevice);
			}
            else
                CP210x_Close (hUSBDevice);
		}

	}
	return j;	//return the number of USBpulse100's
}


//__________________________________________________________________________

USBpulse100Drvr_API int USBpulse100Drvr_Enumerate(int* FcePort,int* FceChannel)
{
    int ct;
    int fp;
    int fc;

//               Win 95  Win 98  Win Me  Win NT 4    Win 2000    Win XP  Win XP SP2  Win 2003 Server
//PlatformID     1       1       1       2           2           2       2           2
//Major Version  4       4       4       4           5           5       5           5
//Minor Version  0       10      90      0           0           1       1           2
//Build          950*    1111    1998    1381        2195        2600    2600        3790

    windowsversion = GetVersion() & 0x8000000F;

	memset((void*)porthandles,0,sizeof(porthandles));

    EnumDevices();	//finds ALL CP210x's

    if (numpulsesfound > cMAXCHANNEL)
		return 0;

    numpulsesfound = GetDeviceData();	//now hunt out the USBpulse100s

    fp = *FcePort;
    fc = *FceChannel;

    if ((fp > 0) && (fc > 0) && (numpulsesfound > 0) && (numpulsesfound <= cMAXCHANNEL))
	{
        *FcePort = -1;    //will use as a test to see if forcing happened
        *FceChannel = -1;
        for (ct = 1; ct <= numpulsesfound;ct++)
        {
            if (pulseportnumbers[ct] == fp)    //find the port requested
            {
                pulseportnumbers[fc] = pulseportnumbers[ct];    //copy over the relevant details
                strcpy(pulseproductnames[fc],pulseproductnames[ct]);    //copy over the relevant details
                strcpy(pulseserialnumbers[fc],pulseserialnumbers[ct]);
                pulsehwrevs[fc] = pulsehwrevs[ct];
                *FcePort = fp;
                *FceChannel = fc;
				return 1;	//but DONT set drvnumpulsesfound to 1 (still need to test that channel requested is in valid range)!
			}
		}
	}

    return(numpulsesfound);
}

//__________________________________________________________________________

USBpulse100Drvr_API int USBpulse100Drvr_GetProductName(int channel, char* s, int* l)
{
    if ((channel > 0) && (channel <= numpulsesfound))
	{
        strcpy(s,pulseproductnames[channel]);
		*l = strlen(pulseproductnames[channel]);
        return(1);
	}
    else
	{
        strcpy(s,"");
		*l=0;
        return(0);
	}
}

//__________________________________________________________________________

USBpulse100Drvr_API int USBpulse100Drvr_GetSerialNumber(int channel, char* s, int* l)
{
    if ((channel > 0) && (channel <= numpulsesfound))
	{
        strcpy(s,pulseserialnumbers[channel]);
		*l = strlen(pulseserialnumbers[channel]);
        return(1);
	}
    else
	{
        s = "";
		*l=0;
        return(0);
	}
}

//__________________________________________________________________________

USBpulse100Drvr_API int USBpulse100Drvr_GetHWRev(int channel, int* l)
{
    if ((channel > 0) && (channel <= numpulsesfound))
	{
        *l = pulsehwrevs[channel];
        return(1);
	}
    else
	{
        *l = -1;
        return(0);
	}
}

//__________________________________________________________________________

USBpulse100Drvr_API int USBpulse100Drvr_GetPortNumber(int channel, int* l)
{
    if ((channel > 0) && (channel <= numpulsesfound))
	{
        *l = pulseportnumbers[channel];
        return(1);
	}
    else
	{
        *l = -1;
        return(0);
	}
}


//__________________________________________________________________________
BOOL APIENTRY DllMain( HANDLE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{

    hMe = hModule;

    switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
		case DLL_PROCESS_DETACH:
			{
				if (lpReserved) break;
			}

    }
    return TRUE;
}

//__________________________________________________________________________

USBpulse100Drvr_API void USBpulse100Drvr_DrvrIssue(wchar_t *libissue)
{
	DWORD lpdwHandle;
	UINT dwBytes;
	BOOL status;
	LPVOID lpData;
	UINT cbTranslate;
	void *lpBuffer;
	wchar_t SubBlock[100],FileName[100],FileVer[100],DLLName[100];

	struct LANGANDCODEPAGE {
	  WORD wLanguage;
	  WORD wCodePage;
	} *lpTranslate;

	swprintf(libissue,L"Problem getting issue");

	GetModuleFileNameW((HINSTANCE)hMe,DLLName,99);	//hModule for a DLL is same as hInstance so just cast it !

	dwBytes = GetFileVersionInfoSizeW(DLLName,&lpdwHandle);	//comes from the VS_VERSION_INFO resource
	lpData = (void *)malloc(dwBytes);
	if (lpData == NULL)
      return;

   status = GetFileVersionInfoW(DLLName,lpdwHandle,dwBytes,lpData);

	if (!status)
      {
   	wcscpy(libissue,DLLName);
   	//swprintf(libissue,L"Problem getting File Ver. Info");
	free(lpData);
      return;
      }

	VerQueryValueW(lpData,
              L"\\VarFileInfo\\Translation",
              (LPVOID*)&lpTranslate,
              &cbTranslate);

	//Now look up the 1st (only) set of strings for the lang and codepage returned
	swprintf( SubBlock,
		L"\\StringFileInfo\\%04x%04x\\FileDescription",
		lpTranslate[0].wLanguage,
		lpTranslate[0].wCodePage);

	// Retrieve file description for language and code page "0".
	VerQueryValueW(	lpData,
					SubBlock,
					&lpBuffer,
					&dwBytes);
	swprintf(FileName,L"%s ",(wchar_t *)lpBuffer);

	swprintf( SubBlock,
		L"\\StringFileInfo\\%04x%04x\\FileVersion",
		lpTranslate[0].wLanguage,
		lpTranslate[0].wCodePage);

	 // Retrieve file ver for language and code page "0".
	VerQueryValueW(	lpData,
					SubBlock,
					&lpBuffer,
					&dwBytes);

	swprintf(FileVer,L"%s",(wchar_t *)lpBuffer);

	swprintf(libissue,L"Name: %s Ver: %s",FileName,FileVer);

	free(lpData);

}

//__________________________________________________________________________

USBpulse100Drvr_API UCHAR USBpulse100Drvr_ReadReg(int channel, int reg)
{
	DWORD got;

    if (porthandles[channel])
	{
        commstr[0] = (UCHAR)(0x60 | reg);
		if (!WriteFile(porthandles[channel],commstr,1,&got,NULL))
		{
            AbortNow(channel);
            return(0);
		}
		if (!ReadFile(porthandles[channel],commstr,1,&got,NULL))
		{
            AbortNow(channel);
            return(0);
		}
        if (got != 1)
		{
            AbortNow(channel);
            return(0);
		}
        else
            return(commstr[0]);
	}
	else
		return(0);
}

//__________________________________________________________________________

USBpulse100Drvr_API void USBpulse100Drvr_ReadRegMulti(int channel, int reg, DWORD N, UCHAR* data)
{
	DWORD got;

    if (porthandles[channel])
	{
        commstr[0] = (UCHAR)(0x40 | reg);
		if (!WriteFile(porthandles[channel],commstr,1,&got,NULL))
		{
            AbortNow(channel);
		}
		if (!ReadFile(porthandles[channel],data,N,&got,NULL))
		{
            AbortNow(channel);
		}
        if (got != N)
		{
            AbortNow(channel);
		}
	}
    else
        memset(data,32,N);   //MUST return something !!!! Else can get run time error 5
}

//__________________________________________________________________________

USBpulse100Drvr_API void USBpulse100Drvr_WriteReg(int channel, int reg, UCHAR data)
{
	DWORD got;

    if (porthandles[channel])
	{
        commstr[0] = (UCHAR)(0x20 | reg);
        commstr[1] = data;

		if (!WriteFile(porthandles[channel],commstr,2,&got,NULL))
		{
            AbortNow(channel);
		}
        if (got != 2)
		{
            AbortNow(channel);
		}
    }
}

//__________________________________________________________________________

USBpulse100Drvr_API void USBpulse100Drvr_WriteRegMulti(int channel, UCHAR* data, DWORD N)
{
	DWORD got;

    if (porthandles[channel])
	{
		if (!WriteFile(porthandles[channel],data,N,&got,NULL))
		{
            AbortNow(channel);
		}
        if (got != N)
		{
            AbortNow(channel);
		}
    }
}

//__________________________________________________________________________

USBpulse100Drvr_API int USBpulse100Drvr_OpenAndReset(int channel)
{
	DCB commDCB;
	COMMTIMEOUTS timeouts;


    if (!driverdemomode)
	{
		sprintf((char*)commstr,"\\\\.\\COM%d",pulseportnumbers[channel]);
		porthandles[channel] = CreateFile((char*)commstr, GENERIC_READ|GENERIC_WRITE, 0,
                       0, OPEN_EXISTING, FILE_FLAG_NO_BUFFERING, 0);
		if (porthandles[channel] == INVALID_HANDLE_VALUE)
		{
			porthandles[channel] = 0;
			return(0);
		}
		else
		{
			GetCommState(porthandles[channel],&commDCB);
			//commDCB.fBinary = true;
			//commDCB.fDtrControl = DTR_CONTROL_ENABLE;
			//commDCB.fRtsControl  = RTS_CONTROL_ENABLE;
			//commDCB.fOutxCtsFlow = false;
			//commDCB.fOutxDsrFlow = false;
			//commDCB.fOutxDsrFlow = false;
			//commDCB.fOutX = false;
			//commDCB.fInX = false;
			commDCB.BaudRate = 921600;
			commDCB.fParity = false;
			commDCB.StopBits = ONESTOPBIT;
			commDCB.ByteSize = 8;

			if (SetCommState(porthandles[channel],&commDCB))
			{
				EscapeCommFunction(porthandles[channel],SETBREAK);
				EscapeCommFunction(porthandles[channel],SETBREAK);
				EscapeCommFunction(porthandles[channel],CLRBREAK);

				timeouts.ReadIntervalTimeout = 50; //ms
				timeouts.ReadTotalTimeoutMultiplier = 0;
				timeouts.ReadTotalTimeoutConstant = 50;	//ms
				timeouts.WriteTotalTimeoutMultiplier = 0;
				timeouts.WriteTotalTimeoutConstant = 0;

				if (!SetCommTimeouts(porthandles[channel], &timeouts))
				{
					return(0);
				}
			}
			else
				status = (int)GetLastError;
		}
    }
	else
		porthandles[channel] = 0;


	return(1);

}

//__________________________________________________________________________

void Delay(int c)
{
	c /= 2;	//takes about 2ms to do a single read over the USB
    if (c <= 0)
        c=1;
	while(c--)
	{
		USBpulse100Drvr_ReadReg(1,cREVRaddr);
	}
}

//__________________________________________________________________________

UCHAR ValidateByte(int c)
{
    if (c < 0)
        return 0;
    else
	if (c > 255)
        return 255;
    else
        return (UCHAR)c;
}

//__________________________________________________________________________

USBpulse100Drvr_API int USBpulse100Drvr_GetControllerRev(int channel, int* r)
{
    if (porthandles[channel])
	{
        *r = USBpulse100Drvr_ReadReg(channel, cREVRaddr);
        return 0;
	}
    else
	{
        *r = 0;
        return -1;
    }
}

//__________________________________________________________________________

USBpulse100Drvr_API int USBpulse100Drvr_GetTrigMaster(void)
{
    if (triggerchan <= 0)
	{
        triggerchan = 1; //dont let it linger at 0 !
	}

    return triggerchan;
}

//__________________________________________________________________________

USBpulse100Drvr_API void USBpulse100Drvr_SetTrigMaster(int channel, int master)
{
    if (porthandles[channel])
	{
        readbyte = USBpulse100Drvr_ReadReg(channel, cSUR1addr);
        if (master != 0)
            readbyte = (UCHAR)(readbyte | 0x40);
        else
            readbyte = (UCHAR)(readbyte & 0xBF);


        USBpulse100Drvr_WriteReg(channel, 0, readbyte);
        triggerchan = channel;
    }
}

//__________________________________________________________________________

USBpulse100Drvr_API void USBpulse100Drvr_SetClockMaster(int channel, int master)
{
    if (porthandles[channel])
	{
        readbyte = USBpulse100Drvr_ReadReg(channel, cSUR1addr);
        if (master != 0)
            readbyte = (UCHAR)(readbyte | 0x80);
        else
            readbyte = (UCHAR)(readbyte & 0x7F);


        USBpulse100Drvr_WriteReg(channel, 0, readbyte);
    }
}

//__________________________________________________________________________

USBpulse100Drvr_API void USBpulse100Drvr_SetDetectLine(int channel, int master, int State)
{
    if (porthandles[channel])
	{
        readbyte = USBpulse100Drvr_ReadReg(channel, cSUR2addr);

        readbyte = (UCHAR)(readbyte & 0x7F);   //clear the state bit for J1_8
        readbyte = (UCHAR)(readbyte | ((UCHAR)(State!=0) * 0x80));   //then set it or not

        if (master != 0)
            readbyte = (UCHAR)(readbyte | 0x10);   //set the oe bit to make the pin drive
        else
            readbyte = (UCHAR)(readbyte & 0xEF);    //clear the oe bit and so float the pin (its has a pull up)

        USBpulse100Drvr_WriteReg(channel, cSUR2addr, readbyte);
    }
}

//__________________________________________________________________________

USBpulse100Drvr_API int USBpulse100Drvr_GetDetectLine(int channel)
{
    if (porthandles[channel])
	{
        readbyte = USBpulse100Drvr_ReadReg(channel, cSR1addr);
        return (readbyte & 0x80) >> 7;   //mask out J1_8
    }
	else
		return 0;
}

//__________________________________________________________________________

USBpulse100Drvr_API int USBpulse100Drvr_GetTriggeredStatus(int channel) //the latched status
{
    if (porthandles[channel])
        return (USBpulse100Drvr_ReadReg(channel, cSR1addr) & 2) >> 1;
    else
		return 0;
}

//__________________________________________________________________________

USBpulse100Drvr_API void USBpulse100Drvr_SetLEDMode(int channel, int mode) //0,1,2,3 as modes
{
	if (porthandles[channel])
	{
        readbyte = USBpulse100Drvr_ReadReg(channel, cMISCCRaddr);
        USBpulse100Drvr_WriteReg (channel, cMISCCRaddr, (UCHAR)((readbyte & 0x9F) | ((mode & 3) * 0x20)));
    }
}

//__________________________________________________________________________

USBpulse100Drvr_API void USBpulse100Drvr_SetXYZ(int channel, int x, int y, int z)
{
	if (porthandles[channel])
	{
		if ((x>0) || (y>0) || (z>0))
		{
			//readbyte = USBpulse100Drvr_ReadReg(channel, cSUR1addr);
			//USBpulse100Drvr_WriteReg(channel, cSUR1addr, (UCHAR)(readbyte & 0xFE));		//stop run
		}
		if (x >= 0)
		{
			USBpulse100Drvr_WriteReg (channel, cX3Raddr, (UCHAR)((x >> 24) & 0xFF));
			USBpulse100Drvr_WriteReg (channel, cX2Raddr, (UCHAR)((x >> 16) & 0xFF));
			USBpulse100Drvr_WriteReg (channel, cX1Raddr, (UCHAR)((x >> 8 ) & 0xFF));
			USBpulse100Drvr_WriteReg (channel, cX0Raddr, (UCHAR)((x >> 0 ) & 0xFF));
		}
		if (y >= 0)
		{
			USBpulse100Drvr_WriteReg (channel, cY3Raddr, (UCHAR)((y >> 24) & 0xFF));
			USBpulse100Drvr_WriteReg (channel, cY2Raddr, (UCHAR)((y >> 16) & 0xFF));
			USBpulse100Drvr_WriteReg (channel, cY1Raddr, (UCHAR)((y >> 8 ) & 0xFF));
			USBpulse100Drvr_WriteReg (channel, cY0Raddr, (UCHAR)((y >> 0 ) & 0xFF));
		}
		if (z >= 0)
		{
			USBpulse100Drvr_WriteReg (channel, cZ3Raddr, (UCHAR)((z >> 24) & 0xFF));
			USBpulse100Drvr_WriteReg (channel, cZ2Raddr, (UCHAR)((z >> 16) & 0xFF));
			USBpulse100Drvr_WriteReg (channel, cZ1Raddr, (UCHAR)((z >> 8 ) & 0xFF));
			USBpulse100Drvr_WriteReg (channel, cZ0Raddr, (UCHAR)((z >> 0 ) & 0xFF));
		}
		if ((x>0) || (y>0) || (z>0))
		{
			//USBpulse100Drvr_WriteReg(channel, cSUR1addr, readbyte);		//start (or not!) run
		}
    }
}

//__________________________________________________________________________

USBpulse100Drvr_API void USBpulse100Drvr_SetPLL(int channel, int m, int n, int u, int dly)
{
	if (porthandles[channel])
	{
		if ((m>0) && (n>0) && (u>0))
		{
			int data;
			UCHAR temp;
			int bit;
			unsigned int pllbits;
			double fout,ratio;
			int OBMUX,FBSEL,FBDLY,XDLYSEL;

			#define cMISCCR_pllsclkHI		(1<<0)
			#define cMISCCR_pllsshiftHI		(1<<1)
			#define cMISCCR_pllsdinHI		(1<<2)
			#define cMISCCR_pllsupdateHI	(1<<3)
			#define cMISCCR_pllmodeHI		(1<<4)

			#define cMISCCR_pllsclkLO		(0<<0)
			#define cMISCCR_pllsshiftLO		(0<<1)
			#define cMISCCR_pllsdinLO		(0<<2)
			#define cMISCCR_pllsupdateLO	(0<<3)
			#define cMISCCR_pllmodeLO		(0<<4)

			//readbyte = USBpulse100Drvr_ReadReg(channel, cSUR1addr);
			//USBpulse100Drvr_WriteReg(channel, cSUR1addr, (UCHAR)(readbyte & 0xFE));		//stop run

			//dont set mode to hi until AFTER all the config bits are shifted in to the dynamic
			//s/r reg, else can select a bunch of rubbish bits and stop the clock to the UART !!!!!

			data = USBpulse100Drvr_ReadReg(channel, cMISCCRaddr);
			data &= 0xE0;	//kill all config bits
			USBpulse100Drvr_WriteReg(channel, cMISCCRaddr, (UCHAR)(data | cMISCCR_pllmodeLO | cMISCCR_pllsshiftLO));		//set dynamic mode
			USBpulse100Drvr_WriteReg(channel, cMISCCRaddr, (UCHAR)(data | cMISCCR_pllmodeLO | cMISCCR_pllsshiftHI));		//ready to shift

			/*  from PAPLUSPLLdynamicAN.pdf app note

			26		XDLYSEL Mask-programmable delay select MUX
			25-22	FBDLY[3:0] Delay line tap select MUX
			21-20	FBSEL[1:0] Feedback source MUX
			19-17	OBMUX[2:0] "B" output MUX
			16-15	OAMUX[1:0] "A" output MUX
			14-13	OADIV[1:0] "A" output divider (/v)
			12-11	OBDIV[1:0] "B" output divider (/u)
			10-5	FBDIV[5:0] Feedback signal divider (/m)
			4-0		FINDIV[4:0] Input clock driver (/n)

			Configuration Bits from log file:
			FINDIV   n
			FBDIV    m
			OBDIV    u
			OADIV    00b
			OAMUX    00b
			OBMUX    100b
			FBSEL    01b
			FBDLY    0000b
			XDLYSEL  1b

			*/

			OBMUX = 2;
			FBSEL = 1;
			FBDLY = 0;
			//if (channel > 1)
				FBDLY = dly;

			XDLYSEL = 1;

			fout = 12.5e6*m/(n*u);

			ratio = 12.5e6 / fout;

			//sniff out sub divisions of the base 12.5MHz input and dont use the VCO at all !
			if ((ratio >= 0.999999999) && (ratio <= 1.000000001))
			{
				OBMUX = 1;
				u = 1;
			}
			if ((ratio >= 1.999999999) && (ratio <= 2.000000001))
			{
				OBMUX = 1;
				u = 2;
			}
			if ((ratio >= 2.999999999) && (ratio <= 3.000000001))
			{
				OBMUX = 1;
				u = 3;
			}
			if ((ratio >= 3.999999999) && (ratio <= 4.000000001))
			{
				OBMUX = 1;
				u = 4;
			}

			n--;
			m--;
			u--;
			pllbits = n | (m << 5) | (u << 11);

			pllbits |= (OBMUX << 17) | (FBSEL << 20) | (FBDLY << 22) | (XDLYSEL << 26);	//these are constants as copied from actgen log file for pll (rest of bits are 0)

			for(bit=0;bit<27;bit++)
			{
				USBpulse100Drvr_WriteReg(channel, cMISCCRaddr, temp = (UCHAR)(data | cMISCCR_pllmodeLO | cMISCCR_pllsshiftHI | ((pllbits & 1) ? cMISCCR_pllsdinHI : cMISCCR_pllsdinLO)));		//data bit
				USBpulse100Drvr_WriteReg(channel, cMISCCRaddr, (UCHAR)(temp | cMISCCR_pllsclkHI));	//clock it
				USBpulse100Drvr_WriteReg(channel, cMISCCRaddr, (UCHAR)(temp | cMISCCR_pllsclkLO));
				pllbits >>= 1;
			}

			USBpulse100Drvr_WriteReg(channel, cMISCCRaddr, (UCHAR)(data | cMISCCR_pllmodeLO | cMISCCR_pllsshiftLO | cMISCCR_pllsupdateLO));	//disable shift
			USBpulse100Drvr_WriteReg(channel, cMISCCRaddr, (UCHAR)(data | cMISCCR_pllmodeLO | cMISCCR_pllsshiftHI | cMISCCR_pllsupdateHI));	//make it update
			USBpulse100Drvr_WriteReg(channel, cMISCCRaddr, (UCHAR)(data | cMISCCR_pllmodeHI | cMISCCR_pllsshiftLO | cMISCCR_pllsupdateLO));	//disable shift & update done, leave in dynamic mode

			//USBpulse100Drvr_WriteReg(channel, cSUR1addr, readbyte);		//start (or not!) run
		}
    }
}

//__________________________________________________________________________

USBpulse100Drvr_API void USBpulse100Drvr_SetAmplitude(int channel, float voltage)
{
    int daccode;

	if (porthandles[channel])
	{
		if (voltage < 1.5)
			voltage = 1.5;
		else
		if (voltage > 5)
			voltage = 5;

        daccode = (int)((voltage-1.5)*255/(5-1.5));

		USBpulse100Drvr_WriteReg (channel, cDACARaddr, 2);
		USBpulse100Drvr_WriteReg (channel, cDACDRaddr, ValidateByte(daccode));
        Delay(12);  //let dac & amp settle (time const is about 12ms)
    }
}

//__________________________________________________________________________

USBpulse100Drvr_API void USBpulse100Drvr_SetRun(int channel, int State) //0=stop 1=run
{
	if (porthandles[channel])
	{
        readbyte = USBpulse100Drvr_ReadReg(channel, cSUR1addr);
		if (State)
			readbyte &= 0xF7;	//un-reset the counter
		else
			readbyte |= 0x08;	//reset the counter

        USBpulse100Drvr_WriteReg (channel, cSUR1addr, (UCHAR)((readbyte & 0xFE) | (1 * (UCHAR)(State!=0))));
    }
}

//__________________________________________________________________________

USBpulse100Drvr_API void USBpulse100Drvr_SetArm(int channel, int State) //0=disarm 1=arm
{
	if (porthandles[channel])
	{
        readbyte = USBpulse100Drvr_ReadReg(channel, cSUR1addr);
        USBpulse100Drvr_WriteReg (channel, cSUR1addr, (UCHAR)((readbyte & 0xFD) | (2 * (UCHAR)(State!=0))));
    }
}

//__________________________________________________________________________

USBpulse100Drvr_API void USBpulse100Drvr_SetBypass(int channel, int State) //0=counter 1=bypass
{
	if (porthandles[channel])
	{
		UCHAR force_counter_reset = 0;
		UCHAR force_counter_notreset = 0xF7;

		readbyte = (UCHAR)(USBpulse100Drvr_ReadReg(channel, cSUR3addr) & 2);	//one shot mode ??
		if (readbyte)	//in one shot mode, need counter to make the gating pulse so dont reset it !
		{
		}
		else
		if (State)	//but otherwise, if just bypass set, can jam counter reset to save power
		{
			//force_counter_reset = 0x08;
		}

        readbyte = USBpulse100Drvr_ReadReg(channel, cSUR1addr);
        USBpulse100Drvr_WriteReg (channel, cSUR1addr, (UCHAR)((readbyte & 0xFB & force_counter_notreset) | (4 * (UCHAR)(State!=0)) | force_counter_reset));
    }
}

//__________________________________________________________________________

USBpulse100Drvr_API void USBpulse100Drvr_SetTrigger(int channel, int State) //0=disable 1=trigger
{
	if (porthandles[channel])
	{
        readbyte = USBpulse100Drvr_ReadReg(channel, cSUR1addr);
        USBpulse100Drvr_WriteReg (channel, cSUR1addr, (UCHAR)((readbyte & 0xDF) | (32 * (UCHAR)(State!=0))));
    }
}

//__________________________________________________________________________

USBpulse100Drvr_API void USBpulse100Drvr_SetInvert(int channel, int State) //0=norm 1=invert
{
	if (porthandles[channel])
	{
        readbyte = USBpulse100Drvr_ReadReg(channel, cSUR2addr);
        USBpulse100Drvr_WriteReg (channel, cSUR2addr, (UCHAR)((readbyte & 0xFE) | (1 * (UCHAR)(State!=0))));
    }
}

//__________________________________________________________________________

USBpulse100Drvr_API void USBpulse100Drvr_SetEnable(int channel, int State) //0=hi-z 1=enable
{
	if (porthandles[channel])
	{
        readbyte = USBpulse100Drvr_ReadReg(channel, cSUR2addr);
        USBpulse100Drvr_WriteReg (channel, cSUR2addr, (UCHAR)((readbyte & 0xFD) | (2 * (UCHAR)(State!=0))));
    }
}

//__________________________________________________________________________

USBpulse100Drvr_API void USBpulse100Drvr_SetPRNG(int channel, int State) //0=off 1=on
{
	if (porthandles[channel])
	{
        readbyte = USBpulse100Drvr_ReadReg(channel, cSUR3addr);
        USBpulse100Drvr_WriteReg (channel, cSUR3addr, (UCHAR)((readbyte & 0xFE) | (1 * (UCHAR)(State!=0))));
    }
}

//__________________________________________________________________________

USBpulse100Drvr_API void USBpulse100Drvr_SetOneshot(int channel, int State) //0=off 1=on
{
	if (porthandles[channel])
	{
        readbyte = USBpulse100Drvr_ReadReg(channel, cSUR3addr);
        USBpulse100Drvr_WriteReg (channel, cSUR3addr, (UCHAR)((readbyte & 0xFD) | (2 * (UCHAR)(State!=0))));
    }
}

//__________________________________________________________________________

USBpulse100Drvr_API void USBpulse100Drvr_InitPulse(int channel, int master)   //1 to 4
{
    USBpulse100Drvr_WriteReg (channel, cSUR1addr, 0);
    USBpulse100Drvr_WriteReg (channel, cSUR2addr, 0);
    USBpulse100Drvr_WriteReg (channel, cSUR3addr, 0);

    //every port that gets opened, strobe the masters J1_6 pin to sync the new device & the existing ones
    USBpulse100Drvr_WriteReg (1, 1, 0x20);
    USBpulse100Drvr_WriteReg (1, 1, 0x24);
    USBpulse100Drvr_WriteReg (1, 1, 0x4);
    USBpulse100Drvr_WriteReg (1, 1, 0x24);

	USBpulse100Drvr_SetClockMaster (channel,master);
    USBpulse100Drvr_SetLEDMode (channel, 1);
}