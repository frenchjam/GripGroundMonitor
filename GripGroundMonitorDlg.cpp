// GripGroundMonitorDlg.cpp : implementation file
//

#include "stdafx.h"
#include "GripGroundMonitor.h"
#include "GripGroundMonitorDlg.h"

// Defines that increase the amount of info in the memory leak report.
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#include <stdlib.h>

// Provide the access() function to check for file existance.
#include <io.h>

// A convenient printf-like message box.
#include "fMessageBox.h"

// GRIP Script line parser.
#include "DexInterpreterFunctions.h"

#define X 0
#define Y 1
#define Z 2

#define MISSING_VALUE	9999.9999

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

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

char CGripGroundMonitorDlg::picture[MAX_STEPS][256];
char CGripGroundMonitorDlg::message[MAX_STEPS][132];
char *CGripGroundMonitorDlg::type[MAX_STEPS];
bool CGripGroundMonitorDlg::comment[MAX_STEPS];

float CGripGroundMonitorDlg::ManipulandumPosition[MAX_FRAMES][3];
float CGripGroundMonitorDlg::Time[MAX_FRAMES];


CGripGroundMonitorDlg::CGripGroundMonitorDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CGripGroundMonitorDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CGripGroundMonitorDlg)
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

	strcpy( PictureFilenamePrefix, "Z:\\SoftwareDevelopement\\DexterousManipulation\\DexPictures\\" );

	type_status = "Status (script will continue)";
	type_query =  "Query  (waiting for response)";
	type_alert =  "Alert  (displayed only if error)";

	lowerPositionLimit = -500.0;
	upperPositionLimit =  750.0;

}

void CGripGroundMonitorDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CGripGroundMonitorDlg)
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
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////

// Initialize the objects used to generate graphs on the display.

void CGripGroundMonitorDlg::Intialize2DGraphics() {

	HWND parent;
	RECT rect;

	// The Display will draw into a defined subwindow. 
	// All this code creates the link between the Display object
	//  and the corresponding subwindow, setting the size of the 
	//  Display object to that of the window.
	// More of this should be done inside the routine that creates
	//  the Display, but for historical purposes it is done here.
	parent = m_stripcharts.GetSafeHwnd();
	m_stripcharts.GetClientRect( &rect );
   	stripchart_display = CreateOglDisplay();
	SetOglWindowParent( parent );
	DisplaySetSizePixels( stripchart_display, rect.right, rect.bottom );
	DisplaySetScreenPosition( stripchart_display, rect.left, rect.top );
	DisplayInit( stripchart_display );
	
	// Create an array of Views that will be used to plot data in stripchart form.
	stripchart_layout = CreateLayout( stripchart_display, 5, 1 );
	LayoutSetDisplayEdgesRelative( stripchart_layout, 0.01, 0.01, 0.99, 0.99 );

	// Create a Display and View for each of the phase plot subwindows:
	//  ZY, XY, XZ and COP (center-of-pressure)
	// Note that each instance of Display and View has a unique name, but they are also
	//  stored in lists so that they can be processed by for loops.

	// First is ZY (sagittal plane)
	parent = m_zy.GetSafeHwnd();
	m_zy.GetClientRect( &rect );
	zy_display = phase_display[0] = CreateOglDisplay();
	SetOglWindowParent(  parent );
	DisplaySetSizePixels( zy_display, rect.right, rect.bottom );
	DisplaySetScreenPosition( zy_display, rect.left, rect.top );
	DisplayInit( zy_display );

	zy_view =  phase_view[0] = CreateView( zy_display );
	ViewSetDisplayEdgesRelative( zy_view, 0.01, 0.01, 0.99, 0.99 );
	ViewSetEdges( zy_view, 0, 0, 1, 1 );
	ViewMakeSquare(zy_view);

	// Repeat for XY (frontal plane)
	parent = m_xy.GetSafeHwnd();
	m_xy.GetClientRect( &rect );
	xy_display = phase_display[1] = CreateOglDisplay();
	SetOglWindowParent(  parent );
	DisplaySetSizePixels( xy_display, rect.right, rect.bottom );
	DisplaySetScreenPosition( xy_display, rect.left, rect.top );
	DisplayInit( xy_display );

	xy_view =  phase_view[1] = CreateView( xy_display );
	ViewSetDisplayEdgesRelative( xy_view, 0.01, 0.01, 0.99, 0.99 );
	ViewSetEdges( xy_view, 0, 0, 1, 1 );
	ViewMakeSquare(xy_view);

	// Repeat for XZ (coronal plane)
	parent = m_xz.GetSafeHwnd();
	m_xz.GetClientRect( &rect );
	xz_display = phase_display[2] = CreateOglDisplay();
	SetOglWindowParent(  parent );
	DisplaySetSizePixels( xz_display, rect.right, rect.bottom );
	DisplaySetScreenPosition( xz_display, rect.left, rect.top );
	DisplayInit( xz_display );

	xz_view =  phase_view[2] = CreateView( xz_display );
	ViewSetDisplayEdgesRelative( xz_view, 0.01, 0.01, 0.99, 0.99 );
	ViewSetEdges( xz_view, 0, 0, 1, 1 );
	ViewMakeSquare(xz_view);

	// Repeat for COP (center-of-pressure)
	parent = m_cop.GetSafeHwnd();
	m_cop.GetClientRect( &rect );
	cop_display = phase_display[3] = CreateOglDisplay();
	SetOglWindowParent(  parent );
	DisplaySetSizePixels( cop_display, rect.right, rect.bottom );
	DisplaySetScreenPosition( cop_display, rect.left, rect.top );
	DisplayInit( cop_display );

	cop_view = phase_view[3] = CreateView( cop_display );
	ViewSetDisplayEdgesRelative( cop_view, 0.01, 0.01, 0.99, 0.99 );
	ViewSetEdges( cop_view, 0, 0, 1, 1 );
	ViewMakeSquare(cop_view);

//	DisplayDisableCache( stripchart_display );
//	for ( int i = 0; i < N_PHASEPLOTS; i++ ) DisplayDisableCache( phase_display[i] );

	

}

