/***************************************************************************
 midi.c (C) 1999 Christoph Reichenbach, TU Darmstadt


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
/* Turns a resource into a valid midi block */

#include <sound.h>

#define SCI_SOUND_MIDI0_NOMAP
#define MAX_MIDI_MAPS 4
/* Maximum amount of emulated MIDI instrument banks */

gint8 MIDI_map_default[128] = {
#ifdef SCI_SOUND_MIDI0_NOMAP
  0X00, 0X01, 0X02, 0X03, 0X04, 0X05, 0X06, 0X07, 0X08, 0X09, 0X0A, 0X0B, 0X0C, 0X0D, 0X0E, 0X0F,
  0X10, 0X11, 0X12, 0X13, 0X14, 0X15, 0X16, 0X17, 0X18, 0X19, 0X1A, 0X1B, 0X1C, 0X1D, 0X1E, 0X1F,
  0X20, 0X21, 0X22, 0X23, 0X24, 0X25, 0X26, 0X27, 0X28, 0X29, 0X2A, 0X2B, 0X2C, 0X2D, 0X2E, 0X2F,
  0X30, 0X31, 0X32, 0X33, 0X34, 0X35, 0X36, 0X37, 0X38, 0X39, 0X3A, 0X3B, 0X3C, 0X3D, 0X3E, 0X3F,
  0X40, 0X41, 0X42, 0X43, 0X44, 0X45, 0X46, 0X47, 0X48, 0X49, 0X4A, 0X4B, 0X4C, 0X4D, 0X4E, 0X4F,
  0X50, 0X51, 0X52, 0X53, 0X54, 0X55, 0X56, 0X57, 0X58, 0X59, 0X5A, 0X5B, 0X5C, 0X5D, 0X5E, 0X5F,
  0X60, 0X61, 0X62, 0X63, 0X64, 0X65, 0X66, 0X67, 0X68, 0X69, 0X6A, 0X6B, 0X6C, 0X6D, 0X6E, 0X6F,
  0X70, 0X71, 0X72, 0X73, 0X74, 0X75, 0X76, 0X77, 0X78, 0X79, 0X7A, 0X7B, 0X7C, 0X7D, 0X7E, 0X7F};
#else /* !SCI_SOUND_MIDI0_NOMAP */
  0X00, 0X01, 0X02, 0X2b, 0x38, 0x39, 0x3c, 0X2b, 0X2c, 0X09, 0x09, 0X0B, 0X0C, 0X0D, 0X0E, 0X0F,
  0X10, 0X11, 0X12, 0X13, 0X14, 0X15, 0X16, 0x20, 0X18, 0X19, 0X1A, 0X1B, 0X1C, 0x18, 0x30, 0X1F,
  0X20, 0X21, 0X22, 0X23, 0X24, 0X25, 0X26, 0X27, 0X28, 0X29, 0X2A, 0X2B, 0X2C, 0X2D, 0X2E, 0X2F,
  0X30, 0X31, 0X32, 0X33, 0X34, 0X35, 0X36, 0X37, 0X38, 0X39, 0X3A, 0X3B, 0X3C, 0X3D, 0X3E, 0X3F,
  0X40, 0X41, 0X42, 0X43, 0X44, 0X45, 0X46, 0X47, 0X48, 0X49, 0X4A, 0X4B, 0X4C, 0X4D, 0X4E, 0X4F,
  0X50, 0X51, 0X52, 0X53, 0X54, 0X55, 0X56, 0X57, 0X58, 0X59, 0X5A, 0X5B, 0X5C, 0X5D, 0X36, 0X5F,
  0X60, 0X61, 0X62, 0X63, 0X64, 0X65, 0X66, 0X67, 0X68, 0X69, 0X6A, 0X6B, 0X6C, 0X6D, 0X6E, 0X6F,
  0X70, 0X71, 0X72, 0X73, 0X74, 0X75, 0X76, 0X77, 0X78, 0X79, 0X7A, 0X7B, 0X7C, 0X7D, 0X7E, 0X7F};
#endif /* !SCI_SOUND_MIDI0_NOMAP */
/* The map above is obsolete.
*/


static gint8 MIDI_map[MAX_MIDI_MAPS][128];


