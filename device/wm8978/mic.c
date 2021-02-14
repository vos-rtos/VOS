#include "mic.h"
#include "stm32f4xx_hal.h"
#include "wm8978.h"
#include "i2s.h"
#include "sai.h"
int aaxx = 1;
s32 mic_open(s32 port)
{
	s32 ret = 0;
	WM8978_Init();
	WM8978_HPvol_Set(40,40);
	WM8978_SPKvol_Set(40);

	WM8978_ADDA_Cfg(0,1);		//开启ADC
	WM8978_Input_Cfg(1,1,0);	//开启输入通道(MIC&LINE IN)
	WM8978_Output_Cfg(0,1);		//开启BYPASS输出
	WM8978_MIC_Gain(46);		//MIC增益设置
	//WM8978_LINEIN_Gain(0);
	WM8978_SPKvol_Set(0);		//关闭喇叭.
	WM8978_I2S_Cfg(2,0);		//飞利浦标准,16位数据长度
#ifdef STM32F407xx
	i2s_mode_set(port, MODE_I2S_RECORDER);

	ret = i2s_open(port, I2S_STANDARD_PHILIPS,I2S_MODE_MASTER_TX,I2S_CPOL_LOW,I2S_DATAFORMAT_16B);
	I2S2_SampleRate_Set(16000);	//设置采样率
	DMA1_Stream4->CR &= ~(1<<4);	//关闭传输完成中断(这里不用中断送数据)

	//启动录音接收
	i2s_rx_dma_start(port);
	//启动发送, 要发送，才能接收，spi原理
	i2s_tx_dma_start(port);
#elif defined(STM32F429xx)
	sai_mode_set(port, MODE_SAI_RECORDER);

	ret = sai_open(port, SAI_MODESLAVE_RX, SAI_CLOCKSTROBING_RISINGEDGE, SAI_DATASIZE_16);
	SAIA_SampleRate_Set(0, 16000);
	DMA2_Stream3->CR &= ~(1<<4);	//关闭传输完成中断(这里不用中断送数据)
	//启动录音接收
	sai_rx_dma_start(port);
	//启动发送, 要发送，才能接收，spi原理
	sai_tx_dma_start(port);
#endif
//	while (aaxx) {
//		VOSTaskDelay(5);
//	}

	return ret;
}

s32 mic_recvs(s32 port, u8 *buf, s32 len, u32 timeout_ms)
{
#ifdef STM32F407xx
	return i2s_recvs(port, buf, len, timeout_ms);
#elif defined(STM32F429xx)
	return sai_recvs(port, buf, len, timeout_ms);
#endif
}

s32 mic_ctrl(s32 port, s32 option, void *value, s32 len)
{

}

s32 mic_close(s32 port)
{

}

#include "ff.h"
#include "wm8978.h"
#include "i2s.h"
#include "wavplay.h"

void recoder_wav_init(__WaveHeader* wavhead) //初始化WAV头
{
	wavhead->riff.ChunkID=0X46464952;	//"RIFF"
	wavhead->riff.ChunkSize=0;			//还未确定,最后需要计算
	wavhead->riff.Format=0X45564157; 	//"WAVE"
	wavhead->fmt.ChunkID=0X20746D66; 	//"fmt "
	wavhead->fmt.ChunkSize=16; 			//大小为16个字节
	wavhead->fmt.AudioFormat=0X01; 		//0X01,表示PCM;0X01,表示IMA ADPCM
 	wavhead->fmt.NumOfChannels=2;		//双声道
 	wavhead->fmt.SampleRate=16000;		//16Khz采样率 采样速率
 	wavhead->fmt.ByteRate=wavhead->fmt.SampleRate*4;//字节速率=采样率*通道数*(ADC位数/8)
 	wavhead->fmt.BlockAlign=4;			//块大小=通道数*(ADC位数/8)
 	wavhead->fmt.BitsPerSample=16;		//16位PCM
   	wavhead->data.ChunkID=0X61746164;	//"data"
 	wavhead->data.ChunkSize=0;			//数据大小,还需要计算
}

void mic_test()
{
	s32 wavsize;
	u32 bw;
	static u8 buf[8*1024];
	s32 mark = 0;
	s32 total = 0;
	s32 ret = 0;
	FIL fmp3;
	__WaveHeader *wavhead=0;
 	DIR recdir;	 					//目录
 	u8 *pname=0;

  	while(f_opendir(&recdir,"0:/RECORDER"))//打开录音文件夹
 	{
		VOSTaskDelay(200);
		f_mkdir("0:/RECORDER");				//创建该目录
	}
 	wavhead=(__WaveHeader*)vmalloc(sizeof(__WaveHeader));//开辟__WaveHeader字节的内存区域
	pname=vmalloc(30);						//申请30个字节内存,类似"0:RECORDER/REC00001.wav"

	pname[0]=0;					//pname没有任何文件名


	wavsize = 0;
	vvsprintf((char*)pname, 30, "0:/REC%d.wav",0);
	recoder_wav_init(wavhead);				//初始化wav数据
	ret=f_open(&fmp3,(const TCHAR*)pname, FA_CREATE_ALWAYS | FA_WRITE);
	if(ret==0)
	{
		ret=f_write(&fmp3,(const void*)wavhead,sizeof(__WaveHeader),&bw);//写入头数据
	}

	//mic_open(1);
	mic_open(0);
	u32 mark_time = VOSGetTimeMs();
	kprintf("recorder begin!\r\n");
	while (1) {
		//ret = mic_recvs(1, buf, sizeof(buf), 100);
		ret = mic_recvs(0, buf, sizeof(buf), 100);
		if (ret > 0) {
			total = ret;
			mark = 0;
			while (1) {
				ret = f_write(&fmp3, buf+mark, total-mark, &bw);
				if (ret == 0 && bw > 0) {
					mark += bw;
					wavsize += bw;
				}
				if (mark == total) break;
				VOSTaskDelay(5);
			}
			if (VOSGetTimeMs() - mark_time > 20 * 1000) {//任意键跳出
				kprintf("recorder end!\r\n");
				wavhead->riff.ChunkSize=wavsize+36;		//整个文件的大小-8;
				wavhead->data.ChunkSize=wavsize;		//数据大小
				f_lseek(&fmp3,0);						//偏移到文件头.
				f_write(&fmp3,(const void*)wavhead,sizeof(__WaveHeader),&bw);//写入头数据
				f_close(&fmp3);
				goto END;
			}
		}
	}

END:
	vfree(wavhead);		//释放内存
	vfree(pname);		//释放内存
}
