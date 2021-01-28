/*
 * FAAC - Freeware Advanced Audio Coder
 * Copyright (C) 2001 Menno Bakker
 * Copyright (C) 2002-2004 Krzysztof Nikiel
 * Copyright (C) 2004 Dan Villiom P. Christiansen
 *
 * This library is vfree software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * $Id: main.c,v 1.82 2009/01/24 01:10:20 menno Exp $
 */

#ifdef _MSC_VER
# define HAVE_LIBMP4V2  1
#endif

//#ifdef HAVE_CONFIG_H
#include "faac_config.h"
//#endif
#include "vos.h"
#include "ff.h"

#ifdef HAVE_LIBMP4V2
# include <mp4.h>
#endif

#define DEFAULT_TNS     0

#ifdef _WIN32
#include <windows.h>
#include <fcntl.h>
#else
//#include <signal.h>
#endif

/* the BSD derivatives don't define __unix__ */
#if defined(__APPLE__) || defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__bsdi__)
#define __unix__
#endif

#ifdef __unix__
#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include <string.h>

#ifdef HAVE_GETOPT_H
# include <getopt.h>
#else
# include "getopt.h"
# include "getopt.c"
#endif

#if !defined(HAVE_STRCASECMP) && !defined(_WIN32)
# define strcasecmp strcmp
#endif

#ifdef _WIN32
# undef stderr
# define stderr stdout
#endif

#include "input.h"

#include <faac.h>

static const char *usage =
  "Usage: %s [options] [-o outfile] infiles ...\n"
  "\n"
  "\t<infiles> and/or <outfile> can be \"-\", which means stdin/stdout.\n"
  "\n"
  "See also:\n"
  "\t\"%s --help\" for short help on using FAAC\n"
  "\t\"%s --long-help\" for a description of all options for FAAC.\n"
  "\t\"%s --license\" for the license terms for FAAC.\n\n";

static const char *short_help =
  "Usage: %s [options] infiles ...\n"
  "Options:\n"
  "  -q <quality>\tSet quantizer quality.\n"
  "  -b <bitrate>\tSet average bitrate to x kbps. (ABR, lower quality mode)\n"
  "  -c <freq>\tSet the bandwidth in Hz. (default=automatic)\n"
  "  -o X\t\tSet output file to X (only for one input file)\n"
  "  -r\t\tUse RAW AAC output file.\n"
  "  -P\t\tRaw PCM input mode (default 44100Hz 16bit stereo).\n"
  "  -R\t\tRaw PCM input rate.\n"
  "  -B\t\tRaw PCM input sample size (8, 16 (default), 24 or 32bits).\n"
  "  -C\t\tRaw PCM input channels.\n"
  "  -X\t\tRaw PCM swap input bytes\n"
  "  -I <C,LF>\tInput channel config, default is 3,4 (Center third, LF fourth)\n"
  "\n"
  "MP4 specific options:\n"
#ifdef HAVE_LIBMP4V2
  "  -w\t\tWrap AAC data in MP4 container. (default for *.mp4 and *.m4a)\n"
  "  -s\t\tOptimize MP4 container layout after encoding\n"
  "  --artist X\tSet artist to X\n"
  "  --writer X\tSet writer to X\n"
  "  --title X\tSet title to X\n"
  "  --genre X\tSet genre to X\n"
  "  --album X\tSet album to X\n"
  "  --compilation\tSet compilation\n"
  "  --track X\tSet track to X (number/total)\n"
  "  --disc X\tSet disc to X (number/total)\n"
  "  --year X\tSet year to X\n"
  "  --cover-art X\tRead cover art from file X\n"
  "  --comment X\tSet comment to X\n"
#else
  "  MP4 support unavailable.\n"
#endif
  "\n"
  "Documentation:\n"
  "  --license\tShow the FAAC license.\n"
  "  --help\tShow this abbreviated help.\n"
  "  --long-help\tShow complete help.\n"
  "\n"
  "  More tips can be found in the audiocoding.com Knowledge Base at\n"
  "  <http://www.audiocoding.com/wiki/>\n"
  "\n";

static const char *long_help =
  "Usage: %s [options] infiles ...\n"
  "\n"
  "Quality-related options:\n"
  "  -q <quality>\tSet default variable bitrate (VBR) quantizer quality in percent.\n"
  "\t\t(default: 100, averages at approx. 120 kbps VBR for a normal\n"
  "\t\tstereo input file with 16 bit and 44.1 kHz sample rate; max.\n"
  "\t\tvalue 500, min. 10).\n"
  "  -b <bitrate>\tSet average bitrate (ABR) to approximately <bitrate> kbps.\n"