static char *GM_Instrument_Names[] = {
  /*00*/  "Acoustic Grand Piano",
  /*01*/  "Bright Acoustic Piano",
  /*02*/  "Electric Grand Piano",
  /*03*/  "Honky-Tonk",
  /*04*/  "Rhodes Piano",
  /*05*/  "Chorused Piano",
  /*06*/  "Harpsichord",
  /*07*/  "Clavinet",
  /*08*/  "Celesta",
  /*09*/  "Glockenspiel",
  /*0a*/  "Music Box",
  /*0b*/  "Vibraphone",
  /*0c*/  "Marimba",
  /*0d*/  "Xylophone",
  /*0e*/  "Tubular Bells",
  /*0f*/  "Dulcimer",
  /*10*/  "Hammond Organ",
  /*11*/  "Percussive Organ",
  /*12*/  "Rock Organ",
  /*13*/  "Church Organ",
  /*14*/  "Reed Organ",
  /*15*/  "Accordion",
  /*16*/  "Harmonica",
  /*17*/  "Tango Accordion",
  /*18*/  "Acoustic Guitar (Nylon)",
  /*19*/  "Acoustic Guitar (Steel)",
  /*1a*/  "Electric Guitar (Jazz)",
  /*1b*/  "Electric Guitar (Clean)",
  /*1c*/  "Electric Guitar (Muted)",
  /*1d*/  "Overdriven Guitar",
  /*1e*/  "Distortion Guitar",
  /*1f*/  "Guitar Harmonics",
  /*20*/  "Acoustic Bass",
  /*21*/  "Electric Bass (Finger)",
  /*22*/  "Electric Bass (Pick)",
  /*23*/  "Fretless Bass",
  /*24*/  "Slap Bass 1",
  /*25*/  "Slap Bass 2",
  /*26*/  "Synth Bass 1",
  /*27*/  "Synth Bass 2",
  /*28*/  "Violin",
  /*29*/  "Viola",
  /*2a*/  "Cello",
  /*2b*/  "Contrabass",
  /*2c*/  "Tremolo Strings",
  /*2d*/  "Pizzicato Strings",
  /*2e*/  "Orchestral Harp",
  /*2f*/  "Timpani",
  /*30*/  "String Ensemble 1",
  /*31*/  "String Ensemble 2",
  /*32*/  "Synth Strings 1",
  /*33*/  "Synth Strings 2",
  /*34*/  "Choir Aahs",
  /*35*/  "Voice Oohs",
  /*36*/  "Synth Voice",
  /*37*/  "Orchestra Hit",
  /*38*/  "Trumpet",
  /*39*/  "Trombone",
  /*3a*/  "Tuba",
  /*3b*/  "Muted Trumpet",
  /*3c*/  "French Horn",
  /*3d*/  "Brass Section",
  /*3e*/  "Synth Brass 1",
  /*3f*/  "Synth Brass 2",
  /*40*/  "Soprano Sax",
  /*41*/  "Alto Sax",
  /*42*/  "Tenor Sax",
  /*43*/  "Baritone Sax",
  /*44*/  "Oboe",
  /*45*/  "English Horn",
  /*46*/  "Bassoon",
  /*47*/  "Clarinet",
  /*48*/  "Piccolo",
  /*49*/  "Flute",
  /*4a*/  "Recorder",
  /*4b*/  "Pan Flute",
  /*4c*/  "Blown Bottle",
  /*4d*/  "Shakuhachi",
  /*4e*/  "Whistle",
  /*4f*/  "Ocarina",
  /*50*/  "Lead 1 - Square Wave",
  /*51*/  "Lead 2 - Saw Tooth",
  /*52*/  "Lead 3 - Calliope",
  /*53*/  "Lead 4 - Chiflead",
  /*54*/  "Lead 5 - Charang",
  /*55*/  "Lead 6 - Voice",
  /*56*/  "Lead 7 - Fifths",
  /*57*/  "Lead 8 - Bass+Lead",
  /*58*/  "Pad 1 - New Age",
  /*59*/  "Pad 2 - Warm",
  /*5a*/  "Pad 3 - Polysynth",
  /*5b*/  "Pad 4 - Choir",
  /*5c*/  "Pad 5 - Bow",
  /*5d*/  "Pad 6 - Metallic",
  /*5e*/  "Pad 7 - Halo",
  /*5f*/  "Pad 8 - Sweep",
  /*60*/  "FX 1 - Rain",
  /*61*/  "FX 2 - Soundtrack",
  /*62*/  "FX 3 - Crystal",
  /*63*/  "FX 4 - Atmosphere",
  /*64*/  "FX 5 - Brightness",
  /*65*/  "FX 6 - Goblins",
  /*66*/  "FX 7 - Echoes",
  /*67*/  "FX 8 - Sci-fi",
  /*68*/  "Sitar",
  /*69*/  "Banjo",
  /*6a*/  "Shamisen",
  /*6b*/  "Koto",
  /*6c*/  "Kalimba",
  /*6d*/  "Bagpipe",
  /*6e*/  "Fiddle",
  /*6f*/  "Shannai",
  /*70*/  "Tinkle Bell",
  /*71*/  "Agogo",
  /*72*/  "Steel Drum",
  /*73*/  "Wook Block",
  /*74*/  "Taiko Drum",
  /*75*/  "Melodic Tom",
  /*76*/  "Synth Drum",
  /*77*/  "Reverse Cymbal",
  /*78*/  "Guitar Fret Noise",
  /*79*/  "Breath Noise",
  /*7a*/  "Seashore",
  /*7b*/  "Bird Tweet",
  /*7c*/  "Telephone",
  /*7d*/  "Helicopter",
  /*7e*/  "Applause",
  /*7f*/  "Gunshot"
};


