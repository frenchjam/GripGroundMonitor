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

#define IDT_TIMER1 1001

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

Vector3 CGripGroundMonitorDlg::ManipulandumRotations[MAX_FRAMES];
Vector3 CGripGroundMonitorDlg::ManipulandumPosition[MAX_FRAMES];
float CGripGroundMonitorDlg::Acceleration[MAX_FRAMES][3];
float CGripGroundMonitorDlg::GripForce[MAX_FRAMES];
Vector3 CGripGroundMonitorDlg::LoadForce[MAX_FRAMES];
double CGripGroundMonitorDlg::LoadForceMagnitude[MAX_FRAMES];
Vector3 CGripGroundMonitorDlg::CenterOfPressure[2][MAX_FRAMES];
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

	lowerRotationLimit = - Pi;
	upperRotationLimit =   Pi;

	lowerPositionLimitSpecific[X] = -200.0;
	upperPositionLimitSpecific[X] =  600.0, 
		
	lowerPositionLimitSpecific[Y] = -100.0;
	upperPositionLimitSpecific[Y] =  700.0, 

	lowerPositionLimitSpecific[Z] = -800.0;
	upperPositionLimitSpecific[Z] =    0.0;

	lowerAccelerationLimit = -2.0;
	upperAccelerationLimit =  2.0;


	lowerForceLimit = -10.0;
	upperForceLimit =  10.0;

	lowerGripLimit =  -2.0;
	upperGripLimit =  40.0;

	lowerCopLimit =  -0.030;
	upperCopLimit =   0.030;


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
	LayoutSetDisplayEdgesRelative( stripchart_layout, 0.01, 0.055, 0.99, 0.99 );
	visibility_view = CreateView( stripchart_display );
	ViewSetDisplayEdgesRelative( visibility_view, 0.015, 0.01, 0.985, 0.05 );

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

#if 0
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
#endif

	// Repeat for COP (center-of-pressure)
	parent = m_xz.GetSafeHwnd();
	m_cop.GetClientRect( &rect );
	cop_display = phase_display[2] = CreateOglDisplay();
	SetOglWindowParent(  parent );
	DisplaySetSizePixels( cop_display, rect.right, rect.bottom );
	DisplaySetScreenPosition( cop_display, rect.left, rect.top );
	DisplayInit( cop_display );

	cop_view = phase_view[2] = CreateView( cop_display );
	ViewSetDisplayEdgesRelative( cop_view, 0.01, 0.01, 0.99, 0.99 );
	ViewSetEdges( cop_view, 0, 0, 1, 1 );
	ViewMakeSquare(cop_view);

}

/////////////////////////////////////////////////////////////////////////////

void CGripGroundMonitorDlg::ResetBuffers( void ) {
	nFrames = 0;
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
	for ( int i = X; i <= Z; i++ ) {
		ViewSelectColor( view, i );
		if ( stop_frame > start_frame ) ViewPlotAvailableDoubles( view, &ManipulandumPosition[0][i], start_frame, stop_frame, sizeof( *ManipulandumPosition ), MISSING_DOUBLE );
	}

}

