/* VM and kernel declarations */

#include <script.h>


typedef unsigned char byte;

struct freelist_node
{
  int offset;
  int size;
  struct freelist_node* next;
};
typedef struct
{
  byte data[0x10000];
  struct freelist_node* freelist;
} heap;

typedef struct
{
  int id;
  object* class;
  byte* heap;
  int offset;
} instance;

typedef struct
{
  heap* heap;
  int acc;
  int sp;
  int pp;
  int previous; /*for pprev*/
} state;

typedef int kernel_function(state* s);

extern kernel_function* kfuncs[];
extern instance* instance_map[];
extern int max_instance;


int emulate(state* s, object* obj, int method);
instance* instantiate(heap* h, object* o);
