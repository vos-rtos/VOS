/****************************************************************************
    MP4 input module

    Copyright (C) 2017 Krzysztof Nikiel

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
****************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <limits.h>

#include "unicode_support.h"
#include "mp4read.h"
#include "ff.h"
#include "vos.h"

enum ATOM_TYPE
{
    ATOM_STOP = 0 /* end of atoms */ ,
    ATOM_NAME /* plain atom */ ,
    ATOM_DESCENT,               /* starts group of children */
    ATOM_ASCENT,                /* ends group */
    ATOM_DATA,
};
typedef struct
{
    uint16_t opcode;
    void *data;
} creator_t;


mp4config_t mp4config = { 0 };

static FIL g_fin;

static inline uint32_t bswap32(const uint32_t u32)
{
#ifndef WORDS_BIGENDIAN
#if defined (__GNUC__) && ((__GNUC__ > 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ >= 3)))
    return __builtin_bswap32(u32);
#elif defined (_MSC_VER)
    return _byteswap_ulong(u32);
#else
    return (u32 << 24) | ((u32 << 8) & 0xFF0000) | ((u32 >> 8) & 0xFF00) | (u32 >> 24);
#endif
#else
    return u32;
#endif
}

static inline uint16_t bswap16(const uint16_t u16)
{
#ifndef WORDS_BIGENDIAN
#if defined (__GNUC__) && ((__GNUC__ > 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ >= 8)))
    return __builtin_bswap16(u16);
#elif defined (_MSC_VER)
    return _byteswap_ushort(u16);
#else
    return (u16 << 8) | (u16 >> 8);
#endif
#else
    return u16;
#endif
}

enum {ERR_OK = 0, ERR_FAIL = -1, ERR_UNSUPPORTED = -2};

static int datain(void *data, int size)
{
	u32 num;
	FRESULT res;

	num = 0;
	res = f_read (&g_fin, data, size, &num);
	if (res == 0 && num == size) {
		return size;
	}
	else
		return ERR_FAIL;
//    if (fread(data, 1, size, g_fin) != size)
//        return ERR_FAIL;
//    return size;
}

static int stringin(char *txt, int sizemax)
{
	u32 num;
	FRESULT res;
    int size;
    for (size = 0; size < sizemax; size++)
    {
//        if (fread(txt + size, 1, 1, g_fin) != 1)
//            return ERR_FAIL;
    	res = f_read (&g_fin, txt + size, 1, &num);
    	if (res || num != 1)
    		return ERR_FAIL;
        if (!txt[size])
            break;
    }
    txt[sizemax-1] = '\0';

    return size;
}

static uint32_t u32in(void)
{
    uint32_t u32;
    datain(&u32, 4);
    u32 = bswap32(u32);
    return u32;
}

static uint16_t u16in(void)
{
    uint16_t u16;
    datain(&u16, 2);
    u16 = bswap16(u16);
    return u16;
}

static int u8in(void)
{
    uint8_t u8;
    datain(&u8, 1);
    return u8;
}

static int ftypin(int size)
{
    enum {BUFSIZE = 40};
    char buf[BUFSIZE];
    uint32_t u32;

    buf[4] = 0;
    datain(buf, 4);
    u32 = u32in();

    if (mp4config.verbose.header)
        kprintf( "Brand:\t\t\t%s(version %d)\n", buf, u32);

    stringin(buf, BUFSIZE);

    if (mp4config.verbose.header)
        kprintf( "Compatible brands:\t%s\n", buf);

    return size;
}

enum
{ SECSINDAY = 24 * 60 * 60 };
static char *mp4time(time_t t)
{
    int y;

    // subtract some seconds from the start of 1904 to the start of 1970
    for (y = 1904; y < 1970; y++)
    {
        t -= 365 * SECSINDAY;
        if (!(y & 3))
            t -= SECSINDAY;
    }
    return ctime(&t);
}

static int mdhdin(int size)
{
    // version/flags
    u32in();
    // Creation time
    mp4config.ctime = u32in();
    // Modification time
    mp4config.mtime = u32in();
    // Time scale
    mp4config.samplerate = u32in();
    // Duration
    mp4config.samples = u32in();
    // Language
    u16in();
    // pre_defined
    u16in();

    return size;
};

