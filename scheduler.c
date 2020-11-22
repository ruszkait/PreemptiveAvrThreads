#include <avr\interrupt.h>
#include "scheduler.h"

// ================================================================================================================
// === Constants
// ================================================================================================================

#define MAIN_THREAD_HANDLE  0
#define EVENT_SIGNALED 1
#define EVENT_NOT_SIGNALED 0

// ================================================================================================================
// === Multitasking types
// ================================================================================================================

#define BoolT unsigned char

struct Multitask_ThreadContextT
{
	StackT m_StackPointer;
	MutexHandleT m_WaitsOnMutex;
	EventHandleT m_WaitsOnEvent;
};

struct Multitask_MultitaskInfoT
{
	ThreadHandleT m_ActiveThread;
	ThreadHandleT m_NumberOfThreads;
	struct Multitask_ThreadContextT m_ThreadContexts[MAX_THREADS];

	MutexHandleT m_NumberOfMutexes;
	// The Id of the thread that locked the mutex
	MutexHandleT m_MutexeLockedByThread[MAX_MUTEXES];

	EventHandleT m_NumberOfEvents;
	// Is the Event signaled
	BoolT m_EventSignaled[MAX_EVENTS];
};

// ================================================================================================================
// === Global variables
// ================================================================================================================

struct Multitask_MultitaskInfoT s_MultitaskInfo;
ThreadHandleT s_ThreadHandleCounter;

// ================================================================================================================
// === Multitasking functions
// ================================================================================================================

// Interrupt handler for Timer0 overflow
ISR(TIMER0_OVF_vect, ISR_NAKED)
{
	asm
	(
		// Save the used register
		"push __tmp_reg__"  "\n\t"
		 	
		// Check if the MCUCR register bit SE is set
		"in __tmp_reg__, %[E_MCUCR]"  "\n\t"
		"sbic  __tmp_reg__, %[E_SE]"  "\n\t"
		"rjmp interrupt_after_sleep" "\n\t"

		//	MCUCR bit SE was not set -> Normal preempting interrupt
		"pop __tmp_reg__" "\n\t"
		"rjmp Multitask_ScheduleThread" "\n\t"

		// MCUCR bit SE was set -> Interrupt after a sleep
		// This case we just restore the register and return to the point after the sleep
		"interrupt_after_sleep: " "\n\t"
		"pop __tmp_reg__" "\n\t"
		"reti" "\n\t"
		:: [E_MCUCR] "I" (_SFR_IO_ADDR(MCUCR)), [E_SE] "I" (SE)
	);
}

void Multitask_SetupTimer(void)
{
	// Set the prescaler
	TCCR0 = (1 << CS02 | 1 << CS00);
}

void Multitask_EnableTimerInterrupt(void)
{
	// Enable Timer 0 overflow interrupt
	TIMSK |= 1 << TOIE0;
}

void Multitask_DisableTimerInterrupt(void)
{
	// Disable Timer 0 overflow interrupt
	TIMSK &= ~(1 << TOIE0);
}


void Multitask_ClearTimer(void)
{
	// Reset the timer counter -> set the counter to 0
	TCNT0 = 0;
}

void Multitask_Initialize(void)
{
	// Initialize multithreading structures
	s_MultitaskInfo.m_NumberOfThreads = 1;
	s_MultitaskInfo.m_ActiveThread = MAIN_THREAD_HANDLE;
	
	s_MultitaskInfo.m_NumberOfMutexes = 0;
	s_MultitaskInfo.m_ThreadContexts[s_MultitaskInfo.m_ActiveThread].m_WaitsOnMutex = INVALID_MUTEX;
	
	s_MultitaskInfo.m_NumberOfEvents = 0;
	s_MultitaskInfo.m_ThreadContexts[s_MultitaskInfo.m_ActiveThread].m_WaitsOnEvent = INVALID_EVENT;

	// Setup the timer
	Multitask_SetupTimer();
	Multitask_EnableTimerInterrupt();
	Multitask_ClearTimer();
	
	// Enable interrupts -> multitasing starts
	asm("sei");
}

// Make the compiler to skip prolog and epilogue
void Multitask_ScheduleThread(void) __attribute__ ((naked));

