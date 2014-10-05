// DexScriptCrawlerStepDetails.cpp : implementation file
//

#include "stdafx.h"
#include "GripGroundMonitor.h"
#include "DexScriptCrawlerStepDetails.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// DexScriptCrawlerStepDetails dialog


DexScriptCrawlerStepDetails::DexScriptCrawlerStepDetails(CWnd* pParent /*=NULL*/)
	: CDialog(DexScriptCrawlerStepDetails::IDD, pParent)
{
	//{{AFX_DATA_INIT(DexScriptCrawlerStepDetails)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void DexScriptCrawlerStepDetails::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(DexScriptCrawlerStepDetails)
	DDX_Control(pDX, IDC_CURRENT_LINE, m_linedetails);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(DexScriptCrawlerStepDetails, CDialog)
	//{{AFX_MSG_MAP(DexScriptCrawlerStepDetails)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// DexScriptCrawlerStepDetails message handlers

void DexScriptCrawlerStepDetails::OnOK() 
{
	CDialog::OnOK();
}

void DexScriptCrawlerStepDetails::DisplayLine( const char *line ) {
	SetDlgItemText( IDC_CURRENT_LINE, line );
}
