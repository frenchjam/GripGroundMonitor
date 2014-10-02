// GripGroundMonitor.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "GripGroundMonitor.h"
#include "GripGroundMonitorDlg.h"

#include <shellapi.h>

#include "..\Useful\fMessageBox.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CGripGroundMonitorApp

BEGIN_MESSAGE_MAP(CGripGroundMonitorApp, CWinApp)
	//{{AFX_MSG_MAP(CGripGroundMonitorApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGripGroundMonitorApp construction

CGripGroundMonitorApp::CGripGroundMonitorApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CGripGroundMonitorApp object

CGripGroundMonitorApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CGripGroundMonitorApp initialization

BOOL CGripGroundMonitorApp::InitInstance()
{

	if (!AfxSocketInit())
	{
		AfxMessageBox(IDP_SOCKETS_INIT_FAILED);
		return FALSE;
	}

	AfxEnableControlContainer();

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.

#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif

	// Hold the pathnames to the packet cache and to the scripts.
	// Default values are defined here.
	char packetPathRoot[PATHLENGTH] = "GripPackets";
	char scriptDirectory[PATHLENGTH] = "scripts\\";

	// Override the defaults with command line parameters.
	// We are using the hidden __argc and __argv variables for simplicity.
	// First parameter on the command line is the path to the packet cache.
	if ( __argc > 1 ) strcpy( packetPathRoot, __argv[1] );
	// Second is the directory containing the GRIP scripts.
	if ( __argc > 2 ) strcpy( scriptDirectory, __argv[2] );

	// fMessageBox( MB_OK, "Test", "Args (%d): %s %s", __argc, __argv[1], __argv[2] );

	// Run the dialog with the specified cache and script directories.
	CGripGroundMonitorDlg *dlg = new CGripGroundMonitorDlg( NULL, packetPathRoot, scriptDirectory );
	m_pMainWnd = dlg;
	int nResponse = dlg->DoModal();
	if (nResponse == IDOK)
	{
		// TODO: Place code here to handle when the dialog is
		//  dismissed with OK
	}
	else if (nResponse == IDCANCEL)
	{
		// TODO: Place code here to handle when the dialog is
		//  dismissed with Cancel
	}

	delete dlg;

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}
