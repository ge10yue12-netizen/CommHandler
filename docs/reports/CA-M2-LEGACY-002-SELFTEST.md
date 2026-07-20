# 自检自测报告

## 1. 基本信息

- 变更编号：CA-M2-LEGACY-002
- 日期：2026-07-20

## 2. 证据

```text
已构建 CommHandler Debug DLL
MSBuild CoreSelfTest → 成功；PostBuild 复制依赖
CoreSelfTest → === done: 0 failed ===
含：dll smoke configure / open accept / settled / exclusive released
主程序：临时 ProjectGuid 构建成功并覆盖 CommunicationAssistant.exe
```

## 3. 键名核对（源码）

- 网口：iTransferType, iModel, sLocalIP, iLocalPort, sDestIP, iDestPort, iProtoType, input/output, bInquireSendFlag；万测另设 bOpenCMD/cCMD
- 串口：iComPort(0-based), iComBaud(索引), iDataBits, iParity, iStopBits, iProtocolType

## 4. 运行依赖

OutDir 需：CommHandler.dll、Art_DAQ.dll、nicaiu.dll、NET_AMC3XER.dll、Usb_AMC2XE_Dll.dll（PostBuild 尽力复制）