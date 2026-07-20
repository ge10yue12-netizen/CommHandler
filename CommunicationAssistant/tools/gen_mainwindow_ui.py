# -*- coding: utf-8 -*-
"""Generate CommunicationAssistant/ui/MainWindow.ui (appearance-equivalent shell)."""
from __future__ import print_function
import os

OUT = os.path.join(os.path.dirname(__file__), "..", "ui", "MainWindow.ui")


def prop(name, typ, value, indent=2):
    sp = " " * indent
    if typ == "string":
        return '%s<property name="%s">\n%s <string>%s</string>\n%s</property>\n' % (sp, name, sp, value, sp)
    if typ == "bool":
        return '%s<property name="%s">\n%s <bool>%s</bool>\n%s</property>\n' % (sp, name, sp, value, sp)
    if typ == "number":
        return '%s<property name="%s">\n%s <number>%s</number>\n%s</property>\n' % (sp, name, sp, value, sp)
    if typ == "enum":
        return '%s<property name="%s">\n%s <enum>%s</enum>\n%s</property>\n' % (sp, name, sp, value, sp)
    if typ == "set":
        return '%s<property name="%s">\n%s <set>%s</set>\n%s</property>\n' % (sp, name, sp, value, sp)
    if typ == "sizepolicy":
        h, v, hs, vs = value
        return (
            '%s<property name="sizePolicy">\n'
            '%s <sizepolicy hsizetype="%s" vsizetype="%s">\n'
            '%s  <horstretch>%s</horstretch>\n'
            '%s  <verstretch>%s</verstretch>\n'
            '%s </sizepolicy>\n'
            '%s</property>\n' % (sp, sp, h, v, sp, hs, sp, vs, sp, sp)
        )
    return ""


def item_combo(text):
    return (
        '     <item>\n'
        '      <property name="text">\n'
        '       <string>%s</string>\n'
        '      </property>\n'
        '     </item>\n' % text
    )


parts = []
parts.append('<?xml version="1.0" encoding="UTF-8"?>\n')
parts.append('<ui version="4.0">\n')
parts.append(' <class>MainWindow</class>\n')
parts.append(' <widget class="QMainWindow" name="MainWindow">\n')
parts.append(prop("geometry", "string", "", 2).replace(
    '<string></string>',
    '<rect>\n    <x>0</x>\n    <y>0</y>\n    <width>1100</width>\n    <height>720</height>\n   </rect>'
))
# fix geometry properly
parts = parts[:-1]
parts.append(
    '  <property name="geometry">\n'
    '   <rect>\n'
    '    <x>0</x>\n'
    '    <y>0</y>\n'
    '    <width>1100</width>\n'
    '    <height>720</height>\n'
    '   </rect>\n'
    '  </property>\n'
)
parts.append(prop("minimumSize", "string", "").replace(
    '<string></string>',
    '<size>\n    <width>900</width>\n    <height>600</height>\n   </size>'
))
parts = [p for p in parts if 'minimumSize' not in p or '<size>' in p or 'property name="geometry"' in p or p.startswith('<?') or p.startswith('<ui') or p.startswith(' <class') or p.startswith(' <widget class="QMainWindow"')]
# rewrite cleanly
parts = []
parts.append('''<?xml version="1.0" encoding="UTF-8"?>
<ui version="4.0">
 <class>MainWindow</class>
 <widget class="QMainWindow" name="MainWindow">
  <property name="geometry">
   <rect>
    <x>0</x>
    <y>0</y>
    <width>1100</width>
    <height>720</height>
   </rect>
  </property>
  <property name="minimumSize">
   <size>
    <width>900</width>
    <height>600</height>
   </size>
  </property>
  <property name="windowTitle">
   <string>通信调试助手 — Native / Legacy</string>
  </property>
  <widget class="QWidget" name="centralwidget">
   <layout class="QHBoxLayout" name="rootLayout">
    <property name="spacing">
     <number>0</number>
    </property>
    <property name="leftMargin">
     <number>0</number>
    </property>
    <property name="topMargin">
     <number>0</number>
    </property>
    <property name="rightMargin">
     <number>0</number>
    </property>
    <property name="bottomMargin">
     <number>0</number>
    </property>
    <item>
     <widget class="QScrollArea" name="sideScroll">
      <property name="objectName">
       <string notr="true">SideScroll</string>
      </property>
      <property name="minimumSize">
       <size>
        <width>340</width>
        <height>0</height>
       </size>
      </property>
      <property name="maximumSize">
       <size>
        <width>340</width>
        <height>16777215</height>
       </size>
      </property>
      <property name="widgetResizable">
       <bool>true</bool>
      </property>
      <property name="frameShape">
       <enum>QFrame::NoFrame</enum>
      </property>
      <widget class="QWidget" name="sidePanel">
       <property name="objectName">
        <string notr="true">SidePanel</string>
       </property>
       <property name="minimumSize">
        <size>
         <width>300</width>
         <height>0</height>
        </size>
       </property>
       <layout class="QVBoxLayout" name="sideLay">
        <property name="spacing">
         <number>6</number>
        </property>
        <property name="leftMargin">
         <number>16</number>
        </property>
        <property name="topMargin">
         <number>16</number>
        </property>
        <property name="rightMargin">
         <number>16</number>
        </property>
        <property name="bottomMargin">
         <number>16</number>
        </property>
        <item>
         <widget class="QLabel" name="appTitle">
          <property name="objectName">
           <string notr="true">AppTitle</string>
          </property>
          <property name="text">
           <string>通信调试助手</string>
          </property>
         </widget>
        </item>
        <item>
         <widget class="QLabel" name="secWorkMode">
          <property name="text">
           <string>工作模式</string>
          </property>
         </widget>
        </item>
        <item>
         <widget class="QComboBox" name="workModeCombo"/>
        </item>
        <item>
         <widget class="QLabel" name="secConn">
          <property name="text">
           <string>连接方式</string>
          </property>
         </widget>
        </item>
        <item>
         <widget class="QComboBox" name="transportCombo"/>
        </item>
        <item>
         <widget class="QStackedWidget" name="paramStack">
''')

