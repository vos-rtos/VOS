/*
** FAAD2 - Freeware Advanced Audio (AAC) Decoder including SBR decoding
** Copyright (C) 2003-2005 M. Bakker, Nero AG, http://www.nero.com
**
** This program is vfree software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
**
** Any non-GPL usage of this software or parts of this software is strictly
** forbidden.
**
** The "appropriate copyright message" mentioned in section 2c of the GPLv2
** must read: "Code from FAAD2 is copyright (c) Nero AG, www.nero.com"
**
** Commercial non-GPL licensing of this software is possible.
** For more info contact Nero AG through Mpeg4AAClicense@nero.com.
**
** $Id: main.c,v 1.89 2015/01/19 09:46:12 knik Exp $
**/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <io.h>
#ifndef __MINGW32__
#define off_t __int64
#endif
#else
//#include <time.h>
#endif

#include "vos.h"
#include "ff.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#include <neaacdec.h>

#include "unicode_support.h"

#include "mp4read.h"
#include "audio.h"

#include "wm8978/audio.h"
#define AUDIO_WM8978_PORT 1


#ifdef HAVE_GETOPT_H
# include <getopt.h>
#else
# include "getopt.h"
# include "getopt.c"
#endif

#ifndef min
#define min(a,b) ( (a) < (b) ? (a) : (b) )
#endif

#define MAX_CHANNELS 2 /* make this higher to support files with
                          more channels */

#define MAX_PERCENTS 384

static int quiet = 0;

void x_ungetc(u8 ch, FIL *f);
/* FAAD file buffering routines */
typedef struct {
    long bytes_into_buffer;
    long bytes_consumed;
    long file_offset;
    unsigned char *buffer;
    int at_eof;
    FIL infile;
} aac_buffer;


static int fill_buffer(aac_buffer *b)
{
	u32 num;
	FRESULT res;
    int bread;

    if (b->bytes_consumed > 0)
    {
        if (b->bytes_into_buffer)
        {
            memmove((void*)b->buffer, (void*)(b->buffer + b->bytes_consumed),
                b->bytes_into_buffer*sizeof(unsigned char));
        }

        if (!b->at_eof)
        {
//            bread = fread((void*)(b->buffer + b->bytes_into_buffer), 1,
//                b->bytes_consumed, b->infile);
//            if (bread != b->bytes_consumed)
//                b->at_eof = 1;
            res = f_read (&b->infile, (void*)(b->buffer + b->bytes_into_buffer),
            		b->bytes_consumed, &bread);
            if (res == 0 && bread != b->bytes_consumed)
                b->at_eof = 1;

            b->bytes_into_buffer += bread;
        }

        b->bytes_consumed = 0;

        if (b->bytes_into_buffer > 3)
        {
            if (memcmp(b->buffer, "TAG", 3) == 0)
                b->bytes_into_buffer = 0;
        }
        if (b->bytes_into_buffer > 11)
        {
            if (memcmp(b->buffer, "LYRICSBEGIN", 11) == 0)
                b->bytes_into_buffer = 0;
        }
        if (b->bytes_into_buffer > 8)
        {
            if (memcmp(b->buffer, "APETAGEX", 8) == 0)
                b->bytes_into_buffer = 0;
        }
    }

    return 1;
}

static void advance_buffer(aac_buffer *b, int bytes)
{
    while ((b->bytes_into_buffer > 0) && (bytes > 0))
    {
        int chunk = min(bytes, b->bytes_into_buffer);

        bytes -= chunk;
        b->file_offset += chunk;
        b->bytes_consumed = chunk;
        b->bytes_into_buffer -= chunk;

        if (b->bytes_into_buffer == 0)
            fill_buffer(b);
    }
}

static void lookforheader(aac_buffer *b)
{
    int i = 0;
    while (!b->at_eof )
    {
        if (b->bytes_into_buffer > 4)
        {
            if( ((b->buffer[0+i] == 0xff) && ((b->buffer[1+i] & 0xf6) == 0xf0)) ||
                (b->buffer[0+i] == 'A'    && b->buffer[1+i] == 'D' && b->buffer[2+i] == 'I' && b->buffer[3+i] == 'F'))
            {
                fill_buffer(b);
                break;
            } else {
                i++;
                b->file_offset       += 1;
                b->bytes_consumed    += 1;
                b->bytes_into_buffer -= 1;
            }
        }
        else
        {
            fill_buffer(b);
            i = 0;
        }
    }
}

static int adts_sample_rates[] = {96000,88200,64000,48000,44100,32000,24000,22050,16000,12000,11025,8000,7350,0,0,0};

static int adts_parse(aac_buffer *b, int *bitrate, float *length)
{
    int frames, frame_length;
    int t_framelength = 0;
    int samplerate;
    float frames_per_sec, bytes_per_frame;

    /* Read all frames to ensure correct time and bitrate */
    for (frames = 0; /* */; frames++)
    {
        fill_buffer(b);

        if (b->bytes_into_buffer > 7)
        {
            /* check syncword */
            if (!((b->buffer[0] == 0xFF)&&((b->buffer[1] & 0xF6) == 0xF0)))
                break;

            if (frames == 0)
                samplerate = adts_sample_rates[(b->buffer[2]&0x3c)>>2];

            frame_length = ((((unsigned int)b->buffer[3] & 0x3)) << 11)
                | (((unsigned int)b->buffer[4]) << 3) | (b->buffer[5] >> 5);
            if (frame_length == 0)
                break;

            t_framelength += frame_length;

            if (frame_length > b->bytes_into_buffer)
                break;

            advance_buffer(b, frame_length);
        } else {
            break;
        }
    }

    frames_per_sec = (float)samplerate/1024.0f;
    if (frames != 0)
        bytes_per_frame = (float)t_framelength/(float)(frames*1000);
    else
        bytes_per_frame = 0;
    *bitrate = (int)(8. * bytes_per_frame * frames_per_sec + 0.5);
    if (frames_per_sec != 0)
        *length = (float)frames/frames_per_sec;
    else
        *length = 1;

    return 1;
}



