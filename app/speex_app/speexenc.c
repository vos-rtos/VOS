#include "speex_stm32_config.h"

#include <stdio.h>

#include <getopt.h>

#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "speex/speex.h"
#include "speex/speex_header.h"
#include "speex/speex_stereo.h"
#include <ogg/ogg.h>

#include "skeleton.h"
#include "ff.h"
#include "vos.h"

#if !defined(__LITTLE_ENDIAN__) && ( defined(WORDS_BIGENDIAN) || defined(__BIG_ENDIAN__) )
#define le_short(s) ((short) ((unsigned short) (s) << 8) | ((unsigned short) (s) >> 8))
#define be_short(s) ((short) (s))
#else
#define le_short(s) ((short) (s))
#define be_short(s) ((short) ((unsigned short) (s) << 8) | ((unsigned short) (s) >> 8))
#endif

/** Convert little endian */
static inline spx_int32_t le_int(spx_int32_t i)
{
#if !defined(__LITTLE_ENDIAN__) && ( defined(WORDS_BIGENDIAN) || defined(__BIG_ENDIAN__) )
   spx_uint32_t ui, ret;
   ui = i;
   ret =  ui>>24;
   ret |= (ui>>8)&0x0000ff00;
   ret |= (ui<<8)&0x00ff0000;
   ret |= (ui<<24);
   return ret;
#else
   return i;
#endif
}


void comment_init(char **comments, int* length, char *vendor_string);
void comment_add(char **comments, int* length, char *tag, char *val);


/*Write an Ogg page to a file pointer*/
int oe_write_page(ogg_page *page, FIL *fp)
{
   int written;
   u32 num;
   FRESULT res;
   u32 mark = 0;

//   int dumphex(const unsigned char *buf, int size);
//   dumphex(page->header, page->header_len);
   while(1) {
	   res = f_write (fp, page->header+mark, page->header_len-mark, &num);
	   if (res == 0 && num > 0) {
		   mark += num;
	   }
	   if (mark >= page->header_len) break;
   }
   written = mark;
   mark = 0;
//   dumphex(page->body, page->body_len);
   while (1) {
	   res = f_write (fp, page->body+mark, page->body_len-mark, &num);
	   if (res == 0 && num > 0) {
		   mark += num;
	   }
	   if (mark >= page->body_len) break;
   }
   written += mark;
   //written = fwrite(page->header,1,page->header_len, fp);
   //written += fwrite(page->body,1,page->body_len, fp);

   return written;
}

#define MAX_FRAME_SIZE 2000
#define MAX_FRAME_BYTES 2000

static u32 totalxxxxx = 0;
/* Convert input audio bits, endians and channels */
static int read_samples(FIL *fin,int frame_size, int bits, int channels, int lsb, short * input, char *buff, spx_int32_t *size)
{
	u32 num;
	   FRESULT res;
   unsigned char in[MAX_FRAME_BYTES*2];
   int i;
   short *s;
   int nb_read;
   size_t to_read;

   if (size && *size<=0)
   {
      return 0;
   }

   to_read = bits/8*channels*frame_size;

   /*Read input audio*/
   if (size)
   {
      if (*size >= to_read)
      {
         *size -= to_read;
      }
      else
      {
         to_read = *size;
         *size = 0;
      }
   }

   if (buff)
   {
      for (i=0;i<12;i++)
         in[i]=buff[i];
      num = 0;
      res = f_read (fin, in+12, to_read-12, &num);
      if (res == 0) {
      nb_read = num + 12;
      }
      else {
    	  nb_read = 0;
      }
      //nb_read = fread(in+12,1,to_read-12,fin) + 12;
      if (size)
         *size += 12;
   } else {
	   num = 0;
	   res = f_read (fin, in, to_read, &num);
	   if (res != 0) {
		   kprintf("xxxxxxxxxxxxxxxxxxxxxxxxxxx\r\n");
	   }
	   nb_read = num;
	   if (res == 0) {
		   totalxxxxx += num;
		   kprintf("%d!\r\n", totalxxxxx);
	   }
      //nb_read = fread(in,1,to_read,fin);
   }

   nb_read /= bits/8*channels;

   /*kprintf ("%d\r\n", nb_read);*/
   if (nb_read==0)
      return 0;

   s=(short*)in;
   if(bits==8)
   {
      /* Convert 8->16 bits */
      for(i=frame_size*channels-1;i>=0;i--)
      {
         s[i]=(in[i]<<8)^0x8000;
      }
   } else
   {
      /* convert to our endian format */
      for(i=0;i<frame_size*channels;i++)
      {
         if(lsb)
            s[i]=le_short(s[i]);
         else
            s[i]=be_short(s[i]);
      }
   }

   /* FIXME: This is probably redundent now */
   /* copy to float input buffer */
   for (i=0;i<frame_size*channels;i++)
   {
      input[i]=(short)s[i];
   }

   for (i=nb_read*channels;i<frame_size*channels;i++)
   {
      input[i]=0;
   }


   return nb_read;
}

void add_fishead_packet (ogg_stream_state *os) {

   fishead_packet fp;

   memset(&fp, 0, sizeof(fp));
   fp.ptime_n = 0;
   fp.ptime_d = 1000;
   fp.btime_n = 0;
   fp.btime_d = 1000;

   add_fishead_to_stream(os, &fp);
}

