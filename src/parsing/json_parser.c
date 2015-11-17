#ifndef JSON_PARSER_H
#define JSON_PARSER_H

#include "parsing/parser_interface.h"

#include "klib/kson.h"
#include "klib/khash.h"

#include "debug/async_debug.h"

typedef struct _JSONData
{
  kson_t* nodeTree;
  kson_node_t* currentNode;
  char searchPath[ PARSER_MAX_KEY_PATH_LENGTH ];
}
JSONData;
                                                
KHASH_MAP_INIT_INT( JSONInt, JSONData )
static khash_t( JSONInt )* jsonDataList = NULL;

IMPLEMENT_INTERFACE( PARSER_FUNCTIONS )

int LoadStringData( const char* configString )
{
  if( configString == NULL ) return PARSED_DATA_INVALID_ID;
  
  if( jsonDataList == NULL ) jsonDataList = kh_init( JSONInt );
  
  int dataKey = (int) kh_size( jsonDataList );
  
  int insertionStatus;
  khint_t newDataIndex = kh_put( JSONInt, jsonDataList, dataKey, &insertionStatus );
  DEBUG_PRINT( "Trying to insert key: %d (status: %d)", dataKey, insertionStatus );
  if( insertionStatus > 0 )
  {
    //DEBUG_PRINT( "data content: %s", configString );                                           
    kh_value( jsonDataList, newDataIndex ).nodeTree = kson_parse( (const char*) configString );
    
    if( kh_value( jsonDataList, newDataIndex ).nodeTree == NULL )
    {
      UnloadData( (int) newDataIndex );
      return PARSED_DATA_INVALID_ID;
    }
  
    kh_value( jsonDataList, newDataIndex ).currentNode = kh_value( jsonDataList, newDataIndex ).nodeTree->root;

    //DEBUG_PRINT( "Found %lu keys in new JSON data", (size_t) kh_value( jsonDataList, newDataIndex ).nodeTree->root->n );
  }
  
  return (int) kh_key( jsonDataList, newDataIndex );
}

int LoadFileData( const char* filePath )
{
  if( filePath == NULL ) return PARSED_DATA_INVALID_ID;
  
  char filePathExt[ PARSER_MAX_FILE_PATH_LENGTH ];
  sprintf( filePathExt, "%s.json", filePath );
  
  DEBUG_PRINT( "looking for file: %s", filePathExt );
  
  FILE* configFile = fopen( filePathExt, "r" );
  if( configFile == NULL ) return PARSED_DATA_INVALID_ID;
  
  fseek( configFile, 0, SEEK_END );
  long int fileSize = ftell( configFile );
  char* configString = (char*) calloc( fileSize + 1, sizeof(char) );
  fseek( configFile, 0, SEEK_SET );
  fread( configString, sizeof(char), fileSize, configFile );
  fclose( configFile );  
  
  int newDataID = LoadStringData( configString );

  free( configString );
  
  if( newDataID != PARSED_DATA_INVALID_ID ) DEBUG_PRINT( "file %s data inserted", filePathExt );

  return newDataID;
}

void UnloadData( int dataID )
{
  khint_t dataIndex = kh_get( JSONInt, jsonDataList, (khint_t) dataID );
  if( dataIndex == kh_end( jsonDataList ) ) return;
    
  kson_destroy( kh_value( jsonDataList, dataIndex ).nodeTree );
    
  kh_del( JSONInt, jsonDataList, dataIndex );
    
  if( kh_size( jsonDataList ) == 0 )
  {
    kh_destroy( JSONInt, jsonDataList );
    jsonDataList = NULL;
  }
}

