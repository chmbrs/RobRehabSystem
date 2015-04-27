/*******************************************************************************
* This example uses the Network Variable library to communicate between a
* DLL running on a real-time target and an executable running on a host machine.
* The real-time DLL generates and publishes sine wave data based on the 
* amplitude and frequency specified by the host executable. When the host 
* executable exits it signals the real-time DLL to exit as well.
*
* This example uses several network variables. Two network variables communicate 
* the amplitude and frequency information from the host executable to the real-
* time DLL. Another network variable publishes the sine wave data from the real-
* time DLL to the host executable. One more network variable indicates that the 
* host executable is exiting so the real-time DLL can exit as well.
*
* The real-time DLL uses multiple threads. The main thread publishes data while
* Network Variable Library threads run callbacks when network variable values
* are updated. The real-time DLL uses thread safe variables to pass data between
* the network variable callbacks and the main thread.
*
* NOTE: This example requires the LabWindows/CVI Real-Time Module.
*
* This example consists of two projects:
*
* NetworkVariableHost.prj - This project builds an executable with a user 
* interface. This executable communicates with the real-time DLL over TCP via 
* the Network Variable Library.
*
* NetworkVariableRT.prj - This project builds a DLL that will run on a real-time
* target. This DLL interacts with the host executable over TCP via the Network
* Variable Library. Download and run the DLL on a real-time target. See the 
* LabWindows/CVI documentation for detailed instructions.
*
* Run the real-time DLL project first and then run the host executable. When you
* exit the host executable the real-time DLL project will also exit.
*******************************************************************************/

/* Include files */
//#include <ansi_c.h>
#include <stdbool.h>
#include <cvinetv.h>
#include <cvirte.h>		
#include <userint.h>

#include "async_debug.h"
#include "cvirte_ip_connection.h"
#include "NetworkVariable.h"
#include "common.h"

#include "data_logging.h"

/* Global variables */
static int panel;
static CNVSubscriber gWaveSubscriber;
static CNVWriter gMaxToggleWriter, gMinToggleWriter, gMotorToggleWriter, gMaxStiffnessWriter, gSetpointWriter;

/* Function prototypes */
static void* UpdateData( void* );
static void ConnectToNetworkVariables( void );
int CVICALLBACK GainCallback( int, int, int, void*, int, int );
void CVICALLBACK DataCallback( void*, CNVData, void* );

int axisLogID;
enum { POSITION, VELOCITY, TORQUE, TENSION, REFERENCE_VALUE, PROPORTIONAL_GAIN, DERIVATIVE_GAIN, INTEGRAL_GAIN, DISPLAY_N_VALUES };

enum { AGONIST, ANTAGONIST };
int muscleGroup = AGONIST;

int axisID = 0;

Thread_Handle dataConnectionThreadID;
bool isDataUpdateRunning = false;

char address[ 256 ] = "169.254.110.158";

/* Program entry-point */
int main( int argc, char *argv[] )
{
	CNVData enableData;
	
	if( InitCVIRTE( 0, argv, 0 ) == 0 )
		return -1;
  
  //dataConnectionThreadID = Thread_Start( UpdateData, NULL, JOINABLE );
  int dataClientID = AsyncIPConnection_Open( address, "50001", UDP );
  AsyncIPConnection_WriteMessage( dataClientID, "0 0.0 0.0 0.0 0.0 0.0" );
	
  axisLogID = DataLogging_CreateLog( NULL, "axis", DISPLAY_N_VALUES );
  
	if( (panel = LoadPanel( 0, "NetworkVariable.uir", PANEL )) < 0 )
		return -1;

	ConnectToNetworkVariables();
	
	DisplayPanel( panel );
	RunUserInterface();
	
	CNVCreateScalarDataValue( &enableData, CNVBool, (char) 0 );
	CNVWrite( gMotorToggleWriter, enableData, CNVDoNotWait );
  
  //isDataUpdateRunning = false;
  //Thread_WaitExit( dataConnectionThreadID, 5000 );
  AsyncIPConnection_Close( dataClientID );
  
  DataLogging_CloseLog( axisLogID );
  
	// Cleanup
	CNVDisposeData( enableData );
	CNVDispose( gWaveSubscriber );
	CNVDispose( gMaxToggleWriter );
	CNVDispose( gMinToggleWriter );
	CNVDispose( gMotorToggleWriter );
  CNVDispose( gMaxStiffnessWriter );
  CNVDispose( gSetpointWriter );
	CNVFinish();
  
	DiscardPanel( panel );
	
  CloseCVIRTE();
  
	return 0;
}