# Serial page
parts.append('''          <widget class="QWidget" name="pageSerial">
           <layout class="QFormLayout" name="formSerial">
            <property name="leftMargin"><number>0</number></property>
            <property name="topMargin"><number>8</number></property>
            <property name="rightMargin"><number>0</number></property>
            <property name="bottomMargin"><number>0</number></property>
            <item row="0" column="0"><widget class="QLabel" name="lblPort"><property name="text"><string>端口</string></property></widget></item>
            <item row="0" column="1"><widget class="QComboBox" name="portCombo"/></item>
            <item row="1" column="0"><widget class="QLabel" name="lblBaud"><property name="text"><string>波特率</string></property></widget></item>
            <item row="1" column="1"><widget class="QComboBox" name="baudCombo"/></item>
            <item row="2" column="0"><widget class="QLabel" name="lblDataBits"><property name="text"><string>数据位</string></property></widget></item>
            <item row="2" column="1"><widget class="QComboBox" name="dataBitsCombo"/></item>
            <item row="3" column="0"><widget class="QLabel" name="lblParity"><property name="text"><string>校验</string></property></widget></item>
            <item row="3" column="1"><widget class="QComboBox" name="parityCombo"/></item>
            <item row="4" column="0"><widget class="QLabel" name="lblStopBits"><property name="text"><string>停止位</string></property></widget></item>
            <item row="4" column="1"><widget class="QComboBox" name="stopBitsCombo"/></item>
           </layout>
          </widget>
''')

# TCP client
parts.append('''          <widget class="QWidget" name="pageTcpClient">
           <layout class="QFormLayout" name="formTcpClient">
            <property name="leftMargin"><number>0</number></property>
            <property name="topMargin"><number>8</number></property>
            <property name="rightMargin"><number>0</number></property>
            <property name="bottomMargin"><number>0</number></property>
            <item row="0" column="0"><widget class="QLabel" name="lblTcpHost"><property name="text"><string>远端地址</string></property></widget></item>
            <item row="0" column="1"><widget class="QLineEdit" name="tcpHostEdit"><property name="text"><string>127.0.0.1</string></property></widget></item>
            <item row="1" column="0"><widget class="QLabel" name="lblTcpPort"><property name="text"><string>远端端口</string></property></widget></item>
            <item row="1" column="1"><widget class="QSpinBox" name="tcpPortSpin"><property name="maximum"><number>65535</number></property><property name="minimum"><number>1</number></property><property name="value"><number>9000</number></property></widget></item>
           </layout>
          </widget>
''')