static int hdlr1in(int size)
{
    uint8_t buf[5];

    buf[4] = 0;
    // version/flags
    u32in();
    // pre_defined
    u32in();
    // Component subtype
    datain(buf, 4);
    if (mp4config.verbose.header)
        kprintf( "*track media type: '%s': ", buf);
    if (memcmp("soun", buf, 4))
    {
        if (mp4config.verbose.header)
            kprintf( "unsupported, skipping\n");
        return ERR_UNSUPPORTED;
    }
    else
    {
        if (mp4config.verbose.header)
            kprintf( "OK\n");
    }
    // reserved
    u32in();
    u32in();
    u32in();
    // name
    // null terminate
    u8in();

    return size;
};

static int stsdin(int size)
{
    // version/flags
    u32in();
    // Number of entries(one 'mp4a')
    if (u32in() != 1) //fixme: error handling
        return ERR_FAIL;

    return size;
};

static int mp4ain(int size)
{
    // Reserved (6 bytes)
    u32in();
    u16in();
    // Data reference index
    u16in();
    // Version
    u16in();
    // Revision level
    u16in();
    // Vendor
    u32in();
    // Number of channels
    mp4config.channels = u16in();
    // Sample size (bits)
    mp4config.bits = u16in();
    // Compression ID
    u16in();
    // Packet size
    u16in();
    // Sample rate (16.16)
    // fractional framerate, probably not for audio
    // rate integer part
    u16in();
    // rate reminder part
    u16in();

    return size;
}


static uint32_t getsize(void)
{
    int cnt;
    uint32_t size = 0;
    for (cnt = 0; cnt < 4; cnt++)
    {
        int tmp = u8in();

        size <<= 7;
        size |= (tmp & 0x7f);
        if (!(tmp & 0x80))
            break;
    }
    return size;
}

static int esdsin(int size)
{
    // descriptor tree:
    // MP4ES_Descriptor
    //   MP4DecoderConfigDescriptor
    //      MP4DecSpecificInfoDescriptor
    //   MP4SLConfigDescriptor
    enum
    { TAG_ES = 3, TAG_DC = 4, TAG_DSI = 5, TAG_SLC = 6 };

    // version/flags
    u32in();
    if (u8in() != TAG_ES)
        return ERR_FAIL;
    getsize();
    // ESID
    u16in();
    // flags(url(bit 6); ocr(5); streamPriority (0-4)):
    u8in();

    if (u8in() != TAG_DC)
        return ERR_FAIL;
    getsize();
    if (u8in() != 0x40) /* not MPEG-4 audio */
        return ERR_FAIL;
    // flags
    u8in();
    // buffer size (24 bits)
    mp4config.buffersize = u16in() << 8;
    mp4config.buffersize |= u8in();
    // bitrate
    mp4config.bitratemax = u32in();
    mp4config.bitrateavg = u32in();

    if (u8in() != TAG_DSI)
        return ERR_FAIL;
    mp4config.asc.size = getsize();
    if (mp4config.asc.size > sizeof(mp4config.asc.buf))
        return ERR_FAIL;
    // get AudioSpecificConfig
    datain(mp4config.asc.buf, mp4config.asc.size);

    if (u8in() != TAG_SLC)
        return ERR_FAIL;
    getsize();
    // "predefined" (no idea)
    u8in();

    return size;
}

static int sttsin(int size)
{
    if (size < 16) //min stts size
        return ERR_FAIL;

    return size;
}

static int stszin(int size)
{
    int cnt;
    uint32_t ofs;

    // version/flags
    u32in();
    // Sample size
    u32in();
    // Number of entries
    mp4config.frame.ents = u32in();

    if (!(mp4config.frame.ents + 1))
        return ERR_FAIL;

    mp4config.frame.data = malloc(sizeof(*mp4config.frame.data)
                                  * (mp4config.frame.ents + 1));

    if (!mp4config.frame.data)
        return ERR_FAIL;

    ofs = 0;
    mp4config.frame.data[0] = ofs;
    for (cnt = 0; cnt < mp4config.frame.ents; cnt++)
    {
        uint32_t fsize = u32in();

        ofs += fsize;
        if (mp4config.frame.maxsize < fsize)
            mp4config.frame.maxsize = fsize;

        mp4config.frame.data[cnt + 1] = ofs;

        if (ofs < mp4config.frame.data[cnt])
            return ERR_FAIL;
    }

    return size;
}

