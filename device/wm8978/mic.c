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

	WM8978_ADDA_Cfg(0,1);		//����ADC
	WM8978_Input_Cfg(1,1,0);	//��������ͨ��(MIC&LINE IN)
	WM8978_Output_Cfg(0,1);		//����BYPASS���
	WM8978_MIC_Gain(46);		//MIC��������
	//WM8978_LINEIN_Gain(0);
	WM8978_SPKvol_Set(0);		//�ر�����.
	WM8978_I2S_Cfg(2,0);		//�����ֱ�׼,16λ���ݳ���
#ifdef STM32F407xx
	i2s_mode_set(port, MODE_I2S_RECORDER);

	ret = i2s_open(port, I2S_STANDARD_PHILIPS,I2S_MODE_MASTER_TX,I2S_CPOL_LOW,I2S_DATAFORMAT_16B);
	I2S2_SampleRate_Set(16000);	//���ò�����
	DMA1_Stream4->CR &= ~(1<<4);	//�رմ�������ж�(���ﲻ���ж�������)

	//����¼������
	i2s_rx_dma_start(port);
	//��������, Ҫ���ͣ����ܽ��գ�spiԭ��
	i2s_tx_dma_start(port);
#elif defined(STM32F429xx)
	sai_mode_set(port, MODE_SAI_RECORDER);

	ret = sai_open(port, SAI_MODESLAVE_RX, SAI_CLOCKSTROBING_RISINGEDGE, SAI_DATASIZE_16);
	SAIA_SampleRate_Set(0, 16000);
	DMA2_Stream3->CR &= ~(1<<4);	//�رմ�������ж�(���ﲻ���ж�������)
	//����¼������
	sai_rx_dma_start(port);
	//��������, Ҫ���ͣ����ܽ��գ�spiԭ��
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

void recoder_wav_init(__WaveHeader* wavhead) //��ʼ��WAVͷ
{
	wavhead->riff.ChunkID=0X46464952;	//"RIFF"
	wavhead->riff.ChunkSize=0;			//��δȷ��,�����Ҫ����
	wavhead->riff.Format=0X45564157; 	//"WAVE"
	wavhead->fmt.ChunkID=0X20746D66; 	//"fmt "
	wavhead->fmt.ChunkSize=16; 			//��СΪ16���ֽ�
	wavhead->fmt.AudioFormat=0X01; 		//0X01,��ʾPCM;0X01,��ʾIMA ADPCM
 	wavhead->fmt.NumOfChannels=2;		//˫����
 	wavhead->fmt.SampleRate=16000;		//16Khz������ ��������
 	wavhead->fmt.ByteRate=wavhead->fmt.SampleRate*4;//�ֽ�����=������*ͨ����*(ADCλ��/8)
 	wavhead->fmt.BlockAlign=4;			//���С=ͨ����*(ADCλ��/8)
 	wavhead->fmt.BitsPerSample=16;		//16λPCM
   	wavhead->data.ChunkID=0X61746164;	//"data"
 	wavhead->data.ChunkSize=0;			//���ݴ�С,����Ҫ����
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
 	DIR recdir;	 					//Ŀ¼
 	u8 *pname=0;

  	while(f_opendir(&recdir,"0:/RECORDER"))//��¼���ļ���
 	{
		VOSTaskDelay(200);
		f_mkdir("0:/RECORDER");				//������Ŀ¼
	}
 	wavhead=(__WaveHeader*)vmalloc(sizeof(__WaveHeader));//����__WaveHeader�ֽڵ��ڴ�����
	pname=vmalloc(30);						//����30���ֽ��ڴ�,����"0:RECORDER/REC00001.wav"

	pname[0]=0;					//pnameû���κ��ļ���


	wavsize = 0;
	vvsprintf((char*)pname, 30, "0:/REC%d.wav",0);
	recoder_wav_init(wavhead);				//��ʼ��wav����
	ret=f_open(&fmp3,(const TCHAR*)pname, FA_CREATE_ALWAYS | FA_WRITE);
	if(ret==0)
	{
		ret=f_write(&fmp3,(const void*)wavhead,sizeof(__WaveHeader),&bw);//д��ͷ����
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
			if (VOSGetTimeMs() - mark_time > 20 * 1000) {//���������
				kprintf("recorder end!\r\n");
				wavhead->riff.ChunkSize=wavsize+36;		//�����ļ��Ĵ�С-8;
				wavhead->data.ChunkSize=wavsize;		//���ݴ�С
				f_lseek(&fmp3,0);						//ƫ�Ƶ��ļ�ͷ.
				f_write(&fmp3,(const void*)wavhead,sizeof(__WaveHeader),&bw);//д��ͷ����
				f_close(&fmp3);
				goto END;
			}
		}
	}

END:
	vfree(wavhead);		//�ͷ��ڴ�
	vfree(pname);		//�ͷ��ڴ�
}
