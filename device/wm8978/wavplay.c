#include "wavplay.h"
#include "ff.h"
#include "i2s.h"
#include "wm8978.h"
#if 0
void (*i2s_tx_callback)(void);

//V1.0 ˵��
//1,֧��16λ/24λWAV�ļ�����
//2,��߿���֧�ֵ�192K/24bit��WAV��ʽ.
//////////////////////////////////////////////////////////////////////////////////
__audiodev audiodev;

#define vu8 volatile u8

__wavctrl wavctrl;		//WAV���ƽṹ��
vu8 wavtransferend=0;	//i2s������ɱ�־
vu8 wavwitchbuf=0;		//i2sbufxָʾ��־

//��ʼ��Ƶ����
void audio_start(void)
{
	audiodev.status=3<<0;//��ʼ����+����ͣ
	I2S_Play_Start();
}
//�ر���Ƶ����
void audio_stop(void)
{
	audiodev.status=0;
	I2S_Play_Stop();
}

//WAV������ʼ��
//fname:�ļ�·��+�ļ���
//wavx:wav ��Ϣ��Žṹ��ָ��
//����ֵ:0,�ɹ�;1,���ļ�ʧ��;2,��WAV�ļ�;3,DATA����δ�ҵ�.
u8 wav_decode_init(u8* fname,__wavctrl* wavx)
{
	FIL*ftemp;
	u8 *buf;
	u32 br=0;
	u8 res=0;

	ChunkRIFF *riff;
	ChunkFMT *fmt;
	ChunkFACT *fact;
	ChunkDATA *data;
	ftemp=(FIL*)vmalloc(sizeof(FIL));
	buf=vmalloc(512);
	if(ftemp&&buf)	//�ڴ�����ɹ�
	{
		res=f_open(ftemp,(TCHAR*)fname,FA_READ);//���ļ�
		if(res==FR_OK)
		{
			f_read(ftemp,buf,512,&br);	//��ȡ512�ֽ�������
			riff=(ChunkRIFF *)buf;		//��ȡRIFF��
			if(riff->Format==0X45564157)//��WAV�ļ�
			{
				fmt=(ChunkFMT *)(buf+12);	//��ȡFMT��
				fact=(ChunkFACT *)(buf+12+8+fmt->ChunkSize);//��ȡFACT��
				if(fact->ChunkID==0X74636166||fact->ChunkID==0X5453494C)wavx->datastart=12+8+fmt->ChunkSize+8+fact->ChunkSize;//����fact/LIST���ʱ��(δ����)
				else wavx->datastart=12+8+fmt->ChunkSize;
				data=(ChunkDATA *)(buf+wavx->datastart);	//��ȡDATA��
				if(data->ChunkID==0X61746164)//�����ɹ�!
				{
					wavx->audioformat=fmt->AudioFormat;		//��Ƶ��ʽ
					wavx->nchannels=fmt->NumOfChannels;		//ͨ����
					wavx->samplerate=fmt->SampleRate;		//������
					wavx->bitrate=fmt->ByteRate*8;			//�õ�λ��
					wavx->blockalign=fmt->BlockAlign;		//�����
					wavx->bps=fmt->BitsPerSample;			//λ��,16/24/32λ

					wavx->datasize=data->ChunkSize;			//���ݿ��С
					wavx->datastart=wavx->datastart+8;		//��������ʼ�ĵط�.

					printf("wavx->audioformat:%d\r\n",wavx->audioformat);
					printf("wavx->nchannels:%d\r\n",wavx->nchannels);
					printf("wavx->samplerate:%d\r\n",wavx->samplerate);
					printf("wavx->bitrate:%d\r\n",wavx->bitrate);
					printf("wavx->blockalign:%d\r\n",wavx->blockalign);
					printf("wavx->bps:%d\r\n",wavx->bps);
					printf("wavx->datasize:%d\r\n",wavx->datasize);
					printf("wavx->datastart:%d\r\n",wavx->datastart);
				}else res=3;//data����δ�ҵ�.
			}else res=2;//��wav�ļ�

		}else res=1;//���ļ�����
	}
	f_close(ftemp);
	vfree(ftemp);//�ͷ��ڴ�
	vfree(buf);
	return 0;
}

