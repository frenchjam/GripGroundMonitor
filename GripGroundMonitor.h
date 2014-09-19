// GripGroundMonitor.h : main header file for the GRIPGROUNDMONITOR application
//

#if !defined(AFX_GRIPGROUNDMONITOR_H__E75B8D9D_488F_497A_87A1_560B8228F240__INCLUDED_)
#define AFX_GRIPGROUNDMONITOR_H__E75B8D9D_488F_497A_87A1_560B8228F240__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CGripGroundMonitorApp:
// See GripGroundMonitor.cpp for the implementation of this class
//

class CGripGroundMonitorApp : public CWinApp
{
public:
	CGripGroundMonitorApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGripGroundMonitorApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CGripGroundMonitorApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GRIPGROUNDMONITOR_H__E75B8D9D_488F_497A_87A1_560B8228F240__INCLUDED_)