void CGripGroundMonitorDlg::GraphManipulandumPosition( View view, int start_frame, int stop_frame ) {

	DisplayActivate( stripchart_display );
	ViewErase( view );
	ViewSetXLimits( view, start_frame, stop_frame );
	ViewSetYLimits( view, lowerPositionLimit, upperPositionLimit );

	for ( int i = 0; i < 3; i++ ) {
		ViewSelectColor( view, i );
		ViewPlotAvailableFloats( view, &ManipulandumPosition[0][i], start_frame, stop_frame, sizeof( *ManipulandumPosition ), MISSING_VALUE );
	}

}

void CGripGroundMonitorDlg::PlotManipulandumPosition( int start_frame, int stop_frame ) {

	View view;

	static struct {
		int abscissa;
		int ordinate;
	} pair[3] = { {Z,Y}, {X,Y}, {X,Z} };

	for ( int i = 0; i < 3; i++ ) {
		DisplayActivate( phase_display[i] );
		Erase( phase_display[i] );
		view = phase_view[i];
		ViewSetXLimits( view, lowerPositionLimit, upperPositionLimit );
		ViewSetYLimits( view, lowerPositionLimit, upperPositionLimit );
		ViewSelectColor( view, i );
		ViewXYPlotAvailableFloats( view, &ManipulandumPosition[0][pair[i].abscissa], &ManipulandumPosition[0][pair[i].ordinate], start_frame, stop_frame, sizeof( *ManipulandumPosition ), sizeof( *ManipulandumPosition ), MISSING_VALUE );
	}

}


void CGripGroundMonitorDlg::Draw2DGraphics() {

	View view;
	Display display;
	int i;

	DisplayActivate( stripchart_display );
	Erase( stripchart_display );
	Color( stripchart_display, GREY4 );

	for ( i = 0; i < LayoutViews( stripchart_layout ); i++ ) {

		view = LayoutViewN( stripchart_layout, i );
		ViewBox( view );

	}

	for ( i = 0; i < N_PHASEPLOTS; i++ ) {

		display = phase_display[i];
		view = phase_view[i];
		DisplayActivate( display );
		Erase( display );
		ViewSelectColor( view, i );
		ViewBox( view );

	}

	GraphManipulandumPosition( LayoutViewN( stripchart_layout, 0 ), 0, nFrames );
	PlotManipulandumPosition( 0, nFrames );

}
/////////////////////////////////////////////////////////////////////////////

// Routines to walk the script tree.

enum { NORMAL_EXIT = 0, NO_USER_FILE, NO_LOG_FILE, ERROR_EXIT };
#define MAX_TOKENS	32