# TCP server
parts.append('''          <widget class="QWidget" name="pageTcpServer">
           <layout class="QFormLayout" name="formTcpServer">
            <property name="leftMargin"><number>0</number></property>
            <property name="topMargin"><number>8</number></property>
            <property name="rightMargin"><number>0</number></property>
            <property name="bottomMargin"><number>0</number></property>
            <item row="0" column="0"><widget class="QLabel" name="lblTcpSrvAddr"><property name="text"><string>本地地址</string></property></widget></item>
            <item row="0" column="1"><widget class="QLineEdit" name="tcpServerAddrEdit"><property name="text"><string>0.0.0.0</string></property></widget></item>
            <item row="1" column="0"><widget class="QLabel" name="lblTcpSrvPort"><property name="text"><string>本地端口</string></property></widget></item>
            <item row="1" column="1"><widget class="QSpinBox" name="tcpServerPortSpin"><property name="maximum"><number>65535</number></property><property name="minimum"><number>1</number></property><property name="value"><number>9000</number></property></widget></item>
           </layout>
          </widget>
''')

# UDP
parts.append('''          <widget class="QWidget" name="pageUdp">
           <layout class="QFormLayout" name="formUdp">
            <property name="leftMargin"><number>0</number></property>
            <property name="topMargin"><number>8</number></property>
            <property name="rightMargin"><number>0</number></property>
            <property name="bottomMargin"><number>0</number></property>
            <item row="0" column="0"><widget class="QLabel" name="lblUdpLocalAddr"><property name="text"><string>本地地址</string></property></widget></item>
            <item row="0" column="1"><widget class="QLineEdit" name="udpLocalAddrEdit"><property name="text"><string>0.0.0.0</string></property></widget></item>
            <item row="1" column="0"><widget class="QLabel" name="lblUdpLocalPort"><property name="text"><string>本地端口</string></property></widget></item>
            <item row="1" column="1"><widget class="QSpinBox" name="udpLocalPortSpin"><property name="maximum"><number>65535</number></property><property name="minimum"><number>1</number></property><property name="value"><number>9001</number></property></widget></item>
            <item row="2" column="0"><widget class="QLabel" name="lblUdpRemoteAddr"><property name="text"><string>远端地址</string></property></widget></item>
            <item row="2" column="1"><widget class="QLineEdit" name="udpRemoteAddrEdit"><property name="text"><string>127.0.0.1</string></property></widget></item>
            <item row="3" column="0"><widget class="QLabel" name="lblUdpRemotePort"><property name="text"><string>远端端口</string></property></widget></item>
            <item row="3" column="1"><widget class="QSpinBox" name="udpRemotePortSpin"><property name="maximum"><number>65535</number></property><property name="minimum"><number>1</number></property><property name="value"><number>9002</number></property></widget></item>
           </layout>
          </widget>
''')

