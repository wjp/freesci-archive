/***************************************************************************
 map-mt32-to-gm.c (C) 1999,2001 Christoph Reichenbach, TU Darmstadt
        (C) 1999-2000 Rickard Lind
        (C) 2008 Walter van Niftrik

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

#include <sciresource.h>
#include <engine.h>
#include <stdarg.h>
#include <sfx-instrument-map.h>

#define MAP_NOT_FOUND -1
#define NOMAP -2
#define RHYTHM -3

typedef struct {
	gint8 gm_instr;
	int keyshift;
	int finetune;
	int bender_range;
	gint8 gm_rhythmkey;
	int volume;
} MIDI_map_t;

static struct {
  guint8 group; /* 0=GroupA, 1=GroupB, 2=Memory, 3=Rhythm */
  char *name;
} MT32_patch[128];

MIDI_map_t MIDI_mapping[128];
int MIDI_mappings_nr = 128;

char *GM_Instrument_Names[] = {

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
char *GM_Percussion_Names[] = {
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


static struct {
  char *name;
  gint8 gm_instr;
  gint8 gm_rhythm_key;
} MT32_PresetTimbreMaps[] = {
/*000*/  {"AcouPiano1", 0, NOMAP},
/*001*/  {"AcouPiano2", 1, NOMAP},
/*002*/  {"AcouPiano3", 0, NOMAP},
/*003*/  {"ElecPiano1", 4, NOMAP},
/*004*/  {"ElecPiano2", 5, NOMAP},
/*005*/  {"ElecPiano3", 4, NOMAP},
/*006*/  {"ElecPiano4", 5, NOMAP},
/*007*/  {"Honkytonk ", 3, NOMAP},
/*008*/  {"Elec Org 1", 16, NOMAP},
/*009*/  {"Elec Org 2", 17, NOMAP},
/*010*/  {"Elec Org 3", 18, NOMAP},
/*011*/  {"Elec Org 4", 18, NOMAP},
/*012*/  {"Pipe Org 1", 19, NOMAP},
/*013*/  {"Pipe Org 2", 19, NOMAP},
/*014*/  {"Pipe Org 3", 20, NOMAP},
/*015*/  {"Accordion ", 21, NOMAP},
/*016*/  {"Harpsi 1  ", 6, NOMAP},
/*017*/  {"Harpsi 2  ", 6, NOMAP},
/*018*/  {"Harpsi 3  ", 6, NOMAP},
/*019*/  {"Clavi 1   ", 7, NOMAP},
/*020*/  {"Clavi 2   ", 7, NOMAP},
/*021*/  {"Clavi 3   ", 7, NOMAP},
/*022*/  {"Celesta 1 ", 8, NOMAP},
/*023*/  {"Celesta 2 ", 8, NOMAP},
/*024*/  {"Syn Brass1", 62, NOMAP},
/*025*/  {"Syn Brass2", 63, NOMAP},
/*026*/  {"Syn Brass3", 62, NOMAP},
/*027*/  {"Syn Brass4", 63, NOMAP},
/*028*/  {"Syn Bass 1", 38, NOMAP},
/*029*/  {"Syn Bass 2", 39, NOMAP},
/*030*/  {"Syn Bass 3", 38, NOMAP},
/*031*/  {"Syn Bass 4", 39, NOMAP},
/*032*/  {"Fantasy   ", 88, NOMAP},
/*033*/  {"Harmo Pan ", 89, NOMAP},
/*034*/  {"Chorale   ", 52, NOMAP},
/*035*/  {"Glasses   ", 98, NOMAP},
/*036*/  {"Soundtrack", 97, NOMAP},
/*037*/  {"Atmosphere", 99, NOMAP},
/*038*/  {"Warm Bell ", 89, NOMAP},
/*039*/  {"Funny Vox ", 85, NOMAP},
/*040*/  {"Echo Bell ", 39, NOMAP},
/*041*/  {"Ice Rain  ", 101, NOMAP},
/*042*/  {"Oboe 2001 ", 68, NOMAP},
/*043*/  {"Echo Pan  ", 87, NOMAP},
/*044*/  {"DoctorSolo", 86, NOMAP},
/*045*/  {"Schooldaze", 103, NOMAP},
/*046*/  {"BellSinger", 88, NOMAP},
/*047*/  {"SquareWave", 80, NOMAP},
/*048*/  {"Str Sect 1", 48, NOMAP},
/*049*/  {"Str Sect 2", 48, NOMAP},
/*050*/  {"Str Sect 3", 49, NOMAP},
/*051*/  {"Pizzicato ", 45, NOMAP},
/*052*/  {"Violin 1  ", 40, NOMAP},
/*053*/  {"Violin 2  ", 40, NOMAP},
/*054*/  {"Cello 1   ", 42, NOMAP},
/*055*/  {"Cello 2   ", 42, NOMAP},
/*056*/  {"Contrabass", 43, NOMAP},
/*057*/  {"Harp 1    ", 46, NOMAP},
/*058*/  {"Harp 2    ", 46, NOMAP},
/*059*/  {"Guitar 1  ", 24, NOMAP},
/*060*/  {"Guitar 2  ", 25, NOMAP},
/*061*/  {"Elec Gtr 1", 26, NOMAP},
/*062*/  {"Elec Gtr 2", 27, NOMAP},
/*063*/  {"Sitar     ", 104, NOMAP},
/*064*/  {"Acou Bass1", 32, NOMAP},
/*065*/  {"Acou Bass2", 33, NOMAP},
/*066*/  {"Elec Bass1", 34, NOMAP},
/*067*/  {"Elec Bass2", 39, NOMAP},
/*068*/  {"Slap Bass1", 36, NOMAP},
/*069*/  {"Slap Bass2", 37, NOMAP},
/*070*/  {"Fretless 1", 35, NOMAP},
/*071*/  {"Fretless 2", 35, NOMAP},
/*072*/  {"Flute 1   ", 73, NOMAP},
/*073*/  {"Flute 2   ", 73, NOMAP},
/*074*/  {"Piccolo 1 ", 72, NOMAP},
/*075*/  {"Piccolo 2 ", 72, NOMAP},
/*076*/  {"Recorder  ", 74, NOMAP},
/*077*/  {"Panpipes  ", 75, NOMAP},
/*078*/  {"Sax 1     ", 64, NOMAP},
/*079*/  {"Sax 2     ", 65, NOMAP},
/*080*/  {"Sax 3     ", 66, NOMAP},
/*081*/  {"Sax 4     ", 67, NOMAP},
/*082*/  {"Clarinet 1", 71, NOMAP},
/*083*/  {"Clarinet 2", 71, NOMAP},
/*084*/  {"Oboe      ", 68, NOMAP},
/*085*/  {"Engl Horn ", 69, NOMAP},
/*086*/  {"Bassoon   ", 70, NOMAP},
/*087*/  {"Harmonica ", 22, NOMAP},
/*088*/  {"Trumpet 1 ", 56, NOMAP},
/*089*/  {"Trumpet 2 ", 56, NOMAP},
/*090*/  {"Trombone 1", 57, NOMAP},
/*091*/  {"Trombone 2", 57, NOMAP},
/*092*/  {"Fr Horn 1 ", 60, NOMAP},
/*093*/  {"Fr Horn 2 ", 60, NOMAP},
/*094*/  {"Tuba      ", 58, NOMAP},
/*095*/  {"Brs Sect 1", 61, NOMAP},
/*096*/  {"Brs Sect 2", 61, NOMAP},
/*097*/  {"Vibe 1    ", 11, NOMAP},
/*098*/  {"Vibe 2    ", 11, NOMAP},
/*099*/  {"Syn Mallet", 15, NOMAP},
/*100*/  {"Wind Bell ", 88, NOMAP},
/*101*/  {"Glock     ", 9, NOMAP},
/*102*/  {"Tube Bell ", 14, NOMAP},
/*103*/  {"Xylophone ", 13, NOMAP},
/*104*/  {"Marimba   ", 12, NOMAP},
/*105*/  {"Koto      ", 107, NOMAP},
/*106*/  {"Sho       ", 111, NOMAP},
/*107*/  {"Shakuhachi", 77, NOMAP},
/*108*/  {"Whistle 1 ", 78, NOMAP},
/*109*/  {"Whistle 2 ", 78, NOMAP},
/*110*/  {"BottleBlow", 76, NOMAP},
/*111*/  {"BreathPipe", 121, NOMAP},
/*112*/  {"Timpani   ", 47, NOMAP},
/*113*/  {"MelodicTom", 117, NOMAP},
/*114*/  {"Deep Snare", RHYTHM, 37},
/*115*/  {"Elec Perc1", 115, NOMAP}, /* ? */
/*116*/  {"Elec Perc2", 118, NOMAP}, /* ? */
/*117*/  {"Taiko     ", 116, NOMAP},
/*118*/  {"Taiko Rim ", 118, NOMAP},
/*119*/  {"Cymbal    ", RHYTHM, 50},
/*120*/  {"Castanets ", RHYTHM, NOMAP},
/*121*/  {"Triangle  ", 112, NOMAP},
/*122*/  {"Orche Hit ", 55, NOMAP},
/*123*/  {"Telephone ", 124, NOMAP},
/*124*/  {"Bird Tweet", 123, NOMAP},
/*125*/  {"OneNoteJam", NOMAP, NOMAP}, /* ? */
/*126*/  {"WaterBells", 98, NOMAP},
/*127*/  {"JungleTune", NOMAP, NOMAP} /* ? */
};

static struct {
char *name;
gint8 gm_instr;
gint8 gm_rhythmkey;
} MT32_RhythmTimbreMaps[] = {
/*00*/  {"Acou BD   ", RHYTHM, 34},
/*01*/  {"Acou SD   ", RHYTHM, 37},
/*02*/  {"Acou HiTom", 117, 49},
/*03*/  {"AcouMidTom", 117, 46},
/*04*/  {"AcouLowTom", 117, 40},
/*05*/  {"Elec SD   ", RHYTHM, 39},
/*06*/  {"Clsd HiHat", RHYTHM, 41},
/*07*/  {"OpenHiHat1", RHYTHM, 45},
/*08*/  {"Crash Cym ", RHYTHM, 48},
/*09*/  {"Ride Cym  ", RHYTHM, 50},
/*10*/  {"Rim Shot  ", RHYTHM, 36},
/*11*/  {"Hand Clap ", RHYTHM, 38},
/*12*/  {"Cowbell   ", RHYTHM, 55},
/*13*/  {"Mt HiConga", RHYTHM, 61},
/*14*/  {"High Conga", RHYTHM, 62},
/*15*/  {"Low Conga ", RHYTHM, 63},
/*16*/  {"Hi Timbale", RHYTHM, 64},
/*17*/  {"LowTimbale", RHYTHM, 65},
/*18*/  {"High Bongo", RHYTHM, 59},
/*19*/  {"Low Bongo ", RHYTHM, 60},
/*20*/  {"High Agogo", 113, 66},
/*21*/  {"Low Agogo ", 113, 67},
/*22*/  {"Tambourine", RHYTHM, 53},
/*23*/  {"Claves    ", RHYTHM, 74},
/*24*/  {"Maracas   ", RHYTHM, 69},
/*25*/  {"SmbaWhis L", 78, 71},
/*26*/  {"SmbaWhis S", 78, 70},
/*27*/  {"Cabasa    ", RHYTHM, 68},
/*28*/  {"Quijada   ", RHYTHM, 72},
/*29*/  {"OpenHiHat2", RHYTHM, 43}
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
NOMAP, NOMAP, NOMAP, NOMAP, NOMAP, NOMAP, NOMAP, NOMAP
};

/* +++ - Don't change unless you've got a good reason
   ++  - Looks good, sounds ok
   +   - Not too bad, but is it right?
   ?   - Where do I map this one?
   ??  - Any good ideas?
   ??? - I'm clueless?
   R   - Rhythm... */
static struct {
  char *name;
  gint8 gm_instr;
  gint8 gm_rhythm_key;
} MT32_MemoryTimbreMaps[] = {
{"AccPnoKA2 ", 1, NOMAP},     /* ++ (KQ1) */
{"Acou BD   ", RHYTHM, 34},   /* R (PQ2) */
{"Acou SD   ", RHYTHM, 37},   /* R (PQ2) */
{"AcouPnoKA ", 0, NOMAP},     /* ++ (KQ1) */
{"BASS      ", 32, NOMAP},    /* + (LSL3) */
{"BASSOONPCM", 70, NOMAP},    /* + (CB) */
{"BEACH WAVE", 122, NOMAP},   /* + (LSL3) */
{"BagPipes  ", 109, NOMAP},
{"BassPizzMS", 45, NOMAP},    /* ++ (HQ) */
{"BassoonKA ", 70, NOMAP},    /* ++ (KQ1) */
{"Bell    MS", 112, NOMAP},   /* ++ (iceMan) */
{"Bells   MS", 112, NOMAP},   /* + (HQ) */
{"Big Bell  ", 14, NOMAP},    /* + (CB) */
{"Bird Tweet", 123, NOMAP},
{"BrsSect MS", 61, NOMAP},    /* +++ (iceMan) */
{"CLAPPING  ", 126, NOMAP},   /* ++ (LSL3) */
{"Cabasa    ", RHYTHM, 68},   /* R (HBoG) */
{"Calliope  ", 82, NOMAP},    /* +++ (HQ) */
{"CelticHarp", 46, NOMAP},    /* ++ (CoC) */
{"Chicago MS", 1, NOMAP},     /* ++ (iceMan) */
{"Chop      ", 117, NOMAP},
{"Chorale MS", 52, NOMAP},    /* + (CoC) */
{"ClarinetMS", 71, NOMAP},
{"Claves    ", RHYTHM, 74},   /* R (PQ2) */
{"Claw    MS", 118, NOMAP},    /* + (HQ) */
{"ClockBell ", 14, NOMAP},    /* + (CB) */
{"ConcertCym", RHYTHM, 54},   /* R ? (KQ1) */
{"Conga   MS", RHYTHM, 63},   /* R (HQ) */
{"CoolPhone ", 124, NOMAP},   /* ++ (LSL3) */
{"CracklesMS", 115, NOMAP}, /* ? (CoC, HQ) */
{"CreakyD MS", NOMAP, NOMAP}, /* ??? (KQ1) */
{"Cricket   ", 120, NOMAP}, /* ? (CB) */
{"CrshCymbMS", RHYTHM, 56},   /* R +++ (iceMan) */
{"CstlGateMS", NOMAP, NOMAP}, /* ? (HQ) */
{"CymSwellMS", RHYTHM, 54},   /* R ? (CoC, HQ) */
{"CymbRollKA", RHYTHM, 56},   /* R ? (KQ1) */
{"Cymbal Lo ", NOMAP, NOMAP}, /* R ? (LSL3) */
{"card      ", NOMAP, NOMAP}, /* ? (HBoG) */
{"DirtGtr MS", 30, NOMAP},    /* + (iceMan) */
{"DirtGtr2MS", 29, NOMAP},    /* + (iceMan) */
{"E Bass  MS", 33, NOMAP},    /* + (SQ3) */
{"ElecBassMS", 33, NOMAP},
{"ElecGtr MS", 27, NOMAP},    /* ++ (iceMan) */
{"EnglHornMS", 69, NOMAP},
{"FantasiaKA", 88, NOMAP},
{"Fantasy   ", 99, NOMAP},    /* + (PQ2) */
{"Fantasy2MS", 99, NOMAP},    /* ++ (CoC, HQ) */
{"Filter  MS", 95, NOMAP},    /* +++ (iceMan) */
{"Filter2 MS", 95, NOMAP},    /* ++ (iceMan) */
{"Flame2  MS", 121, NOMAP},   /* ? (HQ) */
{"Flames  MS", 121, NOMAP},   /* ? (HQ) */
{"Flute   MS", 73, NOMAP},    /* +++ (HQ) */
{"FogHorn MS", 58, NOMAP},
{"FrHorn1 MS", 60, NOMAP},    /* +++ (HQ) */
{"FunnyTrmp ", 56, NOMAP},    /* ++ (CB) */
{"GameSnd MS", 80, NOMAP},
{"Glock   MS", 9, NOMAP},     /* +++ (HQ) */
{"Gunshot   ", 127, NOMAP},   /* +++ (CB) */
{"Hammer  MS", NOMAP, NOMAP}, /* ? (HQ) */
{"Harmonica2", 22, NOMAP},    /* +++ (CB) */
{"Harpsi 1  ", 6, NOMAP},     /* + (HBoG) */
{"Harpsi 2  ", 6, NOMAP},     /* +++ (CB) */
{"Heart   MS", 116, NOMAP},   /* ? (iceMan) */
{"Horse1  MS", 115, NOMAP},   /* ? (CoC, HQ) */
{"Horse2  MS", 115, NOMAP},   /* ? (CoC, HQ) */
{"InHale  MS", 121, NOMAP},   /* ++ (iceMan) */
{"KNIFE     ", 120, NOMAP},   /* ? (LSL3) */
{"KenBanjo  ", 105, NOMAP},   /* +++ (CB) */
{"Kiss    MS", 25, NOMAP},    /* ++ (HQ) */
{"KongHit   ", NOMAP, NOMAP}, /* ??? (KQ1) */
{"Koto      ", 107, NOMAP},   /* +++ (PQ2) */
{"Laser   MS", 81, NOMAP},    /* ?? (HQ) */
{"Meeps   MS", 62, NOMAP},    /* ? (HQ) */
{"MTrak   MS", 62, NOMAP},    /* ?? (iceMan) */
{"MachGun MS", 127, NOMAP},   /* ? (iceMan) */
{"OCEANSOUND", 122, NOMAP},   /* + (LSL3) */
{"Oboe 2001 ", 68, NOMAP},    /* + (PQ2) */
{"Ocean   MS", 122, NOMAP},   /* + (iceMan) */
{"PPG 2.3 MS", 75, NOMAP},    /* ? (iceMan) */
{"PianoCrank", NOMAP, NOMAP}, /* ? (CB) */
{"PicSnareMS", RHYTHM, 39},   /* R ? (iceMan) */
{"PiccoloKA ", 72, NOMAP},    /* +++ (KQ1) */
{"PinkBassMS", 39, NOMAP},
{"Pizz2     ", 45, NOMAP},    /* ++ (CB) */
{"Portcullis", NOMAP, NOMAP}, /* ? (KQ1) */
{"Raspbry MS", 81, NOMAP},    /* ? (HQ) */
{"RatSqueek ", 72, NOMAP},    /* ? (CB, CoC) */
{"Record78  ", NOMAP, NOMAP}, /* +++ (CB) */
{"RecorderMS", 74, NOMAP},    /* +++ (CoC) */
{"Red Baron ", 125, NOMAP},   /* ? (CB) */
{"ReedPipMS ", 20, NOMAP},    /* +++ (Coc) */
{"RevCymb MS", 119, NOMAP},
{"RifleShot ", 127, NOMAP},   /* + (CB) */
{"RimShot MS", RHYTHM, 36},   /* R */
{"SHOWER    ", 52, NOMAP},    /* ? (LSL3) */
{"SQ Bass MS", 32, NOMAP},    /* + (SQ3) */
{"ShakuVibMS", 79, NOMAP},    /* + (iceMan) */
{"SlapBassMS", 36, NOMAP},    /* +++ (iceMan) */
{"Snare   MS", RHYTHM, 37},   /* R (HQ) */
{"Some Birds", 123, NOMAP},   /* + (CB) */
{"Sonar   MS", 78, NOMAP},    /* ? (iceMan) */
{"Soundtrk2 ", 97, NOMAP},    /* +++ (CB) */
{"Soundtrack", 97, NOMAP},    /* ++ (CoC) */
{"SqurWaveMS", 80, NOMAP},
{"StabBassMS", 34, NOMAP},    /* + (iceMan) */
{"SteelDrmMS", 114, NOMAP},   /* +++ (iceMan) */
{"StrSect1MS", 48, NOMAP},    /* ++ (HQ) */
{"String  MS", 45, NOMAP},    /* + (CoC) */
{"Syn-Choir ", 91, NOMAP},
{"Syn Brass4", 63, NOMAP},    /* ++ (PQ2) */
{"SynBass MS", 38, NOMAP},
{"SwmpBackgr", 120, NOMAP},    /* ?? (CB,HQ) */
{"T-Bone2 MS", 57, NOMAP},    /* +++ (HQ) */
{"Taiko     ", 116, 34},      /* +++ (Coc) */
{"Taiko Rim ", 118, 36},      /* +++ (LSL3) */
{"Timpani1  ", 47, NOMAP},    /* +++ (CB) */
{"Tom     MS", 117, 47},      /* +++ (iceMan) */
{"Toms    MS", 117, 47},      /* +++ (CoC, HQ) */
{"Tpt1prtl  ", 56, NOMAP},    /* +++ (KQ1) */
{"TriangleMS", 112, 80},      /* R (CoC) */
{"Trumpet 1 ", 56, NOMAP},    /* +++ (CoC) */
{"Type    MS", 114, NOMAP},   /* ? (iceMan) */
{"WaterBells", 98, NOMAP},    /* + (PQ2) */
{"WaterFallK", NOMAP, NOMAP}, /* ? (KQ1) */
{"Whiporill ", 123, NOMAP},   /* + (CB) */
{"Wind      ", NOMAP, NOMAP}, /* ? (CB) */
{"Wind    MS", NOMAP, NOMAP}, /* ? (HQ, iceMan) */
{"Wind2   MS", NOMAP, NOMAP}, /* ? (CoC) */
{"Woodpecker", 115, NOMAP},   /* ? (CB) */
{"WtrFall MS", NOMAP, NOMAP}, /* ? (CoC, HQ, iceMan) */
{0, 0}
};

gint8
_lookup_instrument(char *iname)
{
  int i = 0;

  while (MT32_MemoryTimbreMaps[i].name) {
    if (strncasecmp(iname, MT32_MemoryTimbreMaps[i].name, 10) == 0)
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
    if (strncasecmp(iname, MT32_MemoryTimbreMaps[i].name, 10) == 0)
      return MT32_MemoryTimbreMaps[i].gm_rhythm_key;
    i++;
  }
  return MAP_NOT_FOUND;
}
#if 0
int
map_MIDI_instruments(resource_mgr_t *resmgr)
{
  int memtimbres;
  int patches;
  int i;
  guint8 group, number, keyshift, finetune, bender_range;
  guint8 *patchpointer;
  guint32 pos;
  resource_t *patch1;

  patch1 = scir_find_resource(resmgr, sci_patch, 1, 0);

  for (i = 0; i < 128; i++) {
    MIDI_mapping[i].gm_instr = MT32_PresetTimbreMaps[i].gm_instr;
    MIDI_mapping[i].keyshift = 0x40;
    MIDI_mapping[i].finetune = 0x2000;
    MIDI_mapping[i].bender_range = 0x0c;
    MT32_patch[i].name = MT32_PresetTimbreMaps[i].name;
  }

  for (i = 0; i < 128; i++) {
    MIDI_mapping[i].gm_rhythmkey = MT32_PresetRhythmKeymap[i];
    MIDI_mapping[i].volume = 128;
  }

  if (!patch1) {
    if (resmgr->sci_version < SCI_VERSION_1) {
      fprintf(stderr,"No patch.001 found: using default MT-32 mapping magic!\n");
      return 1;
    }
    return 0;
  }

  memtimbres = *(patch1->data + 0x1EB);
  pos = 0x1EC + memtimbres * 0xF6;

  if (patch1->size > pos && \
      ((0x100 * *(patch1->data + pos) + *(patch1->data +pos + 1)) == 0xABCD)) {
    patches = 96;
    pos += 2 + 8 * 48;
  } else
    patches = 48;

  SCIsdebug("MIDI mapping magic: %d MT-32 Patches detected\n", patches);
  if (memtimbres > 0)
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
    }

    MT32_patch[i].group = group;

      switch (group) {
        case 0:
	  MT32_patch[i].name = MT32_PresetTimbreMaps[number].name;
	  MIDI_mapping[i].gm_instr = MT32_PresetTimbreMaps[number].gm_instr;
	  break;
        case 1:
	  MT32_patch[i].name = MT32_PresetTimbreMaps[number + 64].name;
          MIDI_mapping[i].gm_instr = MT32_PresetTimbreMaps[number + 64].gm_instr;
	  break;
        case 2:
	  MT32_patch[i].name = (char *) patch1->data + 0x1EC + number * 0xF6;
	  MIDI_mapping[i].gm_instr = _lookup_instrument((char *) patch1->data + 0x1EC + number * 0xF6);
	  /* SCIsdebug("Patch %d => %d\n",i, MIDI_mapping[i].gm_instr); */
	  break;
        case 3:
	  MT32_patch[i].name = MT32_RhythmTimbreMaps[number].name;
	  MIDI_mapping[i].gm_instr = MT32_RhythmTimbreMaps[number].gm_instr;
	  break;
        default:
	  break;
      MIDI_mapping[i].keyshift = 0x40 + ((int) (keyshift & 0x3F) - 24);
      if (finetune != 50)
	SCIsdebug("Patch %d, fintune %d\n", i, finetune-50);
      /* MIDI_mapping[i].finetune = 0x2000 + (((gint32) (finetune & 0x7F) - 50) << 11) / 25; */
      MIDI_mapping[i].finetune = 0x2000 + ((int) (finetune & 0x7F) - 50);
      MIDI_mapping[i].bender_range = (int) (bender_range & 0x1F);
    }
  }

  if (patch1->size > pos && \
      ((0x100 * *(patch1->data + pos) + *(patch1->data + pos + 1)) == 0xDCBA)) {
    for (i = 0; i < 64 ; i++) {
      number = *(patch1->data + pos + 4 * i + 2);
      if (number < 64)
	MIDI_mapping[i + 23].gm_rhythmkey = _lookup_rhythm_key((char *) patch1->data + 0x1EC + number * 0xF6);
      else if (number < 94)
        MIDI_mapping[i + 23].gm_rhythmkey = MT32_RhythmTimbreMaps[number - 64].gm_rhythmkey;
      else
	MIDI_mapping[i + 23].gm_rhythmkey = NOMAP;
      MIDI_mapping[i + 23].volume = (((long) *(patch1->data + pos + 4 * i + 3))*128) /100;
      /* SCIsdebug("%d => %d\n", i + 23, MIDI_mapping[i + 23].gm_rhythmkey); */
    }
    SCIsdebug("MIDI mapping magic: MT-32 Rhythm Channel Note Map\n", memtimbres);
  }

  return 0;
}
#endif
sfx_instrument_map_t *
sfx_instrument_map_mt32_to_gm(byte *data, size_t size)
{
	sfx_instrument_map_t * map;
	int i;
	int type;

	map = sfx_instrument_map_new(0);

	for (i = 0; i < SFX_INSTRUMENTS_NR; i++) {
		map->patch_map[i] = MT32_PresetTimbreMaps[i].gm_instr;
		map->patch_key_shift[i] = 0;
		map->patch_volume_adjust[i] = 0;
		map->velocity_map_index[i] = SFX_NO_VELOCITY_MAP;
	}

	map->percussion_volume_adjust = 0;
	for (i = 0; i < SFX_RHYTHM_NR; i++)
		map->percussion_map[i] = MT32_PresetRhythmKeymap[i];

	if (!data) {
		sciprintf("No MT-32 patch data supplied, using default mapping\n");
		return map;
	}

	type = sfx_instrument_map_detect(data, size);

	if (type == SFX_MAP_UNKNOWN) {
		sciprintf("Patch data format unknown, using default mapping\n");
		return map;
	}
	if (type == SFX_MAP_SCI1) {
		sciprintf("SCI1 MT-32 patch data is currently not supported, using default mapping\n");
		return map;
	}

	return map;
}

