/***************************************************************************
 sciunpack.c Copyright (C) 1999 Christoph Reichenbach, TU Darmstadt


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

 History:

   990327 - created (CJR)

***************************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#include <engine.h>
#include <sound.h>
#include <console.h>

/* #define DRAW_GRAPHICS */

#ifdef _MSC_VER
#include <direct.h>
#define extern __declspec(dllimport) extern
#endif

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif /* HAVE_GETOPT_H */

#ifdef DRAW_GRAPHICS
#  ifdef HAVE_LIBPNG
#    include <graphics_png.h>
#  endif /* HAVE_LIBPNG */
#endif /* DRAW_GRAPHICS */

#ifdef _MSC_VER
/* [DJ] fchmod is not in Visual C++ RTL - and probably not needed,anyway */
#define fchmod(file,mode)
#define CREAT_OPTIONS O_BINARY

#endif

#ifdef _DOS
/* [RS] (see comment above, but read MS-DOS instead of Visual C++ RTL) */
#define fchmod(file,mode)
#define CREAT_OPTIONS O_BINARY

#endif

#ifdef __unix__
#define CREAT_OPTIONS 0x640
#endif

static int conversion = 1;
static int list = 0;
static int verbose = 0;
static int with_header = 1;
static int dissect = 0;
static int color_mode = 0;

void
print_resource_filename(FILE* file, int type, int number)
{
  if (sci_version < SCI_VERSION_1)
    fprintf(file, "%s.%03d", Resource_Types[type], number);
  else
    fprintf(file, "%d.%s", number, resource_type_suffixes[type]);
}

void
sprint_resource_filename(char* buf, int type, int number)
{
  if (sci_version < SCI_VERSION_1)
    sprintf(buf, "%s.%03d", Resource_Types[type], number);
  else
    sprintf(buf, "%d.%s", number, resource_type_suffixes[type]);
}

#ifdef HAVE_GETOPT_LONG
static struct option options[] = {
  {"no-conversion", no_argument, &conversion, 0},
  {"version", no_argument, 0, 256},
  {"verbose", no_argument, &verbose, 1},
  {"help", no_argument, 0, 'h'},
  {"output-file", required_argument, 0, 'o'},
  {"list", no_argument, &list, 1},
  {"with-header", no_argument, &with_header, 1},
  {"without-header", no_argument, &with_header, 0},
#ifdef DRAW_GRAPHICS
  {"palette-dither", no_argument, &color_mode, SCI_COLOR_DITHER},
  {"palette-interpolate", no_argument, &color_mode, SCI_COLOR_INTERPOLATE},
  {"palette-dither256", no_argument, &color_mode, SCI_COLOR_DITHER256},
#endif /* DRAW_GRAPHICS */
  {"dissect", no_argument, &dissect, 1},
  {"gamedir", required_argument, 0, 'd'},
  {0, 0, 0, 0}};

#endif /* HAVE_GETOPT_LONG */


void unpack_resource(int stype, int snr, char *outfilename);


