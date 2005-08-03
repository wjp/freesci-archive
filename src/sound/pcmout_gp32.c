/***************************************************************************
 pcmout_gp32.c Copyright (C) 2005 Walter van Niftrik
               Copyright (C) 2003,2004 Mirko Roller
 
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

#include <pthread.h>
#include <wrap.h>
#include <pcmout.h>

/* Start of code taken from mirkoSDK. */

#define L3D                             0x800   //bit 11
#define L3M                             0x400   //bit 10
#define L3C                             0x200   //bit 9
#define L3DELAY                            16
#define L3_MASK                         0xe00
#define FSMASK 0x7fffffff
#define FS384  0x80000000

enum _busmode { iisbus=0, msbbus=0x4 };
enum _fsmode  { fs512 =0, fs384, fs256 };
static u16 *samplebuffer;

static
void _WrL3Data(unsigned char data,int halt)
{
    signed long i,j;

    if(halt)
    {

        rPEDAT |= L3C;                //L3C=H(while tstp, L3 interface halt condition)
        for(j=0;j<L3DELAY;j++)
            ;       //tstp(L3) > 190ns
        rPEDAT &= (~L3M);
        for (j=0;j<L3DELAY;j++)
            ;
        rPEDAT |= L3M;
    }

    rPEDAT = (rPEDAT & ~L3_MASK) | (L3C|L3M);        //L3M=H(in data transfer mode)
    for(j=0;j<L3DELAY;j++)
        ;                          //tsu(L3)D > 190ns

    //PD[8:6]=L3D:L3C:L3M
    for(i=0;i<8;i++)
    {
        if(data&0x1)    //if data's LSB is 'H'
        {
            rPEDAT &= ~L3C;           //L3C=L
            rPEDAT |= L3D;            //L3D=H
            for(j=0;j<L3DELAY;j++)
                ;   //tcy(L3) > 500ns
            rPEDAT |= (L3C|L3D);      //L3C=H,L3D=H
            for(j=0;j<L3DELAY;j++)
                ;   //tcy(L3) > 500ns
        }
        else            //if data's LSB is 'L'
        {
            rPEDAT &= ~L3C;           //L3C=L
            rPEDAT &= ~L3D;           //L3D=L
            for(j=0;j<L3DELAY;j++)
                ;   //tcy(L3) > 500ns
            rPEDAT |= L3C;            //L3C=H
            rPEDAT &= ~L3D;           //L3D=L
            for(j=0;j<L3DELAY;j++)
                ;   //tcy(L3) > 500ns
        }
        data>>=1;       //for check next bit
    }

    rPEDAT = (rPEDAT & ~L3_MASK) | L3C;
    for (j=0;j<L3DELAY;j++)
        ;
    rPEDAT |= L3M;

}

static
void _WrL3Addr(unsigned char data)
{
    signed long i,j;

    rPEDAT = rPEDAT & ~L3_MASK;       //L3D=L/L3M=L(in address mode)/L3C=L
    rPEDAT |= L3C;                    //L3C=H
    for(j=0;j<L3DELAY;j++)
        ;           //tsu(L3) > 190ns

    //PD[8:6]=L3D:L3C:L3M
    for(i=0;i<8;i++)    //LSB first
    {
        if(data&0x1)    //if data's LSB is 'H'
        {
            rPEDAT &= ~L3C;           //L3C=L
            rPEDAT |= L3D;            //L3D=H
            for(j=0;j<L3DELAY;j++)
                ;   //tcy(L3) > 500ns
            rPEDAT |= L3C;            //L3C=H
            rPEDAT |= L3D;            //L3D=H
            for(j=0;j<L3DELAY;j++)
                ;   //tcy(L3) > 500ns
        }
        else            //if data's LSB is 'L'
        {
            rPEDAT &= ~L3C;           //L3C=L
            rPEDAT &= ~L3D;           //L3D=L
            for(j=0;j<L3DELAY;j++)
                ;   //tcy(L3) > 500ns
            rPEDAT |= L3C;            //L3C=H
            rPEDAT &= ~L3D;           //L3D=L
            for(j=0;j<L3DELAY;j++)
                ;   //tcy(L3) > 500ns
        }
        data >>=1;
    }
    //L3M=H,L3C=H
    rPEDAT = (rPEDAT & ~L3_MASK) | (L3C|L3M);
}

static
int abs_( int x )
{
    x = x < 0 ? -x : x;
    return x;
}

static
unsigned long calcRate( long pclk_, long rate_ )
{
    long fs256 = pclk_ / 256;
    long fs384 = pclk_ / 384;
    long fs256min = 0xffff;
    long fs384min = 0xffff;
    long t;
    int n;
    int ps256 = -1;
    int ps384 = -1;
    int ret;

    for (n = 0; n < 32; n++)
    {
        t = fs256 / (n + 1);            // prescaler+1

        if (abs_((rate_ - t)) < fs256min)
        {
            fs256min = abs_((rate_ - t));
            ps256 = n;
        }
    }

    for (n = 0; n < 32; n++)
    {
        t = fs384 / (n + 1);
        if (abs_((rate_ - t)) < fs384min)
        {
            fs384min = abs_((rate_ - t));
            ps384 = n;
        }
    }

    if (fs256min > fs384min)
    {
        ret = ps384 | 0x80000000;
    }
    else
    {
        ret = ps256;
    }

    return ret;
}