#define MIDI_PERCUSSIONS 9
/* This is the MIDI channel used for percussion instruments */

/* The GM Percussion map is downwards compatible to the MT32 map, which is used in SCI */
static char *GM_Percussion_Names[] = {
  /*00*/  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  /*10*/  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  /*20*/  0, 0, 0, /* The preceeding percussions are not covered by the GM standard */
  /*23*/  "Bass Drum",
  /*24*/  "Bass Drum 1",
  /*25*/  "Side Stick",
  /*26*/  "Acoustic Snare",
  /*27*/  "Hand Clap",
  /*28*/  "Electric Snare",
  /*29*/  "Low Floor Tom",
  /*2a*/  "Closed Hi-Hat",
  /*2b*/  "High Floor Tom",
  /*2c*/  "Pedal Hi-Hat",
  /*2d*/  "Low Tom",
  /*2e*/  "Open Hi-Hat",
  /*2f*/  "Low-Mid Tom",
  /*30*/  "Hi-Mid Tom",
  /*31*/  "Crash Cymbal 1",
  /*32*/  "High Tom",
  /*33*/  "Ride Cymbal 1",
  /*34*/  "Chinese Cymbal",
  /*35*/  "Ride Bell",
  /*36*/  "Tambourine",
  /*37*/  "Splash Cymbal",
  /*38*/  "Cowbell",
  /*39*/  "Crash Cymbal 2",
  /*3a*/  "Vibraslap",
  /*3b*/  "Ride Cymbal 2",
  /*3c*/  "Hi Bongo",
  /*3d*/  "Low Bongo",
  /*3e*/  "Mute Hi Conga",
  /*3f*/  "Open Hi Conga",
  /*40*/  "Low Conga",
  /*41*/  "High Timbale"
  /*42*/  "Low Timbale"
  /*43*/  "High Agogo",
  /*44*/  "Low Agogo",
  /*45*/  "Cabasa",
  /*46*/  "Maracas",
  /*47*/  "Short Whistle",
  /*48*/  "Long Whistle",
  /*49*/  "Short Guiro",
  /*4a*/  "Long Guiro",
  /*4b*/  "Claves",
  /*4c*/  "Hi Wood Block",
  /*4d*/  "Low Wood Block",
  /*4e*/  "Mute Cuica",
  /*4f*/  "Open Cuica",
  /*50*/  "Mute Triangle",
  /*51*/  "Open Triangle"
};

#define INSTR_NOT_FOUND -1
#define INSTR_NOT_MAPPED -2

