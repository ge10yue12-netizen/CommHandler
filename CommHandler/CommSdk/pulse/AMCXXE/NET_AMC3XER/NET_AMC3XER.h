#ifdef PCI_AMC_3X_EXPORTS
#define MYAPI extern "C"  __declspec(dllexport) 
#else
#define MYAPI extern "C" __declspec(dllimport)
#endif
 
MYAPI unsigned int __stdcall SOCKET_init(void);
MYAPI void __stdcall SOCKET_delete(void);


MYAPI int __stdcall DoSet(char* destIP,unsigned char outport);
MYAPI int __stdcall DoReset(char* destIP,unsigned char outport);
MYAPI int __stdcall DoWriteAll(char* destIP,unsigned char outport);
MYAPI int __stdcall DiRead(char* destIP,unsigned char *io_input);
MYAPI unsigned int __stdcall GetCardId(char* destIP,unsigned int* idnum,unsigned int* idnum1,unsigned int* idnum2);
MYAPI unsigned int __stdcall SetCardNewIP(char *destIP,char *newIP,char *SubnetMask,char *Gateway);
 


 MYAPI int __stdcall Read_Position(char* destIP,unsigned int Axs,unsigned int* Pos,unsigned char* RunState,unsigned char* IOState,unsigned char* SyncIO);
 
  
 MYAPI int __stdcall SYNC_Start(char* destIP);
 MYAPI int __stdcall SYNC_Stop(char* destIP);
 MYAPI  int __stdcall  Set_Axs(char* destIP,unsigned int Axs,unsigned int Run_EN,unsigned int Csta_EN,unsigned int Cstp_EN,unsigned int Csd_EN);
 MYAPI  int __stdcall  AxsStop(char* destIP,unsigned int Axs);
 MYAPI  int __stdcall  MovToOrg(char* destIP,unsigned int Axs,unsigned int Dir,unsigned char Outmod,unsigned int Speed);
 MYAPI  int __stdcall  FL_ContinueMov(char* destIP,unsigned int Axs,unsigned int Dir,unsigned char Outmod,unsigned int Vo,unsigned int Vt,unsigned int WaitSYNC);
 
MYAPI  int __stdcall  FH_ContinueMov(char* destIP,unsigned int Axs,unsigned int Dir,unsigned char Outmod,unsigned int Vo,unsigned int Vt,unsigned int SD_EN,unsigned int WaitSYNC);
MYAPI  int __stdcall  FH_ContinueAdjustSpeed(char* destIP,unsigned int Axs,unsigned int Vo,unsigned int Vt);
MYAPI int __stdcall DeltMov(char* destIP,unsigned int Axs,unsigned int curve,unsigned int Dir,unsigned char Outmod,unsigned int Vo,unsigned int Vt,unsigned int Length,unsigned int StartDec,unsigned int Acctime,unsigned int Dectime,unsigned int SD_EN,unsigned int WaitSYNC);
 MYAPI int __stdcall Read_EndAccP(char* destIP,unsigned int* AccP);
 MYAPI int __stdcall Read_Speed(char* destIP,unsigned int Axs,unsigned int* speed);
MYAPI int __stdcall   AXIS_Interpolation2(char* destIP,unsigned int Axs1,unsigned int Axs2,unsigned int curve,unsigned int Dir1,unsigned int Dir2,
									unsigned char Outmod,unsigned int Vo,unsigned int Vt,unsigned int Length1,unsigned int Length2,unsigned int StartDec,
									unsigned int Acctime,unsigned int Dectime,unsigned int SD_EN,unsigned int WaitSYNC);
 
MYAPI int __stdcall   AXIS_Interpolation3(char* destIP,unsigned int Axs1,unsigned int Axs2,unsigned int Axs3,unsigned int curve,unsigned int Dir1,unsigned int Dir2,unsigned int Dir3,
									unsigned char Outmod,unsigned int Vo,unsigned int Vt,unsigned int Length1,unsigned int Length2,unsigned int Length3,unsigned int StartDec,
									unsigned int Acctime,unsigned int Dectime,unsigned int SD_EN,unsigned int WaitSYNC);
MYAPI int __stdcall   AXIS_Interpolation_Set(char* destIP,unsigned int Axs,unsigned int curve,unsigned int Dir,
				unsigned char Outmod,unsigned int Vo,unsigned int Vt,unsigned int Length,unsigned int LinA,unsigned int StartDec,
				unsigned int Acctime,unsigned int Dectime,unsigned int SD_EN,unsigned int Axs_Main);
MYAPI int __stdcall   AxsSyncStart(char* destIP,unsigned int Axs1,unsigned int Axs2,unsigned int Axs3,unsigned int Axs4);
MYAPI  int __stdcall  AxsSyncStop(char* destIP,unsigned int Axs1,unsigned int Axs2,unsigned int Axs3,unsigned int Axs4); MYAPI  int __stdcall Set_Encorder(char* destIP,int Axs,int mod,int z_reset_en,int z_dir,int set8000,int enable);
MYAPI   int __stdcall  Read_Encorder(char* destIP,int Axs, unsigned int* Value);
MYAPI  int __stdcall    Circular_Busy(char* destIP);
MYAPI int __stdcall    Circular_Reset(char* destIP);
MYAPI int __stdcall     Circular_Interpolation(char* destIP,unsigned int Axs1,unsigned int Axs2,unsigned char Outmod,unsigned int Vt,int xi,int yi,int xe,int ye,int foward,int continu,unsigned int WaitSYNC);

MYAPI int __stdcall ADSingle(char *destIP, int chan, float* adResult);
MYAPI	int __stdcall ADContinue(char *destIP, int chan_first,int chan_last,  int ad_num, int ad_freq,float *databuf);
MYAPI int __stdcall ADContinuConfig(char *destIP, int chan_first, int chan_last, int ad_freq);
MYAPI int __stdcall GetAdBuffSize(void);
MYAPI int __stdcall ReadAdBuff(float* databuf, int num);
MYAPI int __stdcall ADContinuStop(void);
MYAPI int __stdcall  DA_sigle_out(char *destIP, int chan, unsigned short value);
MYAPI int __stdcall  PWM_out(char *destIP, int chan, unsigned int Freq, float duty);