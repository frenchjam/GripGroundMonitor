// IddGripGroundDialog.cpp : implementation file
//

#include "stdafx.h"
#include "GripGroundMonitor.h"
#include "IddGripGroundDialog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// IddGripGroundDialog dialog


IddGripGroundDialog::IddGripGroundDialog(CWnd* pParent /*=NULL*/)
	: CDialog(IddGripGroundDialog::IDD, pParent)
{
	//{{AFX_DATA_INIT(IddGripGroundDialog)
	//}}AFX_DATA_INIT
}


void IddGripGroundDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(IddGripGroundDialog)
	DDX_Control(pDX, IDC_TIMESCALE, m_timescale);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(IddGripGroundDialog, CDialog)
	//{{AFX_MSG_MAP(IddGripGroundDialog)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// IddGripGroundDialog message handlers