/*
 * Adds the fishead packets in the skeleton output stream along with the e_o_s packet
 */
void add_fisbone_packet (ogg_stream_state *os, spx_int32_t serialno, SpeexHeader *header) {

   fisbone_packet fp;

   memset(&fp, 0, sizeof(fp));
   fp.serial_no = serialno;
   fp.nr_header_packet = 2 + header->extra_headers;
   fp.granule_rate_n = header->rate;
   fp.granule_rate_d = 1;
   fp.start_granule = 0;
   fp.preroll = 3;
   fp.granule_shift = 0;

   add_message_header_field(&fp, "Content-Type", "audio/x-speex");

   add_fisbone_to_stream(os, &fp);
}

static void version()
{
   const char* speex_version;
   speex_lib_ctl(SPEEX_LIB_GET_VERSION_STRING, (void*)&speex_version);
   kprintf ("speexenc (Speex encoder) version %s (compiled " __DATE__ ")\r\n", speex_version);
   kprintf ("Copyright (C) 2002-2006 Jean-Marc Valin\r\n");
}

static void version_short()
{
   const char* speex_version;
   speex_lib_ctl(SPEEX_LIB_GET_VERSION_STRING, (void*)&speex_version);
   kprintf ("speexenc version %s\r\n", speex_version);
   kprintf ("Copyright (C) 2002-2006 Jean-Marc Valin\r\n");
}

static void usage()
{
   kprintf ("Usage: speexenc [options] input_file output_file\r\n");
   kprintf ("\r\n");
   kprintf ("Encodes input_file using Speex. It can read the WAV or raw files.\r\n");
   kprintf ("\r\n");
   kprintf ("input_file can be:\r\n");
   kprintf ("  filename.wav      wav file\r\n");
   kprintf ("  filename.*        Raw PCM file (any extension other than .wav)\r\n");
   kprintf ("  -                 stdin\r\n");
   kprintf ("\r\n");
   kprintf ("output_file can be:\r\n");
   kprintf ("  filename.spx      Speex file\r\n");
   kprintf ("  -                 stdout\r\n");
   kprintf ("\r\n");
   kprintf ("Options:\r\n");
   kprintf (" -n, --narrowband   Narrowband (8 kHz) input file\r\n");
   kprintf (" -w, --wideband     Wideband (16 kHz) input file\r\n");
   kprintf (" -u, --ultra-wideband \"Ultra-wideband\" (32 kHz) input file\r\n");
   kprintf (" --quality n        Encoding quality (0-10), default 8\r\n");
   kprintf (" --bitrate n        Encoding bit-rate (use bit-rate n or lower)\r\n");
   kprintf (" --vbr              Enable variable bit-rate (VBR)\r\n");
   kprintf (" --vbr-max-bitrate  Set max VBR bit-rate allowed\r\n");
   kprintf (" --abr rate         Enable average bit-rate (ABR) at rate bps\r\n");
   kprintf (" --vad              Enable voice activity detection (VAD)\r\n");
   kprintf (" --dtx              Enable file-based discontinuous transmission (DTX)\r\n");
   kprintf (" --comp n           Set encoding complexity (0-10), default 3\r\n");
   kprintf (" --nframes n        Number of frames per Ogg packet (1-10), default 1\r\n");
#ifdef USE_SPEEXDSP
   kprintf (" --denoise          Denoise the input before encoding\r\n");
   kprintf (" --agc              Apply adaptive gain control (AGC) before encoding\r\n");
#endif
   kprintf (" --no-highpass      Disable the encoder's built-in high-pass filter\r\n");
   kprintf (" --skeleton         Outputs ogg skeleton metadata (may cause incompatibilities)\r\n");
   kprintf (" --comment          Add the given string as an extra comment. This may be\r\n");
   kprintf ("                     used multiple times\r\n");
   kprintf (" --author           Author of this track\r\n");
   kprintf (" --title            Title for this track\r\n");
   kprintf (" -h, --help         This help\r\n");
   kprintf (" -v, --version      Version information\r\n");
   kprintf (" -V                 Verbose mode (show bit-rate)\r\n");
   kprintf (" --print-rate       Print the bitrate for each frame to standard output\r\n");
   kprintf ("Raw input options:\r\n");
   kprintf (" --rate n           Sampling rate for raw input\r\n");
   kprintf (" --stereo           Consider raw input as stereo\r\n");
   kprintf (" --le               Raw input is little-endian\r\n");
   kprintf (" --be               Raw input is big-endian\r\n");
   kprintf (" --8bit             Raw input is 8-bit unsigned\r\n");
   kprintf (" --16bit            Raw input is 16-bit signed\r\n");
   kprintf ("Default raw PCM input is 16-bit, little-endian, mono\r\n");
   kprintf ("\r\n");
   kprintf ("More information is available from the Speex site: http://www.speex.org\r\n");
   kprintf ("\r\n");
   kprintf ("Please report bugs to the mailing list `speex-dev@xiph.org'.\r\n");
}
#define fgetc vfgetc
u8 vfgetc(FIL *file)
{
	u8 ch;
	f_gets (&ch, 1, file);
	return ch;
}