"\t\t(max. value 152 kbps/stereo with a 16 kHz cutoff, can be raised\n"
  "\t\twith a higher -c setting).\n"
  "  -c <freq>\tSet the bandwidth in Hz (default: automatic, i.e. adapts\n"
  "\t\tmaximum value to input sample rate).\n"
  "\n"
  "Input/output options:\n"
  "  -\t\t<stdin/stdout>: If you simply use a hyphen/minus sign instead\n"
  "\t\tof an input file name, FAAC can encode directly from stdin,\n"
  "\t\tthus enabling piping from other applications and utilities. The\n"
  "\t\tsame works for stdout as well, so FAAC can pipe its output to\n"
  "\t\tother apps such as a server.\n"
  "  -o X\t\tSet output file to X (only for one input file)\n"
  "\t\tonly for one input file; you can use *.aac, *.mp4, *.m4a or\n"
  "\t\t*.m4b as file extension, and the file format will be set\n"
  "\t\tautomatically to ADTS or MP4).\n"
  "  -P\t\tRaw PCM input mode (default: off, i.e. expecting a WAV header;\n"
  "\t\tnecessary for input files or bitstreams without a header; using\n"
  "\t\tonly -P assumes the default values for -R, -B and -C in the\n"
  "\t\tinput file).\n"
  "  -R\t\tRaw PCM input sample rate in Hz (default: 44100 Hz, max. 96 kHz)\n"
  "  -B\t\tRaw PCM input sample size (default: 16, also possible 8, 24, 32\n"
  "\t\tbit fixed or float input).\n"
  "  -C\t\tRaw PCM input channels (default: 2, max. 33 + 1 LFE).\n"
  "  -X\t\tRaw PCM swap input bytes (default: bigendian).\n"
  "  -I <C[,LFE]>\tInput multichannel configuration (default: 3,4 which means\n"
  "\t\tCenter is third and LFE is fourth like in 5.1 WAV, so you only\n"
  "\t\thave to specify a different position of these two mono channels\n"
  "\t\tin your multichannel input files if they haven't been reordered\n"
  "\t\talready).\n"
  "\n"
  "MP4 specific options:\n"
#ifdef HAVE_LIBMP4V2
  "  -w\t\tWrap AAC data in MP4 container. (default for *.mp4, *.m4a and\n"
  "\t\t*.m4b)\n"
  "  -s\t\tOptimize MP4 container layout after encoding.\n"
  "  --artist X\tSet artist to X\n"
  "  --writer X\tSet writer/composer to X\n"
  "  --title X\tSet title/track name to X\n"
  "  --genre X\tSet genre to X\n"
  "  --album X\tSet album/performer to X\n"
  "  --compilation\tMark as compilation\n"
  "  --track X\tSet track to X (number/total)\n"
  "  --disc X\tSet disc to X (number/total)\n"
  "  --year X\tSet year to X\n"
  "  --cover-art X\tRead cover art from file X\n"
  "\t\tSupported image formats are GIF, JPEG, and PNG.\n"
  "  --comment X\tSet comment to X\n"
#else
  "  MP4 support unavailable.\n"
#endif
  "\n"
  "Expert options, only for testing purposes:\n"
#if !DEFAULT_TNS
  "  --tns  \tEnable coding of TNS, temporal noise shaping.\n"
#else
  "  --no-tns\tDisable coding of TNS, temporal noise shaping.\n"
#endif
  "  --no-midside\tDon\'t use mid/side coding.\n"
  "  --mpeg-vers X\tForce AAC MPEG version, X can be 2 or 4\n"
  "  --obj-type X\tAAC object type. (LC (Low Complexity, default), Main or LTP\n"
  "\t\t(Long Term Prediction)\n"
  "  --shortctl X\tEnforce block type (0 = both (default); 1 = no short; 2 = no\n"
  "\t\tlong).\n"
  "  -r\t\tGenerate raw AAC bitstream (i.e. without any headers).\n"
  "\t\tNot advised!!!, RAW AAC files are practically useless!!!\n"
  "\n"
  "Documentation:\n"
  "  --license\tShow the FAAC license.\n"
  "  --help\tShow this abbreviated help.\n"
  "  --long-help\tShow complete help.\n"
  "\n"
  "  More tips can be found in the audiocoding.com Knowledge Base at\n"
  "  <http://www.audiocoding.com/wiki/>\n"
  "\n";

