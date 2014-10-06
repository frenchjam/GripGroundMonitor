// GripGroundMonitorDlg.h : header file
//

#if !defined(AFX_GRIPGROUNDMONITORDLG_H__B42FBB9C_FC18_44B0_A7F5_2EEA8245BBAB__INCLUDED_)
#define AFX_GRIPGROUNDMONITORDLG_H__B42FBB9C_FC18_44B0_A7F5_2EEA8245BBAB__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "..\Useful\VectorsMixin.h"
#include "DexScriptCrawlerStepDetails.h"
#include "GripPackets.h"

// 2D Graphics Package
#include "useful.h"
#include "..\2dGraphics\2dGraphicsLib\OglDisplayInterface.h"
#include "..\2dGraphics\2dGraphicsLib\OglDisplay.h"
#include "..\2dGraphics\2dGraphicsLib\Views.h"
#include "..\2dGraphics\2dGraphicsLib\Layouts.h"
#include "..\2dGraphics\2dGraphicsLib\Displays.h" 

#include "..\Grip\DexAnalogMixin.h"

// Array dimensions for the GRIP script elements.
#define MAX_STEPS				4096
#define MAX_MENU_ITEMS			256
#define MAX_MENU_ITEM_LENGTH	1024

// Max number of frames (data slices)
#define MAX_FRAMES (4*60*60*20)
#define CODA_MARKERS 20
#define	CODA_UNITS	2
#define MANIPULANDUM_FIRST_MARKER 0
#define MANIPULANDUM_LAST_MARKER  7

#define STRIPCHARTS	6
#define SPAN_VALUES	6

// A convenient macro to hand message/picture pairs.
#define add_to_message_pair(x,y,z) { strcpy( message[lines], y ); if ( z ) strcpy( picture[lines], z ); else strcpy( picture[lines], "" ); type[lines] = type_alert; }

// Hold over from the good old C days.
#define TRUE	1
#define FALSE	0

/////////////////////////////////////////////////////////////////////////////
// CGripGroundMonitorDlg dialog

class CGripGroundMonitorDlg : public CDialog, DexAnalogMixin
{
// Construction
public:
	CGripGroundMonitorDlg(CWnd* pParent = NULL, const char *packet_buffer_root	 = ".\\", const char *script_path = ".\\", bool simulate = false );	// standard constructor

	// Routines for parsing the GRIP scripts.

	void ParseTaskFile ( const char *filename );
	void ParseProtocolFile ( const char *filename );
	void ParseSessionFile ( const char *filename ) ;
	int  ParseSubjectFile ( const char *filename );

	void Intialize2DGraphics();
	void Draw2DGraphics();
	void GoToSpecifiedSubject( int subject );
	void GoToSpecifiedProtocol( int protocol );
	void GoToSpecifiedTask( int task );
	void GoToSpecifiedStep( int step );

	void GoToSpecified( int subject, int protocol, int task, int step );

	// Data buffers.
	unsigned int	nFrames;
	static float	RealMarkerTime[MAX_FRAMES];
	static float	CompressedMarkerTime[MAX_FRAMES];
	static float	RealAnalogTime[MAX_FRAMES];
	static float	CompressedAnalogTime[MAX_FRAMES];
	static Vector3	ManipulandumPosition[MAX_FRAMES];
	static Vector3	ManipulandumRotations[MAX_FRAMES];
	static Vector3	LoadForce[MAX_FRAMES];
	static double	LoadForceMagnitude[MAX_FRAMES];
	static float	Acceleration[MAX_FRAMES][3];
	static float	GripForce[MAX_FRAMES];
	static Vector3	CenterOfPressure[2][MAX_FRAMES];
	static char		MarkerVisibility[MAX_FRAMES][CODA_MARKERS];
	static char		ManipulandumVisibility[MAX_FRAMES];

	double lowerPositionLimit;
	double upperPositionLimit;

	double lowerRotationLimit;
	double upperRotationLimit;

	double lowerPositionLimitSpecific[3];
	double upperPositionLimitSpecific[3];

	double lowerForceLimit;
	double upperForceLimit;
	double lowerGripLimit;
	double upperGripLimit;

	double lowerAccelerationLimit;
	double upperAccelerationLimit;

	double lowerCopLimit;
	double upperCopLimit;

	static int windowSpan[SPAN_VALUES];
	static char *massDecoder[4];
	static char *soundBar[16];

	const char *packetBufferPathRoot;
	const char *scriptDirectory;

	char markerVisibilityString[CODA_UNITS][32];

	void ResetBuffers( void );
	void GraphManipulandumPosition( View view, double start_instant, double stop_instant, int start_frame, int stop_frame );
	void GraphManipulandumRotations( View view, double start_instant, double stop_instant, int start_frame, int stop_frame );
	void PlotManipulandumPosition( double start_window, double stop_window, int start_frame, int stop_frame );
	void GraphLoadForce( View view, double start_instant, double stop_instant, int start_frame, int stop_frame ) ;
	void GraphAcceleration( View view, double start_instant, double stop_instant, int start_frame, int stop_frame ) ;
	void GraphGripForce( View view, double start_instant, double stop_instant, int start_frame, int stop_frame ) ;
	void GraphVisibility( View view, double start_instant, double stop_instant, int start_frame, int stop_frame ) ;
	void GraphCoP( View view, double start_instant, double stop_instant, int start_frame, int stop_frame );
	void PlotCoP( double start_window, double stop_window, int start_frame, int stop_frame );

	int GetLatestGripHK( GripHealthAndStatusInfo *hk );
	int GetGripRT( void );
	int SimulateGripRT ( void );

	bool simulateData;

	// Dialog Data
	//{{AFX_DATA(CGripGroundMonitorDlg)
	enum { IDD = IDD_GRIPGROUNDMONITOR_DIALOG };
	CSliderCtrl	m_timescale;
	CScrollBar	m_scrollbar;
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
	DexScriptCrawlerStepDetails *m_detailsPopup;

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
	View	visibility_view;

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
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnDblclkSteps();
	afx_msg void OnSetfocusSubjectid();
	afx_msg void OnSetfocusProtocolid();
	afx_msg void OnSetfocusTaskid();
	afx_msg void OnSetfocusStepid();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GRIPGROUNDMONITORDLG_H__B42FBB9C_FC18_44B0_A7F5_2EEA8245BBAB__INCLUDED_)
