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

#include <gfx_operations.h>


sci_kernel_function_t kfunct_mappers[] = {
	{"Load", kLoad},
	{"UnLoad", kUnLoad},
	{"GameIsRestarting", kGameIsRestarting },
	{"NewList", kNewList },
	{"GetSaveDir", kGetSaveDir },
	{"GetCWD", kGetCWD },
	{"SetCursor", kSetCursor },
	{"FindKey", kFindKey },
	{"NewNode", kNewNode },
	{"AddToFront", kAddToFront },
	{"AddToEnd", kAddToEnd },
	{"Show", kShow },
	{"PicNotValid", kPicNotValid },
	{"Random", kRandom },
	{"Abs", kAbs },
	{"Sqrt", kSqrt },
	{"OnControl", kOnControl },
	{"HaveMouse", kHaveMouse },
	{"GetAngle", kGetAngle },
	{"GetDistance", kGetDistance },
	{"LastNode", kLastNode },
	{"FirstNode", kFirstNode },
	{"NextNode", kNextNode },
	{"PrevNode", kPrevNode },
	{"NodeValue", kNodeValue },
	{"Clone", kClone },
	{"DisposeClone", kDisposeClone },
	{"ScriptID", kScriptID },
	{"MemoryInfo", kMemoryInfo },
	{"DrawPic", kDrawPic },
	{"DisposeList", kDisposeList },
	{"DisposeScript", kDisposeScript },
	{"GetPort", kGetPort },
	{"SetPort", kSetPort },
	{"NewWindow", kNewWindow },
	{"DisposeWindow", kDisposeWindow },
	{"IsObject", kIsObject },
	{"Format", kFormat },
	{"DrawStatus", kDrawStatus },
	{"DrawMenuBar", kDrawMenuBar },
	{"AddMenu", kAddMenu },
	{"SetMenu", kSetMenu },
	{"AddToPic", kAddToPic },
	{"CelWide", kCelWide },
	{"CelHigh", kCelHigh },
	{"Display", kDisplay },
	{"Animate", kAnimate },
	{"GetTime", kGetTime },
	{"DeleteKey", kDeleteKey },
	{"StrLen", kStrLen },
	{"GetFarText", kGetFarText },
	{"StrEnd", kStrEnd },
	{"StrCat", kStrCat },
	{"StrCmp", kStrCmp },
	{"StrCpy", kStrCpy },
	{"StrAt", kStrAt },
	{"ReadNumber", kReadNumber },
	{"DrawControl", kDrawControl },
	{"NumCels", kNumCels },
	{"NumLoops", kNumLoops },
	{"TextSize", kTextSize },
	{"InitBresen", kInitBresen },
	{"DoBresen", kDoBresen },
	{"CanBeHere", kCanBeHere },
	{"DrawCel", kDrawCel },
	{"DirLoop", kDirLoop },
	{"CoordPri", kCoordPri },
	{"PriCoord", kPriCoord },
	{"ValidPath", kValidPath },
	{"RespondsTo", kRespondsTo },
	{"FOpen", kFOpen },
	{"FPuts", kFPuts },
	{"FGets", kFGets },
	{"FClose", kFClose },
	{"TimesSin", kTimesSin },
	{"SinMult", kTimesSin },
	{"TimesCos", kTimesCos },
	{"CosMult", kTimesCos },
	{"MapKeyToDir", kMapKeyToDir },
	{"GlobalToLocal", kGlobalToLocal },
	{"LocalToGlobal", kLocalToGlobal },
	{"Wait", kWait },
	{"CosDiv", kCosDiv },
	{"SinDiv", kSinDiv },
	{"BaseSetter", kBaseSetter },
	{"Parse", kParse },
	{"ShakeScreen", kShakeScreen },
#ifdef _WIN32
	{"DeviceInfo", kDeviceInfo_Win32},
#else /* !_WIN32 */
	{"DeviceInfo", kDeviceInfo_Unix},
#endif
	{"HiliteControl", kHiliteControl},
	{"GetMenu", kGetMenu},
	{"MenuSelect", kMenuSelect},
	{"GetEvent", kGetEvent },
	{"CheckFreeSpace", kCheckFreeSpace },
	{"DoSound", kDoSound },
	{"SetSynonyms", kSetSynonyms },
	{"FlushResources", kFlushResources },
	{"SetDebug", kSetDebug },
	{"GetSaveFiles", kGetSaveFiles },
	{"CheckSaveGame", kCheckSaveGame },
	{"SaveGame", kSaveGame },
	{"RestoreGame", kRestoreGame },
	{"SetJump", kSetJump },
	{"EditControl", kEditControl },
	{"EmptyList", kEmptyList },
	{"AddAfter", kAddAfter },
	{"RestartGame", kRestartGame },
	{"SetNowSeen", kSetNowSeen },
	{"Graph", kGraph },
	{"TimesTan", kTimesTan },
	{"TimesCot", kTimesCot },
	{"Said", kSaid },
	{"DoAvoider", kDoAvoider },

  /* Experimental functions */
	{"FileIO", kFileIO },
	{"Memory", kMemory },
  /* Special and NOP stuff */
	{SCRIPT_UNKNOWN_FUNCTION_STRING, k_Unknown },
	{0,0} /* Terminator */
};



