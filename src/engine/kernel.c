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
#include <kernel_types.h>


void kLoad(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kUnLoad(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kGameIsRestarting(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kGetCWD(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kSetCursor(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kShow(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kPicNotValid(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kRandom(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kAbs(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kSqrt(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kOnControl(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kHaveMouse(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kJoystick(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kGetAngle(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kGetDistance(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kDrawPic(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kGetPort(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kSetPort(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kNewWindow(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kDisposeWindow(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kFormat(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kDrawStatus(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kDrawMenuBar(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kAddMenu(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kSetMenu(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kCelWide(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kCelHigh(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kDisplay(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kGetTime(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kStrLen(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kGetFarText(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kStrEnd(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kStrCat(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kStrCmp(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kStrCpy(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kStrAt(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kReadNumber(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kNumCels(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kNumLoops(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kDrawCel(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kDirLoop(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kCoordPri(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kPriCoord(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kValidPath(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kFOpen(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kFPuts(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kFGets(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kFClose(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kTimesSin(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kTimesCos(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kCosMult(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kSinMult(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kTimesTan(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kTimesCot(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kMapKeyToDir(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kGlobalToLocal(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kLocalToGlobal(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kWait(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kCosDiv(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kSinDiv(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kParse(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kShakeScreen(struct _state *s, int funct_nr, int argc, heap_ptr argp);
#ifdef _WIN32
void kDeviceInfo_Win32(struct _state *s, int funct_nr, int argc, heap_ptr argp);
#else
void kDeviceInfo_Unix(struct _state *s, int funct_nr, int argc, heap_ptr argp);
#endif
void kRestartGame(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kSaid(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kDoSound(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kSetSynonyms(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kGraph(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kGetEvent(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kGetMenu(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kMenuSelect(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kCheckFreeSpace(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kFlushResources(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kGetSaveFiles(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kSetDebug(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kSetJump(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kCheckSaveGame(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kSaveGame(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kRestoreGame(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kDoAvoider(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kFileIO(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kSort(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kAvoidPath(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kLock(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kMemory(struct _state *s, int funct_nr, int argc, heap_ptr argp);


/* New kernel functions */
reg_t kEditControl(struct _state *s, int funct_nr, int argc, reg_t *argv);
reg_t kDrawControl(struct _state *s, int funct_nr, int argc, reg_t *argv);
reg_t kHiliteControl(struct _state *s, int funct_nr, int argc, reg_t *argv);
reg_t kClone(struct _state *s, int funct_nr, int argc, reg_t *argv);
reg_t kDisposeClone(struct _state *s, int funct_nr, int argc, reg_t *argv);
reg_t kCanBeHere(struct _state *s, int funct_nr, int argc, reg_t *argv);
reg_t kSetNowSeen(struct _state *s, int funct_nr, int argc, reg_t *argv);
reg_t kInitBresen(struct _state *s, int funct_nr, int argc, reg_t *argv);
reg_t kDoBresen(struct _state *s, int funct_nr, int argc, reg_t *argv);
reg_t kBaseSetter(struct _state *s, int funct_nr, int argc, reg_t *argp);
reg_t kAddToPic(struct _state *s, int funct_nr, int argc, reg_t *argv);
reg_t kAnimate(struct _state *s, int funct_nr, int argc, reg_t *argv);

reg_t kScriptID(struct _state *s, int funct_nr, int argc, reg_t *argv);
reg_t kDisposeScript(struct _state *s, int funct_nr, int argc, reg_t *argv);
reg_t kIsObject(struct _state *s, int funct_nr, int argc, reg_t *argv);
reg_t kRespondsTo(struct _state *s, int funct_nr, int argc, reg_t *argv);
reg_t kNewList(struct _state *s, int funct_nr, int argc, reg_t *argv);
reg_t kDisposeList(struct _state *s, int funct_nr, int argc, reg_t *argv);
reg_t kNewNode(struct _state *s, int funct_nr, int argc, reg_t *argv);
reg_t kFirstNode(struct _state *s, int funct_nr, int argc, reg_t *argv);
reg_t kLastNode(struct _state *s, int funct_nr, int argc, reg_t *argv);
reg_t kEmptyList(struct _state *s, int funct_nr, int argc, reg_t *argv);
reg_t kNextNode(struct _state *s, int funct_nr, int argc, reg_t *argv);
reg_t kPrevNode(struct _state *s, int funct_nr, int argc, reg_t *argv);
reg_t kNodeValue(struct _state *s, int funct_nr, int argc, reg_t *argv);
reg_t kAddAfter(struct _state *s, int funct_nr, int argc, reg_t *argv);
reg_t kAddToFront(struct _state *s, int funct_nr, int argc, reg_t *argv);
reg_t kAddToEnd(struct _state *s, int funct_nr, int argc, reg_t *argv);
reg_t kFindKey(struct _state *s, int funct_nr, int argc, reg_t *argv);
reg_t kDeleteKey(struct _state *s, int funct_nr, int argc, reg_t *argv);
reg_t kMemoryInfo(struct _state *s, int funct_nr, int argc, reg_t *argv);
reg_t kGetSaveDir(struct _state *s, int funct_nr, int argc, reg_t *argv);
reg_t kTextSize(struct _state *s, int funct_nr, int argc, reg_t *argv);
reg_t k_Unknown(struct _state *s, int funct_nr, int argc, reg_t *argv);

/* The Unknown/Unnamed kernel function */
reg_t kstub(struct _state *s, int funct_nr, int argc, reg_t *argv);
/* for unimplemented kernel functions */
reg_t kNOP(struct _state *s, int funct_nr, int argc, reg_t *argv);
/* for kernel functions that don't do anything */
reg_t kFsciEmu(struct _state *s, int funct_nr, int argc, reg_t *argv);
/* Emulating "old" kernel functions on the heap */


#define SCI_MAPPED_UNKNOWN_KFUNCTIONS_NR 0x75
/* kfunct_mappers below doubles for unknown kfunctions */


sci_kernel_function_t kfunct_mappers[] = {
/*00*/	{KF_OLD, "Load", {old:kLoad}},
/*01*/	{KF_OLD, "UnLoad", {old:kUnLoad}},
/*02*/	{KF_NEW, "ScriptID", {new:{kScriptID, "ii*"}}},
/*03*/	{KF_NEW, "DisposeScript", {new:{kDisposeScript, "i"}}},
/*04*/	{KF_NEW, "Clone", {new:{kClone, "o"}}},
/*05*/	{KF_NEW, "DisposeClone", {new:{kDisposeClone, "o"}}},
/*06*/	{KF_NEW, "IsObject", {new:{kIsObject, "."}}},
/*07*/	{KF_NEW, "RespondsTo", {new:{kRespondsTo, ".i"}}},
/*08*/	{KF_OLD, "DrawPic", {old:kDrawPic}},
/*09*/	{KF_OLD, "Show", {old:kShow}},
/*0a*/	{KF_OLD, "PicNotValid", {old:kPicNotValid}},
/*0b*/	{KF_NEW, "Animate", {new:{kAnimate, "LI*"}}}, /* More like l?i? */
/*0c*/	{KF_NEW, "SetNowSeen", {new:{kSetNowSeen, "oi*"}}},
/*0d*/	{KF_OLD, "NumLoops", {old:kNumLoops}},
/*0e*/	{KF_OLD, "NumCels", {old:kNumCels}},
/*0f*/	{KF_OLD, "CelWide", {old:kCelWide}},
/*10*/	{KF_OLD, "CelHigh", {old:kCelHigh}},
/*11*/	{KF_OLD, "DrawCel", {old:kDrawCel}},
/*12*/	{KF_NEW, "AddToPic", {new:{kAddToPic, "Il*"}}},
/*13*/	{KF_OLD, "NewWindow", {old:kNewWindow}},
/*14*/	{KF_OLD, "GetPort", {old:kGetPort}},
/*15*/	{KF_OLD, "SetPort", {old:kSetPort}},
/*16*/	{KF_OLD, "DisposeWindow", {old:kDisposeWindow}},
/*17*/	{KF_NEW, "DrawControl", {new:{kDrawControl, "o"}}},
/*18*/	{KF_NEW, "HiliteControl", {new:{kHiliteControl, "o"}}},
/*19*/	{KF_NEW, "EditControl", {new:{kEditControl, "ZoZo"}}},
/*1a*/	{KF_NEW, "TextSize", {new:{kTextSize, "rrii*"}}},
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
/*32*/	{KF_NEW, "NewList", {new:{kNewList, ""}}},
/*33*/	{KF_NEW, "DisposeList", {new:{kDisposeList, "l"}}},
/*34*/	{KF_NEW, "NewNode", {new:{kNewNode, ".."}}},
/*35*/	{KF_NEW, "FirstNode", {new:{kFirstNode, "Zl"}}},
/*36*/	{KF_NEW, "LastNode", {new:{kLastNode, "l"}}},
/*37*/	{KF_NEW, "EmptyList", {new:{kEmptyList, "l"}}},
/*38*/	{KF_NEW, "NextNode", {new:{kNextNode, "n"}}},
/*39*/	{KF_NEW, "PrevNode", {new:{kPrevNode, "n"}}},
/*3a*/	{KF_NEW, "NodeValue", {new:{kNodeValue, "n"}}},
/*3b*/	{KF_NEW, "AddAfter", {new:{kAddAfter, "lnn"}}},
/*3c*/	{KF_NEW, "AddToFront", {new:{kAddToFront, "ln"}}},
/*3d*/	{KF_NEW, "AddToEnd", {new:{kAddToEnd, "ln"}}},
/*3e*/	{KF_NEW, "FindKey", {new:{kFindKey, "l."}}},
/*3f*/	{KF_NEW, "DeleteKey", {new:{kDeleteKey, "l."}}},
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
/*4f*/	{KF_NEW, "BaseSetter", {new:{kBaseSetter, "o"}}},
/*50*/	{KF_OLD, "DirLoop", {old:kDirLoop}},
/*51*/	{KF_NEW, "CanBeHere", {new:{kCanBeHere, "ol*"}}},
/*52*/	{KF_OLD, "OnControl", {old:kOnControl}},
/*53*/	{KF_NEW, "InitBresen", {new:{kInitBresen, "oi*"}}},
/*54*/	{KF_NEW, "DoBresen", {new:{kDoBresen, "o"}}},
/*55*/	{KF_OLD, "DoAvoider", {old:kDoAvoider}},
/*56*/	{KF_OLD, "SetJump", {old:kSetJump}},
/*57*/	{KF_OLD, "SetDebug", {old:kSetDebug}},
/*58*/	{KF_NONE, "InspectObj"},
/*59*/	{KF_NONE, "ShowSends"},
/*5a*/	{KF_NONE, "ShowObjs"},
/*5b*/	{KF_NONE, "ShowFree"},
/*5c*/	{KF_NEW, "MemoryInfo", {new:{kMemoryInfo, "i"}}},
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



reg_t
kMemoryInfo(state_t *s, int funct_nr, int argc, reg_t *argv)
{
	switch (argv[0].offset) {
	case 0: /* Total free heap memory */
	case 1: /* Largest heap block available */
	case 2: /* Largest available hunk memory block */
	case 3: /* Total amount of hunk memory */
	case 4: /* Amount of free DOS paragraphs- SCI01 */
		return make_reg(0, 0x7fff); /* Must not be 0xffff, or some memory calculations will overflow */

	default: SCIkwarn(SCIkWARNING, "Unknown MemoryInfo operation: %04x\n", argv[0].offset);
	}
	return NULL_REG;
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
kernel_compile_signature(char **s)
{
	char *src = *s;
	char *result;
	int ellipsis = 0;
	char v;
	int index = 0;

	if (!src)
		return; /* NULL signature: Nothing to do */

	result = sci_malloc(strlen(*s) + 1);

	while (*src) {
		char c;
		v = 0;

		if (ellipsis) {
			sciprintf("INTERNAL ERROR when compiling kernel"
				  " function signature '%s': non-terminal ellipsis\n", *s,
				  *src);
			exit(1);
		}

		do {
			char cc;
			cc = c = *src++;
			if (c >= 'A' || c <= 'Z')
				cc = c | KSIG_SPEC_SUM_DONE;

			switch (cc) {

			case KSIG_SPEC_LIST:
				v |= KSIG_LIST;
				break;

			case KSIG_SPEC_NODE:
				v |= KSIG_NODE;
				break;

			case KSIG_SPEC_REF:
				v |= KSIG_REF;
				break;

			case KSIG_SPEC_OBJECT:
				v |= KSIG_OBJECT;
				break;

			case KSIG_SPEC_ARITHMETIC:
				v |= KSIG_ARITHMETIC;
				break;

			case KSIG_SPEC_NULL:
				v |= KSIG_NULL;
				break;

			case KSIG_SPEC_ANY:
				v |= KSIG_ANY;
				break;

			case KSIG_SPEC_ELLIPSIS:
				v |= KSIG_ELLIPSIS;
				ellipsis = 1;
				break;

			default: {
				sciprintf("INTERNAL ERROR when compiling kernel"
					  " function signature '%s': (%02x) not understood (aka"
					  " '%c')\n",
					  *s, c, c);
				exit(1);
			}

			}
		} while (*src && (*src == '*' || (c < 'a' && c != '.')));

		/* To handle sum types */
		result[index++] = v;
	}

	result[index] = 0;
	*s = result; /* Write back */
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
			s->kfunct_table[functnr].signature = NULL;
			++ignored;
			break;

		case KF_NEW:
			s->kfunct_table[functnr] = kfunct_mappers[found].data.new;
			kernel_compile_signature(&(s->kfunct_table[functnr].signature));
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

void
free_kfunct_tables(state_t *s)
{
	int i;

	for (i = 0; i < s->kernel_names_nr; i++)
		if (s->kfunct_table[i].signature)
			free(s->kfunct_table[i].signature);

	sci_free(s->kfunct_table);
	s->kfunct_table = NULL;

	sci_free(s->kfunct_emu_table);
	s->kfunct_emu_table = NULL;
}

int
determine_reg_type(state_t *s, reg_t reg)
{
	mem_obj_t *mobj;

	if (!reg.segment) {
		if (!reg.offset)
			return KSIG_ARITHMETIC | KSIG_NULL;
		return KSIG_ARITHMETIC;
	}

	if ((reg.segment >= s->seg_manager.heap_size)
	    || !s->seg_manager.heap[reg.segment])
		return 0; /* Invalid */

	mobj = s->seg_manager.heap[reg.segment];

	switch (mobj->type) {

	case MEM_OBJ_SCRIPT:
		if (reg.offset <= mobj->data.script.buf_size
		    && reg.offset >= -SCRIPT_OBJECT_MAGIC_OFFSET
		    && RAW_IS_OBJECT(mobj->data.script.buf + reg.offset)) {
			int idx = RAW_GET_CLASS_INDEX(mobj->data.script.buf + reg.offset);
			if (idx >= 0 && idx < mobj->data.script.objects_nr)
				return KSIG_OBJECT;
			else
				return KSIG_REF;
		} else
			return KSIG_REF;

	case MEM_OBJ_CLONES:
		if (ENTRY_IS_VALID(&(mobj->data.clones), reg.offset))
			return KSIG_OBJECT;
		else
			return 0;

	case MEM_OBJ_LOCALS:
		if (reg.offset < mobj->data.locals.nr)
			return KSIG_REF;
		else
			return 0;

	case MEM_OBJ_STACK:
		if (reg.offset < mobj->data.stack.nr)
			return KSIG_REF;
		else
			return 0;

	case MEM_OBJ_SYS_STRINGS:
		if (reg.offset < SYS_STRINGS_MAX
		    && mobj->data.sys_strings.strings[reg.offset].name)
			return KSIG_REF;
		else
			return 0;

	case MEM_OBJ_LISTS:
		if (ENTRY_IS_VALID(&(mobj->data.lists), reg.offset))
			return KSIG_LIST;
		else
			return 0;

	case MEM_OBJ_NODES:
		if (ENTRY_IS_VALID(&(mobj->data.nodes), reg.offset))
			return KSIG_NODE;
		else
			return 0;

	default:
		return 0;

	}
}

int
kernel_matches_signature(state_t *s, char *sig, int argc, reg_t *argv)
{
	if (!sig)
		return 1;

	while (*sig && argc) {
		if (*sig != KSIG_ANY) {
			int type = determine_reg_type(s, *argv);

			if (!type) {
				sciprintf("[KERN] Could not determine type of ref "PREG";"
					  " failing signature check\n",
					  PRINT_REG(*argv));
				return 0;
			}

			if (!(type & *sig))
				return 0;

		}
		if (!(*sig & KSIG_ELLIPSIS))
			++sig;
		++argv;
		--argc;
	}

	if (argc)
		return 0; /* Too many arguments */
	else
		return (*sig == 0 || (*sig & KSIG_ELLIPSIS));
}

reg_t *
kernel_dereference_pointer(struct _state *s, reg_t pointer, int entries)
{
	mem_obj_t *mobj;
	reg_t *base = NULL;
	int count;

	if (!pointer.segment
	    || (pointer.segment >= s->seg_manager.heap_size)
	    || !s->seg_manager.heap[pointer.segment]) {
		return NULL; /* Invalid */
		SCIkdebug(SCIkERROR, "Attempt to dereference invalid pointer "PREG"!",
			  PRINT_REG(pointer));
		return NULL;
	}

	mobj = s->seg_manager.heap[pointer.segment];

	switch (mobj->type) {

	case MEM_OBJ_SCRIPT:
		if (entries || pointer.offset > mobj->data.script.buf_size) {
			SCIkdebug(SCIkERROR, "Attempt to dereference invalid pointer "PREG
				  " into script segment (script size=%d, space requested=%d)\n",
				  PRINT_REG(pointer), mobj->data.script.buf_size, entries);
			return NULL;
		}
		return (reg_t *) (mobj->data.script.buf + pointer.offset);
		break;

	case MEM_OBJ_LOCALS:
		count = mobj->data.locals.nr;
		base = mobj->data.locals.locals;
		break;

	case MEM_OBJ_STACK:
		count = mobj->data.stack.nr;
		base = mobj->data.stack.entries;
		break;
		
	case MEM_OBJ_SYS_STRINGS:
		if (pointer.offset < SYS_STRINGS_MAX
		    && mobj->data.sys_strings.strings[pointer.offset].name)
			return (reg_t *) (mobj->data.sys_strings.strings[pointer.offset].value);
		else {
			SCIkdebug(SCIkERROR, "Attempt to dereference invalid pointer "PREG"!",
				  PRINT_REG(pointer));
			return NULL;
		}

	default:
		SCIkdebug(SCIkERROR, "Trying to dereference pointer "PREG" to inappropriate"
			  " segment!",
			  PRINT_REG(pointer));
		return NULL;
	}

	if (pointer.offset + entries > count) {
		SCIkdebug(SCIkERROR, "Trying to dereference pointer "PREG" beyond end of segment!",
			  PRINT_REG(pointer));
		return NULL;
	} return
		base + pointer.offset;
}
