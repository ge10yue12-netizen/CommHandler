// 下列 ifdef 块是创建使从 DLL 导出更简单的
// 宏的标准方法。此 DLL 中的所有文件都是用命令行上定义的 USB_DAQ_V52_DLL_EXPORTS
// 符号编译的。在使用此 DLL 的
// 任何其他项目上不应定义此符号。这样，源文件中包含此文件的任何其他项目都会将
// USB_DAQ_V52_DLL_API 函数视为是从 DLL 导入的，而此 DLL 则将用此宏定义的
// 符号视为是被导出的。
#ifdef USB_AMC2XE_DLL_EXPORTS
#define USB_AMC2XE_DLL_API extern "C" __declspec(dllexport)
#else
#define USB_AMC2XE_DLL_API extern "C" __declspec(dllimport)
#endif

USB_AMC2XE_DLL_API int __stdcall  OpenUSB_2XE(void);
USB_AMC2XE_DLL_API int __stdcall  CloseUSB_2XE (void);
USB_AMC2XE_DLL_API int __stdcall  Get_Device_Num_2XE(void);
USB_AMC2XE_DLL_API int __stdcall  Reset_Usb_Device_2XE(int dev);
USB_AMC2XE_DLL_API int __stdcall DoSet_2XE(int dev,unsigned char chan,unsigned char state);
USB_AMC2XE_DLL_API int __stdcall DiRead_2XE(int dev,unsigned char *value);
USB_AMC2XE_DLL_API int __stdcall DoRead_2XE(int dev,unsigned char *value);
 
USB_AMC2XE_DLL_API unsigned int __stdcall GetCardId_2XE(int dev,unsigned int* id1,unsigned int* id2,unsigned int* id3);
USB_AMC2XE_DLL_API int __stdcall Read_Position_2XE(int dev,unsigned int Axs,unsigned int* Pos,unsigned char* RunState,unsigned char* IOState,unsigned char* SyncIO); 
 
 USB_AMC2XE_DLL_API int __stdcall SYNC_Start_2XE(int dev);
 USB_AMC2XE_DLL_API int __stdcall SYNC_Stop_2XE(int dev);
 USB_AMC2XE_DLL_API  int __stdcall  Set_Axs_2XE(int dev,unsigned int Axs,unsigned int Run_EN,unsigned int Csta_EN,unsigned int Cstp_EN,unsigned int Csd_EN);
 USB_AMC2XE_DLL_API  int __stdcall  AxsStop_2XE(int dev,unsigned int Axs);
 USB_AMC2XE_DLL_API  int __stdcall  MovToOrg_2XE(int dev,unsigned int Axs,unsigned int Dir,unsigned char Outmod,unsigned int Speed);
 USB_AMC2XE_DLL_API  int __stdcall  FL_ContinueMov_2XE(int dev,unsigned int Axs,unsigned int Dir,unsigned char Outmod,unsigned int Vo,unsigned int Vt);
 
USB_AMC2XE_DLL_API  int __stdcall  FH_ContinueMov_2XE(int dev,unsigned int Axs,unsigned int Dir,unsigned char Outmod,unsigned int Vo,unsigned int Vt);
USB_AMC2XE_DLL_API  int __stdcall  FH_ContinueAdjustSpeed_2XE(int dev,unsigned int Axs,unsigned int Dir,unsigned int Vo,unsigned int Vt);
USB_AMC2XE_DLL_API int __stdcall DeltMov_2XE(int dev,unsigned int Axs,unsigned int curve,unsigned int Dir,unsigned char Outmod,unsigned int Vo,unsigned int Vt,unsigned int Length,unsigned int StartDec,unsigned int Acctime,unsigned int Dectime);
 
 USB_AMC2XE_DLL_API int __stdcall Read_Speed_2XE(int dev,unsigned int Axs,unsigned int* speed);
USB_AMC2XE_DLL_API int __stdcall   AXIS_Interpolation2_2XE(int dev,unsigned int Axs1,unsigned int Axs2,unsigned int curve,unsigned int Dir1,unsigned int Dir2,
									unsigned char Outmod,unsigned int Vo,unsigned int Vt,unsigned int Length1,unsigned int Length2,unsigned int StartDec,
									unsigned int Acctime,unsigned int Dectime);

 USB_AMC2XE_DLL_API  int __stdcall  AxsSyncStop_2XE(int dev,unsigned int Axs1,unsigned int Axs2,unsigned int Axs3,unsigned int Axs4);
 USB_AMC2XE_DLL_API  int __stdcall Set_Encorder_2XE(int dev,int Axs,int mod,int z_reset_en,int z_dir,int set8000,int enable);
 USB_AMC2XE_DLL_API   int __stdcall  Read_Encorder_2XE(int dev,int Axs, unsigned int* Value);