static void* UpdateData( void* callbackData )
{
  int dataClientID = AsyncIPConnection_Open( address, "50001", UDP );
  
  if( dataClientID != -1 ) isDataUpdateRunning = true;
  
  while( isDataUpdateRunning )
  {
    AsyncIPConnection_WriteMessage( dataClientID, "0 0.0 0.0 0.0 0.0 0.0" );
  }
  
  AsyncIPConnection_Close( dataClientID );
  
  Thread_Exit( 0 );
  return NULL;
}

static void ConnectToNetworkVariables( void )
{
	char path[ 512 ];
	
	// Get address of real-time target.
	//PromptPopup("Prompt", "Enter Real-Time Target Name/IP Address:", address, sizeof(address) - 1);
	
	// Connect to network variables.
	sprintf( path, "\\\\%s\\" PROCESS "\\%s", address, MAX_TOGGLE_VARIABLE );
	CNVCreateWriter( path, 0, 0, 10000, 0, &gMaxToggleWriter );
	
	sprintf( path, "\\\\%s\\" PROCESS "\\%s", address, MIN_TOGGLE_VARIABLE );
	CNVCreateWriter( path, 0, 0, 10000, 0, &gMinToggleWriter );

	sprintf( path, "\\\\%s\\" PROCESS "\\%s", address, ENABLE_VARIABLE );
	CNVCreateWriter( path, 0, 0, 10000, 0, &gMotorToggleWriter );
  
  sprintf( path, "\\\\%s\\" PROCESS "\\%s", address, STIFFNESS_VARIABLE );
	CNVCreateWriter( path, 0, 0, 10000, 0, &gMaxStiffnessWriter );
  
  sprintf( path, "\\\\%s\\" PROCESS "\\%s", address, SETPOINT_VARIABLE );
	CNVCreateWriter( path, 0, 0, 10000, 0, &gSetpointWriter );

	sprintf( path, "\\\\%s\\" PROCESS "\\%s", address, WAVE_VARIABLE );
	CNVCreateSubscriber( path, DataCallback, 0, 0, 10000, 0, &gWaveSubscriber );
	
	// Manually call the control callbacks to write the initial values for
	// amplitude and frequency to the network variables.
  SetCtrlVal( panel, PANEL_MAX_TOGGLE, 0 );
  SetCtrlVal( panel, PANEL_MIN_TOGGLE, 0 );
  SetCtrlVal( panel, PANEL_MOTOR_TOGGLE, 0 );
	ChangeStateCallback( panel, PANEL_MAX_TOGGLE, EVENT_COMMIT, 0, 0, 0 );
	ChangeStateCallback( panel, PANEL_MIN_TOGGLE, EVENT_COMMIT, 0, 0, 0 );
  ChangeStateCallback( panel, PANEL_MOTOR_TOGGLE, EVENT_COMMIT, 0, 0, 0 );
  
	ChangeValueCallback( panel, PANEL_STIFFNESS, EVENT_COMMIT, 0, 0, 0 );
  ChangeValueCallback( panel, PANEL_SETPOINT, EVENT_COMMIT, 0, 0, 0 );
}

int CVICALLBACK ChangeMuscleGroupCallback( int panel, int control, int event, void* callbackData, int eventData1, int eventData2 )
{
  switch( event )
	{
		case EVENT_COMMIT:
			// Get the new value.
			GetCtrlVal( panel, control, &muscleGroup );
			break;
	}
	return 0;
}

int CVICALLBACK ChangeStateCallback( int panel, int control, int event, void* callbackData, int eventData1, int eventData2 )
{
	CNVData	data;
	int enabled;
  
  int messageCode = 0;
	
	switch( event )
	{
		case EVENT_COMMIT:
			// Get the new value.
			GetCtrlVal( panel, control, &enabled );
      
      messageCode |= ( axisID & 0x000000ff );
      messageCode |= ( ( enabled * 0x100 ) & 0x0000ff00 );
      messageCode |= ( ( muscleGroup * 0x10000 ) & 0x00ff0000 );
      
			CNVCreateScalarDataValue( &data, CNVInt32, messageCode );
		
			// Write the new value to the appropriate network variable.
			if( control == PANEL_MAX_TOGGLE )
				CNVWrite( gMaxToggleWriter, data, CNVDoNotWait );
			else if( control == PANEL_MIN_TOGGLE )
				CNVWrite( gMinToggleWriter, data, CNVDoNotWait );
      else if( control == PANEL_MOTOR_TOGGLE )
        CNVWrite( gMotorToggleWriter, data, CNVDoNotWait );
		
			CNVDisposeData( data );
      
			break;
	}
	return 0;
}