static struct {
char *name;
int gm_instr;
} SCI0_patch2_instrmaps[] = {
{"       ", INSTR_NOT_MAPPED},
{"SnareDr", INSTR_NOT_MAPPED}, /* percussion */
{"HiHat  ", INSTR_NOT_MAPPED}, /* percussion */
{"Kick D.", INSTR_NOT_MAPPED}, /* percussion */
{"Tom Tom", INSTR_NOT_MAPPED}, /* percussion */
{"Trump2 ", 0x38},
{"T-Bone2", 0x39},
{"FHorn  ", 0x3c},
{"StrSct1", 0x2c}, /* ? */
{"Timpani", 0x2f},
{"CBass  ", 0x2b},
{"Glock  ", 0x09},
{"Flute  ", 0x49},
{"Tuba   ", 0x3a},
{"BrsSect", 0x3d},
{"Pizz   ", 0x2d}, /* ? */
{"AcouBas", 0x20},
{"Sax 2  ", 0x42}, /* ? */
{"EnglHrn", 0x45},
{"PipeOrg", 0x13}, /* ? */
{"PanPipe", 0x4b},
{"Clarint", 0x47},
{"BassPiz", 0x22}, /* ?? */
{"Conga  ", INSTR_NOT_MAPPED},
{"Sitar  ", 0x68},
{"Fantasy", 0x63},
{"Guitar ", 0x18}, /* ? */
{"Choral ", 0x34}, /* ? */
{"SynBas1", 0x26},
{"GameSnd", 0x50}, /* ? */
{"Calliop", 0x52},
{"Claw   ", INSTR_NOT_MAPPED},
{"Flames ", INSTR_NOT_MAPPED},
{"Swords ", INSTR_NOT_MAPPED},
{"Armor  ", INSTR_NOT_MAPPED},
{"Splat  ", INSTR_NOT_MAPPED},
{"Fall   ", INSTR_NOT_MAPPED},
{"Crash  ", INSTR_NOT_MAPPED},
{"Laser  ", 0x67}, /* ??? */
{"Vase   ", INSTR_NOT_MAPPED},
{"Spit   ", INSTR_NOT_MAPPED},
{"Ninga  ", INSTR_NOT_MAPPED},
{"Flame2 ", INSTR_NOT_MAPPED},
{"Crackle", INSTR_NOT_MAPPED},
{"Slurp  ", INSTR_NOT_MAPPED},
{"Knifstk", INSTR_NOT_MAPPED},
{"Pft    ", INSTR_NOT_MAPPED},
{"Poof   ", INSTR_NOT_MAPPED},
{"Firdart", INSTR_NOT_MAPPED},
{"Lghtblt", INSTR_NOT_MAPPED},
{"Rasp   ", INSTR_NOT_MAPPED},
{"Explode", INSTR_NOT_MAPPED},
{"Lock   ", INSTR_NOT_MAPPED},
{"Buildup", INSTR_NOT_MAPPED},
{"Boing  ", INSTR_NOT_MAPPED},
{"Flame2 ", INSTR_NOT_MAPPED},
{"Thunder", INSTR_NOT_MAPPED},
{"Meeps  ", INSTR_NOT_MAPPED},
{"RimShot", INSTR_NOT_MAPPED},
{"CSTLGat", 0x72}, /* ??? */
{"Agogo  ", 0x71},
{"SchoolD", INSTR_NOT_MAPPED},
{"Whistle", 0x4e},
{"SnBrass", 0x3d}, /* PQ2 */
{"SnBass4", 0x27},
{"SndTrk ", INSTR_NOT_MAPPED},
{"WarmBel", 0x63},
{"EchoPan", 0x57}, /* ? */
{"DrSolo ", INSTR_NOT_MAPPED},
{"SQ Bass", 0x25}, /* ??? */
{"EGuit 2", 0x1b}, /* ? */
{"IceRain", INSTR_NOT_MAPPED},
{"Cello 2", 0x2a},
{"OrchHit", 0x37},
{"English", 0x45},
{"TBone 2", 0x39},
{"Chorale", 0x5b}, /* ? */
{"Koto   ", 0x6b},
{"FunnyVx", INSTR_NOT_MAPPED},
{"ElOboe ", 0x44}, /* ? */
{"Taiko  ", 0x74},
{"SlapBas", 0x24},
{"Shakuha", 0x4d},
{"HonkyT ", 0x03},
{"Marimba", 0x0c},
{"EOrgan2", 0x12}, /* ? */
{"E Bass1", 0x21},
{"Bassoon", 0x46},
{"Sho    ", INSTR_NOT_MAPPED},
{"EchoBel", INSTR_NOT_MAPPED},
{"ElPerc2", INSTR_NOT_MAPPED},
{"SquareW", 0x50},
{"Fretles", 0x23}, /* ?? */
{"NoteJam", INSTR_NOT_MAPPED},
{"ElPerc1", INSTR_NOT_MAPPED},
{"Trumpt1", 0x38},
{"DeepSnr", INSTR_NOT_MAPPED},
{"Xylophn", 0x0d},
{"HarmoPn", 0x16}, /* ?? */
{"EdoorLk", INSTR_NOT_MAPPED},
{"AirLock", INSTR_NOT_MAPPED},
{"Clang  ", INSTR_NOT_MAPPED},
{"Blades ", INSTR_NOT_MAPPED},
{"TrshGun", INSTR_NOT_MAPPED},
{"Snake  ", INSTR_NOT_MAPPED},
{"Jello  ", INSTR_NOT_MAPPED},
{"Liftoff", INSTR_NOT_MAPPED},
{"Guard  ", INSTR_NOT_MAPPED},
{"TrkDoor", INSTR_NOT_MAPPED},
{"Scanner", INSTR_NOT_MAPPED},
{"Machine", INSTR_NOT_MAPPED},
{"Burp   ", INSTR_NOT_MAPPED},
{"Motor  ", INSTR_NOT_MAPPED},
{"Flift  ", INSTR_NOT_MAPPED},
{"Takeoff", INSTR_NOT_MAPPED},
{"EQuake ", INSTR_NOT_MAPPED},
{"ContraB", 0x2b},
{"Telepho", 0x7c},
{"RD Cymb", INSTR_NOT_MAPPED},
{"Hollow ", 0x66}, /* ? */
{"Bells  ", 0x70}, /* ? */
{"Piano  ", 0x00},
{"POrgan1", 0x11}, /* ? */
{"SynClv1", 0x07}, /* ? */
{"Honkey2", 0x03},
{"NewEP3 ", 0x58}, /* ?? */
{"POrgan2", 0x11}, /* ? */
{"COrgan2", 0x13}, /* ? */
{"NewEP  ", 0x03},
{"SynLead", 0x51}, /* ?? */
{"Clarine", 0x47},
{"Synvoic", 0x36},
{"RichSt3", 0x30},
{"RECRDR1", 0x4a},
{"Oboe   ", 0x44},
{"Bass 2 ", 0x20},
{"SoloVio", 0x29},
{"Cello1 ", 0x2a},
{"Strings", 0x31}, /* ? */
{"Fuzz Gt", 0x78}, /* ? */
{"Banjo  ", 0x69},
{"Stadium", INSTR_NOT_MAPPED},
{"FunkBas", INSTR_NOT_MAPPED},
{"Bass 2 ", 0x20},
{"Trumpet", 0x38},
{"Trombon", 0x39},
{"FrenchH", 0x3c},
{"Trumpt2", 0x38},
{"Flugelh", 0x00},
{"Glasses", 0x0e}, /* ?? */

{"Glocken", 0x70},
{"Vibes1 ", 0x0b},
{"WaterB ", 0x62}, /* ? */
{"TubeBel", 0x0e},
{"EPno 1n", 0x04},
{"Fant MS", 0x58}, /* ? */
{"BottleB", 0x4c},
{"Gunshot", 0x7f},
{"StrSec3", 0x30},
{"OBXaBrt", 0x3e},
{"OBXa MS", 0x3f},
{"Sbrass3", 0x3d},
{"UprtBas", 0x20},
{"StrSec1", 0x30},
{"StrSec2", 0x31},
{"Guitar2", 0x19},
{"Busy   ", 0x50}, /* ?? */
{"WoodBlk", INSTR_NOT_MAPPED}, /* percussion */
{"JungleT", 0x54},
{0,0}
};



