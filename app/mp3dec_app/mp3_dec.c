#include "mp3_dec.h"
#include "mad.h"
#include "audio.h"
#include "vringbuf.h"
#include "string.h"

#define MP3_CHANGE_2_CHANNEL

#define MP3_RING_MAX (128*1024)

#define NULL 0

#define AUDIO_WM8978_PORT 1

#define GET_TIME_MS() VOSGetTimeMs()

#define debug kprintf
#define assert(x) if (!(x)) {kprintf("assert error: %s!\r\n", __FUNCTION__); while (1) {};}

struct StLoopBytesMgr *GetMgrOfLoopMgr(); //wav and mp3 share loop buffer.
int InitLoopBytesBuffer();


static struct StMp3Stream gMp3Stream;
static int local_samplerate=0;//16000;
static int local_channel = 1;


#define BUFFER_MAX (4*1024)


typedef struct StMp3Stream {
#if MP3_DEBUG
    unsigned int time_mark;
    volatile unsigned int db_mark_decode;
    volatile unsigned int db_mark_decode_max;
#endif
    volatile unsigned int end_flag; //开始就禁止播放。
    volatile unsigned int end_done; // end_flag只是告诉结束，真正结束需要判断end_done==1
    volatile unsigned int man_break;//这时标志需要立即停止输出任何数据，包括libmap解码器里的数据
    volatile unsigned int mp3_duration; //ms
    volatile unsigned int mp3_samplerate;
    volatile unsigned int mp3_bitrate;
    int nReaded;
    int nPos;
    unsigned char buf[BUFFER_MAX];
	StVOSRingBuf *ring_buf; //环形缓冲
    int (*getdata)(unsigned char *buf, int len, int timeout);
}StMp3Stream;

void SetAudioOptions(unsigned int channnel, unsigned int samplerate)
{
    s64 time_mark = 0;
    u32 ret = 0;
    struct StMp3Stream *pMgr = &gMp3Stream;

	if (local_channel != (int)channnel) {

	local_channel = channnel;
//	local_samplerate = samplerate;
//	djy_audio_dac_close();
//	djy_audio_dac_open(AUDIO_DMA_BUFF_SIZE, local_channel, local_samplerate);
//	if (u32g_Volume > 100) {
//	  u32g_Volume = 100;
//	}
//	djy_audio_dac_ctrl(AUD_DAC_CMD_SET_VOLUME, &u32g_Volume);

		debug("==== info: SetAudioOptions, set channel:%d, sample rate: %d! ====\r\n", local_channel, local_samplerate);
	}

    if (local_samplerate != (int)samplerate) {
        local_samplerate = samplerate;
        switch (samplerate) {
            case    audio_sample_rate_11025:
            case    audio_sample_rate_22050:
            case    audio_sample_rate_44100:
            case    audio_sample_rate_12000:
            case    audio_sample_rate_24000:
            case    audio_sample_rate_48000:
            case    audio_sample_rate_8000:
            case    audio_sample_rate_16000:
            case    audio_sample_rate_32000:
                local_samplerate = samplerate;
                break;
            default:
                local_samplerate = audio_sample_rate_44100;
                break;
        }
        audio_ctrl(AUDIO_WM8978_PORT, AUDIO_OPT_AUDIO_SAMPLE, &local_samplerate, 4);
        debug("==== info: SetAudioOptions, set sample rate: %d! ====\r\n", local_samplerate);
    }

}

void Mp3DataStartPlay ()
{
    struct StMp3Stream *pMgr = &gMp3Stream;
    pMgr->end_flag = 0;
    pMgr->end_done = 0;
    pMgr->man_break = 0;
}

int Mp3PlayIsEnded()
{
    struct StMp3Stream *pMgr = &gMp3Stream;
    return pMgr->end_done;
}


void Mp3DataEndPlay ()
{
    struct StMp3Stream *pMgr = &gMp3Stream;
    pMgr->end_flag = 1; //启动结束通知，还没结束，需要缓冲区播放完成才能结束。
}

int Mp3EndDataIsSet()
{
    struct StMp3Stream *pMgr = &gMp3Stream;
    return pMgr->end_flag == 1;
}

int Mp3IsBusy()
{
    struct StMp3Stream *pMgr = &gMp3Stream;
    return !pMgr->end_done;
}

static int NetGetData(unsigned char *buffer, int len, int timeout)
{
    struct StMp3Stream *pMgr = &gMp3Stream;
    int ret = 0;
    s32 irq_save = __vos_irq_save();
    ret = VOSRingBufGet(pMgr->ring_buf, buffer, len);
    if (ret == 0) {
    	VOSTaskDelay(100);
    }
    __vos_irq_restore(irq_save);
    return ret;
}



