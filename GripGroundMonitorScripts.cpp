// GripGroundMonitorScripts.cpp : implementation file
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

/////////////////////////////////////////////////////////////////////////////

// Routines to walk the script tree.

char CGripGroundMonitorDlg::picture[MAX_STEPS][256];
char CGripGroundMonitorDlg::message[MAX_STEPS][132];
char *CGripGroundMonitorDlg::type[MAX_STEPS];
bool CGripGroundMonitorDlg::comment[MAX_STEPS];

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


	SendDlgItemMessage( IDC_STEPS, LB_ADDSTRING, 0, (LPARAM) "<Waiting to start ...>" );
	strcpy( message[lines], status_message );
	strcpy( picture[lines], status_picture );
	type[lines] = type_status;
	comment[lines] = true;
	stepID[lines] = current_step;
	lines++;

	while ( fgets( line, sizeof( line ), fp ) ) {

		line[strlen( line ) - 1] = 0;
		SendDlgItemMessage( IDC_STEPS, LB_ADDSTRING, 0, (LPARAM) line );
		tokens = ParseCommaDelimitedLine( token, line );

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

	// We want to work in the same directory as the subject file.
	// Get the directory from the filename.
	char directory[1024];
	for ( int i = strlen( filename ); i >=0; i-- ) {
		if ( filename[i] == '\\' ) break;
	}
	strncpy( directory, filename, i + 1 );
	directory[i+1] = 0;


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
			strncpy( task_filename, directory, sizeof( task_filename ) );
			strncat( task_filename, token[2],  sizeof( task_filename ) );
			// Check if it is present and readable, unless told to ignore it.
			if ( !strstr( task_filename, "ignore" ) ) {
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

	// We want to work in the same directory as the subject file.
	// Get the directory from the filename.
	char directory[1024];
	for ( int i = strlen( filename ); i >=0; i-- ) {
		if ( filename[i] == '\\' ) break;
	}
	strncpy( directory, filename, i + 1 );
	directory[i+1] = 0;


	// Open the session file, if we can.
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
			strncpy( protocol_filename, directory, sizeof( protocol_filename ) );
			strncat( protocol_filename, token[2], sizeof( protocol_filename ) );
			// Check if it is present and readable, unless told to ignore it.
			if ( !strstr( protocol_filename, "ignore" ) ) {
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


int CGripGroundMonitorDlg::ParseSubjectFile ( const char *filename ) {

	FILE *fp;

	int tokens;
	char *token[MAX_TOKENS];
	char line[2048];
	int line_n = 0;

	char session_filename[1024];
	int subjects = 0;

	// We want to work in the same directory as the subject file.
	// Get the directory from the filename.
	char directory[1024];
	for ( int i = strlen( filename ); i >=0; i-- ) {
		if ( filename[i] == '\\' ) break;
	}
	strncpy( directory, filename, i + 1 );
	directory[i+1] = 0;

	// Open the root file, if we can.
	fp = fopen( filename, "r" );
	if ( !fp ) {

		char msg[1024];
		sprintf( msg, "Error opening subject file %s for read.", filename );
		printf( "%s\n", msg );
		MessageBox( msg, "DexScriptRunner", MB_OK | MB_ICONERROR );
		return( ERROR_EXIT );
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
			strncpy( session_filename, directory, sizeof( session_filename ) );
			strncat( session_filename, token[3], sizeof( session_filename ) );

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
			sprintf( msg, "%s Line %03d Wrong number of parameters: %s\n", filename, line_n, line );
			MessageBox( msg, "DexScriptRunner", MB_OK | MB_ICONERROR );
			exit( ERROR_EXIT );
		}			
	}

	fclose( fp );
	return( 0 );

}
