/*============================================================================*/
/*                 ART-Control / Data Acquisition                             */
/*----------------------------------------------------------------------------*/
/*    Copyright (c)  BeiJing ART-Control 2017-2018.  All Rights Reserved.     */
/*----------------------------------------------------------------------------*/
/*                                                                            */
/* Title:       ArtDAQ.h                                                      */
/* Purpose:     Include file for ArtDAQ  library support.                     */
/*                                                                            */
/*============================================================================*/

#ifndef ___ART_DAQ_H___
#define ___ART_DAQ_H___

#ifdef __cplusplus
extern "C" {
#endif


#define ART_API                 __stdcall
#define ART_API_C               __cdecl
#define ART_CALLBACK            __cdecl


#ifndef __ART_INT_8_TYPE_DEFINED__
#define __ART_INT_8_TYPE_DEFINED__
	typedef char                int8;
#endif

#ifndef __ART_INT_16_TYPE_DEFINED__
#define __ART_INT_16_TYPE_DEFINED__
	typedef short               int16;
#endif

#ifndef __ART_INT_32_TYPE_DEFINED__
#define __ART_INT_32_TYPE_DEFINED__
	typedef long                int32;
#endif

#ifndef __ART_INT_64_TYPE_DEFINED__
#define __ART_INT_64_TYPE_DEFINED__
	typedef __int64             int64;
#endif

#ifndef __ART_FLOAT_32_TYPE_DEFINED__
#define __ART_FLOAT_32_TYPE_DEFINED__
	typedef float               float32;
#endif

#ifndef __ART_FLOAT_64_TYPE_DEFINED__
#define __ART_FLOAT_64_TYPE_DEFINED__
	typedef double              float64;
#endif

#ifndef __ART_UNSIGNED_INT_8_TYPE_DEFINED__
#define __ART_UNSIGNED_INT_8_TYPE_DEFINED__
	typedef unsigned char       uInt8;
#endif

#ifndef __ART_UNSIGNED_INT_16_TYPE_DEFINED__
#define __ART_UNSIGNED_INT_16_TYPE_DEFINED__
	typedef unsigned short      uInt16;
#endif

#ifndef __ART_UNSIGNED_INT_32_TYPE_DEFINED__
#define __ART_UNSIGNED_INT_32_TYPE_DEFINED__
	typedef unsigned long       uInt32;
#endif

#ifndef __ART_UNSIGNED_INT_64_TYPE_DEFINED__
#define __ART_UNSIGNED_INT_64_TYPE_DEFINED__
	typedef unsigned __int64    uInt64;
#endif

#ifndef __ART_BOOL_32_TYPE_DEFINED__
#define __ART_BOOL_32_TYPE_DEFINED__
	typedef unsigned long       bool32;
#endif

	///<
#ifndef TRUE
#define TRUE           (1L)
#endif
#ifndef FALSE
#define FALSE          (0L)
#endif

	///<
#ifndef NULL
#define NULL           (0L)
#endif

	typedef void* TaskHandle;


	/******************************************************************************
	*** ArtDAQ Attributes **************************************************************
	******************************************************************************/

	//********** System Attributes **********
#define ArtDAQ_Sys_Tasks                                                    0x1267 // Indicates an array that contains the names of all tasks saved on the system.
#define ArtDAQ_Sys_DevNames                                                 0x193B // Indicates the names of all devices installed in the system.
#define ArtDAQ_Sys_MajorVersion                                             0x1272 // Indicates the major portion of the installed version of ArtDAQ, such as 1 for version 1.5.3.
#define ArtDAQ_Sys_MinorVersion                                             0x1923 // Indicates the minor portion of the installed version of ArtDAQ, such as 5 for version 1.5.3.
#define ArtDAQ_Sys_UpdateVersion                                            0x2F22 // Indicates the update portion of the installed version of ArtDAQ, such as 3 for version 1.5.3.

//********** Task Attributes **********
#define ArtDAQ_Task_Name                                                    0x1276 // Indicates the name of the task.
#define ArtDAQ_Task_Channels                                                0x1273 // Indicates the names of all virtual channels in the task.
#define ArtDAQ_Task_NumChans                                                0x2181 // Indicates the number of virtual channels in the task.
#define ArtDAQ_Task_Devices                                                 0x230E // Indicates an array containing the names of all devices in the task.
#define ArtDAQ_Task_NumDevices                                              0x29BA // Indicates the number of devices in the task.
#define ArtDAQ_Task_Complete                                                0x1274 // Indicates whether the task completed execution.

//********** Device Attributes **********
#define ArtDAQ_Dev_ProductType                                              0x0631 // Indicates the product name of the device.
#define ArtDAQ_Dev_ProductNum                                               0x231D // Indicates the unique hardware identification number for the device.
#define ArtDAQ_Dev_SerialNum                                                0x0632 // Indicates the serial number of the device. This value is zero if the device does not have a serial number.
#define ArtDAQ_Dev_AI_PhysicalChans                                         0x231E // Indicates the number of the analog input physical channels available on the device.
#define ArtDAQ_Dev_AI_SupportedMeasTypes                                    0x2FD2 // Indicates the measurement types supported by the physical channels of the device. Refer to Measurement Types for information on specific channels.
#define ArtDAQ_Dev_AI_MaxSingleChanRate                                     0x298C // Indicates the maximum rate for an analog input task if the task contains only a single channel from this device.
#define ArtDAQ_Dev_AI_MaxMultiChanRate                                      0x298D // Indicates the maximum sampling rate for an analog input task from this device. To find the maximum rate for the task, take the minimum of Maximum Single Channel Rate or the indicated sampling rate of this device divided by the number of channels to acquire data from (including cold-junction compensation and auto zero channels).
#define ArtDAQ_Dev_AI_MinRate                                               0x298E // Indicates the minimum rate for an analog input task on this device. ArtDAQ returns a warning or error if you attempt to sample at a slower rate.
#define ArtDAQ_Dev_AI_SampModes                                             0x2FDC // Indicates sample modes supported by devices that support sample clocked analog input.
#define ArtDAQ_Dev_AI_TrigUsage                                             0x2986 // Indicates the triggers supported by this device for an analog input task.
#define ArtDAQ_Dev_AI_VoltageRngs                                           0x2990 // Indicates pairs of input voltage ranges supported by this device. Each pair consists of the low value, followed by the high value.
#define ArtDAQ_Dev_AO_PhysicalChans                                         0x231F // Indicates the number of the analog output physical channels available on the device.
#define ArtDAQ_Dev_AO_SupportedOutputTypes                                  0x2FD3 // Indicates the generation types supported by the physical channels of the device. Refer to Output Types for information on specific channels.
#define ArtDAQ_Dev_AO_SampModes                                             0x2FDD // Indicates sample modes supported by devices that support sample clocked analog output.
#define ArtDAQ_Dev_AO_MaxRate                                               0x2997 // Indicates the maximum analog output rate of the device.
#define ArtDAQ_Dev_AO_MinRate                                               0x2998 // Indicates the minimum analog output rate of the device.
#define ArtDAQ_Dev_AO_TrigUsage                                             0x2987 // Indicates the triggers supported by this device for analog output tasks.
#define ArtDAQ_Dev_AO_VoltageRngs                                           0x299B // Indicates pairs of output voltage ranges supported by this device. Each pair consists of the low value, followed by the high value.
#define ArtDAQ_Dev_AO_CurrentRngs                                           0x299C // Indicates pairs of output current ranges supported by this device. Each pair consists of the low value, followed by the high value.
#define ArtDAQ_Dev_AO_Gains                                                 0x299D // Indicates the output gain settings supported by this device.
#define ArtDAQ_Dev_DI_Lines                                                 0x2320 // Indicates an array containing the names of the digital input lines available on the device.
#define ArtDAQ_Dev_DI_Ports                                                 0x2321 // Indicates an array containing the names of the digital input ports available on the device.
#define ArtDAQ_Dev_DI_MaxRate                                               0x2999 // Indicates the maximum digital input rate of the device.
#define ArtDAQ_Dev_DI_TrigUsage                                             0x2988 // Indicates the triggers supported by this device for digital input tasks.
#define ArtDAQ_Dev_DO_Lines                                                 0x2322 // Indicates an array containing the names of the digital output lines available on the device.
#define ArtDAQ_Dev_DO_Ports                                                 0x2323 // Indicates an array containing the names of the digital output ports available on the device.
#define ArtDAQ_Dev_DO_MaxRate                                               0x299A // Indicates the maximum digital output rate of the device.
#define ArtDAQ_Dev_DO_TrigUsage                                             0x2989 // Indicates the triggers supported by this device for digital output tasks.
#define ArtDAQ_Dev_CI_PhysicalChans                                         0x2324 // Indicates the number of the counter input physical channels available on the device.
#define ArtDAQ_Dev_CI_SupportedMeasTypes                                    0x2FD4 // Indicates the measurement types supported by the physical channels of the device. Refer to Measurement Types for information on specific channels.
#define ArtDAQ_Dev_CI_TrigUsage                                             0x298A // Indicates the triggers supported by this device for counter input tasks.
#define ArtDAQ_Dev_CI_SampModes                                             0x2FDE // Indicates sample modes supported by devices that support sample clocked counter input.
#define ArtDAQ_Dev_CI_MaxSize                                               0x299F // Indicates in bits the size of the counters on the device.
#define ArtDAQ_Dev_CI_MaxTimebase                                           0x29A0 // Indicates in hertz the maximum counter timebase frequency.
#define ArtDAQ_Dev_CO_PhysicalChans                                         0x2325 // Indicates an array containing the names of the counter output physical channels available on the device.
#define ArtDAQ_Dev_CO_SupportedOutputTypes                                  0x2FD5 // Indicates the generation types supported by the physical channels of the device. Refer to Output Types for information on specific channels.
#define ArtDAQ_Dev_CO_SampModes                                             0x2FDF // Indicates sample modes supported by devices that support sample clocked counter output.
#define ArtDAQ_Dev_CO_TrigUsage                                             0x298B // Indicates the triggers supported by this device for counter output tasks.
#define ArtDAQ_Dev_CO_MaxSize                                               0x29A1 // Indicates in bits the size of the counters on the device.
#define ArtDAQ_Dev_CO_MaxTimebase                                           0x29A2 // Indicates in hertz the maximum counter timebase frequency.

//********** Physical Channel Attributes **********
#define ArtDAQ_PhysicalChan_AI_InputSrcs                                    0x2FD8 // Indicates the list of input sources supported by the channel. Channels may support using the signal from the I/O connector or one of several calibration signals.

//********** Channel Attributes **********
#define ArtDAQ_AI_Max                                                       0x17DD // Specifies the maximum value you expect to measure. This value is in the units you specify with a units property. When you query this property, it returns the coerced maximum value that the device can measure with the current settings.
#define ArtDAQ_AI_Min                                                       0x17DE // Specifies the minimum value you expect to measure. This value is in the units you specify with a units property.  When you query this property, it returns the coerced minimum value that the device can measure with the current settings.
#define ArtDAQ_AI_MeasType                                                  0x0695 // Indicates the measurement to take with the analog input channel and in some cases, such as for temperature measurements, the sensor to use.
#define ArtDAQ_AI_TermCfg                                                   0x1097 // Specifies the terminal configuration for the channel.
#define ArtDAQ_AI_InputSrc                                                  0x2198 // Specifies the source of the channel. You can use the signal from the I/O connector or one of several calibration signals. Certain devices have a single calibration signal bus. For these devices, you must specify the same calibration signal for all channels you connect to a calibration signal.
#define ArtDAQ_AI_Voltage_Units                                             0x1094 // Specifies the units to use to return voltage measurements from the channel.
#define ArtDAQ_AI_Current_Units                                             0x0701 // Specifies the units to use to return current measurements from the channel.
#define ArtDAQ_AI_Strain_Units                                              0x0981 // Specifies the units to use to return strain measurements from the channel.
#define ArtDAQ_AI_Excit_Src                                                 0x17F4 // Specifies the source of excitation.
#define ArtDAQ_AI_Excit_Val                                                 0x17F5 // Specifies the amount of excitation that the sensor requires. If Voltage or Current is  ArtDAQ_Val_Voltage, this value is in volts. If Voltage or Current is  ArtDAQ_Val_Current, this value is in amperes.
#define ArtDAQ_AI_Coupling                                                  0x0064 // Specifies the coupling for the channel.
#define ArtDAQ_AO_Max                                                       0x1186 // Specifies the maximum value you expect to generate. The value is in the units you specify with a units property. If you try to write a value larger than the maximum value, ArtDAQ generates an error. ArtDAQ might coerce this value to a smaller value if other task settings restrict the device from generating the desired maximum.
#define ArtDAQ_AO_Min                                                       0x1187 // Specifies the minimum value you expect to generate. The value is in the units you specify with a units property. If you try to write a value smaller than the minimum value, ArtDAQ generates an error. ArtDAQ might coerce this value to a larger value if other task settings restrict the device from generating the desired minimum.
#define ArtDAQ_AO_OutputType                                                0x1108 // Indicates whether the channel generates voltage,  current, or a waveform.
#define ArtDAQ_AO_Voltage_Units                                             0x1184 // Specifies in what units to generate voltage on the channel. Write data to the channel in the units you select.
#define ArtDAQ_AO_Current_Units                                             0x1109 // Specifies in what units to generate current on the channel. Write data to the channel in the units you select.
#define ArtDAQ_DI_NumLines                                                  0x2178 // Indicates the number of digital lines in the channel.
#define ArtDAQ_DO_NumLines                                                  0x2179 // Indicates the number of digital lines in the channel.
#define ArtDAQ_CI_Max                                                       0x189C // Specifies the maximum value you expect to measure. This value is in the units you specify with a units property. When you query this property, it returns the coerced maximum value that the hardware can measure with the current settings.
#define ArtDAQ_CI_Min                                                       0x189D // Specifies the minimum value you expect to measure. This value is in the units you specify with a units property. When you query this property, it returns the coerced minimum value that the hardware can measure with the current settings.
#define ArtDAQ_CI_MeasType                                                  0x18A0 // Indicates the measurement to take with the channel.
#define ArtDAQ_CI_Freq_Units                                                0x18A1 // Specifies the units to use to return frequency measurements.
#define ArtDAQ_CI_Freq_StartingEdge                                         0x0799 // Specifies between which edges to measure the frequency of the signal.
#define ArtDAQ_CI_Freq_MeasMeth                                             0x0144 // Specifies the method to use to measure the frequency of the signal.
#define ArtDAQ_CI_Freq_MeasTime                                             0x0145 // Specifies in seconds the length of time to measure the frequency of the signal if Method is ArtDAQ_Val_HighFreq2Ctr. Measurement accuracy increases with increased measurement time and with increased signal frequency. If you measure a high-frequency signal for too long, however, the count register could roll over, which results in an incorrect measurement.
#define ArtDAQ_CI_Freq_Div                                                  0x0147 // Specifies the value by which to divide the input signal if  Method is ArtDAQ_Val_LargeRng2Ctr. The larger the divisor, the more accurate the measurement. However, too large a value could cause the count register to roll over, which results in an incorrect measurement.
#define ArtDAQ_CI_Period_Units                                              0x18A3 // Specifies the unit to use to return period measurements.
#define ArtDAQ_CI_Period_StartingEdge                                       0x0852 // Specifies between which edges to measure the period of the signal.
#define ArtDAQ_CI_Period_MeasMeth                                           0x192C // Specifies the method to use to measure the period of the signal.
#define ArtDAQ_CI_Period_MeasTime                                           0x192D // Specifies in seconds the length of time to measure the period of the signal if Method is ArtDAQ_Val_HighFreq2Ctr. Measurement accuracy increases with increased measurement time and with increased signal frequency. If you measure a high-frequency signal for too long, however, the count register could roll over, which results in an incorrect measurement.
#define ArtDAQ_CI_Period_Div                                                0x192E // Specifies the value by which to divide the input signal if Method is ArtDAQ_Val_LargeRng2Ctr. The larger the divisor, the more accurate the measurement. However, too large a value could cause the count register to roll over, which results in an incorrect measurement.
#define ArtDAQ_CI_CountEdges_InitialCnt                                     0x0698 // Specifies the starting value from which to count.
#define ArtDAQ_CI_CountEdges_ActiveEdge                                     0x0697 // Specifies on which edges to increment or decrement the counter.
#define ArtDAQ_CI_CountEdges_Term                                           0x18C7 // Specifies the input terminal of the signal to measure.
#define ArtDAQ_CI_CountEdges_Dir                                            0x0696 // Specifies whether to increment or decrement the counter on each edge.
#define ArtDAQ_CI_CountEdges_DirTerm                                        0x21E1 // Specifies the source terminal of the digital signal that controls the count direction if Direction is ArtDAQ_Val_ExtControlled.
#define ArtDAQ_CI_CountEdges_CountReset_Enable                              0x2FAF // Specifies whether to reset the count on the active edge specified with Terminal.
#define ArtDAQ_CI_CountEdges_CountReset_ResetCount                          0x2FB0 // Specifies the value to reset the count to.
#define ArtDAQ_CI_CountEdges_CountReset_Term                                0x2FB1 // Specifies the input terminal of the signal to reset the count.
#define ArtDAQ_CI_CountEdges_CountReset_DigFltr_MinPulseWidth               0x2FB4 // Specifies the minimum pulse width the filter recognizes.
#define ArtDAQ_CI_CountEdges_CountReset_ActiveEdge                          0x2FB2 // Specifies on which edge of the signal to reset the count.
#define ArtDAQ_CI_PulseWidth_Units                                          0x0823 // Specifies the units to use to return pulse width measurements.
#define ArtDAQ_CI_PulseWidth_Term                                           0x18AA // Specifies the input terminal of the signal to measure.
#define ArtDAQ_CI_PulseWidth_StartingEdge                                   0x0825 // Specifies on which edge of the input signal to begin each pulse width measurement.
#define ArtDAQ_CI_DutyCycle_Term                                            0x308D // Specifies the input terminal of the signal to measure.
#define ArtDAQ_CI_DutyCycle_StartingEdge                                    0x3092 // Specifies which edge of the input signal to begin the duty cycle measurement.
#define ArtDAQ_CI_SemiPeriod_Units                                          0x18AF // Specifies the units to use to return semi-period measurements.
#define ArtDAQ_CI_SemiPeriod_Term                                           0x18B0 // Specifies the input terminal of the signal to measure.
#define ArtDAQ_CI_SemiPeriod_StartingEdge                                   0x22FE // Specifies on which edge of the input signal to begin semi-period measurement. Semi-period measurements alternate between high time and low time, starting on this edge.
#define ArtDAQ_CI_TwoEdgeSep_Units                                          0x18AC // Specifies the units to use to return two-edge separation measurements from the channel.
#define ArtDAQ_CI_TwoEdgeSep_FirstTerm                                      0x18AD // Specifies the source terminal of the digital signal that starts each measurement.
#define ArtDAQ_CI_TwoEdgeSep_FirstEdge                                      0x0833 // Specifies on which edge of the first signal to start each measurement.
#define ArtDAQ_CI_TwoEdgeSep_SecondTerm                                     0x18AE // Specifies the source terminal of the digital signal that stops each measurement.
#define ArtDAQ_CI_TwoEdgeSep_SecondEdge                                     0x0834 // Specifies on which edge of the second signal to stop each measurement.
#define ArtDAQ_CI_Pulse_Freq_Units                                          0x2F0B // Specifies the units to use to return pulse specifications in terms of frequency.
#define ArtDAQ_CI_Pulse_Freq_Term                                           0x2F04 // Specifies the input terminal of the signal to measure.
#define ArtDAQ_CI_Pulse_Freq_Start_Edge                                     0x2F05 // Specifies on which edge of the input signal to begin pulse measurement.
#define ArtDAQ_CI_Pulse_Time_Units                                          0x2F13 // Specifies the units to use to return pulse specifications in terms of high time and low time.
#define ArtDAQ_CI_Pulse_Time_Term                                           0x2F0C // Specifies the input terminal of the signal to measure.
#define ArtDAQ_CI_Pulse_Time_StartEdge                                      0x2F0D // Specifies on which edge of the input signal to begin pulse measurement.
#define ArtDAQ_CI_Pulse_Ticks_Term                                          0x2F14 // Specifies the input terminal of the signal to measure.
#define ArtDAQ_CI_Pulse_Ticks_StartEdge                                     0x2F15 // Specifies on which edge of the input signal to begin pulse measurement.
#define ArtDAQ_CI_AngEncoder_Units                                          0x18A6 // Specifies the units to use to return angular position measurements from the channel.
#define ArtDAQ_CI_AngEncoder_PulsesPerRev                                   0x0875 // Specifies the number of pulses the encoder generates per revolution. This value is the number of pulses on either signal A or signal B, not the total number of pulses on both signal A and signal B.
#define ArtDAQ_CI_AngEncoder_InitialAngle                                   0x0881 // Specifies the starting angle of the encoder. This value is in the units you specify with Units.
#define ArtDAQ_CI_LinEncoder_Units                                          0x18A9 // Specifies the units to use to return linear encoder measurements from the channel.
#define ArtDAQ_CI_LinEncoder_DistPerPulse                                   0x0911 // Specifies the distance to measure for each pulse the encoder generates on signal A or signal B. This value is in the units you specify with Units.
#define ArtDAQ_CI_LinEncoder_InitialPos                                     0x0915 // Specifies the position of the encoder when the measurement begins. This value is in the units you specify with Units.
#define ArtDAQ_CI_Encoder_DecodingType                                      0x21E6 // Specifies how to count and interpret the pulses the encoder generates on signal A and signal B. ArtDAQ_Val_X1, ArtDAQ_Val_X2, and ArtDAQ_Val_X4 are valid for quadrature encoders only. ArtDAQ_Val_TwoPulseCounting is valid for two-pulse encoders only.
#define ArtDAQ_CI_Encoder_AInputTerm                                        0x219D // Specifies the terminal to which signal A is connected.
#define ArtDAQ_CI_Encoder_BInputTerm                                        0x219E // Specifies the terminal to which signal B is connected.
#define ArtDAQ_CI_Encoder_ZInputTerm                                        0x219F // Specifies the terminal to which signal Z is connected.
#define ArtDAQ_CI_Encoder_ZIndexEnable                                      0x0890 // Specifies whether to use Z indexing for the channel.
#define ArtDAQ_CI_Encoder_ZIndexVal                                         0x0888 // Specifies the value to which to reset the measurement when signal Z is high and signal A and signal B are at the states you specify with Z Index Phase. Specify this value in the units of the measurement.
#define ArtDAQ_CI_Encoder_ZIndexPhase                                       0x0889 // Specifies the states at which signal A and signal B must be while signal Z is high for ArtDAQ to reset the measurement. If signal Z is never high while signal A and signal B are high, for example, you must choose a phase other than ArtDAQ_Val_AHighBHigh.
#define ArtDAQ_CI_CtrTimebaseSrc                                            0x0143 // Specifies the terminal of the timebase to use for the counter.
#define ArtDAQ_CI_CtrTimebaseRate                                           0x18B2 // Specifies in Hertz the frequency of the counter timebase. Specifying the rate of a counter timebase allows you to take measurements in terms of time or frequency rather than in ticks of the timebase. If you use an external timebase and do not specify the rate, you can take measurements only in terms of ticks of the timebase.
#define ArtDAQ_CI_CtrTimebaseActiveEdge                                     0x0142 // Specifies whether a timebase cycle is from rising edge to rising edge or from falling edge to falling edge.
#define ArtDAQ_CO_OutputType                                                0x18B5 // Indicates how to define pulses generated on the channel.
#define ArtDAQ_CO_Pulse_IdleState                                           0x1170 // Specifies the resting state of the output terminal.
#define ArtDAQ_CO_Pulse_Term                                                0x18E1 // Specifies on which terminal to generate pulses.
#define ArtDAQ_CO_Pulse_Time_Units                                          0x18D6 // Specifies the units in which to define high and low pulse time.
#define ArtDAQ_CO_Pulse_HighTime                                            0x18BA // Specifies the amount of time that the pulse is at a high voltage. This value is in the units you specify with Units or when you create the channel.
#define ArtDAQ_CO_Pulse_LowTime                                             0x18BB // Specifies the amount of time that the pulse is at a low voltage. This value is in the units you specify with Units or when you create the channel.
#define ArtDAQ_CO_Pulse_Time_InitialDelay                                   0x18BC // Specifies in seconds the amount of time to wait before generating the first pulse.
#define ArtDAQ_CO_Pulse_DutyCyc                                             0x1176 // Specifies the duty cycle of the pulses. The duty cycle of a signal is the width of the pulse divided by period. ArtDAQ uses this ratio and the pulse frequency to determine the width of the pulses and the delay between pulses.
#define ArtDAQ_CO_Pulse_Freq_Units                                          0x18D5 // Specifies the units in which to define pulse frequency.
#define ArtDAQ_CO_Pulse_Freq                                                0x1178 // Specifies the frequency of the pulses to generate. This value is in the units you specify with Units or when you create the channel.
#define ArtDAQ_CO_Pulse_Freq_InitialDelay                                   0x0299 // Specifies in seconds the amount of time to wait before generating the first pulse.
#define ArtDAQ_CO_Pulse_HighTicks                                           0x1169 // Specifies the number of ticks the pulse is high.
#define ArtDAQ_CO_Pulse_LowTicks                                            0x1171 // Specifies the number of ticks the pulse is low.
#define ArtDAQ_CO_Pulse_Ticks_InitialDelay                                  0x0298 // Specifies the number of ticks to wait before generating the first pulse.
#define ArtDAQ_CO_CtrTimebaseSrc                                            0x0339 // Specifies the terminal of the timebase to use for the counter. Typically, ArtDAQ uses one of the internal counter timebases when generating pulses. Use this property to specify an external timebase and produce custom pulse widths that are not possible using the internal timebases.
#define ArtDAQ_CO_CtrTimebaseRate                                           0x18C2 // Specifies in Hertz the frequency of the counter timebase. Specifying the rate of a counter timebase allows you to define output pulses in seconds rather than in ticks of the timebase. If you use an external timebase and do not specify the rate, you can define output pulses only in ticks of the timebase.
#define ArtDAQ_CO_CtrTimebaseActiveEdge                                     0x0341 // Specifies whether a timebase cycle is from rising edge to rising edge or from falling edge to falling edge.
#define ArtDAQ_CO_Count                                                     0x0293 // Indicates the current value of the count register.
#define ArtDAQ_CO_OutputState                                               0x0294 // Indicates the current state of the output terminal of the counter.
#define ArtDAQ_CO_EnableInitialDelayOnRetrigger                             0x2EC9 // Specifies whether to apply the initial delay to retriggered pulse trains.
#define ArtDAQ_ChanType                                                     0x187F // Indicates the type of the virtual channel.
#define ArtDAQ_PhysicalChanName                                             0x18F5 // Specifies the name of the physical channel upon which this virtual channel is based.
#define ArtDAQ_ChanDescr                                                    0x1926 // Specifies a user-defined description for the channel.
#define ArtDAQ_ChanIsGlobal                                                 0x2304 // Indicates whether the channel is a global channel.

//********** Read Attributes **********
#define ArtDAQ_Read_AutoStart                                               0x1826 // Specifies if an ArtDAQ Read function automatically starts the task  if you did not start the task explicitly by using ArtDAQStartTask(). The default value is TRUE. When  an ArtDAQ Read function starts a finite acquisition task, it also stops the task after reading the last sample.
#define ArtDAQ_Read_OverWrite                                               0x1211 // Specifies whether to overwrite samples in the buffer that you have not yet read.

//********** Timing Attributes **********
#define ArtDAQ_Sample_Mode                                                  0x1300 // Specifies if a task acquires or generates a finite number of samples or if it continuously acquires or generates samples.
#define ArtDAQ_Sample_SampPerChan                                           0x1310 // Specifies the number of samples to acquire or generate for each channel if Sample Mode is ArtDAQ_Val_FiniteSamps. If Sample Mode is ArtDAQ_Val_ContSamps, ArtDAQ uses this value to determine the buffer size.
#define ArtDAQ_SampTimingType                                               0x1347 // Specifies the type of sample timing to use for the task.
#define ArtDAQ_SampClk_Rate                                                 0x1344 // Specifies the sampling rate in samples per channel per second. If you use an external source for the Sample Clock, set this input to the maximum expected rate of that clock.
#define ArtDAQ_SampClk_MaxRate                                              0x22C8 // Indicates the maximum Sample Clock rate supported by the task, based on other timing settings. For output tasks, the maximum Sample Clock rate is the maximum rate of the DAC. For input tasks, ArtDAQ calculates the maximum sampling rate differently for multiplexed devices than simultaneous sampling devices.
#define ArtDAQ_SampClk_Src                                                  0x1852 // Specifies the terminal of the signal to use as the Sample Clock.
#define ArtDAQ_SampClk_ActiveEdge                                           0x1301 // Specifies on which edge of a clock pulse sampling takes place. This property is useful primarily when the signal you use as the Sample Clock is not a periodic clock.
#define ArtDAQ_SampClk_Timebase_Src                                         0x1308 // Specifies the terminal of the signal to use as the Sample Clock Timebase.
#define ArtDAQ_AIConv_Src                                                   0x1502 // Specifies the terminal of the signal to use as the AI Convert Clock.
#define ArtDAQ_RefClk_Src                                                   0x1316 // Specifies the terminal of the signal to use as the Reference Clock.
#define ArtDAQ_SyncPulse_Src                                                0x223D // Specifies the terminal of the signal to use as the synchronization pulse. The synchronization pulse resets the clock dividers and the ADCs/DACs on the device.

//********** Trigger Attributes **********
#define ArtDAQ_StartTrig_Type                                               0x1393 // Specifies the type of trigger to use to start a task.
#define ArtDAQ_StartTrig_Term                                               0x2F1E // Indicates the name of the internal Start Trigger terminal for the task. This property does not return the name of the trigger source terminal.
#define ArtDAQ_DigEdge_StartTrig_Src                                        0x1407 // Specifies the name of a terminal where there is a digital signal to use as the source of the Start Trigger.
#define ArtDAQ_DigEdge_StartTrig_Edge                                       0x1404 // Specifies on which edge of a digital pulse to start acquiring or generating samples.
#define ArtDAQ_AnlgEdge_StartTrig_Src                                       0x1398 // Specifies the name of a virtual channel or terminal where there is an analog signal to use as the source of the Start Trigger.
#define ArtDAQ_AnlgEdge_StartTrig_Slope                                     0x1397 // Specifies on which slope of the trigger signal to start acquiring or generating samples.
#define ArtDAQ_AnlgEdge_StartTrig_Lvl                                       0x1396 // Specifies at what threshold in the units of the measurement or generation to start acquiring or generating samples. Use Slope to specify on which slope to trigger on this threshold.
#define ArtDAQ_AnlgWin_StartTrig_Src                                        0x1400 // Specifies the name of a virtual channel or terminal where there is an analog signal to use as the source of the Start Trigger.
#define ArtDAQ_AnlgWin_StartTrig_When                                       0x1401 // Specifies whether the task starts acquiring or generating samples when the signal enters or leaves the window you specify with Bottom and Top.
#define ArtDAQ_AnlgWin_StartTrig_Top                                        0x1403 // Specifies the upper limit of the window. Specify this value in the units of the measurement or generation.
#define ArtDAQ_AnlgWin_StartTrig_Btm                                        0x1402 // Specifies the lower limit of the window. Specify this value in the units of the measurement or generation.
#define ArtDAQ_StartTrig_Delay                                              0x1856 // Specifies Specifies an amount of time to wait after the Start Trigger is received before acquiring or generating the first sample. This value is in the units you specify with Delay Units.
#define ArtDAQ_StartTrig_DelayUnits                                         0x18C8 // Specifies the units of Delay.
#define ArtDAQ_StartTrig_DigFltr_MinPulseWidth                              0x2224 // Specifies in seconds the minimum pulse width the filter recognizes.
#define ArtDAQ_StartTrig_Retriggerable                                      0x190F // Specifies whether a finite task resets and waits for another Start Trigger after the task completes. When you set this property to TRUE, the device performs a finite acquisition or generation each time the Start Trigger occurs until the task stops. The device ignores a trigger if it is in the process of acquiring or generating signals.
#define ArtDAQ_RefTrig_Type                                                 0x1419 // Specifies the type of trigger to use to mark a reference point for the measurement.
#define ArtDAQ_RefTrig_PretrigSamples                                       0x1445 // Specifies the minimum number of pretrigger samples to acquire from each channel before recognizing the reference trigger. Post-trigger samples per channel are equal to Samples Per Channel minus the number of pretrigger samples per channel.
#define ArtDAQ_RefTrig_Term                                                 0x2F1F // Indicates the name of the internal Reference Trigger terminal for the task. This property does not return the name of the trigger source terminal.
#define ArtDAQ_DigEdge_RefTrig_Src                                          0x1434 // Specifies the name of a terminal where there is a digital signal to use as the source of the Reference Trigger.
#define ArtDAQ_DigEdge_RefTrig_Edge                                         0x1430 // Specifies on what edge of a digital pulse the Reference Trigger occurs.
#define ArtDAQ_AnlgEdge_RefTrig_Src                                         0x1424 // Specifies the name of a virtual channel or terminal where there is an analog signal to use as the source of the Reference Trigger.
#define ArtDAQ_AnlgEdge_RefTrig_Slope                                       0x1423 // Specifies on which slope of the source signal the Reference Trigger occurs.
#define ArtDAQ_AnlgEdge_RefTrig_Lvl                                         0x1422 // Specifies in the units of the measurement the threshold at which the Reference Trigger occurs.  Use Slope to specify on which slope to trigger at this threshold.
#define ArtDAQ_AnlgWin_RefTrig_Src                                          0x1426 // Specifies the name of a virtual channel or terminal where there is an analog signal to use as the source of the Reference Trigger.
#define ArtDAQ_AnlgWin_RefTrig_When                                         0x1427 // Specifies whether the Reference Trigger occurs when the source signal enters the window or when it leaves the window. Use Bottom and Top to specify the window.
#define ArtDAQ_AnlgWin_RefTrig_Top                                          0x1429 // Specifies the upper limit of the window. Specify this value in the units of the measurement.
#define ArtDAQ_AnlgWin_RefTrig_Btm                                          0x1428 // Specifies the lower limit of the window. Specify this value in the units of the measurement.
#define ArtDAQ_RefTrig_AutoTrigEnable                                       0x2EC1 // Specifies whether to send a software trigger to the device when a hardware trigger is no longer active in order to prevent a timeout.
#define ArtDAQ_RefTrig_AutoTriggered                                        0x2EC2 // Indicates whether a completed acquisition was triggered by the auto trigger. If an acquisition has not completed after the task starts, this property returns FALSE. This property is only applicable when Enable  is TRUE.
#define ArtDAQ_RefTrig_Delay                                                0x1483 // Specifies in seconds the time to wait after the device receives the Reference Trigger before switching from pretrigger to posttrigger samples.
#define ArtDAQ_RefTrig_DigFltr_MinPulseWidth                                0x2ED8 // Specifies in seconds the minimum pulse width the filter recognizes.
#define ArtDAQ_PauseTrig_Type                                               0x1366 // Specifies the type of trigger to use to pause a task.
#define ArtDAQ_PauseTrig_Term                                               0x2F20 // Indicates the name of the internal Pause Trigger terminal for the task. This property does not return the name of the trigger source terminal.
#define ArtDAQ_AnlgLvl_PauseTrig_Src                                        0x1370 // Specifies the name of a virtual channel or terminal where there is an analog signal to use as the source of the trigger.
#define ArtDAQ_AnlgLvl_PauseTrig_When                                       0x1371 // Specifies whether the task pauses above or below the threshold you specify with Level.
#define ArtDAQ_AnlgLvl_PauseTrig_Lvl                                        0x1369 // Specifies the threshold at which to pause the task. Specify this value in the units of the measurement or generation. Use Pause When to specify whether the task pauses above or below this threshold.
#define ArtDAQ_AnlgWin_PauseTrig_Src                                        0x1373 // Specifies the name of a virtual channel or terminal where there is an analog signal to use as the source of the trigger.
#define ArtDAQ_AnlgWin_PauseTrig_When                                       0x1374 // Specifies whether the task pauses while the trigger signal is inside or outside the window you specify with Bottom and Top.
#define ArtDAQ_AnlgWin_PauseTrig_Top                                        0x1376 // Specifies the upper limit of the window. Specify this value in the units of the measurement or generation.
#define ArtDAQ_AnlgWin_PauseTrig_Btm                                        0x1375 // Specifies the lower limit of the window. Specify this value in the units of the measurement or generation.
#define ArtDAQ_DigLvl_PauseTrig_Src                                         0x1379 // Specifies the name of a terminal where there is a digital signal to use as the source of the Pause Trigger.
#define ArtDAQ_DigLvl_PauseTrig_When                                        0x1380 // Specifies whether the task pauses while the signal is high or low.
#define ArtDAQ_PauseTrig_DigFltr_MinPulseWidth                              0x2229 // Specifies in seconds the minimum pulse width the filter recognizes.

//********** Export Signal Attributes **********
#define ArtDAQ_Exported_AIConvClk_OutputTerm                                0x1687 // Specifies the terminal to which to route the AI Convert Clock.
#define ArtDAQ_Exported_SampClk_OutputTerm                                  0x1663 // Specifies the terminal to which to route the Sample Clock.
#define ArtDAQ_Exported_PauseTrig_OutputTerm                                0x1615 // Specifies the terminal to which to route the Pause Trigger.
#define ArtDAQ_Exported_RefTrig_OutputTerm                                  0x0590 // Specifies the terminal to which to route the Reference Trigger.
#define ArtDAQ_Exported_StartTrig_OutputTerm                                0x0584 // Specifies the terminal to which to route the Start Trigger.
#define ArtDAQ_Exported_CtrOutEvent_OutputTerm                              0x1717 // Specifies the terminal to which to route the Counter Output Event.
#define ArtDAQ_Exported_CtrOutEvent_OutputBehavior                          0x174F // Specifies whether the exported Counter Output Event pulses or changes from one state to the other when the counter reaches terminal count.
#define ArtDAQ_Exported_CtrOutEvent_Pulse_Polarity                          0x1718 // Specifies the polarity of the pulses at the output terminal of the counter when Output Behavior is ArtDAQ_Val_Pulse. ArtDAQ ignores this property if Output Behavior is ArtDAQ_Val_Toggle.
#define ArtDAQ_Exported_CtrOutEvent_Toggle_IdleState                        0x186A // Specifies the initial state of the output terminal of the counter when Output Behavior is ArtDAQ_Val_Toggle. The terminal enters this state when ArtDAQ commits the task.
#define ArtDAQ_Exported_SampClkTimebase_OutputTerm                          0x18F9 // Specifies the terminal to which to route the Sample Clock Timebase.
#define ArtDAQ_Exported_SyncPulseEvent_OutputTerm                           0x223C // Specifies the terminal to which to route the Synchronization Pulse Event.

/******************************************************************************
*** ArtDAQ Values ************************************************************
******************************************************************************/

#define ArtDAQ_Val_Cfg_Default                                              -1 // Default

//*** Values for the Mode parameter of ArtDAQ_TaskControl ***
#define ArtDAQ_Val_Task_Start                                               0   // Start
#define ArtDAQ_Val_Task_Stop                                                1   // Stop
#define ArtDAQ_Val_Task_Verify                                              2   // Verify
#define ArtDAQ_Val_Task_Commit                                              3   // Commit
#define ArtDAQ_Val_Task_Reserve                                             4   // Reserve
#define ArtDAQ_Val_Task_Unreserve                                           5   // Unreserve
#define ArtDAQ_Val_Task_Abort                                               6   // Abort

//*** Values for ArtDAQ_AI_MeasType ***
//*** Values for ArtDAQ_Dev_AI_SupportedMeasTypes ***
//*** Values for ArtDAQ_PhysicalChan_AI_SupportedMeasTypes ***
//*** Value set AIMeasurementType ***
#define ArtDAQ_Val_Voltage                                                  10322 // Voltage
#define ArtDAQ_Val_Current                                                  10134 // Current
#define ArtDAQ_Val_Resistance                                               10278 // Resistance
#define ArtDAQ_Val_Strain_Gage                                              10300 // Strain Gage
#define ArtDAQ_Val_Voltage_IEPESensor                                       15966 // IEPE Sensor

//*** Values for ArtDAQ_AO_OutputType ***
//*** Values for ArtDAQ_Dev_AO_SupportedOutputTypes ***
//*** Values for ArtDAQ_PhysicalChan_AO_SupportedOutputTypes ***
//*** Value set AOOutputChannelType ***
#define ArtDAQ_Val_Voltage                                                  10322 // Voltage
#define ArtDAQ_Val_Current                                                  10134 // Current

//*** Values for ArtDAQ_CI_MeasType ***
//*** Values for ArtDAQ_Dev_CI_SupportedMeasTypes ***
//*** Values for ArtDAQ_PhysicalChan_CI_SupportedMeasTypes ***
//*** Value set CIMeasurementType ***
#define ArtDAQ_Val_Freq                                                     10179 // Frequency
#define ArtDAQ_Val_Period                                                   10256 // Period
#define ArtDAQ_Val_CountEdges                                               10125 // Count Edges
#define ArtDAQ_Val_PulseWidth                                               10359 // Pulse Width
#define ArtDAQ_Val_SemiPeriod                                               10289 // Semi Period
#define ArtDAQ_Val_PulseFrequency                                           15864 // Pulse Frequency
#define ArtDAQ_Val_PulseTime                                                15865 // Pulse Time
#define ArtDAQ_Val_PulseTicks                                               15866 // Pulse Ticks
#define ArtDAQ_Val_DutyCycle                                                16070 // Duty Cycle
#define ArtDAQ_Val_Position_AngEncoder                                      10360 // Position:Angular Encoder
#define ArtDAQ_Val_Position_LinEncoder                                      10361 // Position:Linear Encoder
#define ArtDAQ_Val_TwoEdgeSep                                               10267 // Two Edge Separation

//*** Values for ArtDAQ_CO_OutputType ***
//*** Values for ArtDAQ_Dev_CO_SupportedOutputTypes ***
//*** Values for ArtDAQ_PhysicalChan_CO_SupportedOutputTypes ***
//*** Value set COOutputType ***
#define ArtDAQ_Val_Pulse_Time                                               10269 // Pulse:Time
#define ArtDAQ_Val_Pulse_Freq                                               10119 // Pulse:Frequency
#define ArtDAQ_Val_Pulse_Ticks                                              10268 // Pulse:Ticks

//*** Values for ArtDAQ_ChanType ***
//*** Value set ChannelType ***
#define ArtDAQ_Val_AI                                                       10100 // Analog Input
#define ArtDAQ_Val_AO                                                       10102 // Analog Output
#define ArtDAQ_Val_DI                                                       10151 // Digital Input
#define ArtDAQ_Val_DO                                                       10153 // Digital Output
#define ArtDAQ_Val_CI                                                       10131 // Counter Input
#define ArtDAQ_Val_CO                                                       10132 // Counter Output

//*** Values for ArtDAQ_AI_TermCfg ***
//*** Value set InputTermCfg ***
#define ArtDAQ_Val_RSE                                                      10083 // RSE
#define ArtDAQ_Val_NRSE                                                     10078 // NRSE
#define ArtDAQ_Val_Diff                                                     10106 // Differential
#define ArtDAQ_Val_PseudoDiff                                               12529 // Pseudodifferential

//*** Values for ArtDAQ_AI_Coupling ***
//*** Value set Coupling1 ***
#define ArtDAQ_Val_AC                                                       10045 // AC
#define ArtDAQ_Val_DC                                                       10050 // DC
#define ArtDAQ_Val_GND                                                      10066 // GND

//*** Values for ArtDAQ_AI_Excit_Src ***
//*** Value set ExcitationSource ***
#define ArtDAQ_Val_Internal                                                 10200 // Internal
#define ArtDAQ_Val_External                                                 10167 // External
#define ArtDAQ_Val_None                                                     10230 // None

//*** Values for ArtDAQ_AI_Voltage_Units ***
//*** Values for ArtDAQ_AO_Voltage_Units ***
//*** Value set VoltageUnits1 ***
#define ArtDAQ_Val_Volts                                                    10348 // Volts

//*** Values for ArtDAQ_AI_Current_Units ***
//*** Values for ArtDAQ_AO_Current_Units ***
//*** Value set CurrentUnits1 ***
#define ArtDAQ_Val_Amps                                                     10342 // Amps

//*** Values for ArtDAQ_CI_Freq_Units ***
//*** Value set FrequencyUnits3 ***
#define ArtDAQ_Val_Hz                                                       10373 // Hz
#define ArtDAQ_Val_Ticks                                                    10304 // Ticks

//*** Values for ArtDAQ_CI_Pulse_Freq_Units ***
//*** Values for ArtDAQ_CO_Pulse_Freq_Units ***
//*** Value set FrequencyUnits2 ***
#define ArtDAQ_Val_Hz                                                       10373 // Hz

//*** Values for ArtDAQ_CI_Period_Units ***
//*** Values for ArtDAQ_CI_PulseWidth_Units ***
//*** Values for ArtDAQ_CI_TwoEdgeSep_Units ***
//*** Values for ArtDAQ_CI_SemiPeriod_Units ***
//*** Value set TimeUnits3 ***
#define ArtDAQ_Val_Seconds                                                  10364 // Seconds
#define ArtDAQ_Val_Ticks                                                    10304 // Ticks

//*** Values for ArtDAQ_CI_Pulse_Time_Units ***
//*** Values for ArtDAQ_CO_Pulse_Time_Units ***
//*** Value set TimeUnits2 ***
#define ArtDAQ_Val_Seconds                                                  10364 // Seconds

//*** Values for ArtDAQ_CI_AngEncoder_Units ***
//*** Value set AngleUnits2 ***
#define ArtDAQ_Val_Degrees                                                  10146 // Degrees
#define ArtDAQ_Val_Radians                                                  10273 // Radians
#define ArtDAQ_Val_Ticks                                                    10304 // Ticks

//*** Values for ArtDAQ_CI_LinEncoder_Units ***
//*** Value set LengthUnits3 ***
#define ArtDAQ_Val_Meters                                                   10219 // Meters
#define ArtDAQ_Val_Inches                                                   10379 // Inches
#define ArtDAQ_Val_Ticks                                                    10304 // Ticks

//*** Values for ArtDAQ_CI_Freq_MeasMeth ***
//*** Values for ArtDAQ_CI_Period_MeasMeth ***
//*** Value set CounterFrequencyMethod ***
#define ArtDAQ_Val_LowFreq1Ctr                                              10105 // Low Frequency with 1 Counter
#define ArtDAQ_Val_HighFreq2Ctr                                             10157 // High Frequency with 2 Counters
#define ArtDAQ_Val_LargeRng2Ctr                                             10205 // Large Range with 2 Counters
//#define ArtDAQ_Val_DynAvg                                                   16065 // Dynamic Averaging

//*** Values for AArtDQ_CI_CountEdges_Dir ***
//*** Value set CountDirection1 ***
#define ArtDAQ_Val_CountUp                                                  10128 // Count Up
#define ArtDAQ_Val_CountDown                                                10124 // Count Down
#define ArtDAQ_Val_ExtControlled                                            10326 // Externally Controlled

//*** Values for ArtDAQ_CI_Encoder_DecodingType ***
//*** Values for ArtDAQ_CI_Velocity_Encoder_DecodingType ***
//*** Value set EncoderType2 ***
#define ArtDAQ_Val_X1                                                       10090 // X1
#define ArtDAQ_Val_X2                                                       10091 // X2
#define ArtDAQ_Val_X4                                                       10092 // X4
#define ArtDAQ_Val_TwoPulseCounting                                         10313 // Two Pulse Counting

//*** Values for ArtDAQ_CI_Encoder_ZIndexPhase ***
//*** Value set EncoderZIndexPhase1 ***
#define ArtDAQ_Val_AHighBHigh                                               10040 // A High B High
#define ArtDAQ_Val_AHighBLow                                                10041 // A High B Low
#define ArtDAQ_Val_ALowBHigh                                                10042 // A Low B High
#define ArtDAQ_Val_ALowBLow                                                 10043 // A Low B Low


//*** Values for ArtDAQ_Exported_CtrOutEvent_OutputBehavior ***
//*** Value set ExportActions2 ***
#define ArtDAQ_Val_Pulse                                                    10265 // Pulse
#define ArtDAQ_Val_Toggle                                                   10307 // Toggle

//*** Values for ArtDAQ_Exported_CtrOutEvent_Pulse_Polarity ***
//*** Value set Polarity2 ***
#define ArtDAQ_Val_ActiveHigh                                               10095 // Active High
#define ArtDAQ_Val_ActiveLow                                                10096 // Active Low

//*** Values for ArtDAQ_CI_Freq_StartingEdge ***
//*** Values for ArtDAQ_CI_Period_StartingEdge ***
//*** Values for ArtDAQ_CI_CountEdges_ActiveEdge ***
//*** Values for ArtDAQ_CI_CountEdges_CountReset_ActiveEdge ***
//*** Values for ArtDAQ_CI_DutyCycle_StartingEdge ***
//*** Values for ArtDAQ_CI_PulseWidth_StartingEdge ***
//*** Values for ArtDAQ_CI_TwoEdgeSep_FirstEdge ***
//*** Values for ArtDAQ_CI_TwoEdgeSep_SecondEdge ***
//*** Values for ArtDAQ_CI_SemiPeriod_StartingEdge ***
//*** Values for ArtDAQ_CI_Pulse_Freq_Start_Edge ***
//*** Values for ArtDAQ_CI_Pulse_Time_StartEdge ***
//*** Values for ArtDAQ_CI_Pulse_Ticks_StartEdge ***
//*** Values for ArtDAQ_SampClk_ActiveEdge ***
//*** Values for ArtDAQ_DigEdge_StartTrig_Edge ***
//*** Values for ArtDAQ_DigEdge_RefTrig_Edge ***
//*** Values for ArtDAQ_AnlgEdge_StartTrig_Slope ***
//*** Values for ArtDAQ_AnlgEdge_RefTrig_Slope ***
//*** Value set Edge1 ***
#define ArtDAQ_Val_Rising                                                   10280 // Rising
#define ArtDAQ_Val_Falling                                                  10171 // Falling

//*** Values for ArtDAQ_CI_CountEdges_GateWhen ***
//*** Values for ArtDAQ_CI_OutputState ***
//*** Values for ArtDAQ_CO_Pulse_IdleState ***
//*** Values for ArtDAQ_CO_OutputState ***
//*** Values for ArtDAQ_DigLvl_PauseTrig_When ***
//*** Values for ArtDAQ_Exported_CtrOutEvent_Toggle_IdleState ***
//*** Value set Level1 ***
#define ArtDAQ_Val_High                                                     10192 // High
#define ArtDAQ_Val_Low                                                      10214 // Low

//*** Value set for the state parameter of ArtDAQ_SetDigitalPowerUpStates ***
#define ArtDAQ_Val_High                                                     10192 // High
#define ArtDAQ_Val_Low                                                      10214 // Low
#define ArtDAQ_Val_Input                                                    10310 // Input

//*** Value set AcquisitionType ***
#define ArtDAQ_Val_FiniteSamps                                              10178 // Finite Samples
#define ArtDAQ_Val_ContSamps                                                10123 // Continuous Samples

//*** Values for the everyNsamplesEventType parameter of ArtDAQ_RegisterEveryNSamplesEvent ***
#define ArtDAQ_Val_Acquired_Into_Buffer                                     1     // Acquired Into Buffer
#define ArtDAQ_Val_Transferred_From_Buffer                                  2     // Transferred From Buffer

//*** Value for the Number of Samples per Channel parameter of ArtDAQ_ReadAnalogF64, ArtDAQ_ReadBinaryI16
#define ArtDAQ_Val_Auto                                                     -1

//*** Values for the Fill Mode parameter of ArtDAQ_Readxxxx ***
#define ArtDAQ_Val_GroupByChannel                                           0   // Group by Channel
#define ArtDAQ_Val_GroupByScanNumber                                        1   // Group by Scan Number

//*** Values for ArtDAQ_Read_OverWrite ***
//*** Value set OverwriteMode1 ***
#define ArtDAQ_Val_OverwriteUnreadSamps                                     10252 // Overwrite Unread Samples
#define ArtDAQ_Val_DoNotOverwriteUnreadSamps                                10159 // Do Not Overwrite Unread Samples

//*** Values for ArtDAQ_Write_RegenMode ***
//*** Value set RegenerationMode1 ***
#define ArtDAQ_Val_AllowRegen                                               10097 // Allow Regeneration
#define ArtDAQ_Val_DoNotAllowRegen                                          10158 // Do Not Allow Regeneration

//*** Values for the Line Grouping parameter of ArtDAQ_CreateDIChan and ArtDAQ_CreateDOChan ***
#define ArtDAQ_Val_ChanPerLine                                              0   // One Channel For Each Line
#define ArtDAQ_Val_ChanForAllLines                                          1   // One Channel For All Lines

//*** Values for ArtDAQ_SampTimingType ***
//*** Value set SampleTimingType ***
#define ArtDAQ_Val_SampOnClk                                                10388 // Sample Clock
#define ArtDAQ_Val_Implicit                                                 10451 // Implicit
#define ArtDAQ_Val_OnDemand                                                 10390 // On Demand

//*** Value set Signal ***
#define ArtDAQ_Val_AIConvertClock                                           12484 // AI Convert Clock
#define ArtDAQ_Val_SampleClock                                              12487 // Sample Clock
#define ArtDAQ_Val_RefClock                                                 12535 // Reference Clock
#define ArtDAQ_Val_PauseTrigger                                             12489 // Pause Trigger
#define ArtDAQ_Val_ReferenceTrigger                                         12490 // Reference Trigger
#define ArtDAQ_Val_StartTrigger                                             12491 // Start Trigger
#define ArtDAQ_Val_CounterOutputEvent                                       12494 // Counter Output Event
#define ArtDAQ_Val_SampClkTimebase                                          12495 // Sample Clock Timebase
#define ArtDAQ_Val_SyncPulseEvent                                           12496 // Sync Pulse Event

//*** Values for ArtDAQ_PauseTrig_Type ***
//*** Value set TriggerType6 ***
#define ArtDAQ_Val_AnlgLvl                                                  10101 // Analog Level
#define ArtDAQ_Val_AnlgWin                                                  10103 // Analog Window
#define ArtDAQ_Val_DigLvl                                                   10152 // Digital Level
#define ArtDAQ_Val_DigPattern                                               10398 // Digital Pattern
#define ArtDAQ_Val_None                                                     10230 // None

//*** Values for ArtDAQ_RefTrig_Type ***
//*** Value set TriggerType8 ***
#define ArtDAQ_Val_AnlgEdge                                                 10099 // Analog Edge
#define ArtDAQ_Val_DigEdge                                                  10150 // Digital Edge
#define ArtDAQ_Val_DigPattern                                               10398 // Digital Pattern
#define ArtDAQ_Val_AnlgWin                                                  10103 // Analog Window
#define ArtDAQ_Val_None                                                     10230 // None

//*** Values for ArtDAQ_StartTrig_Type ***
//*** Value set TriggerType10 ***
#define ArtDAQ_Val_AnlgEdge                                                 10099 // Analog Edge
#define ArtDAQ_Val_DigEdge                                                  10150 // Digital Edge
#define ArtDAQ_Val_DigPattern                                               10398 // Digital Pattern
#define ArtDAQ_Val_AnlgWin                                                  10103 // Analog Window
#define ArtDAQ_Val_None                                                     10230 // None

//*** Values for ArtDAQ_AnlgWin_StartTrig_When ***
//*** Values for ArtDAQ_AnlgWin_RefTrig_When ***
//*** Value set WindowTriggerCondition1 ***
#define ArtDAQ_Val_EnteringWin                                              10163 // Entering Window
#define ArtDAQ_Val_LeavingWin                                               10208 // Leaving Window

//*** Values for ArtDAQ_AnlgLvl_PauseTrig_When ***
//*** Value set ActiveLevel ***
#define ArtDAQ_Val_AboveLvl                                                 10093 // Above Level
#define ArtDAQ_Val_BelowLvl                                                 10107 // Below Level

//*** Values for ArtDAQ_AnlgWin_PauseTrig_When ***
//*** Value set WindowTriggerCondition2 ***
#define ArtDAQ_Val_InsideWin                                                10199 // Inside Window
#define ArtDAQ_Val_OutsideWin                                               10251 // Outside Window

//*** Values for ArtDAQ_StartTrig_DelayUnits ***
//*** Value set DigitalWidthUnits1 ***
#define ArtDAQ_Val_SampClkPeriods                                           10286 // Sample Clock Periods
#define ArtDAQ_Val_Seconds                                                  10364 // Seconds
#define ArtDAQ_Val_Ticks                                                    10304 // Ticks

//*** Values for Read/Write Data Format
#define ArtDAQ_Val_Binary_U32                                               1
#define ArtDAQ_Val_Voltage_F64                                              2
#define ArtDAQ_Val_CounterDutyCycleAndFrequency_F64                         3
#define ArtDAQ_Val_CounterHighAndLowTimes_F64                               4
#define ArtDAQ_Val_CounterHighAndLowTicks_U32                               5

/******************************************************************************
*** ArtDAQ Function Declarations *********************************************
******************************************************************************/

/******************************************************/
/***         Task Configuration/Control             ***/
/******************************************************/

	int32 ART_API     ArtDAQ_LoadTask(const char taskName[], TaskHandle* taskHandle);
	int32 ART_API     ArtDAQ_CreateTask(const char taskName[], TaskHandle* taskHandle);
	int32 ART_API     ArtDAQ_StartTask(TaskHandle taskHandle);
	int32 ART_API     ArtDAQ_StopTask(TaskHandle taskHandle);
	int32 ART_API     ArtDAQ_ClearTask(TaskHandle taskHandle);
	int32 ART_API     ArtDAQ_TestTask(TaskHandle taskHandle);

	int32 ART_API     ArtDAQ_WaitUntilTaskDone(TaskHandle taskHandle, float64 timeToWait);
	int32 ART_API     ArtDAQ_IsTaskDone(TaskHandle taskHandle, bool32* isTaskDone);

	int32 ART_API     ArtDAQ_GetTaskAttribute(TaskHandle taskHandle, int32 attributeType, void* attribute, int32 size);

	typedef int32(ART_CALLBACK* ArtDAQ_EveryNSamplesEventCallbackPtr)(TaskHandle taskHandle, int32 everyNsamplesEventType, uInt32 nSamples, void* callbackData);
	typedef int32(ART_CALLBACK* ArtDAQ_DoneEventCallbackPtr)(TaskHandle taskHandle, int32 status, void* callbackData);
	typedef int32(ART_CALLBACK* ArtDAQ_SignalEventCallbackPtr)(TaskHandle taskHandle, int32 signalID, void* callbackData);

	int32 ART_API     ArtDAQ_RegisterEveryNSamplesEvent(TaskHandle task, int32 everyNsamplesEventType, uInt32 nSamples, uInt32 options, ArtDAQ_EveryNSamplesEventCallbackPtr callbackFunction, void* callbackData);
	int32 ART_API     ArtDAQ_RegisterDoneEvent(TaskHandle task, uInt32 options, ArtDAQ_DoneEventCallbackPtr callbackFunction, void* callbackData);
	int32 ART_API     ArtDAQ_RegisterSignalEvent(TaskHandle task, int32 signalID, uInt32 options, ArtDAQ_SignalEventCallbackPtr callbackFunction, void* callbackData);

	/******************************************************/
	/***        Channel Configuration/Creation          ***/
	/******************************************************/

	int32 ART_API     ArtDAQ_CreateAIVoltageChan(TaskHandle taskHandle, const char physicalChannel[], const char nameToAssignToChannel[], int32 terminalConfig, float64 minVal, float64 maxVal, int32 units, const char customScaleName[]);
	int32 ART_API     ArtDAQ_CreateAIVoltageIEPEChan(TaskHandle taskHandle, const char physicalChannel[], const char nameToAssignToChannel[], int32 terminalConfig, int32 coupling, float64 minVal, float64 maxVal, int32 currentExcitSource, float64 currentExcitVal);

	int32 ART_API     ArtDAQ_CreateAOVoltageChan(TaskHandle taskHandle, const char physicalChannel[], const char nameToAssignToChannel[], float64 minVal, float64 maxVal, int32 units, const char customScaleName[]);
	int32 ART_API     ArtDAQ_CreateAOCurrentChan(TaskHandle taskHandle, const char physicalChannel[], const char nameToAssignToChannel[], float64 minVal, float64 maxVal, int32 units, const char customScaleName[]);

	int32 ART_API     ArtDAQ_CreateDIChan(TaskHandle taskHandle, const char lines[], const char nameToAssignToLines[], int32 lineGrouping);
	int32 ART_API     ArtDAQ_CreateDOChan(TaskHandle taskHandle, const char lines[], const char nameToAssignToLines[], int32 lineGrouping);

	int32 ART_API     ArtDAQ_CreateCIFreqChan(TaskHandle taskHandle, const char counter[], const char nameToAssignToChannel[], float64 minVal, float64 maxVal, int32 units, int32 edge, int32 measMethod, float64 measTime, uInt32 divisor, const char customScaleName[]);
	int32 ART_API     ArtDAQ_CreateCIPeriodChan(TaskHandle taskHandle, const char counter[], const char nameToAssignToChannel[], float64 minVal, float64 maxVal, int32 units, int32 edge, int32 measMethod, float64 measTime, uInt32 divisor, const char customScaleName[]);
	int32 ART_API     ArtDAQ_CreateCICountEdgesChan(TaskHandle taskHandle, const char counter[], const char nameToAssignToChannel[], int32 edge, uInt32 initialCount, int32 countDirection);
	int32 ART_API     ArtDAQ_CreateCIPulseWidthChan(TaskHandle taskHandle, const char counter[], const char nameToAssignToChannel[], float64 minVal, float64 maxVal, int32 units, int32 startingEdge, const char customScaleName[]);
	int32 ART_API     ArtDAQ_CreateCISemiPeriodChan(TaskHandle taskHandle, const char counter[], const char nameToAssignToChannel[], float64 minVal, float64 maxVal, int32 units, const char customScaleName[]);
	int32 ART_API     ArtDAQ_CreateCITwoEdgeSepChan(TaskHandle taskHandle, const char counter[], const char nameToAssignToChannel[], float64 minVal, float64 maxVal, int32 units, int32 firstEdge, int32 secondEdge, const char customScaleName[]);
	int32 ART_API     ArtDAQ_CreateCIPulseChanFreq(TaskHandle taskHandle, const char counter[], const char nameToAssignToChannel[], float64 minVal, float64 maxVal, int32 units);
	int32 ART_API     ArtDAQ_CreateCIPulseChanTime(TaskHandle taskHandle, const char counter[], const char nameToAssignToChannel[], float64 minVal, float64 maxVal, int32 units);
	int32 ART_API     ArtDAQ_CreateCIPulseChanTicks(TaskHandle taskHandle, const char counter[], const char nameToAssignToChannel[], const char sourceTerminal[], float64 minVal, float64 maxVal);
	int32 ART_API     ArtDAQ_CreateCILinEncoderChan(TaskHandle taskHandle, const char counter[], const char nameToAssignToChannel[], int32 decodingType, bool32 ZidxEnable, float64 ZidxVal, int32 ZidxPhase, int32 units, float64 distPerPulse, float64 initialPos, const char customScaleName[]);
	int32 ART_API     ArtDAQ_CreateCIAngEncoderChan(TaskHandle taskHandle, const char counter[], const char nameToAssignToChannel[], int32 decodingType, bool32 ZidxEnable, float64 ZidxVal, int32 ZidxPhase, int32 units, uInt32 pulsesPerRev, float64 initialAngle, const char customScaleName[]);

	int32 ART_API     ArtDAQ_CreateCOPulseChanFreq(TaskHandle taskHandle, const char counter[], const char nameToAssignToChannel[], int32 units, int32 idleState, float64 initialDelay, float64 freq, float64 dutyCycle);
	int32 ART_API     ArtDAQ_CreateCOPulseChanTime(TaskHandle taskHandle, const char counter[], const char nameToAssignToChannel[], int32 units, int32 idleState, float64 initialDelay, float64 lowTime, float64 highTime);
	int32 ART_API     ArtDAQ_CreateCOPulseChanTicks(TaskHandle taskHandle, const char counter[], const char nameToAssignToChannel[], const char sourceTerminal[], int32 idleState, int32 initialDelay, int32 lowTicks, int32 highTicks);


	/******************************************************/
	/***                    Timing                      ***/
	/******************************************************/
	int32 ART_API     ArtDAQ_CfgSampClkTiming(TaskHandle taskHandle, const char source[], float64 rate, int32 activeEdge, int32 sampleMode, int32 sampsPerChan);
	int32 ART_API     ArtDAQ_CfgImplicitTiming(TaskHandle taskHandle, int32 sampleMode, uInt64 sampsPerChan);
	/******************************************************/
	/***                  Triggering                    ***/
	/******************************************************/

	int32 ART_API     ArtDAQ_DisableStartTrig(TaskHandle taskHandle);
	int32 ART_API     ArtDAQ_CfgDigEdgeStartTrig(TaskHandle taskHandle, const char triggerSource[], int32 triggerEdge);
	int32 ART_API     ArtDAQ_CfgAnlgEdgeStartTrig(TaskHandle taskHandle, const char triggerSource[], int32 triggerSlope, float64 triggerLevel);
	int32 ART_API     ArtDAQ_CfgAnlgWindowStartTrig(TaskHandle taskHandle, const char triggerSource[], int32 triggerWhen, float64 windowTop, float64 windowBottom);

	int32 ART_API     ArtDAQ_DisableRefTrig(TaskHandle taskHandle);
	int32 ART_API     ArtDAQ_CfgDigEdgeRefTrig(TaskHandle taskHandle, const char triggerSource[], int32 triggerEdge, uInt32 pretriggerSamples);
	int32 ART_API     ArtDAQ_CfgAnlgEdgeRefTrig(TaskHandle taskHandle, const char triggerSource[], int32 triggerSlope, float64 triggerLevel, uInt32 pretriggerSamples);
	int32 ART_API     ArtDAQ_CfgAnlgWindowRefTrig(TaskHandle taskHandle, const char triggerSource[], int32 triggerWhen, float64 windowTop, float64 windowBottom, uInt32 pretriggerSamples);

	int32 ART_API     ArtDAQ_DisablePauseTrig(TaskHandle taskHandle);
	int32 ART_API     ArtDAQ_CfgDigLvlPauseTrig(TaskHandle taskHandle, const char triggerSource[], int32 triggerWhen);
	int32 ART_API     ArtDAQ_CfgAnlgLvlPauseTrig(TaskHandle taskHandle, const char triggerSource[], int32 triggerWhen, float64 triggerLevel);
	int32 ART_API     ArtDAQ_CfgAnlgWindowPauseTrig(TaskHandle taskHandle, const char triggerSource[], int32 triggerWhen, float64 windowTop, float64 windowBottom);

	int32 ART_API     ArtDAQ_SendSoftwareTrigger(TaskHandle taskHandle, int32 triggerID);
	/******************************************************/
	/***                 Read Data                      ***/
	/******************************************************/

	int32 ART_API     ArtDAQ_ReadAnalogF64(TaskHandle taskHandle, int32 numSampsPerChan, float64 timeout, bool32 fillMode, float64 readArray[], uInt32 arraySizeInSamps, int32* sampsPerChanRead, bool32* reserved);
	int32 ART_API     ArtDAQ_ReadAnalogScalarF64(TaskHandle taskHandle, float64 timeout, float64* value, bool32* reserved);
	int32 ART_API     ArtDAQ_ReadBinaryI16(TaskHandle taskHandle, int32 numSampsPerChan, float64 timeout, bool32 fillMode, int16 readArray[], uInt32 arraySizeInSamps, int32* sampsPerChanRead, bool32* reserved);
	int32 ART_API     ArtDAQ_ReadBinaryU16(TaskHandle taskHandle, int32 numSampsPerChan, float64 timeout, bool32 fillMode, uInt16 readArray[], uInt32 arraySizeInSamps, int32* sampsPerChanRead, bool32* reserved);
	int32 ART_API     ArtDAQ_ReadBinaryI32(TaskHandle taskHandle, int32 numSampsPerChan, float64 timeout, bool32 fillMode, int32 readArray[], uInt32 arraySizeInSamps, int32* sampsPerChanRead, bool32* reserved);
	int32 ART_API     ArtDAQ_ReadBinaryU32(TaskHandle taskHandle, int32 numSampsPerChan, float64 timeout, bool32 fillMode, uInt32 readArray[], uInt32 arraySizeInSamps, int32* sampsPerChanRead, bool32* reserved);

	int32 ART_API     ArtDAQ_ReadDigitalU8(TaskHandle taskHandle, int32 numSampsPerChan, float64 timeout, bool32 fillMode, uInt8 readArray[], uInt32 arraySizeInSamps, int32* sampsPerChanRead, bool32* reserved);
	int32 ART_API     ArtDAQ_ReadDigitalU16(TaskHandle taskHandle, int32 numSampsPerChan, float64 timeout, bool32 fillMode, uInt16 readArray[], uInt32 arraySizeInSamps, int32* sampsPerChanRead, bool32* reserved);
	int32 ART_API     ArtDAQ_ReadDigitalU32(TaskHandle taskHandle, int32 numSampsPerChan, float64 timeout, bool32 fillMode, uInt32 readArray[], uInt32 arraySizeInSamps, int32* sampsPerChanRead, bool32* reserved);
	int32 ART_API     ArtDAQ_ReadDigitalScalarU32(TaskHandle taskHandle, float64 timeout, uInt32* value, bool32* reserved);
	int32 ART_API     ArtDAQ_ReadDigitalLines(TaskHandle taskHandle, int32 numSampsPerChan, float64 timeout, bool32 fillMode, uInt8 readArray[], uInt32 arraySizeInBytes, int32* sampsPerChanRead, int32* numBytesPerSamp, bool32* reserved);
	int32 ART_API     ArtDAQ_ReadDigitalPort(const char deviceName[], int32 portIndex, uInt32* portVal);

	int32 ART_API     ArtDAQ_ReadCounterF64(TaskHandle taskHandle, int32 numSampsPerChan, float64 timeout, float64 readArray[], uInt32 arraySizeInSamps, int32* sampsPerChanRead, bool32* reserved);
	int32 ART_API     ArtDAQ_ReadCounterU32(TaskHandle taskHandle, int32 numSampsPerChan, float64 timeout, uInt32 readArray[], uInt32 arraySizeInSamps, int32* sampsPerChanRead, bool32* reserved);
	int32 ART_API     ArtDAQ_ReadCounterScalarF64(TaskHandle taskHandle, float64 timeout, float64* value, bool32* reserved);
	int32 ART_API     ArtDAQ_ReadCounterScalarU32(TaskHandle taskHandle, float64 timeout, uInt32* value, bool32* reserved);
	int32 ART_API     ArtDAQ_ReadCtrFreq(TaskHandle taskHandle, int32 numSampsPerChan, float64 timeout, float64 readArrayFrequency[], float64 readArrayDutyCycle[], uInt32 arraySizeInSamps, int32* sampsPerChanRead, bool32* reserved);
	int32 ART_API     ArtDAQ_ReadCtrTime(TaskHandle taskHandle, int32 numSampsPerChan, float64 timeout, float64 readArrayHighTime[], float64 readArrayLowTime[], uInt32 arraySizeInSamps, int32* sampsPerChanRead, bool32* reserved);
	int32 ART_API     ArtDAQ_ReadCtrTicks(TaskHandle taskHandle, int32 numSampsPerChan, float64 timeout, uInt32 readArrayHighTicks[], uInt32 readArrayLowTicks[], uInt32 arraySizeInSamps, int32* sampsPerChanRead, bool32* reserved);
	int32 ART_API     ArtDAQ_ReadCtrFreqScalar(TaskHandle taskHandle, float64 timeout, float64* frequency, float64* dutyCycle, bool32* reserved);
	int32 ART_API     ArtDAQ_ReadCtrTimeScalar(TaskHandle taskHandle, float64 timeout, float64* highTime, float64* lowTime, bool32* reserved);
	int32 ART_API     ArtDAQ_ReadCtrTicksScalar(TaskHandle taskHandle, float64 timeout, uInt32* highTicks, uInt32* lowTicks, bool32* reserved);

	/******************************************************/
	/***                 Write Data                     ***/
	/******************************************************/

	int32 ART_API     ArtDAQ_WriteAnalogF64(TaskHandle taskHandle, int32 numSampsPerChan, bool32 autoStart, float64 timeout, bool32 dataLayout, const float64 writeArray[], int32* sampsPerChanWritten, bool32* reserved);
	int32 ART_API     ArtDAQ_WriteAnalogScalarF64(TaskHandle taskHandle, bool32 autoStart, float64 timeout, float64 value, bool32* reserved);
	int32 ART_API     ArtDAQ_WriteBinaryI16(TaskHandle taskHandle, int32 numSampsPerChan, bool32 autoStart, float64 timeout, bool32 dataLayout, const int16 writeArray[], int32* sampsPerChanWritten, bool32* reserved);
	int32 ART_API     ArtDAQ_WriteBinaryU16(TaskHandle taskHandle, int32 numSampsPerChan, bool32 autoStart, float64 timeout, bool32 dataLayout, const uInt16 writeArray[], int32* sampsPerChanWritten, bool32* reserved);

	int32 ART_API     ArtDAQ_WriteDigitalU8(TaskHandle taskHandle, int32 numSampsPerChan, bool32 autoStart, float64 timeout, bool32 dataLayout, const uInt8 writeArray[], int32* sampsPerChanWritten, bool32* reserved);
	int32 ART_API     ArtDAQ_WriteDigitalU16(TaskHandle taskHandle, int32 numSampsPerChan, bool32 autoStart, float64 timeout, bool32 dataLayout, const uInt16 writeArray[], int32* sampsPerChanWritten, bool32* reserved);
	int32 ART_API     ArtDAQ_WriteDigitalU32(TaskHandle taskHandle, int32 numSampsPerChan, bool32 autoStart, float64 timeout, bool32 dataLayout, const uInt32 writeArray[], int32* sampsPerChanWritten, bool32* reserved);
	int32 ART_API     ArtDAQ_WriteDigitalScalarU32(TaskHandle taskHandle, bool32 autoStart, float64 timeout, uInt32 value, bool32* reserved);
	int32 ART_API     ArtDAQ_WriteDigitalLines(TaskHandle taskHandle, int32 numSampsPerChan, bool32 autoStart, float64 timeout, bool32 dataLayout, const uInt8 writeArray[], int32* sampsPerChanWritten, bool32* reserved);

	int32 ART_API     ArtDAQ_WriteCtrFreq(TaskHandle taskHandle, int32 numSampsPerChan, bool32 autoStart, float64 timeout, const float64 frequency[], const float64 dutyCycle[], int32* numSampsPerChanWritten, bool32* reserved);
	int32 ART_API     ArtDAQ_WriteCtrTime(TaskHandle taskHandle, int32 numSampsPerChan, bool32 autoStart, float64 timeout, const float64 highTime[], const float64 lowTime[], int32* numSampsPerChanWritten, bool32* reserved);
	int32 ART_API     ArtDAQ_WriteCtrTicks(TaskHandle taskHandle, int32 numSampsPerChan, bool32 autoStart, float64 timeout, const uInt32 highTicks[], const uInt32 lowTicks[], int32* numSampsPerChanWritten, bool32* reserved);
	int32 ART_API     ArtDAQ_WriteCtrFreqScalar(TaskHandle taskHandle, bool32 autoStart, float64 timeout, float64 frequency, float64 dutyCycle, bool32* reserved);
	int32 ART_API     ArtDAQ_WriteCtrTimeScalar(TaskHandle taskHandle, bool32 autoStart, float64 timeout, float64 highTime, float64 lowTime, bool32* reserved);
	int32 ART_API     ArtDAQ_WriteCtrTicksScalar(TaskHandle taskHandle, bool32 autoStart, float64 timeout, uInt32 highTicks, uInt32 lowTicks, bool32* reserved);
	/******************************************************/
	/***               Events & Signals                 ***/
	/******************************************************/

	// Terminology:  For hardware, "signals" comprise "clocks," "triggers," and (output) "events".
	// Software signals or events are not presently supported.

	// For possible values for parameter signalID see "Value set Signal" in Values section above.
	int32 ART_API     ArtDAQ_ExportSignal(TaskHandle taskHandle, int32 signalID, const char outputTerminal[]);
	int32 ART_API     ArtDAQ_ExportCtrOutEvent(TaskHandle taskHandle, const char outputTerminal[], int32 outputBehavior, int32 pulsePolarity, int32 toggleIdleState);

	/******************************************************/
	/***             Buffer Configurations              ***/
	/******************************************************/

	int32 ART_API     ArtDAQ_CfgInputBuffer(TaskHandle taskHandle, uInt32 numSampsPerChan);
	int32 ART_API     ArtDAQ_CfgOutputBuffer(TaskHandle taskHandle, uInt32 numSampsPerChan);

	/******************************************************/
	/***   Device Configuration                         ***/
	/******************************************************/

	int32 ART_API     ArtDAQ_ResetDevice(const char deviceName[]);
	int32 ART_API     ArtDAQ_SelfTestDevice(const char deviceName[]);
	int32 ART_API     ArtDAQ_GetDeviceAttribute(const char deviceName[], int32 attributeType, void* attribute, TaskHandle taskHandle);

	int32 ART_API     ArtDAQ_SetDigitalPowerUpStates(const char deviceName[], const char channelNames[], int32 state);
	int32 ART_API     ArtDAQ_GetDigitalPowerUpStates(const char deviceName[], const char channelNames[], int32 state[], uInt32 arraySize);
	int32 ART_API     ArtDAQ_Set5VPowerOutputStates(const char deviceName[], bool32 outputEnable);
	int32 ART_API     ArtDAQ_Get5VPowerOutputStates(const char deviceName[], bool32* outputEnable, bool32* reserved);
	int32 ART_API     ArtDAQ_Set5VPowerPowerUpStates(const char deviceName[], bool32 outputEnable);
	int32 ART_API     ArtDAQ_Get5VPowerPowerUpStates(const char deviceName[], bool32* outputEnable);

	/******************************************************/
	/***                 Calibration                    ***/
	/******************************************************/

	int32 ART_API     ArtDAQ_SelfCal(const char deviceName[]);

	/******************************************************/
	/***                 Error Handling                 ***/
	/******************************************************/

	int32 ART_API     ArtDAQ_GetErrorString(int32 errorCode, char errorString[], uInt32 bufferSize);
	int32 ART_API     ArtDAQ_GetExtendedErrorInfo(char errorString[], uInt32 bufferSize);

	/******************************************************************************
	*** ART-DAQ Specific Attribute Get/Set/Reset Function Declarations ***********
	******************************************************************************/

	//********** Timing **********
	//*** Set functions for ArtDAQ_AIConv_Src ***
	int32 ART_API     ArtDAQ_SetAIConvClk(TaskHandle taskHandle, const char source[], int32 activeEdge);

	//*** Set functions for ArtDAQ_SampClk_Timebase_Src ***
	int32 ART_API ArtDAQ_SetSampClkTimebaseSrc(TaskHandle taskHandle, const char* data);

	//*** Set functions for ArtDAQ_Exported_SampClkTimebase_OutputTerm ***
	int32 ART_API ArtDAQ_SetExportedSampClkTimebaseOutputTerm(TaskHandle taskHandle, const char* data);

	//*** Set functions for ArtDAQ_RefClk_Src ***
	int32 ART_API ArtDAQ_SetRefClkSrc(TaskHandle taskHandle, const char* data);

	//*** Set functions for ArtDAQ_SyncPulse_Src ***
	int32 ART_API ArtDAQ_SetSyncPulseSrc(TaskHandle taskHandle, const char* data);

	//*** Set functions for ArtDAQ_Exported_SyncPulseEvent_OutputTerm ***
	int32 ART_API ArtDAQ_SetExportedSyncPulseEventOutputTerm(TaskHandle taskHandle, const char* data);

	//********** Trigger **********
	//*** Set/Get functions for ArtDAQ_StartTrig_Delay ***
	int32 ART_API ArtDAQ_GetStartTrigDelay(TaskHandle taskHandle, float64* data);
	int32 ART_API ArtDAQ_SetStartTrigDelay(TaskHandle taskHandle, float64 data);

	//*** Set/Get functions for ArtDAQ_StartTrig_DelayUnits ***
	// Uses value set DigitalWidthUnits1
	int32 ART_API ArtDAQ_GetStartTrigDelayUnits(TaskHandle taskHandle, int32* data);
	int32 ART_API ArtDAQ_SetStartTrigDelayUnits(TaskHandle taskHandle, int32 data);

	//*** Set/Get functions for ArtDAQ_StartTrig_DigFltr_MinPulseWidth ***
	int32 ART_API ArtDAQ_GetStartTrigDigFltrMinPulseWidth(TaskHandle taskHandle, float64* data);
	int32 ART_API ArtDAQ_SetStartTrigDigFltrMinPulseWidth(TaskHandle taskHandle, float64 data);

	//*** Set/Get functions for ArtDAQ_StartTrig_Retriggerable ***
	int32 ART_API ArtDAQ_GetStartTrigRetriggerable(TaskHandle taskHandle, bool32* data);
	int32 ART_API ArtDAQ_SetStartTrigRetriggerable(TaskHandle taskHandle, bool32 data);

	//*** Set/Get functions for ArtDAQ_RefTrig_DigFltr_MinPulseWidth ***
	int32 ART_API ArtDAQ_GetRefTrigDigFltrMinPulseWidth(TaskHandle taskHandle, float64* data);
	int32 ART_API ArtDAQ_SetRefTrigDigFltrMinPulseWidth(TaskHandle taskHandle, float64 data);

	//*** Set/Get functions for ArtDAQ_PauseTrig_DigFltr_MinPulseWidth ***
	int32 ART_API ArtDAQ_GetPauseTrigDigFltrMinPulseWidth(TaskHandle taskHandle, float64* data);
	int32 ART_API ArtDAQ_SetPauseTrigDigFltrMinPulseWidth(TaskHandle taskHandle, float64 data);

	//********** Read **********
	//*** Set/Get functions for ArtDAQ_Read_OverWrite ***
	// Uses value set OverwriteMode1
	int32 ART_API ArtDAQ_GetReadOverWrite(TaskHandle taskHandle, int32* data);
	int32 ART_API ArtDAQ_SetReadOverWrite(TaskHandle taskHandle, int32 data);

	//*** Set/Get functions for ArtDAQ_Read_AutoStart ***
	int32 ART_API ArtDAQ_GetReadAutoStart(TaskHandle taskHandle, bool32* data);
	int32 ART_API ArtDAQ_SetReadAutoStart(TaskHandle taskHandle, bool32 data);

	//********** Write **********
	//*** Set/Get functions for ArtDAQ_Write_RegenMode ***
	// Uses value set RegenerationMode1
	int32 ART_API ArtDAQ_GetWriteRegenMode(TaskHandle taskHandle, int32* data);
	int32 ART_API ArtDAQ_SetWriteRegenMode(TaskHandle taskHandle, int32 data);

	// AI
	//*** Set/Get functions for ArtDAQ_AI_InputSrc ***
	int32 ART_API ArtDAQ_GetAIInputSrc(TaskHandle taskHandle, const char device[], char* data, uInt32 bufferSize);
	int32 ART_API ArtDAQ_SetAIInputSrc(TaskHandle taskHandle, const char device[], const char* data);

	// DI
	//*** Set/Get functions for ArtDAQ_DI_DigFltr_Enable ***
	//int32 ART_API ArtDAQ_GetDIDigFltrEnable(TaskHandle taskHandle, const char channel[], bool32 *data);
	//int32 ART_API ArtDAQ_SetDIDigFltrEnable(TaskHandle taskHandle, const char channel[], bool32 data);
	//int32 ART_API ArtDAQ_ResetDIDigFltrEnable(TaskHandle taskHandle, const char channel[]);
	//
	////*** Set/Get functions for ArtDAQ_DI_DigFltr_MinPulseWidth ***
	//int32 ART_API ArtDAQ_GetDIDigFltrMinPulseWidth(TaskHandle taskHandle, const char channel[], float64 *data);
	//int32 ART_API ArtDAQ_SetDIDigFltrMinPulseWidth(TaskHandle taskHandle, const char channel[], float64 data);
	//int32 ART_API ArtDAQ_ResetDIDigFltrMinPulseWidth(TaskHandle taskHandle, const char channel[]);
	//
	////*** Set/Get functions for ArtDAQ_DI_DigFltr_TimebaseSrc ***
	//int32 ART_API ArtDAQ_GetDIDigFltrTimebaseSrc(TaskHandle taskHandle, const char channel[], char *data, uInt32 bufferSize);
	//int32 ART_API ArtDAQ_SetDIDigFltrTimebaseSrc(TaskHandle taskHandle, const char channel[], const char *data);
	//int32 ART_API ArtDAQ_ResetDIDigFltrTimebaseSrc(TaskHandle taskHandle, const char channel[]);
	//
	////*** Set/Get functions for ArtDAQ_DI_DigFltr_TimebaseRate ***
	//int32 ART_API ArtDAQ_GetDIDigFltrTimebaseRate(TaskHandle taskHandle, const char channel[], float64 *data);
	//int32 ART_API ArtDAQ_SetDIDigFltrTimebaseRate(TaskHandle taskHandle, const char channel[], float64 data);
	//int32 ART_API ArtDAQ_ResetDIDigFltrTimebaseRate(TaskHandle taskHandle, const char channel[]);

	//********** CTR **********
	//*** Set/Get functions for CI CountEdges CountReset ***
	int32 ART_API ArtDAQ_CfgCICountEdgesCountReset(TaskHandle taskHandle, const char sourceTerminal[], uInt32 resetCount, int32 activeEdge, float64 digFltrMinPulseWidth);
	int32 ART_API ArtDAQ_DisableCICountEdgesCountReset(TaskHandle taskHandle);

	//*** Set/Get functions for ArtDAQ_CO_Pulse_Term ***
	int32 ART_API ArtDAQ_GetCOPulseTerm(TaskHandle taskHandle, const char channel[], char* data, uInt32 bufferSize);
	int32 ART_API ArtDAQ_SetCOPulseTerm(TaskHandle taskHandle, const char channel[], const char* data);
	int32 ART_API ArtDAQ_ResetCOPulseTerm(TaskHandle taskHandle, const char channel[]);

	//*** Set/Get functions for ArtDAQ_CO_Count ***
	int32 ART_API ArtDAQ_GetCOCount(TaskHandle taskHandle, const char channel[], uInt32* data);

	//*** Set/Get functions for ArtDAQ_CO_OutputState ***
	// Uses value set Level1
	int32 ART_API ArtDAQ_GetCOOutputState(TaskHandle taskHandle, const char channel[], int32* data);

	//*** Set/Get functions for ArtDAQ_CO_EnableInitialDelayOnRetrigger ***
	int32 ART_API ArtDAQ_GetCOEnableInitialDelayOnRetrigger(TaskHandle taskHandle, const char channel[], bool32* data);
	int32 ART_API ArtDAQ_SetCOEnableInitialDelayOnRetrigger(TaskHandle taskHandle, const char channel[], bool32 data);

	/******************************************************************************
	*** ArtDAQ Error Codes *******************************************************
	******************************************************************************/

#define ArtDAQSuccess                                   (0)
#define ArtDAQFailed(error)                             ((error)<0)
#define ArtDAQSuccessed(error)                          ((error)==0)

#define ArtDAQError_SerivesCanNotConnect                                                    (-229779)
#define ArtDAQError_SendSerivesData                                                         (-229778)
#define ArtDAQError_RecieveSerivesData                                                      (-229776)
#define ArtDAQError_SerivesResponseError                                                    (-229775)

#define ArtDAQError_PALCommunicationsFault                                                  (-50401)
#define ArtDAQError_PALDeviceInitializationFault                                            (-50303)
#define ArtDAQError_PALDeviceNotSupported                                                   (-50302)
#define ArtDAQError_PALDeviceUnknown                                                        (-50301)
#define ArtDAQError_PALMemoryConfigurationFault                                             (-50350)
#define ArtDAQError_PALResourceReserved                                                     (-50103)
#define ArtDAQError_PALFunctionNotFound                                                     (-50255)

#define ArtDAQError_BufferTooSmallForString                                                 (-200228)
#define ArtDAQError_ReadBufferTooSmall                                                      (-200229)
#define ArtDAQError_NULLPtr                                                                 (-200604)
#define ArtDAQError_DuplicateTask                                                           (-200089)
#define ArtDAQError_InvalidTaskName                                                         (-201340)
#define ArtDAQError_InvalidDeviceName                                                       (-201339)
#define ArtDAQError_DeviceNotExist                                                          (-200220)

#define ArtDAQError_InvalidPhysChanString                                                   (-201237)
#define ArtDAQError_PhysicalChanDoesNotExist                                                (-200170)
#define ArtDAQError_DevAlreadyInTask                                                        (-200481)
#define ArtDAQError_ChanAlreadyInTask                                                       (-200489)
#define ArtDAQError_ChanNotInTask                                                           (-200486)
#define ArtDAQError_DevNotInTask                                                            (-200482)
#define ArtDAQError_InvalidTask                                                             (-200088)
#define ArtDAQError_InvalidChannel                                                          (-200087)
#define ArtDAQError_InvalidSyntaxForPhysicalChannelRange                                    (-200086)
#define ArtDAQError_MultiChanTypesInTask                                                    (-200559)
#define ArtDAQError_MultiDevsInTask                                                         (-200558)
#define ArtDAQError_PhysChanDevNotInTask                                                    (-200648)
#define ArtDAQError_RefAndPauseTrigConfigured                                               (-200628)
#define ArtDAQError_ActivePhysChanTooManyLinesSpecdWhenGettingPrpty                         (-200625)

#define ArtDAQError_ActiveDevNotSupportedWithMultiDevTask                                   (-201207)
#define ArtDAQError_RealDevAndSimDevNotSupportedInSameTask                                  (-201206)
#define ArtDAQError_DevsWithoutSyncStrategies                                               (-201426)
#define ArtDAQError_DevCannotBeAccessed                                                     (-201003)
#define ArtDAQError_SampleRateNumChansConvertPeriodCombo                                    (-200081)
#define ArtDAQError_InvalidAttributeValue                                                   (-200077)
#define ArtDAQError_CanNotPerformOpWhileTaskRunning                                         (-200479)
#define ArtDAQError_CanNotPerformOpWhenNoChansInTask                                        (-200478)
#define ArtDAQError_CanNotPerformOpWhenNoDevInTask                                          (-200477)
#define ArtDAQError_ErrorOperationTimedOut                                                  (-200474)
#define ArtDAQError_CannotSetPropertyWhenTaskRunning                                        (-200557)
#define ArtDAQError_WriteFailsBufferSizeAutoConfigured                                      (-200547)
#define ArtDAQError_CannotReadWhenAutoStartFalseAndTaskNotRunningOrCommitted                (-200473)
#define ArtDAQError_CannotWriteWhenAutoStartFalseAndTaskNotRunningOrCommitted               (-200472)
#define ArtDAQError_CannotWriteNotStartedAutoStartFalseNotOnDemandBufSizeZero               (-200802)
#define ArtDAQError_CannotWriteToFiniteCOTask                                               (-201291)

#define ArtDAQError_SamplesNotYetAvailable                                                  (-200284)
#define ArtDAQError_SamplesNoLongerAvailable                                                (-200279)
#define ArtDAQError_SamplesWillNeverBeAvailable                                             (-200278)
#define ArtDAQError_RuntimeAborted_Routing                                                  (-88709)
#define ArtDAQError_Timeout                                                                 (-26802)
#define ArtDAQError_MinNotLessThanMax                                                       (-200082)
#define ArtDAQError_InvalidNumberSamplesToRead                                              (-200096)
#define ArtDAQError_InvalidNumSampsToWrite                                                  (-200622)

#define ArtDAQError_DeviceNameNotFound_Routing                                              (-88717)
#define ArtDAQError_InvalidRoutingSourceTerminalName_Routing                                (-89120)
#define ArtDAQError_InvalidTerm_Routing                                                     (-89129)
#define ArtDAQError_UnsupportedSignalTypeExportSignal                                       (-200375)

#define ArtDAQError_ChanSizeTooBigForU16PortWrite                                           (-200879)
#define ArtDAQError_ChanSizeTooBigForU16PortRead                                            (-200878)
#define ArtDAQError_ChanSizeTooBigForU32PortWrite                                           (-200566)
#define ArtDAQError_ChanSizeTooBigForU8PortWrite                                            (-200565)
#define ArtDAQError_ChanSizeTooBigForU32PortRead                                            (-200564)
#define ArtDAQError_ChanSizeTooBigForU8PortRead                                             (-200563)
#define ArtDAQError_WaitUntilDoneDoesNotIndicateDone                                        (-200560)
#define ArtDAQError_AutoStartWriteNotAllowedEventRegistered                                 (-200985)
#define ArtDAQError_AutoStartReadNotAllowedEventRegistered                                  (-200984)
#define ArtDAQError_EveryNSamplesAcqIntoBufferEventNotSupportedByDevice                     (-200981)
#define ArtDAQError_EveryNSampsTransferredFromBufferEventNotSupportedByDevice               (-200980)
#define ArtDAQError_CannotRegisterArtDAQSoftwareEventWhileTaskIsRunning                     (-200960)
#define ArtDAQError_EveryNSamplesEventNotSupportedForNonBufferedTasks                       (-200848)
#define ArtDAQError_EveryNSamplesEventNotSupport                                            (-200849)
#define ArtDAQError_BufferSizeNotMultipleOfEveryNSampsEventIntervalWhenDMA                  (-200877)
#define ArtDAQError_EveryNSampsTransferredFromBufferNotForInput                             (-200965)
#define ArtDAQError_EveryNSampsAcqIntoBufferNotForOutput                                    (-200964)
#define ArtDAQError_ReadNoInputChansInTask                                                  (-200460)
#define ArtDAQError_WriteNoOutputChansInTask                                                (-200459)
#define ArtDAQError_InvalidTimeoutVal                                                       (-200453)
#define ArtDAQError_AttributeNotSupportedInTaskContext                                      (-200452)
#define ArtDAQError_NoMoreSpace                                                             (-200293)
#define ArtDAQError_SamplesCanNotYetBeWritten                                               (-200292)
#define ArtDAQError_GenStoppedToPreventRegenOfOldSamples                                    (-200290)
#define ArtDAQError_SamplesWillNeverBeGenerated                                             (-200288)
#define ArtDAQError_CannotReadRelativeToRefTrigUntilDone                                    (-200281)
#define ArtDAQError_ExtSampClkSrcNotSpecified                                               (-200303)
#define ArtDAQError_CannotUpdatePulseGenProperty                                            (-200301)
#define ArtDAQError_InvalidTimingType                                                       (-200300)

#define ArtDAQError_InvalidAnalogTrigSrc                                                    (-200265)
#define ArtDAQError_TrigWhenOnDemandSampTiming                                              (-200262)
#define ArtDAQError_RefTrigWhenContinuous                                                   (-200358)
#define ArtDAQError_SpecifiedAttrNotValid                                                   (-200233)

#define ArtDAQError_OutputBufferEmpty                                                       (-200462)
#define ArtDAQError_InvalidOptionForDigitalPortChannel                                      (-200376)

#define ArtDAQError_CtrMinMax                                                               (-200527)
#define ArtDAQError_WriteChanTypeMismatch                                                   (-200526)
#define ArtDAQError_ReadChanTypeMismatch                                                    (-200525)
#define ArtDAQError_WriteNumChansMismatch                                                   (-200524)
#define ArtDAQError_OneChanReadForMultiChanTask                                             (-200523)

#define ArtDAQError_MultipleCounterInputTask                                                (-200147)
#define ArtDAQError_CounterStartPauseTriggerConflict                                        (-200146)
#define ArtDAQError_CounterInputPauseTriggerAndSampleClockInvalid                           (-200145)
#define ArtDAQError_CounterOutputPauseTriggerInvalid                                        (-200144)
#define ArtDAQError_FileNotFound                                                            (-26103)

#define ArtDAQError_NonbufferedOrNoChannels                                                 (-201395)
#define ArtDAQError_BufferedOperationsNotSupportedOnSelectedLines                           (-201062)
#define ArtDAQError_CalibrationFailed                                                       (-200157)

#define ArtDAQError_InvalidFillModeParameter                                                (-300001)

#define ArtDAQError_PhysChanOutputType                                                      (-200432)
#define ArtDAQError_PhysChanMeasType                                                        (-200431)
#define ArtDAQError_InvalidPhysChanType                                                     (-200430)

#define ArtDAQError_SuitableTimebaseNotFoundTimeCombo2                                      (-200746)
#define ArtDAQError_SuitableTimebaseNotFoundFrequencyCombo2                                 (-200745)

#define ArtDAQWarning_ReturnedDataIsNotEnough                                               (30014)




#ifdef __cplusplus
}
#endif

#endif // ___ART_DAQ_H___;