void CGripGroundMonitorDlg::ParseTaskFile ( const char *filename ) {

	FILE *fp;

	int tokens;
	char *token[MAX_TOKENS];
	char line[2048];
	int line_n = 0;

	int  lines = 0;
	int  current_step = 0;

	char msg[1024];

	char status_picture[256] = "blank.bmp";
	char status_message[256] = "";

	fp = fopen( filename, "r" );
	if ( !fp ) {
		// Signal the error.
		sprintf( msg, "Error opening task file %s for read.\n", filename );
		MessageBox( msg, "DexScriptRunner", MB_OK | MB_ICONERROR );
		return;
	}

	SendDlgItemMessage( IDC_STEPS, LB_RESETCONTENT, 0, 0 );
	while ( fgets( line, sizeof( line ), fp ) ) {

		line[strlen( line ) - 1] = 0;
		SendDlgItemMessage( IDC_STEPS, LB_ADDSTRING, 0, (LPARAM) line );
		if ( strlen( line ) ) tokens = ParseCommaDelimitedLine( token, line );

		if ( tokens ) {

			if ( !strcmp( token[0], "CMD_LOG_MESSAGE" ) ) {
				int value, items;
				if ( !strcmp( token[1], "logmsg" ) ) value = 0;
				else if ( !strcmp( token[1], "usermsg" ) ) value = 1;
				else {
					items = sscanf( token[1], "%d", &value );
					value = items && value;
				}
				if ( value != 0 ) {
					if ( token[2] ) strcpy( status_message, token[2] );
					else strcpy( status_message, "" );
				}
			}
			else if ( !strcmp( token[0], "CMD_SET_PICTURE" ) ) {
				if ( tokens > 1 ) strcpy( status_picture, token[1] );
				else strcpy( status_picture, "blank.bmp" );
			}


			if ( !strcmp( token[0], "CMD_WAIT_SUBJ_READY" ) ) {
				strcpy( message[lines], token[1] );
				strcpy( picture[lines], token[2] );
				type[lines] = type_query;
			}

			else if ( !strcmp( token[0], "CMD_WAIT_MANIP_ATTARGET" ) ) add_to_message_pair( &alert, token[13], token[14] )		
			else if ( !strcmp( token[0], "CMD_WAIT_MANIP_GRIP" ) ) add_to_message_pair( &alert, token[4], token[5] )
			else if ( !strcmp( token[0], "CMD_WAIT_MANIP_GRIPFORCE" ) ) add_to_message_pair( &alert, token[11], token[12] )
			else if ( !strcmp( token[0], "CMD_WAIT_MANIP_SLIP" ) ) add_to_message_pair( &alert, token[11], token[12] )
			else if ( !strcmp( token[0], "CMD_CHK_MASS_SELECTION" ) ) add_to_message_pair( &alert, "Put mass in cradle X and pick up mass from cradle Y.", "TakeMass.bmp" )
			else if ( !strcmp( token[0], "CMD_CHK_HW_CONFIG" ) ) add_to_message_pair( &alert, token[1], token[2] )
			else if ( !strcmp( token[0], "CMD_ALIGN_CODA" ) ) add_to_message_pair( &alert, token[1], token[2] )
			else if ( !strcmp( token[0], "CMD_CHK_CODA_ALIGNMENT" ) ) add_to_message_pair( &alert, token[4], token[5] )
			else if ( !strcmp( token[0], "CMD_CHK_CODA_FIELDOFVIEW" ) ) add_to_message_pair( &alert, token[9], token[10] )
			else if ( !strcmp( token[0], "CMD_CHK_CODA_PLACEMENT" ) ) add_to_message_pair( &alert, token[11], token[12] )
			else if ( !strcmp( token[0], "CMD_CHK_MOVEMENTS_AMPL" ) ) add_to_message_pair( &alert, token[6], token[7] )
			else if ( !strcmp( token[0], "CMD_CHK_MOVEMENTS_CYCLES" ) ) add_to_message_pair( &alert, token[7], token[8] )
			else if ( !strcmp( token[0], "CMD_CHK_START_POS" ) ) add_to_message_pair( &alert, token[7], token[8] )
			else if ( !strcmp( token[0], "CMD_CHK_MOVEMENTS_DIR" ) ) add_to_message_pair( &alert, token[6], token[7] )
			else if ( !strcmp( token[0], "CMD_CHK_COLLISIONFORCE" ) ) add_to_message_pair( &alert, token[4], token[5] )

			else {
				strcpy( message[lines], status_message );
				strcpy( picture[lines], status_picture );
				type[lines] = type_status;
			}
			comment[lines] = false;
			current_step++;
			stepID[lines] = current_step;
		}
		else {
			strcpy( message[lines], status_message );
			strcpy( picture[lines], status_picture );
			type[lines] = type_status;
			comment[lines] = true;
			stepID[lines] = current_step;
		}
		
		lines++;

	}
	
	fclose( fp );

	SendDlgItemMessage( IDC_STEPS, LB_ADDSTRING, 0, (LPARAM) "************ End of Script ************" );
	strcpy( message[lines], "*********** End of Script ***********\n*********** End of Script ***********\n*********** End of Script ***********\n*********** End of Script ***********" );
	strcpy( picture[lines], "blank.bmp" );
	type[lines] = type_status;
	comment[lines] = true;
	stepID[lines] = 9999;



}

