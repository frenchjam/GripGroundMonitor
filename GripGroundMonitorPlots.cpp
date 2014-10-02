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

// Convenient printf-like messages.
#include "..\Useful\fMessageBox.h"
#include "..\Useful\fOutputDebugString.h"

// GRIP Script line parser.
#include "..\Useful\ParseCommaDelimitedLine.h"

// 2D Graphics Package
#include "useful.h"
#include "..\2dGraphics\2dGraphicsLib\OglDisplayInterface.h"
#include "..\2dGraphics\2dGraphicsLib\OglDisplay.h"
#include "..\2dGraphics\2dGraphicsLib\Views.h"
#include "..\2dGraphics\2dGraphicsLib\Layouts.h"
#include "..\2dGraphics\2dGraphicsLib\Displays.h" 

// Define this constant to fill the buffer with simulated data.
// #define FAKE_DATA 

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
float CGripGroundMonitorDlg::Acceleration[MAX_FRAMES][3];
float CGripGroundMonitorDlg::GripForce[MAX_FRAMES];
float CGripGroundMonitorDlg::RealMarkerTime[MAX_FRAMES];
float CGripGroundMonitorDlg::CompressedMarkerTime[MAX_FRAMES];
float CGripGroundMonitorDlg::RealAnalogTime[MAX_FRAMES];
float CGripGroundMonitorDlg::CompressedAnalogTime[MAX_FRAMES];
char  CGripGroundMonitorDlg::MarkerVisibility[MAX_FRAMES][CODA_MARKERS];
char  CGripGroundMonitorDlg::ManipulandumVisibility[MAX_FRAMES];


/////////////////////////////////////////////////////////////////////////////

// Initialize the objects used to generate graphs on the display.

void CGripGroundMonitorDlg::Intialize2DGraphics() {

	HWND parent;
	RECT rect;

	// Set the limits for the various graphs.
	// For the moment they are constant.

	lowerPositionLimit = -500.0;
	upperPositionLimit =  750.0;

	lowerPositionLimitSpecific[X] = -200.0;
	upperPositionLimitSpecific[X] =  600.0, 
		
	lowerPositionLimitSpecific[Y] = -100.0;
	upperPositionLimitSpecific[Y] =  700.0, 

	lowerPositionLimitSpecific[Z] = -800.0;
	upperPositionLimitSpecific[Z] =    0.0;

	lowerAccelerationLimit = -5.0;
	upperAccelerationLimit =  5.0;


	lowerForceLimit = -10.0;
	upperForceLimit =  10.0;

	lowerGripLimit =  -2.0;
	upperGripLimit =  20.0;

	lowerCopLimit =  -25.0;
	upperCopLimit =   25.0;


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
#if 0
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
#endif

}

/////////////////////////////////////////////////////////////////////////////

void CGripGroundMonitorDlg::ResetBuffers( void ) {
	nFrames = 0;

#ifdef FAKE_DATA

	// Simulate some data.

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

#endif


}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CGripGroundMonitorDlg::GraphManipulandumPosition( View view, double start_instant, double stop_instant, int start_frame, int stop_frame ) {

	Display display = view->display;
	DisplayActivate( display );
	
	ViewColor( view, GREY6 );
	ViewBox( view );
	ViewTitle( view, "Manipulandum Position", INSIDE_LEFT, INSIDE_TOP, 0.0 );

	ViewSetXLimits( view, start_instant, stop_instant );
	ViewSetYLimits( view, lowerPositionLimit, upperPositionLimit );
	ViewAxes( view );

	if ( start_frame < start_instant ) start_frame = start_instant;
	if ( stop_frame >= stop_instant ) stop_frame = stop_instant - 1;

	// Plot all 3 components of the manipulandum position in the same view;
	for ( int i = 0; i < 3; i++ ) {
		ViewSelectColor( view, i );
		if ( stop_frame > start_frame ) ViewPlotAvailableFloats( view, &ManipulandumPosition[0][i], start_frame, stop_frame, sizeof( *ManipulandumPosition ), MISSING_FLOAT );
	}

}

