/***************************************************************************
 kernel.c Copyright (C) 1999 Christoph Reichenbach


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

    Christoph Reichenbach (CJR) [jameson@linuxgames.com]

***************************************************************************/

#include <sciresource.h>
#include <engine.h>
#ifdef _WIN32
#	include <windows.h>
#endif /* _WIN32 */
#include <kernel_compat.h>

#include <gfx_operations.h>



#define SCI_MAPPED_UNKNOWN_KFUNCTIONS_NR 0x75
/* kfunct_mappers below doubles for unknown kfunctions */


sci_kernel_function_t kfunct_mappers[] = {
/*00*/	{KF_OLD, "Load", {old:kLoad}},
/*01*/	{KF_OLD, "UnLoad", {old:kUnLoad}},
/*02*/	{KF_OLD, "ScriptID", {old:kScriptID}},
/*03*/	{KF_OLD, "DisposeScript", {old:kDisposeScript}},
/*04*/	{KF_OLD, "Clone", {old:kClone}},
/*05*/	{KF_OLD, "DisposeClone", {old:kDisposeClone}},
/*06*/	{KF_OLD, "IsObject", {old:kIsObject}},
/*07*/	{KF_OLD, "RespondsTo", {old:kRespondsTo}},
/*08*/	{KF_OLD, "DrawPic", {old:kDrawPic}},
/*09*/	{KF_OLD, "Show", {old:kShow}},
/*0a*/	{KF_OLD, "PicNotValid", {old:kPicNotValid}},
/*0b*/	{KF_OLD, "Animate", {old:kAnimate}},
/*0c*/	{KF_OLD, "SetNowSeen", {old:kSetNowSeen}},
/*0d*/	{KF_OLD, "NumLoops", {old:kNumLoops}},
/*0e*/	{KF_OLD, "NumCels", {old:kNumCels}},
/*0f*/	{KF_OLD, "CelWide", {old:kCelWide}},
/*10*/	{KF_OLD, "CelHigh", {old:kCelHigh}},
/*11*/	{KF_OLD, "DrawCel", {old:kDrawCel}},
/*12*/	{KF_OLD, "AddToPic", {old:kAddToPic}},
/*13*/	{KF_OLD, "NewWindow", {old:kNewWindow}},
/*14*/	{KF_OLD, "GetPort", {old:kGetPort}},
/*15*/	{KF_OLD, "SetPort", {old:kSetPort}},
/*16*/	{KF_OLD, "DisposeWindow", {old:kDisposeWindow}},
/*17*/	{KF_OLD, "DrawControl", {old:kDrawControl}},
/*18*/	{KF_OLD, "HiliteControl", {old:kHiliteControl}},
/*19*/	{KF_OLD, "EditControl", {old:kEditControl}},
/*1a*/	{KF_OLD, "TextSize", {old:kTextSize}},
/*1b*/	{KF_OLD, "Display", {old:kDisplay}},
/*1c*/	{KF_OLD, "GetEvent", {old:kGetEvent}},
/*1d*/	{KF_OLD, "GlobalToLocal", {old:kGlobalToLocal}},
/*1e*/	{KF_OLD, "LocalToGlobal", {old:kLocalToGlobal}},
/*1f*/	{KF_OLD, "MapKeyToDir", {old:kMapKeyToDir}},
/*20*/	{KF_OLD, "DrawMenuBar", {old:kDrawMenuBar}},
/*21*/	{KF_OLD, "MenuSelect", {old:kMenuSelect}},
/*22*/	{KF_OLD, "AddMenu", {old:kAddMenu}},
/*23*/	{KF_OLD, "DrawStatus", {old:kDrawStatus}},
/*24*/	{KF_OLD, "Parse", {old:kParse}},
/*25*/	{KF_OLD, "Said", {old:kSaid}},
/*26*/	{KF_OLD, "SetSynonyms", {old:kSetSynonyms}},
/*27*/	{KF_OLD, "HaveMouse", {old:kHaveMouse}},
/*28*/	{KF_OLD, "SetCursor", {old:kSetCursor}},
/*29*/	{KF_OLD, "FOpen", {old:kFOpen}},
/*2a*/	{KF_OLD, "FPuts", {old:kFPuts}},
/*2b*/	{KF_OLD, "FGets", {old:kFGets}},
/*2c*/	{KF_OLD, "FClose", {old:kFClose}},
/*2d*/	{KF_OLD, "SaveGame", {old:kSaveGame}},
/*2e*/	{KF_OLD, "RestoreGame", {old:kRestoreGame}},
/*2f*/	{KF_OLD, "RestartGame", {old:kRestartGame}},
/*30*/	{KF_OLD, "GameIsRestarting", {old:kGameIsRestarting}},
/*31*/	{KF_OLD, "DoSound", {old:kDoSound}},
/*32*/	{KF_OLD, "NewList", {old:kNewList}},
/*33*/	{KF_OLD, "DisposeList", {old:kDisposeList}},
/*34*/	{KF_OLD, "NewNode", {old:kNewNode}},
/*35*/	{KF_OLD, "FirstNode", {old:kFirstNode}},
/*36*/	{KF_OLD, "LastNode", {old:kLastNode}},
/*37*/	{KF_OLD, "EmptyList", {old:kEmptyList}},
/*38*/	{KF_OLD, "NextNode", {old:kNextNode}},
/*39*/	{KF_OLD, "PrevNode", {old:kPrevNode}},
/*3a*/	{KF_OLD, "NodeValue", {old:kNodeValue}},
/*3b*/	{KF_OLD, "AddAfter", {old:kAddAfter}},
/*3c*/	{KF_OLD, "AddToFront", {old:kAddToFront}},
/*3d*/	{KF_OLD, "AddToEnd", {old:kAddToEnd}},
/*3e*/	{KF_OLD, "FindKey", {old:kFindKey}},
/*3f*/	{KF_OLD, "DeleteKey", {old:kDeleteKey}},
/*40*/	{KF_OLD, "Random", {old:kRandom}},
/*41*/	{KF_OLD, "Abs", {old:kAbs}},
/*42*/	{KF_OLD, "Sqrt", {old:kSqrt}},
/*43*/	{KF_OLD, "GetAngle", {old:kGetAngle}},
/*44*/	{KF_OLD, "GetDistance", {old:kGetDistance}},
/*45*/	{KF_OLD, "Wait", {old:kWait}},
/*46*/	{KF_OLD, "GetTime", {old:kGetTime}},
/*47*/	{KF_OLD, "StrEnd", {old:kStrEnd}},
/*48*/	{KF_OLD, "StrCat", {old:kStrCat}},
/*49*/	{KF_OLD, "StrCmp", {old:kStrCmp}},
/*4a*/	{KF_OLD, "StrLen", {old:kStrLen}},
/*4b*/	{KF_OLD, "StrCpy", {old:kStrCpy}},
/*4c*/	{KF_OLD, "Format", {old:kFormat}},
/*4d*/	{KF_OLD, "GetFarText", {old:kGetFarText}},
/*4e*/	{KF_OLD, "ReadNumber", {old:kReadNumber}},
/*4f*/	{KF_OLD, "BaseSetter", {old:kBaseSetter}},
/*50*/	{KF_OLD, "DirLoop", {old:kDirLoop}},
/*51*/	{KF_OLD, "CanBeHere", {old:kCanBeHere}},
/*52*/	{KF_OLD, "OnControl", {old:kOnControl}},
/*53*/	{KF_OLD, "InitBresen", {old:kInitBresen}},
/*54*/	{KF_OLD, "DoBresen", {old:kDoBresen}},
/*55*/	{KF_OLD, "DoAvoider", {old:kDoAvoider}},
/*56*/	{KF_OLD, "SetJump", {old:kSetJump}},
/*57*/	{KF_OLD, "SetDebug", {old:kSetDebug}},
/*58*/	{KF_NONE, "InspectObj"},
/*59*/	{KF_NONE, "ShowSends"},
/*5a*/	{KF_NONE, "ShowObjs"},
/*5b*/	{KF_NONE, "ShowFree"},
/*5c*/	{KF_OLD, "MemoryInfo", {old:kMemoryInfo}},
/*5d*/	{KF_NONE, "StackUsage"},
/*5e*/	{KF_NONE, "Profiler"},
/*5f*/	{KF_OLD, "GetMenu", {old:kGetMenu}},
/*60*/	{KF_OLD, "SetMenu", {old:kSetMenu}},
/*61*/	{KF_OLD, "GetSaveFiles", {old:kGetSaveFiles}},
/*62*/	{KF_OLD, "GetCWD", {old:kGetCWD}},
/*63*/	{KF_OLD, "CheckFreeSpace", {old:kCheckFreeSpace}},
/*64*/	{KF_OLD, "ValidPath", {old:kValidPath}},
/*65*/	{KF_OLD, "CoordPri", {old:kCoordPri}},
/*66*/	{KF_OLD, "StrAt", {old:kStrAt}},
#ifdef _WIN32
/*67*/	{KF_OLD, "DeviceInfo", {old:kDeviceInfo_Win32}},
#else /* !_WIN32 */
/*67*/	{KF_OLD, "DeviceInfo", {old:kDeviceInfo_Unix}},
#endif
/*68*/	{KF_NEW , "GetSaveDir", {new:{kGetSaveDir, ""}}},
/*69*/	{KF_OLD, "CheckSaveGame", {old:kCheckSaveGame}},
/*6a*/	{KF_OLD, "ShakeScreen", {old:kShakeScreen}},
/*6b*/	{KF_OLD, "FlushResources", {old:kFlushResources}},
/*6c*/	{KF_OLD, "TimesSin", {old:kTimesSin}},
/*6d*/	{KF_OLD, "TimesCos", {old:kTimesCos}},
/*6e*/	{KF_NONE, NULL},
/*6f*/	{KF_NONE, NULL},
/*70*/	{KF_OLD, "Graph", {old:kGraph}},
/*71*/	{KF_OLD, "Joystick", {old:kJoystick}},
/*72*/	{KF_NONE, NULL},
/*73*/	{KF_NONE, NULL},

  /* Experimental functions */
/*74*/	{KF_OLD, "FileIO", {old:kFileIO}},
/*(?)*/	{KF_OLD, "Memory", {old:kMemory}},
/*(?)*/	{KF_OLD, "Sort", {old:kSort}},
/*(?)*/	{KF_OLD, "AvoidPath", {old:kAvoidPath}},
/*(?)*/	{KF_OLD, "Lock", {old:kLock}},

  /* Non-experimental Functions without a fixed ID */

	{KF_OLD, "CosMult", {old:kTimesCos}},
	{KF_OLD, "SinMult", {old:kTimesSin}},
/*(?)*/	{KF_OLD, "CosDiv", {old:kCosDiv}},
/*(?)*/	{KF_OLD, "PriCoord", {old:kPriCoord}},
/*(?)*/	{KF_OLD, "SinDiv", {old:kSinDiv}},
/*(?)*/	{KF_OLD, "TimesCot", {old:kTimesCot}},
/*(?)*/	{KF_OLD, "TimesTan", {old:kTimesTan}},

  /* Special and NOP stuff */
	{KF_OLD, NULL, {new:{k_Unknown, NULL}}},

	{KF_TERMINATOR, NULL} /* Terminator */
};