//���buf
//buf:������
//size:���������
//bits:λ��(16/24)
//����ֵ:���������ݸ���
u32 wav_buffill(u8 *buf,u16 size,u8 bits)
{
	u16 readlen=0;
	u32 bread;
	u16 i;
	u8 *p;
	if(bits==24)//24bit��Ƶ,��Ҫ����һ��
	{
		readlen=(size/4)*3;							//�˴�Ҫ��ȡ���ֽ���
		f_read(audiodev.file,audiodev.tbuf,readlen,(UINT*)&bread);	//��ȡ����
		p=audiodev.tbuf;
		for(i=0;i<size;)
		{
			buf[i++]=p[1];
			buf[i]=p[2];
			i+=2;
			buf[i++]=p[0];
			p+=3;
		}
		bread=(bread*4)/3;		//����Ĵ�С.
	}else
	{
		f_read(audiodev.file,buf,size,(UINT*)&bread);//16bit��Ƶ,ֱ�Ӷ�ȡ����
		if(bread<size)//����������,����0
		{
			for(i=bread;i<size-bread;i++)buf[i]=0;
		}
	}
	return bread;
}
//WAV����ʱ,I2S DMA����ص�����
void wav_i2s_dma_tx_callback(void)
{
	u16 i;
	if(DMA1_Stream4->CR&(1<<19))
	{
		wavwitchbuf=0;
		if((audiodev.status&0X01)==0)
		{
			for(i=0;i<WAV_I2S_TX_DMA_BUFSIZE;i++)//��ͣ
			{
				audiodev.i2sbuf1[i]=0;//���0
			}
		}
	}else
	{
		wavwitchbuf=1;
		if((audiodev.status&0X01)==0)
		{
			for(i=0;i<WAV_I2S_TX_DMA_BUFSIZE;i++)//��ͣ
			{
				audiodev.i2sbuf2[i]=0;//���0
			}
		}
	}
	wavtransferend=1;
}
//�õ���ǰ����ʱ��
//fx:�ļ�ָ��
//wavx:wav���ſ�����
void wav_get_curtime(FIL*fx,__wavctrl *wavx)
{
	long long fpos;
 	wavx->totsec=wavx->datasize/(wavx->bitrate/8);	//�����ܳ���(��λ:��)
	fpos=fx->fptr-wavx->datastart; 					//�õ���ǰ�ļ����ŵ��ĵط�
	wavx->cursec=fpos*wavx->totsec/wavx->datasize;	//��ǰ���ŵ��ڶ�������?
}
//����ĳ��WAV�ļ�
//fname:wav�ļ�·��.
//����ֵ:
//KEY0_PRES:��һ��
//KEY1_PRES:��һ��
//����:����
u8 wav_play_song(u8* fname)
{
	u8 key;
	u8 t=0;
	u8 res;
	u32 fillnum;
	audiodev.file=(FIL*)vmalloc(sizeof(FIL));
	audiodev.i2sbuf1=vmalloc(WAV_I2S_TX_DMA_BUFSIZE);
	audiodev.i2sbuf2=vmalloc(WAV_I2S_TX_DMA_BUFSIZE);
	audiodev.tbuf=vmalloc(WAV_I2S_TX_DMA_BUFSIZE);
	if(audiodev.file&&audiodev.i2sbuf1&&audiodev.i2sbuf2&&audiodev.tbuf)
	{
		res=wav_decode_init(fname,&wavctrl);//�õ��ļ�����Ϣ
		if(res==0)//�����ļ��ɹ�
		{
			if(wavctrl.bps==16)
			{
				WM8978_I2S_Cfg(2,0);	//�����ֱ�׼,16λ���ݳ���
				I2S2_Init(I2S_STANDARD_PHILIPS,I2S_MODE_MASTER_TX,I2S_CPOL_LOW,I2S_DATAFORMAT_16B_EXTENDED);	//�����ֱ�׼,��������,ʱ�ӵ͵�ƽ��Ч,16λ��չ֡����
			}else if(wavctrl.bps==24)
			{
				WM8978_I2S_Cfg(2,2);	//�����ֱ�׼,24λ���ݳ���
				I2S2_Init(I2S_STANDARD_PHILIPS,I2S_MODE_MASTER_TX,I2S_CPOL_LOW,I2S_DATAFORMAT_24B);	//�����ֱ�׼,��������,ʱ�ӵ͵�ƽ��Ч,24λ����
			}
			I2S2_SampleRate_Set(wavctrl.samplerate);//���ò�����
			I2S2_TX_DMA_Init(audiodev.i2sbuf1,audiodev.i2sbuf2,WAV_I2S_TX_DMA_BUFSIZE/2); //����TX DMA
			i2s_tx_callback=wav_i2s_dma_tx_callback;			//�ص�����ָwav_i2s_dma_callback
			audio_stop();
			res=f_open(audiodev.file,(TCHAR*)fname,FA_READ);	//���ļ�
			if(res==0)
			{
				f_lseek(audiodev.file, wavctrl.datastart);		//�����ļ�ͷ
				fillnum=wav_buffill(audiodev.i2sbuf1,WAV_I2S_TX_DMA_BUFSIZE,wavctrl.bps);
				fillnum=wav_buffill(audiodev.i2sbuf2,WAV_I2S_TX_DMA_BUFSIZE,wavctrl.bps);
				audio_start();
				while(res==0)
				{
					while(wavtransferend==0);//�ȴ�wav�������;
					wavtransferend=0;

 					if(wavwitchbuf)fillnum=wav_buffill(audiodev.i2sbuf2,WAV_I2S_TX_DMA_BUFSIZE,wavctrl.bps);//���buf2
					else fillnum=wav_buffill(audiodev.i2sbuf1,WAV_I2S_TX_DMA_BUFSIZE,wavctrl.bps);//���buf1
					while(1)
					{
						wav_get_curtime(audiodev.file,&wavctrl);//�õ���ʱ��͵�ǰ���ŵ�ʱ��
						//audio_msg_show(wavctrl.totsec,wavctrl.cursec,wavctrl.bitrate);
						t++;
						if(t==20)
						{
							t=0;
						}
						if((audiodev.status&0X01)==0) VOSTaskDelay(10);
						else break;
					}
				}
				audio_stop();
			}else res=0XFF;
		}else res=0XFF;
	}else res=0XFF;
	vfree(audiodev.tbuf);	//�ͷ��ڴ�
	vfree(audiodev.i2sbuf1);//�ͷ��ڴ�
	vfree(audiodev.i2sbuf2);//�ͷ��ڴ�
	vfree(audiodev.file);	//�ͷ��ڴ�
	return res;
}
#endif
u8 wav_decode_init(u8* fname,__wavctrl* wavx)
{
	FIL*ftemp;
	u8 *buf;
	u32 br=0;
	u8 res=0;

	ChunkRIFF *riff;
	ChunkFMT *fmt;
	ChunkFACT *fact;
	ChunkDATA *data;
	ftemp=(FIL*)vmalloc(sizeof(FIL));
	buf=vmalloc(512);
	if(ftemp&&buf)	//�ڴ�����ɹ�
	{
		res=f_open(ftemp,(TCHAR*)fname,FA_READ);//���ļ�
		if(res==FR_OK)
		{
			f_read(ftemp,buf,512,&br);	//��ȡ512�ֽ�������
			riff=(ChunkRIFF *)buf;		//��ȡRIFF��
			if(riff->Format==0X45564157)//��WAV�ļ�
			{
				fmt=(ChunkFMT *)(buf+12);	//��ȡFMT��
				fact=(ChunkFACT *)(buf+12+8+fmt->ChunkSize);//��ȡFACT��
				if(fact->ChunkID==0X74636166||fact->ChunkID==0X5453494C)wavx->datastart=12+8+fmt->ChunkSize+8+fact->ChunkSize;//����fact/LIST���ʱ��(δ����)
				else wavx->datastart=12+8+fmt->ChunkSize;
				data=(ChunkDATA *)(buf+wavx->datastart);	//��ȡDATA��
				if(data->ChunkID==0X61746164)//�����ɹ�!
				{
					wavx->audioformat=fmt->AudioFormat;		//��Ƶ��ʽ
					wavx->nchannels=fmt->NumOfChannels;		//ͨ����
					wavx->samplerate=fmt->SampleRate;		//������
					wavx->bitrate=fmt->ByteRate*8;			//�õ�λ��
					wavx->blockalign=fmt->BlockAlign;		//�����
					wavx->bps=fmt->BitsPerSample;			//λ��,16/24/32λ

					wavx->datasize=data->ChunkSize;			//���ݿ��С
					wavx->datastart=wavx->datastart+8;		//��������ʼ�ĵط�.

					printf("wavx->audioformat:%d\r\n",wavx->audioformat);
					printf("wavx->nchannels:%d\r\n",wavx->nchannels);
					printf("wavx->samplerate:%d\r\n",wavx->samplerate);
					printf("wavx->bitrate:%d\r\n",wavx->bitrate);
					printf("wavx->blockalign:%d\r\n",wavx->blockalign);
					printf("wavx->bps:%d\r\n",wavx->bps);
					printf("wavx->datasize:%d\r\n",wavx->datasize);
					printf("wavx->datastart:%d\r\n",wavx->datastart);
				}else res=3;//data����δ�ҵ�.
			}else res=2;//��wav�ļ�

		}else res=1;//���ļ�����
	}
	f_close(ftemp);
	vfree(ftemp);//�ͷ��ڴ�
	vfree(buf);
	return 0;
}