void CGripGroundMonitorDlg::GraphManipulandumRotations( View view, double start_instant, double stop_instant, int start_frame, int stop_frame ) {

	Display display = view->display;
	DisplayActivate( display );
	
	ViewColor( view, GREY6 );
	ViewBox( view );
	ViewTitle( view, "Manipulandum Rotation", INSIDE_LEFT, INSIDE_TOP, 0.0 );

	ViewSetXLimits( view, start_instant, stop_instant );
	ViewSetYLimits( view, lowerRotationLimit, upperRotationLimit );
	ViewAxes( view );

	if ( start_frame < start_instant ) start_frame = start_instant;
	if ( stop_frame >= stop_instant ) stop_frame = stop_instant - 1;

	// Plot all 3 components of the manipulandum position in the same view;
	for ( int i = X; i <= Z; i++ ) {
		ViewSelectColor( view, i );
		if ( stop_frame > start_frame ) ViewPlotAvailableDoubles( view, &ManipulandumRotations[0][i], start_frame, stop_frame, sizeof( *ManipulandumRotations ), MISSING_DOUBLE );
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
	for ( int i = X; i <= Z; i++ ) {
		ViewSelectColor( view, i );
		if ( stop_frame > start_frame ) ViewPlotAvailableDoubles( view, &LoadForce[0][i], start_frame, stop_frame, sizeof( *LoadForce ), MISSING_DOUBLE );
	}
	ViewSelectColor( view, i );
	if ( stop_frame > start_frame ) ViewPlotAvailableDoubles( view, &LoadForceMagnitude[0], start_frame, stop_frame, sizeof( *LoadForceMagnitude ), MISSING_DOUBLE );

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
		
	// Plot all 3 components of the acceleration in a single view;
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
	ViewHorizontalLine( view,  0.01 );
	ViewHorizontalLine( view, -0.01 );
		
	for ( int ati = 0; ati < 2; ati++ ) {
		for ( int i = X; i <= Z; i++ ) {
			ViewSelectColor( view, 3 * ati + i );
			if ( stop_frame > start_frame ) ViewPlotClippedDoubles( view, &CenterOfPressure[ati][0][i], start_frame, stop_frame, sizeof( *CenterOfPressure[ati] ), MISSING_DOUBLE );
		}
	}
		

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
	for ( int i = 0; i < 2; i++ ) {
		DisplayActivate( phase_display[i] );
		Erase( phase_display[i] );
		view = phase_view[i];
		ViewSetXLimits( view, lowerPositionLimitSpecific[pair[i].abscissa], upperPositionLimitSpecific[pair[i].abscissa] );
		ViewSetYLimits( view, lowerPositionLimitSpecific[pair[i].ordinate], upperPositionLimitSpecific[pair[i].ordinate] );
		ViewMakeSquare( view );
		ViewSelectColor( view, i );
		if ( stop_frame > start_frame ) ViewXYPlotAvailableDoubles( view, &ManipulandumPosition[0][pair[i].abscissa], &ManipulandumPosition[0][pair[i].ordinate], start_frame, stop_frame, sizeof( *ManipulandumPosition ), sizeof( *ManipulandumPosition ), MISSING_DOUBLE );
	}

}

void CGripGroundMonitorDlg::PlotCoP( double start_instant, double stop_instant, int start_frame, int stop_frame ) {

	View view;

	if ( start_frame < start_instant ) start_frame = start_instant;
	if ( stop_frame >= stop_instant ) stop_frame = stop_instant - 1;

	DisplayActivate( cop_display );
	Erase( cop_display );
	view = cop_view;
	ViewSetXLimits( view, lowerCopLimit, upperCopLimit );
	ViewSetYLimits( view, lowerCopLimit, upperCopLimit );
	ViewMakeSquare( view );

	if ( stop_frame > start_frame ) {
		ViewSetColor( view, RED );
		ViewScatterPlotAvailableDoubles( view, SYMBOL_FILLED_SQUARE, &CenterOfPressure[0][0][Z], &CenterOfPressure[0][0][Y], start_frame, stop_frame, sizeof( *CenterOfPressure[0] ), sizeof( *CenterOfPressure[0] ), MISSING_FLOAT );
		ViewSetColor( view, BLUE );
		ViewScatterPlotAvailableDoubles( view, SYMBOL_FILLED_SQUARE, &CenterOfPressure[1][0][Z], &CenterOfPressure[1][0][Y], start_frame, stop_frame, sizeof( *CenterOfPressure[1] ), sizeof( *CenterOfPressure[1] ), MISSING_FLOAT );
	}
	ViewSetColor( view, CYAN );
	ViewCircle( view, 0.0, 0.0, 0.010 );
	ViewSetColor( view, MAGENTA );
	ViewCircle( view, 0.0, 0.0, 0.020 );

}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

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
	int first_instant = last_instant - windowSpan[width];
	fOutputDebugString( "Span: %d %d\n", width, SPAN_VALUES );

	GraphManipulandumPosition( LayoutViewN( stripchart_layout, 0 ), first_instant, last_instant, first_sample, last_sample );
	GraphManipulandumRotations( LayoutViewN( stripchart_layout, 1 ), first_instant, last_instant, first_sample, last_sample );
	GraphAcceleration( LayoutViewN( stripchart_layout, 2 ), first_instant, last_instant, first_sample, last_sample );
	GraphGripForce( LayoutViewN( stripchart_layout, 3 ), first_instant, last_instant, first_sample, last_sample );
	GraphLoadForce( LayoutViewN( stripchart_layout, 4 ), first_instant, last_instant, first_sample, last_sample );
	GraphCoP( LayoutViewN( stripchart_layout, 5 ), first_instant, last_instant, first_sample, last_sample );
	GraphVisibility( visibility_view, first_instant, last_instant, first_sample, last_sample );

	PlotManipulandumPosition( first_instant, last_instant, first_sample, last_sample );
	PlotCoP( first_instant, last_instant, first_sample, last_sample );

}