void CGripGroundMonitorDlg::ParseProtocolFile ( const char *filename ) {

	FILE *fp;

	int tokens;
	char *token[MAX_TOKENS];
	char line[2048];
	int line_n = 0;

	int  tasks = 0;
	char task_filename[1024];

	char msg[1024];

	fp = fopen( filename, "r" );
	if ( !fp ) {
		// Signal the error.
		sprintf( msg, "Error opening protocol file %s for read.\n", filename );
		MessageBox( msg, "DexScriptRunner", MB_OK | MB_ICONERROR );
		return;
	}

	// Work our way through the session file line-by-line.
	SendDlgItemMessage( IDC_TASKS, LB_RESETCONTENT, 0, 0 );
	while ( fgets( line, sizeof( line ), fp ) ) {

		line_n++;
		tokens = ParseCommaDelimitedLine( token, line );

		// Lines in protocol files are divided into 4 fields.
		if ( tokens == 4 ) {
			// First parameter must be CMD_TASK.
			if ( strcmp( token[0], "CMD_TASK" ) ) {
				sprintf( msg, "%s Line %03d Command not CMD_TASK: %s\n", filename, line_n, token[0] );
				MessageBox( msg, "DexScriptRunner", MB_OK | MB_ICONERROR );
			}
			// Second parameter is a task ID. Should be an integer value.
			if ( 1 != sscanf( token[1], "%d", &taskID[tasks] ) ) {
				sprintf( msg, "%s Line %03d Error reading task ID: %s\n", filename, line_n, token[1] );
				MessageBox( msg, "DexScriptRunner", MB_OK | MB_ICONERROR );
				exit( - 1 );
			}
			// The third item is the name of the protocol file.
			strcpy( task_filename, token[2] );
			// Check if it is present and readable, unless told to ignore it.
			if ( strcmp( task_filename, "ignore" ) ) {
				if ( _access( task_filename, 0x00 ) ) {
					sprintf( msg, "%s Line %03d Cannot access protocol file: %s\n", filename, line_n, task_filename );
					MessageBox( msg, "DexScriptRunner", MB_OK | MB_ICONERROR );
				}
				else {
					// Add the filename to the list of protocols.
					strcpy( task_file[tasks], task_filename );
					SendDlgItemMessage( IDC_TASKS, LB_ADDSTRING, 0, (LPARAM) token[3] );
					tasks++;
				}
			}

		}
		else if ( tokens != 0 ) {
			sprintf( msg, "%s Line %03d Wrong number of parameters: %s\n", filename, line_n, line );
			MessageBox( msg, "DexScriptRunner", MB_OK | MB_ICONERROR );
			return;
		}

	}

	fclose( fp );

}

