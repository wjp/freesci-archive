The C File Storage Meta Language (CFSML)
----------------------------------------

Specification of version 0.8


This documentation is provided WITHOUT WARRANTY of any kind.



Purpose of the CFSML
--------------------
It's damn hard to write functions that read and write C records to or from
a text file and keep them in sync when changes are made to that record.
The CFSML strives to help here at least to some extent, by providing
special preprocessor commands that take care of those problems.


Comments and syntax
-------------------
Comments are shell-style; they start with a hash ("#") sign and extend
to the end of the line. Declarations may be followed by semicolons to
keep the EMACS autoformatter satisfied, but this is not required.


CFSML declarations
------------------
Each CFSML file contains one CFSML block, starting with a single line stating
"%CFSML", and ending with a single line stating "%END CFSML". This is the
CFSML declaration block; the cfsml preprocessor will replace it with functions
for saving/reading the data types declared there.
CFSML implementation need not support more than one CFSML declaration block
per file.

The declaration block contains only type declarations and type meta-
declarations. A declaration looks like this:

TYPE [typename] "C representation" USING [writer] [reader];

for example

TYPE string "char *" USING stringwriter stringreader;

to declare the type "string", which is used for char *. The functions
stringwriter and stringreader must have been declared before the CFSML
block, and they must look like this:

void
stringwriter(FILE *fd, char **foo)

and

void
stringreader(FILE *fd, char **foo, char *lastval, int *line, int *hiteof)

Generally, they must take one FILE* and one pointer to a variable of
the specified type as parameters. The reader function also takes
the parameters "char *lastval", a dynamically allocated string containing
the line that was read last and should be interpreted before reading
more data from the file (Reading ahead is required in order to find the
ends of records and arrays).

For example, if something like
foobar = [3][
  1
  2
]

was being parsed (foobar being an int array), then the int reader function
would be called with "1" as a parameter (leading and trailing whitespace is
stripped). It would not need to read anything from the file. (The file
descriptor is passed for multi-line structures like arrays or records).


However, CFSML comes with two built-in variable types: int and
string. This doesn't only remove the need to declare those, it also allows
one special exception to the TYPE declaration: "LIKE xxx" instead of "USING
foo bar",  where xxx is one of those types (int should be most common). This
simply means that the variable is written and read like one of those variables;
CFSML will automagically create reading and writing functions for
them.
Please note that int includes the C type "long int"; you can write and
read "long" variables 'LIKE int'.

Example:
TYPE short "short" LIKE int;

would generate

void
_cfsml_write_short(FILE *fd, short *foo)
{
	...
}

void
_cfsml_read_short(FILE *fd, short *foo, char *lastval, int *line, int *hiteof)
{
	...
}



Records and Arrays
------------------
It is also possible to declare records, like this:

