#include <console.h>
#include <script.h>
#include <vocabulary.h>
#include <stdio.h>
#include <stdlib.h>

int main()
{
	con_passthrough = 1;
	loadResources(SCI_VERSION_AUTODETECT, 1);

	if(loadObjects())
	{
		fprintf(stderr, "Unable to load object hierarchy\n");
		return 1;
	}

	printObject(object_root, SCRIPT_PRINT_METHODS|SCRIPT_PRINT_CHILDREN);
	return 0;
}
