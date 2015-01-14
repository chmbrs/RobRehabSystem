///////////////////////////////////////////////////////////////////////////////
///// Wrapper library for thread management (creation and syncronization) ///// 
///// using low level operating system native methods (Posix Version)     /////
///////////////////////////////////////////////////////////////////////////////

#ifndef THREADS_H
#define THREADS_H

#ifdef __cplusplus
extern "C"{
#endif

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define INFINITE 0xffffffff

// Mutex aquisition and release
#define LOCK_THREAD( lock ) pthread_mutex_trylock( lock )
#define UNLOCK_THREAD( lock ) pthread_mutex_unlock( lock )

// Returns unique identifier of the calling thread
#define THREAD_ID pthread_self()

// Controls the thread opening mode. JOINABLE if you want the thread to only end and free its resources
// when calling wait_thread_end on it. DETACHED if you want it to do that by itself.
enum { DETACHED, JOINABLE };


// Aliases for platform abstraction
typedef pthread_t Thread_Handle;
typedef pthread_mutex_t* Thread_Lock;

// List of created mutexes
static pthread_mutex_t* lock_list;
static size_t n_locks = 0;

// List of manipulators for the created (joinable) threads
static pthread_t* handle_list;
static size_t n_handles = 0;

// Number of running threads
static size_t n_threads = 0;

// Request new unique mutex for using in thread syncronization
Thread_Lock get_new_thread_lock()
{
  lock_list = (pthread_mutex_t*) realloc( lock_list, ( n_locks + 1 ) * sizeof(pthread_mutex_t) );
  pthread_mutex_init( &lock_list[ n_locks ], NULL );
  return &lock_list[ n_locks++ ];
}

// Setup new thread to run the given method asyncronously
Thread_Handle run_thread( void* (*function)( void* ), void* args, int mode )
{
  static pthread_t handle;
  
  if( pthread_create( &handle, NULL, function, args ) != 0 )
  {
    perror( "run_thread: pthread_create: error creating new thread:" );
    return 0;
  }

  #ifdef DEBUG_2
  printf( "run_thread: created thread %x successfully\n", handle_list[ n_handles ] );
  #endif

  n_threads++;
  
  if( mode == JOINABLE )
  {
	handle_list = (pthread_t*) realloc( handle_list, ( n_handles + 1 ) * sizeof(pthread_t) );
	handle_list[ n_handles ] = handle;
	n_handles++;
  }
  else if( mode == DETACHED ) 
	pthread_detach( handle );

  return handle;
}

// Exit the calling thread, returning the given value
void exit_thread( uint32_t exit_code )
{
  static uint32_t exit_code_storage;
  exit_code_storage = exit_code;
  n_threads--;

  #ifdef DEBUG_2
  printf( "exit_thread: thread exiting with code: %u\n", exit_code );
  #endif

  pthread_exit( &exit_code_storage );
}

// Wait for the thread of the given manipulator to exit and return its exiting value
uint32_t wait_thread_end( Thread_Handle handle, unsigned int milisseconds )
{
  static void* exit_code_ref = NULL;
  static struct timespec timeout;
  
  timeout.tv_sec = (time_t) INFINITE /*( milisseconds / 1000 )*/;
  timeout.tv_nsec = (long) 1000000 * ( milisseconds % 1000 );
  
  #ifdef DEBUG_1
  printf( "wait_thread_end: waiting thread %x\n", handle );
  #endif
  
  if( pthread_timedjoin_np( handle, &exit_code_ref, &timeout ) != 0 )
  {
    perror( "wait_thread_end: pthread_timedjoin_np: error waiting for thread end" );
    return 0;
  }
  
  #ifdef DEBUG_1
  printf( "wait_thread_end: thread returned\n" );
  #endif
  
  if( exit_code_ref != NULL )
  {
    #ifdef DEBUG_1
    printf( "wait_thread_end: thread exit code: %u\n", *((uint32_t*) exit_code_ref) );
    #endif
    
    return *((uint32_t*) exit_code_ref);
  }
  else
    return 0;
}

// Returns number of running threads (method for encapsulation purposes)
size_t get_threads_number()
{
  return n_threads;
}

// Handle the destruction of the remaining threads and mutexes when the program quits
void end_threading()
{  
  int id;
  
  for( id = 0; id < n_locks; id++ )
    pthread_mutex_destroy( &(lock_list[ id ]) );
  
  for( id = 0; id < n_handles; id++ )
    pthread_detach( handle_list[ id ] );
  
  return;
}

#ifdef __cplusplus
}
#endif

#endif /* THREADS_H */
