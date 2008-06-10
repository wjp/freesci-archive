# Build file for the ScummVM plugin

MODULE := engines/sci

MODULE_OBJS = \
	src/engine/game.o \
	src/engine/gc.o \
	src/engine/grammar.o \
	src/engine/kernel.o \
	src/engine/kevent.o \
	src/engine/kfile.o \
	src/engine/kgraphics.o \
	src/engine/klists.o \
	src/engine/kmath.o \
	src/engine/kmenu.o \
	src/engine/kmovement.o \
	src/engine/kpathing.o \
	src/engine/kscripts.o \
	src/engine/ksound.o \
	src/engine/kstring.o \
	src/engine/said.o \
	src/engine/savegame.o \
	src/engine/scriptconsole.o \
	src/engine/scriptdebug.o \
	src/engine/seg_manager.o \
	src/engine/sys_strings.o \
	src/engine/vm.o \
	src/gfx/antialias.o \
	src/gfx/font.o \
	src/gfx/font-5x8.o \
	src/gfx/font-6x10.o \
	src/gfx/gfx_res_options.o \
	src/gfx/gfx_resource.o \
	src/gfx/gfx_support.o \
	src/gfx/gfx_tools.o \
	src/gfx/menubar.o \
	src/gfx/operations.o \
	src/gfx/resmgr.o \
	src/gfx/sbtree.o \
	src/gfx/sci_widgets.o \
	src/gfx/widgets.o \
	src/gfx/drivers/scummvm_driver.o \
	src/gfx/resource/sci_cursor_0.o \
	src/gfx/resource/sci_font.o \
	src/gfx/resource/sci_pal_1.o \
	src/gfx/resource/sci_pic_0.o \
	src/gfx/resource/sci_resmgr.o \
	src/gfx/resource/sci_view_0.o \
	src/gfx/resource/sci_view_1.o \
	src/scicore/aatree.o \
	src/scicore/console.o \
	src/scicore/decompress0.o \
	src/scicore/decompress01.o \
	src/scicore/decompress1.o \
	src/scicore/decompress11.o \
	src/scicore/exe.o \
	src/scicore/exe_lzexe.o \
	src/scicore/exe_raw.o \
	src/scicore/int_hashmap.o \
	src/scicore/reg_t_hashmap.o \
	src/scicore/resource.o \
	src/scicore/resource_map.o \
	src/scicore/resource_patch.o \
	src/scicore/sci_memory.o \
	src/scicore/script.o \
	src/scicore/tools.o \
	src/scicore/versions.o \
	src/scicore/vocab.o \
	src/scicore/vocab_debug.o \
	src/scummvm/detection.o \
	src/scummvm/scummvm_engine.o \
	src/sfx/adlib.o \
	src/sfx/core.o \
	src/sfx/iterator.o \
	src/sfx/pcm-iterator.o \
	src/sfx/songlib.o \
	src/sfx/time.o \
	src/sfx/device/devices.o \
	src/sfx/mixer/mixers.o \
	src/sfx/mixer/soft.o \
	src/sfx/pcm_device/pcm_devices.o \
	src/sfx/pcm_device/scummvm.o \
	src/sfx/player/players.o \
	src/sfx/player/polled.o \
	src/sfx/player/realtime.o \
	src/sfx/seq/sequencers.o \
	src/sfx/softseq/amiga.o \
	src/sfx/softseq/fmopl.o \
	src/sfx/softseq/opl2.o \
	src/sfx/softseq/pcspeaker.o \
	src/sfx/softseq/SN76496.o \
	src/sfx/softseq/softsequencers.o \
	src/sfx/timer/scummvm.o \
	src/sfx/timer/timers.o

CPPFLAGS+=-DSCUMMVM -Iengines/sci/src/include

# Build .c files as C++
%.o: %.c
	$(MKDIR) $(*D)/$(DEPDIR)
	$(CXX) -Wp,-MMD,"$(*D)/$(DEPDIR)/$(*F).d",-MQ,"$@",-MP $(CXXFLAGS) $(CPPFLAGS) -c $(<) -o $*.o

# Generate savegame.cpp
$(srcdir)/engines/sci/src/engine/savegame.cpp: $(srcdir)/engines/sci/src/engine/savegame.cfsml
	cat $< | perl $(srcdir)/engines/sci/engine/cfsml.pl -f savegame.cfsml > $@

# This module can be built as a plugin
ifeq ($(ENABLE_SCI), DYNAMIC_PLUGIN)
PLUGIN := 1
endif

# Include common rules
include $(srcdir)/rules.mk
