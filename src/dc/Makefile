BIN_TARGET = freesci.bin
ELF_TARGET = freesci.elf

FSCI_BASE = ../..

OBJS =	$(FSCI_BASE)/src/config.o \
	$(FSCI_BASE)/src/dc/3dutils.o \
	$(FSCI_BASE)/src/dc/bkg.o \
	$(FSCI_BASE)/src/dc/bsearch.o \
	$(FSCI_BASE)/src/dc/dc.o \
	$(FSCI_BASE)/src/dc/dc_save.o \
	$(FSCI_BASE)/src/dc/gamemenu.o \
	$(FSCI_BASE)/src/dc/selectgame.o \
	$(FSCI_BASE)/src/dc/snd_stream.o \
	$(FSCI_BASE)/src/dc/snprintf.o \
	$(FSCI_BASE)/src/dc/texture.o \
	$(FSCI_BASE)/src/engine/game.o \
	$(FSCI_BASE)/src/engine/grammar.o \
	$(FSCI_BASE)/src/engine/kernel.o \
	$(FSCI_BASE)/src/engine/kevent.o \
	$(FSCI_BASE)/src/engine/kfile.o \
	$(FSCI_BASE)/src/engine/kgraphics.o \
	$(FSCI_BASE)/src/engine/klists.o \
	$(FSCI_BASE)/src/engine/kmath.o \
	$(FSCI_BASE)/src/engine/kmenu.o \
	$(FSCI_BASE)/src/engine/kmovement.o \
	$(FSCI_BASE)/src/engine/kpathing.o \
	$(FSCI_BASE)/src/engine/kscripts.o \
	$(FSCI_BASE)/src/engine/ksound.o \
	$(FSCI_BASE)/src/engine/kstring.o \
	$(FSCI_BASE)/src/engine/said.o \
	$(FSCI_BASE)/src/engine/scriptdebug.o \
	$(FSCI_BASE)/src/engine/seg_manager.o \
	$(FSCI_BASE)/src/engine/sys_strings.o \
	$(FSCI_BASE)/src/engine/vm.o \
	$(FSCI_BASE)/src/gfx/antialias.o \
	$(FSCI_BASE)/src/gfx/drivers/dc_driver.o \
	$(FSCI_BASE)/src/gfx/drivers/gfx_drivers.o \
	$(FSCI_BASE)/src/gfx/drivers/null_driver.o \
	$(FSCI_BASE)/src/gfx/font.o \
	$(FSCI_BASE)/src/gfx/font-5x8.o\
	$(FSCI_BASE)/src/gfx/font-6x10.o\
	$(FSCI_BASE)/src/gfx/gfx_console.o \
	$(FSCI_BASE)/src/gfx/gfx_resource.o \
	$(FSCI_BASE)/src/gfx/gfx_res_options.o \
	$(FSCI_BASE)/src/gfx/gfx_support.o \
	$(FSCI_BASE)/src/gfx/gfx_tools.o \
	$(FSCI_BASE)/src/gfx/menubar.o \
	$(FSCI_BASE)/src/gfx/operations.o \
	$(FSCI_BASE)/src/gfx/resmgr.o \
	$(FSCI_BASE)/src/gfx/resource/sci_cursor_0.o \
	$(FSCI_BASE)/src/gfx/resource/sci_font.o \
	$(FSCI_BASE)/src/gfx/resource/sci_pal_1.o \
	$(FSCI_BASE)/src/gfx/resource/sci_pic_0.o \
	$(FSCI_BASE)/src/gfx/resource/sci_pic_1.o \
	$(FSCI_BASE)/src/gfx/resource/sci_resmgr.o \
	$(FSCI_BASE)/src/gfx/resource/sci_view_0.o \
	$(FSCI_BASE)/src/gfx/resource/sci_view_1.o \
	$(FSCI_BASE)/src/gfx/sbtree.o \
	$(FSCI_BASE)/src/gfx/sci_widgets.o \
	$(FSCI_BASE)/src/gfx/widgets.o \
	$(FSCI_BASE)/src/main.o \
	$(FSCI_BASE)/src/menu/game_select_init.o \
	$(FSCI_BASE)/src/menu/game_select_screen.o \
	$(FSCI_BASE)/src/scicore/console.o \
	$(FSCI_BASE)/src/scicore/decompress0.o \
	$(FSCI_BASE)/src/scicore/decompress01.o \
	$(FSCI_BASE)/src/scicore/decompress1.o \
	$(FSCI_BASE)/src/scicore/decompress11.o \
	$(FSCI_BASE)/src/scicore/fnmatch.o \
	$(FSCI_BASE)/src/scicore/int_hashmap.o \
	$(FSCI_BASE)/src/scicore/modules.o \
	$(FSCI_BASE)/src/scicore/old_objects.o \
	$(FSCI_BASE)/src/scicore/resource.o \
	$(FSCI_BASE)/src/scicore/resource_map.o \
	$(FSCI_BASE)/src/scicore/resource_patch.o \
	$(FSCI_BASE)/src/scicore/sci_memory.o \
	$(FSCI_BASE)/src/scicore/script.o \
	$(FSCI_BASE)/src/scicore/tools.o \
	$(FSCI_BASE)/src/scicore/vocab.o \
	$(FSCI_BASE)/src/scicore/vocab_debug.o \
	$(FSCI_BASE)/src/sfx/iterator.o \
	$(FSCI_BASE)/src/sfx/songlib.o \
	$(FSCI_BASE)/src/sfx/core.o \
	$(FSCI_BASE)/src/sfx/adlib.o \
	$(FSCI_BASE)/src/sfx/pcm-iterator.o \
	$(FSCI_BASE)/src/sfx/time.o \
	$(FSCI_BASE)/src/sfx/player/players.o \
	$(FSCI_BASE)/src/sfx/player/realtime.o \
	$(FSCI_BASE)/src/sfx/player/polled.o \
	$(FSCI_BASE)/src/sfx/device/devices.o \
	$(FSCI_BASE)/src/sfx/timer/timers.o \
	$(FSCI_BASE)/src/sfx/seq/sequencers.o \
	$(FSCI_BASE)/src/sfx/mixer/mixers.o \
	$(FSCI_BASE)/src/sfx/mixer/soft.o \
	$(FSCI_BASE)/src/sfx/pcm_device/pcm_devices.o \
	$(FSCI_BASE)/src/sfx/pcm_device/audiobuf.o \
	$(FSCI_BASE)/src/sfx/softseq/softsequencers.o \
	$(FSCI_BASE)/src/sfx/softseq/SN76496.o \
	$(FSCI_BASE)/src/sfx/softseq/fmopl.o \
	$(FSCI_BASE)/src/sfx/softseq/opl2.o \
	$(FSCI_BASE)/src/sfx/softseq/pcspeaker.o

all: $(BIN_TARGET)

include $(KOS_BASE)/Makefile.rules

KOS_LOCAL_CFLAGS := -I$(FSCI_BASE)/src/dc -I. -I$(FSCI_BASE)/src/include \
	-I$(FSCI_BASE)/src/include/beos -D_DREAMCAST -DHAVE_CONFIG_H \
	-DWANT_CONSOLE -DYY_NEVER_INTERACTIVE

clean: rm-elf rm-bin
	-rm -f $(OBJS)

rm-elf:
	-rm -f $(ELF_TARGET)

rm-bin:
	-rm -f $(BIN_TARGET)

$(ELF_TARGET): $(OBJS)
	$(KOS_CC) $(KOS_CFLAGS) $(KOS_LDFLAGS) -o $(ELF_TARGET) $(KOS_START) \
		$(OBJS) $(OBJEXTRA) -lm -lz $(KOS_LIBS)

$(BIN_TARGET): $(ELF_TARGET)
	$(KOS_OBJCOPY) -O binary $(ELF_TARGET) $(BIN_TARGET)

run: $(BIN_TARGET)
	$(KOS_LOADER) $(BIN_TARGET)