void CGripGroundMonitorDlg::ParseSessionFile ( const char *filename ) {

	FILE *fp;

	int tokens;
	char *token[MAX_TOKENS];
	char line[2048];
	int line_n = 0;

	int protocols = 0;
	char protocol_filename[1024];

	char msg[1024];

	fp = fopen( filename, "r" );
	if ( !fp ) {
		// Signal the error.
		sprintf( msg, "Error opening session file %s for read.\n", filename );
		MessageBox( msg, "DexScriptRunner", MB_OK | MB_ICONERROR );
		return;
	}

	// Work our way through the session file line-by-line.
	SendDlgItemMessage( IDC_PROTOCOLS, LB_RESETCONTENT, 0, 0 );
	while ( fgets( line, sizeof( line ), fp ) ) {

		line_n++;
		tokens = ParseCommaDelimitedLine( token, line );

		// Lines in session files are divided into 4 fields.
		if ( tokens == 4 ) {
			// First parameter must be CMD_PROTOCOL.
			if ( strcmp( token[0], "CMD_PROTOCOL" ) ) {
				sprintf( msg, "%s Line %03d Command not CMD_PROTOCOL: %s\n", filename, line_n, token[0] );
				MessageBox( msg, "DexScriptRunner", MB_OK | MB_ICONERROR );
			}
			// Second parameter is a protocol ID. Should be an integer value.
			if ( 1 != sscanf( token[1], "%d", &protocolID[protocols] ) ) {
				sprintf( msg, "%s Line %03d Error reading protocol ID: %s\n", filename, line_n, token[1] );
				MessageBox( msg, "DexScriptRunner", MB_OK | MB_ICONERROR );
				exit( - 1 );
			}
			// The third item is the name of the protocol file.
			strcpy( protocol_filename, token[2] );
			// Check if it is present and readable, unless told to ignore it.
			if ( strcmp( protocol_filename, "ignore" ) ) {
				if ( _access( protocol_filename, 0x00 ) ) {
					sprintf( msg, "%s Line %03d Cannot access protocol file: %s\n", filename, line_n, protocol_filename );
					MessageBox( msg, "DexScriptRunner", MB_OK | MB_ICONERROR );
				}
				else {
					// Add the filename to the list of protocols.
					strcpy( protocol_file[protocols], protocol_filename );
					SendDlgItemMessage( IDC_PROTOCOLS, LB_ADDSTRING, 0, (LPARAM) token[3] );
					protocols++;
				}
			}

		}
		else if ( tokens != 0 ) {
			sprintf( msg, "%s Line %03d Wrong number of parameters: %s\n", filename, line_n, line );
			MessageBox( msg, "DexScriptRunner", MB_OK | MB_ICONERROR );
			return;
		}

	}

	fclose( fp );

}


void CGripGroundMonitorDlg::ParseSubjectFile ( void ) {

	char user_file[1024] = "users.dex";
	char log_file[1024] = "DexLintLog.txt";

	FILE *fp;

	int tokens;
	char *token[MAX_TOKENS];
	char line[2048];
	int line_n = 0;

	char session_filename[256];
	int subjects = 0;

	// Open the root file, if we can.
	fp = fopen( user_file, "r" );
	if ( !fp ) {

		char msg[1024];
		sprintf( msg, "Error opening subject file %s for read.", user_file );
		printf( "%s\n", msg );
		MessageBox( msg, "DexScriptRunner", MB_OK | MB_ICONERROR );
		exit( NO_USER_FILE );
	}

	// Step through line by line and follow the links to the session files.
	SendDlgItemMessage( IDC_SUBJECTS, LB_RESETCONTENT, 0, 0 );
	while ( fgets( line, sizeof( line ), fp ) ) {

		// Count the lines.
		line_n++;

		// Break the line into pieces as defined by the DEX/GRIP ICD.
		tokens = ParseCommaDelimitedLine( token, line );

		// Parse each line and do some syntax error checking.
		if ( tokens == 5 ) {

			// First parameter must be CMD_USER.
			if ( strcmp( token[0], "CMD_USER" ) ) {
				char msg[1024];
				sprintf( msg, "Line %03d Command not CMD_USER: %s\n", line_n, token[0] );
				MessageBox( msg, "DexScriptRunner", MB_OK | MB_ICONERROR );
				exit( ERROR_EXIT );
			}
			// Second parameter is a subject ID. Should be an integer value.
			// Verify also that subject IDs are unique.
			if ( 1 != sscanf( token[1], "%d", &subjectID[subjects] ) ) {
				char msg[1024];
				// Report error for invalid subject ID field.
				sprintf( msg, "Line %03d Error reading subject ID: %s\n", line_n, token[1] );
				MessageBox( msg, "DexScriptRunner", MB_OK | MB_ICONERROR );
				exit( ERROR_EXIT );
			}
			// The third parameter is the name of the session file.
			// Here we check that it is present and if so, we process it as well.
			strcpy( session_filename, token[3] );
			if ( _access( session_filename, 0x00 ) ) {
				char msg[1024];
				// The file must not only be present, it also has to be readable.
				// I had a funny problem with this at one point. Maybe MAC OS had created links.
				sprintf( msg, "Line %03d Cannot access session file: %s\n", line_n, session_filename );
				MessageBox( msg, "DexScriptRunner", MB_OK | MB_ICONERROR );
				exit( ERROR_EXIT );
			}	
			else {
			
				strcpy( session_file[subjects], session_filename );
				SendDlgItemMessage( IDC_SUBJECTS, LB_ADDSTRING, 0, (LPARAM) token[4] );
				subjects++;
			
			}
			// There is a fifth field to the line, which is text describing the subject.
			// For the moment I don't do any error checking on that parameter.
		}
		else if ( tokens != 0 ) {
			char msg[1024];
			sprintf( msg, "%s Line %03d Wrong number of parameters: %s\n", user_file, line_n, line );
			MessageBox( msg, "DexScriptRunner", MB_OK | MB_ICONERROR );
			exit( ERROR_EXIT );
		}			
	}

	fclose( fp );

}