#define SCI_MAPPED_UNKNOWN_KFUNCTIONS_NR 0x75

static kfunct * unknown_function_map[SCI_MAPPED_UNKNOWN_KFUNCTIONS_NR] = { /* Map for unknown kernel functions */
/*0x00*/ kLoad,
/*0x01*/ kUnLoad,
/*0x02*/ kScriptID,
/*0x03*/ kDisposeScript,
/*0x04*/ kClone,
/*0x05*/ kDisposeClone,
/*0x06*/ kIsObject,
/*0x07*/ kRespondsTo,
/*0x08*/ kDrawPic,
/*0x09*/ kShow,
/*0x0a*/ kPicNotValid,
/*0x0b*/ kAnimate,
/*0x0c*/ kSetNowSeen,
/*0x0d*/ kNumLoops,
/*0x0e*/ kNumCels,
/*0x0f*/ kCelWide,
/*0x10*/ kCelHigh,
/*0x11*/ kDrawCel,
/*0x12*/ kAddToPic,
/*0x13*/ kNewWindow,
/*0x14*/ kGetPort,
/*0x15*/ kSetPort,
/*0x16*/ kDisposeWindow,
/*0x17*/ kDrawControl,
/*0x18*/ kHiliteControl,
/*0x19*/ kEditControl,
/*0x1a*/ kTextSize,
/*0x1b*/ kDisplay,
/*0x1c*/ kGetEvent,
/*0x1d*/ kGlobalToLocal,
/*0x1e*/ kLocalToGlobal,
/*0x1f*/ kMapKeyToDir,
/*0x20*/ kDrawMenuBar,
/*0x21*/ kMenuSelect,
/*0x22*/ kAddMenu,
/*0x23*/ kDrawStatus,
/*0x24*/ kParse,
/*0x25*/ kSaid,
/*0x26*/ kSetSynonyms,
/*0x27*/ kHaveMouse,
/*0x28*/ kSetCursor,
/*0x29*/ kFOpen,
/*0x2a*/ kFPuts,
/*0x2b*/ kFGets,
/*0x2c*/ kFClose,
/*0x2d*/ kSaveGame,
/*0x2e*/ kRestoreGame,
/*0x2f*/ kRestartGame,
/*0x30*/ kGameIsRestarting,
/*0x31*/ kDoSound,
/*0x32*/ kNewList,
/*0x33*/ kDisposeList,
/*0x34*/ kNewNode,
/*0x35*/ kFirstNode,
/*0x36*/ kLastNode,
/*0x37*/ kEmptyList,
/*0x38*/ kNextNode,
/*0x39*/ kPrevNode,
/*0x3a*/ kNodeValue,
/*0x3b*/ kAddAfter, /* AddAfter */
/*0x3c*/ kAddToFront,
/*0x3d*/ kAddToEnd,
/*0x3e*/ kFindKey,
/*0x3f*/ kDeleteKey,
/*0x40*/ kRandom,
/*0x41*/ kAbs,
/*0x42*/ kSqrt,
/*0x43*/ kGetAngle,
/*0x44*/ kGetDistance,
/*0x45*/ kWait,
/*0x46*/ kGetTime,
/*0x47*/ kStrEnd,
/*0x48*/ kStrCat,
/*0x49*/ kStrCmp,
/*0x4a*/ kStrLen,
/*0x4b*/ kStrCpy,
/*0x4c*/ kFormat,
/*0x4d*/ kGetFarText,
/*0x4e*/ kReadNumber,
/*0x4f*/ kBaseSetter,
/*0x50*/ kDirLoop,
/*0x51*/ kCanBeHere,
/*0x52*/ kOnControl,
/*0x53*/ kInitBresen,
/*0x54*/ kDoBresen,
/*0x55*/ kNOP, /* DoAvoider */
/*0x56*/ kSetJump,
/*0x57*/ kSetDebug,
/*0x58*/ NULL, /* kInspectObj */
/*0x59*/ NULL, /* ShowSends */
/*0x5a*/ NULL, /* ShowObjs */
/*0x5b*/ NULL, /* ShowFree */
/*0x5c*/ kMemoryInfo,
/*0x5d*/ NULL, /* StackUsage */
/*0x5e*/ NULL, /* Profiler */
/*0x5f*/ kGetMenu,
/*0x60*/ kSetMenu,
/*0x61*/ kGetSaveFiles,
/*0x62*/ kGetCWD,
/*0x63*/ kCheckFreeSpace,
/*0x64*/ kValidPath,
/*0x65*/ kCoordPri,
/*0x66*/ kStrAt,
#ifdef _WIN32
/*0x67*/ kDeviceInfo_Win32,
#else
/*0x67*/ kDeviceInfo_Unix,
#endif
/*0x68*/ kGetSaveDir,
/*0x69*/ kCheckSaveGame,
/*0x6a*/ kShakeScreen,
/*0x6b*/ kFlushResources,
/*0x6c*/ kTimesSin,
/*0x6d*/ kTimesCos,
/*0x6e*/ NULL,
/*0x6f*/ NULL,
/*0x70*/ kGraph,
/*0x71*/ kJoystick,
/*0x72*/ NULL,
/*0x73*/ NULL,
/*0x74*/ kFileIO
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
	"Room numbers"
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


/* Allocates a set amount of memory for a specified use and returns a handle to it. */
int
kalloc(state_t *s, int type, int space)
{
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
}


/* Returns a pointer to the memory indicated by the specified handle */
byte *
kmem(state_t *s, int handle)
{
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
}

/* Frees the specified handle. Returns 0 on success, 1 otherwise. */
int
kfree(state_t *s, int handle)
{
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


void
k_Unknown(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
	kfunct *funct = (funct_nr >= SCI_MAPPED_UNKNOWN_KFUNCTIONS_NR)? NULL : unknown_function_map[funct_nr];

	if (!funct) {
		SCIkwarn(SCIkSTUB, "Unhandled Unknown function %04x\n", funct_nr);
	} else funct(s, funct_nr, argc, argp);
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

void
kstub(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
	int i;

	SCIkwarn(SCIkWARNING, "Unimplemented syscall: %s[%x](", s->kernel_names[funct_nr], funct_nr);

	for (i = 0; i < argc; i++) {
		sciprintf("%04x", 0xffff & PARAM(i));
		if (i+1 < argc) sciprintf(", ");
	}
	sciprintf(")\n");
}


void
kNOP(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
	SCIkwarn(SCIkWARNING, "Warning: Kernel function 0x%02x invoked: NOP\n", funct_nr);
}


void
script_map_kernel(state_t *s)
{
	int functnr;
	int mapped = 0;

	s->kfunct_table = sci_malloc(sizeof(kfunct *) * (s->kernel_names_nr + 1));

	for (functnr = 0; functnr < s->kernel_names_nr; functnr++) {
		int seeker, found = -1;

		for (seeker = 0; (found == -1) && kfunct_mappers[seeker].functname; seeker++)
			if (strcmp(kfunct_mappers[seeker].functname, s->kernel_names[functnr]) == 0) {
				found = seeker; /* Found a kernel function with the same name! */
				mapped++;
			}

		if (found == -1) {

			sciprintf("Warning: Kernel function %s[%x] unmapped\n", s->kernel_names[functnr], functnr);
			s->kfunct_table[functnr] = kstub;

		} else s->kfunct_table[functnr] = kfunct_mappers[found].kernel_function;

	} /* for all functions requesting to be mapped */

	sciprintf("Mapped %d of %d kernel functions.\n", mapped, s->kernel_names_nr);

}