static int stcoin(int size)
{
    // version/flags
    u32in();
    // Number of entries
    if (u32in() < 1)
        return ERR_FAIL;
    // first chunk offset
    mp4config.mdatofs = u32in();
    // ignore the rest

    return size;
}

#if 0
static int tagtxt(char *tagname, const char *tagtxt)
{
    //int txtsize = strlen(tagtxt);
    int size = 0;
    //int datasize = txtsize + 16;

#if 0
    size += u32out(datasize + 8);
    size += dataout(tagname, 4);
    size += u32out(datasize);
    size += dataout("data", 4);
    size += u32out(1);
    size += u32out(0);
    size += dataout(tagtxt, txtsize);
#endif

    return size;
}

static int tagu32(char *tagname, int n /*number of stored fields*/)
{
    //int numsize = n * 4;
    int size = 0;
    //int datasize = numsize + 16;

#if 0
    size += u32out(datasize + 8);
    size += dataout(tagname, 4);
    size += u32out(datasize);
    size += dataout("data", 4);
    size += u32out(0);
    size += u32out(0);
#endif

    return size;
}
#endif

static int metain(int size)
{
    // version/flags
    u32in();

    return ERR_OK;
};

static int hdlr2in(int size)
{
    uint8_t buf[4];

    // version/flags
    u32in();
    // Predefined
    u32in();
    // Handler type
    datain(buf, 4);
    if (memcmp(buf, "mdir", 4))
        return ERR_FAIL;
    datain(buf, 4);
    if (memcmp(buf, "appl", 4))
        return ERR_FAIL;
    // Reserved
    u32in();
    u32in();
    // null terminator
    u8in();

    return size;
};

