/***************************************************************************
 commands_scr.c (C) 1999 Christoph Reichenbach, TU Darmstadt


 This program may be modified and copied freely according to the terms of
 the GNU general public license (GPL), as long as the above copyright
 notice and the licensing information contained herein are preserved.

 Please refer to www.gnu.org for licensing details.

 This work is provided AS IS, without warranty of any kind, expressed or
 implied, including but not limited to the warranties of merchantibility,
 noninfringement, and fitness for a specific purpose. The author will not
 be held liable for any damage caused by this work or derivatives of it.

 By using this source code, you agree to the licensing terms as stated
 above.


 Please contact the maintainer for bug reports or inquiries.

 Current Maintainer:

    Christoph Reichenbach (CJR) [creichen@rbg.informatik.tu-darmstadt.de]

***************************************************************************/
/* Commands related to the scripts */

#include <config.h>
#include <console.h>
#ifdef SCI_CONSOLE
#include <script.h>

extern int cmd_paramlength;
extern cmd_param_t *cmd_params;


object *
_find_object(int depth)
/* Returns the object described by the first 'depth' parameters,
** or null if no such object exists.
*/
{
  if (depth) {
    object *workobj = _find_object(depth - 1);
    int i;

    if (!workobj)
      return NULL;

    for (i = 0; i < workobj->children.used; i++) {
      if (strcmp(cmd_params[depth - 1].str,
		 workobj->children.data[i]->name) == 0)
	return workobj->children.data[i];
    }

    return NULL;

  } else return object_root;
}

void
_display_obj_info(object *obj)
{
  int i;

  if (obj->parent)
    sciprintf("%s<-%s:\n", obj->parent->name, obj->name);
  else
    sciprintf("root object: %s:\n", obj->name);

  sciprintf("Children: size=%d, used=%d\nChildren:\n",
	    obj->children.size, obj->children.used);

  for (i=0; i < obj->children.used; i++)
    sciprintf("  %s\n", obj->children.data[i]->name);

  sciprintf("Methods:%d\n", obj->method_count);
}

int
c_objinfo(state_t *s)
{
  object *workobj = _find_object(cmd_paramlength);

  if (workobj == NULL)
    sciprintf("No such class\n");
  else
    _display_obj_info(workobj);

  return 0;
}

int
c_objmethods(state_t *s)
{
  object *workobj = _find_object(cmd_paramlength);

  if (workobj == NULL)
    sciprintf("No such class\n");
  else
    sciprintf("Function is temporarily out of order\n");
    /*    printObject(workobj, SCRIPT_PRINT_METHODS); */

  return 0;
}


#endif /* SCI_CONSOLE */
