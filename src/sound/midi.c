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

 Roland MT-32 to General MIDI conversion:

    Rickard Lind [rpl@dd.chalmers.se]

***************************************************************************/
/* Turns a resource into a valid midi block */

#include <sound.h>
#include <engine.h>
#include <stdarg.h>

static struct {
gint8 gm_instr;
int keyshift;
int finetune;
int bender_range;
} MIDI_patchmap[128];
static gint8 MIDI_rhythmkey[128];

static char *GM_Instrument_Names[] = {

 /*000*/  "Acoustic Grand Piano",
 /*001*/  "Bright Acoustic Piano",
 /*002*/  "Electric Grand Piano",
 /*003*/  "Honky-tonk Piano",
 /*004*/  "Electric Piano 1",
 /*005*/  "Electric Piano 2",
 /*006*/  "Harpsichord",
 /*007*/  "Clavinet",
 /*008*/  "Celesta",
 /*009*/  "Glockenspiel",
 /*010*/  "Music Box",
 /*011*/  "Vibraphone",
 /*012*/  "Marimba",
 /*013*/  "Xylophone",
 /*014*/  "Tubular Bells",
 /*015*/  "Dulcimer",
 /*016*/  "Drawbar Organ",
 /*017*/  "Percussive Organ",
 /*018*/  "Rock Organ",
 /*019*/  "Church Organ",
 /*020*/  "Reed Organ",
 /*021*/  "Accordion",
 /*022*/  "Harmonica",
 /*023*/  "Tango Accordion",
 /*024*/  "Acoustic Guitar (nylon)",
 /*025*/  "Acoustic Guitar (steel)",
 /*026*/  "Electric Guitar (jazz)",
 /*027*/  "Electric Guitar (clean)",
 /*028*/  "Electric Guitar (muted)",
 /*029*/  "Overdriven Guitar",
 /*030*/  "Distortion Guitar",
 /*031*/  "Guitar Harmonics",
 /*032*/  "Acoustic Bass",
 /*033*/  "Electric Bass (finger)",
 /*034*/  "Electric Bass (pick)",
 /*035*/  "Fretless Bass",
 /*036*/  "Slap Bass 1",
 /*037*/  "Slap Bass 2",
 /*038*/  "Synth Bass 1",
 /*039*/  "Synth Bass 2",
 /*040*/  "Violin",
 /*041*/  "Viola",
 /*042*/  "Cello",
 /*043*/  "Contrabass",
 /*044*/  "Tremolo Strings",
 /*045*/  "Pizzicato Strings",
 /*046*/  "Orchestral Harp",
 /*047*/  "Timpani",
 /*048*/  "String Ensemble 1",
 /*049*/  "String Ensemble 2",
 /*050*/  "SynthStrings 1",
 /*051*/  "SynthStrings 2",
 /*052*/  "Choir Aahs",
 /*053*/  "Voice Oohs",
 /*054*/  "Synth Voice",
 /*055*/  "Orchestra Hit",
 /*056*/  "Trumpet",
 /*057*/  "Trombone",
 /*058*/  "Tuba",
 /*059*/  "Muted Trumpet",
 /*060*/  "French Horn",
 /*061*/  "Brass Section",
 /*062*/  "SynthBrass 1",
 /*063*/  "SynthBrass 2",
 /*064*/  "Soprano Sax",
 /*065*/  "Alto Sax",
 /*066*/  "Tenor Sax",
 /*067*/  "Baritone Sax",
 /*068*/  "Oboe",
 /*069*/  "English Horn",
 /*070*/  "Bassoon",
 /*071*/  "Clarinet",
 /*072*/  "Piccolo",
 /*073*/  "Flute",
 /*074*/  "Recorder",
 /*075*/  "Pan Flute",
 /*076*/  "Blown Bottle",
 /*077*/  "Shakuhachi",
 /*078*/  "Whistle",
 /*079*/  "Ocarina",
 /*080*/  "Lead 1 (square)",
 /*081*/  "Lead 2 (sawtooth)",
 /*082*/  "Lead 3 (calliope)",
 /*083*/  "Lead 4 (chiff)",
 /*084*/  "Lead 5 (charang)",
 /*085*/  "Lead 6 (voice)",
 /*086*/  "Lead 7 (fifths)",
 /*087*/  "Lead 8 (bass+lead)",
 /*088*/  "Pad 1 (new age)",
 /*089*/  "Pad 2 (warm)",
 /*090*/  "Pad 3 (polysynth)",
 /*091*/  "Pad 4 (choir)",
 /*092*/  "Pad 5 (bowed)",
 /*093*/  "Pad 6 (metallic)",
 /*094*/  "Pad 7 (halo)",
 /*095*/  "Pad 8 (sweep)",
 /*096*/  "FX 1 (rain)",
 /*097*/  "FX 2 (soundtrack)",
 /*098*/  "FX 3 (crystal)",
 /*099*/  "FX 4 (atmosphere)",
 /*100*/  "FX 5 (brightness)",
 /*101*/  "FX 6 (goblins)",
 /*102*/  "FX 7 (echoes)",
 /*103*/  "FX 8 (sci-fi)",
 /*104*/  "Sitar",
 /*105*/  "Banjo",
 /*106*/  "Shamisen",
 /*107*/  "Koto",
 /*108*/  "Kalimba",
 /*109*/  "Bag pipe",
 /*110*/  "Fiddle",
 /*111*/  "Shannai",
 /*112*/  "Tinkle Bell",
 /*113*/  "Agogo",
 /*114*/  "Steel Drums",
 /*115*/  "Woodblock",
 /*116*/  "Taiko Drum",
 /*117*/  "Melodic Tom",
 /*118*/  "Synth Drum",
 /*119*/  "Reverse Cymbal",
 /*120*/  "Guitar Fret Noise",
 /*121*/  "Breath Noise",
 /*122*/  "Seashore",
 /*123*/  "Bird Tweet",
 /*124*/  "Telephone Ring",
 /*125*/  "Helicopter",
 /*126*/  "Applause",
 /*127*/  "Gunshot"
};