static enum mad_flow header(void *data, struct mad_header const *stream)
{
    struct StMp3Stream *priv = (struct StMp3Stream *)data;
    if (stream) {
        priv->mp3_bitrate = stream->bitrate;
        priv->mp3_samplerate = stream->samplerate;
        //priv->mp3_duration = stream->duration.fraction*1000/352800000UL+stream->duration.seconds*1000;
        priv->mp3_duration = stream->duration.fraction/352800UL+stream->duration.seconds*1000;

        //kprintf("mp3_duration play(ms)=%d(ms)!\r\n", priv->mp3_duration);
    }
#if MP3_DEBUG
    gMp3Stream.db_mark_decode = GET_TIME_MS();
#endif
    return MAD_FLOW_CONTINUE;
}

static enum mad_flow input(void *data, struct mad_stream *stream)
{
//    enum mad_flow op = MAD_FLOW_CONTINUE;
    int last = 0;
    struct StMp3Stream *priv = (struct StMp3Stream *)data;
    if (!priv->getdata) return MAD_FLOW_STOP;

    if ( stream->buffer == NULL )
    {
        priv->nReaded = priv->getdata(priv->buf, BUFFER_MAX, 1000);
//        debug("info: %s(%d), getdata, ret=%d!\r\n", __FUNCTION__, __LINE__, priv->nReaded);
        if (priv->nReaded > 0) {
            mad_stream_buffer(stream, priv->buf, priv->nReaded);
        }
        return MAD_FLOW_CONTINUE;
    }

    priv->nPos = stream->next_frame - priv->buf;
    if (priv->nReaded > priv->nPos) {
        last = priv->nReaded - priv->nPos;
        memcpy(priv->buf, priv->buf+priv->nPos, last);
    }
#if 1
    priv->nReaded = priv->getdata(priv->buf+last, BUFFER_MAX-last, 1000);
    if (priv->nReaded <= 0) {
        return MAD_FLOW_CONTINUE;
    }
#else
//    debug("info: %s(%d), getdata, ret=%d!\r\n", __FUNCTION__, __LINE__, priv->nReaded);
    priv->nReaded = priv->getdata(priv->buf+last, BUFFER_MAX-last, 0xffffffff);
    if (priv->nReaded <= 0) {
        return MAD_FLOW_STOP;
    }
#endif
    priv->nPos = 0;
    priv->nReaded += last;
    mad_stream_buffer(stream, priv->buf, priv->nReaded);

    return MAD_FLOW_CONTINUE;
}


static inline signed int scale(mad_fixed_t sample)
{
    sample += (1L << (MAD_F_FRACBITS - 16));
    if (sample >= MAD_F_ONE)
        sample = MAD_F_ONE - 1;
    else if (sample < -MAD_F_ONE)
        sample = -MAD_F_ONE;
    return sample >> (MAD_F_FRACBITS + 1 - 16);
}


static enum mad_flow output(void *data, struct mad_header const *header, struct mad_pcm *pcm)
{
    struct StMp3Stream *priv= &gMp3Stream;
    unsigned int nchannels;
    unsigned int nsamples;
    unsigned int n;
    mad_fixed_t const *left_ch;
    mad_fixed_t const *right_ch;

    unsigned char *ptmp = 0;
#if 1
    static unsigned char buffer[6912];
#else
    static unsigned char *buffer = 0;
    if (buffer==0) {
        buffer = (unsigned char*)malloc(6912);
    }
    if (buffer==0) {
        return MAD_FLOW_IGNORE;
    }
#endif



    nchannels = pcm->channels;
    n = nsamples = pcm->length;
    left_ch = pcm->samples[0];
#ifdef MP3_CHANGE_2_CHANNEL
    if (nchannels == 1) {
        right_ch = pcm->samples[0];
        nchannels = 2;
    }
    else {
        right_ch = pcm->samples[1];
    }
#else
    right_ch = pcm->samples[1];
#endif
    ptmp = buffer;
    while (nsamples--) {
        signed int sample;
        sample = scale(*left_ch++);
        *(ptmp++) = sample >> 0;
        *(ptmp++) = sample >> 8;
        if (nchannels == 2) {
            sample = scale(*right_ch++);
            *(ptmp++) = sample >> 0;
            *(ptmp++) = sample >> 8;
        }
    }
    if (nchannels == 2)
        n *= 4;
    else
        n *= 2;

    ptmp = buffer;

    SetAudioOptions(nchannels, priv->mp3_samplerate);

#if MP3_DEBUG
    if (gMp3Stream.db_mark_decode_max < GET_TIME_MS()-gMp3Stream.db_mark_decode) {
        gMp3Stream.db_mark_decode_max = GET_TIME_MS()-gMp3Stream.db_mark_decode;
    }
    //kprintf("mp3 frame decode spend %d(ms), max=%d(ms)!\r\n", GET_TIME_MS()-gMp3Stream.db_mark_decode, gMp3Stream.db_mark_decode_max);
#endif