/////////////////////////////////////////////////////////////////////////////

void CGripGroundMonitorDlg::ResetBuffers( void ) {
	nFrames = 0;

	// Simulate some data.
	unsigned int frame;
	nFrames = 60 * 20;
	for ( frame = 0; frame <= nFrames && frame < MAX_FRAMES; frame++ ) {

		Time[frame] = (float) frame * 0.05;
		ManipulandumPosition[frame][X] = 30.0 * sin( Time[frame] * Pi * 2.0 / 5.0 );
		ManipulandumPosition[frame][Y] = 300.0 * cos( Time[frame] * Pi * 2.0 / 5.0 ) + 200.0;
		ManipulandumPosition[frame][Z] = -75.0 * sin( Time[frame] * Pi * 2.0 / 5.0 ) - 300.0;

	}


}

/////////////////////////////////////////////////////////////////////////////
// CGripGroundMonitorDlg message handlers

BOOL CGripGroundMonitorDlg::OnInitDialog()
{
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
	
	// Reset the data buffers.
	ResetBuffers();

	// Create the 2D graphics displays.
	Intialize2DGraphics();
	Draw2DGraphics();

	// Starting from the root, load all the script items.
	ParseSubjectFile();
	SendDlgItemMessage( IDC_SUBJECTS, LB_SETCURSEL, 0, 0 );
	OnSelchangeSubjects();


	
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
		CDialog::OnPaint();
	}
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CGripGroundMonitorDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

void CGripGroundMonitorDlg::OnSelchangeSubjects() 
{
	// Update when a different subject is selected.
	int subject = SendDlgItemMessage( IDC_SUBJECTS, LB_GETCURSEL, 0, 0 );
	ParseSessionFile( session_file[subject] );
	SendDlgItemMessage( IDC_PROTOCOLS, LB_SETCURSEL, 0, 0 );
	SetDlgItemInt( IDC_SUBJECTID, subjectID[subject] );
	OnSelchangeProtocols();	
}

void CGripGroundMonitorDlg::OnSelchangeTasks() 
{
	// Update when the selected task changes.
	int task = SendDlgItemMessage( IDC_TASKS, LB_GETCURSEL, 0, 0 );
	ParseTaskFile( task_file[task] );
	// Need to reset the step to the top if the task changes.
	SendDlgItemMessage( IDC_STEPS, LB_SETCURSEL, 0, 0 );
	SetDlgItemInt( IDC_TASKID, taskID[task] );
	OnSelchangeSteps();	
}

void CGripGroundMonitorDlg::OnSelchangeSteps() 
{
	HBITMAP bm;

	// TODO: Add your control notification handler code here
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
		strcpy( picture_path, PictureFilenamePrefix );
		strcat( picture_path, picture[selected_line] );
		bm = (HBITMAP) LoadImage( NULL, picture_path, IMAGE_BITMAP, (int) (.5 * 540), (int) (.5 * 405), LR_CREATEDIBSECTION | LR_LOADFROMFILE | LR_VGACOLOR );
		SendDlgItemMessage( IDC_PICTURE, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM) bm );
	}	
}

void CGripGroundMonitorDlg::OnSelchangeProtocols() 