const char *SCIk_Debug_Names[SCIk_DEBUG_MODES] = {
	"Stubs",
	"Lists and nodes",
	"Graphics",
	"Character handling",
	"Memory management",
	"Function parameter checks",
	"Bresenham algorithms",
	"Audio subsystem",
	"System graphics driver",
	"Base setter results",
	"Parser",
	"Menu handling",
	"Said specs",
	"File I/O",
	"Time",
	"Room numbers",
	"FreeSCI 0.3.3 kernel emulation"
};


/******************** Kernel Oops ********************/

int
kernel_oops(state_t *s, char *file, int line, char *reason)
{
	sciprintf("Kernel Oops in file %s, line %d: %s\n", file, line, reason);
	fprintf(stderr,"Kernel Oops in file %s, line %d: %s\n", file, line, reason);
	script_debug_flag = script_error_flag = 1;
	return 0;
}

#warning "Re-implement hunk space (1)!"
/* Allocates a set amount of memory for a specified use and returns a handle to it. */
int
kalloc(state_t *s, int type, int space)
{
#if 0
	int seeker = 0;

	while ((seeker < MAX_HUNK_BLOCKS) && (s->hunk[seeker].size))
		seeker++;

	if (seeker == MAX_HUNK_BLOCKS)
		KERNEL_OOPS("Out of hunk handles! Try increasing MAX_HUNK_BLOCKS in engine.h");
	else {
		s->hunk[seeker].data = sci_malloc(s->hunk[seeker].size = space);
		s->hunk[seeker].type = type;
	}

	SCIkdebug(SCIkMEM, "Allocated %d at hunk %04x\n", space, seeker | (sci_memory << 11));

	return (seeker | (sci_memory << 11));
#endif
}