static
int InitIIS(int bus,int freq,int bit)
{
    // bit 0 =  8bit
    // bit 1 = 16bit

    int prescale=0;
    int sysfs=0;
    /****** IIS Initialize ******/

    rIISPSR   = 0;
    rIISCON   = 0;
    rIISMOD   = 0;
    rIISFIFCON= 0;

    prescale = calcRate(bus,freq);  // Calculate if we need 256 or 384 mode
    if (prescale & FS384)
    {
        sysfs=1;
        prescale &= FSMASK;
    }  // 384fs
    else
        sysfs = 0; //256fs


    {
        char IISIFENA =0;   // 0  IIS interface enable (start)
        char IISPSENA =1;   // 1  IIS prescaler enable
        char RXCHIDLE =1;   // 2  Receive channel idle command
        char TXCHIDLE =0;   // 3  Transmit channel idle command
        char RXDMAENA =0;   // 4  Receive DMA service request enable
        char TXDMAENA =1;   // 5  Transmit DMA service request enable
        rIISCON= (TXDMAENA<<5)|(RXDMAENA<<4)|(TXCHIDLE<<3)|(RXCHIDLE<<2)|(IISPSENA<<1)|(IISIFENA<<0);
    }


    {
        char SBCLKFS  =1;    // 0-1 Serial bit clock frequency select   0=16fs 1=32fs  2=48fs
        char MCLKFS   =sysfs;// 2   Master clock frequency select       0=256  1=384
        char SDBITS   =bit;  // 3   Serial data bit per channel         0=8Bit 1=16Bit
        char SIFMT    =0;    // 4   Serial interface format             0=IIs  1=MSB
        char ACTLEVCH =0;    // 5   Active level pf left/right channel  0=low for LeftChannel 1=High for LeftChannel
        char TXRXMODE =2;    // 6-7 Transmit/receive mode select        0=no transfer 1=receive mode 2=transmit mode 3=transmit and receive mode
        char MODE     =0;    // 8   Master/slave mode select            0=master mode(output) 1=slave mode(input)
        rIISMOD= (MODE<<8) | (TXRXMODE<<6) | (ACTLEVCH<<5)|(SIFMT<<4)|(SDBITS<<3)|(MCLKFS<<2)|(SBCLKFS<<0);
    }


    {
        char RXFIFOENA =0;  // 8   recieve  fifo enable       0=disable 1=enable
        char TXFIFOENA =1;  // 9   transmit fifo enable       0=disable 1=enable
        char RXFIFOMODE=0;  //10   recieve  fifo access mode  0=normal 1=DMA
        char TXFIFOMODE=1;  //11   transmit fifo access mode  0=normal 1=DMA
        rIISFIFCON=(TXFIFOMODE<<11)|(RXFIFOMODE<<10)|(TXFIFOENA<<9)|(RXFIFOENA<<8);
    }

    //prescale = calcRate(bus,freq);
    rIISPSR=(prescale<<5)|(prescale<<0);
    rIISCON |=0x1; // iis enable

    if (sysfs == 0)
        sysfs=fs256;
    else
        sysfs=fs384;

    return sysfs;
}

static
void Init1330(int fsmode, int busmode,int vol)
{     // fsbus=2 busmode=0

    /****** I/O Port D Initialize ******/
    rPGCON |= 0xa0;
    rPEDAT  = (rPEDAT & ~0xe00) | (0x400|0x200);
    rPEUP  |= 0xe00;
    rPECON  = (rPECON & (~(0x3f << 18))) | (0x15<<18);

    // setSysfreq( int fsmode, int busmode )
    _WrL3Addr(0x14+2); //STATUS (0) (000101xx+10)
    _WrL3Data( (fsmode<<4) + (busmode<<1), 0); // fsmode,busmode

    //GpControlVolume(0x3f);
    vol &= 0x3f;
    _WrL3Addr(0x14 + 0);
    _WrL3Data(vol & 0x3f, 1);

    //data type transfer // data value - no de-emphasis, no muting
    _WrL3Addr(0x14 + 0);
    _WrL3Data(0x80,1);
}