int read_wav_header(FIL *file, int *rate, int *channels, int *format, spx_int32_t *size)
{
   char ch[5];
   spx_int32_t itmp;
   spx_int16_t stmp;
   spx_int32_t bpersec;
   spx_int16_t balign;
   int skip_bytes;
   int i;
   u32 num;
   FRESULT res;
   ch[4]=0;
#if 0
   fread(ch, 1, 4, file);
   if (strcmp(ch, "RIFF")!=0)
   {
      fseek(file, 0, SEEK_SET);
      return 0;
   }

   fread(&itmp, 4, 1, file);
   *size = le_int(itmp-36);

   fread(ch, 1, 4, file);
   if (strcmp(ch, "WAVE")!=0)
   {
      fprintf (stderr, "RIFF file is not a WAVE file\n");
      return -1;
   }
#endif
   //fread(ch, 1, 4, file);
   num = 0;
	res = f_read (file, ch, 4, &num);
   while (strcmp(ch, "fmt ")!=0)
   {
      //fread(&itmp, 4, 1, file);
	   num = 0;
      res = f_read (file, ch, 4, &num);
      itmp = le_int(itmp);
      /*fprintf (stderr, "skip=%d\n", itmp);*/
      /*strange way of seeking, but it works even for pipes*/
      for (i=0;i<itmp;i++)
         fgetc(file);
      /*fseek(file, itmp, SEEK_CUR);*/
      //fread(ch, 1, 4, file);
      num = 0;
      res = f_read (file, ch, 4, &num);
      if (f_eof(file))
      {
         kprintf ("Corrupted WAVE file: no \"fmt \"\r\n");
         return -1;
      }
   }
   /*if (strcmp(ch, "fmt ")!=0)
   {
      fprintf (stderr, "Corrupted WAVE file: no \"fmt \"\n");
      return -1;
      }*/

   //fread(&itmp, 4, 1, file);
   num = 0;
   res = f_read (file, &itmp, 4, &num);
   itmp = le_int(itmp);
   skip_bytes=itmp-16;
   /*fprintf (stderr, "skip=%d\n", skip_bytes);*/

   //fread(&stmp, 2, 1, file);
   num = 0;
   res = f_read (file, &stmp, 2, &num);
   stmp = le_short(stmp);
   if (stmp!=1)
   {
      kprintf ("Only PCM encoding is supported\r\n");
      return -1;
   }

   //fread(&stmp, 2, 1, file);
   num = 0;
   res = f_read (file, &stmp, 2, &num);
   stmp = le_short(stmp);
   *channels = stmp;

   if (stmp>2)
   {
      kprintf ("Only mono and (intensity) stereo supported\r\n");
      return -1;
   }

   //fread(&itmp, 4, 1, file);
   num = 0;
   res = f_read (file, &itmp, 4, &num);
   itmp = le_int(itmp);
   *rate = itmp;
   if (*rate != 8000 && *rate != 16000 && *rate != 11025 && *rate != 22050 && *rate != 32000 && *rate != 44100 && *rate != 48000)
   {
      kprintf ("Only 8 kHz (narrowband) and 16 kHz (wideband) supported (plus 11.025 kHz and 22.05 kHz, but your mileage may vary)\r\n");
      return -1;
   }

   //fread(&itmp, 4, 1, file);
   num = 0;
   res = f_read (file, &itmp, 4, &num);
   bpersec = le_int(itmp);

   //fread(&stmp, 2, 1, file);
   num = 0;
   res = f_read (file, &stmp, 2, &num);
   balign = le_short(stmp);

   //fread(&stmp, 2, 1, file);
   num = 0;
   res = f_read (file, &stmp, 2, &num);
   stmp = le_short(stmp);
   if (stmp!=16 && stmp!=8)
   {
      kprintf ("Only 8/16-bit linear supported\r\n");
      return -1;
   }
   *format=stmp;

   if (bpersec!=*rate**channels*stmp/8)
   {
      kprintf ("Corrupted header: ByteRate mismatch\r\n");
      return -1;
   }

   if (balign!=*channels*stmp/8)
   {
      kprintf ("Corrupted header: BlockAlign mismatch\r\n");
      return -1;
   }


   /*strange way of seeking, but it works even for pipes*/
   if (skip_bytes>0)
      for (i=0;i<skip_bytes;i++)
         fgetc(file);

   /*fseek(file, skip_bytes, SEEK_CUR);*/

   //fread(ch, 1, 4, file);
   num = 0;
   res = f_read (file, ch, 4, &num);
   while (strcmp(ch, "data")!=0)
   {
      //fread(&itmp, 4, 1, file);
	   num = 0;
      res = f_read (file, &itmp, 4, &num);
      itmp = le_int(itmp);
      /*strange way of seeking, but it works even for pipes*/
      for (i=0;i<itmp;i++)
         fgetc(file);
      /*fseek(file, itmp, SEEK_CUR);*/
      //fread(ch, 1, 4, file);
      num = 0;
      res = f_read (file, ch, 4, &num);
      if (f_eof(file))
      {
         kprintf ("Corrupted WAVE file: no \"data\"\r\n");
         return -1;
      }
   }

   /*Ignore this for now*/
   //fread(&itmp, 4, 1, file);
   num = 0;
   res = f_read (file, &itmp, 4, &num);
   itmp = le_int(itmp);

   *size=itmp;

   return 1;
}


