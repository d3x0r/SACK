

#define USBpulse100Drvr_API  extern "C" _declspec(dllexport)

USBpulse100Drvr_API int USBpulse100Drvr_PortOpen(int channel);
USBpulse100Drvr_API void USBpulse100Drvr_Close(int channel);
USBpulse100Drvr_API HANDLE USBpulse100Drvr_OpenDrvr(void);
USBpulse100Drvr_API void USBpulse100Drvr_CloseDrvr(HANDLE hDrvr);
USBpulse100Drvr_API void USBpulse100Drvr_SetDemoMode(int State);
USBpulse100Drvr_API int USBpulse100Drvr_Enumerate(int* FcePort,int* FceChannel);
USBpulse100Drvr_API int USBpulse100Drvr_GetProductName(int channel, char* s, int* l);
USBpulse100Drvr_API int USBpulse100Drvr_GetSerialNumber(int channel, char* s, int* l);
USBpulse100Drvr_API int USBpulse100Drvr_GetHWRev(int channel, int* l);
USBpulse100Drvr_API int USBpulse100Drvr_GetPortNumber(int channel, int* l);
USBpulse100Drvr_API void USBpulse100Drvr_DrvrIssue(wchar_t *libissue);
USBpulse100Drvr_API UCHAR USBpulse100Drvr_ReadReg(int channel, int reg);
USBpulse100Drvr_API void USBpulse100Drvr_ReadRegMulti(int channel, int reg, DWORD N, UCHAR* data);
USBpulse100Drvr_API void USBpulse100Drvr_WriteReg(int channel, int reg, UCHAR data);
USBpulse100Drvr_API void USBpulse100Drvr_WriteRegMulti(int channel, UCHAR* data, DWORD N);
USBpulse100Drvr_API int USBpulse100Drvr_OpenAndReset(int channel);
USBpulse100Drvr_API int USBpulse100Drvr_GetControllerRev(int channel, int* r);
USBpulse100Drvr_API int USBpulse100Drvr_GetTrigMaster(void);
USBpulse100Drvr_API void USBpulse100Drvr_SetTrigMaster(int channel, int master);
USBpulse100Drvr_API void USBpulse100Drvr_SetClockMaster(int channel, int master);
USBpulse100Drvr_API void USBpulse100Drvr_SetDetectLine(int channel, int master, int State);
USBpulse100Drvr_API int USBpulse100Drvr_GetDetectLine(int channel);
USBpulse100Drvr_API int USBpulse100Drvr_GetTriggeredStatus(int channel); //the latched status
USBpulse100Drvr_API void USBpulse100Drvr_SetLEDMode(int channel, int mode); //0,1,2,3 as modes
USBpulse100Drvr_API void USBpulse100Drvr_SetXYZ(int channel, int x, int y, int z);
USBpulse100Drvr_API void USBpulse100Drvr_SetPLL(int channel, int m, int n, int u, int dly);
USBpulse100Drvr_API void USBpulse100Drvr_SetAmplitude(int channel, float voltage);
USBpulse100Drvr_API void USBpulse100Drvr_SetRun(int channel, int State); //0=stop 1=run
USBpulse100Drvr_API void USBpulse100Drvr_SetArm(int channel, int State); //0=disarm 1=arm
USBpulse100Drvr_API void USBpulse100Drvr_SetBypass(int channel, int State); //0=counter 1=bypass
USBpulse100Drvr_API void USBpulse100Drvr_SetTrigger(int channel, int State); //0=disable 1=trigger
USBpulse100Drvr_API void USBpulse100Drvr_SetInvert(int channel, int State); //0=norm 1=invert
USBpulse100Drvr_API void USBpulse100Drvr_SetEnable(int channel, int State); //0=hi-z 1=enable
USBpulse100Drvr_API void USBpulse100Drvr_SetPRNG(int channel, int State); //0=off 1=on
USBpulse100Drvr_API void USBpulse100Drvr_SetOneshot(int channel, int State); //0=off 1=on
USBpulse100Drvr_API void USBpulse100Drvr_InitPulse(int channel, int master);   //1 to 4
