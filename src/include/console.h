/***************************************************************************
 sci_console.h Copyright (C) 1999 Christoph Reichenbach, TU Darmstadt


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
/* Header file for the SCI console.
** Please note that the console does not use the priority field; the console
** should therefore be drawn after everything else has been drawn (with the
** possible exception of the mouse pointer).
*/

#ifndef _SCI_CONSOLE_H_
#define _SCI_CONSOLE_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#ifdef _DOS
#include <sci_dos.h>
#endif

#include <engine.h>
#include <graphics.h>
#include <uinput.h>
#define SCI_CONSOLE

#define SCI_CONSOLE_MAX_INPUT 157
/* Max number of chars in the input rows+1 */
#define SCI_CONSOLE_INPUT_BUFFER 80
/* Command line history buffer size */

#define SCI_CONSOLE_OUTPUT_BUFFER 2048
/* Output buffer size in lines*/
#define SCI_CONSOLE_LINE_WIDTH 40
/* Width of the output line */


#define SCI_CONSOLE_BGCOLOR 0x01
/* Background color of the console */
#define SCI_CONSOLE_BORDERCOLOR 0x08
/* Color of the console border */
#define SCI_CONSOLE_FGCOLOR 0x0f
/* Foreground text color */
#define SCI_CONSOLE_INPUTCOLOR 0x0e
/* Color of input prompt and text */
#define SCI_CONSOLE_CURSORCOLOR 0x04
/* Color of the command line cursor */

extern int con_display_row;
/* Row displayed in the line closest to the bottom of the screen */
extern int con_row_counter;
/* Number of rows, >= con_display_row */
extern int con_visible_rows;
/* number of visible console rows */
extern char *con_input_line;
/* input line */
extern int con_cursor;
/* cursor position on the input line */
extern int con_passthrough;
/* Echo all sciprintf() stuff to the text console */
extern FILE *con_file;
/* Echo all sciprintf() output to a text file */

/* Output buffer */
extern char con_outputbuf[SCI_CONSOLE_OUTPUT_BUFFER][SCI_CONSOLE_LINE_WIDTH];
extern int con_outputbufpos; /* buffer line */
extern int con_outputbufcolumn;
extern int con_outputbuflen;
extern int con_outputlookback;



struct _state; /* state_t later on */

typedef union {
  long val;
  char *str;
} cmd_param_t;

extern int cmd_paramlength;
/* The number of parameters passed to a function called from the parser */

extern cmd_param_t *cmd_params;
/* The parameters passed to a function called by the parser */

extern struct _state *con_gamestate;
/* The game state as used by some of the console commands */


/*** FUNCTION DEFINITIONS ***/


void
sciprintf(char *fmt, ...);
/* Prints a string to the console stack
** Parameters: fmt: a printf-style format string
**             ...: Additional parameters as defined in fmt
** Returns   : (void)
*/

char *
con_input(sci_event_t *event);
/* Handles an input event
** Parameters: event: The event to handle
** Returns   : (char *) Either NULL, or a pointer to the content of the
**             command line
** This function will handle input events in a way that will allow the
** editing of the command line buffer. It will reset the command line to
** an empty string and return its former contents if ENTER was pressed.
** It will set the event to SCI_NO_EVENT if it was processed.
*/



void
con_draw_string(picture_t pic, int map, int row, int maxlen, char *string, int color);
/* Draws a string to a specified row
** Parameters: pic: The picture_t to draw to
**             map: The map inside the picture_t to draw to
**             row: The absolute row to draw to
**             maxlen: The maximum length to draw (should be <= 40)
**             string: The string to draw
**             color: The color to draw the string in
** Returns   : (void)
*/


void
con_init(void);
/* Initializes the command parser
** Parameters: (void)
** Returns   : (void)
** This function will initialize hook up a few commands to the parser.
** It must be called before cmdParse() is used.
*/

void
con_init_gfx(void);
/* initializes the graphical part of the command parser
** Parameters: (void)
** Returns   : (void)
** This function should be called in all console-using programs that use libscigraphics.
*/


void
con_parse(struct _state *s, char *command);
/* Parses a command and summons appropriate facilities to handle it
** Parameters: (state_t *) s: The state_t to use
**             command: The command to execute
** Returns   : (void)
*/


int
con_hook_command(int command(struct _state *s), char *name, char *param, char *description);
/* Adds a command to the parser's command list
** Parameters: command: The command to add
**             name: The command's name
**             param: A description of the parameters it takes
**             description: A short description of what it does
** Returns   : 0 if successful, 1 if appending failed because
**             of an incorrect *param string, 'command'==0, or
**             'name' already being in use.
** A valid param string is either empty (no parameters allowed)
** or contains one of the following tokens:
** i (an int)
** s (a 'string' (char *))
** h (a byte, described in hexadecimal digits)
** x* (an arbitrary (possibly 0) number of 'x' tokens)
** The '*' token may only be used as the last token of the list.
**
** Please note that the 'h' type does accept hexadecimal numbers greater
** than 0xff and less than 0x00, but clips them to this range.
**
** Example: "isi*" would define the function to take an int, a
** string, and an arbitrary number of ints as parameters (in that sequence).
**
** When the function is called, it can retrieve its parameters from cmd_params;
** the actual number of parameters is stored in cmd_paramlength.
** It is allowed to modify the char*s from a cmd_params[] element, as long
** as no element beyond strlen(cmd_params[x].str)+1 is accessed.
*/


int
con_hook_int(int *pointer, char *name, char *description);
/* Adds an int to the list of modifyable ints.
** Parameters: pointer: Pointer to the int to add to the list
**             name: Name for this value
**             description: A short description for the value
** Returns   : 0 on success, 1 if either value has already been added
**             or if name is already being used for a different value.
** The internal list of int references is used by some of the basic commands.
*/


byte *
con_backup_screen(struct _state *s);
/* Backups the currently visible screen
** Parameters: (state_t *) s: The state_t whose picture we're going to back up
** Returns   : (byte *): The malloc'd backup
*/


void
con_restore_screen(struct _state *s, byte *backup);
/* Restores a backupped screen
** Parameters: (state_t *) s: The state_t we're restoring to
**             (byte *) backup: The backup to restore
** Returns   : (void)
** Frees the allocated data after restoring it
*/


void
con_draw(struct _state *s, byte *backup);
/* Draws the on-screen console
** Parameters: (state_t *) s: The state_t to draw it on
**             (byte *) backup: The data from which the original backup data may be taken
** Returns   : (void)
*/


int
sci_hexdump(byte *data, int length, int offsetplus);

/***************************************************************************/
/* console commands */

int c_version(struct _state *s); /* displays the package and version number */
int c_list(struct _state *s); /* lists various types of things */
int c_man(struct _state *s); /* 'manual page' */
int c_set(struct _state *s); /* sets an int variable */
int c_print(struct _state *s); /* prints a variable */
int c_size(struct _state *s); /* displays the size of a resource */
int c_dump(struct _state *s); /* gives a hex dump of a resource */
int c_objinfo(struct _state *s); /* shows some info about one class */
int c_objmethods(struct _state *s); /* Disassembles all methods of a class */
int c_hexgrep(struct _state *s); /* Searches a string in one resource or resource class */
int c_selectornames(struct _state *s); /* Displays all selector names */
int c_kernelnames(struct _state *s); /* Displays all kernel function names */
int c_dissectscript(struct _state *s); /* Splits a script into objects and explains them */

#endif /* _SCI_CONSOLE_H_ */ 
