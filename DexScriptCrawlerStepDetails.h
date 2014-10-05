#if !defined(AFX_DEXSCRIPTCRAWLERSTEPDETAILS_H__DA230F58_4BC9_4D35_B21C_83A4A99023AB__INCLUDED_)
#define AFX_DEXSCRIPTCRAWLERSTEPDETAILS_H__DA230F58_4BC9_4D35_B21C_83A4A99023AB__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DexScriptCrawlerStepDetails.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// DexScriptCrawlerStepDetails dialog

class DexScriptCrawlerStepDetails : public CDialog
{
// Construction
public:
	DexScriptCrawlerStepDetails(CWnd* pParent = NULL);   // standard constructor
	void DisplayLine( const char *line );

// Dialog Data
	//{{AFX_DATA(DexScriptCrawlerStepDetails)
	enum { IDD = IDD_FULLLINE };
	CStatic	m_linedetails;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(DexScriptCrawlerStepDetails)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(DexScriptCrawlerStepDetails)
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DEXSCRIPTCRAWLERSTEPDETAILS_H__DA230F58_4BC9_4D35_B21C_83A4A99023AB__INCLUDED_)