#warning "Re-implement hunk space (2)!"
/* Returns a pointer to the memory indicated by the specified handle */
byte *
kmem(state_t *s, int handle)
{
#if 0
	if ((handle >> 11) != sci_memory) {
		SCIkwarn(SCIkERROR, "Error: kmem() without a handle (%04x)\n", handle);
		return 0;
	}

	handle &= 0x7ff;

	if ((handle < 0) || (handle >= MAX_HUNK_BLOCKS)) {
		SCIkwarn(SCIkERROR, "Error: kmem() with invalid handle\n");
		return 0;
	}

	return (byte *) s->hunk[handle & 0x7ff].data;
#endif
}

#warning "Re-implement hunk space (3)!"
/* Frees the specified handle. Returns 0 on success, 1 otherwise. */
int
kfree(state_t *s, int handle)
{
#if 0
	if ((handle >> 11) != sci_memory) {
		SCIkwarn(SCIkERROR, "Attempt to kfree() non-handle (%04x)\n", handle);
		return 1;
	}

	SCIkdebug(SCIkMEM, "Freeing hunk %04x\n", handle);

	handle &= 0x7ff;

	if ((handle < 0) || (handle >= MAX_HUNK_BLOCKS)) {
		SCIkwarn(SCIkERROR, "Error: Attempt to kfree() with invalid handle\n");
		return 1;
	}

	if (s->hunk[handle].size == 0) {
		SCIkwarn(SCIkERROR, "Error: Attempt to kfree() non-allocated memory\n");
		return 1;
	}

	free(s->hunk[handle].data);
	s->hunk[handle].data = NULL;
	s->hunk[handle].size = 0;

	return 0;
#endif
}