uint32_t read_callback(void *user_data, void *buffer, uint32_t length)
{
	u32 num;
	FRESULT res;
//    return fread(buffer, 1, length, (FILE*)user_data);
    num = 0;
    res = f_read ((FIL*)user_data, buffer, length, &num);
    return num;
}

uint32_t seek_callback(void *user_data, uint64_t position)
{
//    return fseek((FILE*)user_data, position, SEEK_SET);
	return f_lseek((FIL*)user_data, position);
}

/* MicroSoft channel definitions */
#define SPEAKER_FRONT_LEFT             0x1
#define SPEAKER_FRONT_RIGHT            0x2
#define SPEAKER_FRONT_CENTER           0x4
#define SPEAKER_LOW_FREQUENCY          0x8
#define SPEAKER_BACK_LEFT              0x10
#define SPEAKER_BACK_RIGHT             0x20
#define SPEAKER_FRONT_LEFT_OF_CENTER   0x40
#define SPEAKER_FRONT_RIGHT_OF_CENTER  0x80
#define SPEAKER_BACK_CENTER            0x100
#define SPEAKER_SIDE_LEFT              0x200
#define SPEAKER_SIDE_RIGHT             0x400
#define SPEAKER_TOP_CENTER             0x800
#define SPEAKER_TOP_FRONT_LEFT         0x1000
#define SPEAKER_TOP_FRONT_CENTER       0x2000
#define SPEAKER_TOP_FRONT_RIGHT        0x4000
#define SPEAKER_TOP_BACK_LEFT          0x8000
#define SPEAKER_TOP_BACK_CENTER        0x10000
#define SPEAKER_TOP_BACK_RIGHT         0x20000
#define SPEAKER_RESERVED               0x80000000

static long aacChannelConfig2wavexChannelMask(NeAACDecFrameInfo *hInfo)
{
    if (hInfo->channels == 6 && hInfo->num_lfe_channels)
    {
        return SPEAKER_FRONT_LEFT + SPEAKER_FRONT_RIGHT +
            SPEAKER_FRONT_CENTER + SPEAKER_LOW_FREQUENCY +
            SPEAKER_BACK_LEFT + SPEAKER_BACK_RIGHT;
    } else {
        return 0;
    }
}

static char *position2string(int position)
{
    switch (position)
    {
    case FRONT_CHANNEL_CENTER: return "Center front";
    case FRONT_CHANNEL_LEFT:   return "Left front";
    case FRONT_CHANNEL_RIGHT:  return "Right front";
    case SIDE_CHANNEL_LEFT:    return "Left side";
    case SIDE_CHANNEL_RIGHT:   return "Right side";
    case BACK_CHANNEL_LEFT:    return "Left back";
    case BACK_CHANNEL_RIGHT:   return "Right back";
    case BACK_CHANNEL_CENTER:  return "Center back";
    case LFE_CHANNEL:          return "LFE";
    case UNKNOWN_CHANNEL:      return "Unknown";
    default: return "";
    }

    return "";
}

static void print_channel_info(NeAACDecFrameInfo *frameInfo)
{
    /* print some channel info */
    int i;
    long channelMask = aacChannelConfig2wavexChannelMask(frameInfo);

    kprintf("  ---------------------\r\n");
    if (frameInfo->num_lfe_channels > 0)
    {
        kprintf(" | Config: %2d.%d Ch     |", frameInfo->channels-frameInfo->num_lfe_channels, frameInfo->num_lfe_channels);
    } else {
        kprintf(" | Config: %2d Ch       |", frameInfo->channels);
    }
    if (channelMask)
        kprintf(" WARNING: channels are reordered according to\r\n");
    else
        kprintf("\r\n");
    kprintf("  ---------------------");
    if (channelMask)
        kprintf("  MS defaults defined in WAVE_FORMAT_EXTENSIBLE\r\n");
    else
        kprintf("\r\n");
    kprintf(" | Ch |    Position    |\r\n");
    kprintf("  ---------------------\r\n");
    for (i = 0; i < frameInfo->channels; i++)
    {
        kprintf(" | %.2d | %-14s |\r\n", i, position2string((int)frameInfo->channel_position[i]));
    }
    kprintf("  ---------------------\r\n");
    kprintf("\r\n");
}

static int FindAdtsSRIndex(int sr)
{
    int i;

    for (i = 0; i < 16; i++)
    {
        if (sr == adts_sample_rates[i])
            return i;
    }
    return 16 - 1;
}

