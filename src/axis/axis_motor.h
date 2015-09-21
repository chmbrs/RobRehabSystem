#ifndef AXIS_MOTOR_H
#define AXIS_MOTOR_H

#ifdef __cplusplus
    extern "C" {
#endif

#include "axis/axis_types.h"

#include "file_parsing/json_parser.h"

#include "klib/khash.h"
      
#include "debug/async_debug.h"

enum AxisMotorVariables { MOTOR_POSITION, MOTOR_VELOCITY, MOTOR_FORCE, MOTOR_VARS_NUMBER };
      
typedef struct _MotorData
{
  int axisID;
  AxisInterface interface;
  double measuresList[ MOTOR_VARS_NUMBER ];
  double measureGainsList[ MOTOR_VARS_NUMBER ];
  double measureOffsetsList[ MOTOR_VARS_NUMBER ];
  enum AxisMotorVariables operationMode;
  unsigned int encoderResolution;
  double currentToForceRatio;
  double gearReduction;
}
MotorData;

typedef MotorData* Motor;

KHASH_MAP_INIT_INT( MotorInt, Motor )
static khash_t( MotorInt )* motorsList = NULL;

/*static AxisMotor AxisMotor_Init( const char* );
static inline void AxisMotor_End( AxisMotor );
static inline void AxisMotor_Enable( AxisMotor );
static inline void AxisMotor_Disable( AxisMotor );
static inline void AxisMotor_Reset( AxisMotor );
static void AxisMotor_SetOffset( AxisMotor );
static inline bool AxisMotor_IsEnabled( AxisMotor );
static inline bool AxisMotor_HasError( AxisMotor );
static double* AxisMotor_ReadMeasures( AxisMotor );
static void AxisMotor_WriteControl( AxisMotor, double );*/

#define NAMESPACE AxisMotor

#define NAMESPACE_FUNCTIONS( namespace ) \
        NAMESPACE_FUNCTION( int, namespace, Init, const char* ) \
        NAMESPACE_FUNCTION( void, namespace, End, int ) \
        NAMESPACE_FUNCTION( void, namespace, Enable, int ) \
        NAMESPACE_FUNCTION( void, namespace, Disable, int ) \
        NAMESPACE_FUNCTION( void, namespace, Reset, int ) \
        NAMESPACE_FUNCTION( void, namespace, SetOffset, int ) \
        NAMESPACE_FUNCTION( bool, namespace, IsEnabled, int ) \
        NAMESPACE_FUNCTION( bool, namespace, HasError, int ) \
        NAMESPACE_FUNCTION( double*, namespace, ReadMeasures, int ) \
        NAMESPACE_FUNCTION( void, namespace, WriteControl, int, double ) \
        NAMESPACE_FUNCTION( void, namespace, SetOperationMode, int, enum AxisMotorVariables ) \

#define NAMESPACE_FUNCTION( rvalue, namespace, name, ... ) static rvalue namespace##_##name( __VA_ARGS__ );
NAMESPACE_FUNCTIONS( NAMESPACE )
#undef NAMESPACE_FUNCTION

#define NAMESPACE_FUNCTION( rvalue, namespace, name, ... ) rvalue (*name)( __VA_ARGS__ );
const struct { NAMESPACE_FUNCTIONS( NAMESPACE ) }
#undef NAMESPACE_FUNCTION
#define NAMESPACE_FUNCTION( rvalue, namespace, name, ... ) .name = namespace##_##name,
NAMESPACE = { NAMESPACE_FUNCTIONS( NAMESPACE ) };
#undef NAMESPACE_FUNCTION

#undef NAMESPACE_FUNCTIONS
#undef NAMESPACE

static inline AxisMotor LoadMotorData( const char* );
static inline void UnloadMotorData( CANInterface* );

static inline void ReadRawMeasures( AxisMotor motor );

static int AxisMotor_Init( const char* configFileName )
{
  DEBUG_EVENT( 0, "Initializing Axis Motor %s", configFileName );
  
  if( motorsList == NULL ) motorsList = kh_init( MotorInt );
  
  int configKey = (int) kh_str_hash_func( configFileName );
  
  int insertionStatus;
  khint_t newMotorID = kh_put( MotorInt, motorsList, configKey, &insertionStatus );
  if( insertionStatus > 0 )
  {
    kh_value( motorsList, newMotorID ) = LoadMotorData( configFileName );
    if( kh_value( motorsList, newMotorID ) == NULL )
    {
      AxisMotor_End( (int) newMotorID );
      return -1;
    }
  }
  
  DEBUG_EVENT( 0, "Axis Motor %s initialized (iterator: %d)", configFileName, (int) newMotorID );
  
  return (int) newMotorID;
}

static void AxisMotor_End( int motorID )
{
  if( !kh_exist( motorsList, (khint_t) motorID ) ) return;
  
  Motor motor = kh_value( motorsList, (khint_t) motorID );
  
  UnloadMotorData( motor );
    
  kh_del( MotorInt, motorsList, (khint_t) motorID );
    
  if( kh_size( motorsList ) == 0 )
  {
    kh_destroy( MotorInt, motorsList );
    motorsList = NULL;
  }
}

static void AxisMotor_Enable( int motorID )
{
  if( !kh_exist( motorsList, (khint_t) motorID ) ) return;
  
  Motor motor = kh_value( motorsList, (khint_t) motorID );
  
  motor->interface->Enable( motor->axisID );
}

static void AxisMotor_Disable( int motorID )
{
  if( !kh_exist( motorsList, (khint_t) motorID ) ) return;
  
  Motor motor = kh_value( motorsList, (khint_t) motorID );
  
  motor->interface->Disable( motor->axisID );
}

static void AxisMotor_Reset( int motorID )
{
  if( !kh_exist( motorsList, (khint_t) motorID ) ) return;
  
  Motor motor = kh_value( motorsList, (khint_t) motorID );
  
  motor->interface->Reset( motor->axisID );
}

static void AxisMotor_SetOffset( int motorID )
{
  static double rawMeasuresList[ AXIS_VARS_NUMBER ];
  
  if( !kh_exist( motorsList, (khint_t) motorID ) ) return;
  
  Motor motor = kh_value( motorsList, (khint_t) motorID );
  
  motor->interface->ReadMeasures( motor->axisID, rawMeasuresList );
  
  motor->measureOffsetsList[ MOTOR_POSITION ] = rawMeasuresList[ AXIS_ENCODER ] * motor->measureGainsList[ MOTOR_POSITION ];
  motor->measureOffsetsList[ MOTOR_VELOCITY ] = rawMeasuresList[ AXIS_RPS ] * motor->measureGainsList[ MOTOR_VELOCITY ];
  motor->measureOffsetsList[ MOTOR_FORCE ] = rawMeasuresList[ AXIS_CURRENT ] * motor->measureGainsList[ MOTOR_FORCE ];
}

static bool AxisMotor_IsEnabled( int motorID )
{
  if( !kh_exist( motorsList, (khint_t) motorID ) ) return false;
  
  Motor motor = kh_value( motorsList, (khint_t) motorID );
  
  return motor->interface->IsEnabled( motor->axisID );
}

static bool AxisMotor_HasError( int motorID )
{
  if( !kh_exist( motorsList, (khint_t) motorID ) ) return false;
  
  Motor motor = kh_value( motorsList, (khint_t) motorID );
  
  return motor->interface->HasError( motor->axisID );
}

static double* AxisMotor_ReadMeasures( int motorID )
{
  static double rawMeasuresList[ AXIS_VARS_NUMBER ];
  
  if( !kh_exist( motorsList, (khint_t) motorID ) ) return NULL;
  
  Motor motor = kh_value( motorsList, (khint_t) motorID );
  
  motor->interface->ReadMeasures( motor->axisID, rawMeasuresList );
  
  motor->measuresList[ MOTOR_POSITION ] = rawMeasuresList[ AXIS_ENCODER ] * motor->measureGainsList[ MOTOR_POSITION ];
  motor->measuresList[ MOTOR_VELOCITY ] = rawMeasuresList[ AXIS_RPS ] * motor->measureGainsList[ MOTOR_VELOCITY ];
  motor->measuresList[ MOTOR_FORCE ] = rawMeasuresList[ AXIS_CURRENT ] * motor->measureGainsList[ MOTOR_FORCE ];
  
  for( size_t measureIndex = 0; measureIndex < MOTOR_VARS_NUMBER; measureIndex++ )
    motor->measuresList[ measureIndex ] -= motor->measureOffsetsList[ measureIndex ];
}

static void AxisMotor_WriteControl( int motorID, double setpoint )
{
  if( !kh_exist( motorsList, (khint_t) motorID ) ) return;
  
  Motor motor = kh_value( motorsList, (khint_t) motorID );
  
  double axisSetpoint = ( setpoint / motor->measureGainsList[ motor->operationMode ] ) + motor->measureOffsetsList[ motor->operationMode ];
  
  motor->interface->WriteSetpoint( motor->axisID, axisSetpoint );
}

const int AXIS_OPERATION_MODES[ MOTOR_VARS_NUMBER ] = { AXIS_ENCODER, AXIS_RPS, AXIS_CURRENT };
static void AxisMotor_SetOperationMode( int motorID, enum AxisMotorVariables mode )
{
  if( !kh_exist( motorsList, (khint_t) motorID ) ) return;
  
  if( mode < 0 || mode >= MOTOR_VARS_NUMBER ) return;
  
  Motor motor = kh_value( motorsList, (khint_t) motorID );
  
  motor->interface->SetOperationMode( motor->axisID, AXIS_OPERATION_MODES[ mode ] );
  
  motor->operationMode = mode;
}
                                    
                                    
static inline Motor LoadMotorData( const char* configFileName )
{
  Motor newMotor = (Motor) malloc( sizeof(MotorData) );
  
  // File Parsing
  
  
  newMotor->interface = &AxisCANEPOSOperations;
  newMotor->axisID = newMotor->interface->Connect( "CAN Motor Teste" );
  newMotor->operationMode = AXIS_CURRENT;
  newMotor->interface->SetOperationMode( newMotor->axisID, newMotor->operationMode );
  
  unsigned int encoderResolution = 1;
  double currentToForceRatio = 151.8; // 0.0302;
  double gearReduction = 1.0; // 0.0025 / ( 2 * 2 * M_PI );
  
  newMotor->measureGainsList[ MOTOR_POSITION ] = 1.0 / ( encoderResolution * gearReduction );
  newMotor->measureGainsList[ MOTOR_VELOCITY ] = 1.0 / gearReduction;
  newMotor->measureGainsList[ MOTOR_FORCE ] = currentToForceRatio * gearReduction;
  
  return newMotor;
}

static inline void UnloadMotorData( Motor motor )
{
  if( motor == NULL ) return;
  
  motor->interface->Disconnect( motor->axisID );
  
  free( motor );
}

#ifdef __cplusplus
    }
#endif

#endif  // AXIS_MOTOR_H
