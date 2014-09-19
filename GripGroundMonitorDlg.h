// GripGroundMonitorDlg.h : header file
//

#if !defined(AFX_GRIPGROUNDMONITORDLG_H__B42FBB9C_FC18_44B0_A7F5_2EEA8245BBAB__INCLUDED_)
#define AFX_GRIPGROUNDMONITORDLG_H__B42FBB9C_FC18_44B0_A7F5_2EEA8245BBAB__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// Array dimensions for the GRIP script elements.
#define MAX_STEPS				4096
#define MAX_MENU_ITEMS			256
#define MAX_MENU_ITEM_LENGTH	256

// Max number of frames (data slices)
#define MAX_FRAMES (2*60*60*20)

// A convenient macro to hand message/picture pairs.
#define add_to_message_pair(x,y,z) { strcpy( message[lines], y ); if ( z ) strcpy( picture[lines], z ); else strcpy( picture[lines], "" ); type[lines] = type_alert; }

// Hold over from the good old C days.
#define TRUE	1
#define FALSE	0

/////////////////////////////////////////////////////////////////////////////
// CGripGroundMonitorDlg dialog

class CGripGroundMonitorDlg : public CDialog
{
// Construction
public:
	CGripGroundMonitorDlg(CWnd* pParent = NULL);	// standard constructor

	// Routines for parsing the GRIP scripts.

	void ParseTaskFile ( const char *filename );
	void ParseProtocolFile ( const char *filename );
	void ParseSessionFile ( const char *filename ) ;
	void ParseSubjectFile ( void );

	void Intialize2DGraphics();
	void Draw2DGraphics();

	// Data buffers.
	unsigned int	nFrames;
	static float	Time[MAX_FRAMES];
	static float	ManipulandumPosition[MAX_FRAMES][3];
	static float	LoadForce[MAX_FRAMES][3];
	static float	GripForce[MAX_FRAMES];
	static char		MarkerVisibility[MAX_FRAMES][20];

	double lowerPositionLimit;
	double upperPositionLimit;

	void ResetBuffers( void );
	void GraphManipulandumPosition( View view, int start_frame, int stop_frame );
	void PlotManipulandumPosition( int start_frame, int stop_frame );

	// Dialog Data
	//{{AFX_DATA(CGripGroundMonitorDlg)
	enum { IDD = IDD_GRIPGROUNDMONITOR_DIALOG };
	CStatic	m_zy;
	CStatic	m_xz;
	CStatic	m_cop;
	CStatic	m_xy;
	CStatic	m_stripcharts;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGripGroundMonitorDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;

	// Tables holding the items in the session, protocol and task menus.

	char session_file[MAX_MENU_ITEMS][MAX_MENU_ITEM_LENGTH];
	char protocol_file[MAX_MENU_ITEMS][MAX_MENU_ITEM_LENGTH];
	char task_file[MAX_MENU_ITEMS][MAX_MENU_ITEM_LENGTH];

	static char picture[MAX_STEPS][256];
	static char message[MAX_STEPS][132];
	static char *type[MAX_STEPS];
	static bool comment[MAX_STEPS];

	int  stepID[MAX_STEPS];
	int  taskID[MAX_STEPS];
	int  protocolID[MAX_STEPS];
	int  subjectID[MAX_STEPS];

	char PictureFilenamePrefix[1024];

	char *type_status;
	char *type_query;
	char *type_alert;


	// 2D Graphics

	Display zy_display, xy_display, xz_display, cop_display;
	View	zy_view, xy_view, xz_view, cop_view;

	// Put the displays and views into arrays so that we can do some stuff in loops.
	#define N_PHASEPLOTS	4
	Display phase_display[N_PHASEPLOTS];
	View	phase_view[N_PHASEPLOTS];

	Display stripchart_display;
	Layout	stripchart_layout;

	// Generated message map functions
	//{{AFX_MSG(CGripGroundMonitorDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnSelchangeSubjects();
	afx_msg void OnSelchangeTasks();
	afx_msg void OnSelchangeSteps();
	afx_msg void OnSelchangeProtocols();
	afx_msg void OnNextStep();
	afx_msg void OnDoubleclickedNextStep();
	afx_msg void OnGoto();
	afx_msg void OnDestroy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GRIPGROUNDMONITORDLG_H__B42FBB9C_FC18_44B0_A7F5_2EEA8245BBAB__INCLUDED_)
