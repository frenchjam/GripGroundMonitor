#if !defined(AFX_IDDGRIPGROUNDDIALOG_H__EF224E2A_8ED0_487A_837C_7883126A62A8__INCLUDED_)
#define AFX_IDDGRIPGROUNDDIALOG_H__EF224E2A_8ED0_487A_837C_7883126A62A8__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// IddGripGroundDialog.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// IddGripGroundDialog dialog

class IddGripGroundDialog : public CDialog
{
// Construction
public:
	IddGripGroundDialog(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(IddGripGroundDialog)
	enum { IDD = IDD_GRIPGROUNDMONITOR_DIALOG };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(IddGripGroundDialog)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(IddGripGroundDialog)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_IDDGRIPGROUNDDIALOG_H__EF224E2A_8ED0_487A_837C_7883126A62A8__INCLUDED_)