static int ilstin(int size)
{
    enum {NUMSET = 1, GENRE, EXTAG};
    int read = 0;

    static struct {
        char *name;
        char *id;
        int flag;
    } tags[] = {
        {"Album       ", "\xa9" "alb"},
        {"Album Artist", "aART"},
        {"Artist      ", "\xa9" "ART"},
        {"Comment     ", "\xa9" "cmt"},
        {"Cover image ", "covr"},
        {"Compilation ", "cpil"},
        {"Copyright   ", "cprt"},
        {"Date        ", "\xa9" "day"},
        {"Disc#       ", "disk", NUMSET},
        {"Genre       ", "gnre", GENRE},
        {"Grouping    ", "\xa9" "grp"},
        {"Lyrics      ", "\xa9" "lyr"},
        {"Title       ", "\xa9" "nam"},
        {"Rating      ", "rtng"},
        {"BPM         ", "tmpo"},
        {"Encoder     ", "\xa9" "too"},
        {"Track       ", "trkn", NUMSET},
        {"Composer    ", "\xa9" "wrt"},
        {0, "----", EXTAG},
        {0},
    };

    static const char *genres[] = {
        "Blues", "Classic Rock", "Country", "Dance",
        "Disco", "Funk", "Grunge", "Hip-Hop",
        "Jazz", "Metal", "New Age", "Oldies",
        "Other", "Pop", "R&B", "Rap",
        "Reggae", "Rock", "Techno", "Industrial",
        "Alternative", "Ska", "Death Metal", "Pranks",
        "Soundtrack", "Euro-Techno", "Ambient", "Trip-Hop",
        "Vocal", "Jazz+Funk", "Fusion", "Trance",
        "Classical", "Instrumental", "Acid", "House",
        "Game", "Sound Clip", "Gospel", "Noise",
        "Alternative Rock", "Bass", "Soul", "Punk",
        "Space", "Meditative", "Instrumental Pop", "Instrumental Rock",
        "Ethnic", "Gothic", "Darkwave", "Techno-Industrial",
        "Electronic", "Pop-Folk", "Eurodance", "Dream",
        "Southern Rock", "Comedy", "Cult", "Gangsta",
        "Top 40", "Christian Rap", "Pop/Funk", "Jungle",
        "Native US", "Cabaret", "New Wave", "Psychadelic",
        "Rave", "Showtunes", "Trailer", "Lo-Fi",
        "Tribal", "Acid Punk", "Acid Jazz", "Polka",
        "Retro", "Musical", "Rock & Roll", "Hard Rock",
        "Folk", "Folk-Rock", "National Folk", "Swing",
        "Fast Fusion", "Bebob", "Latin", "Revival",
        "Celtic", "Bluegrass", "Avantgarde", "Gothic Rock",
        "Progressive Rock", "Psychedelic Rock", "Symphonic Rock", "Slow Rock",
        "Big Band", "Chorus", "Easy Listening", "Acoustic",
        "Humour", "Speech", "Chanson", "Opera",
        "Chamber Music", "Sonata", "Symphony", "Booty Bass",
        "Primus", "Porn Groove", "Satire", "Slow Jam",
        "Club", "Tango", "Samba", "Folklore",
        "Ballad", "Power Ballad", "Rhythmic Soul", "Freestyle",
        "Duet", "Punk Rock", "Drum Solo", "Acapella",
        "Euro-House", "Dance Hall", "Goa", "Drum & Bass",
        "Club - House", "Hardcore", "Terror", "Indie",
        "BritPop", "Negerpunk", "Polsk Punk", "Beat",
        "Christian Gangsta Rap", "Heavy Metal", "Black Metal", "Crossover",
        "Contemporary Christian", "Christian Rock", "Merengue", "Salsa",
        "Thrash Metal", "Anime", "JPop", "Synthpop",
        "Unknown",
    };

    kprintf( "----------tag list-------------\n");
    while(read < size)
    {
        int asize, dsize;
        uint8_t id[5];
        int cnt;
        uint32_t type;

        id[4] = 0;

        asize = u32in();
        read += asize;
        asize -= 4;
        if (datain(id, 4) < 4)
            return ERR_FAIL;
        asize -= 4;

        for (cnt = 0; tags[cnt].id; cnt++)
        {
            if (!memcmp(id, tags[cnt].id, 4))
                break;
        }

        if (tags[cnt].name)
            kprintf( "%s :   ", tags[cnt].name);
        else
        {
            if (tags[cnt].flag != EXTAG)
                kprintf( "'%s'       :   ", id);
        }

        dsize = u32in();
        asize -= 4;
        if (datain(id, 4) < 4)
            return ERR_FAIL;
        asize -= 4;

        if (tags[cnt].flag != EXTAG)
        {
            if (memcmp(id, "data", 4))
                return ERR_FAIL;
        }
        else
        {
            int spc;

            if (memcmp(id, "mean", 4))
                goto skip;
            dsize -= 8;
            while (dsize > 0)
            {
                u8in();
                asize--;
                dsize--;
            }
            if (asize >= 8)
            {
                dsize = u32in() - 8;
                asize -= 4;
                if (datain(id, 4) < 4)
                    return ERR_FAIL;
                asize -= 4;
                if (memcmp(id, "name", 4))
                    goto skip;
                u32in();
                asize -= 4;
                dsize -= 4;
            }
            spc = 13 - dsize;
            if (spc < 0) spc = 0;
            while (dsize > 0)
            {
                kprintf( "%c",u8in());
                asize--;
                dsize--;
            }
            while (spc--)
                kprintf( " ");
            kprintf( ":   ");
            if (asize >= 8)
            {
                dsize = u32in() - 8;
                asize -= 4;
                if (datain(id, 4) < 4)
                    return ERR_FAIL;
                asize -= 4;
                if (memcmp(id, "data", 4))
                    goto skip;
                u32in();
                asize -= 4;
                dsize -= 4;
            }
            while (dsize > 0)
            {
                kprintf( "%c",u8in());
                asize--;
                dsize--;
            }
            kprintf( "\n");

            goto skip;
        }
        type = u32in();
        asize -= 4;
        u32in();
        asize -= 4;

        switch(type)
        {
        case 1:
            while (asize > 0)
            {
                kprintf( "%c",u8in());
                asize--;
            }
            break;
        case 0:
            switch(tags[cnt].flag)
            {
            case NUMSET:
                u16in();
                asize -= 2;

                kprintf( "%d", u16in());
                asize -= 2;
                kprintf( "/%d", u16in());
                asize -= 2;
                break;
            case GENRE:
                {
                    uint8_t gnum = u16in();
                    asize -= 2;
                    if (!gnum)
                       goto skip;
                    gnum--;
                    if (gnum >= 147)
                        gnum = 147;
                    kprintf( "%s", genres[gnum]);
                }
                break;
            default:
                while(asize > 0)
                {
                    kprintf( "%d/", u16in());
                    asize-=2;
                }
            }
            break;
        case 0x15:
            //kprintf( "(8bit data)");
            while(asize > 0)
            {
                kprintf( "%d", u8in());
                asize--;
                if (asize)
                    kprintf( "/");
            }
            break;
        case 0xd:
            kprintf( "(image data)");
            break;
        default:
            kprintf( "(unknown data type)");
            break;
        }
        kprintf( "\n");

    skip:
        // skip to the end of atom
        while (asize > 0)
        {
            u8in();
            asize--;
        }
    }
    kprintf( "-------------------------------\n");

    return size;
};

