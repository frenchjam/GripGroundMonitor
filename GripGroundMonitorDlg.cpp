// GripGroundMonitorDlg.cpp : implementation file
//

#include "stdafx.h"
#include "GripGroundMonitor.h"
#include "GripGroundMonitorDlg.h"

#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <share.h>
#include <sys\stat.h>
#include <conio.h>

#include "GripPackets.h"

// Defines that increase the amount of info in the memory leak report.
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#include <stdlib.h>

// Provide the access() function to check for file existance.
#include <io.h>

// Convenient printf-like messages.
#include "..\Useful\fMessageBox.h"
#include "..\Useful\fOutputDebugString.h"

// Some useful constants
#include "..\Useful\Useful.h"


// GRIP Script line parser.
#include "..\Useful\ParseCommaDelimitedLine.h"

// Define this constant to fill the buffer with the specified minutes of data.
#define FAKE_DATA 20


#define IDT_TIMER1 1001

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

int CGripGroundMonitorDlg::windowSpan[SPAN_VALUES] = { 20 * 60 * 60, 20 * 60 * 30, 20 * 60 * 10, 20 * 60 * 5, 20 * 60, 20 * 30 };

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
		// No message handlers
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGripGroundMonitorDlg dialog


CGripGroundMonitorDlg::CGripGroundMonitorDlg(CWnd* pParent, const char *packet_buffer_root, const char *script_directory, bool simulate )
	: CDialog(CGripGroundMonitorDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CGripGroundMonitorDlg)
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

	type_status = "Status (script will continue)";
	type_query =  "Query (waiting for response)";
	type_alert =  "Alert (seen only if error)";

	packetBufferPathRoot = packet_buffer_root;
	scriptDirectory = script_directory;
	simulateData = simulate;
	strncpy( PictureFilenamePrefix, scriptDirectory, sizeof( PictureFilenamePrefix ) );
	strncat( PictureFilenamePrefix, "pictures\\", sizeof( PictureFilenamePrefix ) );



}

void CGripGroundMonitorDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CGripGroundMonitorDlg)
	DDX_Control(pDX, IDC_TIMESCALE, m_timescale);
	DDX_Control(pDX, IDC_SCROLLBAR, m_scrollbar);
	DDX_Control(pDX, IDC_ZY, m_zy);
	DDX_Control(pDX, IDC_XZ, m_xz);
	DDX_Control(pDX, IDC_COP, m_cop);
	DDX_Control(pDX, IDC_XY, m_xy);
	DDX_Control(pDX, IDC_STRIPCHARTS, m_stripcharts);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CGripGroundMonitorDlg, CDialog)
	//{{AFX_MSG_MAP(CGripGroundMonitorDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_LBN_SELCHANGE(IDC_SUBJECTS, OnSelchangeSubjects)
	ON_LBN_SELCHANGE(IDC_TASKS, OnSelchangeTasks)
	ON_LBN_SELCHANGE(IDC_STEPS, OnSelchangeSteps)
	ON_LBN_SELCHANGE(IDC_PROTOCOLS, OnSelchangeProtocols)
	ON_BN_CLICKED(IDC_NEXT_STEP, OnNextStep)
	ON_BN_DOUBLECLICKED(IDC_NEXT_STEP, OnDoubleclickedNextStep)
	ON_BN_CLICKED(IDC_GOTO, OnGoto)
	ON_WM_DESTROY()
	ON_WM_TIMER()
	ON_WM_HSCROLL()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////

// Realtime operations.

// Max times to try to open the cache file before asking user to continue or not.
#define MAX_OPEN_CACHE_RETRIES	5
// Pause time in milliseconds between file open retries.
#define RETRY_PAUSE	2000		
// Error code to return if the cache file cannot be opened.
#define ERROR_CACHE_NOT_FOUND	-1000

#define SLICES_PER_PACKET	10

