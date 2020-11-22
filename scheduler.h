#ifndef SCHEDULER_H
#define SCHEDULER_H

// ================================================================================================================
// === Includes
// ================================================================================================================

#include <stdint.h>

// ================================================================================================================
// === Configuration
// ================================================================================================================

// Thread configuration
#define MAX_THREADS 2

// Mutex configuration
//#define MUTEXES_SUPPORTED 1
#define MAX_MUTEXES 0

// Event configuration
//#define EVENTS_SUPPORTED 1
#define MAX_EVENTS 0


// ================================================================================================================
// === Constants
// ================================================================================================================

#define INVALID_EVENT 200
#define INVALID_MUTEX 200
#define INVALID_THREAD 200

// ================================================================================================================
// === Type definitions
// ================================================================================================================

/*
typedef uint8_t ThreadHandleT;
typedef uint8_t MutexHandleT;
typedef uint8_t EventHandleT;
typedef void (*ThreadFunctionT)(void);
typedef uint8_t* StackT;
typedef uint8_t StackSizeT;
*/


#define ThreadHandleT uint8_t
#define MutexHandleT uint8_t
#define EventHandleT uint8_t
#define ThreadFunctionT void (*)(void)
#define StackT uint8_t*
#define StackSizeT uint8_t


// ================================================================================================================
// === API functions
// ================================================================================================================

// Initialite the Multitasking
void Multitask_Initialize(void);

// Create a new thread
ThreadHandleT Multitask_CreateThread(void (*p_EntryPoint)(void), StackT p_StackBuffer, StackSizeT p_StackSize);

#endif