const char MIDI_Header[4] = "MThd";
/* Starts every MIDI header */

const char MIDI_Header_data[10] = {0, 0, 0, 6, /* Length of block */
				   0, 1, /* file format */
				   0, 1, /* number of tracks */
				   0, 30}; /* beats per minute (educated guess) */
/* This contains the information (and block length) found in
** the MIDI header.
*/
				   
const char MIDI_Track[4] = "MTrk";
/* Starts every MIDI track */


const char MIDI_NOP[4] = {0xff, 0x01, 0x01, 0x00};
/* text command with length 1 */


#define MAP_INSTRUMENT(channel, sci_instrument)  /* Maps sci_instrument to channel if appropriate */ \
if ((channel == MIDI_PERCUSSIONS) || (MIDI_map[(sci_instrument) / 48][(sci_instrument) % 48] >= 0)) { \
    obstack_1grow(stackp,0xc0 | channel); /* set instrument 'pos' to... */     \
    obstack_1grow(stackp, MIDI_map[(sci_instrument) / 48 ][(sci_instrument % 48)]); /* this. */   \
    muteflags[channel] = 0; \
} else { \
  muteflags[channel] = 1; /* Mute instrument if if isn't mapped */ \
  obstack_grow(stackp, &MIDI_NOP, sizeof(MIDI_NOP)); /* Do nothing */ \
}