int speexenc_test(int argc, char **argv)
{
	u32 num;
	FRESULT res;
   int nb_samples, total_samples=0, nb_encoded;
   int c;
   int option_index = 0;
   char *inFile, *outFile;
   //FILE *fin, *fout;
   FIL fin, fout;
   short input[MAX_FRAME_SIZE];
   spx_int32_t frame_size;
   int quiet=0;
   spx_int32_t vbr_enabled=0;
   spx_int32_t vbr_max=0;
   int abr_enabled=0;
   spx_int32_t vad_enabled=0;
   spx_int32_t dtx_enabled=0;
   int nbBytes;
   const SpeexMode *mode=NULL;
   int modeID = -1;
   void *st;
   SpeexBits bits;
   char cbits[MAX_FRAME_BYTES];
   int with_skeleton = 0;
   struct option long_options[] =
   {
      {"wideband", no_argument, NULL, 0},
      {"ultra-wideband", no_argument, NULL, 0},
      {"narrowband", no_argument, NULL, 0},
      {"vbr", no_argument, NULL, 0},
      {"vbr-max-bitrate", required_argument, NULL, 0},
      {"abr", required_argument, NULL, 0},
      {"vad", no_argument, NULL, 0},
      {"dtx", no_argument, NULL, 0},
      {"quality", required_argument, NULL, 0},
      {"bitrate", required_argument, NULL, 0},
      {"nframes", required_argument, NULL, 0},
      {"comp", required_argument, NULL, 0},
#ifdef USE_SPEEXDSP
      {"denoise", no_argument, NULL, 0},
      {"agc", no_argument, NULL, 0},
#endif
      {"no-highpass", no_argument, NULL, 0},
      {"skeleton",no_argument,NULL, 0},
      {"help", no_argument, NULL, 0},
      {"quiet", no_argument, NULL, 0},
      {"le", no_argument, NULL, 0},
      {"be", no_argument, NULL, 0},
      {"8bit", no_argument, NULL, 0},
      {"16bit", no_argument, NULL, 0},
      {"stereo", no_argument, NULL, 0},
      {"rate", required_argument, NULL, 0},
      {"version", no_argument, NULL, 0},
      {"version-short", no_argument, NULL, 0},
      {"comment", required_argument, NULL, 0},
      {"author", required_argument, NULL, 0},
      {"title", required_argument, NULL, 0},
      {"print-rate", no_argument, NULL, 0},
      {0, 0, 0, 0}
   };
   int print_bitrate=0;
   spx_int32_t rate=0;
   spx_int32_t size;
   int chan=1;
   int fmt=16;
   spx_int32_t quality=-1;
   float vbr_quality=-1;
   int lsb=1;
   ogg_stream_state os;
   ogg_stream_state so; /* ogg stream for skeleton bitstream */
   ogg_page og;
   ogg_packet op;
   int bytes_written=0, ret, result;
   int id=-1;
   SpeexHeader header;
   int nframes=1;
   spx_int32_t complexity=3;
   const char* speex_version;
   char vendor_string[64];
   char *comments;
   int comments_length;
   int close_in=0, close_out=0;
   int eos=0;
   spx_int32_t bitrate=0;
   double cumul_bits=0, enc_frames=0;
   char first_bytes[12];
   int wave_input=0;
   spx_int32_t tmp;
#ifdef USE_SPEEXDSP
   SpeexPreprocessState *preprocess = NULL;
   int denoise_enabled=0, agc_enabled=0;
#endif
   int highpass_enabled=1;
   int output_rate=0;
   spx_int32_t lookahead = 0;

   speex_lib_ctl(SPEEX_LIB_GET_VERSION_STRING, (void*)&speex_version);
   rpl_snprintf(vendor_string, sizeof(vendor_string), "Encoded with Speex %s", speex_version);

   comment_init(&comments, &comments_length, vendor_string);

   /*Process command-line options*/
   while(1)
   {
      c = getopt_long_v (argc, argv, "nwuhvV",
                       long_options, &option_index);
      if (c==-1)
         break;

      switch(c)
      {
      case 0:
         if (strcmp(long_options[option_index].name,"narrowband")==0)
         {
            modeID = SPEEX_MODEID_NB;
         } else if (strcmp(long_options[option_index].name,"wideband")==0)
         {
            modeID = SPEEX_MODEID_WB;
         } else if (strcmp(long_options[option_index].name,"ultra-wideband")==0)
         {
            modeID = SPEEX_MODEID_UWB;
         } else if (strcmp(long_options[option_index].name,"vbr")==0)
         {
            vbr_enabled=1;
         } else if (strcmp(long_options[option_index].name,"vbr-max-bitrate")==0)
         {
            vbr_max=atoi(optarg);
            if (vbr_max<1)
            {
               kprintf ("Invalid VBR max bit-rate value: %d\r\n", vbr_max);
               exit(1);
            }
         } else if (strcmp(long_options[option_index].name,"abr")==0)
         {
            abr_enabled=atoi(optarg);
            if (!abr_enabled)
            {
               kprintf ("Invalid ABR value: %d\r\n", abr_enabled);
               exit(1);
            }
         } else if (strcmp(long_options[option_index].name,"vad")==0)
         {
            vad_enabled=1;
         } else if (strcmp(long_options[option_index].name,"dtx")==0)
         {
            dtx_enabled=1;
         } else if (strcmp(long_options[option_index].name,"quality")==0)
         {
            quality = atoi (optarg);
            vbr_quality=atof(optarg);
         } else if (strcmp(long_options[option_index].name,"bitrate")==0)
         {
            bitrate = atoi (optarg);
         } else if (strcmp(long_options[option_index].name,"nframes")==0)
         {
            nframes = atoi (optarg);
            if (nframes<1)
               nframes=1;
            if (nframes>10)
               nframes=10;
         } else if (strcmp(long_options[option_index].name,"comp")==0)
         {
            complexity = atoi (optarg);
#ifdef USE_SPEEXDSP
         } else if (strcmp(long_options[option_index].name,"denoise")==0)
         {
            denoise_enabled=1;
         } else if (strcmp(long_options[option_index].name,"agc")==0)
         {
            agc_enabled=1;
#endif
         } else if (strcmp(long_options[option_index].name,"no-highpass")==0)
         {
            highpass_enabled=0;
         } else if (strcmp(long_options[option_index].name,"skeleton")==0)
         {
            with_skeleton=1;
         } else if (strcmp(long_options[option_index].name,"help")==0)
         {
            usage();
            exit(0);
         } else if (strcmp(long_options[option_index].name,"quiet")==0)
         {
            quiet = 1;
         } else if (strcmp(long_options[option_index].name,"version")==0)
         {
            version();
            exit(0);
         } else if (strcmp(long_options[option_index].name,"version-short")==0)
         {
            version_short();
            exit(0);
         } else if (strcmp(long_options[option_index].name,"print-rate")==0)
         {
            output_rate=1;
         } else if (strcmp(long_options[option_index].name,"le")==0)
         {
            lsb=1;
         } else if (strcmp(long_options[option_index].name,"be")==0)
         {
            lsb=0;
         } else if (strcmp(long_options[option_index].name,"8bit")==0)
         {
            fmt=8;
         } else if (strcmp(long_options[option_index].name,"16bit")==0)
         {
            fmt=16;
         } else if (strcmp(long_options[option_index].name,"stereo")==0)
         {
            chan=2;
         } else if (strcmp(long_options[option_index].name,"rate")==0)
         {
            rate=atoi (optarg);
         } else if (strcmp(long_options[option_index].name,"comment")==0)
         {
            if (!strchr(optarg, '='))
            {
               kprintf ("Invalid comment: %s\r\n", optarg);
               kprintf ("Comments must be of the form name=value\r\n");
               exit(1);
            }
           comment_add(&comments, &comments_length, NULL, optarg);
         } else if (strcmp(long_options[option_index].name,"author")==0)
         {
           comment_add(&comments, &comments_length, "author=", optarg);
         } else if (strcmp(long_options[option_index].name,"title")==0)
         {
           comment_add(&comments, &comments_length, "title=", optarg);
         }

         break;
      case 'n':
         modeID = SPEEX_MODEID_NB;
         break;
      case 'h':
         usage();
         exit(0);
         break;
      case 'v':
         version();
         exit(0);
         break;
      case 'V':
         print_bitrate=1;
         break;
      case 'w':
         modeID = SPEEX_MODEID_WB;
         break;
      case 'u':
         modeID = SPEEX_MODEID_UWB;
         break;
      case '?':
         usage();
         exit(1);
         break;
      }
   }
   optind = 1;
   inFile=argv[optind];
   outFile=argv[optind+1];

   /*Initialize Ogg stream struct*/
   HwRandomInit();

   if (ogg_stream_init(&os, 5000/*HwRandomBuildRang(0, 10000)*/)==-1)
   {
      kprintf("Error: stream init failed\r\n");
      exit(1);
   }
   if (with_skeleton && ogg_stream_init(&so, HwRandomBuildRang(0, 10000))==-1)
   {
      kprintf("Error: stream init failed\r\n");
      exit(1);
   }

   if (strcmp(inFile, "-")==0)
   {

   }
   else
   {
      //fin = fopen(inFile, "rb");
      res = f_open(&fin, inFile, FA_READ);
      if (res)//!fin)
      {
         while(1);
      }
      close_in=1;
   }

   {
	   num = 0;
	  res = f_read (&fin, first_bytes, 1*12, &num);
      if (res || num != 12)//fread(first_bytes, 1, 12, fin) != 12)
      {
         exit(1);
      }
      if (strncmp(first_bytes,"RIFF",4)==0 || strncmp(first_bytes,"riff",4)==0)
      {
         if (read_wav_header(&fin, &rate, &chan, &fmt, &size)==-1)
            exit(1);
         wave_input=1;
         lsb=1; /* CHECK: exists big-endian .wav ?? */
      }
   }

   if (modeID==-1 && !rate)
   {
      /* By default, use narrowband/8 kHz */
      modeID = SPEEX_MODEID_NB;
      rate=8000;
   } else if (modeID!=-1 && rate)
   {
      mode = speex_lib_get_mode (modeID);
      if (rate>48000)
      {
         kprintf ("Error: sampling rate too high: %d Hz, try down-sampling\r\n", rate);
         exit(1);
      } else if (rate>25000)
      {
         if (modeID != SPEEX_MODEID_UWB)
         {
            kprintf ("Warning: Trying to encode in %s at %d Hz. I'll do it but I suggest you try ultra-wideband instead\r\n", mode->modeName , rate);
         }
      } else if (rate>12500)
      {
         if (modeID != SPEEX_MODEID_WB)
         {
            kprintf ("Warning: Trying to encode in %s at %d Hz. I'll do it but I suggest you try wideband instead\r\n", mode->modeName , rate);
         }
      } else if (rate>=6000)
      {
         if (modeID != SPEEX_MODEID_NB)
         {
            kprintf ("Warning: Trying to encode in %s at %d Hz. I'll do it but I suggest you try narrowband instead\r\n", mode->modeName , rate);
         }
      } else {
         kprintf ("Error: sampling rate too low: %d Hz\r\n", rate);
         exit(1);
      }
   } else if (modeID==-1)
   {
      if (rate>48000)
      {
         kprintf ("Error: sampling rate too high: %d Hz, try down-sampling\r\n", rate);
         exit(1);
      } else if (rate>25000)
      {
         modeID = SPEEX_MODEID_UWB;
      } else if (rate>12500)
      {
         modeID = SPEEX_MODEID_WB;
      } else if (rate>=6000)
      {
         modeID = SPEEX_MODEID_NB;
      } else {
         kprintf ("Error: Sampling rate too low: %d Hz\r\n", rate);
         exit(1);
      }
   } else if (!rate)
   {
      if (modeID == SPEEX_MODEID_NB)
         rate=8000;
      else if (modeID == SPEEX_MODEID_WB)
         rate=16000;
      else if (modeID == SPEEX_MODEID_UWB)
         rate=32000;
   }

   if (!quiet)
      if (rate!=8000 && rate!=16000 && rate!=32000)
         kprintf ("Warning: Speex is only optimized for 8, 16 and 32 kHz. It will still work at %d Hz but your mileage may vary\r\n", rate);

   if (!mode)
      mode = speex_lib_get_mode (modeID);

   speex_init_header(&header, rate, 1, mode);
   header.frames_per_packet=nframes;
   header.vbr=vbr_enabled;
   header.nb_channels = chan;

   {
      char *st_string="mono";
      if (chan==2)
         st_string="stereo";
      if (!quiet)
         kprintf ("Encoding %d Hz audio using %s mode (%s)\r\n",
               header.rate, mode->modeName, st_string);
   }
   /*kprintf ("Encoding %d Hz audio at %d bps using %s mode\r\n",
     header.rate, mode->bitrate, mode->modeName);*/

   /*Initialize Speex encoder*/
   st = speex_encoder_init(mode);

   if (strcmp(outFile,"-")==0)
   {

   }
   else
   {
	  res = f_open(&fout, outFile, FA_WRITE | FA_CREATE_ALWAYS);
      //fout = fopen(outFile, "wb");
      if (res) //!fout)
      {
         exit(1);
      }
      close_out=1;
   }

   speex_encoder_ctl(st, SPEEX_GET_FRAME_SIZE, &frame_size);
   speex_encoder_ctl(st, SPEEX_SET_COMPLEXITY, &complexity);
   speex_encoder_ctl(st, SPEEX_SET_SAMPLING_RATE, &rate);

   if (quality >= 0)
   {
      if (vbr_enabled)
      {
         if (vbr_max>0)
            speex_encoder_ctl(st, SPEEX_SET_VBR_MAX_BITRATE, &vbr_max);
         speex_encoder_ctl(st, SPEEX_SET_VBR_QUALITY, &vbr_quality);
      }
      else
         speex_encoder_ctl(st, SPEEX_SET_QUALITY, &quality);
   }
   if (bitrate)
   {
      if (quality >= 0 && vbr_enabled)
         kprintf ("Warning: --bitrate option is overriding --quality\r\n");
      speex_encoder_ctl(st, SPEEX_SET_BITRATE, &bitrate);
   }
   if (vbr_enabled)
   {
      tmp=1;
      speex_encoder_ctl(st, SPEEX_SET_VBR, &tmp);
   } else if (vad_enabled)
   {
      tmp=1;
      speex_encoder_ctl(st, SPEEX_SET_VAD, &tmp);
   }
   if (dtx_enabled)
      speex_encoder_ctl(st, SPEEX_SET_DTX, &tmp);
   if (dtx_enabled && !(vbr_enabled || abr_enabled || vad_enabled))
   {
      kprintf ("Warning: --dtx is useless without --vad, --vbr or --abr\r\n");
   } else if ((vbr_enabled || abr_enabled) && (vad_enabled))
   {
      kprintf ("Warning: --vad is already implied by --vbr or --abr\r\n");
   }
   if (with_skeleton) {
      kprintf ("Warning: Enabling skeleton output may cause some decoders to fail.\r\n");
   }

   if (abr_enabled)
   {
      speex_encoder_ctl(st, SPEEX_SET_ABR, &abr_enabled);
   }

   speex_encoder_ctl(st, SPEEX_SET_HIGHPASS, &highpass_enabled);

   speex_encoder_ctl(st, SPEEX_GET_LOOKAHEAD, &lookahead);

#ifdef USE_SPEEXDSP
   if (denoise_enabled || agc_enabled)
   {
      preprocess = speex_preprocess_state_init(frame_size, rate);
      speex_preprocess_ctl(preprocess, SPEEX_PREPROCESS_SET_DENOISE, &denoise_enabled);
      speex_preprocess_ctl(preprocess, SPEEX_PREPROCESS_SET_AGC, &agc_enabled);
      lookahead += frame_size;
   }
#endif
   /* first packet should be the skeleton header. */

   if (with_skeleton) {
      add_fishead_packet(&so);
      if ((ret = flush_ogg_stream_to_file(&so, &fout))) {
         kprintf ("Error: failed skeleton (fishead) header to output stream\r\n");
         exit(1);
      } else
         bytes_written += ret;
   }

   /*Write header*/
   {
      int packet_size;
      op.packet = (unsigned char *)speex_header_to_packet(&header, &packet_size);
      op.bytes = packet_size;
      op.b_o_s = 1;
      op.e_o_s = 0;
      op.granulepos = 0;
      op.packetno = 0;
      ogg_stream_packetin(&os, &op);
      vfree(op.packet);

      while((result = ogg_stream_flush(&os, &og)))
      {
         if(!result) break;
         ret = oe_write_page(&og, &fout);
         if(ret != og.header_len + og.body_len)
         {
            kprintf ("Error: failed writing header to output stream\r\n");
            exit(1);
         }
         else
            bytes_written += ret;
      }

      op.packet = (unsigned char *)comments;
      op.bytes = comments_length;
      op.b_o_s = 0;
      op.e_o_s = 0;
      op.granulepos = 0;
      op.packetno = 1;
      ogg_stream_packetin(&os, &op);
   }

   /* fisbone packet should be write after all bos pages */
   if (with_skeleton) {
      add_fisbone_packet(&so, os.serialno, &header);
      if ((ret = flush_ogg_stream_to_file(&so, &fout))) {
         kprintf ("Error: failed writing skeleton (fisbone )header to output stream\r\n");
         exit(1);
      } else
         bytes_written += ret;
   }

   /* writing the rest of the speex header packets */
   while((result = ogg_stream_flush(&os, &og)))
   {
      if(!result) break;
      ret = oe_write_page(&og, &fout);
      if(ret != og.header_len + og.body_len)
      {
         kprintf ("Error: failed writing header to output stream\r\n");
         exit(1);
      }
      else
         bytes_written += ret;
   }

   vfree(comments);

   /* write the skeleton eos packet */
   if (with_skeleton) {
      add_eos_packet_to_stream(&so);
      if ((ret = flush_ogg_stream_to_file(&so, &fout))) {
         kprintf ("Error: failed writing skeleton header to output stream\r\n");
         exit(1);
      } else
         bytes_written += ret;
   }


   speex_bits_init(&bits);

   if (!wave_input)
   {
      nb_samples = read_samples(&fin,frame_size,fmt,chan,lsb,input, first_bytes, NULL);
   } else {
      nb_samples = read_samples(&fin,frame_size,fmt,chan,lsb,input, NULL, &size);
   }
   if (nb_samples==0)
      eos=1;
   total_samples += nb_samples;
   nb_encoded = -lookahead;

   u32 time_mark =  VOSGetTimeMs();

   /*Main encoding loop (one frame per iteration)*/
   while (!eos || total_samples>nb_encoded)
   {
      id++;
      /*Encode current frame*/
      if (chan==2)
         speex_encode_stereo_int(input, frame_size, &bits);

#ifdef USE_SPEEXDSP
      if (preprocess)
         speex_preprocess(preprocess, input, NULL);
#endif
      speex_encode_int(st, input, &bits);

      nb_encoded += frame_size;
      if (print_bitrate) {
         int tmp;
         char ch=13;
         speex_encoder_ctl(st, SPEEX_GET_BITRATE, &tmp);
         fputc (ch, stderr);
         cumul_bits += tmp;
         enc_frames += 1;
         if (!quiet)
         {
            if (vad_enabled || vbr_enabled || abr_enabled)
               kprintf ("Bitrate is use: %d bps  (average %d bps)   ", tmp, (int)(cumul_bits/enc_frames));
            else
               kprintf ("Bitrate is use: %d bps     ", tmp);
            if (output_rate)
               kprintf ("%d\r\n", tmp);
         }

      }

      if (wave_input)
      {
         nb_samples = read_samples(&fin,frame_size,fmt,chan,lsb,input, NULL, &size);
      } else {
         nb_samples = read_samples(&fin,frame_size,fmt,chan,lsb,input, NULL, NULL);
      }
      if (nb_samples==0)
      {
         eos=1;
      }
      if (eos && total_samples<=nb_encoded)
         op.e_o_s = 1;
      else
         op.e_o_s = 0;
      total_samples += nb_samples;

      if ((id+1)%nframes!=0)
         continue;
      speex_bits_insert_terminator(&bits);
      nbBytes = speex_bits_write(&bits, cbits, MAX_FRAME_BYTES);
      speex_bits_reset(&bits);
      op.packet = (unsigned char *)cbits;
      op.bytes = nbBytes;
      op.b_o_s = 0;
      /*Is this redundent?*/
      if (eos && total_samples<=nb_encoded)
         op.e_o_s = 1;
      else
         op.e_o_s = 0;
      op.granulepos = (id+1)*frame_size-lookahead;
      if (op.granulepos>total_samples)
         op.granulepos = total_samples;
      /*kprintf ("granulepos: %d %d %d %d %d %d\r\n", (int)op.granulepos, id, nframes, lookahead, 5, 6);*/
      op.packetno = 2+id/nframes;

      ogg_stream_packetin(&os, &op);

#if 1
      /*Write all new pages (most likely 0 or 1)*/
      while (ogg_stream_pageout(&os,&og))
      {
    	 //dumphex(og.body, og.body_len);
         ret = oe_write_page(&og, &fout);
         if(ret != og.header_len + og.body_len)
         {
            kprintf ("Error: failed writing header to output stream\r\n");
            exit(1);
         }
         else
            bytes_written += ret;
      }
//      dumphex(og.header, og.header_len);
//      dumphex(og.body, og.body_len);
#endif
   }
   if ((id+1)%nframes!=0)
   {
      while ((id+1)%nframes!=0)
      {
         id++;
         speex_bits_pack(&bits, 15, 5);
      }
      nbBytes = speex_bits_write(&bits, cbits, MAX_FRAME_BYTES);
      op.packet = (unsigned char *)cbits;
      op.bytes = nbBytes;
      op.b_o_s = 0;
      op.e_o_s = 1;
      op.granulepos = (id+1)*frame_size-lookahead;
      if (op.granulepos>total_samples)
         op.granulepos = total_samples;

      op.packetno = 2+id/nframes;
      ogg_stream_packetin(&os, &op);
   }
   /*Flush all pages left to be written*/
#if 1
   while (ogg_stream_flush(&os, &og))
   {
      ret = oe_write_page(&og, &fout);
      if(ret != og.header_len + og.body_len)
      {
         kprintf ("Error: failed writing header to output stream\r\n");
         exit(1);
      }
      else
         bytes_written += ret;
   }
#endif
   speex_encoder_destroy(st);
   speex_bits_destroy(&bits);
   ogg_stream_clear(&os);

   if (close_in)
	   f_close(&fin);
   if (close_out)
	   f_close(&fout);

   kprintf("spends (%d ms)!\r\n;", (u32)(VOSGetTimeMs()-time_mark));
   return 0;
}