#define MIDI_PERCUSSIONS 9
/* This is the MIDI channel used for percussion instruments */

/* The GM Percussion map is downwards compatible to the MT32 map, which is used in SCI */
static char *GM_Percussion_Names[] = {
  /*00*/  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
  /*10*/  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  /*20*/  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  /*30*/  0, 0, 0, 0,
/* The preceeding percussions are not covered by the GM standard */
  /*34*/  "Acoustic Bass Drum",
  /*35*/  "Bass Drum 1",
  /*36*/  "Side Stick",
  /*37*/  "Acoustic Snare",
  /*38*/  "Hand Clap",
  /*39*/  "Electric Snare",
  /*40*/  "Low Floor Tom",
  /*41*/  "Closed Hi-Hat",
  /*42*/  "High Floor Tom",
  /*43*/  "Pedal Hi-Hat",
  /*44*/  "Low Tom",
  /*45*/  "Open Hi-Hat",
  /*46*/  "Low-Mid Tom",
  /*47*/  "Hi-Mid Tom",
  /*48*/  "Crash Cymbal 1",
  /*49*/  "High Tom",
  /*50*/  "Ride Cymbal 1",
  /*51*/  "Chinese Cymbal",
  /*52*/  "Ride Bell",
  /*53*/  "Tambourine",
  /*54*/  "Splash Cymbal",
  /*55*/  "Cowbell",
  /*56*/  "Crash Cymbal 2",
  /*57*/  "Vibraslap",
  /*58*/  "Ride Cymbal 2",
  /*59*/  "Hi Bongo",
  /*60*/  "Low Bongo",
  /*61*/  "Mute Hi Conga",
  /*62*/  "Open Hi Conga",
  /*63*/  "Low Conga",
  /*64*/  "High Timbale",
  /*65*/  "Low Timbale",
  /*66*/  "High Agogo",
  /*67*/  "Low Agogo",
  /*68*/  "Cabasa",
  /*69*/  "Maracas",
  /*70*/  "Short Whistle",
  /*71*/  "Long Whistle",
  /*72*/  "Short Guiro",
  /*73*/  "Long Guiro",
  /*74*/  "Claves",
  /*75*/  "Hi Wood Block",
  /*76*/  "Low Wood Block",
  /*77*/  "Mute Cuica",
  /*78*/  "Open Cuica",
  /*79*/  "Mute Triangle",
  /*80*/  "Open Triangle"
};

/*******************************************
 * Fancy instrument mappings begin here... *
 *******************************************/

#define MAP_NOT_FOUND -1
#define NOMAP -2

