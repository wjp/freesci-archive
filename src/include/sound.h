/***************************************************************************
 sound.h (C) 1999 Christoph Reichenbach, TU Darmstadt


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
/* Everything concerning the handling of sound.
** As usual, don't rely on the values of the #defines to be final; those
** may very well change if support for OSS or ALSA (or whatever) is added.
*/

#ifndef _SCI_SOUND_H_
#define _SCI_SOUND_H_

#include <config.h>
#ifdef HAVE_GSI_GSI_INTERFACE_C
#define HAVE_GSI
/* This is somewhat shorter */
#endif /* HAVE_GSI_GSI_INTERFACE_C */


#define SCI_SOUND_DEBUG

void _SCIsdebug(const char *format, ...);
#ifdef SCI_SOUND_DEBUG
#define SCIsdebug _SCIsdebug
#else /* !SCI_SOUND_DEBUG */
#define SCIsdebug 1 ? (void)0 : _SCIsdebug
#endif /* !SCI_SOUND_DEBUG */

/* [DJ] SCIswarn cannot be used with more than one parameter: Visual C++
** doesn't support ... in macro parameter lists, and since __FILE__ is
** used, I cannot circumvent it the same way as I did for SCIsdebug.
** If needed, it is possible to define similar macros with any number
** of parametes.
*/

#ifdef __GNUC__
#define SCIswarn(format, param) \
        fprintf(stderr, "FSCI sound: file %s %d (%s): " format, \
		__FILE__, \
		__LINE__, \
		__PRETTY_FUNCTION__, \
		## param);
#else /* !__GNUC__ */
#define SCIswarn(format, param) \
        fprintf(stderr, "FSCI sound: file %s %d: " format, \
		__FILE__, \
		__LINE__, \
		## param);
#endif /* !__GNUC__ */

#define obstack_chunk_alloc malloc
#define obstack_chunk_free free

#include <resource.h>
#include <obstack.h>


#define SCI_SOUND_INTERFACE_NONE 0
#define SCI_SOUND_INTERFACE_GSI 1
#define SCI_SOUND_INTERFACE_LAST SCI_SOUND_INTERFACE_GSI
#define SCI_SOUND_INTERFACE_AUTODETECT 42

#define SCI_SOUND_LOOP 0
#define SCI_SOUND_NOLOOP 1


extern int sci_sound_interface;
/* Containts the current sound interface */

static char *SCI_sound_interfaces[] = {
  "No sound output",
  "General Sound Interface (GSI)"
};

extern gint8 SCI0_snd2gm_map[128];
/* Maps SCI0 sound resource MIDI instruments to gm instruments */



int
initSound(int mode);
/* Initializes the selected sound interface
** Parameters: mode: An output method which should be used
** Returns   : (int) The output method actually chosen
** If a viable output method had already been selected, initSound() will
** uninitialize it. It will also register an uninitialization function with
** atexit().
** The return value will always be equal either to mode or to
** SCI_SOUND_INTERFACE_NONE, unless (mode == SCI_SOUND_INTERFACE_AUTODETECT).
*/


int
playSound(guint8 *data, int loop);
/* Plays a sound resource
** Parameters: data: Points to the resource data to play
**             loop: One of SCI_SOUND_LOOP or SCI_SOUND_NOLOOP, to determine
**             whether the sound should be looped or played only once.
** Returns   : (int) 0 on success, 1 if playing failed
*/


int
setSoundVolume(int volume);
/* Sets the sound output volume
** Parameters: volume: A volume from 0 to 255, with 255 being the maximum
** Returns   : (int) 0 on success, 1 if setting failed
*/


void
stopSound(void);
/* Tries to stop any sound currently being played
** Parameters: void
** Returns   : void
*/


guint8 *
makeMIDI0(const guint8 *src, int *size);
/* Turns a sound resource into a MIDI block.
** Parameters: src: points to the resource.data information
**             size: points to an int which is set to contain the MIDI
**                   block length.
** Returns   : (guint8 *) The malloc()ed MIDI block, or NULL if conversion
**             failed.
** *FIXME*: Aborts in some cases if out of memory. This is OK while testing,
** but should be adjusted for the public release.
*/

int
mapMIDIInstruments(void);
/* Automagically detects MIDI instrument mappings
** Parameters: (void)
** Returns   : (int) 0 on success, 1 on failure
** This function requires patch.002 to be present, or it will fail. However,
** for SCI1 and later, it will expect MIDI information to be stored as GM,
** and return with success.
** If detection fails, the MIDI maps will default to 1:1 General MIDI mappings.
*/

#endif /* _SCI_SOUND_H_ */