# Legacy page with nested stack
parts.append('''          <widget class="QWidget" name="pageLegacy">
           <layout class="QFormLayout" name="formLegacy">
            <property name="leftMargin"><number>0</number></property>
            <property name="topMargin"><number>8</number></property>
            <property name="rightMargin"><number>0</number></property>
            <property name="bottomMargin"><number>0</number></property>
            <item row="0" column="0"><widget class="QLabel" name="lblLegacyComm"><property name="text"><string>通信类型</string></property></widget></item>
            <item row="0" column="1"><widget class="QComboBox" name="legacyCommTypeCombo"/></item>
            <item row="1" column="0"><widget class="QLabel" name="lblLegacyProto"><property name="text"><string>协议</string></property></widget></item>
            <item row="1" column="1"><widget class="QComboBox" name="legacyProtocolCombo"/></item>
            <item row="2" column="0"><widget class="QLabel" name="lblLegacyEp"><property name="text"><string>端点</string></property></widget></item>
            <item row="2" column="1">
             <widget class="QStackedWidget" name="legacyEndpointStack">
              <widget class="QWidget" name="pageLegacyNet">
               <layout class="QFormLayout" name="formLegacyNet">
                <property name="leftMargin"><number>0</number></property>
                <property name="topMargin"><number>0</number></property>
                <property name="rightMargin"><number>0</number></property>
                <property name="bottomMargin"><number>0</number></property>
                <item row="0" column="0"><widget class="QLabel" name="lblLegacyXfer"><property name="text"><string>传输</string></property></widget></item>
                <item row="0" column="1"><widget class="QComboBox" name="legacyTransferCombo"/></item>
                <item row="1" column="0"><widget class="QLabel" name="lblLegacyModel"><property name="text"><string>角色</string></property></widget></item>
                <item row="1" column="1"><widget class="QComboBox" name="legacyModelCombo"/></item>
                <item row="2" column="0"><widget class="QLabel" name="lblLegacyLocalAddr"><property name="text"><string>本地地址</string></property></widget></item>
                <item row="2" column="1"><widget class="QLineEdit" name="legacyLocalAddrEdit"><property name="text"><string>127.0.0.1</string></property></widget></item>
                <item row="3" column="0"><widget class="QLabel" name="lblLegacyLocalPort"><property name="text"><string>本地端口</string></property></widget></item>
                <item row="3" column="1"><widget class="QSpinBox" name="legacyLocalPortSpin"><property name="maximum"><number>65535</number></property><property name="value"><number>0</number></property></widget></item>
                <item row="4" column="0"><widget class="QLabel" name="lblLegacyRemoteAddr"><property name="text"><string>远端地址</string></property></widget></item>
                <item row="4" column="1"><widget class="QLineEdit" name="legacyRemoteAddrEdit"><property name="text"><string>127.0.0.1</string></property></widget></item>
                <item row="5" column="0"><widget class="QLabel" name="lblLegacyRemotePort"><property name="text"><string>远端端口</string></property></widget></item>
                <item row="5" column="1"><widget class="QSpinBox" name="legacyRemotePortSpin"><property name="maximum"><number>65535</number></property><property name="minimum"><number>1</number></property><property name="value"><number>9000</number></property></widget></item>
               </layout>
              </widget>
              <widget class="QWidget" name="pageLegacySerial">
               <layout class="QFormLayout" name="formLegacySerial">
                <property name="leftMargin"><number>0</number></property>
                <property name="topMargin"><number>0</number></property>
                <property name="rightMargin"><number>0</number></property>
                <property name="bottomMargin"><number>0</number></property>
                <item row="0" column="0"><widget class="QLabel" name="lblLegacyPort"><property name="text"><string>端口</string></property></widget></item>
                <item row="0" column="1"><widget class="QComboBox" name="legacyPortCombo"/></item>
                <item row="1" column="0"><widget class="QLabel" name="lblLegacyBaud"><property name="text"><string>波特率</string></property></widget></item>
                <item row="1" column="1"><widget class="QComboBox" name="legacyBaudCombo"/></item>
               </layout>
              </widget>
             </widget>
            </item>
            <item row="3" column="0"><widget class="QLabel" name="lblLegacyMockSpacer"><property name="text"><string/></property></widget></item>
            <item row="3" column="1">
             <widget class="QCheckBox" name="legacyMockCheck">
              <property name="text"><string>使用 Mock 后端（不调 DLL）</string></property>
             </widget>
            </item>
            <item row="4" column="0"><widget class="QLabel" name="lblLegacyCap"><property name="text"><string>能力</string></property></widget></item>
            <item row="4" column="1">
             <widget class="QLabel" name="legacyCapTipLabel">
              <property name="objectName"><string notr="true">CapTip</string></property>
              <property name="text"><string/></property>
              <property name="wordWrap"><bool>true</bool></property>
             </widget>
            </item>
           </layout>
          </widget>
''')