	int mark = 0;
	s32 sended = 0;
	while (1) {
		sended = audio_sends(AUDIO_WM8978_PORT, &ptmp[mark], n-mark, 100);
		if (sended > 0) {
			mark += sended;
		}
		if (sended == 0) {
			VOSTaskDelay(5);
		}
		if (mark  ==  n) {
			break;
		}
	}
    return MAD_FLOW_CONTINUE;
}


static enum mad_flow error(void *data, struct mad_stream *stream, struct mad_frame *frame)
{
    (void)data;
    (void)stream;
    (void)frame;

    //kprintf("warning: frame decode return error, and continue flow ...!\r\n");
    return MAD_FLOW_CONTINUE;
}

u32 Mp3Framebitrate()
{
    struct StMp3Stream *priv = &gMp3Stream;
    return priv->mp3_bitrate;
}

s32 mp3_dec_play(u8 *buf, s32 len, u32 timeout)
{
	s32 ret = 0;
	struct StMp3Stream *pMgr = &gMp3Stream;
    s32 irq_save = __vos_irq_save();
    ret = VOSRingBufSet(pMgr->ring_buf, buf, len);
    __vos_irq_restore(irq_save);
    return ret;
}

void clear_mp3_stream() {
    struct StMp3Stream *pMgr = &gMp3Stream;
#if MP3_DEBUG
    pMgr->time_mark = 0;
    pMgr->db_mark_decode = 0;
    pMgr->db_mark_decode_max = 0;
#endif
    pMgr->mp3_duration = 0; //ms
    pMgr->mp3_samplerate = 0;
    pMgr->mp3_bitrate = 0;
    pMgr->nReaded = 0;
    pMgr->nPos = 0;
    memset(pMgr->buf, 0, BUFFER_MAX);
    //pMgr->getdata = 0;
}

void mp3_dec_main(void *arg)
{
    s32 ret = 0;
    struct mad_decoder decoder;

    while (1) {

        gMp3Stream.getdata = NetGetData;
        mad_decoder_init(&decoder, &gMp3Stream, input, header, 0, output, error, 0);
        mad_decoder_options(&decoder, 0);
        ret = mad_decoder_run(&decoder, MAD_DECODER_MODE_SYNC);

        mad_decoder_finish(&decoder);
        VOSTaskDelay(1000);
        kprintf("--------mp3--------loop!!!\r\n");
    }
}

int mp3_stream_reset()
{
    int cnt = 0;
    struct StMp3Stream *pMgr = &gMp3Stream;
    return 0;
}
#include "ff.h"
s32 mp3_dec_file(s8 *path)
{
    s32 ret = 0;
    s32 length = 1024;
    FIL fmp3;
    ret = f_open(&fmp3, (TCHAR*)path, FA_READ);
    if (ret) {
        kprintf("error: mp3_dec_file, path=%s!\r\n", path);
        return -1;
    }
    s32 file_size = f_size(&fmp3);
    kprintf ("info: file size: %d!\r\n", file_size);

    u8 *p = (u8*)vmalloc(length);
    if (p == 0) goto ERROR_RET;

    s32 n = 0;
    s32 pos = 0;
    s32 offset = 0;
    while (1) {
        if (offset >= file_size) break;
        n = 0;
   		ret = f_read (&fmp3, p, length, &n);
   		if (ret != 0) {
   			kprintf("error: read file return %d!\r\n", ret);
   			break;
   		}
        if (n > 0) {
            offset += n;
            pos = 0;
            while (1) {
                if (pos >= n) break;
                ret = mp3_dec_play(p+pos, n-pos, 1000);
                if (ret > 0){
                    pos += ret;
                }
                else {
                    //kprintf("mp3_play_from_file->Mp3PlayData, ret=%d!\r\n", ret);
                    VOSTaskDelay(100);
                }
            }
        }
        else {
            kprintf("mp3_play_from_file->fread, n=%d!\r\n", n);
            VOSTaskDelay(30);
        }
    }

ERROR_RET:
    f_close(&fmp3);
    if (p) vfree(p);
    return 0;
}

long long task_mp3_dec_stack[1024*3];
s32 mp3_dec_init()
{
	s32 ret = 0;
    struct StMp3Stream *pMgr = &gMp3Stream;
    memset(pMgr, 0, sizeof(struct StMp3Stream));
    pMgr->end_flag = 1;//开始就禁止播放。
    pMgr->end_done = 1;// end_flag只是告诉结束，真正结束需要判断end_done==1
    pMgr->man_break = 0; //这时标志需要立即停止输出任何数据，包括libmap解码器里的数据
    pMgr->ring_buf = VOSRingBufCreate(MP3_RING_MAX);
    if (pMgr->ring_buf==0) return -1;
    ret = audio_open(AUDIO_WM8978_PORT, AUDIO_ADC_16BIT);
    s32 task_id = VOSTaskCreate(mp3_dec_main, 0, task_mp3_dec_stack, sizeof(task_mp3_dec_stack), TASK_PRIO_REAL, "mp3_dec");
    return ret;
}