int CGripGroundMonitorDlg::GetLatestGripHK( int *subject, int *protocol, int *task, int *step ) {

	static int count = 0;

	int  fid;
	int packets_read = 0;
	int bytes_read;
	int return_code;
	static unsigned short previousTMCounter = 0;
	unsigned short bit = 0;
	int retry_count;
	int mb_answer;

	EPMTelemetryPacket packet;
	EPMTelemetryHeaderInfo epmHeader;
	GripHealthAndStatusInfo hk;

	struct {
		int user;
		int protocol;
		int task;
		int step;
	} non_zero = {0, 0, 0, 0};

	char filename[1024];

	CreateGripPacketCacheFilename( filename, sizeof( filename ), GRIP_HK_BULK_PACKET, packetBufferPathRoot );

	// Attempt to open the packet cache to read the accumulated packets.
	// If it is not immediately available, try for a few seconds then query the user.
	// The user can choose to continue to wait or cancel program execution.
	do {
		for ( retry_count = 0; retry_count  < MAX_OPEN_CACHE_RETRIES; retry_count ++ ) {
			// If open succeeds, it will return zero. So if zero return, break from retry loop.
			fid = _open( filename, _O_RDONLY | _O_BINARY, _SH_DENYNO, _S_IWRITE | _S_IREAD  );
			if ( fid != 0 ) break;
			// Wait a second before trying again.
			Sleep( RETRY_PAUSE );
		}
		// If fid is non-zero, file is open, so break out of loop and continue.
		if ( fid != 0 ) break;
		// If return_code is non-zero, we are here because the retry count has been reached without opening the file.
		// Ask the user if they want to keep on trying or abort.
		else {
			mb_answer = fMessageBox( MB_RETRYCANCEL, "GripMMI", "Error opening %s for binary read.\nContinue trying?", filename );
			if ( mb_answer == IDCANCEL ) exit( ERROR_CACHE_NOT_FOUND ); // User chose to abort.
		}
	} while ( true ); // Keep trying until success or until user cancels.

	// Read in all of the data packets in the file.
	packets_read = 0;
	while ( true ) {
		bytes_read = _read( fid, &packet, hkPacketLengthInBytes );
		// Return less than zero means read error.
		if ( bytes_read < 0 ) {
			fMessageBox( MB_OK, "GripMMI", "Error reading from %s.", filename );
			exit( -1 );
		}
		// Return less than expected number of bytes means we have read all packets.
		if ( bytes_read < hkPacketLengthInBytes ) break;

		packets_read++;
		// Check that it is a valid GRIP packet. It would be strange if it was not.
		ExtractEPMTelemetryHeaderInfo( &epmHeader, &packet );
		if ( epmHeader.epmSyncMarker != EPM_TELEMETRY_SYNC_VALUE || epmHeader.TMIdentifier != GRIP_HK_ID ) {
			fMessageBox( MB_OK, "GripMMIlite", "Unrecognized packet from %s.", filename );
			exit( -1 );
		}
		// When the subject exits a task, the task counter goes to zero, but the subject
		//  sees the menu item for the next task highlighted. So we try to guess what 
		//  the next step is going to be. To do that, we have to look for transitions to 0, 0;
		ExtractGripHealthAndStatusInfo( &hk, &packet );
		if ( hk.user != 0 ) non_zero.user = hk.user;
		if ( hk.protocol != 0 ) non_zero.protocol = hk.protocol;
		if ( hk.task != 0 ) non_zero.task = hk.task;

	}
	// Finished reading. Close the file and check for errors.
	return_code = _close( fid );
	if ( return_code ) {
		fMessageBox( MB_OK, "GripMMI", "Error closing %s after binary read.\nError code: %s", filename, return_code );
		exit( return_code );
	}

	ExtractGripHealthAndStatusInfo( &hk, &packet );
	*subject = ( hk.user == 0 ? non_zero.user : hk.user );
	*protocol = ( hk.protocol == 0 ? non_zero.protocol : hk.protocol );
	*task = ( hk.task == 0 ? non_zero.task + 1 : hk.task );
	*step = hk.step;


	// Check if there were new packets since the last time we read the cache.
	// Return TRUE if yes, FALSE if no.
	if ( previousTMCounter != epmHeader.TMCounter ) {
		previousTMCounter = epmHeader.TMCounter;
		return( TRUE );
	}
	else return ( FALSE );
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	
int CGripGroundMonitorDlg::GetGripRT( void ) {

	static int count = 0;

	int  fid;
	int packets_read = 0;
	int bytes_read;
	int return_code;
	static unsigned short previousTMCounter = 0;
	unsigned short bit = 0;
	int retry_count;
	int mb_answer;

	EPMTelemetryPacket		packet;
	EPMTelemetryHeaderInfo	epmHeader;
	GripRealtimeDataInfo	rt;

	int mrk, grp;

	char filename[1024];

	// Empty the data buffers, or more precisely, fill them with missing values.
	ResetBuffers();

	CreateGripPacketCacheFilename( filename, sizeof( filename ), GRIP_RT_SCIENCE_PACKET, packetBufferPathRoot );

	// Attempt to open the packet cache to read the accumulated packets.
	// If it is not immediately available, try for a few seconds then query the user.
	// The user can choose to continue to wait or cancel program execution.
	do {
		for ( retry_count = 0; retry_count  < MAX_OPEN_CACHE_RETRIES; retry_count ++ ) {
			// If open succeeds, it will return zero. So if zero return, break from retry loop.
			fid = _open( filename, _O_RDONLY | _O_BINARY, _SH_DENYNO, _S_IWRITE | _S_IREAD  );
			if ( fid != 0 ) break;
			// Wait a second before trying again.
			Sleep( RETRY_PAUSE );
		}
		// If fid is non-zero, file is open, so break out of loop and continue.
		if ( fid != 0 ) break;
		// If return_code is non-zero, we are here because the retry count has been reached without opening the file.
		// Ask the user if they want to keep on trying or abort.
		else {
			mb_answer = fMessageBox( MB_RETRYCANCEL, "GripMMI", "Error opening %s for binary read.\nContinue trying?", filename );
			if ( mb_answer == IDCANCEL ) exit( ERROR_CACHE_NOT_FOUND ); // User chose to abort.
		}
	} while ( true ); // Keep trying until success or until user cancels.

	// Read in all of the data packets in the file.
	packets_read = 0;
	while ( nFrames < MAX_FRAMES && rtPacketLengthInBytes == (bytes_read = _read( fid, &packet, rtPacketLengthInBytes )) ) {
		packets_read++;
		if ( bytes_read < 0 ) {
			fMessageBox( MB_OK, "GripMMI", "Error reading from %s.", filename );
			exit( -1 );
		}
		// Check that it is a valid GRIP packet. It would be strange if it was not.
		ExtractEPMTelemetryHeaderInfo( &epmHeader, &packet );
		if ( epmHeader.epmSyncMarker != EPM_TELEMETRY_SYNC_VALUE || epmHeader.TMIdentifier != GRIP_RT_ID ) {
			fMessageBox( MB_OK, "GripMMIlite", "Unrecognized packet from %s.", filename );
			exit( -1 );
		}
			
		// Transfer the data to the buffers for plotting.
		ExtractGripRealtimeDataInfo( &rt, &packet );
		for ( int slice = 0; slice < RT_SLICES_PER_PACKET && nFrames < MAX_FRAMES; slice++ ) {

			// Approximate the time, assuming 20 samples per second.
			// I know that this is not true. A future version will do
			//  something more clever to compute the real time of each data point.
			RealMarkerTime[nFrames] = nFrames * .05;
			if ( rt.dataSlice[slice].manipulandumVisibility ) {
				ManipulandumPosition[nFrames][X] = rt.dataSlice[slice].position[X] / 10.0;
				ManipulandumPosition[nFrames][Y] = rt.dataSlice[slice].position[Y] / 10.0;
				ManipulandumPosition[nFrames][Z] = rt.dataSlice[slice].position[Z] / 10.0;
				FilterManipulandumPosition( ManipulandumPosition[nFrames] );
			}
			else {
				ManipulandumPosition[nFrames][X] = MISSING_DOUBLE;
				ManipulandumPosition[nFrames][Y] = MISSING_DOUBLE;
				ManipulandumPosition[nFrames][Z] = MISSING_DOUBLE;
			}
			RealAnalogTime[nFrames] = nFrames * 0.05;
			
			// The ICD does not say what is the reference frame for the force data.
			// I've tried to get it right here, but I am not convinced.
			// Need to test this more thoroughly.
			GripForce[nFrames] = ComputeGripForce( rt.dataSlice[slice].ft[0].force, rt.dataSlice[slice].ft[1].force );
			GripForce[nFrames] = FilterGripForce( GripForce[nFrames] );
			ComputeLoadForce( LoadForce[nFrames], rt.dataSlice[slice].ft[0].force, rt.dataSlice[slice].ft[1].force );
			LoadForceMagnitude[nFrames] = FilterLoadForce( LoadForce[nFrames] );

			for ( int ati = 0; ati < N_FORCE_TRANSDUCERS; ati++ ) {
				double cop_distance = ComputeCoP( CenterOfPressure[ati][nFrames], rt.dataSlice[slice].ft[ati].force, rt.dataSlice[slice].ft[ati].torque );
				if ( cop_distance >= 0.0 ) FilterCoP( ati, CenterOfPressure[ati][nFrames] );
			}

			for ( mrk = 0, bit = 0x01; mrk < CODA_MARKERS; mrk++, bit = bit << 1 ) {

				grp = ( mrk >= 8 ? ( mrk >= 12 ? mrk + 20 : mrk + 10 ) : mrk ) + 35;
				if ( rt.dataSlice[slice].markerVisibility[0] & bit ) MarkerVisibility[nFrames][mrk] = grp;
				else MarkerVisibility[nFrames][mrk] = MISSING_CHAR;

			}
			if (  (rt.dataSlice[slice].manipulandumVisibility & 0x01) ) ManipulandumVisibility[nFrames] = 10;
			else ManipulandumVisibility[nFrames] = 0;

			Acceleration[nFrames][X] = (double) rt.dataSlice[slice].acceleration[X];
			Acceleration[nFrames][Y] = (double) rt.dataSlice[slice].acceleration[Y];
			Acceleration[nFrames][Z] = (double) rt.dataSlice[slice].acceleration[Z];

			nFrames++;
		}

	}
	// Finished reading. Close the file and check for errors.
	return_code = _close( fid );
	if ( return_code ) {
		fMessageBox( MB_OK, "GripMMI", "Error closing %s after binary read.\nError code: %s", filename, return_code );
		exit( return_code );
	}

	fOutputDebugString( "Acquired Frames (max %d): %d\n", MAX_FRAMES, nFrames );
	if ( nFrames >= MAX_FRAMES ) {
		fMessageBox( MB_OK | MB_ICONERROR, "GripMMI", "Internal buffers full." );
	}


	// Check if there were new packets since the last time we read the cache.
	// Return TRUE if yes, FALSE if no.
	if ( previousTMCounter != epmHeader.TMCounter ) {
		previousTMCounter = epmHeader.TMCounter;
		return( TRUE );
	}
	else return ( FALSE );
}

int CGripGroundMonitorDlg::SimulateGripRT ( void ) {

	// Simulate some data to test memory limites and graphics functions.
	// Each time through it adds more data to the buffers, so as to 
	//  simulate the progressive arrival of real-time data packets.

	int i, mrk;

	static int count = 0;
	count++;
	unsigned int fill_frames = 60 * 20 * count;
	for ( nFrames = 0; nFrames <= fill_frames && nFrames < MAX_FRAMES; nFrames++ ) {

		RealMarkerTime[nFrames] = (float) nFrames * 0.05;
		ManipulandumPosition[nFrames][X] = 30.0 * sin( RealMarkerTime[nFrames] * Pi * 2.0 / 30.0 );
		ManipulandumPosition[nFrames][Y] = 300.0 * cos( RealMarkerTime[nFrames] * Pi * 2.0 / 30.0 ) + 200.0;
		ManipulandumPosition[nFrames][Z] = -75.0 * sin( RealMarkerTime[nFrames] * Pi * 2.0 / 155.0 ) - 300.0;

		GripForce[nFrames] = fabs( -5.0 * sin( RealMarkerTime[nFrames] * Pi * 2.0 / 155.0 )  );
		for ( i = X; i <= Z; i++ ) {
			LoadForce[nFrames][i] = ManipulandumPosition[nFrames][ (i+2) % 3] / 200.0;
		}

		for ( mrk = 0; mrk <CODA_MARKERS; mrk++ ) {

			int grp = ( mrk >= 8 ? ( mrk >= 16 ? mrk + 20 : mrk + 10 ) : mrk ) + 35;
			if ( nFrames == 0 ) MarkerVisibility[nFrames][mrk] = grp;
			else {
				if ( MarkerVisibility[nFrames-1][mrk] != MISSING_CHAR ) {
					if ( rand() % 1000 < 1 ) MarkerVisibility[nFrames][mrk] = MISSING_CHAR;
					else MarkerVisibility[nFrames][mrk] = grp;
				}
				else {
					if ( rand() % 1000 < 1 ) MarkerVisibility[nFrames][mrk] = grp;
					else MarkerVisibility[nFrames][mrk] = MISSING_CHAR;
				}
			}
		}
			
		ManipulandumVisibility[nFrames] = 0;
		for ( mrk = MANIPULANDUM_FIRST_MARKER; mrk <= MANIPULANDUM_LAST_MARKER; mrk++ ) {
			if ( MarkerVisibility[nFrames][mrk] != MISSING_CHAR ) ManipulandumVisibility[nFrames]++;
		}
		if ( ManipulandumVisibility[nFrames] < 3 ) ManipulandumPosition[nFrames][X] = ManipulandumPosition[nFrames][Y] = ManipulandumPosition[nFrames][Z] = MISSING_FLOAT;
		ManipulandumVisibility[nFrames] *= 3;

	}

	fOutputDebugString( "nFrames: %d\n", nFrames );

	// Always return as if there were new packets available in the cache.
	// Maybe this should be changed to return FALSE if the buffers were already full.
	return( TRUE );

}

/////////////////////////////////////////////////////////////////////////////
// CGripGroundMonitorDlg message handlers

BOOL CGripGroundMonitorDlg::OnInitDialog()
{

	HBITMAP logobm = 0;
	char user_file_path[PATHLENGTH] = "users.dex";
	char logo_file_path[PATHLENGTH];

	CDialog::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
	
		// Shrink the font for the window showing the full current line in the script.
	HFONT hFontLine = CreateFont (12, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, "Arial");
	SendDlgItemMessage( IDC_SUBJECTS, WM_SETFONT, WPARAM (hFontLine), FALSE);
	SendDlgItemMessage( IDC_PROTOCOLS, WM_SETFONT, WPARAM (hFontLine), FALSE);
	SendDlgItemMessage( IDC_TASKS, WM_SETFONT, WPARAM (hFontLine), FALSE);
	SendDlgItemMessage( IDC_STEPS, WM_SETFONT, WPARAM (hFontLine), FALSE);

	// Reset the data buffers.
	ResetBuffers();

	// Starting from the root, load all the script items.
	strncpy( user_file_path, scriptDirectory, sizeof( user_file_path )) ;
	strncat( user_file_path, "users.dex", sizeof( user_file_path )) ;
	if ( 0 != ParseSubjectFile( user_file_path )) PostQuitMessage( 0 );
	SendDlgItemMessage( IDC_SUBJECTS, LB_SETCURSEL, 0, 0 );
	OnSelchangeSubjects();

	// Create the 2D graphics displays.
	Intialize2DGraphics();
	Draw2DGraphics();

	SendDlgItemMessage( IDC_TIMESCALE, TBM_SETRANGEMAX, TRUE, SPAN_VALUES - 1 );
	SendDlgItemMessage( IDC_TIMESCALE, TBM_SETRANGEMIN, TRUE, 0 );
	SendDlgItemMessage( IDC_LIVE, BM_SETCHECK, BST_CHECKED, 0 );
	SendDlgItemMessage( IDC_SCRIPTS_LIVE, BM_SETCHECK, BST_CHECKED, 0 );
	m_scrollbar.SetScrollRange( 0, 1000 );
	m_scrollbar.SetScrollPos( 1000 );

	strncpy( logo_file_path, PictureFilenamePrefix, sizeof( logo_file_path )) ;
	strncat( logo_file_path, "GripLogo.bmp", sizeof( logo_file_path )) ;
	logobm = (HBITMAP) LoadImage( NULL, logo_file_path, IMAGE_BITMAP, (int) (.45 * 540), (int) (.45 * 405), LR_CREATEDIBSECTION | LR_LOADFROMFILE | LR_VGACOLOR );
	SendDlgItemMessage( IDC_COP, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM) logobm );


	SetTimer( IDT_TIMER1, 500, NULL );


	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CGripGroundMonitorDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CGripGroundMonitorDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		// Tell all the subwindows to redraw themselves.
		// This causes the PsyPhy2dGraphics displays to redraw.
		SendMessageToDescendants( WM_PAINT, 0, 0, TRUE, FALSE );
		// Do what every a dialog window would usually do.
		CDialog::OnPaint();
	}
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CGripGroundMonitorDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