/*****************************************/
/************* Kernel functions **********/
/*****************************************/

char *old_save_dir;

void
kRestartGame(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
	old_save_dir= sci_strdup((char *) s->heap+s->save_dir+2);
	s->restarting_flags |= SCI_GAME_IS_RESTARTING_NOW;
	s->restarting_flags &= ~SCI_GAME_WAS_RESTARTED_AT_LEAST_ONCE; /* This appears to help */
	s->execution_stack_pos = s->execution_stack_base;
	if (s->sound_server)
		s->sound_server->command(s, SOUND_COMMAND_STOP_ALL, 0, 0);
	script_abort_flag = 1; /* Force vm to abort ASAP */
}


/* kGameIsRestarting():
** Returns the restarting_flag in acc
*/
void
kGameIsRestarting(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
	s->acc = (s->restarting_flags & SCI_GAME_WAS_RESTARTED);

	if ((old_save_dir)&&(s->save_dir))
		{
			strcpy((char *) s->heap + s->save_dir + 2, old_save_dir);
			free(old_save_dir);
			old_save_dir = NULL;
		}
	if (argc) {/* Only happens during replay */
		if (!PARAM(0)) /* Set restarting flag */
			s->restarting_flags &= ~SCI_GAME_WAS_RESTARTED;
	}
}

void
kHaveMouse(state_t *s, int funct_nr, int argc, heap_ptr argp)
{

	s->acc = (s->have_mouse_flag
		  && gfxop_have_mouse(s->gfx_state))? -1 : 0;
}



void
kMemoryInfo(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
	switch (PARAM(0)) {
	case 0: s->acc = heap_meminfo(s->_heap); break;
	case 1: s->acc = heap_largest(s->_heap); break;
	case 2: /* Largest available hunk memory block */
	case 3: /* Total amount of hunk memory */
	case 4: s->acc = (gint16) 0xffff; break; /* Amount of free DOS paragraphs- SCI01 */
	default: SCIkwarn(SCIkWARNING, "Unknown MemoryInfo operation: %04x\n", PARAM(0));
	}
}


reg_t
k_Unknown(state_t *s, int funct_nr, int argc, reg_t *argv)
{
	if (funct_nr >= SCI_MAPPED_UNKNOWN_KFUNCTIONS_NR) {
		SCIkwarn(SCIkSTUB, "Unhandled Unknown function %04x\n", funct_nr);
		return NULL_REG;
	} else switch(kfunct_mappers[funct_nr].type) {

	case KF_OLD:
		return kFsciEmu(s, funct_nr, argc, argv);

	case KF_NEW:
		return kfunct_mappers[funct_nr].data.new.fun(s, funct_nr, argc, argv);

	case KF_NONE:
	default:
		SCIkwarn(SCIkSTUB, "Unhandled Unknown function %04x\n", funct_nr);
		return NULL_REG;
	}
}