/*
 Comments will be stored in the Vorbis style.
 It is describled in the "Structure" section of
    http://www.xiph.org/ogg/vorbis/doc/v-comment.html

The comment header is decoded as follows:
  1) [vendor_length] = read an unsigned integer of 32 bits
  2) [vendor_string] = read a UTF-8 vector as [vendor_length] octets
  3) [user_comment_list_length] = read an unsigned integer of 32 bits
  4) iterate [user_comment_list_length] times {
     5) [length] = read an unsigned integer of 32 bits
     6) this iteration's user comment = read a UTF-8 vector as [length] octets
     }
  7) [framing_bit] = read a single bit as boolean
  8) if ( [framing_bit]  unset or end of packet ) then ERROR
  9) done.

  If you have troubles, please write to ymnk@jcraft.com.
 */

#define readint(buf, base) (((buf[base+3]<<24)&0xff000000)| \
                           ((buf[base+2]<<16)&0xff0000)| \
                           ((buf[base+1]<<8)&0xff00)| \
                            (buf[base]&0xff))
#define writeint(buf, base, val) do{ buf[base+3]=((val)>>24)&0xff; \
                                     buf[base+2]=((val)>>16)&0xff; \
                                     buf[base+1]=((val)>>8)&0xff; \
                                     buf[base]=(val)&0xff; \
                                 }while(0)

