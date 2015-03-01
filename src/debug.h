///////////////////////////////////////////////////////////////////////
/////                  Error handling utilities                   /////
///////////////////////////////////////////////////////////////////////

#ifndef ERROR_H
#define ERROR_H

#include <stdio.h>
#include <stdlib.h>
  
#ifdef _MSC_VER
  #define INLINE __forceinline /* use __forceinline (VC++ specific) */
#else
  #define INLINE inline        /* use standard inline */
#endif

#define ERROR_PRINT( format, ... ) fprintf( stderr, "%s: ", __func__ ); fprintf( stderr, format,  __VA_ARGS__ );

#ifdef DEEP_DEBUG
  #define EVENT_DEBUG( format, ... ) printf( "%s: ", __func__ ); printf( format,  __VA_ARGS__ );
  #define LOOP_DEBUG( format, ... ) printf( "%s: ", __func__ ); printf( format,  __VA_ARGS__ );
#elif SIMPLE_DEBUG
  #define EVENT_DEBUG( format, ... ) printf( "%s: ", __func__ ); printf( format,  __VA_ARGS__ );
  #define LOOP_DEBUG( format, ... ) do { } while( 0 )
#else
  #define EVENT_DEBUG( format, ... ) do { } while( 0 )
  #define LOOP_DEBUG( format, ... ) do { } while( 0 )
#endif

extern INLINE void print_platform_error( const char* message )
{
  #ifdef WIN32
  fprintf( stderr, "%s: code: %d\n", message, GetLastError() );
  #else
  perror( message );
  #endif
}

#endif // ERROR_H
