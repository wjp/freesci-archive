/***************************************************************************
 pcmout_al.c Copyright (C) 2002 Rainer Canavan, Solomon Peachy,
				Claudio Matsuoka

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

***************************************************************************/

#include <pcmout.h>

#ifdef HAVE_DMEDIA_AUDIO_H

#include <dmedia/audio.h>
#include <pthread.h>

static ALconfig alconfig;
static ALport alport;
static int alfd;
static fd_set alfdset;
pthread_attr_t althreadattr;
pthread_t althread;

static gint16 *buffer;
static gint16 largebuffer[BUFFER_SIZE*4];

void* althreadplay(void* arg) 
{
   GTimeVal timeout = {1, 1};
   int nframes;

   pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,NULL);
   pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);

   do {
      nframes=mix_sound(BUFFER_SIZE);
      if (alfd>0) {   /* untested */
         FD_ZERO(&alfdset);
         FD_SET(alfd,&alfdset);
         alSetFillPoint(alport,BUFFER_SIZE>>1);
         select(alfd+1, NULL, &alfdset, NULL, (struct timeval *)&timeout);
      }

      alWriteFrames(alport, buffer, nframes);
      pthread_testcancel();
   } while (alport!=0);

   pthread_exit(NULL);
}

static int pcmout_al_open(gint16 *b, guint16 rate) 
{
   ALpv alparam;
   int  aldev, alitf;
   
   buffer=b;

   alconfig=alNewConfig();
   if (!alconfig) {
      fprintf(stderr, "sgiAL: Couldn't create config: %s\n", alGetErrorString(oserror()));
      return -1;
   }
   alSetSampFmt(alconfig, AL_SAMPFMT_TWOSCOMP);
   alSetWidth(alconfig, AL_SAMPLE_16);
   alSetChannels(alconfig, 2);

   aldev=alGetResourceByName(AL_SYSTEM, "out.analog", AL_DEVICE_TYPE);
   if (!aldev) {
      fprintf(stderr, "sgiAL: invalid device: \n");
      return -1;
   }
   if (alitf=alGetResourceByName(AL_SYSTEM, "out.analog", AL_INTERFACE_TYPE)) {
      alparam.param=AL_INTERFACE;
      alparam.value.i=alitf;
      if (alSetParams(aldev, &alparam, 1) < 0) {
         fprintf(stderr, "sgiAL: invalid ALinterface \n");
         return -1;
      }
   }
   

   alSetDevice(alconfig, aldev);

   alport=alOpenPort("FreeSCI", "w", alconfig);
   if (!alport) {
      fprintf(stderr, "sigAL: Couldn't open ALport: %s\n", alGetErrorString(oserror()));
      return -1;
   }
   
   alparam.param=AL_RATE;
   alparam.value.i=rate;
   if (alSetParams(aldev, &alparam, 1) < 0) {
      fprintf(stderr, "sgiAL: invalid sampling rate: %i \n", rate);
      alClosePort(alport);
      return -1;
   }

   alfd = alGetFD(alport); /* get a fd to wait for */
   if (alfd = -1) {
      fprintf(stderr, "sgiAL: Can't get File Descriptor: %s \n", alGetErrorString(oserror()));
   }


   pthread_attr_init(&althreadattr);
   if (pthread_create(&althread, &althreadattr, &althreadplay, NULL)!=0) {
      fprintf(stderr, "sgiAL: couldn't create thread! \n");
      alClosePort(alport);
      return -1;
   }
   return 0;
}

static int pcmout_al_close() 
{
   pthread_cancel(althread);
   alClosePort(alport);
   pthread_attr_destroy(&althreadattr);
   alfd=-1;
   alport=0;
   return 0;
}

pcmout_driver_t pcmout_driver_al = {
  "irixal",
  "v0.01",
  (int (*)(struct _pcmout_driver*, char*, char*))NULL,
  &pcmout_al_open,
  &pcmout_al_close
};

#endif