void comment_init(char **comments, int* length, char *vendor_string)
{
  int vendor_length=strlen(vendor_string);
  int user_comment_list_length=0;
  int len=4+vendor_length+4;
  char *p=(char*)malloc(len);
  if(p==NULL){
     kprintf ("malloc failed in comment_init()\r\n");
     exit(1);
  }
  writeint(p, 0, vendor_length);
  memcpy(p+4, vendor_string, vendor_length);
  writeint(p, 4+vendor_length, user_comment_list_length);
  *length=len;
  *comments=p;
}
void comment_add(char **comments, int* length, char *tag, char *val)
{
  char* p=*comments;
  int vendor_length=readint(p, 0);
  int user_comment_list_length=readint(p, 4+vendor_length);
  int tag_len=(tag?strlen(tag):0);
  int val_len=strlen(val);
  int len=(*length)+4+tag_len+val_len;

  p=(char*)realloc(p, len);
  if(p==NULL){
     kprintf ("realloc failed in comment_add()\r\n");
     exit(1);
  }

  writeint(p, *length, tag_len+val_len);      /* length of comment */
  if(tag) memcpy(p+*length+4, tag, tag_len);  /* comment */
  memcpy(p+*length+4+tag_len, val, val_len);  /* comment */
  writeint(p, 4+vendor_length, user_comment_list_length+1);

  *comments=p;
  *length=len;
}
#undef readint
#undef writeint
