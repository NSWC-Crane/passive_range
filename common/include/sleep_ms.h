#ifndef _UNIVERSAL_MS_SLEEP_H
#define _UNIVERSAL_MS_SLEEP_H

#if defined(_WIN32) | defined(__WIN32__) | defined(__WIN32) | defined(_WIN64) | defined(__WIN64)
	#include <windows.h>
#endif

#include <cstdint>
#include <ctime>


// create a sleep function that can be used in both Windows and Linux
void sleep_ms(uint32_t value)
{

#if defined(_WIN32) | defined(__WIN32__) | defined(__WIN32) | defined(_WIN64) | defined(__WIN64)
	Sleep(value);
#else
	const timespec delay[]= {0, (uint32_t)(value*1000000)} ;
	nanosleep(delay, NULL);
#endif

}   // end of sleep_ms

#endif  // _UNIVERSAL_MS_SLEEP_H