RECORD [recordname] "C representation" EXTENDS [super_record] {
	[member #1];
	[member #2];
	...
}

with the members looking like this:

[declared type] [identifier];

for example:

RECORD baserec {
	int foo;
	int bar;

	int frobmeharder[4];
}

This example introduces two additional features: The possibility to omit the
C representation if it is equal to the identifier, and arrays. Arrays are
used similarly to the way they are treated in C, except that special keywords
must be used to describe dynamic arrays.

Dynamically allocated block arrays are also possible:

RECORD baserec {

	int myarray1[DYNAMIC 4];

	int length;
	int myarray2[DYNAMIC length];
}


Here, length elements of myarray2 will be stored; during restore time,
myarray2 will be set to the result of a malloc(sizeof(int) * length),
and each of the elements will be restored. This is fairly straightforward.

The following keywords can be used inside arrays:
- STATIC/DYNAMIC: Use to define array types. If ommited, static is assumed.
  Dynamic arrays will get the memory they requre allocated during restoration
  time.
- MAXWRITE followed by a int-like variable member of the underlying record
  that is used to limit the number of elements to be written / read.

If the dynamic/static keyword is followed by a string instead of a numeric
value, the length is assumed to be variable and equal to that string token
if interpreted as a member of the underlying structure (see the example if
this sounds too weird).


Here is another example for a dynamically resized stack:

RECORD dynstack {

	int memsize; # Amount of memory allocated
	int tos;     # Top-of stack element

	int data[memsize, MAXWRITE tos+1];

}

This would write only the data up to and including tos+1, unless tos was
negative (in this case, no entries would be written) or greater than
memsize (in that case, only memsize entries would be written).


It is currently required that all elements inside the brackets must be
declared before the record using them.


Records built on top of other records
-------------------------------------
Sometimes, when doing OOish stuff, several separate types share a common
ancestor that defines their contents. In C, this is typically facilitated
by using some kind of macros; CFSML supports this via the EXTENDS keyword.
Example:

RECORD foo {
    int x;
    int y;
}

RECORD bar "extended_bar_t" EXTENDS foo {
    int z;
}

The declaration of bar has the same semantics as the explicit

RECORD bar "extended_bar_t" {
    int x;
    int y;
    int z;
}

but, if foo is changed, bar need not be changed explicitly, making
maintenance easier.


Error reporting during file recovery
------------------------------------
During read time, errors are reported using a built-in function
(_cfsml_error()). This function prints any error reports to stderr.
It is not yet possible to replace that function or to add additional
information (like the file name) to its output. This may be added to
the specifications at a later time.


Pointers
--------
It is possible to store relative pointers. This feature should
be used with care; for portability, only pointers relative to other
pointers pointing to the same data type should be stored.

Example:

RECORD name "struct name" {

	string fullname;
	POINTER firstname RELATIVETO fullname;
	POINTER lastname RELATIVETO fullname;
}

This will only work if firstname and lastname are, indeed, relative to the
string pointed to by "fullname".

Also, absolute pointers are supported. The syntax is similar, but not identical
to the one employed in C:

	int * foo;

(Note the asterisk must be separated from type and variable name by whitespace).
Saving these pointers is relatively straightforward; it will, however, cause
problems if foo points to an invalid memory address. Empty pointers MUST be NULL
for this to work.

While restoring, it is assured that no memory is lost. All allocations are kept
track of, and all memory allocated dynamically is automatically freed if
restoring failed. This holds until the next atomic restoration operation is
encountered.



Reading and writing
-------------------
To read data into a variable of a previously defined data type, use
%CFSMLREAD type dataptr FROM filehandle [ERRVAR err_variable]
	 [FIRSTTOKEN token] [LINECOUNTER counter]
where
	- type is the type of *dataptr
	- dataptr is a pointer to a variable or struct of the specified type
	- filehandle is the file handle of an open and readable FILE* that
	  should be parsed.
	- err_variable is an optional variable which will be set to != 0 if
	  reading was aborted because of any error.
	- token is an optional first token that can be supplied to the reader
	  function; this is useful if you want to use %CFSMLREAD inside a
	  function that might be called by CFSML generated code and passed a
	  token that it wants to pass on
	- counter is an optional variable that will be increased each time a
	  newline is hit. If not supplied, a CFSML generated variable will be
	  used for this task.

For an atomic read, use %CFSMLREAD-ATOMIC. Atomic reads guarantee that, if
they fail, all memory allocated by them will be freed, unless another atomic
read was hit by recursion.
Only after a %CFSMLREAD-ATOMIC command is it guaranteed that the previous
command did not leave any allocated memory. Typically, this call would be
the actual call invoked from the rest of the program, while other %CFSMLREAD
commands would be invoked by recursion.

To write data, use %CFSMLWRITE similarly (with a writeable file
handle, of course):
%CFSMLWRITE type dataptr INTO filehandle



Artificial limitations
----------------------
- Not all data type possibilities can be covered cleanly (like static strings
  or multi-dimensional records).
  Oh, and be careful with doubly-linked lists ;-)


---
This is not yet version 1.0 of the spec, so additions or changes are
possible until the final release. Feel free to suggest improvements;
however, their chance of being accepted is much more likely with code
to back them up.


Changes since 0.7:
- Removed "*" in "[*]", replaced it with the normal unary pointer
  operator
Changes since 0.6:
- Added EXTENDS
- Added "*"
Changes since 0.5.1:
- Added FIRSTTOKEN
- Added LINECOUNTER
Changes since 0.5:
- Renamed "EOFVAR" to "ERRVAR"
Changes since 0.4:
- Added "EOFVAR" construct for %CFSMLREAD.


The author can be contacted at jameson@linuxgames.com.

  Christoph Reichenbach, 01/2000

