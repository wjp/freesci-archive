#ifndef UTIL_H
#define UTIL_H

#include <stdlib.h>
#include <stdio.h>

/* An excercise in bloated macro writing ;-)*/

/*Declare a flexarray type*/
#define FLEXARRAY(type, extra) struct{\
	type* data;\
	int used, size, itemsize;\
	extra\
}

/*Initialize a flexarray*/
#define FLEXARRAY_INIT(type, array) do {(array).itemsize=sizeof(type); (array).size=0; (array).used=0;} while(0)

/*Prints an error message and returns (from the function using the macro)*/
#define FLEXARRAY_PANIC(message, errorcode) do{\
	fprintf##message;\
	errorcode;\
} while(0)

/*Use if the state of the array is incnsistent*/
#define FLEXARRAY_PANIC_BADSTATE(array, errorcode) FLEXARRAY_PANIC((stderr, "PANIC: flexarray "#array" is in an inconsistent state (%d,%d).\n", (array).used, (array).size), errorcode)

/*Use if memory allocation fails*/
#define FLEXARRAY_PANIC_MEMORY(array, bytes, errorcode) FLEXARRAY_PANIC((stderr, "PANIC: Unable to allocate %d bytes for flexarray"#array"\n", bytes), errorcode)

/*Allocates an initial array*/
#define FLEXARRAY_ALLOCATE_INITIAL(array, errorcode) do{\
	(array).data=malloc(4*(array).itemsize);\
	if((array).data==0) FLEXARRAY_PANIC_MEMORY(array, 4*(array).itemsize, errorcode);\
	(array).size=4;\
	(array).used=0;\
} while(0)

/*Doubles the size of the allocated area*/
#define FLEXARRAY_RESIZE(array, errorcode) do{\
	int size=(array).size*2*(array).itemsize;\
	(array).data=realloc((array).data, size);\
	if((array).data==0) FLEXARRAY_PANIC_MEMORY(array, size, errorcode);\
	(array).size*=2;\
} while(0)

/*Appends /value/ at the end of the array, resizing as necessary*/
#define FLEXARRAY_APPEND(array, value, errorcode) do\
{\
	if((array).used>=(array).size)\
	{\
		if((array).size==0) FLEXARRAY_ALLOCATE_INITIAL(array, errorcode);\
		else {\
			if((array).used==(array).size) FLEXARRAY_RESIZE(array, errorcode);\
			else FLEXARRAY_PANIC_BADSTATE(array, errorcode);\
		}\
	}\
	(array).data[(array).used++]=(value);\
}while(0)

/*Adds space for a value at the end of the array, resizing as necessary, but
 *does not initialize the value*/
#define FLEXARRAY_ADD_SPACE(array, items, errorcode) do{\
	if((array).used+items>(array).size)\
	{\
		if((array).size==0) FLEXARRAY_ALLOCATE_INITIAL(array, errorcode);\
		else if((array).used==(array).size) FLEXARRAY_RESIZE(array, errorcode);\
		else FLEXARRAY_PANIC_BADSTATE(array, errorcode);\
	}\
	(array).used++;\
} while(0)

#endif