// Update to be performed when the selected user (subject) changes.
void CGripGroundMonitorDlg::OnSelchangeSubjects() 
{
	// Update when a different subject is selected.
	int subject = SendDlgItemMessage( IDC_SUBJECTS, LB_GETCURSEL, 0, 0 );
	ParseSessionFile( session_file[subject] );
	SendDlgItemMessage( IDC_PROTOCOLS, LB_SETCURSEL, 0, 0 );
	SetDlgItemInt( IDC_SUBJECTID, subjectID[subject] );
	OnSelchangeProtocols();	
}

// Update to be performed when the selected protocol changes.
void CGripGroundMonitorDlg::OnSelchangeProtocols() 

{
	
	int protocol = SendDlgItemMessage( IDC_PROTOCOLS, LB_GETCURSEL, 0, 0 );
	ParseProtocolFile( protocol_file[protocol] );
	SendDlgItemMessage( IDC_TASKS, LB_SETCURSEL, 0, 0 );
	SetDlgItemInt( IDC_PROTOCOLID, protocolID[protocol] );
	OnSelchangeTasks();
	
}

// Update to be performed when the selected task changes.
void CGripGroundMonitorDlg::OnSelchangeTasks() 
{
	int task = SendDlgItemMessage( IDC_TASKS, LB_GETCURSEL, 0, 0 );
	ParseTaskFile( task_file[task] );
	// Need to reset the step to the top if the task changes.
	SendDlgItemMessage( IDC_STEPS, LB_SETCURSEL, 0, 0 );
	SetDlgItemInt( IDC_TASKID, taskID[task] );
	OnSelchangeSteps();	
}