static unsigned char *MakeAdtsHeader(int *dataSize, NeAACDecFrameInfo *hInfo, int old_format)
{
    unsigned char *data;
    int profile = (hInfo->object_type - 1) & 0x3;
    int sr_index = ((hInfo->sbr == SBR_UPSAMPLED) || (hInfo->sbr == NO_SBR_UPSAMPLED)) ?
        FindAdtsSRIndex(hInfo->samplerate / 2) : FindAdtsSRIndex(hInfo->samplerate);
    int skip = (old_format) ? 8 : 7;
    int framesize = skip + hInfo->bytesconsumed;

    if (hInfo->header_type == ADTS)
        framesize -= skip;

    *dataSize = 7;

    data = vmalloc(*dataSize * sizeof(unsigned char));
    memset(data, 0, *dataSize * sizeof(unsigned char));

    data[0] += 0xFF; /* 8b: syncword */

    data[1] += 0xF0; /* 4b: syncword */
    /* 1b: mpeg id = 0 */
    /* 2b: layer = 0 */
    data[1] += 1; /* 1b: protection absent */

    data[2] += ((profile << 6) & 0xC0); /* 2b: profile */
    data[2] += ((sr_index << 2) & 0x3C); /* 4b: sampling_frequency_index */
    /* 1b: private = 0 */
    data[2] += ((hInfo->channels >> 2) & 0x1); /* 1b: channel_configuration */

    data[3] += ((hInfo->channels << 6) & 0xC0); /* 2b: channel_configuration */
    /* 1b: original */
    /* 1b: home */
    /* 1b: copyright_id */
    /* 1b: copyright_id_start */
    data[3] += ((framesize >> 11) & 0x3); /* 2b: aac_frame_length */

    data[4] += ((framesize >> 3) & 0xFF); /* 8b: aac_frame_length */

    data[5] += ((framesize << 5) & 0xE0); /* 3b: aac_frame_length */
    data[5] += ((0x7FF >> 6) & 0x1F); /* 5b: adts_buffer_fullness */

    data[6] += ((0x7FF << 2) & 0x3F); /* 6b: adts_buffer_fullness */
    /* 2b: num_raw_data_blocks */

    return data;
}

/* globals */
char *progName;

static const char *file_ext[] =
{
    NULL,
    ".wav",
    ".aif",
    ".au",
    ".au",
    ".pcm",
    NULL
};

static void usage(void)
{
    kprintf("\nUsage:\r\n");
    kprintf("%s [options] infile.aac\r\n", progName);
    kprintf("Options:\r\n");
    kprintf(" -h    Shows this help screen.\r\n");
    kprintf(" -i    Shows info about the input file.\r\n");
    kprintf(" -a X  Write MPEG-4 AAC ADTS output file.\r\n");
    kprintf(" -t    Assume old ADTS format.\r\n");
    kprintf(" -o X  Set output filename.\r\n");
    kprintf(" -f X  Set output format. Valid values for X are:\r\n");
    kprintf("        1:  Microsoft WAV format (default).\r\n");
    kprintf("        2:  RAW PCM data.\r\n");
    kprintf(" -b X  Set output sample format. Valid values for X are:\r\n");
    kprintf("        1:  16 bit PCM data (default).\r\n");
    kprintf("        2:  24 bit PCM data.\r\n");
    kprintf("        3:  32 bit PCM data.\r\n");
    kprintf("        4:  32 bit floating point data.\r\n");
    kprintf("        5:  64 bit floating point data.\r\n");
    kprintf(" -s X  Force the samplerate to X (for RAW files).\r\n");
    kprintf(" -l X  Set object type. Supported object types:\r\n");
    kprintf("        1:  Main object type.\r\n");
    kprintf("        2:  LC (Low Complexity) object type.\r\n");
    kprintf("        4:  LTP (Long Term Prediction) object type.\r\n");
    kprintf("        23: LD (Low Delay) object type.\r\n");
    kprintf(" -d    Down matrix 5.1 to 2 channels\r\n");
    kprintf(" -w    Write output to stdio instead of a file.\r\n");
    kprintf(" -g    Disable gapless decoding.\r\n");
    kprintf(" -q    Quiet - suppresses status messages.\r\n");
    kprintf(" -j X  Jump - start output X seconds into track (MP4 files only).\r\n");
    kprintf("Example:\r\n");
    kprintf("       %s infile.aac\r\n", progName);
    kprintf("       %s infile.mp4\r\n", progName);
    kprintf("       %s -o outfile.wav infile.aac\r\n", progName);
    kprintf("       %s -w infile.aac > outfile.wav\r\n", progName);
    kprintf("       %s -a outfile.aac infile.aac\r\n", progName);
    return;
}

static int my_audio_16bit(void *sample_buffer,
                             unsigned int samples)
{
    int ret;
    unsigned int i;
    short *sample_buffer16 = (short*)sample_buffer;
    char *data = vmalloc(samples*16*sizeof(char)/8);

    for (i = 0; i < samples; i++)
    {
        data[i*2] = (char)(sample_buffer16[i] & 0xFF);
        data[i*2+1] = (char)((sample_buffer16[i] >> 8) & 0xFF);
    }

	int mark = 0;
	s32 sended = 0;
	s32 totals = samples*(16/8);
	while (1) {
		sended = audio_sends(AUDIO_WM8978_PORT, &data[mark], totals-mark, 100);
		if (sended > 0) {
			mark += sended;
		}
		if (sended == 0) {
			VOSTaskDelay(5);
		}
		if (mark  ==  totals) {
			break;
		}
	}
    if (data) vfree(data);

    return ret;
}