static struct {
char *name;
gint8 gm_instr;
} MT32_PresetTimbreMaps[] = {
/*000*/  {"Acou Piano 1", 0},
/*001*/  {"Acou Piano 2", 1},
/*002*/  {"Acou Piano 3", 2},
/*003*/  {"Elec Piano 1", 4},
/*004*/  {"Elec Piano 2", 5},
/*005*/  {"Elec Piano 3", 4},
/*006*/  {"Elec Piano 4", 5},
/*007*/  {"Honkytonk", 3},
/*008*/  {"Elec Org 1", 16},
/*009*/  {"Elec Org 2", 17},
/*010*/  {"Elec Org 3", 18},
/*011*/  {"Elec Org 4", 18},
/*012*/  {"Pipe Org 1", 19},
/*013*/  {"Pipe Org 2", 19},
/*014*/  {"Pipe Org 3", 20},
/*015*/  {"Accordion", 21},
/*016*/  {"Harpsi 1", 6},
/*017*/  {"Harpsi 2", 6},
/*018*/  {"Harpsi 3", 6},
/*019*/  {"Clavi 1", 7},
/*020*/  {"Clavi 2", 7},
/*021*/  {"Clavi 3", 7},
/*022*/  {"Celesta 1", 8},
/*023*/  {"Celesta 2", 8},
/*024*/  {"Syn Brass 1", 62},
/*025*/  {"Syn Brass 2", 63},
/*026*/  {"Syn Brass 3", 62},
/*027*/  {"Syn Brass 4", 63},
/*028*/  {"Syn Bass 1", 38},
/*029*/  {"Syn Bass 2", 39},
/*030*/  {"Syn Bass 3", 38},
/*031*/  {"Syn Bass 4", 39},
/*032*/  {"Fantasy", 88},
/*033*/  {"Harmo Pan", 89},
/*034*/  {"Chorale", 52},
/*035*/  {"Glasses", 98},
/*036*/  {"Soundtrack", 97},
/*037*/  {"Atmosphere", 96},
/*038*/  {"Warm Bell", 91},
/*039*/  {"Funny Vox", 85},
/*040*/  {"Echo Bell", 39},
/*041*/  {"Ice Rain", 101},
/*042*/  {"Oboe 2001", 68},
/*043*/  {"Echo Pan", 95},
/*044*/  {"Doctor Solo", 86},
/*045*/  {"Schooldaze", 103},
/*046*/  {"Bellsinger", 88},
/*047*/  {"Square Wave", 80},
/*048*/  {"Str Sect 1", 48},
/*049*/  {"Str Sect 2", 49},
/*050*/  {"Str Sect 3", 50},
/*051*/  {"Pizzicato", 45},
/*052*/  {"Violin 1", 40},
/*053*/  {"Violin 2", 40},
/*054*/  {"Cello 1", 42},
/*055*/  {"Cello 2", 42},
/*056*/  {"Contrabass", 43},
/*057*/  {"Harp 1", 46},
/*058*/  {"Harp 2", 46},
/*059*/  {"Guitar 1", 24},
/*060*/  {"Guitar 2", 25},
/*061*/  {"Elec Gtr 1", 26},
/*062*/  {"Elec Gtr 2", 27},
/*063*/  {"Sitar", 104},
/*064*/  {"Acou Bass 1", 32},
/*065*/  {"Acou Bass 2", 33},
/*066*/  {"Elec Bass 1", 34},
/*067*/  {"Elec Bass 2", 39},
/*068*/  {"Slap Bass 1", 36},
/*069*/  {"Slap Bass 2", 37},
/*070*/  {"Fretless 1", 35},
/*071*/  {"Fretless 2", 35},
/*072*/  {"Flute 1", 73},
/*073*/  {"Flute 2", 73},
/*074*/  {"Piccolo 1", 72},
/*075*/  {"Piccolo 2", 72},
/*076*/  {"Recorder", 74},
/*077*/  {"Pan Pipes", 75},
/*078*/  {"Sax 1", 64},
/*079*/  {"Sax 2", 65},
/*080*/  {"Sax 3", 66},
/*081*/  {"Sax 4", 67},
/*082*/  {"Clarinet 1", 71},
/*083*/  {"Clarinet 2", 71},
/*084*/  {"Oboe", 68},
/*085*/  {"Engl Horn", 69},
/*086*/  {"Bassoon", 70},
/*087*/  {"Harmonica", 22},
/*088*/  {"Trumpet 1", 56},
/*089*/  {"Trumpet 2", 56}, /* QfG1 with 59 sound not as good as 56 */
/*090*/  {"Trombone 1", 57},
/*091*/  {"Trombone 2", 57},
/*092*/  {"Fr Horn 1", 60},
/*093*/  {"Fr Horn 2", 60},
/*094*/  {"Tuba", 58},
/*095*/  {"Brs Sect 1", 61},
/*096*/  {"Brs Sect 2", 61},
/*097*/  {"Vibe 1", 11},
/*098*/  {"Vibe 2", 11},
/*099*/  {"Syn Mallet", 12},
/*100*/  {"Windbell", 88},
/*101*/  {"Glock", 9},
/*102*/  {"Tube Bell", 14},
/*103*/  {"Xylophone", 13},
/*104*/  {"Marimba", 12},
/*105*/  {"Koto", 107},
/*106*/  {"Sho", 111},
/*107*/  {"Shakuhachi", 77},
/*108*/  {"Whistle 1", 78},
/*109*/  {"Whistle 2", 78},
/*110*/  {"Bottleblow", 76},
/*111*/  {"Breathpipe", 121},
/*112*/  {"Timpani", 47},
/*113*/  {"Melodic Tom", 117},
/*114*/  {"Deep Snare", 127},
/*115*/  {"Elec Perc 1", 115},
/*116*/  {"Elec Perc 2", 118},
/*117*/  {"Taiko", 116},
/*118*/  {"Taiko   Rim", 118},
/*119*/  {"Cymbal", 126},
/*120*/  {"Castanets", 121},
/*121*/  {"Triangle", 112},
/*122*/  {"Orche Hit", 55},
/*123*/  {"Telephone", 124},
/*124*/  {"Bird Tweet", 123},
/*125*/  {"One Note Jam", 125},
/*126*/  {"Water Bells", 98},
/*127*/  {"Jungle Tune", 127}
};