// Update to be performed when the selected step is changed.
void CGripGroundMonitorDlg::OnSelchangeSteps() 
{
	static HBITMAP bm = 0;

	char line[1024];
	int selected_line = SendDlgItemMessage( IDC_STEPS, LB_GETCURSEL, 0, 0 );

	// Center the selected line in the box.
	SendDlgItemMessage( IDC_STEPS, LB_SETCURSEL, selected_line + 14, 0 );
	SendDlgItemMessage( IDC_STEPS, LB_SETCURSEL, selected_line, 0 );

	// Show the full line in a larger box.
	SendDlgItemMessage( IDC_STEPS, LB_GETTEXT, selected_line, (LPARAM) line );
	SetDlgItemText( IDC_CURRENT_LINE, line );

	// Show what kind of display it is.
	SetDlgItemText( IDC_TYPE, type[selected_line] );

	// Show or hide buttons accordingly.
	if ( type[selected_line] == type_alert ) {
		GetDlgItem( IDOK )->ShowWindow( SW_HIDE );
		GetDlgItem( IDRETRY )->ShowWindow( SW_SHOW );
		GetDlgItem( IDIGNORE )->ShowWindow( SW_SHOW );
		GetDlgItem( IDCANCEL )->ShowWindow( SW_SHOW );
		GetDlgItem( IDINTERRUPT )->ShowWindow( SW_HIDE );
	}
	else if ( type[selected_line] == type_query ) {
		GetDlgItem( IDOK )->ShowWindow( SW_SHOW );
		GetDlgItem( IDRETRY )->ShowWindow( SW_HIDE );
		GetDlgItem( IDIGNORE )->ShowWindow( SW_HIDE );
		GetDlgItem( IDCANCEL )->ShowWindow( SW_SHOW );
		GetDlgItem( IDINTERRUPT )->ShowWindow( SW_HIDE );
	}
	else {
		GetDlgItem( IDOK )->ShowWindow( SW_HIDE );
		GetDlgItem( IDIGNORE )->ShowWindow( SW_HIDE );
		GetDlgItem( IDRETRY )->ShowWindow( SW_HIDE );
		GetDlgItem( IDCANCEL )->ShowWindow( SW_HIDE );
		GetDlgItem( IDINTERRUPT )->ShowWindow( SW_SHOW );
	}

	SetDlgItemInt( IDC_STEPID, stepID[selected_line] );

	// Convert \n escape sequence to newline. 
	for ( char *ptr =message[selected_line]; *ptr; ptr++ ) {
		if ( *ptr == '\\' && *(ptr+1) == 'n' ) {
			*ptr = '\r';
			*(ptr+1) = '\n';
		}
	}
	SetDlgItemText( IDC_STATUS_TEXT, message[selected_line] );

	// Show the picture.
	if ( strlen( picture[selected_line] ) ) {
		char picture_path[1024];
		strncpy( picture_path, PictureFilenamePrefix, sizeof( picture_path ) );
		strncat( picture_path, picture[selected_line], sizeof( picture_path ) );
		// If we had loaded a bitmap previously, free the associated memory.
		DeleteObject( bm );
		bm = (HBITMAP) LoadImage( NULL, picture_path, IMAGE_BITMAP, (int) (.5 * 540), (int) (.5 * 405), LR_CREATEDIBSECTION | LR_LOADFROMFILE | LR_VGACOLOR );
		SendDlgItemMessage( IDC_PICTURE, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM) bm );
	}	
}

