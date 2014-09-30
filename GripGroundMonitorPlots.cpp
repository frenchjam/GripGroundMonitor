// GripGroundMonitorPlots.cpp : implementation file
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

// A convenient printf-like message box.
#include "..\Useful\fMessageBox.h"

// GRIP Script line parser.
#include "..\Useful\ParseCommaDelimitedLine.h"

// Define this constant to fill the buffer with the specified minutes of data.
#define FAKE_DATA 20

#define X 0
#define Y 1
#define Z 2

#define ROLL	0
#define PITCH	1
#define YAW		2

#define IDT_TIMER1 1001

#define MISSING_FLOAT	9999.9999
#define MISSING_CHAR	127

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

float CGripGroundMonitorDlg::ManipulandumOrientation[MAX_FRAMES][3];
float CGripGroundMonitorDlg::ManipulandumPosition[MAX_FRAMES][3];
float CGripGroundMonitorDlg::LoadForce[MAX_FRAMES][3];
float CGripGroundMonitorDlg::GripForce[MAX_FRAMES];
float CGripGroundMonitorDlg::Time[MAX_FRAMES];
char  CGripGroundMonitorDlg::MarkerVisibility[MAX_FRAMES][CODA_MARKERS];
char  CGripGroundMonitorDlg::ManipulandumVisibility[MAX_FRAMES];


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
	stripchart_layout = CreateLayout( stripchart_display, STRIPCHARTS, 1 );
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

}

void CGripGroundMonitorDlg::GraphManipulandumPosition( View view, int start_frame, int stop_frame ) {

	Display display = view->display;
	DisplayActivate( display );
	
	ViewColor( view, GREY6 );
	ViewBox( view );
	ViewTitle( view, "Manipulandum Position", INSIDE_LEFT, INSIDE_TOP, 0.0 );

	ViewSetXLimits( view, start_frame, stop_frame );
	ViewSetYLimits( view, lowerPositionLimit, upperPositionLimit );
	ViewAxes( view );

	// Plot all 3 components of the manipulandum position in the same view;
	for ( int i = 0; i < 3; i++ ) {
		ViewSelectColor( view, i );
		ViewPlotAvailableFloats( view, &ManipulandumPosition[0][i], start_frame, stop_frame, sizeof( *ManipulandumPosition ), MISSING_FLOAT );
	}

}

void CGripGroundMonitorDlg::GraphLoadForce( View view, int start_frame, int stop_frame ) {

	Display display = view->display;
	DisplayActivate( display );
	
	ViewColor( view, GREY6 );
	ViewBox( view );
	ViewTitle( view, "Load Force", INSIDE_LEFT, INSIDE_TOP, 0.0 );

	ViewSetXLimits( view, start_frame, stop_frame );
	ViewSetYLimits( view, lowerForceLimit, upperForceLimit );
	ViewAxes( view );

	// Plot all 3 components of the load force in the same view;
	for ( int i = 0; i < 3; i++ ) {
		ViewSelectColor( view, i );
		ViewPlotAvailableFloats( view, &LoadForce[0][i], start_frame, stop_frame, sizeof( *LoadForce ), MISSING_FLOAT );
	}

}

void CGripGroundMonitorDlg::GraphGripForce( View view, int start_frame, int stop_frame ) {

	Display display = view->display;
	DisplayActivate( display );
	
	ViewColor( view, GREY6 );
	ViewBox( view );
	ViewTitle( view, "Grip Force", INSIDE_LEFT, INSIDE_TOP, 0.0 );

	ViewSetXLimits( view, start_frame, stop_frame );
	ViewSetYLimits( view, lowerGripLimit, upperGripLimit );
	ViewAxes( view );
		
	ViewSelectColor( view, CYAN );
	ViewPlotAvailableFloats( view, &GripForce[0], start_frame, stop_frame, sizeof( *GripForce ), MISSING_FLOAT );

}

void CGripGroundMonitorDlg::PlotManipulandumPosition( int start_frame, int stop_frame ) {

	View view;

	// Define the pairs for each phase plot.
	static struct {
		int abscissa;
		int ordinate;
	} pair[3] = { {Z,Y}, {X,Y}, {X,Z} };

	// XY plot of Manipulandum position data.
	for ( int i = 0; i < 3; i++ ) {
		DisplayActivate( phase_display[i] );
		Erase( phase_display[i] );
		view = phase_view[i];
		ViewSetXLimits( view, lowerPositionLimit, upperPositionLimit );
		ViewSetYLimits( view, lowerPositionLimit, upperPositionLimit );
		ViewSelectColor( view, i );
		ViewXYPlotAvailableFloats( view, &ManipulandumPosition[0][pair[i].abscissa], &ManipulandumPosition[0][pair[i].ordinate], start_frame, stop_frame, sizeof( *ManipulandumPosition ), sizeof( *ManipulandumPosition ), MISSING_FLOAT );
	}

}

