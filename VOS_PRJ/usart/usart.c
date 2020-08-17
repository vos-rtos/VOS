#include "sys.h"
#include "vos.h"
#include "usart.h"

int fputc(int ch, FILE *f)
{ 	
	USART1->SR = (uint16_t)~0x0040; //解决第一个字节发不出来问题
	while((USART1->SR&0X40)==0);
	USART1->DR = (u8) ch;      
	return ch;
}


void uart_init(u32 bound){

	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA,ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1,ENABLE);


	GPIO_PinAFConfig(GPIOA,GPIO_PinSource9,GPIO_AF_USART1);
	GPIO_PinAFConfig(GPIOA,GPIO_PinSource10,GPIO_AF_USART1);
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9 | GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(GPIOA,&GPIO_InitStructure);

	USART_InitStructure.USART_BaudRate = bound;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(USART1, &USART_InitStructure);
	
	USART_Cmd(USART1, ENABLE);
	
	USART_ClearFlag(USART1, USART_FLAG_TC);
	
	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);


	NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority =2;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
}


volatile s32 gUartRxCnts = 0;
volatile s32 gUartRxWrIdx = 0;
volatile s32 gUartRxRdIdx = 0;
u8 gUart1Buf[1024];

void USART1_IRQHandler(void)
{
	u32 irq_save = 0;
	u8 Res;
	VOSIntEnter();
	if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
	{
		Res =USART_ReceiveData(USART1);
		irq_save = __local_irq_save();
		if (gUartRxCnts == sizeof(gUart1Buf)) {
			gUartRxCnts = 0; //覆盖，从头算起
			kprintf("USART1_IRQHandler -----errror\r\n");
		}
		gUart1Buf[gUartRxWrIdx++] = Res;
		gUartRxCnts++;
		gUartRxWrIdx %= sizeof(gUart1Buf);
		__local_irq_restore(irq_save);
	}
	VOSIntExit ();
} 

s32 vgetc(u8 *ch)
{
	s32 ret = 0;
	u32 irq_save;
	irq_save = __vos_irq_save();
	if (gUartRxCnts > 0) {
		*ch = gUart1Buf[gUartRxRdIdx++];
		gUartRxCnts--;
		gUartRxRdIdx %= sizeof(gUart1Buf);
		ret = 1;
	}
	__vos_irq_restore(irq_save);

	return ret;

}
s32 vgets(u8 *buf, s32 len)
{
	s32 copy_len;
	s32 left = 0;
	u32 irq_save;
	irq_save = __vos_irq_save();
	copy_len = gUartRxCnts > len ? len : gUartRxCnts;
	if (copy_len > 0) {
		if (sizeof(gUart1Buf) - gUartRxRdIdx >= copy_len) {
			memcpy(buf, &gUart1Buf[gUartRxRdIdx], copy_len);
			gUartRxRdIdx += copy_len;
		}
		else {
			left = sizeof(gUart1Buf)-gUartRxRdIdx;
			memcpy(buf, &gUart1Buf[gUartRxRdIdx], left);
			copy_len -= left;
			memcpy(&buf[left], &gUart1Buf[0], copy_len);
			gUartRxRdIdx = copy_len;
		}
		gUartRxRdIdx %= sizeof(gUart1Buf);
		gUartRxCnts -= copy_len;
	}
	__vos_irq_restore(irq_save);
	return copy_len;
}



void vputs(s8 *str, s32 len)
{
	u32 irq_save;
	s32 i;
	//irq_save = __vos_irq_save();
	for (i=0; i<len; i++) {
		USART_SendData(USART1, (u8)str[i]);
		while(USART_GetFlagStatus(USART1,USART_FLAG_TC)!=SET);
	}
	//__vos_irq_restore(irq_save);
}


 



