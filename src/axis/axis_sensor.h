#ifndef AXIS_SENSOR_H
#define AXIS_SENSOR_H

#ifdef __cplusplus
    extern "C" {
#endif

#include "axis/axis_types.h"
#include "spline3_interpolation.h"
      
#include "file_parsing/json_parser.h"

#include "debug/async_debug.h"
      
typedef struct _SensorData
{
  int axisID;
  AxisInterface interface;
  size_t measureIndex;
  Splined3Curve* measureConversionCurve;
  double inputBuffer[ 6 ];
  double inputOffset;
}
SensorData;

typedef SensorData* Sensor;

KHASH_MAP_INIT_INT( SensorInt, Sensor )
static khash_t( SensorInt )* sensorsList = NULL;

/*static AxisSensor AxisSensor_Init( const char* );
static inline void AxisSensor_End( AxisSensor );
static inline void AxisSensor_Reset( AxisSensor );
static void AxisSensor_SetOffset( AxisSensor );
static inline bool AxisSensor_IsEnabled( AxisSensor );
static inline bool AxisSensor_HasError( AxisSensor );
static double AxisSensor_Read( AxisSensor );*/

#define NAMESPACE AxisSensor

#define NAMESPACE_FUNCTIONS \
        NAMESPACE_FUNCTION( int, Init, const char* ) \
        NAMESPACE_FUNCTION( void, End, int ) \
        NAMESPACE_FUNCTION( void, Reset, int ) \
        NAMESPACE_FUNCTION( void, SetOffset, int ) \
        NAMESPACE_FUNCTION( bool, IsEnabled, int ) \
        NAMESPACE_FUNCTION( bool, HasError, int ) \
        NAMESPACE_FUNCTION( double, Read, int )

#define NAMESPACE_FUNCTION( rvalue, name, _VA_ARGS_ ) static rvalue NAMESPACE##_##name( _VA_ARGS_ );
NAMESPACE_FUNCTIONS
#undef NAMESPACE_FUNCTION

#define NAMESPACE_FUNCTION( rvalue, name, _VA_ARGS_ ) rvalue (*name)( _VA_ARGS_ );
const struct { NAMESPACE_FUNCTIONS }
#undef NAMESPACE_FUNCTION
#define NAMESPACE_FUNCTION( rvalue, name, _VA_ARGS_ ) .name = NAMESPACE##_##name,
NAMESPACE = { NAMESPACE_FUNCTIONS };
#undef NAMESPACE_FUNCTION

#undef NAMESPACE_FUNCTIONS
#undef NAMESPACE

static inline Sensor LoadSensorData( const char* );
static inline void UnloadSensorData( AxisSensor );

static inline void ReadRawMeasures( AxisSensor sensor );

static int AxisSensor_Init( const char* configFileName )
{
  DEBUG_EVENT( 0, "Initializing Axis Sensor %s", configFileName );
  
  if( sensorsList == NULL ) sensorsList = kh_init( SensorInt );
  
  int configKey = (int) kh_str_hash_func( configFileName );
  
  int insertionStatus;
  khint_t newSensorID = kh_put( SensorInt, sensorsList, configKey, &insertionStatus );
  if( insertionStatus > 0 )
  {
    kh_value( sensorsList, newMotorID ) = LoadSensorData( configFileName );
    if( kh_value( sensorsList, newSensorID ) == NULL )
    {
      AxisSensor_End( (int) newSensorID );
      return -1;
    }
  }
  
  DEBUG_EVENT( 0, "Axis Sensor %s initialized (iterator: %d)", configFileName, (int) newSensorID );
  
  return (int) newSensorID;
}

static void AxisSensor_End( int sensorID )
{
  if( !kh_exist( sensorsList, (khint_t) sensorID ) ) return;
  
  Sensor sensor = kh_value( sensorsList, (khint_t) sensorID );
  
  UnloadSensorData( sensor );
    
  kh_del( SensorInt, sensorsList, (khint_t) sensorID );
    
  if( kh_size( sensorsList ) == 0 )
  {
    kh_destroy( SensorInt, sensorsList );
    sensorsList = NULL;
  }
}

static void AxisSensor_Reset( int sensorID )
{
  if( !kh_exist( sensorsList, (khint_t) sensorID ) ) return;
  
  Sensor sensor = kh_value( sensorsList, (khint_t) sensorID );
  
  sensor->interface->Reset( sensor->axisID );
}

static void AxisSensor_SetOffset( int sensorID )
{
  static double rawMeasuresList[ AXIS_DIMENSIONS_NUMBER ];
  
  if( !kh_exist( sensorsList, (khint_t) sensorID ) ) return 0.0;
  
  Sensor sensor = kh_value( sensorsList, (khint_t) sensorID );
  
  sensor->interface->ReadMeasures( sensor->axisID, rawMeasuresList );
  
  sensor->inputOffset = rawMeasuresList[ sensor->measureIndex ];
}

static bool AxisSensor_IsEnabled( int sensorID )
{
  if( !kh_exist( sensorsList, (khint_t) sensorID ) ) return false;
  
  Sensor sensor = kh_value( sensorsList, (khint_t) sensorID );
  
  return sensor->interface->IsEnabled( sensor->axisID );
}

static bool AxisSensor_HasError( int sensorID )
{
  if( !kh_exist( sensorsList, (khint_t) sensorID ) ) return false;
  
  Sensor sensor = kh_value( sensorsList, (khint_t) sensorID );
  
  return sensor->interface->HasError( sensor->axisID );
}

const double p1 = -5.6853e-024;
const double p2 = 9.5074e-020;
const double p3 = -5.9028e-016;
const double p4 = 1.6529e-012;
const double p5 = -2.0475e-009;
const double p6 = 9.3491e-007;
const double p7 = 0.0021429;
const double p8 = 2.0556;
static double AxisSensor_Read( int sensorID )
{
  static double rawMeasuresList[ AXIS_MEASURES_NUMBER ];
  
  if( !kh_exist( sensorsList, (khint_t) sensorID ) ) return;
  
  Sensor sensor = kh_value( sensorsList, (khint_t) sensorID );
  
  sensor->interface->ReadMeasures( sensor->axisID, rawMeasuresList );
  
  sensor->inputBuffer[5] = sensor->inputBuffer[4];
  sensor->inputBuffer[4] = sensor->inputBuffer[3];
  sensor->inputBuffer[3] = sensor->inputBuffer[2];
  sensor->inputBuffer[2] = sensor->inputBuffer[1];
  sensor->inputBuffer[1] = sensor->inputBuffer[0];
  sensor->inputBuffer[0] = rawMeasuresList[ sensor->measureIndex ] - sensor->inputOffset;
  
  double inputFiltered = (sensor->inputBuffer[0] + sensor->inputBuffer[1] + sensor->inputBuffer[2] + sensor->inputBuffer[3]+ sensor->inputBuffer[4] + sensor->inputBuffer[5])/6;
    
  double measure = p1 * pow(inputFiltered,7) + p2 * pow(inputFiltered,6) + p3 * pow(inputFiltered,5) 
                   + p4 * pow(inputFiltered,4) + p5 * pow(inputFiltered,3) + p6 * pow(inputFiltered,2) + p7 * inputFiltered + p8;   //mm
  
  return measure;
}

static inline Sensor LoadSensorData( const char* )
{
  Sensor newSensor = (Sensor) malloc( sizeof(SensorData) );
  
  // File Parsing
  
  
  newSensor->interface = &AxisCANEPOSOperations;
  newSensor->axisID = newSensor->interface->Connect( "CAN Sensor Teste" );
  
  return newSensor;
}

static inline void UnloadSensorData( Sensor sensor )
{
  if( sensor == NULL ) return;
  
  sensor->interface->Disconnect( sensor->axisID );
  
  free( sensor );
}

#ifdef __cplusplus
    }
#endif

#endif  // AXIS_SENSOR_H