void CGripGroundMonitorDlg::GraphVisibility( View view, int start_frame, int stop_frame ) {

	Display display = view->display;
	DisplayActivate( display );
	
	ViewColor( view, GREY6 );
	ViewBox( view );
	ViewTitle( view, "Visibility", INSIDE_LEFT, INSIDE_TOP, 0.0 );

	ViewSetXLimits( view, start_frame, stop_frame );
	ViewSetYLimits( view, 0, 100 );
	ViewAxes( view );

	// Plot all the visibility traces in the same view;
	// Each marker is assigned a unique non-zero value when it is visible,
	//  such that the traces are spread out and grouped in the view.
	for ( int mrk = 0; mrk < CODA_MARKERS; mrk++ ) {
		ViewColor( view, mrk % 3 );
		ViewPlotAvailableChars( view, &MarkerVisibility[0][mrk], start_frame, stop_frame, sizeof( *MarkerVisibility ), MISSING_CHAR );
	}

	ViewColor( view, BLACK );
	ViewBoxPlotChars( view, &ManipulandumVisibility[0],  start_frame, stop_frame, sizeof( *ManipulandumVisibility ) );
	ViewColor( view, RED );
	ViewHorizontalLine( view, 9 );

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
	GraphGripForce( LayoutViewN( stripchart_layout, 1 ), 0, nFrames );
	GraphLoadForce( LayoutViewN( stripchart_layout, 2 ), 0, nFrames );
	GraphVisibility( LayoutViewN( stripchart_layout, 3 ), 0, nFrames );

	PlotManipulandumPosition( 0, nFrames );

}

/////////////////////////////////////////////////////////////////////////////

void CGripGroundMonitorDlg::ResetBuffers( void ) {
	nFrames = 0;

#ifdef FAKE_DATA

	// Simulate some data.

	int i, mrk;

	unsigned int frame;
	nFrames = FAKE_DATA * 60 * 20;
	for ( frame = 0; frame <= nFrames && frame < MAX_FRAMES; frame++ ) {

		Time[frame] = (float) frame * 0.05;
		ManipulandumPosition[frame][X] = 30.0 * sin( Time[frame] * Pi * 2.0 / 50.0 );
		ManipulandumPosition[frame][Y] = 300.0 * cos( Time[frame] * Pi * 2.0 / 75.0 ) + 200.0;
		ManipulandumPosition[frame][Z] = -75.0 * sin( Time[frame] * Pi * 2.0 / 155.0 ) - 300.0;

		GripForce[frame] = fabs( -5.0 * sin( Time[frame] * Pi * 2.0 / 155.0 )  );
		for ( i = X; i <= Z; i++ ) {
			LoadForce[frame][i] = ManipulandumPosition[frame][ (i+2) % 3] / 200.0;
		}

		for ( mrk = 0; mrk <CODA_MARKERS; mrk++ ) {

			int grp = ( mrk >= 8 ? ( mrk >= 16 ? mrk + 20 : mrk + 10 ) : mrk ) + 35;
			if ( frame == 0 ) MarkerVisibility[frame][mrk] = grp;
			else {
				if ( MarkerVisibility[frame-1][mrk] != MISSING_CHAR ) {
					if ( rand() % 1000 < 1 ) MarkerVisibility[frame][mrk] = MISSING_CHAR;
					else MarkerVisibility[frame][mrk] = grp;
				}
				else {
					if ( rand() % 1000 < 1 ) MarkerVisibility[frame][mrk] = grp;
					else MarkerVisibility[frame][mrk] = MISSING_CHAR;
				}
			}
		}
			
		ManipulandumVisibility[frame] = 0;
		for ( mrk = MANIPULANDUM_FIRST_MARKER; mrk <= MANIPULANDUM_LAST_MARKER; mrk++ ) {
			if ( MarkerVisibility[frame][mrk] != MISSING_CHAR ) ManipulandumVisibility[frame]++;
		}
		if ( ManipulandumVisibility[frame] < 3 ) ManipulandumPosition[frame][X] = ManipulandumPosition[frame][Y] = ManipulandumPosition[frame][Z] = MISSING_FLOAT;
		ManipulandumVisibility[frame] *= 3;

	}

#endif


}