{
	
	// Update when the selected protocol changes.
	int protocol = SendDlgItemMessage( IDC_PROTOCOLS, LB_GETCURSEL, 0, 0 );
	ParseProtocolFile( protocol_file[protocol] );
	SendDlgItemMessage( IDC_TASKS, LB_SETCURSEL, 0, 0 );
	SetDlgItemInt( IDC_PROTOCOLID, protocolID[protocol] );
	OnSelchangeTasks();
	
}

void CGripGroundMonitorDlg::OnNextStep() 
{
	// Skip to the next non-comment step.
	int selected_line = SendDlgItemMessage( IDC_STEPS, LB_GETCURSEL, 0, 0 );
	selected_line++;
	while ( comment[selected_line] ) selected_line++;
	SendDlgItemMessage( IDC_STEPS, LB_SETCURSEL, selected_line, 0 );
	OnSelchangeSteps();
	
}

void CGripGroundMonitorDlg::OnDoubleclickedNextStep() 
{
	// Skip to the next step that is a query or error message.
	int selected_line = SendDlgItemMessage( IDC_STEPS, LB_GETCURSEL, 0, 0 );
	while ( comment[selected_line] || type[selected_line] == type_status ) selected_line++;
	SendDlgItemMessage( IDC_STEPS, LB_SETCURSEL, selected_line, 0 );
	OnSelchangeSteps();
	
}

void CGripGroundMonitorDlg::OnGoto() 
{
	int i, current_selection;

	int subject = GetDlgItemInt( IDC_SUBJECTID );
	current_selection =  SendDlgItemMessage( IDC_SUBJECTS, LB_GETCURSEL, 0, 0 );
	for ( i = 0; i < MAX_STEPS - 1; i++ ) {
		if ( subjectID[i] == subject ) break;
	}
	if ( subjectID[i] != subject ) {
		fMessageBox( MB_OK | MB_ICONERROR, "DexScriptCrawler", "Subject ID %3d not recognized.", subject );
		SetDlgItemInt( IDC_SUBJECTID, subjectID[current_selection] );
	}
	else if ( i != current_selection ) {
		SendDlgItemMessage( IDC_SUBJECTS, LB_SETCURSEL, i, 0 );
		OnSelchangeSubjects();
	}


	int protocol = GetDlgItemInt( IDC_PROTOCOLID );
	current_selection =  SendDlgItemMessage( IDC_PROTOCOLS, LB_GETCURSEL, 0, 0 );
	for ( i = 0; i < MAX_STEPS - 1; i++ ) {
		if ( protocolID[i] == protocol ) break;
	}
	if ( protocolID[i] != protocol ) {
		fMessageBox( MB_OK | MB_ICONERROR, "DexScriptCrawler", "Protocol ID %3d not recognized.", protocol );
		SetDlgItemInt( IDC_PROTOCOLID, protocolID[current_selection] );
	}
	else if ( i != current_selection ) {
		SendDlgItemMessage( IDC_PROTOCOLS, LB_SETCURSEL, i, 0 );
		OnSelchangeProtocols();
	}


	int task = GetDlgItemInt( IDC_TASKID );
	current_selection =  SendDlgItemMessage( IDC_TASKS, LB_GETCURSEL, 0, 0 );
	for ( i = 0; i < MAX_STEPS - 1; i++ ) {
		if ( taskID[i] == task ) break;
	}
	if ( taskID[i] != task ) {
		fMessageBox( MB_OK | MB_ICONERROR, "DexScriptCrawler", "Task ID %3d not recognized.", task );
		SetDlgItemInt( IDC_TASKID, taskID[current_selection] );
	}
	else if ( i != current_selection ) {
		SendDlgItemMessage( IDC_TASKS, LB_SETCURSEL, i, 0 );
		OnSelchangeTasks();
	}


	int step = GetDlgItemInt( IDC_STEPID );
	for ( i = 0; i < MAX_STEPS; i++ ) {
		if ( stepID[i] >= step ) break;
	}
	SendDlgItemMessage( IDC_STEPS, LB_SETCURSEL, i, 0 );
	OnSelchangeSteps();	
}

void CGripGroundMonitorDlg::OnDestroy() 
{
	CDialog::OnDestroy();
	
	// TODO: Add your message handler code here
	DestroyViews();
	DestroyLayouts();
	DestroyOglDisplays();
	
}