static creator_t *g_atom = 0;
static int parse(uint32_t *sizemax)
{
    long apos = 0;
//    long aposmax = ftell(g_fin) + *sizemax;
    long aposmax = f_tell(&g_fin) + *sizemax;
    uint32_t size;

    if (g_atom->opcode != ATOM_NAME)
    {
        kprintf( "parse error: root is not a 'name' opcode\n");
        return ERR_FAIL;
    }
    //kprintf( "looking for '%s'\n", (char *)g_atom->data);

    // search for atom in the file
    while (1)
    {
        char name[4];
        uint32_t tmp;

//        apos = ftell(g_fin);
        apos = f_tell(&g_fin);
        if (apos >= (aposmax - 8))
        {
            kprintf( "parse error: atom '%s' not found\n", (char *)g_atom->data);
            return ERR_FAIL;
        }
        if ((tmp = u32in()) < 8)
        {
//            kprintf( "invalid atom size %x @%lx\n", tmp, ftell(g_fin));
        	kprintf( "invalid atom size %x @%lx\n", tmp, f_tell(&g_fin));
            return ERR_FAIL;
        }

        size = tmp;
        if (datain(name, 4) != 4)
        {
            // EOF
//            kprintf( "can't read atom name @%lx\n", ftell(g_fin));
        	kprintf( "can't read atom name @%lx\n", f_tell(&g_fin));
            return ERR_FAIL;
        }

        //kprintf( "atom: '%c%c%c%c'(%x)", name[0],name[1],name[2],name[3], size);

        if (!memcmp(name, g_atom->data, 4))
        {
            //kprintf( "OK\n");
            break;
        }
        //kprintf( "\n");

//        fseek(g_fin, apos + size, SEEK_SET);
        f_lseek(&g_fin, apos + size);
    }
    *sizemax = size;
    g_atom++;
    if (g_atom->opcode == ATOM_DATA)
    {
        int err = ((int (*)(int)) g_atom->data)(size - 8);
        if (err < ERR_OK)
        {
//            fseek(g_fin, apos + size, SEEK_SET);
        	f_lseek(&g_fin, apos + size);
            return err;
        }
        g_atom++;
    }
    if (g_atom->opcode == ATOM_DESCENT)
    {
//        long apos = ftell(g_fin);;
    	long apos = f_tell(&g_fin);;
        //kprintf( "descent\n");
        g_atom++;
        while (g_atom->opcode != ATOM_STOP)
        {
            uint32_t subsize = size - 8;
            int ret;
            if (g_atom->opcode == ATOM_ASCENT)
            {
                g_atom++;
                break;
            }
            f_lseek(&g_fin, apos);
            if ((ret = parse(&subsize)) < 0)
                return ret;
        }
        //kprintf( "ascent\n");
    }

//    fseek(g_fin, apos + size, SEEK_SET);
    f_lseek(&g_fin, apos + size);

    return ERR_OK;
}



