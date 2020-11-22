#include "scheduler.h"
#include <avr\io.h>

unsigned char s_ThreadAStack[0x80];

void ThreadA(void)
{
	unsigned int delay = 0;
	while (1)
	{
		PORTD |= (1 << PD0);
		while (++delay	< 100);
		PORTD &= ~(1 << PD0);
		while (--delay	< 100);
	}
}


// ***********************************************************
// Main program
//
int main(void)
{
   // Initialize the multitasking environment
	Multitask_Initialize();

	Multitask_CreateThread(ThreadA, s_ThreadAStack, sizeof(s_ThreadAStack));

	DDRD |= 1 << PD7 | 1 << PD0;

	unsigned int delay = 0;
	while (1)
	{
		PORTD |= (1 << PD7);
		while (++delay	< 100);
		PORTD &= ~(1 << PD7);
		while (--delay	< 100);
	}
}