void CGripGroundMonitorDlg::GraphLoadForce( View view, double start_instant, double stop_instant, int start_frame, int stop_frame ) {

	Display display = view->display;
	DisplayActivate( display );
	
	ViewColor( view, GREY6 );
	ViewBox( view );
	ViewTitle( view, "Load Force", INSIDE_LEFT, INSIDE_TOP, 0.0 );

	if ( start_frame < start_instant ) start_frame = start_instant;
	if ( stop_frame >= stop_instant ) stop_frame = stop_instant - 1;

	ViewSetXLimits( view, start_instant, stop_instant );
	ViewSetYLimits( view, lowerForceLimit, upperForceLimit );
	ViewAxes( view );
	ViewHorizontalLine( view, 0.0 );
	ViewHorizontalLine( view, 4.0 );
	ViewHorizontalLine( view, -4.0 );

	// Plot all 3 components of the load force in the same view;
	for ( int i = 0; i < 3; i++ ) {
		ViewSelectColor( view, i );
		if ( stop_frame > start_frame ) ViewPlotAvailableFloats( view, &LoadForce[0][i], start_frame, stop_frame, sizeof( *LoadForce ), MISSING_FLOAT );
	}

}

void CGripGroundMonitorDlg::GraphGripForce( View view, double start_instant, double stop_instant, int start_frame, int stop_frame ) {

	Display display = view->display;
	DisplayActivate( display );
	
	ViewColor( view, GREY6 );
	ViewBox( view );
	ViewTitle( view, "Grip Force", INSIDE_LEFT, INSIDE_TOP, 0.0 );

	if ( start_frame < start_instant ) start_frame = start_instant;
	if ( stop_frame >= stop_instant ) stop_frame = stop_instant - 1;

	ViewSetXLimits( view, start_instant, stop_instant );
	ViewSetYLimits( view, lowerGripLimit, upperGripLimit );
	ViewAxes( view );
		
	ViewSelectColor( view, CYAN );
	if ( stop_frame > start_frame ) {
//		ViewAutoScaleAvailableFloats( view, &GripForce[0], start_frame, stop_frame, sizeof( *GripForce ), MISSING_FLOAT );
		ViewPlotAvailableFloats( view, &GripForce[0], start_frame, stop_frame, sizeof( *GripForce ), MISSING_FLOAT );
	}

}

void CGripGroundMonitorDlg::GraphAcceleration( View view, double start_instant, double stop_instant, int start_frame, int stop_frame ) {

	Display display = view->display;
	DisplayActivate( display );
	
	ViewColor( view, GREY6 );
	ViewBox( view );
	ViewTitle( view, "Acceleration", INSIDE_LEFT, INSIDE_TOP, 0.0 );

	if ( start_frame < start_instant ) start_frame = start_instant;
	if ( stop_frame >= stop_instant ) stop_frame = stop_instant - 1;

	ViewSetXLimits( view, start_instant, stop_instant );
	ViewSetYLimits( view, lowerAccelerationLimit, upperAccelerationLimit );
	ViewAxes( view );
		
	// Plot all 3 components of the load force in the same view;
	for ( int i = 0; i < 3; i++ ) {
		ViewSelectColor( view, i );
		if ( stop_frame > start_frame ) ViewPlotAvailableFloats( view, &Acceleration[0][i], start_frame, stop_frame, sizeof( *Acceleration ), MISSING_FLOAT );
	}

}

void CGripGroundMonitorDlg::GraphCoP( View view, double start_instant, double stop_instant, int start_frame, int stop_frame ) {

	Display display = view->display;
	DisplayActivate( display );
	
	ViewColor( view, GREY6 );
	ViewBox( view );
	ViewTitle( view, "Center of Pressure", INSIDE_LEFT, INSIDE_TOP, 0.0 );

	if ( start_frame < start_instant ) start_frame = start_instant;
	if ( stop_frame >= stop_instant ) stop_frame = stop_instant - 1;

	ViewSetXLimits( view, start_instant, stop_instant );
	ViewSetYLimits( view, lowerCopLimit, upperCopLimit );
	ViewAxes( view );
		
	ViewHorizontalLine( view,  10.0 );
	ViewHorizontalLine( view, -10.0 );

}