parts.append('''         </widget>
        </item>
        <item>
         <widget class="QPushButton" name="btnOpen">
          <property name="objectName"><string notr="true">PrimaryButton</string></property>
          <property name="text"><string>打开连接</string></property>
         </widget>
        </item>
        <item>
         <widget class="QLabel" name="statusLabel">
          <property name="objectName"><string notr="true">StatusPill</string></property>
          <property name="text"><string>状态：已创建</string></property>
         </widget>
        </item>
        <item>
         <widget class="QLabel" name="clientSectionLabel">
          <property name="text"><string>已连接客户端</string></property>
         </widget>
        </item>
        <item>
         <widget class="QComboBox" name="clientCombo"/>
        </item>
        <item>
         <widget class="QPushButton" name="btnDisconnectClient">
          <property name="objectName"><string notr="true">SecondaryButton</string></property>
          <property name="text"><string>断开选中客户端</string></property>
         </widget>
        </item>
        <item>
         <widget class="QLabel" name="secDisplay">
          <property name="text"><string>显示与抓包</string></property>
         </widget>
        </item>
        <item>
         <widget class="QCheckBox" name="hexDisplayCheck">
          <property name="text"><string>十六进制显示</string></property>
          <property name="checked"><bool>true</bool></property>
         </widget>
        </item>
        <item>
         <widget class="QCheckBox" name="captureEnableCheck">
          <property name="text"><string>抓包落盘（JSONL）</string></property>
          <property name="checked"><bool>true</bool></property>
         </widget>
        </item>
        <item>
         <widget class="QPushButton" name="btnClear">
          <property name="objectName"><string notr="true">GhostButton</string></property>
          <property name="text"><string>清空数据/日志</string></property>
         </widget>
        </item>
        <item>
         <spacer name="sideStretch">
          <property name="orientation"><enum>Qt::Vertical</enum></property>
          <property name="sizeHint" stdset="0"><size><width>20</width><height>40</height></size></property>
         </spacer>
        </item>
       </layout>
      </widget>
     </widget>
    </item>
''')