static int decodeAACfile(char *aacfile, char *sndfile, char *adts_fn, int to_stdout,
                  int def_srate, int object_type, int outputFormat, int fileType,
                  int downMatrix, int infoOnly, int adts_out, int old_format,
                  float *song_length)
{
	u32 num;
	FRESULT res;
    int tagsize;
    unsigned long samplerate;
    unsigned char channels;
    void *sample_buffer;

    audio_file *aufile = NULL;

    FIL adtsFile;
    unsigned char *adtsData;
    int adtsDataSize;

    NeAACDecHandle hDecoder;
    NeAACDecFrameInfo frameInfo;
    NeAACDecConfigurationPtr config;

    char percents[MAX_PERCENTS];
    int percent, old_percent = -1;
    int bread, fileread;
    int header_type = 0;
    int bitrate = 0;
    float length = 0;

    int first_time = 1;
    int retval;
    int streaminput = 0;

    aac_buffer b;

    memset(&b, 0, sizeof(aac_buffer));

    if (adts_out)
    {
        //adtsFile = faad_fopen(adts_fn, "wb");
        res = f_open(&adtsFile, adts_fn, FA_WRITE | FA_CREATE_ALWAYS);
        if (res)
        {
            kprintf("Error opening file: %s\r\n", adts_fn);
            return 1;
        }
    }

	//b.infile = faad_fopen(aacfile, "rb");
	res = f_open(&b.infile, aacfile, FA_READ);
	if (res)
	{
		/* unable to open file */
		kprintf("Error opening file: %s\r\n", aacfile);
		return 1;
	}
    //retval = fseek(b.infile, 0, SEEK_END);
    retval = f_lseek(&b.infile, b.infile.obj.objsize);

    if (retval )
    {
        kprintf("Input not seekable %s\r\n", aacfile);
        fileread = -1;
        streaminput = 1;
    } else {
//        fileread = ftell(b.infile);
    	fileread = f_tell(&b.infile);
//        fseek(b.infile, 0, SEEK_SET);
        f_lseek(&b.infile, 0);
    };

    if (!(b.buffer = (unsigned char*)vmalloc(FAAD_MIN_STREAMSIZE*MAX_CHANNELS)))
    {
        kprintf("Memory allocation error\r\n");
        return 0;
    }
    memset(b.buffer, 0, FAAD_MIN_STREAMSIZE*MAX_CHANNELS);

//    bread = fread(b.buffer, 1, FAAD_MIN_STREAMSIZE*MAX_CHANNELS, b.infile);
    bread = 0;
    res = f_read (&b.infile, b.buffer, FAAD_MIN_STREAMSIZE*MAX_CHANNELS, &bread);

    b.bytes_into_buffer = bread;
    b.bytes_consumed = 0;
    b.file_offset = 0;

    if (bread != FAAD_MIN_STREAMSIZE*MAX_CHANNELS)
        b.at_eof = 1;

    tagsize = 0;
    if (!memcmp(b.buffer, "ID3", 3))
    {
        /* high bit is not used */
        tagsize = (b.buffer[6] << 21) | (b.buffer[7] << 14) |
            (b.buffer[8] <<  7) | (b.buffer[9] <<  0);

        tagsize += 10;
        advance_buffer(&b, tagsize);
        fill_buffer(&b);
    }

    hDecoder = NeAACDecOpen();

    /* Set the default object type and samplerate */
    /* This is useful for RAW AAC files */
    config = NeAACDecGetCurrentConfiguration(hDecoder);
    if (def_srate)
        config->defSampleRate = def_srate;
    config->defObjectType = object_type;
    config->outputFormat = outputFormat;
    config->downMatrix = downMatrix;
    config->useOldADTSFormat = old_format;
    //config->dontUpSampleImplicitSBR = 1;
    NeAACDecSetConfiguration(hDecoder, config);

    /* get AAC infos for printing */
    header_type = 0;
    if (streaminput == 1)
        lookforheader(&b);

    if ((b.buffer[0] == 0xFF) && ((b.buffer[1] & 0xF6) == 0xF0))
    {
        if (streaminput == 1)
        {
            int /*frames,*/ frame_length;
            int samplerate;
            float frames_per_sec, bytes_per_frame;
            channels = 2;
            samplerate = adts_sample_rates[(b.buffer[2]&0x3c)>>2];
            frame_length = ((((unsigned int)b.buffer[3] & 0x3)) << 11)
                | (((unsigned int)b.buffer[4]) << 3) | (b.buffer[5] >> 5);
            frames_per_sec = (float)samplerate/1024.0f;
            bytes_per_frame = (float)frame_length/(float)(1000);
            bitrate = (int)(8. * bytes_per_frame * frames_per_sec + 0.5);
            length = 1;
            kprintf("Streamed input format  samplerate %d channels %d.\r\n", samplerate, channels);
        } else {
            adts_parse(&b, &bitrate, &length);
//            fseek(b.infile, tagsize, SEEK_SET);
            f_lseek(&b.infile, tagsize);
            //bread = fread(b.buffer, 1, FAAD_MIN_STREAMSIZE*MAX_CHANNELS, b.infile);
            bread = 0;
            res = f_read (&b.infile, b.buffer, FAAD_MIN_STREAMSIZE*MAX_CHANNELS, &bread);
            if (bread != FAAD_MIN_STREAMSIZE*MAX_CHANNELS)
                b.at_eof = 1;
            else
                b.at_eof = 0;
            b.bytes_into_buffer = bread;
            b.bytes_consumed = 0;
            b.file_offset = tagsize;
        }

        header_type = 1;
    }
    else if (memcmp(b.buffer, "ADIF", 4) == 0)
    {
        int skip_size = (b.buffer[4] & 0x80) ? 9 : 0;
        bitrate = ((unsigned int)(b.buffer[4 + skip_size] & 0x0F)<<19) |
            ((unsigned int)b.buffer[5 + skip_size]<<11) |
            ((unsigned int)b.buffer[6 + skip_size]<<3) |
            ((unsigned int)b.buffer[7 + skip_size] & 0xE0);

        length = (float)fileread;
        if (length != 0)
        {
            length = ((float)length*8.f)/((float)bitrate) + 0.5f;
        }

        bitrate = (int)((float)bitrate/1000.0f + 0.5f);

        header_type = 2;
    }

    *song_length = length;

    fill_buffer(&b);
    if ((bread = NeAACDecInit(hDecoder, b.buffer,
        b.bytes_into_buffer, &samplerate, &channels)) < 0)
    {
        /* If some error initializing occured, skip the file */
        kprintf("Error initializing decoder library.\r\n");
        if (b.buffer)
            vfree(b.buffer);
        NeAACDecClose(hDecoder);
        //if (b.infile != stdin)
            f_close(&b.infile);
        return 1;
    }
    advance_buffer(&b, bread);
    fill_buffer(&b);

    /* print AAC file info */
    kprintf("%s file info:\r\n", aacfile);
    switch (header_type)
    {
    case 0:
        kprintf("RAW\r\n");
        break;
    case 1:
        kprintf("ADTS, %.3f sec, %d kbps, %d Hz\r\n",
            length, bitrate, samplerate);
        break;
    case 2:
        kprintf("ADIF, %.3f sec, %d kbps, %d Hz\r\n",
            length, bitrate, samplerate);
        break;
    }

    if (infoOnly)
    {
        NeAACDecClose(hDecoder);
        //if (b.infile != stdin)
        f_close(&b.infile);
        if (b.buffer)
            vfree(b.buffer);
        return 0;
    }
    u32 time_mark =  VOSGetTimeMs();
    s32 ret = audio_open(AUDIO_WM8978_PORT, AUDIO_ADC_16BIT);
    s32 sam = 16000;
    audio_ctrl(AUDIO_WM8978_PORT, AUDIO_OPT_AUDIO_SAMPLE, &sam, 4);

    kprintf("aac decoder start ...\r\n");
    do
    {
        sample_buffer = NeAACDecDecode(hDecoder, &frameInfo,
            b.buffer, b.bytes_into_buffer);

        if (adts_out == 1)
        {
            int skip = (old_format) ? 8 : 7;
            adtsData = MakeAdtsHeader(&adtsDataSize, &frameInfo, old_format);

            /* write the adts header */
            //fwrite(adtsData, 1, adtsDataSize, adtsFile);
            num = 0;
            res = f_write (&adtsFile, adtsData, adtsDataSize, &num);
            /* write the frame data */
            if (frameInfo.header_type == ADTS) {
                //fwrite(b.buffer + skip, 1, frameInfo.bytesconsumed - skip, adtsFile);
            	res = f_write (&adtsFile, b.buffer + skip, frameInfo.bytesconsumed - skip, &num);
            } else {
                //fwrite(b.buffer, 1, frameInfo.bytesconsumed, adtsFile);
            	res = f_write (&adtsFile, b.buffer, frameInfo.bytesconsumed, &num);
            }
        }

        /* update buffer indices */
        advance_buffer(&b, frameInfo.bytesconsumed);

        /* check if the inconsistent number of channels */
        if (aufile != NULL && frameInfo.channels != aufile->channels)
            frameInfo.error = 12;

        if (frameInfo.error > 0)
        {
            kprintf("Error: %s\r\n",
                NeAACDecGetErrorMessage(frameInfo.error));
        }

        /* open the sound file now that the number of channels are known */
        if (first_time && !frameInfo.error)
        {
            /* print some channel info */
            print_channel_info(&frameInfo);

            if (!adts_out)
            {
                /* open output file */
                if (!to_stdout)
                {
                    aufile = open_audio_file(sndfile, frameInfo.samplerate, frameInfo.channels,
                        outputFormat, fileType, aacChannelConfig2wavexChannelMask(&frameInfo));
                }
                if (aufile == NULL)
                {
                    if (b.buffer)
                        vfree(b.buffer);
                    NeAACDecClose(hDecoder);
                    //if (b.infile != stdin)
                        f_close(&b.infile);
                    return 0;
                }
            } else {
                kprintf("Writing output MPEG-4 AAC ADTS file.\r\n");
            }
            first_time = 0;
        }

        percent = min((int)(b.file_offset*100)/fileread, 100);
        if (percent > old_percent)
        {
            old_percent = percent;
            rpl_snprintf(percents, MAX_PERCENTS, "%d%% decoding %s.", percent, aacfile);
            kprintf("%s\r\n", percents);
        }
#if 0
        if ((frameInfo.error == 0) && (frameInfo.samples > 0) && (!adts_out))
        {
            if (write_audio_file(aufile, sample_buffer, frameInfo.samples, 0) == 0)
                break;
        }
#else//write_audio_16bit(aufile, buf + offset*2, samples);
        if ((frameInfo.error == 0) && (frameInfo.samples > 0) && (!adts_out)){
        	my_audio_16bit(sample_buffer, frameInfo.samples);
        }
#endif
        /* fill buffer */
        fill_buffer(&b);

        if (b.bytes_into_buffer == 0)
            sample_buffer = NULL; /* to make sure it stops now */

    } while (sample_buffer != NULL);

    NeAACDecClose(hDecoder);
    kprintf("spends (%d ms)!\r\n;", (u32)(VOSGetTimeMs()-time_mark));
    if (adts_out == 1)
    {
        f_close(&adtsFile);
    }

        f_close(&b.infile);

    if (!first_time)// && !adts_out)
        close_audio_file(aufile);

    if (b.buffer)
        vfree(b.buffer);

    return frameInfo.error;
}