int  CVICALLBACK ChangeValueCallback( int panel, int control, int event, void* callbackData, int eventData1, int eventData2 )
{
  CNVData	data;
	double value;
	
	switch( event )
	{
		case EVENT_COMMIT:
			// Get the new value.
			GetCtrlVal( panel, control, &value );
      
			CNVCreateScalarDataValue( &data, CNVDouble, value );
		
			// Write the new value to the appropriate network variable.
			if( control == PANEL_STIFFNESS )
				CNVWrite( gMaxStiffnessWriter, data, CNVDoNotWait );
			else if( control == PANEL_SETPOINT )
				CNVWrite( gSetpointWriter, data, CNVDoNotWait );
		
			CNVDisposeData( data );
      
			break;
	}
	return 0;
}

int CVICALLBACK QuitCallback( int panel, int event, void *callbackData, int eventData1, int eventData2 )
{
  switch( event )
	{
		case EVENT_CLOSE:
			
      QuitUserInterface( 0 );
      
			break;
	}
	return 0;
}

void CVICALLBACK DataCallback( void* handle, CNVData data, void* callbackData )
{
	static double dataArray[ DISPLAY_N_VALUES * NUM_POINTS ];
  static double positionValues[ NUM_POINTS ], velocityValues[ NUM_POINTS ], torqueValues[ NUM_POINTS ], tensionValues[ NUM_POINTS ];
  static double pGainValues[ NUM_POINTS ], dGainValues[ NUM_POINTS ], setpointValues[ NUM_POINTS ];
	
	// Get the published data.
	CNVGetArrayDataValue( data, CNVDouble, dataArray, DISPLAY_N_VALUES * NUM_POINTS );
  
  DataLogging_SaveData( axisLogID, dataArray, DISPLAY_N_VALUES * NUM_POINTS );
  
  for( size_t pointIndex = 0; pointIndex < NUM_POINTS; pointIndex++ )
  {
    positionValues[ pointIndex ] = dataArray[ pointIndex * DISPLAY_N_VALUES + POSITION ];
    velocityValues[ pointIndex ] = dataArray[ pointIndex * DISPLAY_N_VALUES + VELOCITY ];
    setpointValues[ pointIndex ] = dataArray[ pointIndex * DISPLAY_N_VALUES + REFERENCE_VALUE ];
    pGainValues[ pointIndex ] = dataArray[ pointIndex * DISPLAY_N_VALUES + PROPORTIONAL_GAIN ];
    dGainValues[ pointIndex ] = dataArray[ pointIndex * DISPLAY_N_VALUES + DERIVATIVE_GAIN ];
    torqueValues[ pointIndex ] = dataArray[ pointIndex * DISPLAY_N_VALUES + TORQUE ];
    tensionValues[ pointIndex ] = dataArray[ pointIndex * DISPLAY_N_VALUES + TENSION ];
  }
  
	// Plot the data to the graph.
	DeleteGraphPlot( panel, PANEL_GRAPH_POSITION, -1, VAL_DELAYED_DRAW );
	PlotY( panel, PANEL_GRAPH_POSITION, setpointValues, NUM_POINTS, VAL_DOUBLE, VAL_THIN_LINE, VAL_NO_POINT, VAL_SOLID, 1, VAL_RED );
  PlotY( panel, PANEL_GRAPH_POSITION, positionValues, NUM_POINTS, VAL_DOUBLE, VAL_FAT_LINE, VAL_NO_POINT, VAL_SOLID, 1, VAL_YELLOW );
  PlotY( panel, PANEL_GRAPH_POSITION, torqueValues, NUM_POINTS, VAL_DOUBLE, VAL_THIN_LINE, VAL_NO_POINT, VAL_SOLID, 1, VAL_GREEN );
  PlotY( panel, PANEL_GRAPH_POSITION, tensionValues, NUM_POINTS, VAL_DOUBLE, VAL_THIN_LINE, VAL_NO_POINT, VAL_SOLID, 1, VAL_MAGENTA );
  
  DeleteGraphPlot( panel, PANEL_GRAPH_EMG, -1, VAL_DELAYED_DRAW );
  //PlotY( panel, PANEL_GRAPH_EMG, emgValues, NUM_POINTS, VAL_DOUBLE, VAL_THIN_LINE, VAL_NO_POINT, VAL_SOLID, 1, VAL_BLUE );
  PlotY( panel, PANEL_GRAPH_EMG, pGainValues, NUM_POINTS, VAL_DOUBLE, VAL_FAT_LINE, VAL_NO_POINT, VAL_SOLID, 1, VAL_RED );
  PlotY( panel, PANEL_GRAPH_EMG, dGainValues, NUM_POINTS, VAL_DOUBLE, VAL_FAT_LINE, VAL_NO_POINT, VAL_SOLID, 1, VAL_YELLOW );
		
	CNVDisposeData( data );
}