# Main panel
parts.append('''    <item>
     <widget class="QWidget" name="mainPanel">
      <property name="objectName"><string notr="true">MainPanel</string></property>
      <layout class="QVBoxLayout" name="mainLay">
       <property name="spacing"><number>10</number></property>
       <property name="leftMargin"><number>16</number></property>
       <property name="topMargin"><number>16</number></property>
       <property name="rightMargin"><number>16</number></property>
       <property name="bottomMargin"><number>12</number></property>
       <item>
        <widget class="QSplitter" name="panesSplitter">
         <property name="orientation"><enum>Qt::Vertical</enum></property>
         <property name="childrenCollapsible"><bool>false</bool></property>
         <widget class="QWidget" name="dataPane">
          <layout class="QVBoxLayout" name="dataLay">
           <property name="spacing"><number>4</number></property>
           <property name="leftMargin"><number>0</number></property>
           <property name="topMargin"><number>0</number></property>
           <property name="rightMargin"><number>0</number></property>
           <property name="bottomMargin"><number>0</number></property>
           <item>
            <widget class="QLabel" name="dataTitle">
             <property name="objectName"><string notr="true">PaneTitle</string></property>
             <property name="text"><string>数据显示（接收指令 / 测数 / 收发载荷，便于校验）</string></property>
            </widget>
           </item>
           <item>
            <widget class="QPlainTextEdit" name="dataView">
             <property name="objectName"><string notr="true">DataView</string></property>
             <property name="readOnly"><bool>true</bool></property>
             <property name="placeholderText"><string>此处显示收到的指令与测数、以及已提交的发送载荷，便于核对是否正确</string></property>
            </widget>
           </item>
          </layout>
         </widget>
         <widget class="QWidget" name="logPane">
          <layout class="QVBoxLayout" name="logLay">
           <property name="spacing"><number>4</number></property>
           <property name="leftMargin"><number>0</number></property>
           <property name="topMargin"><number>0</number></property>
           <property name="rightMargin"><number>0</number></property>
           <property name="bottomMargin"><number>0</number></property>
           <item>
            <widget class="QLabel" name="logTitle">
             <property name="objectName"><string notr="true">PaneTitle</string></property>
             <property name="text"><string>日志（连接 / 控制指令 / 边界与异常）</string></property>
            </widget>
           </item>
           <item>
            <widget class="QPlainTextEdit" name="log">
             <property name="objectName"><string notr="true">LogView</string></property>
             <property name="readOnly"><bool>true</bool></property>
            </widget>
           </item>
          </layout>
         </widget>
        </widget>
       </item>
       <item>
        <widget class="QLabel" name="sendWorkbenchTitle">
         <property name="objectName"><string notr="true">PaneTitle</string></property>
         <property name="text"><string>发送工作台</string></property>
        </widget>
       </item>
       <item>
        <widget class="QTabWidget" name="sendTabs">
         <property name="objectName"><string notr="true">SendTabs</string></property>
         <property name="documentMode"><bool>true</bool></property>
         <property name="currentIndex"><number>0</number></property>
         <widget class="QWidget" name="tabSingle">
          <attribute name="title"><string>单条发送</string></attribute>
          <layout class="QVBoxLayout" name="singleLay">
           <property name="spacing"><number>6</number></property>
           <property name="leftMargin"><number>8</number></property>
           <property name="topMargin"><number>8</number></property>
           <property name="rightMargin"><number>8</number></property>
           <property name="bottomMargin"><number>8</number></property>
           <item>
            <layout class="QHBoxLayout" name="singleOpts">
             <item>
              <widget class="QCheckBox" name="hexSendCheck">
               <property name="text"><string>十六进制发送（仅 Native）</string></property>
               <property name="checked"><bool>true</bool></property>
              </widget>
             </item>
             <item>
              <widget class="QComboBox" name="legacySendModeCombo"/>
             </item>
            </layout>
           </item>
           <item>
            <layout class="QHBoxLayout" name="sendRow">
             <item>
              <widget class="QPlainTextEdit" name="sendEdit">
               <property name="objectName"><string notr="true">SendEdit</string></property>
               <property name="maximumSize"><size><width>16777215</width><height>88</height></size></property>
              </widget>
             </item>
             <item>
              <widget class="QPushButton" name="btnSend">
               <property name="objectName"><string notr="true">SendButton</string></property>
               <property name="text"><string>➤</string></property>
               <property name="toolTip"><string>发送当前内容</string></property>
              </widget>
             </item>
            </layout>
           </item>
          </layout>
         </widget>
         <widget class="QWidget" name="tabList">
          <attribute name="title"><string>发送列表</string></attribute>
          <layout class="QVBoxLayout" name="listLay">
           <property name="spacing"><number>6</number></property>
           <property name="leftMargin"><number>8</number></property>
           <property name="topMargin"><number>8</number></property>
           <property name="rightMargin"><number>8</number></property>
           <property name="bottomMargin"><number>8</number></property>
           <item>
            <layout class="QHBoxLayout" name="toolRow">
             <item><widget class="QPushButton" name="btnListAdd"><property name="objectName"><string notr="true">SecondaryButton</string></property><property name="text"><string>添加</string></property></widget></item>
             <item><widget class="QPushButton" name="btnListRemove"><property name="objectName"><string notr="true">SecondaryButton</string></property><property name="text"><string>删除选中</string></property></widget></item>
             <item><widget class="QPushButton" name="btnListImport"><property name="objectName"><string notr="true">SecondaryButton</string></property><property name="text"><string>导入 txt/csv…</string></property></widget></item>
             <item><widget class="QPushButton" name="btnListExport"><property name="objectName"><string notr="true">SecondaryButton</string></property><property name="text"><string>导出…</string></property></widget></item>
             <item><widget class="QPushButton" name="btnListSendSelected"><property name="objectName"><string notr="true">SecondaryButton</string></property><property name="text"><string>发送选中行</string></property></widget></item>
             <item><spacer name="toolStretch"><property name="orientation"><enum>Qt::Horizontal</enum></property><property name="sizeHint" stdset="0"><size><width>40</width><height>20</height></size></property></spacer></item>
            </layout>
           </item>
           <item>
            <widget class="QTableWidget" name="sendListTable">
             <property name="objectName"><string notr="true">SendList</string></property>
             <property name="minimumSize"><size><width>0</width><height>140</height></size></property>
             <property name="selectionBehavior"><enum>QAbstractItemView::SelectRows</enum></property>
             <property name="selectionMode"><enum>QAbstractItemView::ExtendedSelection</enum></property>
             <property name="columnCount"><number>5</number></property>
             <column><property name="text"><string>启用</string></property></column>
             <column><property name="text"><string>名称</string></property></column>
             <column><property name="text"><string>载荷</string></property></column>
             <column><property name="text"><string>延时ms</string></property></column>
             <column><property name="text"><string>格式</string></property></column>
            </widget>
           </item>
           <item>
            <layout class="QHBoxLayout" name="schedRow">
             <item><widget class="QComboBox" name="schedModeCombo"/></item>
             <item><widget class="QSpinBox" name="schedIntervalSpin"><property name="maximum"><number>3600000</number></property><property name="value"><number>1000</number></property><property name="suffix"><string> ms 默认间隔</string></property></widget></item>
             <item><widget class="QSpinBox" name="schedCountSpin"><property name="maximum"><number>1000000</number></property><property name="minimum"><number>1</number></property><property name="value"><number>10</number></property><property name="prefix"><string>次数 </string></property></widget></item>
             <item><widget class="QPushButton" name="btnSchedStart"><property name="objectName"><string notr="true">SecondaryButton</string></property><property name="text"><string>启动调度</string></property></widget></item>
             <item><widget class="QPushButton" name="btnSchedPause"><property name="objectName"><string notr="true">SecondaryButton</string></property><property name="text"><string>暂停</string></property></widget></item>
             <item><widget class="QPushButton" name="btnSchedResume"><property name="objectName"><string notr="true">SecondaryButton</string></property><property name="text"><string>继续</string></property></widget></item>
             <item><widget class="QPushButton" name="btnSchedStop"><property name="objectName"><string notr="true">SecondaryButton</string></property><property name="text"><string>停止</string></property></widget></item>
            </layout>
           </item>
           <item>
            <widget class="QLabel" name="listHint">
             <property name="objectName"><string notr="true">CapTip</string></property>
             <property name="text"><string>列表列：启用 | 名称 | 载荷 | 延时ms | 格式。可用「导入」选择本机 .txt / .csv。</string></property>
             <property name="wordWrap"><bool>true</bool></property>
            </widget>
           </item>
          </layout>
         </widget>
        </widget>
       </item>
       <item>
        <layout class="QHBoxLayout" name="footer">
         <item>
          <widget class="QLabel" name="txCountLabel">
           <property name="objectName"><string notr="true">FooterStat</string></property>
           <property name="text"><string>发送：0</string></property>
          </widget>
         </item>
         <item>
          <spacer name="footerSp1"><property name="orientation"><enum>Qt::Horizontal</enum></property><property name="sizeType"><enum>QSizePolicy::Fixed</enum></property><property name="sizeHint" stdset="0"><size><width>16</width><height>20</height></size></property></spacer>
         </item>
         <item>
          <widget class="QLabel" name="rxCountLabel">
           <property name="objectName"><string notr="true">FooterStat</string></property>
           <property name="text"><string>接收：0 - 0</string></property>
          </widget>
         </item>
         <item>
          <spacer name="footerStretch"><property name="orientation"><enum>Qt::Horizontal</enum></property><property name="sizeHint" stdset="0"><size><width>40</width><height>20</height></size></property></spacer>
         </item>
         <item>
          <widget class="QPushButton" name="btnResetCount">
           <property name="objectName"><string notr="true">GhostButton</string></property>
           <property name="text"><string>复位计数</string></property>
          </widget>
         </item>
        </layout>
       </item>
      </layout>
     </widget>
    </item>
   </layout>
  </widget>
 </widget>
 <resources/>
 <connections/>
</ui>
''')

text = "".join(parts)
# Fix duplicate objectName properties - Qt Designer uses name= attribute; objectName property for stylesheet may need to match name
# For widgets where we set objectName property differently from name (SideScroll vs sideScroll), stylesheet uses objectName.
# In Qt, setObjectName is from name= in .ui by default. Extra objectName property overrides.

out_path = os.path.normpath(OUT)
with open(out_path, "w", encoding="utf-8") as f:
    f.write(text)
print("Wrote", out_path, "bytes", len(text.encode("utf-8")))
