#include <console.h>
#include <script.h>
#include <vocabulary.h>
#include <old_objects.h>
#include <stdio.h>
#include <stdlib.h>

void
graph_update_box(state_t *s, int x,int y,int z,int w)
{}; /* Dummy because of braindead library design */

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