static
void GpPcmPlay(unsigned short* src, int size, int rep, int bit)
{
    // bit 0 = 8bit
    // bit 1 =16bit
    if (bit==1)
        size/=2;

    rDIDST2=(1<<30)+        /* APB destination on peripherial bus */
            (1<<29)+        /* fixed adress */
            ((int)IISFIF);  /* IIS FIFO txd */
    rDISRC2=(0<<30)+        /* AHP source on system bus */
            (0<<29)+        /* 0=auto-increment 1=fixed/not changed */
            (int)(src);     /* buffer adress */
    rDCON2=(1<<30)+        /* handshake mode */
           (0<<29)+        /* DREQ and DACK are synchronized to PCLK (APB clock) */
           (0<<28)+        /* 1=generate irq when transfer is done */
           (0<<27)+        /* unit transfer !!!! */
           (0<<26)+        /* single service !!!! */
           (0<<24)+        /* dma-req.source=I2SSDO */
           (1<<23)+        /* (H/W request mode) */
           (rep<<22)+      /* 0=auto reload on */
           (bit<<20)+      /* data size, byte,hword,word */
           (size);         /* transfer size (hwords) */

    rDMASKTRIG2=(0<<2)+(1<<1)+0;    //no-stop, DMA2 channel on, no-sw trigger
}

static
void GpPcmStop(void)
{
    rIISCON=0x0;        /* IIS stop */
    rDMASKTRIG2=(1<<2); /* DMA2 stop */
    rIISFIFCON=0x0;     /* for FIFO flush */
    rDMASKTRIG2=(1<<2)+(0<<1)+0; // STOP DMA2, DMACHANNEL2 is turned off
}

static
u32 gp_getSamplepos()
{
    return rDCSRC2;
}

// BSIZE uncached memory for the ringbuffer ( 8kb maximmum )
static int       BSIZE=1920*2;     // size of ringbuffer in Bytes, maximum is 16kb

static void clearRingbuffer()
{
    int i;
    u16 *buf = samplebuffer;
    for (i=0;i<(BSIZE)/2;i++)
        *buf++ = 0;
}

// Init Sound, starts a Ringbuffer with 2 segments
// Ringbuffer is BSIZE (in bytes)
// One segement is BSIZE/2
static void initSound( int freq, int bit, int ringsize)
{
    int sysfs=0;
    samplebuffer = malloc(ringsize);
    if (bit == 8)
        bit=0;
    else
        bit=1;
    BSIZE = ringsize;
    GpPcmStop();
    sysfs=InitIIS (gp_getPCLK(),freq,bit);
    Init1330( sysfs, iisbus,0);
    clearRingbuffer();
    GpPcmPlay( samplebuffer, BSIZE, 0, bit); // Loop endless
}

/* End of code taken from mirkoSDK. */

static gint16 *buffer;
static pthread_t thread;

volatile static int pcm_run;
static guint16 pcm_rate;
static guint8 pcm_stereo;
static guint16 pcm_buffer_size;

static int ringbuf_chunk_size;
static int ringbuf_chunks;

/* Next chunk we need to fill. */
static int ringbuf_next_chunk;

/* Sndstream buffer time in seconds. */
#define STREAM_BUF_TIME 0.2f

static int
current_playing_chunk()
{
    return (gp_getSamplepos() - (unsigned int) samplebuffer) / ringbuf_chunk_size;
}

static void *
pcm_thread(void *p)
{
    struct sched_param param;
    param.sched_priority = 0;

    while (pcm_run)
    {
        int i;
        int count;
        gint16 *buf;

        while (ringbuf_next_chunk == current_playing_chunk())
            /* We can't fill audio yet. */
            sched_yield();

        pthread_setschedparam(thread, SCHED_RR, &param);

        buf = samplebuffer + ringbuf_chunk_size / 2 * ringbuf_next_chunk;

        count = mix_sound(pcm_buffer_size);

        if (pcm_stereo)
        {
            for (i = 0; i < count * 2; i++)
                buf[i] = buffer[i];
        }
        else
        {
            gint16 *buf = samplebuffer + ringbuf_chunk_size / 2 * ringbuf_next_chunk;
            for (i = 0; i < count; i++)
            {
                buf[2 * i] = buffer[i];
                buf[2 * i + 1] = buffer[i];
            }
        }

        if (++ringbuf_next_chunk == ringbuf_chunks)
            ringbuf_next_chunk = 0;
        
        pthread_setschedparam(thread, SCHED_OTHER, &param);
    }

    return NULL;
}

static int
pcmout_gp32_open(gint16 *b, guint16 buffer_size, guint16 rate, guint8 stereo)
{
    ringbuf_chunk_size = buffer_size * 4;
    buffer = b;
    pcm_run = 1;
    pcm_rate = rate;
    pcm_stereo = stereo;
    pcm_buffer_size = buffer_size;

    ringbuf_chunks = (rate * 4 * STREAM_BUF_TIME) / ringbuf_chunk_size;
    if (ringbuf_chunks < 2)
        ringbuf_chunks = 2;

    ringbuf_next_chunk = 0;
    initSound(rate, 16, ringbuf_chunk_size * ringbuf_chunks);
    pthread_create(&thread, NULL, pcm_thread, NULL);
    return 0;
}

static int
pcmout_gp32_close()
{
    pcm_run = 0;
    pthread_join(thread, NULL);
    GpPcmStop();
    free(samplebuffer);

    return 0;
}

pcmout_driver_t pcmout_driver_gp32 =
    {
        "gp32",
        "0.1",
        NULL,
        &pcmout_gp32_open,
        &pcmout_gp32_close
    };