void Multitask_ScheduleThread(void)
{
	//
	// Phase 1: Store the environment of the current thread
	//
	
	asm
	(
		// Clear the interrupts until the scheduling is done
		"cli" "\n\t"
		
		// Push the registers of the thread which is active before the scheduling
		"push r0" "\n\t"
		"push r1" "\n\t"
		"push r2" "\n\t"
		"push r3" "\n\t"
		"push r4" "\n\t"
		"push r5" "\n\t"
		"push r6" "\n\t"
		"push r7" "\n\t"
		"push r8" "\n\t"
		"push r9" "\n\t"
		"push r10" "\n\t"
		"push r11" "\n\t"
		"push r12" "\n\t"
		"push r13" "\n\t"
		"push r14" "\n\t"
		"push r15" "\n\t"
		"push r16" "\n\t"
		"push r17" "\n\t"
		"push r18" "\n\t"
		"push r19" "\n\t"
		"push r20" "\n\t"
		"push r21" "\n\t"
		"push r22" "\n\t"
		"push r23" "\n\t"
		"push r24" "\n\t"
		"push r25" "\n\t"
		"push r26" "\n\t"
		"push r27" "\n\t"
		"push r28" "\n\t"
		"push r29" "\n\t"
		"push r30" "\n\t"
		"push r31" "\n\t"
	
		// Save the status register
		"in __tmp_reg__, __SREG__" "\n\t"
		"push __tmp_reg__" "\n\t"
	);

	// Store the stack pointer in the thread context
	s_MultitaskInfo.m_ThreadContexts[s_MultitaskInfo.m_ActiveThread].m_StackPointer = (StackT)SP;

	//
	// Phase 2: Schedule the next task
	//

	while (1)
	{
		s_ThreadHandleCounter = s_MultitaskInfo.m_NumberOfThreads;
		// Try to find a thread that could be scheduled
		for (; 0 != s_ThreadHandleCounter; --s_ThreadHandleCounter)
		{
			// Take next thread
			++s_MultitaskInfo.m_ActiveThread;

			// Threads are in a ring buffer
			if (s_MultitaskInfo.m_NumberOfThreads == s_MultitaskInfo.m_ActiveThread)
				s_MultitaskInfo.m_ActiveThread = MAIN_THREAD_HANDLE;

			if (INVALID_MUTEX == s_MultitaskInfo.m_ThreadContexts[s_MultitaskInfo.m_ActiveThread].m_WaitsOnMutex &&
				 INVALID_EVENT == s_MultitaskInfo.m_ThreadContexts[s_MultitaskInfo.m_ActiveThread].m_WaitsOnEvent)
				{
					break;
				}

			// Schedule the thread if it is waiting on a mutex which is not locked by any thread
			if (INVALID_THREAD == s_MultitaskInfo.m_MutexeLockedByThread[s_MultitaskInfo.m_ThreadContexts[s_MultitaskInfo.m_ActiveThread].m_WaitsOnMutex])
			{
				// This thread is not waiting any more on that mutex
				s_MultitaskInfo.m_ThreadContexts[s_MultitaskInfo.m_ActiveThread].m_WaitsOnMutex = INVALID_MUTEX;
				break;
			}
			
			// Schedule the thread if it is waiting on an event which is signaled
			if (s_MultitaskInfo.m_EventSignaled[s_MultitaskInfo.m_ThreadContexts[s_MultitaskInfo.m_ActiveThread].m_WaitsOnEvent])
			{
				// Clear the event signaled state
				s_MultitaskInfo.m_EventSignaled[s_MultitaskInfo.m_ThreadContexts[s_MultitaskInfo.m_ActiveThread].m_WaitsOnEvent] = EVENT_NOT_SIGNALED;
				
				// This thread is not waiting any more on that event
				s_MultitaskInfo.m_ThreadContexts[s_MultitaskInfo.m_ActiveThread].m_WaitsOnEvent = INVALID_EVENT;
				
				break;
			}

		}

		
		// A thread was found to schedule, so leave the infinte cycle
		if (0 != s_ThreadHandleCounter)
		{
			break;
		}
		else
		{
			// No thread was found which could have been scheduled

			// Put the CPU into sleep. Only an external interrupt can do something
			// to make any of the threads scheduleable

			// Disable time interrupt, because it will not change the
			// the thread states -> this would just wake up the CPU in vain
			Multitask_DisableTimerInterrupt();

			// Set the CPU to Idle mode

			// Set the MCUCR bit SE
			MCUCR |= 1 << SE;
			
			asm
			(
				// Enable interrupts and start sleeping
				"sei" "\n\t"
				"sleep" "\n\t"
				// After the sleep the CPU executes the ISR
				// Then the execution continues here
				"cli" "\n\t"
			);
			
			// Clear the MCUCR bit SE
			MCUCR &= ~(1 << SE);
			
			
			// Now enable the timer interrupt again
			Multitask_EnableTimerInterrupt();
		}
	}

	// Reset the timer -> the time slice given to the new thread will be
	// not reduced by the scheduling delay
	Multitask_ClearTimer();

	//
	// Phase 3: Activate the environment of the new thread
	//


	// Set the stack pointer to the stack of the new thread
	SP = (int)s_MultitaskInfo.m_ThreadContexts[s_MultitaskInfo.m_ActiveThread].m_StackPointer;
		
	asm
	(
		// Restore the Status register
		"pop __tmp_reg__" "\n\t"
		"out __SREG__, __tmp_reg__" "\n\t"

		// Pop the registers of the thread which is active after the scheduling
		"pop r31" "\n\t"
		"pop r30" "\n\t"
		"pop r29" "\n\t"
		"pop r28" "\n\t"
		"pop r27" "\n\t"
		"pop r26" "\n\t"
		"pop r25" "\n\t"
		"pop r24" "\n\t"
		"pop r23" "\n\t"
		"pop r22" "\n\t"
		"pop r21" "\n\t"
		"pop r20" "\n\t"
		"pop r19" "\n\t"
		"pop r18" "\n\t"
		"pop r17" "\n\t"
		"pop r16" "\n\t"
		"pop r15" "\n\t"
		"pop r14" "\n\t"
		"pop r13" "\n\t"
		"pop r12" "\n\t"
		"pop r11" "\n\t"
		"pop r10" "\n\t"
		"pop r9" "\n\t"
		"pop r8" "\n\t"
		"pop r7" "\n\t"
		"pop r6" "\n\t"
		"pop r5" "\n\t"
		"pop r4" "\n\t"
		"pop r3" "\n\t"
		"pop r2" "\n\t"
		"pop r1" "\n\t"
		"pop r0" "\n\t"
	
		// Jump to the new thread
		"reti" "\n\t"
	);
}