int main(int argc, char** argv)
{
  int i;
  int stype = -1;
  int snr;
  char *resourcenumber_string = 0;
  char *outfilename = 0;
  int optindex = 0;
  int c;
  char *gamedir = NULL;

#ifdef HAVE_GETOPT_LONG
  while ((c = getopt_long(argc, argv, "vhlo:d:", options, &optindex)) > -1) {
#else /* !HAVE_GETOPT_LONG */
  while ((c = getopt(argc, argv, "vhlo:d:")) > -1) {
#endif /* !HAVE_GETOPT_LONG */
      
      switch (c)
	{
	case 256:
	  printf("sciunpack ("PACKAGE") "VERSION"\n");
	  printf("This program is copyright (C) 1999 Christoph Reichenbach.\n"
		 "It comes WITHOUT WARRANTY of any kind.\n"
		 "This is free software, released under the GNU General Public License.\n");
	  exit(0);
	  
	case 'h':
	  printf("Usage: sciunpack [options] <resource.number>\n"
		 "    or sciunpack [options] <resource> <number>\n"
                 "    If * is specified instead of <number>, \n"
                 "    all resources of given type will be unpacked.\n"
		 "\nAvailable options:\n"
		 " --version              Prints the version number\n"
		 " --verbose     -v       Enables additional output\n"
		 " --help        -h       Displays this help message\n"
		 " --list        -l       List all resources\n"
		 " --output-file -o       Selects output file\n"
                 " --gamedir     -d       Read game resources from dir\n"
		 " --with-header          Forces the SCI header to be written (default)\n"
                 " --without-header       Prevents the two SCI header bytes from being written\n"
#ifdef DRAW_GRAPHICS
		 " --palette-dither       Forces colors in 16 color games to be dithered\n"
		 " --palette-interpolate  Does color interpolation when drawing picture resources\n"
		 " --palette-dither256    Does dithering in 256 colors\n"
#endif /* DRAW_GRAPHICS */
		 " --no-conversion        Does not convert special resources\n"
                 " --dissect              Dissects script resources, stores in <number>.script\n"
		 "\nAs a default, 'resource.number' is the output filename, with the following\n"
		 "exceptions:\n"
		 "  sound resources:   Will be converted to MIDI, stored in <number>.midi\n"
#ifdef DRAW_GRAPHICS
		 "  picture resources: Will be converted to PNG, stored in <number>.png\n"
#endif /* DRAW_GRAPHICS */
		 "\nThis behaviour can be overridden with the --no-conversion switch.\n");
	  exit(0);
	  
	case 'v':
	  verbose = 1;
	  break;

	case 'l':
	  list = 1;
	  break;
	  
	case 'o':
	  outfilename = optarg;
	  break;

        case 'd':
          if (gamedir) free (gamedir);
          gamedir = strdup (optarg);
          break;

	case 0: /* getopt_long already did this for us */
	case '?':
	  /* getopt_long already printed an error message. */
	  break;
	  
	default:
	  return -1;
	}
    }

  if (!list) {
    char *resstring = argv[optind];

    if (optind == argc) {
      fprintf(stderr,"Resource identifier required\n");
      return 1;
    }

    if ((resourcenumber_string = (char *) strchr(resstring, '.'))) {
      *resourcenumber_string++ = 0;
    } else if (optind+1 == argc) {
      fprintf(stderr,"Resource number required\n");
      return 1;
    } else resourcenumber_string = argv[optind+1];

    for (i=0; i< 18; i++)
      if ((strcmp(Resource_Types[i], resstring)==0)) stype = i;
    if (stype==-1) {
      printf("Could not find the resource type '%s'.\n", resstring);
      return 1;
    }
  } /* !list */

  if (gamedir)
    if (chdir (gamedir))
      {
	printf ("Error changing to game directory '%s'\n", gamedir);
	exit(1);
      }

  if ((i = loadResources(SCI_VERSION_AUTODETECT, 0))) {
    fprintf(stderr,"SCI Error: %s!\n", SCI_Error_Types[i]);
    return 1;
  };

  if (verbose) printf("Autodetect determined: %s\n", SCI_Version_Types[sci_version]);


  if (list) {
    int i;

    if (verbose) {
      for (i=0; i<max_resource; i++) {
	printf("%i: ",i);
	print_resource_filename(stdout, resource_map[i].type, resource_map[i].number);
	printf("   has size %i\n", resource_map[i].length);
      }

      fprintf(stderr," Reading complete. Actual resource count is %i\n",
	      max_resource);
    } else {
      for (i=0; i<max_resource; i++) {
	print_resource_filename(stdout, resource_map[i].type, resource_map[i].number);
	printf("\n");
      }
    }
    freeResources();
    return 0;
  }

  if (!strcmp (resourcenumber_string, "*"))
  {
    int i;
    for (i=0; i<max_resource; i++)
      if (resource_map[i].type == stype)
        unpack_resource (stype, resource_map[i].number, NULL);
  } else {
    snr = atoi(resourcenumber_string);
    unpack_resource(stype, snr, outfilename);
  }

  freeResources();
  return 0;
}
  

void unpack_resource(int stype, int snr, char *outfilename)
{
  char fnamebuffer[12]; /* stores default file name */
  resource_t *found;

  if ((stype == sci_sound) && conversion && (sci_version > SCI_VERSION_0)) {
    fprintf(stderr,"MIDI conversion is only supported for SCI version 0\n");
    conversion = 0;
  }

  if (!outfilename) {
    outfilename = fnamebuffer;
    if ((stype == sci_sound) && conversion) {
	mapMIDIInstruments();
	sprintf(outfilename,"%03d.midi", snr);
    }
#ifdef DRAW_GRAPHICS
    else if ((stype == sci_pic) && conversion)
      sprintf(outfilename,"%03d.png", snr);
#endif /* DRAW_GRAPHICS */
    else
      sprint_resource_filename(outfilename, stype, snr);
  }

  if (verbose) {
    printf("seeking ");
    print_resource_filename(stdout, stype, snr);
    printf("...\n");
  }

  if ((found = findResource(stype, snr))) {

#ifdef DRAW_GRAPHICS
    if ((stype == sci_pic) && conversion) {
      int i;
      picture_t pic = alloc_empty_picture(SCI_RESOLUTION_320X200, SCI_COLORDEPTH_8BPP);
      draw_pic0(pic, 1, 0, found->data);
      if ((i = write_pic_png(outfilename, pic->maps[0]))) {
	fprintf(stderr,"Writing the png failed (%d)\n",i);
      } else if (verbose) printf("Done.\n");
      free_picture(pic);
    } else 
#endif /* DRAW_GRAPHICS */
    if ((stype == sci_script) && dissect) {
      FILE *f;

      sprintf (outfilename, "%03d.script", snr);
      f=fopen (outfilename, "wt");
      con_file=f;
      script_dissect(snr, NULL);
      fclose (f);
    } else 
    {

      int outf = creat(outfilename, CREAT_OPTIONS);

#ifdef HAVE_OBSTACK_H
      if ((stype == sci_sound) && conversion) {
	int midilength;
	guint8 *outdata = makeMIDI0(found->data, &midilength);
	if (!outdata) {
	  fprintf(stderr,"MIDI conversion failed. Aborting...\n");
	  return;
	}
	if (verbose) printf("MIDI conversion from %d bytes of sound resource"
			    " to a %d bytes MIDI file.\n",found->length,
			    midilength);
	write(outf, outdata, midilength);
	free(outdata);
      } else {
#endif /* HAVE_OBSTACK_H */
	guint8 header = 0x80 | found->type;

	if (with_header) {
	  write(outf, &header, 1);
	  header = 0x00;
	  write(outf, &header, 1);
	}

	write(outf,  found->data, found->length);
#ifdef HAVE_OBSTACK_H
      }
#endif /* HAVE_OBSTACK_H */

      fchmod(outf, 0644);
      close(outf);
      fchmod(outf, 0644);

      if (verbose) printf("Done.\n");
    }

  } else printf("Resource not found.\n");
}
