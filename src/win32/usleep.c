#include <windows.h>

extern void
usleep (long usec)
{
	Sleep(usec / 1000);
}