static const unsigned long srates[] =
{
    96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 16000,
    12000, 11025, 8000
};



static int decodeMP4file(char *mp4file, char *sndfile, char *adts_fn, int to_stdout,
                  int outputFormat, int fileType, int downMatrix, int noGapless,
                  int infoOnly, int adts_out, float *song_length, float seek_to)
{
	u32 num;
	FRESULT res;
    /*int track;*/
    unsigned long samplerate;
    unsigned char channels;
    void *sample_buffer;

    long sampleId, startSampleId;

    audio_file *aufile = NULL;

    FIL adtsFile;
    unsigned char *adtsData;
    int adtsDataSize;

    NeAACDecHandle hDecoder;
    NeAACDecConfigurationPtr config;
    NeAACDecFrameInfo frameInfo;
    mp4AudioSpecificConfig mp4ASC;

    char percents[MAX_PERCENTS];
    int percent, old_percent = -1;

    int first_time = 1;

    /* for gapless decoding */
    unsigned int useAacLength = 1;
    unsigned int framesize;
    unsigned decoded;


    if (!quiet)
    {
        mp4config.verbose.header = 1;
        mp4config.verbose.tags = 1;
    }
    if (mp4read_open(mp4file))
    {
        /* unable to open file */
        kprintf("Error opening file: %s\r\n", mp4file);
        return 1;
    }

    hDecoder = NeAACDecOpen();

    /* Set configuration */
    config = NeAACDecGetCurrentConfiguration(hDecoder);
    config->outputFormat = outputFormat;
    config->downMatrix = downMatrix;
    //config->dontUpSampleImplicitSBR = 1;
    NeAACDecSetConfiguration(hDecoder, config);

    if (adts_out)
    {
        //adtsFile = faad_fopen(adts_fn, "wb");
        res = f_open(&adtsFile, adts_fn, FA_WRITE | FA_CREATE_ALWAYS);
        if (res)
        {
            kprintf("Error opening file: %s\r\n", adts_fn);
            return 1;
        }
    }

    if(NeAACDecInit2(hDecoder, mp4config.asc.buf, mp4config.asc.size,
                    &samplerate, &channels) < 0)
    {
        /* If some error initializing occured, skip the file */
        kprintf("Error initializing decoder library.\r\n");
        NeAACDecClose(hDecoder);
        mp4read_close();
        return 1;
    }

    framesize = 1024;
    useAacLength = 0;
    decoded = 0;

    if (mp4config.asc.size)
    {
        if (NeAACDecAudioSpecificConfig(mp4config.asc.buf, mp4config.asc.size, &mp4ASC) >= 0)
        {
            if (mp4ASC.frameLengthFlag == 1) framesize = 960;
            if (mp4ASC.sbr_present_flag == 1 || mp4ASC.forceUpSampling) framesize *= 2;
        }
    }

    /* print some mp4 file info */
    kprintf("%s file info:\r\n", mp4file);
    {
        char *tag = NULL, *item = NULL;
        /*int k, j;*/
        char *ot[6] = { "NULL", "MAIN AAC", "LC AAC", "SSR AAC", "LTP AAC", "HE AAC" };
        float seconds;
        seconds = (float)mp4config.samples/(float)mp4ASC.samplingFrequency;

        *song_length = seconds;

        kprintf("%s\t%.3f secs, %d ch, %d Hz\r\n", ot[(mp4ASC.objectTypeIndex > 5)?0:mp4ASC.objectTypeIndex],
            seconds, mp4ASC.channelsConfiguration, mp4ASC.samplingFrequency);
    }

    if (infoOnly)
    {
        NeAACDecClose(hDecoder);
        mp4read_close();
        return 0;
    }

    startSampleId = 0;
    if (seek_to > 0.1)
        startSampleId = (int64_t)(seek_to * mp4config.samplerate / framesize);

    mp4read_seek(startSampleId);
    for (sampleId = startSampleId; sampleId < mp4config.frame.ents; sampleId++)
    {
        /*int rc;*/
        long dur;
        unsigned int sample_count;
        unsigned int delay = 0;

        if (mp4read_frame())
            break;

        sample_buffer = NeAACDecDecode(hDecoder, &frameInfo, mp4config.bitbuf.data, mp4config.bitbuf.size);

        if (!sample_buffer) {
            /* unable to decode file, abort */
            break;
        }

        if (adts_out == 1)
        {
            adtsData = MakeAdtsHeader(&adtsDataSize, &frameInfo, 0);

            /* write the adts header */
            //fwrite(adtsData, 1, adtsDataSize, adtsFile);
            res = f_write (&adtsFile, adtsData, adtsDataSize, &num);
            //fwrite(mp4config.bitbuf.data, 1, frameInfo.bytesconsumed, adtsFile);
            res = f_write (&adtsFile, mp4config.bitbuf.data, frameInfo.bytesconsumed, &num);
        }

        dur = frameInfo.samples / frameInfo.channels;
        decoded += dur;

        if (decoded > mp4config.samples)
            dur += mp4config.samples - decoded;

        if (dur > framesize)
        {
            kprintf("Warning: excess frame detected in MP4 file.\r\n");
            dur = framesize;
        }

        if (!noGapless)
        {
            if (useAacLength || (mp4config.samplerate != samplerate)) {
                sample_count = frameInfo.samples;
            } else {
                sample_count = (unsigned int)(dur * frameInfo.channels);
                if (sample_count > frameInfo.samples)
                    sample_count = frameInfo.samples;
            }
        } else {
            sample_count = frameInfo.samples;
        }

        /* open the sound file now that the number of channels are known */
        if (first_time && !frameInfo.error)
        {
            /* print some channel info */
            print_channel_info(&frameInfo);

            if (!adts_out)
            {
                /* open output file */

				aufile = open_audio_file(sndfile, frameInfo.samplerate, frameInfo.channels,
					outputFormat, fileType, aacChannelConfig2wavexChannelMask(&frameInfo));

                if (aufile == NULL)
                {
                    NeAACDecClose(hDecoder);
                    mp4read_close();
                    return 0;
                }
            }
            first_time = 0;
        }

        percent = min((int)(sampleId*100)/mp4config.frame.ents, 100);
        if (percent > old_percent)
        {
            old_percent = percent;
            snprintf(percents, MAX_PERCENTS, "%d%% decoding %s.", percent, mp4file);
            kprintf("%s\r\n", percents);
        }

        if ((frameInfo.error == 0) && (sample_count > 0) && (!adts_out))
        {
            if (write_audio_file(aufile, sample_buffer, sample_count, delay) == 0)
                break;
        }

        if (frameInfo.error > 0)
        {
            kprintf("Warning: %s\r\n",
                NeAACDecGetErrorMessage(frameInfo.error));
        }
    }

    NeAACDecClose(hDecoder);

    //if (adts_out == 1)
    {
        f_close(&adtsFile);
    }

    mp4read_close();

    if (!first_time && !adts_out)
        close_audio_file(aufile);

    return frameInfo.error;
}

