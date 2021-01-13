#ifndef __WAVPLAY_H
#define __WAVPLAY_H
//V1.0 ˵��
//1,֧��16λ/24λWAV�ļ�����
//2,��߿���֧�ֵ�192K/24bit��WAV��ʽ.
//////////////////////////////////////////////////////////////////////////////////
#include "vos.h"
#include "ff.h"

#define __packed __attribute__((packed))

#define WAV_I2S_TX_DMA_BUFSIZE    8192		//����WAV TX DMA �����С(����192Kbps@24bit��ʱ��,��Ҫ����8192��Ų��Ῠ)

//RIFF��
typedef struct
{
    u32 ChunkID;		   	//chunk id;����̶�Ϊ"RIFF",��0X46464952
    u32 ChunkSize ;		   	//���ϴ�С;�ļ��ܴ�С-8
    u32 Format;	   			//��ʽ;WAVE,��0X45564157
}__packed ChunkRIFF ;
//fmt��
typedef struct
{
    u32 ChunkID;		   	//chunk id;����̶�Ϊ"fmt ",��0X20746D66
    u32 ChunkSize ;		   	//�Ӽ��ϴ�С(������ID��Size);����Ϊ:20.
    u16 AudioFormat;	  	//��Ƶ��ʽ;0X01,��ʾ����PCM;0X11��ʾIMA ADPCM
	u16 NumOfChannels;		//ͨ������;1,��ʾ������;2,��ʾ˫����;
	u32 SampleRate;			//������;0X1F40,��ʾ8Khz
	u32 ByteRate;			//�ֽ�����;
	u16 BlockAlign;			//�����(�ֽ�);
	u16 BitsPerSample;		//�����������ݴ�С;4λADPCM,����Ϊ4
//	u16 ByteExtraData;		//���ӵ������ֽ�;2��; ����PCM,û���������
}__packed ChunkFMT;
//fact��
typedef __packed struct
{
    u32 ChunkID;		   	//chunk id;����̶�Ϊ"fact",��0X74636166;
    u32 ChunkSize ;		   	//�Ӽ��ϴ�С(������ID��Size);����Ϊ:4.
    u32 NumOfSamples;	  	//����������;
}ChunkFACT;
//LIST��
typedef struct
{
    u32 ChunkID;		   	//chunk id;����̶�Ϊ"LIST",��0X74636166;
    u32 ChunkSize ;		   	//�Ӽ��ϴ�С(������ID��Size);����Ϊ:4.
}__packed ChunkLIST;

//data��
typedef struct
{
    u32 ChunkID;		   	//chunk id;����̶�Ϊ"data",��0X5453494C
    u32 ChunkSize ;		   	//�Ӽ��ϴ�С(������ID��Size)
}__packed ChunkDATA;

//wavͷ
typedef struct
{
	ChunkRIFF riff;	//riff��
	ChunkFMT fmt;  	//fmt��
//	ChunkFACT fact;	//fact�� ����PCM,û������ṹ��
	ChunkDATA data;	//data��
}__packed __WaveHeader;

//wav ���ſ��ƽṹ��
typedef struct
{
    u16 audioformat;			//��Ƶ��ʽ;0X01,��ʾ����PCM;0X11��ʾIMA ADPCM
	u16 nchannels;				//ͨ������;1,��ʾ������;2,��ʾ˫����;
	u16 blockalign;				//�����(�ֽ�);
	u32 datasize;				//WAV���ݴ�С

    u32 totsec ;				//���׸�ʱ��,��λ:��
    u32 cursec ;				//��ǰ����ʱ��

    u32 bitrate;	   			//������(λ��)
	u32 samplerate;				//������
	u16 bps;					//λ��,����16bit,24bit,32bit

	u32 datastart;				//����֡��ʼ��λ��(���ļ������ƫ��)
}__packed __wavctrl;


#define I2S_TX_DMA_BUF_SIZE    10240	//����TX DMA �����С(����192Kbps@24bit��ʱ��,��Ҫ����8192��Ų��Ῠ)


//���ֲ��ſ�����
typedef struct
{
	//2��I2S�����BUF
	u8 *i2sbuf1;
	u8 *i2sbuf2;
	u8 *tbuf;				//��ʱ����,����24bit�����ʱ����Ҫ�õ�
	FIL *file;				//��Ƶ�ļ�ָ��

	u8 status;				//bit0:0,��ͣ����;1,��������
							//bit1:0,��������;1,��������
} __packed __audiodev;
extern __audiodev audiodev;	//���ֲ��ſ�����


u8 wav_decode_init(u8* fname,__wavctrl* wavx);
u32 wav_buffill(u8 *buf,u16 size,u8 bits);
void wav_i2s_dma_tx_callback(void);
u8 wav_play_song(u8* fname);
#endif