static struct {
char *name;
gint8 gm_instr;
gint8 gm_rhythmkey;
} MT32_RhythmTimbreMaps[] = {
/*00*/  {"Acou BD", NOMAP, 34},
/*01*/  {"Acou SD", NOMAP, 37},
/*02*/  {"Acou Hi Tom", NOMAP, 49},
/*03*/  {"Acou Mid Tom", NOMAP, 46},
/*04*/  {"Acou Low Tom", NOMAP, 40},
/*05*/  {"Elec SD", NOMAP, 39},
/*06*/  {"Clsd Hi Hat", NOMAP, 41},
/*07*/  {"Open Hi Hat 1", NOMAP, 45},
/*08*/  {"Crash Cym", NOMAP, 48},
/*09*/  {"Ride Cym", NOMAP, 50},
/*10*/  {"Rim Shot", NOMAP, 36},
/*11*/  {"Hand Clap", NOMAP, 38},
/*12*/  {"Cowbell", NOMAP, 55},
/*13*/  {"Mt High Conga", NOMAP, 61},
/*14*/  {"High Conga", NOMAP, 62},
/*15*/  {"Low Conga", NOMAP, 63},
/*16*/  {"High Timbale", NOMAP, 64},
/*17*/  {"Low Timbale", NOMAP, 65},
/*18*/  {"High Bongo", NOMAP, 59},
/*19*/  {"Low Bongo", NOMAP, 60},
/*20*/  {"High Agogo", 113, 66},
/*21*/  {"Low Agogo", 113, 67},
/*22*/  {"Tambourine", NOMAP, 53},
/*23*/  {"Claves", NOMAP, 74},
/*24*/  {"Maracas", NOMAP, 69},
/*25*/  {"Smba Whis L", 78, 71},
/*26*/  {"Smba Whis S", 78, 70},
/*27*/  {"Cabasa", NOMAP, 68},
/*28*/  {"Quijada", NOMAP, 72},
/*29*/  {"Open Hi Hat 2", NOMAP, 43}
};

static gint8 MT32_PresetRhythmKeymap[] = {
NOMAP, NOMAP, NOMAP, NOMAP, NOMAP, NOMAP, NOMAP, NOMAP, NOMAP, NOMAP,
NOMAP, NOMAP, NOMAP, NOMAP, NOMAP, NOMAP, NOMAP, NOMAP, NOMAP, NOMAP,
NOMAP, NOMAP, NOMAP, NOMAP, NOMAP, NOMAP, NOMAP, NOMAP, NOMAP, NOMAP,
NOMAP, NOMAP, NOMAP, NOMAP, 34, 34, 36, 37, 38, 39,
40, 41, 42, 43, 44, 45, 46, 47, 48, 49,
50, NOMAP, NOMAP, 53, NOMAP, 55, NOMAP, NOMAP, NOMAP, 59,
60, 61, 62, 63, 64, 65, 66, 67, 68, 69,
70, 71, 72, NOMAP, 74, NOMAP, NOMAP, NOMAP, NOMAP, NOMAP,
NOMAP, NOMAP, NOMAP, NOMAP, NOMAP, NOMAP, NOMAP, NOMAP, NOMAP, NOMAP,
NOMAP, NOMAP, NOMAP, NOMAP, NOMAP, NOMAP, NOMAP, NOMAP, NOMAP, NOMAP,
NOMAP, NOMAP, NOMAP, NOMAP, NOMAP, NOMAP, NOMAP, NOMAP, NOMAP, NOMAP,
NOMAP, NOMAP, NOMAP, NOMAP, NOMAP, NOMAP, NOMAP, NOMAP, NOMAP, NOMAP,
NOMAP, NOMAP, NOMAP, NOMAP, NOMAP, NOMAP, NOMAP
};