void
kFlushResources(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
	/* Nothing to do */
  SCIkdebug(SCIkROOM, "Entering room number %d\n", UPARAM(0));
}

void
kSetDebug(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
	sciprintf("Debug mode activated\n");

	script_debug_flag = 1; /* Enter debug mode */
	_debug_seeking = _debug_step_running = 0;
}

#define _K_NEW_GETTIME_TICKS 0
#define _K_NEW_GETTIME_TIME_12HOUR 1
#define _K_NEW_GETTIME_TIME_24HOUR 2
#define _K_NEW_GETTIME_DATE 3

void
kGetTime(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
	struct tm* loc_time;
	GTimeVal time_prec;
	time_t the_time;


	/* Reset optimization flags: If this function is called,
	** the game may be waiting for a timeout  */
	s->kernel_opt_flags &= ~(KERNEL_OPT_FLAG_GOT_EVENT
				 | KERNEL_OPT_FLAG_GOT_2NDEVENT);

#ifdef _WIN32
	if (TIMERR_NOERROR != timeBeginPeriod(1))
	{
		fprintf(stderr, "timeBeginPeriod(1) failed in kGetTime!\n");
	}
#endif /* _WIN32 */

	the_time = time(NULL);
	loc_time = localtime(&the_time);

#ifdef _WIN32
	if (TIMERR_NOERROR != timeEndPeriod(1))
	{
		fprintf(stderr, "timeEndPeriod(1) failed in kGetTime!\n");
	}
#endif /* _WIN32 */

	if (s->version<SCI_VERSION_FTU_NEW_GETTIME) { /* Use old semantics */
		if (argc) { /* Get seconds since last am/pm switch */
			s->acc = loc_time->tm_sec + loc_time->tm_min * 60 + (loc_time->tm_hour % 12) * 3600;
			SCIkdebug(SCIkTIME, "GetTime(timeofday) returns %d\n", s->acc);
		} else { /* Get time since game started */
			sci_get_current_time (&time_prec);
			s-> acc = ((time_prec.tv_usec - s->game_start_time.tv_usec) * 60 / 1000000) +
				(time_prec.tv_sec - s->game_start_time.tv_sec) * 60;
			SCIkdebug(SCIkTIME, "GetTime(elapsed) returns %d\n", s->acc);
		}
	} else {
		int mode = UPARAM_OR_ALT(0, 0); /* The same strange method is still used for distinguishing
						   mode 0 and the others. We assume that this is safe, though */
		switch (mode) {
		case _K_NEW_GETTIME_TICKS : {
			sci_get_current_time (&time_prec);
			s-> acc = ((time_prec.tv_usec - s->game_start_time.tv_usec) * 60 / 1000000) +
				(time_prec.tv_sec - s->game_start_time.tv_sec) * 60;
			SCIkdebug(SCIkTIME, "GetTime(elapsed) returns %d\n", s->acc);
			break;
		}
		case _K_NEW_GETTIME_TIME_12HOUR : {
			loc_time->tm_hour %= 12;
			s->acc=(loc_time->tm_min<<6)|(loc_time->tm_hour<<12)|(loc_time->tm_sec);
			SCIkdebug(SCIkTIME, "GetTime(12h) returns %d\n", s->acc);
			break;
		}
		case _K_NEW_GETTIME_TIME_24HOUR : {
			s->acc=(loc_time->tm_min<<5)|(loc_time->tm_sec>>1)|(loc_time->tm_hour<<11);
			SCIkdebug(SCIkTIME, "GetTime(24h) returns %d\n", s->acc);
			break;
		}
		case _K_NEW_GETTIME_DATE : {
			s->acc=(loc_time->tm_mon<<5)|loc_time->tm_mday|(loc_time->tm_year<<9);
			SCIkdebug(SCIkTIME, "GetTime(date) returns %d\n", s->acc);
			break;
		}
		default: {
			SCIkdebug(SCIkWARNING, "Attempt to use unknown GetTime mode %d\n", mode);
			break;
		}
		}
	}
}