char *license =
  "\nPlease note that the use of this software may require the payment of patent\n"
  "royalties. You need to consider this issue before you start building derivative\n"
  "works. We are not warranting or indemnifying you in any way for patent\n"
  "royalities! YOU ARE SOLELY RESPONSIBLE FOR YOUR OWN ACTIONS!\n"
  "\n"
  "FAAC is based on the ISO MPEG-4 reference code. For this code base the\n"
  "following license applies:\n"
  "\n"
  "This software module was originally developed by\n"
  "\n"
  "FirstName LastName (CompanyName)\n"
  "\n"
  "and edited by\n"
  "\n"
  "FirstName LastName (CompanyName)\n"
  "FirstName LastName (CompanyName)\n"
  "\n"
  "in the course of development of the MPEG-2 NBC/MPEG-4 Audio standard\n"
  "ISO/IEC 13818-7, 14496-1,2 and 3. This software module is an\n"
  "implementation of a part of one or more MPEG-2 NBC/MPEG-4 Audio tools\n"
  "as specified by the MPEG-2 NBC/MPEG-4 Audio standard. ISO/IEC gives\n"
  "users of the MPEG-2 NBC/MPEG-4 Audio standards vfree license to this\n"
  "software module or modifications thereof for use in hardware or\n"
  "software products claiming conformance to the MPEG-2 NBC/ MPEG-4 Audio\n"
  "standards. Those intending to use this software module in hardware or\n"
  "software products are advised that this use may infringe existing\n"
  "patents. The original developer of this software module and his/her\n"
  "company, the subsequent editors and their companies, and ISO/IEC have\n"
  "no liability for use of this software module or modifications thereof\n"
  "in an implementation. Copyright is not released for non MPEG-2\n"
  "NBC/MPEG-4 Audio conforming products. The original developer retains\n"
  "full right to use the code for his/her own purpose, assign or donate\n"
  "the code to a third party and to inhibit third party from using the\n"
  "code for non MPEG-2 NBC/MPEG-4 Audio conforming products. This\n"
  "copyright notice must be included in all copies or derivative works.\n"
  "\n"
  "Copyright (c) 1997.\n"
  "\n"
  "For the changes made for the FAAC project the GNU Lesser General Public\n"
  "License (LGPL), version 2 1991 applies:\n"
  "\n"
  "FAAC - Freeware Advanced Audio Coder\n"
  "Copyright (C) 2001-2004 The individual contributors\n"
  "\n"
  "This library is vfree software; you can redistribute it and/or\n"
  "modify it under the terms of the GNU Lesser General Public\n"
  "License as published by the Free Software Foundation; either\n"
  "version 2.1 of the License, or (at your option) any later version.\n"
  "\n"
  "This library is distributed in the hope that it will be useful,\n"
  "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
  "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU\n"
  "Lesser General Public License for more details.\n"
  "\n"
  "You should have received a copy of the GNU Lesser General Public\n"
  "License along with this library; if not, write to the Free Software\n"
  "Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA\n"
  "\n";

#ifndef min
#define min(a,b) ( (a) < (b) ? (a) : (b) )
#endif

/* globals */
char* progName;
#ifndef _WIN32
volatile int running = 1;
#endif

enum stream_format {
  RAW_STREAM = 0,
  ADTS_STREAM = 1,
};

enum container_format {
  NO_CONTAINER,
#ifdef HAVE_LIBMP4V2
  MP4_CONTAINER,
#endif
};

enum flags {
  SHORTCTL_FLAG = 300,
  TNS_FLAG = 301,
  NO_TNS_FLAG = 302,
  MPEGVERS_FLAG = 303,
  OBJTYPE_FLAG = 304,
  NO_MIDSIDE_FLAG = 306,

#ifdef HAVE_LIBMP4V2
  ARTIST_FLAG = 320,
  TITLE_FLAG = 321,
  GENRE_FLAG = 322,
  ALBUM_FLAG = 323,
  COMPILATION_FLAG = 324,
  TRACK_FLAG = 325,
  DISC_FLAG = 326,
  YEAR_FLAG = 327,
  COVER_ART_FLAG = 328,
  COMMENT_FLAG = 329,
  WRITER_FLAG = 330,
#endif
};

#ifdef HAVE_LIBMP4V2
static int check_image_header(const char *buf) {
  if (!strncmp(buf, "\x89\x50\x4E\x47\x0D\x0A\x1A\x0A", 8))
    return 1; /* PNG */
  else if (!strncmp(buf, "\xFF\xD8\xFF\xE0", 4) ||
       !strncmp(buf, "\xFF\xD8\xFF\xE1", 4))
    return 1; /* JPEG */
  else if (!strncmp(buf, "GIF87a", 6) || !strncmp(buf, "GIF89a", 6))
    return 1; /* GIF */
  else
    return 0;
}
#endif

static int *mkChanMap(int channels, int center, int lf)
{
    int *map;
    int inpos;
    int outpos;

    if (!center && !lf)
        return NULL;

    if (channels < 3)
        return NULL;

    if (lf > 0)
        lf--;
    else
        lf = channels - 1; // default AAC position

    if (center > 0)
        center--;
    else
        center = 0; // default AAC position

    map = vmalloc(channels * sizeof(map[0]));
    memset(map, 0, channels * sizeof(map[0]));

    outpos = 0;
    if ((center >= 0) && (center < channels))
        map[outpos++] = center;

    inpos = 0;
    for (; outpos < (channels - 1); inpos++)
    {
        if (inpos == center)
            continue;
        if (inpos == lf)
            continue;

        map[outpos++] = inpos;
    }
    if (outpos < channels)
    {
        if ((lf >= 0) && (lf < channels))
            map[outpos] = lf;
        else
            map[outpos] = inpos;
    }

    return map;
}

