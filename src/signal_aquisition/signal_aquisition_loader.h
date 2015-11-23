#ifndef SIGNAL_AQUISITION_LOADER_H
#define SIGNAL_AQUISITION_LOADER_H

#include "plugin_loader.h"
#include "signal_aquisition/signal_aquisition_interface.h"

#include "debug/async_debug.h"

bool GetSignalAquisitionInterface( const char* interfaceFilePath, SignalAquisitionInterface* ref_interface )
{
  static char interfaceFilePathExt[ 256 ];
  
  sprintf( interfaceFilePathExt, "%s.%s", interfaceFilePath, PLUGIN_EXTENSION );
  
  DEBUG_PRINT( "trying to load plugin %s", interfaceFilePathExt );
  
  PLUGIN_HANDLE pluginHandle = LOAD_PLUGIN( interfaceFilePathExt );
  if( pluginHandle == NULL ) return false;
  
  DEBUG_PRINT( "found plugin %s (%p)", interfaceFilePathExt, pluginHandle );
  
  LOAD_PLUGIN_FUNCTIONS( SignalAquisition, ref_interface )
  
  DEBUG_PRINT( "plugin %s loaded", interfaceFilePathExt );
  
  return true;
}

#endif // SIGNAL_AQUISITION_LOADER_H