void CGripGroundMonitorDlg::GraphVisibility( View view, double start_instant, double stop_instant, int start_frame, int stop_frame ) {

	Display display = view->display;
	DisplayActivate( display );
	
	ViewColor( view, GREY6 );
	ViewBox( view );
	ViewTitle( view, "Visibility", INSIDE_LEFT, INSIDE_TOP, 0.0 );

	if ( start_frame < start_instant ) start_frame = start_instant;
	if ( stop_frame >= stop_instant ) stop_frame = stop_instant - 1;

	ViewSetXLimits( view, start_instant, stop_instant );
	ViewSetYLimits( view, lowerGripLimit, upperGripLimit );
	ViewSetYLimits( view, 0, 100 );
	ViewAxes( view );

	// Plot all the visibility traces in the same view;
	// Each marker is assigned a unique non-zero value when it is visible,
	//  such that the traces are spread out and grouped in the view.
	for ( int mrk = 0; mrk < CODA_MARKERS; mrk++ ) {
		ViewColor( view, mrk % 3 );
		if ( stop_frame > start_frame ) ViewPlotAvailableChars( view, &MarkerVisibility[0][mrk], start_frame, stop_frame, sizeof( *MarkerVisibility ), MISSING_CHAR );
	}

	ViewColor( view, BLACK );
	if ( stop_frame > start_frame ) ViewBoxPlotChars( view, &ManipulandumVisibility[0],  start_frame, stop_frame, sizeof( *ManipulandumVisibility ) );
	ViewColor( view, RED );
//	ViewHorizontalLine( view, 3 );

}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CGripGroundMonitorDlg::PlotManipulandumPosition( double start_instant, double stop_instant, int start_frame, int stop_frame ) {

	View view;

	// Define the pairs for each phase plot.
	static struct {
		int abscissa;
		int ordinate;
	} pair[3] = { {Z,Y}, {X,Y}, {X,Z} };

	if ( start_frame < start_instant ) start_frame = start_instant;
	if ( stop_frame >= stop_instant ) stop_frame = stop_instant - 1;

	// XY plot of Manipulandum position data.
	for ( int i = 0; i < 3; i++ ) {
		DisplayActivate( phase_display[i] );
		Erase( phase_display[i] );
		view = phase_view[i];
		ViewSetXLimits( view, lowerPositionLimitSpecific[pair[i].abscissa], upperPositionLimitSpecific[pair[i].abscissa] );
		ViewSetYLimits( view, lowerPositionLimitSpecific[pair[i].ordinate], upperPositionLimitSpecific[pair[i].ordinate] );
		ViewSelectColor( view, i );
		if ( stop_frame > start_frame ) ViewXYPlotAvailableFloats( view, &ManipulandumPosition[0][pair[i].abscissa], &ManipulandumPosition[0][pair[i].ordinate], start_frame, stop_frame, sizeof( *ManipulandumPosition ), sizeof( *ManipulandumPosition ), MISSING_FLOAT );
	}

}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CGripGroundMonitorDlg::Draw2DGraphics() {

	View view;
	Display display;
	int i;

	DisplayActivate( stripchart_display );
	Erase( stripchart_display );
	Color( stripchart_display, GREY4 );

	static int range[6] = { 20 * 60 * 60, 20 * 60 * 30, 20 * 60 * 10, 20 * 60 * 5, 20 * 60, 20 * 30 };

	for ( i = 0; i < LayoutViews( stripchart_layout ); i++ ) {

		view = LayoutViewN( stripchart_layout, i );
		ViewBox( view );

	}

	// Skipping the CoP plot for now.
	for ( i = 0; i < N_PHASEPLOTS - 1; i++ ) {

		display = phase_display[i];
		view = phase_view[i];
		DisplayActivate( display );
		Erase( display );
		ViewSelectColor( view, i );
		ViewBox( view );

	}


	int first_sample = 0;
	int last_sample = nFrames - 1;

	int last_instant = nFrames * m_scrollbar.GetScrollPos() / 1000;
	int width = SendDlgItemMessage( IDC_TIMESCALE, TBM_GETPOS );
	int first_instant = last_instant - range[width];

	GraphManipulandumPosition( LayoutViewN( stripchart_layout, 0 ), first_instant, last_instant, first_sample, last_sample );
	GraphAcceleration( LayoutViewN( stripchart_layout, 1 ), first_instant, last_instant, first_sample, last_sample );
	GraphGripForce( LayoutViewN( stripchart_layout, 2 ), first_instant, last_instant, first_sample, last_sample );
	GraphLoadForce( LayoutViewN( stripchart_layout, 3 ), first_instant, last_instant, first_sample, last_sample );
	GraphCoP( LayoutViewN( stripchart_layout, 4 ), first_instant, last_instant, first_sample, last_sample );
	GraphVisibility( LayoutViewN( stripchart_layout, 5 ), first_instant, last_instant, first_sample, last_sample );

	PlotManipulandumPosition( first_instant, last_instant, first_sample, last_sample );

}

