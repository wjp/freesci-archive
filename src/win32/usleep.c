#include <windows.h>
#include <sci_win32.h>
#include <stdio.h>

extern void
usleep (long usec)
{
	sleep(usec / 1000);
}