static struct {
char *name;
gint8 gm_instr;
gint8 gm_rhythm_key;
} MT32_MemoryTimbreMaps[] = {
{"AccPnoKA2 ", 1, NOMAP},
{"Acou BD   ", NOMAP, 34},
{"Acou SD   ", NOMAP, 37},
{"AcouPnoKA ", 0, NOMAP},
{"BASS      ", 32, NOMAP},
{"BASSOONPCM", 70, NOMAP},
{"BEACH WAVE", 122, NOMAP},
{"BagPipes  ", 109, NOMAP},
{"BassPizzMS", 45, NOMAP},
{"BassoonKA ", 70, NOMAP},
{"Bell    MS", 112, NOMAP},
{"Big Bell  ", 14, NOMAP},
{"Bird Tweet", 123, NOMAP},
{"BrsSect MS", 61, NOMAP},
{"CLAPPING  ", 126, NOMAP},
{"Cabasa    ", NOMAP, 68},
{"Calliope  ", 82, NOMAP},
{"CelticHarp", 46, NOMAP},
{"Chicago MS", 90, NOMAP},
{"Chop      ", 117, NOMAP},
{"Chorale MS", 52, NOMAP},
{"ClarinetMS", 71, NOMAP},
{"Claves    ", NOMAP, 74},
{"ClockBell ", 14, NOMAP},
{"CoolPhone ", 124, NOMAP},
{"CrshCymbMS", NOMAP, 48},
{"CstlGateMS", 114, NOMAP},
{"CracklesMS", 120, NOMAP},
{"CymSwellMS", NOMAP, 54},
{"DirtGtr MS", 30, NOMAP},
{"DirtGtr2MS", 29, NOMAP},
{"E Bass  MS", 33, NOMAP},
{"ElecBassMS", 33, NOMAP},
{"ElecGtr MS", 27, NOMAP},
{"EnglHornMS", 69, NOMAP},
{"FantasiaKA", 88, NOMAP},
{"Fantasy   ", 88, NOMAP},
{"Fantasy2MS", 99, NOMAP},
{"Filter  MS", 80, NOMAP}, /* ??? (Codename Iceman) */
{"Filter2 MS", 81, NOMAP}, /* ??? (Codename Iceman) */
{"Flute   MS", 73, NOMAP},
{"FogHorn MS", 58, NOMAP},
{"FrHorn1 MS", 60, NOMAP},
{"FunnyTrmp ", 59, NOMAP},
{"GameSnd MS", 80, NOMAP},
{"Glock   MS", 9, NOMAP},
{"Gunshot   ", 127, NOMAP},
{"Hammer  MS", 114, NOMAP},
{"Harmonica2", 22, NOMAP},
{"Harpsi 1  ", 6, NOMAP},
{"Harpsi 2  ", 6, NOMAP},
{"InHale  MS", 121, NOMAP},
{"KenBanjo  ", 105, NOMAP},
{"Kiss    MS", 25, NOMAP},
{"Koto      ", 107, NOMAP},
{"Laser   MS", 103, NOMAP},
{"MTrak   MS", 97, NOMAP},
{"OCEANSOUND", 122, NOMAP},
{"Oboe 2001 ", 68, NOMAP},
{"Ocean   MS", 122, NOMAP},
{"PianoCrank", NOMAP, NOMAP},
{"PiccoloKA ", 72, NOMAP},
{"PinkBassMS", 39, NOMAP},
{"Pizz2     ", 45, NOMAP},
{"Portcullis", 114, NOMAP},
{"RatSqueek ", 72, NOMAP},
{"Record78  ", NOMAP, NOMAP}, /* Rest in silence (Colonel's Bequest) */
{"RecorderMS", 74, NOMAP},
{"Red Baron ", 125, NOMAP},
{"ReedPipMS ", 20, NOMAP},
{"RevCymb MS", 119, NOMAP},
{"RifleShot ", 127, NOMAP},
{"RimShot MS", NOMAP, 36},
{"SQ Bass MS", 38, NOMAP},
{"SlapBassMS", 36, NOMAP},
{"Some Birds", 123, NOMAP},
{"Sonar   MS", 94, NOMAP},
{"Soundtrk2 ", 97, NOMAP},
{"Soundtrack", 97, NOMAP},
{"SqurWaveMS", 80, NOMAP},
{"StabBassMS", 34, NOMAP},
{"SteelDrmMS", 114, NOMAP},
{"StrSect1MS", 48, NOMAP},
{"String  MS", 46, NOMAP},
{"Syn-Choir ", 52, NOMAP},
{"Syn Brass4", 63, NOMAP},
{"SynBass MS", 38, NOMAP},
{"SwmpBackgr", 96, NOMAP},
{"T-Bone2 MS", 57, NOMAP},
{"Taiko     ", 116, NOMAP},
{"Taiko Rim ", 118, NOMAP},
{"Timpani1  ", 47, NOMAP},
{"Tom     MS", 117, NOMAP},
{"Toms    MS", 117, NOMAP},
{"Tpt1prtl  ", 56, NOMAP},
{"TriangleMS", 112, 80},
{"Trumpet 1 ", 56, NOMAP},
{"WaterBells", 98, NOMAP},
{"WaterFallK", 96, NOMAP},
{"Whiporill ", 123, NOMAP},
{"Wind      ", 121, NOMAP},
{"Wind    MS", 121, NOMAP},
{"Wind2   MS", 121, NOMAP},
{"Woodpecker", 115, NOMAP},
{"WtrFall MS", 96, NOMAP},
{0, 0}
};


SCIsdebug(char *format, ...)
{
  va_list list;

  va_start(list, format);
  vfprintf(stderr, format, list);
  va_end(list);
}



