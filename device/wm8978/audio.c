#include "audio.h"
#include "stm32f4xx_hal.h"
#include "wm8978.h"
#include "i2s.h"

s32 audio_open(s32 port,  s32 data_bits)
{
	WM8978_Init();
	WM8978_HPvol_Set(40,40);
	WM8978_SPKvol_Set(40);

	WM8978_ADDA_Cfg(1,0);
	WM8978_Input_Cfg(0,0,0);
	WM8978_Output_Cfg(1,0);

	if (data_bits == AUDIO_ADC_16BIT) {
		WM8978_I2S_Cfg(2, 0);	//飞利浦标准,16位数据长度I2S_DataFormat_16bextended
		i2s_open(port, I2S_STANDARD_PHILIPS, I2S_MODE_MASTER_TX, I2S_CPOL_LOW, I2S_DATAFORMAT_16B_EXTENDED);
	}
	if (data_bits == AUDIO_ADC_24BIT) {
		WM8978_I2S_Cfg(2, 2);	//飞利浦标准,24位数据长度
		i2s_open(port, I2S_STANDARD_PHILIPS, I2S_MODE_MASTER_TX, I2S_CPOL_LOW, I2S_DATAFORMAT_24B);	//飞利浦标准,主机发送,时钟低电平有效,24位长度
	}
	//wav_play_song("0:/bbb.wav");
}

s32 audio_sends(s32 port, u8 *buf, s32 len, u32 timeout_ms)
{
	return i2s_sends(port, buf, len, timeout_ms);
}

s32 audio_ctrl(s32 port, s32 option, void *value, s32 len)
{
	switch(option) {
	case AUDIO_OPT_AUDIO_SAMPLE:
		I2S2_SampleRate_Set(*(u32*)value);
		break;
	default:
		break;
	}
	return 0;
}

s32 audio_close(s32 port)
{

}
