# Set SDK_BASE to the base directory of Mirko Roller's SDK.

CC = arm-elf-gcc
LD = arm-elf-gcc
AS = arm-elf-as
AR = arm-elf-ar

PRG  = freesci
FSCI_BASE = ../..
OBJS =	$(FSCI_BASE)/src/engine/game.o \
	$(FSCI_BASE)/src/engine/grammar.o \
	$(FSCI_BASE)/src/engine/heap.o \
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
	$(FSCI_BASE)/src/engine/savegame.o \
	$(FSCI_BASE)/src/engine/scriptdebug.o \
	$(FSCI_BASE)/src/engine/simplesaid.o \
	$(FSCI_BASE)/src/engine/vm.o \
	$(FSCI_BASE)/src/gfx/antialias.o \
	$(FSCI_BASE)/src/gfx/drivers/gp32_driver.o \
	$(FSCI_BASE)/src/gfx/drivers/gfx_drivers.o \
	$(FSCI_BASE)/src/gfx/drivers/null_driver.o \
	$(FSCI_BASE)/src/gfx/font.o \
	$(FSCI_BASE)/src/gfx/font-5x8.o \
	$(FSCI_BASE)/src/gfx/font-6x10.o \
	$(FSCI_BASE)/src/gfx/gfx_console.o \
	$(FSCI_BASE)/src/gfx/gfx_resource.o \
	$(FSCI_BASE)/src/gfx/gfx_support.o \
	$(FSCI_BASE)/src/gfx/gfx_tools.o \
	$(FSCI_BASE)/src/gfx/menubar.o \
	$(FSCI_BASE)/src/gfx/operations.o \
	$(FSCI_BASE)/src/gfx/resmgr.o \
	$(FSCI_BASE)/src/gfx/resource/sci_cursor_0.o \
	$(FSCI_BASE)/src/gfx/resource/sci_font.o \
	$(FSCI_BASE)/src/gfx/resource/sci_pal_1.o \
	$(FSCI_BASE)/src/gfx/resource/sci_pic_0.o \
	$(FSCI_BASE)/src/gfx/resource/sci_resmgr.o \
	$(FSCI_BASE)/src/gfx/resource/sci_view_0.o \
	$(FSCI_BASE)/src/gfx/resource/sci_view_1.o \
	$(FSCI_BASE)/src/gfx/sbtree.o \
	$(FSCI_BASE)/src/gfx/sci_widgets.o \
	$(FSCI_BASE)/src/gfx/widgets.o \
	$(FSCI_BASE)/src/main.o \
	$(FSCI_BASE)/src/config.o \
	$(FSCI_BASE)/src/menu/game_select.o \
	$(FSCI_BASE)/src/menu/game_select_init.o \
	$(FSCI_BASE)/src/menu/game_select_screen.o \
	$(FSCI_BASE)/src/scicore/console.o \
	$(FSCI_BASE)/src/scicore/decompress0.o \
	$(FSCI_BASE)/src/scicore/decompress01.o \
	$(FSCI_BASE)/src/scicore/decompress1.o \
	$(FSCI_BASE)/src/scicore/decompress11.o \
	$(FSCI_BASE)/src/scicore/fnmatch.o \
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
	$(FSCI_BASE)/src/sound/fmopl.o \
	$(FSCI_BASE)/src/sound/midi_adlibemu.o \
	$(FSCI_BASE)/src/sound/midi_device.o \
	$(FSCI_BASE)/src/sound/midi_mt32.o \
	$(FSCI_BASE)/src/sound/midi_mt32gm.o \
	$(FSCI_BASE)/src/sound/midiout.o \
	$(FSCI_BASE)/src/sound/oldmidi.o \
	$(FSCI_BASE)/src/sound/pcmout.o \
	$(FSCI_BASE)/src/sound/pcmout_sdl.o \
	$(FSCI_BASE)/src/sound/polled_ss.o \
	$(FSCI_BASE)/src/sound/sfx_save.o \
	$(FSCI_BASE)/src/sound/sound.o \
	$(FSCI_BASE)/src/sound/soundserver.o \
	$(FSCI_BASE)/src/gp32/wrap.o \
	$(FSCI_BASE)/src/gp32/main.o \
	$(FSCI_BASE)/src/dc/keyboard.o

LIBS      = -L$(SDK_BASE)/lib -lmirkoSDK -lm
CRT0      = $(SDK_BASE)/lib/crt0.S
LNKSCRIPT = $(SDK_BASE)/lib/lnkscript
INCLUDES  = -I. -I$(FSCI_BASE)/src/include -I$(SDK_BASE)/lib.src/include \
	-I$(FSCI_BASE)/src/include/beos -I$(FSCI_BASE)/src/dc
CFLAGS    = $(INCLUDES) -O2 -g -Wall -mtune=arm9tdmi -DHAVE_CONFIG_H \
	-DWANT_CONSOLE -DFREESCI_GAMEDIR=\"FREESCI\" \
	-DSTRLEN_FREESCI_GAMEDIR=7 -DMAX_GAMEDIR_SIZE=9 -D_GP32

all: $(PRG).fxe

$(PRG).fxe: $(PRG).bin
	b2fxec -a FreeSCI_team -t FreeSCI $(PRG).bin $(PRG).fxe

$(PRG).elf: $(OBJS) crt0.o
	$(LD) -nostartfiles -Wall -T $(LNKSCRIPT) crt0.o -o $(PRG).elf \
	$(OBJS) $(LIBS)

$(PRG).bin: $(PRG).elf
	arm-elf-objcopy -O binary $(PRG).elf $(PRG).bin

crt0.o:	$(CRT0)
	$(CC) -c -o crt0.o $(CRT0)

run: $(PRG).fxe
	gplink run $(PRG).fxe

clean:
	rm -f crt0.o $(OBJS) $(PRG).bin $(PRG).elf $(PRG).fxe