// When the 'Next' button is pressed, skip to the next non-comment step.
void CGripGroundMonitorDlg::OnNextStep() 
{
	int selected_line = SendDlgItemMessage( IDC_STEPS, LB_GETCURSEL, 0, 0 );
	selected_line++;
	while ( comment[selected_line] ) selected_line++;
	SendDlgItemMessage( IDC_STEPS, LB_SETCURSEL, selected_line, 0 );
	OnSelchangeSteps();
	
}

//  When the 'Next' button is double clicked, skip to the next step that is a query or error message.
void CGripGroundMonitorDlg::OnDoubleclickedNextStep() 
{
	int selected_line = SendDlgItemMessage( IDC_STEPS, LB_GETCURSEL, 0, 0 );
	while ( comment[selected_line] || type[selected_line] == type_status ) selected_line++;
	SendDlgItemMessage( IDC_STEPS, LB_SETCURSEL, selected_line, 0 );
	OnSelchangeSteps();
	
}

// Position the script selections according to the user, protocol, task and step
//  values that are found in the 4 corresponding text boxes.
void CGripGroundMonitorDlg::OnGoto() 
{
	int i, current_selection;

	int subject = GetDlgItemInt( IDC_SUBJECTID );
	int protocol = GetDlgItemInt( IDC_PROTOCOLID );
	int task = GetDlgItemInt( IDC_TASKID );
	int step = GetDlgItemInt( IDC_STEPID );


	current_selection =  SendDlgItemMessage( IDC_SUBJECTS, LB_GETCURSEL, 0, 0 );
	for ( i = 0; i < MAX_STEPS - 1; i++ ) {
		if ( subjectID[i] == subject ) break;
	}
	if ( subjectID[i] != subject ) {
//		fMessageBox( MB_OK | MB_ICONERROR, "DexScriptCrawler", "Subject ID %3d not recognized.\nReverting to previous subject.", subject );
		SetDlgItemInt( IDC_SUBJECTID, subjectID[current_selection] );
	}
	else if ( i != current_selection ) {
		SendDlgItemMessage( IDC_SUBJECTS, LB_SETCURSEL, i, 0 );
		OnSelchangeSubjects();
	}

	current_selection =  SendDlgItemMessage( IDC_PROTOCOLS, LB_GETCURSEL, 0, 0 );
	for ( i = 0; i < MAX_STEPS - 1; i++ ) {
		if ( protocolID[i] == protocol ) break;
	}
	if ( protocolID[i] != protocol ) {
//		fMessageBox( MB_OK | MB_ICONERROR, "DexScriptCrawler", "Protocol ID %3d not recognized.\nReverting to previous protocol.", protocol );
		SetDlgItemInt( IDC_PROTOCOLID, protocolID[current_selection] );
	}
	else if ( i != current_selection ) {
		SendDlgItemMessage( IDC_PROTOCOLS, LB_SETCURSEL, i, 0 );
		OnSelchangeProtocols();
	}

	current_selection =  SendDlgItemMessage( IDC_TASKS, LB_GETCURSEL, 0, 0 );
	for ( i = 0; i < MAX_STEPS - 1; i++ ) {
		if ( taskID[i] == task ) break;
	}
	// Requesting task 0 say to move the selection to the next line.
	if ( task == 0 ) {
		SetDlgItemInt( IDC_TASKID, taskID[current_selection+1] );
		SendDlgItemMessage( IDC_TASKS, LB_SETCURSEL, current_selection+1, 0 );
		OnSelchangeTasks();
	}
	else if ( taskID[i] != task ) {
//		fMessageBox( MB_OK | MB_ICONERROR, "DexScriptCrawler", "Task ID %3d not recognized.", task );
		SetDlgItemInt( IDC_TASKID, taskID[current_selection] );
	}
	else if ( i != current_selection ) {
		SendDlgItemMessage( IDC_TASKS, LB_SETCURSEL, i, 0 );
		OnSelchangeTasks();
	}


	for ( i = 0; i < MAX_STEPS; i++ ) {
		if ( stepID[i] >= step ) break;
	}
	SendDlgItemMessage( IDC_STEPS, LB_SETCURSEL, i, 0 );
	OnSelchangeSteps();	
}

