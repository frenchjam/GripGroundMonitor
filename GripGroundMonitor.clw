; CLW file contains information for the MFC ClassWizard

[General Info]
Version=1
LastClass=IddGripGroundDialog
LastTemplate=CDialog
NewFileInclude1=#include "stdafx.h"
NewFileInclude2=#include "GripGroundMonitor.h"

ClassCount=4
Class1=CGripGroundMonitorApp
Class2=CGripGroundMonitorDlg
Class3=CAboutDlg

ResourceCount=3
Resource1=IDD_ABOUTBOX
Resource2=IDR_MAINFRAME
Class4=IddGripGroundDialog
Resource3=IDD_GRIPGROUNDMONITOR_DIALOG

[CLS:CGripGroundMonitorApp]
Type=0
HeaderFile=GripGroundMonitor.h
ImplementationFile=GripGroundMonitor.cpp
Filter=N

[CLS:CGripGroundMonitorDlg]
Type=0
HeaderFile=GripGroundMonitorDlg.h
ImplementationFile=GripGroundMonitorDlg.cpp
Filter=D
BaseClass=CDialog
VirtualFilter=dWC
LastObject=CGripGroundMonitorDlg

[CLS:CAboutDlg]
Type=0
HeaderFile=GripGroundMonitorDlg.h
ImplementationFile=GripGroundMonitorDlg.cpp
Filter=D

[DLG:IDD_ABOUTBOX]
Type=1
Class=CAboutDlg
ControlCount=4
Control1=IDC_STATIC,static,1342177283
Control2=IDC_STATIC,static,1342308480
Control3=IDC_STATIC,static,1342308352
Control4=IDOK,button,1342373889

[DLG:IDD_GRIPGROUNDMONITOR_DIALOG]
Type=1
Class=IddGripGroundDialog
ControlCount=39
Control1=IDC_STEPS,listbox,1353777409
Control2=IDC_SUBJECTID,edit,1350631552
Control3=IDC_PROTOCOLID,edit,1350631552
Control4=IDC_TASKID,edit,1350631552
Control5=IDC_STEPID,edit,1350631552
Control6=IDC_SUBJECTS,listbox,1352728833
Control7=IDC_PROTOCOLS,listbox,1352728833
Control8=IDC_TASKS,listbox,1352728833
Control9=IDREALCANCEL,button,1073807360
Control10=IDC_STATUS_TEXT,static,1342308352
Control11=IDC_PICTURE,static,1342177806
Control12=IDC_SUBJECT_LABEL,button,1342177287
Control13=IDC_TASK,button,1342177287
Control14=IDC_SESSION_LABEL,button,1342177287
Control15=IDC_STEP,button,1342177287
Control16=IDC_STATIC,button,1342177287
Control17=IDC_CURRENT_LINE,static,1342308352
Control18=IDC_TYPE,static,1342308353
Control19=IDC_STATIC,button,1342177287
Control20=IDC_TASK2,button,1342177287
Control21=IDC_NEXT_STEP,button,1342193664
Control22=IDC_GOTO,button,1342177281
Control23=IDRETRY,button,1342210048
Control24=IDIGNORE,button,1342210048
Control25=IDABORT,button,1342210048
Control26=IDCANCEL,button,1342210048
Control27=IDOK,button,1342210048
Control28=IDINTERRUPT,button,1342210048
Control29=IDC_STRIPCHARTS,static,1342177287
Control30=IDC_STATIC,button,1342177287
Control31=IDC_STATIC,button,1342177287
Control32=IDC_STATIC,button,1342177287
Control33=IDC_STATIC,button,1342177287
Control34=IDC_STATIC,button,1342177287
Control35=IDC_ZY,static,1342177287
Control36=IDC_XY,static,1342177287
Control37=IDC_XZ,static,1342177287
Control38=IDC_COP,static,1342177287
Control39=IDC_STATIC,button,1342177287

[CLS:IddGripGroundDialog]
Type=0
HeaderFile=IddGripGroundDialog.h
ImplementationFile=IddGripGroundDialog.cpp
BaseClass=CDialog
Filter=D
LastObject=IDABORT