#define K_MEMORY_ALLOCATE_CRITICAL 	1
#define K_MEMORY_ALLOCATE_NONCRITICAL   2
#define K_MEMORY_FREE			3
#define	K_MEMORY_MEMCPY			4
#define K_MEMORY_PEEK			5
#define K_MEMORY_POKE			6
void
kMemory(state_t *s, int funct_nr, int argc, heap_ptr argp)
{

	switch (PARAM(0)) {

	case K_MEMORY_ALLOCATE_CRITICAL :

		s->acc=heap_allocate(s->_heap, UPARAM(1))+2;
		if (!s->acc)
			{
				SCIkwarn(SCIkERROR, "Critical heap allocation failed\n");
				script_error_flag = script_debug_flag = 1;
			}
		break;

	case K_MEMORY_ALLOCATE_NONCRITICAL :

		s->acc=heap_allocate(s->_heap, UPARAM(1))+2;
		break;

	case K_MEMORY_FREE :

		heap_free(s->_heap, UPARAM(1)-2);
		break;

	case K_MEMORY_MEMCPY :
		{

			int dest = UPARAM(1);
			int src = UPARAM(2);
			int n = UPARAM(3);

			memcpy(s->heap + dest, s->heap + src, n);
			break;
		}

	case K_MEMORY_PEEK :

		s->acc=GET_HEAP(UPARAM(1));
		break;

	case K_MEMORY_POKE :

		PUT_HEAP(UPARAM(1), UPARAM(2));
		break;

	}
}

reg_t
kstub(state_t *s, int funct_nr, int argc, reg_t *argv)
{
	int i;

	SCIkwarn(SCIkWARNING, "Unimplemented syscall: %s[%x](",
		 s->kernel_names[funct_nr], funct_nr);

	for (i = 0; i < argc; i++) {
		sciprintf(PREG, PRINT_REG(argv[i]));
		if (i+1 < argc) sciprintf(", ");
	}
	sciprintf(")\n");
	return NULL_REG;
}


reg_t
kNOP(state_t *s, int funct_nr, int argc, reg_t *argv)
{
	SCIkwarn(SCIkWARNING, "Warning: Kernel function 0x%02x invoked: NOP\n", funct_nr);
	return NULL_REG;
}

void
script_map_kernel(state_t *s)
{
	int functnr;
	int mapped = 0;
	int emulated = 0;
	int ignored = 0;

	s->kfunct_table = sci_malloc(sizeof(kfunct_sig_pair_t) * (s->kernel_names_nr + 1));
	s->kfunct_emu_table = sci_malloc(sizeof(kfunct_old *) * (s->kernel_names_nr + 1));

	for (functnr = 0; functnr < s->kernel_names_nr; functnr++) {
		int seeker, found = -1;

		for (seeker = 0; (found == -1)
			     && kfunct_mappers[seeker].type != KF_TERMINATOR; seeker++)
			if (kfunct_mappers[seeker].name
			    && strcmp(kfunct_mappers[seeker].name, s->kernel_names[functnr]) == 0)
				found = seeker; /* Found a kernel function with the same name! */

		if (found == -1) {
			sciprintf("Warning: Kernel function %s[%x] unmapped\n",
				  s->kernel_names[functnr], functnr);
			s->kfunct_table[functnr].signature = NULL;
			s->kfunct_table[functnr].fun = kstub;
		} else switch (kfunct_mappers[found].type) {

		case KF_OLD:
			/* emulation-map it */

			sciprintf("Warning: Emulating kernel function %s[%x]!\n",
				  s->kernel_names[functnr], functnr);
			++emulated;
			s->kfunct_table[functnr].signature = NULL;
			s->kfunct_table[functnr].fun = kFsciEmu;
			s->kfunct_emu_table[functnr] = kfunct_mappers[found].data.old;
			break;

		case KF_NONE:
			++ignored;
			break;

		case KF_NEW:
			s->kfunct_table[functnr] = kfunct_mappers[found].data.new;
			++mapped;
			break;
		}

	} /* for all functions requesting to be mapped */

	sciprintf("Handled %d/%d kernel functions, mapping %d",
		  mapped+ignored+emulated, s->kernel_names_nr, mapped);
	if (ignored)
		sciprintf(", ignoring %d", ignored);
	if (emulated)
		sciprintf(" and emulating %d", emulated);
	sciprintf(".\n");

}