void x_ungetc(u8 ch, FIL *f)
{
	u32 num;
	FRESULT res;
	res = f_write (f, &ch, 1, &num);
}

int faad_main(int argc, char *argv[])
{
	u32 num;
	FRESULT res;
    int result;
    int infoOnly = 0;
    int writeToStdio = 0;
    int readFromStdin = 0;
    int object_type = LC;
    int def_srate = 0;
    int downMatrix = 0;
    int format = 1;
    int outputFormat = FAAD_FMT_16BIT;
    int outfile_set = 0;
    int adts_out = 0;
    int old_format = 0;
    int showHelp = 0;
    int mp4file = 0;
    int noGapless = 0;
    char *fnp;
    char *aacFileName = NULL;
    char *audioFileName = NULL;
    char *adtsFileName = NULL;
    float seekTo = 0;
    unsigned char header[8];
    int bread;
    float length = 0;
    FIL hMP4File;
    char *faad_id_string;
    char *faad_copyright_string;
    u32 begin;

    unsigned long cap = NeAACDecGetCapabilities();

    /* begin process command line */
    progName = argv[0];
    while (1) {
        int c = -1;
        int option_index = 0;
        static struct option long_options[] = {
            { "quiet",      0, 0, 'q' },
            { "outfile",    0, 0, 'o' },
            { "adtsout",    0, 0, 'a' },
            { "oldformat",  0, 0, 't' },
            { "format",     0, 0, 'f' },
            { "bits",       0, 0, 'b' },
            { "samplerate", 0, 0, 's' },
            { "objecttype", 0, 0, 'l' },
            { "downmix",    0, 0, 'd' },
            { "info",       0, 0, 'i' },
            { "stdio",      0, 0, 'w' },
            { "stdio",      0, 0, 'g' },
            { "seek",       1, 0, 'j' },
            { "help",       0, 0, 'h' },
            { 0, 0, 0, 0 }
        };

        c = getopt_long(argc, argv, "o:a:s:f:b:l:j:wgdhitq",
            long_options, &option_index);

        if (c == -1)
            break;

        switch (c) {
        case 'o':
            if (optarg)
            {
                outfile_set = 1;
                audioFileName = (char *) vmalloc(sizeof(char) * (strlen(optarg) + 1));
                if (audioFileName == NULL)
                {
                    kprintf("Error allocating memory for audioFileName.\r\n");
                    return 1;
                }
                strcpy(audioFileName, optarg);
            }
            break;
        case 'a':
            if (optarg)
            {
                adts_out = 1;
                adtsFileName = (char *) vmalloc(sizeof(char) * (strlen(optarg) + 1));
                if (adtsFileName == NULL)
                {
                    kprintf("Error allocating memory for adtsFileName.\r\n");
                    return 1;
                }
                strcpy(adtsFileName, optarg);
            }
            break;
        case 's':
            if (optarg)
            {
                char dr[10];
                if (sscanf(optarg, "%s", dr) < 1) {
                    def_srate = 0;
                } else {
                    def_srate = atoi(dr);
                }
            }
            break;
        case 'f':
            if (optarg)
            {
                char dr[10];
                if (sscanf(optarg, "%s", dr) < 1)
                {
                    format = 1;
                } else {
                    format = atoi(dr);
                    if ((format < 1) || (format > 2))
                        showHelp = 1;
                }
            }
            break;
        case 'b':
            if (optarg)
            {
                char dr[10];
                if (sscanf(optarg, "%s", dr) < 1)
                {
                    outputFormat = FAAD_FMT_16BIT; /* just use default */
                } else {
                    outputFormat = atoi(dr);
                    if ((outputFormat < 1) || (outputFormat > 5))
                        showHelp = 1;
                }
            }
            break;
        case 'l':
            if (optarg)
            {
                char dr[10];
                if (sscanf(optarg, "%s", dr) < 1)
                {
                    object_type = LC; /* default */
                } else {
                    object_type = atoi(dr);
                    if ((object_type != LC) &&
                        (object_type != MAIN) &&
                        (object_type != LTP) &&
                        (object_type != LD))
                    {
                        showHelp = 1;
                    }
                }
            }
            break;
        case 'j':
            if (optarg)
            {
                seekTo = atof(optarg);
            }
            break;
        case 't':
            old_format = 1;
            break;
        case 'd':
            downMatrix = 1;
            break;
        case 'w':
            writeToStdio = 1;
            break;
        case 'g':
            noGapless = 1;
            break;
        case 'i':
            infoOnly = 1;
            break;
        case 'h':
            showHelp = 1;
            break;
        case 'q':
            quiet = 1;
            break;
        default:
            break;
        }
    }

    NeAACDecGetVersion(&faad_id_string, &faad_copyright_string);

    kprintf(" *********** Ahead Software MPEG-4 AAC Decoder V%s ******************\r\n", faad_id_string);
#ifndef BUILD_DATE
#define BUILD_DATE __DATE__
#endif
    kprintf(" Build: %s\r\n", BUILD_DATE);
#undef BUILD_DATE
    kprintf("%s", faad_copyright_string);
    if (cap & FIXED_POINT_CAP)
        kprintf(" Fixed point version\r\n");
    else
        kprintf(" Floating point version\r\n");
    kprintf("\r\n");
    kprintf(" This program is vfree software; you can redistribute it and/or modify\r\n");
    kprintf(" it under the terms of the GNU General Public License.\r\n");
    kprintf("\r\n");
    kprintf(" **************************************************************************\r\n");


    /* check that we have at least two non-option arguments */
    /* Print help if requested */
    if (((argc - optind) < 1) || showHelp)
    {
        usage();
        return 1;
    }

    /* point to the specified file name */
    aacFileName = (char *) vmalloc(sizeof(char) * (strlen(argv[optind]) + 1));
    if (aacFileName == NULL)
    {
        kprintf("Error allocating memory for aacFileName.\r\n");
        return 1;
    }
    strcpy(aacFileName, argv[optind]);

    begin = VOSGetTimeMs();

    /* Only calculate the path and open the file for writing if
       we are not writing to stdout.
     */
    if(!writeToStdio && !outfile_set)
    {
        audioFileName = (char *) vmalloc(sizeof(char) * (strlen(aacFileName) + strlen(file_ext[format]) + 1));
        if (audioFileName == NULL)
        {
            kprintf("Error allocating memory for audioFileName.\r\n");
            return 1;
        }
        strcpy(audioFileName, aacFileName);

        fnp = (char *)strrchr(audioFileName,'.');

        if (fnp)
            fnp[0] = '\0';

        strcat(audioFileName, file_ext[format]);
    }

    /* check for mp4 file */
	mp4file = 0;
//        hMP4File = faad_fopen(aacFileName, "rb");
	res = f_open(&hMP4File, aacFileName, FA_READ);
	if (res)
	{
		kprintf("Error opening file: %s\r\n", aacFileName);
		return 1;
	}

    //bread = fread(header, 1, 8, hMP4File);
    bread = 0;
    res = f_read (&hMP4File, header, 8, &bread);
//    if (! readFromStdin )
//      f_close(&hMP4File);

    if (bread != 8) {
        kprintf("Error reading file.\r\n");
        return 1;
    }

    if (header[4] == 'f' && header[5] == 't' && header[6] == 'y' && header[7] == 'p')
        mp4file = 1;

    if (!mp4file && seekTo != 0) {
        kprintf("Warning: can only seek in MP4 files\r\n");
    }

    if (mp4file)
    {
        result = decodeMP4file(aacFileName, audioFileName, adtsFileName, writeToStdio,
            outputFormat, format, downMatrix, noGapless, infoOnly, adts_out, &length, seekTo);
    } else {

    if (readFromStdin == 1) {
        x_ungetc(header[7], &hMP4File);
        x_ungetc(header[6], &hMP4File);
        x_ungetc(header[5], &hMP4File);
        x_ungetc(header[4], &hMP4File);
        x_ungetc(header[3], &hMP4File);
        x_ungetc(header[2], &hMP4File);
        x_ungetc(header[1], &hMP4File);
        x_ungetc(header[0], &hMP4File);
    }

        result = decodeAACfile(aacFileName, audioFileName, adtsFileName, writeToStdio,
            def_srate, object_type, outputFormat, format, downMatrix, infoOnly, adts_out,
            old_format, &length);
    }

    if (audioFileName != NULL)
      vfree (audioFileName);
    if (adtsFileName != NULL)
      vfree (adtsFileName);

    if (!result && !infoOnly)
    {
        float dec_length = (float)(VOSGetTimeMs() - begin)/(float)1000.0;
        kprintf("Decoding %s took: %5.2f sec. %5.2fx real-time.\r\n", aacFileName,
            dec_length, (dec_length > 0.01) ? (length/dec_length) : 0.);
    }

    if (aacFileName != NULL)
      vfree (aacFileName);

    return 0;
}