void CGripGroundMonitorDlg::OnDestroy() 
{
	// Do whatever a CDialog usually does.
	CDialog::OnDestroy();
	
	// Free the memory allocated by the PsyPhy2dGraphics system.
	DestroyViews();
	DestroyLayouts();
	DestroyOglDisplays();
	
}

void CGripGroundMonitorDlg::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
{
	// TODO: Add your message handler code here and/or call default
	
	if ( pScrollBar == &m_scrollbar ) {
		// If the user has touched the scroll bar, turn off live mode.
		SendDlgItemMessage( IDC_LIVE, BM_SETCHECK, BST_UNCHECKED, 0 );
		// Process the various scrollbar events.
		if ( nSBCode == SB_THUMBTRACK ) {
			m_scrollbar.SetScrollPos( nPos );
		}
		else if ( nSBCode == SB_THUMBPOSITION ) {
			m_scrollbar.SetScrollPos( nPos );
			Draw2DGraphics();
			PostMessage( WM_PAINT, 0, 0 );
		}
		else if ( nSBCode == SB_LINELEFT ) {
			// Shift the current window frame backward in time.
			int width_item = SendDlgItemMessage( IDC_TIMESCALE, TBM_GETPOS );
			int width = windowSpan[width_item];
			// The scrollbar runs from 0 to 1000. Here we compute the width of 
			//  the viewing window in scrollbar units and shoft to the left
			//  by one half of that distance.
			m_scrollbar.SetScrollPos( m_scrollbar.GetScrollPos() - width * 1000 / nFrames / 2 );
			Draw2DGraphics();
			PostMessage( WM_PAINT, 0, 0 );
		}
		else if ( nSBCode == SB_LINERIGHT ) {
			// Shift the current window frame forward in time.
			int width_item = SendDlgItemMessage( IDC_TIMESCALE, TBM_GETPOS );
			int width = windowSpan[width_item];
			// The scrollbar runs from 0 to 1000. Here we compute the width of 
			//  the viewing window in scrollbar units and shoft to the left
			//  by one half of that distance.
			m_scrollbar.SetScrollPos( m_scrollbar.GetScrollPos() + width * 1000 / nFrames / 2 );
			Draw2DGraphics();
			PostMessage( WM_PAINT, 0, 0 );
		}
		else if ( nSBCode == SB_PAGELEFT ) {
			// Shift so that we see only the first window of data.
			int width_item = SendDlgItemMessage( IDC_TIMESCALE, TBM_GETPOS );
			int width = windowSpan[width_item];
			// The 975 below means that we position the left edge of the
			//  window a little bit earlier in time. This shows some
			//  blank traces before the start of recording, making it 
			//  intuitively obvious that we are at the earliest part
			//  of the recorded data.
			m_scrollbar.SetScrollPos( width * 975 / nFrames );
			Draw2DGraphics();
			PostMessage( WM_PAINT, 0, 0 );
		}
		else if ( nSBCode == SB_PAGERIGHT ) {
			m_scrollbar.SetScrollPos( 1000 );
			Draw2DGraphics();
			PostMessage( WM_PAINT, 0, 0 );
		}


	}
	// I would like to check here if it is the slider that triggered the event,
	// but the slider is not a CScrollBar.
	// For the moment I just assume that if it is not the scroll bar it is the slider.
	else if ( /* pScrollBar. &m_timescale*/ true ) {
		// Process the various scrollbar events.
		if ( nSBCode == SB_THUMBTRACK ) {
			m_timescale.SetScrollPos( nPos, TRUE );
		}
		else if ( nSBCode == SB_THUMBPOSITION ) {
			m_timescale.SetScrollPos( nPos, TRUE );
			Draw2DGraphics();
			PostMessage( WM_PAINT, 0, 0 );
		}
		// On any other slider event just refresh the sceen.
		else {
			Draw2DGraphics();
			PostMessage( WM_PAINT, 0, 0 );
		}

	}
	else CDialog::OnHScroll( nSBCode, nPos, pScrollBar );

}