guint8 *
makeMIDI0(const guint8 *src, int *size)
/* Generates a MIDI memory block (looking like a MIDI file).
** size is set to the total size of the memory block.
** Returns 0 on failure.
*/
{
  struct obstack stack; /* This is where we store the temp results */
  struct obstack *stackp = &stack;
  char muteflags[16]; /* Remembers unmapped instruments */
  guint32 pos; /* position in src */
  int EOT = 0; /* end of track reached */
  guint8 *result; /* Herein will be the final results */
  int tracklength_index; /* (guint32) result[tracklength_index] (big endian)
			 ** contains the total track length
			 */

  if (!obstack_init(stackp)) return 0;

  obstack_grow(stackp, &MIDI_Header, sizeof(MIDI_Header));
  obstack_grow(stackp, &MIDI_Header_data, sizeof(MIDI_Header_data));
  obstack_grow(stackp, &MIDI_Track, sizeof(MIDI_Track));

  tracklength_index = obstack_object_size(stackp);
  obstack_grow(stackp, &MIDI_Track, 4);
  /* These arbitraty bytes will be overwritten as soon as we have the total
  ** track length
  */

  for (pos = 0; pos < 0x10; pos++) {
    obstack_1grow(stackp, 0); /* don't wait */
    MAP_INSTRUMENT(pos, src[(pos << 1) + 1]);
  }

  pos = 0x21; /* skip the header */

  while (!EOT) {
    int command; /* MIDI command */
    int commandlength; /* length of the command block */
    int alt = 0; /* don't copy */

    /* MIDI delay */
    if (src[pos] > 0x7f) {
      /* Sierra uses the all bits to store length information, but the MIDI
      ** standard allows only 7 bits of information per byte (since
      ** the MSB is used to signal that the next byte contains additional
      ** delay information).
      */
	obstack_1grow(stackp, 0x81);
	obstack_1grow(stackp, src[pos] & 0x7f);
    } else obstack_1grow(stackp, src[pos]);
    pos++;

    /* MIDI commands */
    if (src[pos] & 0x80) {
      commandlength = 1;
      command = src[pos];
    } else commandlength = 0;
    /* else we've got the 'running status' mode */

    if (command < 0x90) commandlength += 2; /* Note Off */
    else if (command < 0xa0) commandlength += 2; /* Note On */
    else if (command < 0xb0) commandlength += 2; /* Polyphonic Key Pressure */
    else if (command < 0xc0) commandlength += 2; /* Controller Change */
    else if (command < 0xd0) { /* Program Change: Set instr. mapping */
      commandlength += 1;
      alt += 1; /* copying is handled here */
      MAP_INSTRUMENT(command & 0xf, src[pos+1]);
    } else if (command < 0xe0) commandlength += 1; /* Channel Key Pressure */
    else if (command < 0xf0) commandlength += 2; /* pitch bend */
    else if (command == 0xfc) { /* End of Track (is this official?) */
      alt = 1;
      obstack_1grow(stackp, 0xff);
      obstack_1grow(stackp, 0x2f);
      obstack_1grow(stackp, 0x00);
      EOT = 1;
    } else if (command == 0xff) { /* Meta command */
      switch (src[pos+1]) {
      case 0x2f: EOT = 1; /* End of Track */
	commandlength += 2; /* FIXME: This assumes that src[pos+2] == 0 */
	break;
      default: SCIswarn("Illegal or unsupported MIDI meta instruction: FF %02x\n",
			src[pos+1]);
      return NULL;
      }
    } else {
      SCIswarn("Illegal or unsupported MIDI extended instruction: %02x\n", command);
      return NULL;
    }

    if ((command < 0xf0) && (muteflags[command & 0x0f])) {
      alt = 1; /* If a channel command was sent to a muted channel, NOP instead */
      if ((command & 0xf0) != 0xc0)
	obstack_grow(stackp, &MIDI_NOP, sizeof(MIDI_NOP)); 
    }

    if (!alt)
      obstack_grow(stackp, src + pos, commandlength);

    pos += commandlength;
  }

  *size = obstack_object_size(stackp);

  SCIsdebug("MIDI: EOT reached at %i, resulting MIDI block size is %i\n",
	    pos, *size);


  result = XALLOC(*size);
  memcpy(result, obstack_finish(stackp), *size);
  obstack_free(stackp, NULL);

  pos = *size - tracklength_index - 4; /* length of the MTrk block */
#ifndef WORDS_BIGENDIAN
  pos = GUINT32_SWAP_LE_BE_CONSTANT(pos);
#endif /* !WORDS_BIGENDIAN */
  *((guint32 *)(result + tracklength_index)) = pos;

  return result;
}