const char MIDI_Header[4] = "MThd";
/* Starts every MIDI header */

const char MIDI_Header_data[10] = { 0, 0, 0, 6,
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

#define MIDI_TIMESTAMP(deltatime) \
if (deltatime > 0x1FFFFF) \
  obstack_1grow(stackp, deltatime & 0xFFFFFFF>> 21 | 0x80); \
if (deltatime > 0x3FFF) \
  obstack_1grow(stackp, deltatime & 0x1FFFFF >> 14 | 0x80); \
if (deltatime > 0x7F) \
  obstack_1grow(stackp, deltatime & 0x3FFF >> 7 | 0x80); \
obstack_1grow(stackp, deltatime & 0x7F) \

#define SETUP_INSTRUMENT(channel, sci_instrument) \
MIDI_TIMESTAMP(0); \
obstack_1grow(stackp, 0xb0 | channel); \
obstack_1grow(stackp, 0x65); \
obstack_1grow(stackp, 0x00); \
MIDI_TIMESTAMP(0); \
obstack_1grow(stackp, 0x64); \
obstack_1grow(stackp, 0x02); \
MIDI_TIMESTAMP(0); \
obstack_1grow(stackp, 0x06); \
obstack_1grow(stackp, MIDI_patchmap[sci_instrument].keyshift); \
MIDI_TIMESTAMP(0); \
obstack_1grow(stackp, 0x65); \
obstack_1grow(stackp, 0x00); \
MIDI_TIMESTAMP(0); \
obstack_1grow(stackp, 0x64); \
obstack_1grow(stackp, 0x01); \
MIDI_TIMESTAMP(0); \
obstack_1grow(stackp, 0x06); \
obstack_1grow(stackp, MIDI_patchmap[sci_instrument].finetune & 0x3FFF >> 7); \
MIDI_TIMESTAMP(0); \
obstack_1grow(stackp, 0x26); \
obstack_1grow(stackp, MIDI_patchmap[sci_instrument].finetune & 0x7F); \
MIDI_TIMESTAMP(0); \
obstack_1grow(stackp, 0x65); \
obstack_1grow(stackp, 0x00); \
MIDI_TIMESTAMP(0); \
obstack_1grow(stackp, 0x64); \
obstack_1grow(stackp, 0x00); \
MIDI_TIMESTAMP(0); \
obstack_1grow(stackp, 0x06); \
obstack_1grow(stackp, MIDI_patchmap[sci_instrument].bender_range); \
MIDI_TIMESTAMP(0); \
obstack_1grow(stackp, 0x26); \
obstack_1grow(stackp, 0x00)

guint8 *
makeMIDI0(const guint8 *src, int *size)
/* Generates a MIDI memory block (looking like a MIDI file).
** size is set to the total size of the memory block.
** Returns 0 on failure.
*/
{
#ifdef HAVE_OBSTACK_H
  struct obstack stack; /* This is where we store the temp results */
  struct obstack *stackp = &stack;
  int chn;
  char muteflags[16]; /* Remembers unmapped instruments */
  guint32 pos; /* position in src */
  guint32 pending_delay = 0;
  guint8 status, laststatus = 0;

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

  for (chn=0; chn < 16; chn ++)
    if (chn != MIDI_PERCUSSIONS)
      muteflags[chn] = 1;
    else
      muteflags[chn] = 0;

  pos = 0x21; /* skip the header */

  while (!EOT) {

    pending_delay += src[pos];
    pos++;    

    if (src[pos] & 0x80) {
      status = src[pos];
      pos++;
    };
    
    if (status < 0xb0) {
    /* Note Off (8x), Note On (9x), Aftertouch (Ax) */
      if ((status & 0x0f) == MIDI_PERCUSSIONS) {
	if (MIDI_rhythmkey[src[pos]] >= 0) {
          MIDI_TIMESTAMP(pending_delay);
	  pending_delay = 0;
	  if (status != laststatus)
	    obstack_1grow(stackp, status);
	  laststatus = status;
	  obstack_1grow(stackp, MIDI_rhythmkey[src[pos]]);
	  obstack_1grow(stackp, src[pos + 1]);
        };
      } else if (muteflags[status & 0x0f] == 0) {
	MIDI_TIMESTAMP(pending_delay);
	pending_delay = 0;
	if (status != laststatus)
	  obstack_1grow(stackp, status);
	laststatus = status;
	obstack_1grow(stackp, src[pos]);
	obstack_1grow(stackp, src[pos + 1]);
      };
      pos += 2;
    } else if (status < 0xc0) {
    /* Controller */
      if (muteflags[status & 0x0f] == 0) {
	MIDI_TIMESTAMP(pending_delay);
	pending_delay = 0;
	if (status != laststatus)
	  obstack_1grow(stackp, status);
	laststatus = status;
	obstack_1grow(stackp, src[pos]);
	obstack_1grow(stackp, src[pos + 1]);
      };
      pos +=2;
    } else if (status < 0xd0) {
    /* Program (patch) Change */
      if (((status & 0xf) != MIDI_PERCUSSIONS) && (src[pos] != 0x7f) && (MIDI_patchmap[src[pos]].gm_instr >= 0)) {
	SCIsdebug("Channel: %d, Patch: %d", status & 0xf, src[pos]);
	SCIsdebug(" => %s\n", GM_Instrument_Names[MIDI_patchmap[src[pos]].gm_instr]);
	MIDI_TIMESTAMP(pending_delay);
	pending_delay = 0;
	obstack_1grow(stackp, status);
	obstack_1grow(stackp, MIDI_patchmap[src[pos]].gm_instr);
	SETUP_INSTRUMENT(status & 0xf, src[pos]);
	laststatus = 0;
	muteflags[status & 0x0f] = 0;
      } else {
        SCIsdebug("* (Channel: %d, Patch: %d)\n", status & 0xf, src[pos]);
        muteflags[status & 0x0f] = 1;
      };
      pos++;
    } else if (status < 0xe0) {
    /* Channel Pressure */
      if (muteflags[status & 0xF] == 0) {
	MIDI_TIMESTAMP(pending_delay);
	pending_delay = 0;
	if (status != laststatus)
	  obstack_1grow(stackp, status);
	laststatus = status;
	obstack_1grow(stackp, src[pos]);
      };
      pos++;
    } else if (status < 0xf0) {
    /* Pitch Wheel */
      if (!muteflags[status & 0xF] == 0) {
	MIDI_TIMESTAMP(pending_delay);
	pending_delay = 0;
	if (status != laststatus)
	  obstack_1grow(stackp, status);
	laststatus = status;
	obstack_1grow(stackp, src[pos]);
	obstack_1grow(stackp, src[pos + 1]);
      };
      pos += 2;
    } else if (status == 0xfc) {
    /* End of Track (is this official?) */
      MIDI_TIMESTAMP(pending_delay);
      pending_delay = 0;
      obstack_1grow(stackp, 0xff);
      obstack_1grow(stackp, 0x2f);
      obstack_1grow(stackp, 0x00);
      EOT = 1;
    } else if (status == 0xff) { /* Meta command */
      switch (src[pos]) {
      case 0x2f:
	MIDI_TIMESTAMP(pending_delay);
	pending_delay = 0;
        obstack_1grow(stackp, status);
        obstack_1grow(stackp, src[pos]);
        obstack_1grow(stackp, 0x00);
	EOT = 1;
	break;
      default:
	SCIsdebug("Illegal or unsupported MIDI meta instruction: FF %02x\n",
			 src[pos+1]);
	return NULL;
      }
    } else {
      SCIsdebug("Illegal or unsupported MIDI extended instruction: %02x\n", status);
      return NULL;
    };
  };

  *size = obstack_object_size(stackp);

  SCIsdebug("MIDI: EOT reached at %i, resulting MIDI block size is %i\n",
	    pos, *size);


  result = malloc(*size);
  memcpy(result, obstack_finish(stackp), *size);
  obstack_free(stackp, NULL);

  pos = *size - tracklength_index - 4; /* length of the MTrk block */
#ifndef WORDS_BIGENDIAN
  pos = GUINT32_SWAP_LE_BE_CONSTANT(pos);
#endif /* !WORDS_BIGENDIAN */
  *((guint32 *)(result + tracklength_index)) = pos;

  return result;
#else /* !HAVE_OBSTACK_H */

  return NULL;
#endif  /* !HAVE_OBSTACK_H */

}

#undef MIDI_TIMESTAMP(deltatime)
#undef SETUP_INSTRUMENT(channel, sci_instrument)

gint8
_lookup_instrument(char *iname)
{
  int i = 0;

  while (MT32_MemoryTimbreMaps[i].name) {
    if (g_strncasecmp(iname, MT32_MemoryTimbreMaps[i].name, 10) == 0)
      return MT32_MemoryTimbreMaps[i].gm_instr;
    i++;
  }
  return MAP_NOT_FOUND;
}

gint8
_lookup_rhythm_key(char *iname)
{
  int i = 0;

  while (MT32_MemoryTimbreMaps[i].name) {
    if (g_strncasecmp(iname, MT32_MemoryTimbreMaps[i].name, 10) == 0)
      return MT32_MemoryTimbreMaps[i].gm_rhythm_key;
    i++;
  }
  return MAP_NOT_FOUND;
}

int
mapMIDIInstruments(void)
{
  resource_t *patch1 = findResource(sci_patch, 1);
  int memtimbres;
  int patches;
  int i;
  guint8 group, number, keyshift, finetune, bender_range;
  guint8 *patchpointer;
  guint32 pos;
  FILE *logfile;

  for (i = 0; i < 128; i++) {
    MIDI_patchmap[i].gm_instr = MT32_PresetTimbreMaps[i].gm_instr;
    MIDI_patchmap[i].keyshift = 0x40;
    MIDI_patchmap[i].finetune = 0x2000;
    MIDI_patchmap[i].bender_range = 0x0c;
  };

  for (i = 0; i < 128; i++)
    MIDI_rhythmkey[i] = MT32_PresetRhythmKeymap[i];

  if (sci_version > SCI_VERSION_1)
    return 0;

  if (!patch1) {
    if (sci_version < SCI_VERSION_1) {
      fprintf(stderr,"No patch.001 found: using default MT-32 mapping magic!\n");
      return 1;
    }
    return 0;
  }

  if (!(logfile = fopen("freesci_instr", "a")))
    perror("While trying to open freesci_instr");

  memtimbres = *(patch1->data + 0x1EB);
  pos = 0x1EC + memtimbres * 0xF6;

  if (patch1->length > pos && \
      ((0x100 * *(patch1->data + pos) + *(patch1->data +pos + 1)) == 0xABCD)) {
    patches = 96;
    pos += 2 + 8 * 48;
  } else 
    patches = 48;

  SCIsdebug("MIDI mapping magic: %d MT-32 Patches detected\n", patches);
  SCIsdebug("MIDI mapping magic: %d MT-32 Memory Timbres\n", memtimbres);

  for (i = 0; i < patches; i++) {
    if (i < 48) {
      group = *(patch1->data + 0x6B + 8 * i);
      number = *(patch1->data + 0x6B + 8 * i + 1);
      keyshift = *(patch1->data + 0x6B + 8 * i + 2);
      finetune = *(patch1->data + 0x6B + 8 * i + 3);
      bender_range = *(patch1->data + 0x6B + 8 * i + 4);
      patchpointer = patch1->data + 0x6B + 8 * i;

    } else {
      group = *(patch1->data + 0x1EC + 8 * (i - 48) + memtimbres * 0xF6 + 2);
      number = *(patch1->data + 0x1EC + 8 * (i - 48) + memtimbres * 0xF6 + 2 + 1);
      keyshift = *(patch1->data + 0x1EC + 8 * (i - 48) + memtimbres * 0xF6 + 2 + 2);
      finetune = *(patch1->data + 0x1EC + 8 * (i - 48) + memtimbres * 0xF6 + 2 + 3);
      bender_range = *(patch1->data + 0x1EC + 8 * (i - 48) + memtimbres * 0xF6 + 2 + 4);
      patchpointer = patch1->data + 0x1EC + 8 * (i - 48) + memtimbres * 0xF6 + 2; 
    };

    if ((patchpointer[0] == 0) && \
        (patchpointer[1] == 0) && \
	(patchpointer[2] == 0) && \
	(patchpointer[3] == 0) && \
	(patchpointer[4] == 0) && \
	(patchpointer[5] == 0) && \
	(patchpointer[6] == 0) && \
	(patchpointer[7] == 0)) {
      MIDI_patchmap[i].gm_instr = NOMAP;
      /* SCIsdebug("Patch %d will be unmapped!\n", i); */
    } else {
      switch (group) {
        case 0:
	  MIDI_patchmap[i].gm_instr = MT32_PresetTimbreMaps[number].gm_instr;
	  break;
        case 1:
          MIDI_patchmap[i].gm_instr = MT32_PresetTimbreMaps[number + 64].gm_instr;
	  break;
        case 2:
	  MIDI_patchmap[i].gm_instr = _lookup_instrument(patch1->data + 0x1EC + number * 0xF6);
	  break;
        case 3:
	  MIDI_patchmap[i].gm_instr = MT32_RhythmTimbreMaps[number].gm_instr;
	  break;
        default:
	  break;
      };
      MIDI_patchmap[i].keyshift = 0x40 + ((int) (keyshift & 0x3F) - 24);
      MIDI_patchmap[i].finetune = 0x2000 + ((int) (finetune & 0x7F) - 50);
      MIDI_patchmap[i].bender_range = (int) (bender_range & 0x1F);
    };
  };

  if (patch1->length > pos && \
      ((0x100 * *(patch1->data + pos) + *(patch1->data + pos + 1)) == 0xDCBA)) {
    for (i = 0; i < 64 ; i++) {
      number = *(patch1->data + pos + 4 * i + 2);
      if (number < 64)
	MIDI_rhythmkey[i + 23] = _lookup_rhythm_key(patch1->data + 0x1EC + number * 0xF6);
      else if (number < 94)
        MIDI_rhythmkey[i + 23] = MT32_RhythmTimbreMaps[number - 64].gm_rhythmkey;
      else
	MIDI_rhythmkey[i + 23] = NOMAP;
    };
    SCIsdebug("MIDI mapping magic: MT-32 Rhythm Channel Note Map\n", memtimbres);
  };
  
  if (logfile)
    fclose(logfile);
  return 0;
}


int
sound_map_instruments(state_t *s)
{
  return 0;
}