// A time gets set to go off relatively often.
// This is where most of the work is done to update with new data.
void CGripGroundMonitorDlg::OnTimer(UINT nIDEvent)
{

	bool new_data_available = false;
	bool live = false;
	bool refilter = false;

	long filtering;
	static long previous_filtering_setting = -1;

	// Kill the timer so that we don't get retriggered before we finish.
	// In other words, treat it like a one-shot timer.
	KillTimer( IDT_TIMER1 );
	
	if ( SendDlgItemMessage( IDC_SCRIPTS_LIVE, BM_GETCHECK, 0, 0 ) ) {
		// Get the latest hk packet info.
		int hk_subject, hk_protocol, hk_task, hk_step;
		int gui_subject, gui_protocol, gui_task, gui_step;
		GetLatestGripHK( &hk_subject, &hk_protocol, &hk_task, &hk_step );

		// Show the selected subject and protocol in the menus.
		gui_subject = GetDlgItemInt( IDC_SUBJECTID );
		if ( hk_subject != gui_subject ) SetDlgItemInt( IDC_SUBJECTID, hk_subject );
		gui_protocol = GetDlgItemInt( IDC_PROTOCOLID );
		if ( hk_protocol != gui_protocol ) SetDlgItemInt( IDC_PROTOCOLID, hk_protocol );
		gui_task = GetDlgItemInt( IDC_TASKID );
		if ( hk_task != gui_task ) SetDlgItemInt( IDC_TASKID, hk_task );
		gui_step = GetDlgItemInt( IDC_STEPID );
		if ( hk_step != gui_step )  SetDlgItemInt( IDC_STEPID, hk_step );
		// Update everything as if the subject, protocol, task and step had been entered by
		//  hand and then someone pushes the GoTo button.
		 OnGoto();
	}

	// If we are in live mode, get the latest real-time 
	// science data and add it to the buffers.
	if ( SendDlgItemMessage( IDC_LIVE, BM_GETCHECK, 0, 0 ) ) {
		// In Live mode we plot up to the latest data.
		// So position the scrollbar slider to reflect that.
		m_scrollbar.SetScrollPos( 1000 );
		live = true;
	}

	// If the filtering choice has changed,set the filter constant and reload.
	filtering = SendDlgItemMessage( IDC_FILTERING, BM_GETCHECK, 0, 0 );
	if ( filtering != previous_filtering_setting ) {
		if (  filtering == BST_CHECKED ) SetFilterConstant( 10.0 );
		else SetFilterConstant( 0.0 );
		previous_filtering_setting = filtering;
		refilter = true;
	}
	
	if ( live || refilter ) {

		// Get the data from the packet cache, or simulate it.
		if ( simulateData ) new_data_available = !( 0 == SimulateGripRT());
		else new_data_available = (bool) !( 0 == GetGripRT());

	}
	// If there is new data or if we are refiltering replot all of it.
	if ( new_data_available || refilter ) {
		Draw2DGraphics();
		PostMessage( WM_PAINT );
	}

	// Start timer again for next round.
	SetTimer( IDT_TIMER1, 200, NULL );

	CDialog::OnTimer(nIDEvent);
}