#undef MAP_INSTRUMENT(channel, sci_instrument)


int
_lookup_instrument(char *iname)
{
  int i = 0;

  while (SCI0_patch2_instrmaps[i].name) {
    if (g_strncasecmp(iname, SCI0_patch2_instrmaps[i].name, 7) == 0)
      return SCI0_patch2_instrmaps[i].gm_instr;
    i++;
  }
  return INSTR_NOT_FOUND;
}


int
mapMIDIInstruments(void)
{
  resource_t *patch2 = findResource(sci_patch, 2);
  int banks;
  int i, instrctr, bankctr;
  FILE *logfile;

  for (i = 0; i < MAX_MIDI_MAPS; i++)
    memcpy(MIDI_map[i], MIDI_map_default, sizeof(MIDI_map_default));

  if (sci_version > SCI_VERSION_1)
    return 0;

  if (!patch2) {
    if (sci_version < SCI_VERSION_1) {
      fprintf(stderr,"No patch.002 found: MIDI mapping magic failed!\n");
      return 1;
    }
    return 0;
  }

  if (!(logfile = fopen("freesci_instr", "a")))
    perror("While trying to open freesci_instr");

  banks = patch2->length / (0x40 * 48);

  SCIsdebug("MIDI mapping magic: %d instrument banks detected\n", banks);

  for (bankctr = 0; bankctr < banks; bankctr++)
    for (instrctr = 0; instrctr < 48; instrctr++)
      {
	char *instrname = patch2->data
	  + (bankctr * ((0x40 * 48) + 2))
	  + (instrctr * 0x40);
	int instr = _lookup_instrument(instrname);

	if (instr >= 0) {

	  MIDI_map[bankctr][instrctr] = instr;

	} else {
	  instrname[7] = 0; /* Evil */
	  MIDI_map[bankctr][instrctr] = INSTR_NOT_MAPPED;
	  SCIsdebug("MIDI Bank %d: Instrument #%d (\"%s\"): ",
		    bankctr, instrctr, instrname);

	  if (instr == INSTR_NOT_FOUND) {
	    SCIsdebug("Unknown instrument\n");

	    if (logfile)
	      fprintf(logfile,"{\"%s\", INSTR_NOT_MAPPED},\n", instrname);

	  } else if (instr == INSTR_NOT_MAPPED) {
	    SCIsdebug("Not mapped\n");
	  } else SCIsdebug("Invalid condition %d\n", instr);
	}
      }

  if (logfile)
    fclose(logfile);
  return 0;
}