static int moovin(int sizemax)
{
//    long apos = ftell(g_fin);
	long apos = f_tell(&g_fin);
    uint32_t atomsize;
    creator_t *old_atom = g_atom;
    int err, ret = sizemax;

    static creator_t mvhd[] = {
        {ATOM_NAME, "mvhd"},
        {0}
    };
    static creator_t trak[] = {
        {ATOM_NAME, "trak"},
        {ATOM_DESCENT},
        {ATOM_NAME, "tkhd"},
        {ATOM_NAME, "mdia"},
        {ATOM_DESCENT},
        {ATOM_NAME, "mdhd"},
        {ATOM_DATA, mdhdin},
        {ATOM_NAME, "hdlr"},
        {ATOM_DATA, hdlr1in},
        {ATOM_NAME, "minf"},
        {ATOM_DESCENT},
        {ATOM_NAME, "smhd"},
        {ATOM_NAME, "dinf"},
        {ATOM_NAME, "stbl"},
        {ATOM_DESCENT},
        {ATOM_NAME, "stsd"},
        {ATOM_DATA, stsdin},
        {ATOM_DESCENT},
        {ATOM_NAME, "mp4a"},
        {ATOM_DATA, mp4ain},
        {ATOM_DESCENT},
        {ATOM_NAME, "esds"},
        {ATOM_DATA, esdsin},
        {ATOM_ASCENT},
        {ATOM_ASCENT},
        {ATOM_NAME, "stts"},
        {ATOM_DATA, sttsin},
        {ATOM_NAME, "stsc"},
        {ATOM_NAME, "stsz"},
        {ATOM_DATA, stszin},
        {ATOM_NAME, "stco"},
        {ATOM_DATA, stcoin},
        {0}
    };

    g_atom = mvhd;
//    atomsize = sizemax + apos - ftell(g_fin);
    atomsize = sizemax + apos - f_tell(&g_fin);
    if (parse(&atomsize) < 0) {
        g_atom = old_atom;
        return ERR_FAIL;
    }

//    fseek(g_fin, apos, SEEK_SET);
    f_lseek(&g_fin, apos);

    while (1)
    {
        //kprintf( "TRAK\n");
        g_atom = trak;
//        atomsize = sizemax + apos - ftell(g_fin);
        atomsize = sizemax + apos - f_tell(&g_fin);
        if (atomsize < 8)
            break;
        //kprintf( "PARSE(%x)\n", atomsize);
        err = parse(&atomsize);
        //kprintf( "SIZE: %x/%x\n", atomsize, sizemax);
        if (err >= 0)
            break;
        if (err != ERR_UNSUPPORTED) {
            ret = err;
            break;
        }
        //kprintf( "UNSUPP\n");
    }

    g_atom = old_atom;
    return ret;
}


static creator_t g_head[] = {
    {ATOM_NAME, "ftyp"},
    {ATOM_DATA, ftypin},
    {0}
};

static creator_t g_moov[] = {
    {ATOM_NAME, "moov"},
    {ATOM_DATA, moovin},
    //{ATOM_DESCENT},
    //{ATOM_NAME, "mvhd"},
    {0}
};

static creator_t g_meta1[] = {
    {ATOM_NAME, "moov"},
    {ATOM_DESCENT},
    {ATOM_NAME, "udta"},
    {ATOM_DESCENT},
    {ATOM_NAME, "meta"},
    {ATOM_DATA, metain},
    {ATOM_DESCENT},
    {ATOM_NAME, "hdlr"},
    {ATOM_DATA, hdlr2in},
    {ATOM_NAME, "ilst"},
    {ATOM_DATA, ilstin},
    {0}
};

static creator_t g_meta2[] = {
    {ATOM_NAME, "meta"},
    {ATOM_DATA, metain},
    {ATOM_DESCENT},
    {ATOM_NAME, "hdlr"},
    {ATOM_DATA, hdlr2in},
    {ATOM_NAME, "ilst"},
    {ATOM_DATA, ilstin},
    {0}
};