ThreadHandleT Multitask_CreateThread(void (*p_EntryPoint)(void), StackT p_StackBuffer, StackSizeT p_StackSize)
{
	// Disable the preemption
	// We can make sure that we will be not preemted here and leave
	// the context data structures in an inconsistent state
	Multitask_DisableTimerInterrupt();
		
	// Allocate a new thread handle
	if (MAX_THREADS == s_MultitaskInfo.m_NumberOfThreads)
		return INVALID_THREAD;

	ThreadHandleT NewThreadHandle = s_MultitaskInfo.m_NumberOfThreads++;

	// Prepare the stack

	// Set the stack pointer to the stack begin
	// Set stack pointer at the end of stack buffer
	StackT p_StackPointer = p_StackBuffer + (p_StackSize - 1);

	// Make space in the stack for the return point (this is the entry point of the function)
	p_StackPointer = p_StackPointer - (sizeof(void (*)(void)) - 1);

	// The current stack pointer is a pointer (last *) to a function pointer void(*)(void)
	// Then this pointer is dereferneced to the function pointer (frist *) and
	// we assign the entry point pointer to this function pointer
	// *((void (**)(void))p_StackPointer) = p_EntryPoint;
	
	// [%a0,%a0+1] = "e" p_StackPointer = *p_StackPointer
	// [%a1,%a1+1] = "e" &p_EntryPoint = &*p_EntryPoint = p_EntryPoint
	asm
	(
		"ldd __tmp_reg__, %a1+1" "\n\t"
		"st %a0+, __tmp_reg__" "\n\t"
		"ld __tmp_reg__, %a1" "\n\t"
		"st %a0, __tmp_reg__" "\n\t"
		:: "e" (p_StackPointer), "e" (&p_EntryPoint)
	);
	
	// Initial register values.
	// Set all 32 registers + SREG are set to 0
	for (unsigned char RegisterIndex = 32; 0 != RegisterIndex; --RegisterIndex)
	{
		--p_StackPointer;
		*p_StackPointer = 0x00;
	}

	// Push the SREG, Interrupt flag enabled
	--p_StackPointer;
	*p_StackPointer = 0x80;

	// Initilize the new context
	struct Multitask_ThreadContextT *NewThreadContext = &s_MultitaskInfo.m_ThreadContexts[NewThreadHandle];
	// Save the stack pointer of the new thread
	NewThreadContext->m_StackPointer = --p_StackPointer;
	// Init syncronization info
	NewThreadContext->m_WaitsOnMutex = INVALID_MUTEX;
	NewThreadContext->m_WaitsOnEvent = INVALID_EVENT;

	// Enable preemption
	Multitask_EnableTimerInterrupt();

	// Returns the Id of the new thread
	return NewThreadHandle;
}