int faac_main(int argc, char *argv[])
{
	FRESULT res;
	u32 num;
	FIL finput;
    int frames, currentFrame;
    faacEncHandle hEncoder;
    pcmfile_t *infile = NULL;

    unsigned long samplesInput, maxBytesOutput, totalBytesWritten=0;

    faacEncConfigurationPtr myFormat;
    unsigned int mpegVersion = MPEG2;
    unsigned int objectType = LOW;
    unsigned int useMidSide = 1;
    static unsigned int useTns = DEFAULT_TNS;
    enum container_format container = NO_CONTAINER;
    int optimizeFlag = 0;
    enum stream_format stream = ADTS_STREAM;
    int cutOff = -1;
    int bitRate = 0;
    unsigned long quantqual = 0;
    int chanC = 3;
    int chanLF = 4;

    char *audioFileName = NULL;
    char *aacFileName = NULL;
    char *aacFileExt = NULL;
    int aacFileNameGiven = 0;

    float *pcmbuf;
    int *chanmap = NULL;

    unsigned char *bitbuf;
    int samplesRead = 0;
    const char *dieMessage = NULL;

    int rawChans = 0; // disabled by default
    int rawBits = 16;
    int rawRate = 44100;
    int rawEndian = 1;

    int shortctl = SHORTCTL_NORMAL;

    FIL outfile;

#ifdef HAVE_LIBMP4V2
    MP4FileHandle MP4hFile = MP4_INVALID_FILE_HANDLE;
    MP4TrackId MP4track = 0;
    unsigned int ntracks = 0, trackno = 0;
    unsigned int ndiscs = 0, discno = 0;
    u_int8_t compilation = 0;
    const char *artist = NULL, *title = NULL, *album = NULL, *year = NULL,
      *genre = NULL, *comment = NULL, *writer = NULL;
    u_int8_t *art = NULL;
    u_int64_t artSize = 0;
    u_int64_t total_samples = 0;
    u_int64_t encoded_samples = 0;
    unsigned int delay_samples;
    unsigned int frameSize;
#endif
    char *faac_id_string;
    char *faac_copyright_string;


    // get faac version
    if (faacEncGetVersion(&faac_id_string, &faac_copyright_string) == FAAC_CFG_VERSION)
    {
        kprintf( "Freeware Advanced Audio Coder\nFAAC %s\n\n", faac_id_string);
    }
    else
    {
        kprintf( __FILE__ "(%d): wrong libfaac version\n", __LINE__);
        return 1;
    }

    /* begin process command line */
    progName = argv[0];
    while (1) {
        static struct option long_options[] = {
            { "help", 0, 0, 'h'},
            { "long-help", 0, 0, 'H'},
            { "raw", 0, 0, 'r'},
            { "no-midside", 0, 0, NO_MIDSIDE_FLAG},
            { "cutoff", 1, 0, 'c'},
            { "quality", 1, 0, 'q'},
            { "pcmraw", 0, 0, 'P'},
            { "pcmsamplerate", 1, 0, 'R'},
            { "pcmsamplebits", 1, 0, 'B'},
            { "pcmchannels", 1, 0, 'C'},
            { "shortctl", 1, 0, SHORTCTL_FLAG},
            { "tns", 0, 0, TNS_FLAG},
            { "no-tns", 0, 0, NO_TNS_FLAG},
            { "mpeg-version", 1, 0, MPEGVERS_FLAG},
            { "obj-type", 1, 0, OBJTYPE_FLAG},
            { "license", 0, 0, 'L'},
#ifdef HAVE_LIBMP4V2
            { "createmp4", 0, 0, 'w'},
            { "optimize", 0, 0, 's'},
            { "artist", 1, 0, ARTIST_FLAG},
            { "title", 1, 0, TITLE_FLAG},
            { "album", 1, 0, ALBUM_FLAG},
            { "track", 1, 0, TRACK_FLAG},
            { "disc", 1, 0, DISC_FLAG},
            { "genre", 1, 0, GENRE_FLAG},
            { "year", 1, 0, YEAR_FLAG},
            { "cover-art", 1, 0, COVER_ART_FLAG},
            { "comment", 1, 0, COMMENT_FLAG},
        { "writer", 1, 0, WRITER_FLAG},
        { "compilation", 0, 0, COMPILATION_FLAG},
#endif
        { "pcmswapbytes", 0, 0, 'X'},
            { 0, 0, 0, 0}
        };
        int c = -1;
        int option_index = 0;

        c = getopt_long(argc, argv, "Hhb:m:o:rnc:q:PR:B:C:I:X"
#ifdef HAVE_LIBMP4V2
                        "ws"
#endif
            ,long_options, &option_index);

        if (c == -1)
            break;

        if (!c)
        {
          dieMessage = usage;
          break;
        }

        switch (c) {
    case 'o':
        {
            int l = strlen(optarg);
        aacFileName = vmalloc(l+1);
        memcpy(aacFileName, optarg, l);
        aacFileName[l] = '\0';
        aacFileNameGiven = 1;
        }
        break;
        case 'r': {
            stream = RAW_STREAM;
            break;
        }
        case NO_MIDSIDE_FLAG: {
            useMidSide = 0;
            break;
        }
        case 'c': {
            unsigned int i;
            if (sscanf(optarg, "%u", &i) > 0) {
                cutOff = i;
            }
            break;
        }
        case 'b': {
            unsigned int i;
            if (sscanf(optarg, "%u", &i) > 0)
            {
                bitRate = 1000 * i;
            }
            break;
        }
        case 'q':
        {
            unsigned int i;
            if (sscanf(optarg, "%u", &i) > 0)
            {
                if (i > 0 && i < 1000)
                    quantqual = i;
            }
            break;
        }
        case 'I':
            sscanf(optarg, "%d,%d", &chanC, &chanLF);
            break;
        case 'P':
            rawChans = 2; // enable raw input
            break;
        case 'R':
        {
            unsigned int i;
            if (sscanf(optarg, "%u", &i) > 0)
            {
                rawRate = i;
                rawChans = (rawChans > 0) ? rawChans : 2;
            }
            break;
        }
        case 'B':
        {
            unsigned int i;
            if (sscanf(optarg, "%u", &i) > 0)
            {
                if (i > 32)
                    i = 32;
                if (i < 8)
                    i = 8;
                rawBits = i;
                rawChans = (rawChans > 0) ? rawChans : 2;
            }
            break;
        }
        case 'C':
        {
            unsigned int i;
            if (sscanf(optarg, "%u", &i) > 0)
                rawChans = i;
            break;
        }
#ifdef HAVE_LIBMP4V2
        case 'w':
        container = MP4_CONTAINER;
            break;
        case 's':
        optimizeFlag = 1;
            break;
    case ARTIST_FLAG:
        artist = optarg;
        break;
    case WRITER_FLAG:
        writer = optarg;
        break;
    case TITLE_FLAG:
        title = optarg;
        break;
    case ALBUM_FLAG:
        album = optarg;
        break;
    case TRACK_FLAG:
        sscanf(optarg, "%d/%d", &trackno, &ntracks);
        break;
    case DISC_FLAG:
        sscanf(optarg, "%d/%d", &discno, &ndiscs);
        break;
    case COMPILATION_FLAG:
        compilation = 0x1;
        break;
    case GENRE_FLAG:
        genre = optarg;
        break;
    case YEAR_FLAG:
        year = optarg;
        break;
    case COMMENT_FLAG:
        comment = optarg;
        break;
    case COVER_ART_FLAG: {
        //FILE *artFile = fopen(optarg, "rb");
    	FIL artFile;
        res = f_open(&artFile, optarg, FA_READ);
        if(res == 0) {
            u_int64_t r;

            f_lseek(&artFile, f_size(&artFile));
        artSize = f_tell(&artFile);

        art = vmalloc(artSize);

            f_lseek(&artFile, 0);
        //clearerr(artFile);

        //r = fread(art, artSize, 1, artFile);
    	r = 0;
        f_read (&artFile, art, artSize, &r);
        if (r != 1) {
            dieMessage = "Error reading cover art file!\n";
            vfree(art);
            art = NULL;
        } else if (artSize < 12 || !check_image_header(art)) {
            /* the above expression checks the image signature */
            dieMessage = "Unsupported cover image file format!\n";
            vfree(art);
            art = NULL;
        }

        f_close(&artFile);
        } else {
            dieMessage = "Error opening cover art file!\n";
        }

        break;
    }
#endif
        case SHORTCTL_FLAG:
            shortctl = atoi(optarg);
            break;
        case TNS_FLAG:
            useTns = 1;
            break;
        case NO_TNS_FLAG:
            useTns = 0;
            break;
    case MPEGVERS_FLAG:
            mpegVersion = atoi(optarg);
            switch(mpegVersion)
            {
            case 2:
                mpegVersion = MPEG2;
                break;
            case 4:
                mpegVersion = MPEG4;
                break;
            default:
            dieMessage = "Unrecognised MPEG version!\n";
            }
            break;
#if 0
    case OBJTYPE_FLAG:
        if (!strcasecmp(optarg, "LC"))
                objectType = LOW;
        else if (!strcasecmp(optarg, "Main"))
            objectType = MAIN;
        else if (!strcasecmp(optarg, "LTP")) {
            mpegVersion = MPEG4;
        objectType = LTP;
        } else
            dieMessage = "Unrecognised object type!\n";
        break;
#endif
        case 'L':
        kprintf( faac_copyright_string);
        dieMessage = license;
        break;
    case 'X':
      rawEndian = 0;
      break;
    case 'H':
      dieMessage = long_help;
      break;
    case 'h':
          dieMessage = short_help;
      break;
    case '?':
        default:
      dieMessage = usage;
          break;
        }
    }

    /* check that we have at least one non-option arguments */
    if (!dieMessage && (argc - optind) > 1 && aacFileNameGiven)
        dieMessage = "Cannot encode several input files to one output file.\n";

    if (argc - optind < 1 || dieMessage)
    {
        kprintf( dieMessage ? dieMessage : usage,
           progName, progName, progName, progName);
        return 1;
    }

    while (argc - optind > 0) {

    /* get the input file name */
    audioFileName = argv[optind++];

    /* generate the output file name, if necessary */
    if (!aacFileNameGiven) {
        char *t = strrchr(audioFileName, '.');
    int l = t ? strlen(audioFileName) - strlen(t) : strlen(audioFileName);

#ifdef HAVE_LIBMP4V2
    aacFileExt = container == MP4_CONTAINER ? ".m4a" : ".aac";
#else
    aacFileExt = ".aac";
#endif

    aacFileName = vmalloc(l+1+4);
    memcpy(aacFileName, audioFileName, l);
    memcpy(aacFileName + l, aacFileExt, 4);
    aacFileName[l+4] = '\0';
    } else {
        aacFileExt = strrchr(aacFileName, '.');

        if (aacFileExt && (!strcmp(".m4a", aacFileExt) || !strcmp(".m4b", aacFileExt) || !strcmp(".mp4", aacFileExt)))
#ifndef HAVE_LIBMP4V2
        kprintf( "WARNING: MP4 support unavailable!\n");
#else
        container = MP4_CONTAINER;
#endif
    }

    /* open the audio input file */
    if (rawChans > 0) // use raw input
    {
    	infile = wav_open_read(audioFileName, 1);
    if (1)
    {
        infile->bigendian = rawEndian;
        infile->channels = rawChans;
        infile->samplebytes = rawBits / 8;
        infile->samplerate = rawRate;
        infile->samples /= (infile->channels * infile->samplebytes);
    }
    }
    else { // header input
    	infile = wav_open_read(audioFileName, 0);
    }
    if (0)
    {
        kprintf( "Couldn't open input file %s\n", audioFileName);
    return 1;
    }


    /* open the encoder library */
    hEncoder = faacEncOpen(infile->samplerate, infile->channels,
        &samplesInput, &maxBytesOutput);

#ifdef HAVE_LIBMP4V2
    if (container != MP4_CONTAINER && (ntracks || trackno || artist ||
                       title ||  album || year || art ||
                       genre || comment || discno || ndiscs ||
                       writer || compilation))
    {
        kprintf( "Metadata requires MP4 output!\n");
    return 1;
    }

    if (container == MP4_CONTAINER)
    {
        mpegVersion = MPEG4;
    stream = RAW_STREAM;
    }

    frameSize = samplesInput/infile->channels;
    delay_samples = frameSize; // encoder delay 1024 samples
#endif
    pcmbuf = (float *)vmalloc(samplesInput*sizeof(float));
    bitbuf = (unsigned char*)vmalloc(maxBytesOutput*sizeof(unsigned char));
    chanmap = mkChanMap(infile->channels, chanC, chanLF);
    if (chanmap)
    {
        kprintf( "Remapping input channels: Center=%d, LFE=%d\n",
            chanC, chanLF);
    }

    if (cutOff <= 0)
    {
        if (cutOff < 0) // default
            cutOff = 0;
        else // disabled
            cutOff = infile->samplerate / 2;
    }
    if (cutOff > (infile->samplerate / 2))
        cutOff = infile->samplerate / 2;

    /* put the options in the configuration struct */
    myFormat = faacEncGetCurrentConfiguration(hEncoder);
    myFormat->aacObjectType = objectType;
    myFormat->mpegVersion = mpegVersion;
    myFormat->useTns = useTns;
    switch (shortctl)
    {
    case SHORTCTL_NOSHORT:
      kprintf( "disabling short blocks\n");
      myFormat->shortctl = shortctl;
      break;
    case SHORTCTL_NOLONG:
      kprintf( "disabling long blocks\n");
      myFormat->shortctl = shortctl;
      break;
    }
    if (infile->channels >= 6)
        myFormat->useLfe = 1;
    myFormat->allowMidside = useMidSide;
    if (bitRate)
        myFormat->bitRate = bitRate / infile->channels;
    myFormat->bandWidth = cutOff;
    if (quantqual > 0)
        myFormat->quantqual = quantqual;
    myFormat->outputFormat = stream;
    myFormat->inputFormat = FAAC_INPUT_FLOAT;
    if (!faacEncSetConfiguration(hEncoder, myFormat)) {
        kprintf( "Unsupported output format!\n");
#ifdef HAVE_LIBMP4V2
        if (container == MP4_CONTAINER) MP4Close(MP4hFile);
#endif
        return 1;
    }

#ifdef HAVE_LIBMP4V2
    /* initialize MP4 creation */
    if (container == MP4_CONTAINER) {
        unsigned char *ASC = 0;
        unsigned long ASCLength = 0;
    char *version_string;

#ifdef MP4_CREATE_EXTENSIBLE_FORMAT
    /* hack to compile against libmp4v2 >= 1.0RC3
     * why is there no version identifier in mp4.h? */
        MP4hFile = MP4Create(aacFileName, MP4_DETAILS_ERROR, 0);
#else
    MP4hFile = MP4Create(aacFileName, MP4_DETAILS_ERROR, 0, 0);
#endif
        if (!MP4_IS_VALID_FILE_HANDLE(MP4hFile)) {
            kprintf( "Couldn't create output file %s\n", aacFileName);
            return 1;
        }

        MP4SetTimeScale(MP4hFile, 90000);
        MP4track = MP4AddAudioTrack(MP4hFile, infile->samplerate, MP4_INVALID_DURATION, MP4_MPEG4_AUDIO_TYPE);
        MP4SetAudioProfileLevel(MP4hFile, 0x0F);
        faacEncGetDecoderSpecificInfo(hEncoder, &ASC, &ASCLength);
        MP4SetTrackESConfiguration(MP4hFile, MP4track, ASC, ASCLength);
    vfree(ASC);

    /* set metadata */
    version_string = vmalloc(strlen(faac_id_string) + 6);
    strcpy(version_string, "FAAC ");
    strcpy(version_string + 5, faac_id_string);
    MP4SetMetadataTool(MP4hFile, version_string);
    vfree(version_string);

    if (artist) MP4SetMetadataArtist(MP4hFile, artist);
    if (writer) MP4SetMetadataWriter(MP4hFile, writer);
    if (title) MP4SetMetadataName(MP4hFile, title);
    if (album) MP4SetMetadataAlbum(MP4hFile, album);
    if (trackno > 0) MP4SetMetadataTrack(MP4hFile, trackno, ntracks);
    if (discno > 0) MP4SetMetadataDisk(MP4hFile, discno, ndiscs);
    if (compilation) MP4SetMetadataCompilation(MP4hFile, compilation);
    if (year) MP4SetMetadataYear(MP4hFile, year);
    if (genre) MP4SetMetadataGenre(MP4hFile, genre);
    if (comment) MP4SetMetadataComment(MP4hFile, comment);
        if (artSize) {
        MP4SetMetadataCoverArt(MP4hFile, art, artSize);
        vfree(art);
    }
    }
    else
    {
#endif
        /* open the aac output file */
        if (!strcmp(aacFileName, "-"))
        {
            //outfile = stdout;
        }
        else
        {
            //outfile = fopen(aacFileName, "wb");
            res = f_open(&outfile, aacFileName, FA_WRITE | FA_CREATE_ALWAYS);
        }
        if (res)
        {
            kprintf( "Couldn't create output file %s\n", aacFileName);
            return 1;
        }
#ifdef HAVE_LIBMP4V2
    }
#endif

    cutOff = myFormat->bandWidth;
    quantqual = myFormat->quantqual;
    bitRate = myFormat->bitRate;
    if (bitRate)
      kprintf( "Average bitrate: %d kbps\n",
          (bitRate + 500)/1000*infile->channels);
    kprintf( "Quantization quality: %ld\n", quantqual);
    kprintf( "Bandwidth: %d Hz\n", cutOff);
    kprintf( "Object type: ");
    switch(objectType)
    {
    case LOW:
        kprintf( "Low Complexity");
        break;
    case MAIN:
        kprintf( "Main");
        break;
    case LTP:
        kprintf( "LTP");
        break;
    }
    kprintf( "(MPEG-%d)", (mpegVersion == MPEG4) ? 4 : 2);
    if (myFormat->useTns)
        kprintf( " + TNS");
    if (myFormat->allowMidside)
        kprintf( " + M/S");
    kprintf( "\n");

    kprintf( "Container format: ");
    switch(container)
    {
    case NO_CONTAINER:
      switch(stream)
    {
    case RAW_STREAM:
      kprintf( "Headerless AAC (RAW)\n");
      break;
    case ADTS_STREAM:
      kprintf( "Transport Stream (ADTS)\n");
      break;
    }
        break;
#ifdef HAVE_LIBMP4V2
    case MP4_CONTAINER:
        kprintf( "MPEG-4 File Format (MP4)\n");
        break;
#endif
    }

    if (&outfile
#ifdef HAVE_LIBMP4V2
        || MP4hFile != MP4_INVALID_FILE_HANDLE
#endif
       )
    {
        int showcnt = 0;
#ifdef _WIN32
        long begin = GetTickCount();
#endif
        if (infile->samples)
            frames = ((infile->samples + 1023) / 1024) + 1;
        else
            frames = 0;
        currentFrame = 0;

        kprintf( "Encoding %s to %s\n", audioFileName, aacFileName);
        if (frames != 0)
            kprintf( "   frame          | bitrate | elapsed/estim | "
            "play/CPU | ETA\n");
        else
            kprintf( " frame | elapsed | play/CPU\n");

        /* encoding loop */
#ifdef _WIN32
    for (;;)
#else
        while (running)
#endif
        {
            int bytesWritten;

            samplesRead = wav_read_float32(infile, pcmbuf, samplesInput, chanmap);

#ifdef HAVE_LIBMP4V2
            total_samples += samplesRead / infile->channels;
#endif

            /* call the actual encoding routine */
            bytesWritten = faacEncEncode(hEncoder,
                (int32_t *)pcmbuf,
                samplesRead,
                bitbuf,
                maxBytesOutput);

            if (bytesWritten)
            {
                currentFrame++;
                showcnt--;
        totalBytesWritten += bytesWritten;
            }

            if ((showcnt <= 0) || !bytesWritten)
            {
                double timeused;
#ifdef __unix__
                struct rusage usage;
#endif
#ifdef _WIN32
                char percent[MAX_PATH + 20];
                timeused = (GetTickCount() - begin) * 1e-3;
#else
#ifdef __unix__
                if (getrusage(RUSAGE_SELF, &usage) == 0) {
                    timeused = (double)usage.ru_utime.tv_sec +
                        (double)usage.ru_utime.tv_usec * 1e-6;
                }
                else
                    timeused = 0;
#else
                timeused = (double)(VOSGetTimeMs()/1000.0);
#endif
#endif
                if (currentFrame && (timeused > 0.1))
                {
                    showcnt += 50;

                    if (frames != 0)
                        kprintf(
                            "\r%5d/%-5d (%3d%%)|  %5.1f  | %6.1f/%-6.1f | %7.2fx | %.1f ",
                            currentFrame, frames, currentFrame*100/frames,
                ((double)totalBytesWritten * 8.0 / 1000.0) /
                ((double)infile->samples / infile->samplerate * currentFrame / frames),
                            timeused,
                            timeused * frames / currentFrame,
                            (1024.0 * currentFrame / infile->samplerate) / timeused,
                            timeused  * (frames - currentFrame) / currentFrame);
                    else
                        kprintf(
                            "\r %5d |  %6.1f | %7.2fx ",
                            currentFrame,
                            timeused,
                            (1024.0 * currentFrame / infile->samplerate) / timeused);

                    //fflush(stderr);
#ifdef _WIN32
                    if (frames != 0)
                    {
                        sprintf(percent, "%.2f%% encoding %s",
                            100.0 * currentFrame / frames, audioFileName);
                        SetConsoleTitle(percent);
                    }
#endif
                }
            }

            /* all done, bail out */
            if (!samplesRead && !bytesWritten)
                break ;

            if (bytesWritten < 0)
            {
                kprintf( "faacEncEncode() failed\n");
                break ;
            }

            if (bytesWritten > 0)
            {
#ifdef HAVE_LIBMP4V2
                u_int64_t samples_left = total_samples - encoded_samples + delay_samples;
                MP4Duration dur = samples_left > frameSize ? frameSize : samples_left;
                MP4Duration ofs = encoded_samples > 0 ? 0 : delay_samples;

                if (container == MP4_CONTAINER)
                {
                    /* write bitstream to mp4 file */
                    MP4WriteSample(MP4hFile, MP4track, bitbuf, bytesWritten, dur, ofs, 1);
                }
                else
                {
#endif
                    /* write bitstream to aac file */
                    //fwrite(bitbuf, 1, bytesWritten, outfile);
                    res = f_write (&outfile, bitbuf, bytesWritten, &num);
#ifdef HAVE_LIBMP4V2
                }

                encoded_samples += dur;
#endif
            }
        }

#ifdef HAVE_LIBMP4V2
        /* clean up */
        if (container == MP4_CONTAINER)
        {
            MP4Close(MP4hFile);
            if (optimizeFlag == 1)
            {
                kprintf( "\n\nMP4 format optimization... ");
                MP4Optimize(aacFileName, NULL, 0);
                kprintf( "Done!");
            }
        } else
#endif
            f_close(&outfile);

        kprintf( "\n\n");
    }

    faacEncClose(hEncoder);

    wav_close(infile);

    if (pcmbuf) vfree(pcmbuf);
    if (bitbuf) vfree(bitbuf);
    if (aacFileNameGiven) vfree(aacFileName);

    }

    return 0;
}
