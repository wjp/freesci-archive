/***************************************************************************
 midiout_dcraw.c Copyright (C) 2004 Walter van Niftrik


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

   Walter van Niftrik <w.f.b.w.v.niftrik@stud.tue.nl>

***************************************************************************/

#include <sys/types.h>
#include <sciresource.h>
#include <midiout.h>

#define SCSMR2 (*((volatile uint16 *) 0xffe80000))
#define SCBRR2 (*((volatile uint8 *) 0xffe80004))
#define SCSCR2 (*((volatile uint16 *) 0xffe80008))
#define SCFCR2 (*((volatile uint16 *) 0xffe80018))
#define SCFSR2 (*((volatile uint16 *) 0xffe80010))
#define SCFTDR2 (*((volatile uint8 *) 0xffe8000c))

static void
serial_init() {
	/* Select internal clock. Clear RIE, TIE, TE and RE, to 0. */
	SCSCR2 = 0x0000;
	
	/* Set TFRST in SCFCR2 to 1. */
	SCFCR2 = 0x0004;

	/* Select 8N1. Use P0 clock (50 Mhz). */
	SCSMR2 = 0x0000;
	
	/* Set 31250 bps. Formula: 50000000 / (32 * baud rate) - 1. */
	SCBRR2 = 50000000 / (32 * 31250) - 1;

	usleep(1000);

	/* Clear TFRST bit to 0. */
	SCFCR2 = 0x0000;

	/* Set TE bit in SCSCR2 to 1. */
	SCSCR2 = 0x0020;
}

static void
serial_write(int data) {
	int timeout = 100000;

	/* Wait until there is space in the buffer */
	while (!(SCFSR2 & 0x0020) && timeout > 0)
		timeout--;

	if (timeout <= 0) {
		sciprintf("Warning: MIDI send timeout.\n");
		return;
	}

	SCFTDR2 = data;

	SCFSR2 &= 0x0093;
}
static int
midiout_dcraw_set_parameter(struct _midiout_driver *drv, char *attribute, char *value)
{
	sciprintf("Unknown dcraw option '%s'!\n", attribute);

	return 0;
}

static int
midiout_dcraw_open()
{
	serial_init();

	return 0;
}

static int
midiout_dcraw_close()
{
	return 0;
}

static int
midiout_dcraw_flush(guint8 code)
{
	return 0;
}

static int
midiout_dcraw_write(guint8 *buffer, unsigned int count, guint32 delta)
{
	int i;
	for (i = 0; i < count; i++)
		serial_write(buffer[i]);
	return count;
}

midiout_driver_t midiout_driver_dcraw = {
  "dcraw",
  "v0.01",
  &midiout_dcraw_set_parameter,
  &midiout_dcraw_open,
  &midiout_dcraw_close,
  &midiout_dcraw_write,
  &midiout_dcraw_flush
};