u8 wav_play_song(u8* fname)
{
	s32 ret = 0;
	static u8 buf[1*1024];//buf[16*1024];
	u32 readed = 0;
	__wavctrl wavctrl;
	FIL fmp3;
	ret = wav_decode_init(fname, &wavctrl);//�õ��ļ�����Ϣ
	if(ret != 0) goto END;

//	WM8978_ADDA_Cfg(1,0);		//����DAC
//	WM8978_Input_Cfg(0,0,0);	//�ر�����ͨ��(MIC&LINE IN)
//	WM8978_Output_Cfg(1,0);		//����DAC���
//	WM8978_MIC_Gain(0);			//MIC��������Ϊ0
//	WM8978_SPKvol_Set(50);		//������������
//
//	I2S_Play_Stop();			//ֹͣʱ�ӷ���
//	I2S_Rec_Stop(); 			//ֹͣ¼��

	if(wavctrl.bps == 16) {
		WM8978_I2S_Cfg(2, 0);	//�����ֱ�׼,16λ���ݳ���
		i2s_open(1, I2S_STANDARD_PHILIPS, I2S_MODE_MASTER_TX, I2S_CPOL_LOW, I2S_DATAFORMAT_16B_EXTENDED);	//�����ֱ�׼,��������,ʱ�ӵ͵�ƽ��Ч,16λ��չ֡����
	}
	else if(wavctrl.bps == 24){
		WM8978_I2S_Cfg(2, 2);	//�����ֱ�׼,24λ���ݳ���
		i2s_open(1, I2S_STANDARD_PHILIPS, I2S_MODE_MASTER_TX, I2S_CPOL_LOW, I2S_DATAFORMAT_24B);	//�����ֱ�׼,��������,ʱ�ӵ͵�ƽ��Ч,24λ����
	}
	else  {
		goto END;
	}



	I2S2_SampleRate_Set(wavctrl.samplerate);

//	DMA_XXX();
//	I2S_Play_Stop();
	s32 mark = 0;
	ret=f_open(&fmp3, (TCHAR*)fname, FA_READ);	//���ļ�
	if(ret == 0) {
		f_lseek(&fmp3, wavctrl.datastart);		//�����ļ�ͷ
		while (1) {							//�˴�Ҫ��ȡ���ֽ���
			f_read(&fmp3, buf, sizeof(buf), (UINT*)&readed);	//��ȡ����
			if (readed < sizeof(buf)) {
				break;
			}
			mark = 0;
			while (1) {
				ret = i2s_sends(1, buf+mark, readed-mark, 1000);
				if (ret > 0) {
					mark += ret;
				}
				if (mark == readed) break;
				if (ret <= 0){
					//VOSTaskDelay(5);
				}
			}
		}
	}
END:
	return ret;
}