int mp4read_frame(void)
{
	u32 num;
	FRESULT res;
    if (mp4config.frame.current >= mp4config.frame.ents)
        return ERR_FAIL;

    mp4config.bitbuf.size = mp4config.frame.data[mp4config.frame.current + 1]
        - mp4config.frame.data[mp4config.frame.current];

//    if (fread(mp4config.bitbuf.data, 1, mp4config.bitbuf.size, g_fin)
//        != mp4config.bitbuf.size)
	res = f_read (&g_fin, mp4config.bitbuf.data, mp4config.bitbuf.size, &num);
	if (res || num != mp4config.bitbuf.size)
    {
        kprintf( "can't read frame data(frame %d@0x%x)\n",
               mp4config.frame.current,
               mp4config.frame.data[mp4config.frame.current]);

        return ERR_FAIL;
    }

    mp4config.frame.current++;

    return ERR_OK;
}

int mp4read_seek(int framenum)
{
    if (framenum > mp4config.frame.ents)
        return ERR_FAIL;
//    if (fseek(g_fin, mp4config.mdatofs + mp4config.frame.data[framenum], SEEK_SET))
//        return ERR_FAIL;
	if (f_lseek(&g_fin, mp4config.mdatofs + mp4config.frame.data[framenum]))
		return ERR_FAIL;
    mp4config.frame.current = framenum;

    return ERR_OK;
}

static void mp4info(void)
{
    kprintf( "Modification Time:\t\t%s\n", mp4time(mp4config.mtime));
    kprintf( "Samplerate:\t\t%d\n", mp4config.samplerate);
    kprintf( "Total samples:\t\t%d\n", mp4config.samples);
    kprintf( "Total channels:\t\t%d\n", mp4config.channels);
    kprintf( "Bits per sample:\t%d\n", mp4config.bits);
    kprintf( "Buffer size:\t\t%d\n", mp4config.buffersize);
    kprintf( "Max bitrate:\t\t%d\n", mp4config.bitratemax);
    kprintf( "Average bitrate:\t%d\n", mp4config.bitrateavg);
    kprintf( "Samples per frame:\t%d\n", mp4config.framesamples);
    kprintf( "Frames:\t\t\t%d\n", mp4config.frame.ents);
    kprintf( "ASC size:\t\t%d\n", mp4config.asc.size);
    kprintf( "Duration:\t\t%.1f sec\n", (float)mp4config.samples/mp4config.samplerate);
    kprintf( "Data offset/size:\t%x/%x\n", mp4config.mdatofs, mp4config.mdatsize);
}

int mp4read_close(void)
{
#define FREE(x) if(x){free(x);x=0;}
    FREE(mp4config.frame.data);
    FREE(mp4config.bitbuf.data);

    return ERR_OK;
}

int mp4read_open(char *name)
{
    uint32_t atomsize;
    int ret;

    mp4read_close();

    //g_fin = faad_fopen(name, "rb");
    FRESULT res = f_open(&g_fin, name, FA_READ);
    if (res) {
        return ERR_FAIL;
    }
    if (mp4config.verbose.header)
        kprintf( "**** MP4 header ****\n");
    g_atom = g_head;
    atomsize = INT_MAX;
    if (parse(&atomsize) < 0)
        goto err;
    g_atom = g_moov;
    atomsize = INT_MAX;
    //rewind(g_fin);
    f_rewind(&g_fin);
    if ((ret = parse(&atomsize)) < 0)
    {
        kprintf( "parse:%d\n", ret);
        goto err;
    }

    // alloc frame buffer
    mp4config.bitbuf.data = malloc(mp4config.frame.maxsize);

    if (!mp4config.bitbuf.data)
        goto err;

    if (mp4config.verbose.header)
    {
        mp4info();
        kprintf( "********************\n");
    }

    if (mp4config.verbose.tags)
    {
//        rewind(g_fin);
    	f_rewind(&g_fin);
        g_atom = g_meta1;
        atomsize = INT_MAX;
        ret = parse(&atomsize);
        if (ret < 0)
        {
            //rewind(g_fin);
        	f_rewind(&g_fin);
            g_atom = g_meta2;
            atomsize = INT_MAX;
            ret = parse(&atomsize);
        }
    }

    return ERR_OK;
err:
    mp4read_close();
    return ERR_FAIL;
}