static inline const kson_node_t* GetPathNode( int dataID, const char* pathFormat, va_list pathArgs )
{
  khint_t dataIndex = kh_get( JSONInt, jsonDataList, (khint_t) dataID );
  if( dataIndex == kh_end( jsonDataList ) ) return NULL;
  
  const kson_node_t* currentNode = kh_value( jsonDataList, dataIndex ).currentNode;
  char* searchPath = kh_value( jsonDataList, dataIndex ).searchPath;
  
  vsnprintf( searchPath, PARSER_MAX_KEY_PATH_LENGTH, pathFormat, pathArgs );
  
  //DEBUG_PRINT( "Search path: %s", searchPath );
  
  for( char* key = strtok( searchPath, "." ); key != NULL; key = strtok( NULL, "." ) )
  {
    if( currentNode == NULL ) break;
        
    if( currentNode->type == KSON_TYPE_BRACE )
      currentNode = kson_by_key( currentNode, key );
    else if( currentNode->type == KSON_TYPE_BRACKET )
      currentNode = kson_by_index( currentNode, strtoul( key, NULL, 10 ) );
  }
    
  return currentNode;
}

char* GetStringValue( int dataID, char* defaultValue, const char* pathFormat, ... )
{
  va_list pathArgs;
  va_start( pathArgs, pathFormat );  
  const kson_node_t* valueNode = GetPathNode( dataID, pathFormat, pathArgs );
  va_end( pathArgs );
  if( valueNode == NULL ) return defaultValue;
  
  if( valueNode->type != KSON_TYPE_SGL_QUOTE && valueNode->type != KSON_TYPE_DBL_QUOTE )
    return defaultValue;
  
  if( valueNode->v.str == NULL ) return defaultValue;
  
  //DEBUG_PRINT( "Found value: %s", valueNode->v.str );
  
  return valueNode->v.str;
}

long GetIntegerValue( int dataID, long defaultValue, const char* pathFormat, ... )
{
  va_list pathArgs;
  va_start( pathArgs, pathFormat );  
  const kson_node_t* valueNode = GetPathNode( dataID, pathFormat, pathArgs );
  va_end( pathArgs );
  if( valueNode == NULL ) return defaultValue;
  
  if( valueNode->type != KSON_TYPE_NO_QUOTE ) return defaultValue;
  
  //DEBUG_PRINT( "Found value: %ld", strtol( valueNode->v.str, NULL, 0 ) );
  
  return strtol( valueNode->v.str, NULL, 0 );
}

double GetRealValue( int dataID, double defaultValue, const char* pathFormat, ... )
{
  va_list pathArgs;
  va_start( pathArgs, pathFormat );  
  const kson_node_t* valueNode = GetPathNode( dataID, pathFormat, pathArgs );
  va_end( pathArgs );
  if( valueNode == NULL ) return defaultValue;
  
  if( valueNode->type != KSON_TYPE_NO_QUOTE ) return defaultValue;
  
  //DEBUG_PRINT( "Found value: %g", strtod( valueNode->v.str, NULL ) );
  
  return strtod( valueNode->v.str, NULL );
}

bool GetBooleanValue( int dataID, bool defaultValue, const char* pathFormat, ... )
{
  va_list pathArgs;
  va_start( pathArgs, pathFormat );  
  const kson_node_t* valueNode = GetPathNode( dataID, pathFormat, pathArgs );
  va_end( pathArgs );
  if( valueNode == NULL ) return defaultValue;
  
  if( valueNode->type != KSON_TYPE_NO_QUOTE ) return defaultValue;
  
  if( strcmp( valueNode->v.str, "true" ) == 0 ) return true;

  return false;
}

size_t GetListSize( int dataID, const char* pathFormat, ... )
{
  va_list pathArgs;
  va_start( pathArgs, pathFormat );  
  const kson_node_t* listNode = GetPathNode( dataID, pathFormat, pathArgs );
  va_end( pathArgs );  
  if( listNode == NULL ) return 0;
  
  if( listNode->type != KSON_TYPE_BRACKET ) return 0;
  
  //DEBUG_PRINT( "List %s size: %u", pathFormat, (size_t) listNode->n );
  
  return (size_t) listNode->n;
}

bool HasKey( int dataID, const char* pathFormat, ... )
{
  va_list pathArgs;
  va_start( pathArgs, pathFormat );  
  const kson_node_t* keyNode = GetPathNode( dataID, pathFormat, pathArgs );
  va_end( pathArgs );
  if( keyNode == NULL ) return false;
  
  return true;
}

#endif // JSON_PARSER_H
